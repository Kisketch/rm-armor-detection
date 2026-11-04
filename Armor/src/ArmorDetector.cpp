#include "ArmorDetector.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <opencv2/highgui.hpp>
#include <string>

// ==================== 检测参数 ====================

namespace {

// 灯条筛选
constexpr float kMinBarLength = 8.0f;    // 灯条长边下限（像素）
constexpr float kMinBarAspect = 1.6f;    // 长宽比下限
constexpr float kMaxBarAspect = 12.0f;   // 长宽比上限
constexpr float kMinFillRatio = 0.40f;   // 轮廓面积 / 最小外接矩形面积
constexpr float kMaxTiltDeg = 40.0f;     // 长轴相对竖直方向的最大倾角

// 装甲板配对
constexpr float kMaxAngleDiff = 12.0f;    // 两灯条长轴夹角上限（度）
constexpr float kMaxLengthRatio = 1.7f;   // 两灯条长度比上限
constexpr float kMaxCenterYDiff = 0.6f;   // 中心 y 差 / 平均灯条长度
constexpr float kMinGapRatio = 0.15f;     // 灯条内间距 / 平均灯条长度
constexpr float kMaxGapRatio = 3.2f;
constexpr float kMinPlateRatio = 1.1f;    // 装甲板宽高比下限
constexpr float kMaxPlateRatio = 5.0f;
constexpr float kLargeArmorRatio = 3.0f;  // 超过该宽高比判为大装甲

// 灯条长轴与水平方向的夹角，归一到 [0, 180)。竖直灯条约为 90。
float LongAxisAngleDeg(const cv::RotatedRect& rect) {
    cv::Point2f p[4];
    rect.points(p);
    const float e01 = static_cast<float>(cv::norm(p[0] - p[1]));
    const float e12 = static_cast<float>(cv::norm(p[1] - p[2]));
    const cv::Point2f axis = (e01 >= e12) ? (p[1] - p[0]) : (p[2] - p[1]);
    float angle = std::atan2(axis.y, axis.x) * 180.0f / static_cast<float>(CV_PI);
    angle = std::fmod(angle + 180.0f, 180.0f);
    return angle;
}

float BarThickness(const cv::RotatedRect& rect) {
    return std::min(rect.size.width, rect.size.height);
}

}  // namespace

// ==================== ArmorDetector 成员函数实现 ====================

// ---- 灯条检测 ----

LightBlobs ArmorDetector::FindLight(Mat& src, int enemy_color) {
    LightBlobs light_blobs;
    enemy_color_ = enemy_color;

    // BGR 通道：0=B 1=G 2=R。红灯的判别量取 R-B，蓝灯取 B-R，
    // 比 HSV 更稳：灯条中心过曝成白/黄时 B 仍然很低，色相会漂移。
    std::vector<cv::Mat> bgr;
    cv::split(src, bgr);

    cv::Mat diff;
    if (enemy_color == ENEMY_RED) {
        cv::subtract(bgr[2], bgr[0], diff);
    } else {
        cv::subtract(bgr[0], bgr[2], diff);
    }

    cv::Mat mask;
    cv::threshold(diff, mask, threshold_, 255, cv::THRESH_BINARY);

    const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);

    // 调试：ARMOR_SAVE_MASK=<起始帧号> 时保存该帧起连续 6 张掩码到 debug/
    static int mask_dump_index = 0;
    const char* mask_dump_from = std::getenv("ARMOR_SAVE_MASK");
    if (mask_dump_from != nullptr) {
        const int from = std::atoi(mask_dump_from);
        if (mask_dump_index >= from && mask_dump_index < from + 6) {
            std::filesystem::create_directories("debug");
            char name[64];
            std::snprintf(name, sizeof(name), "debug/mask_%05d.png", mask_dump_index);
            cv::imwrite(name, mask);
        }
    }
    ++mask_dump_index;

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& contour : contours) {
        const double area = cv::contourArea(contour);
        if (area < 12.0) continue;

        cv::RotatedRect rect = cv::minAreaRect(contour);
        const float w = rect.size.width;
        const float h = rect.size.height;
        if (w < 1.5f || h < 1.5f) continue;

        const float length = std::max(w, h);
        const float thickness = std::min(w, h);
        if (length < kMinBarLength) continue;

        const float aspect = length / thickness;
        if (aspect < kMinBarAspect || aspect > kMaxBarAspect) continue;
        if (area / static_cast<double>(w * h) < kMinFillRatio) continue;
        if (std::fabs(LongAxisAngleDeg(rect) - 90.0f) > kMaxTiltDeg) continue;

        light_blobs.emplace_back(rect, enemy_color);
    }

    // 误检过多时按长度截断，避免配对阶段 O(n^2) 失控
    if (light_blobs.size() > 40) {
        std::partial_sort(light_blobs.begin(), light_blobs.begin() + 40, light_blobs.end(),
                          [](const LightBlob& a, const LightBlob& b) { return a.length > b.length; });
        light_blobs.resize(40);
    }

    return light_blobs;
}

// ---- 装甲板匹配 ----

Armors ArmorDetector::FindArmor(cv::Mat& src, LightBlobs& light_blobs) {
    Armors armors;
    if (light_blobs.size() < 2) return armors;

    struct Candidate {
        Armor armor;
        float score;
        float width;
    };
    std::vector<Candidate> candidates;

    for (std::size_t i = 0; i < light_blobs.size(); ++i) {
        for (std::size_t j = i + 1; j < light_blobs.size(); ++j) {
            const LightBlob& first = light_blobs[i];
            const LightBlob& second = light_blobs[j];

            // 1) 两灯条近似平行
            float angle_diff =
                std::fabs(LongAxisAngleDeg(first.rect) - LongAxisAngleDeg(second.rect));
            if (angle_diff > 90.0f) angle_diff = 180.0f - angle_diff;
            if (angle_diff > kMaxAngleDiff) continue;

            // 2) 长度接近
            const float len_max = std::max(first.length, second.length);
            const float len_min = std::min(first.length, second.length);
            if (len_min < 1.0f || len_max / len_min > kMaxLengthRatio) continue;

            // 3) 左右关系与高度对齐
            const LightBlob* left = &first;
            const LightBlob* right = &second;
            if (left->center.x > right->center.x) std::swap(left, right);
            const float dx = right->center.x - left->center.x;
            const float dy = std::fabs(right->center.y - left->center.y);
            if (dx <= 0.0f) continue;
            const float avg_len = (left->length + right->length) * 0.5f;
            if (dy > kMaxCenterYDiff * avg_len) continue;

            // 4) 间距与整体宽高比
            const float thickness = (BarThickness(left->rect) + BarThickness(right->rect)) * 0.5f;
            const float gap = dx - thickness;
            if (gap < kMinGapRatio * avg_len || gap > kMaxGapRatio * avg_len) continue;
            const float plate_width = dx + thickness;
            const float plate_ratio = plate_width / avg_len;
            if (plate_ratio < kMinPlateRatio || plate_ratio > kMaxPlateRatio) continue;

            // 5) 用两条灯条长轴的上下端点拼成装甲板四点
            //    顺序：0=左下 1=左上 2=右上 3=右下（与 ExtractNumbers 的 warp 目标一致）
            //    LightBlob 构造时已按 y 求出 top/bottom（长轴两端中点）；灯条倾角限制在 ±40°
            //    以内，所以 top 一定在上。不要用“8 个角点按 x 排序取外侧”的做法：灯条细且
            //    稍有倾斜时，最左两个角点可能都是上端角点，四边形自交，数字图被扭成乱码。
            Armor armor;
            armor.points = {left->bottom, left->top, right->top, right->bottom};
            armor.type = plate_ratio >= kLargeArmorRatio ? ArmorType::LARGE : ArmorType::SMALL;
            armor.center =
                (armor.points[0] + armor.points[1] + armor.points[2] + armor.points[3]) * 0.25f;
            armor.yaw = std::atan2(armor.points[2].y - armor.points[1].y,
                                   armor.points[2].x - armor.points[1].x);
            armor.predict_yaw = armor.yaw;
            armor.confidence = 0.0f;
            armor.number.clear();

            // 分数越小越好：夹角、长度比、高度差都归一化到 ~[0,1]
            const float score = angle_diff / kMaxAngleDiff +
                                (len_max / len_min - 1.0f) / (kMaxLengthRatio - 1.0f) +
                                dy / (kMaxCenterYDiff * avg_len);
            candidates.push_back({armor, score, plate_width});
        }
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b) { return a.score < b.score; });

    for (const auto& candidate : candidates) {
        bool duplicate = false;
        for (const auto& kept : armors) {
            const cv::Point2f delta = kept.center - candidate.armor.center;
            if (std::sqrt(delta.x * delta.x + delta.y * delta.y) < 0.5f * candidate.width) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) armors.push_back(candidate.armor);
    }

    return armors;
}

// ---- 数字分类 (原 NumberClassifier) ----

void ArmorDetector::ExtractNumbers(const cv::Mat& src, std::vector<Armor>& armors) {
    /*
    1---2
    |   |
    0---3
    */

    // Light length in image
    const int light_length = 12;
    // Image size after warp
    const int warp_height = 28;
    const int small_armor_width = 32;
    const int large_armor_width = 52;
    // Number ROI size
    const cv::Size roi_size(28, 28);
    const cv::Size input_size(28, 28);
    for (auto& armor : armors) {
        // Warp perspective transform
        cv::Point2f lights_vertices[4] = {armor.points[0], armor.points[1], armor.points[2],
                                          armor.points[3]};

        const int top_light_y = (warp_height - light_length) / 2 - 1;
        const int bottom_light_y = top_light_y + light_length;
        const int warp_width =
            armor.type == ArmorType::SMALL ? small_armor_width : large_armor_width;
        string big_or_small = ARMOR_TYPE_STR[static_cast<int>(armor.type)];
        // cout<<"ARMOR TYPE:"<<big_or_small<<endl;
        cv::Point2f target_vertices[4] = {
            cv::Point(0, bottom_light_y),
            cv::Point(0, top_light_y),
            cv::Point(warp_width - 1, top_light_y),
            cv::Point(warp_width - 1, bottom_light_y),
        };
        cv::Mat number_image;
        auto rotation_matrix = cv::getPerspectiveTransform(lights_vertices, target_vertices);
        cv::warpPerspective(src, number_image, rotation_matrix, cv::Size(warp_width, warp_height));

        // Get ROI
        number_image =
            number_image(cv::Rect(cv::Point((warp_width - roi_size.width) / 2, 0), roi_size));

        // Binarize
        cv::cvtColor(number_image, number_image, cv::COLOR_RGB2GRAY);
        // cv::threshold(number_image, number_image, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

        /*
        * gray_image: 这是输入的灰度图像，即待处理的图像。
        * binary_image: 这是输出的二值化图像，函数将在此处存储结果。
        * 255:
        这是输出的最大像素值。在二值化过程中，大于阈值的像素将被设置为这个值（在这里是255），小于等于阈值的像素将被设置为0。
        * cv::ADAPTIVE_THRESH_GAUSSIAN_C:
        这是自适应阈值化的方法之一。它表示使用高斯加权求和的方法计算邻域的加权平均值作为阈值。
        * cv::THRESH_BINARY:
        这是阈值类型。在这个例子中，表示将大于阈值的像素设置为最大值（255），小于等于阈值的像素设置为0。
        * 11: 这是阈值化计算中使用的邻域大小（block
        size）。它指定了在每个像素周围多大的邻域内计算局部阈值。通常选择一个奇数，比如 3、5、7、11
        等。
        * 2:
        这是从邻域均值或加权均值中减去的常数（C）。它可以用来微调阈值的结果，以适应不同的图像对比度和光照变化。
        调整建议：
        === blockSize
        (第六个参数)：增大该值会使得阈值的计算更加平滑，适用于更大尺寸的图像区域。但是也会导致细节信息丢失，因此需要根据具体情况选择一个合适的值。
        === C
        (第七个参数)：增大该值会使得阈值升高，这意味着更多的像素可能会被视为背景或非目标物体。减小该值则会降低阈值，这样更多的像素会被视为目标物体。
        */
        cv::adaptiveThreshold(number_image, number_image, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                              cv::THRESH_BINARY, 19, -3);

        cv::resize(number_image, number_image, input_size);
        // Set the top 2 rows and bottom 2 rows to black (value 0)
        // number_image.rowRange(0, 2).setTo(cv::Scalar(0));  // Top 2 rows
        // number_image.rowRange(number_image.rows - 2, number_image.rows).setTo(cv::Scalar(0));  //
        // Bottom 2 rows

        armor.number_img = number_image;
    }
}
void ArmorDetector::Classify(std::vector<Armor>& armors) {
    for (auto& armor : armors) {
        // Normalize
        cv::Mat input = armor.number_img / 255.0;
        // Create blob from image
        cv::Mat blob;
        cv::dnn::blobFromImage(input, blob);

        // Set the input blob for the neural network
        mutex_.lock();
        net_.setInput(blob);

        // Forward pass the image blob through the model
        cv::Mat outputs = net_.forward().clone();
        mutex_.unlock();

        //  Do softmax
        float max_prob = *std::max_element(outputs.begin<float>(), outputs.end<float>());
        cv::Mat softmax_prob;
        cv::exp(outputs - max_prob, softmax_prob);
        float sum = static_cast<float>(cv::sum(softmax_prob)[0]);
        softmax_prob /= sum;

        // Decode the output
        double confidence;
        cv::Point class_id_point;
        minMaxLoc(softmax_prob.reshape(1, 1), nullptr, &confidence, nullptr, &class_id_point);
        int label_id = class_id_point.x;

        armor.confidence = confidence;
        armor.number = class_names_[label_id];
    }
    armors.erase(std::remove_if(armors.begin(), armors.end(),
                                [this](const Armor& armor) {
                                    bool mismatch_armor_type = false;
                                    // 检查是否应该忽略这个类别
                                    if (armor.confidence < classify_threshold_) {
                                        return true;
                                    }

                                    for (const auto& ignore_class : ignore_classes_) {
                                        if (armor.number == ignore_class) {
                                            return true;
                                        }
                                    }

#ifdef SENTRY
                                    if (armor.type == ArmorType::LARGE) {
                                        mismatch_armor_type =
                                            armor.number == "outpost" || armor.number == "guard";
                                    } else if (armor.type == ArmorType::SMALL) {
                                        mismatch_armor_type = armor.number == "1" ||
                                                              armor.number == "base" ||
                                                              armor.number == "guard";
                                    }
#else
                if (armor.type == ArmorType::LARGE)
                {
                    mismatch_armor_type =
                        armor.number == "outpost" || armor.number == "guard" || armor.number == "3" || armor.number == "4" || armor.number == "base";
                }
                else if (armor.type == ArmorType::SMALL)
                {
                    mismatch_armor_type = armor.number == "1" || armor.number == "base";
                }
#endif
                                    return mismatch_armor_type;
                                }),
                 armors.end());
}

// ---- 构造函数 (合并 Tradition + NumberClassifier) ----

void ArmorDetector::WriteFrameResult(std::ostream& out, std::size_t frame_number,
                                     const Armors& armors) const {
    // 格式：<帧号>, count=N[, armor1: number=X, points=(x,y),(x,y),(x,y),(x,y)][ | armor2: ...]
    out << frame_number << ", count=" << armors.size();
    for (std::size_t i = 0; i < armors.size(); ++i) {
        out << (i == 0 ? ", " : " | ");
        out << "armor" << (i + 1) << ": number="
            << (armors[i].number.empty() ? "unknown" : armors[i].number) << ", points=";
        for (std::size_t j = 0; j < armors[i].points.size(); ++j) {
            if (j != 0) out << ',';
            out << '(' << std::fixed << std::setprecision(2) << armors[i].points[j].x << ','
                << armors[i].points[j].y << ')';
        }
    }
    out << '\n';
}

ArmorDetector::ArmorDetector(const std::string& model_path, const std::string& label_path,
                             const double threshold, const std::vector<std::string>& ignore_classes)
    : ignore_classes_(ignore_classes), classify_threshold_(threshold) {
    // 原Tradition部分
    enemy_color_ = 0;
    threshold_ = 70;  // 灯条通道差阈值（R-B / B-R），暗光场景下 70 起步
    // 原NumberClassifier部分
    net_ = cv::dnn::readNetFromONNX(model_path);

    std::ifstream label_file(label_path);
    std::string line;
    while (std::getline(label_file, line)) {
        class_names_.push_back(line);
    }
}
