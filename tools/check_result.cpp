// 结果文件自检工具：校验格式/行数/坐标，并可把 txt 中的四点反渲染回视频帧。
// 用法:
//   check_result <结果.txt> <视频> [--render-every N]
#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>

namespace {

struct ArmorRecord {
    std::string number;
    std::vector<cv::Point2f> points;
};

struct FrameRecord {
    int frame = 0;
    std::vector<ArmorRecord> armors;
};

const std::regex kLineRe(R"(^(\d+), count=(\d+)(.*)$)");
const std::regex kArmorRe(
    R"(armor\d+: number=([^,]+), points=\(([-\d.]+),([-\d.]+)\),\(([-\d.]+),([-\d.]+)\),\(([-\d.]+),([-\d.]+)\),\(([-\d.]+),([-\d.]+)\))");

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "用法: " << argv[0] << " <结果.txt> <视频> [--render-every N]\n";
        return 1;
    }
    const std::string txt_path = argv[1];
    const std::string video_path = argv[2];
    int render_every = 0;
    for (int i = 3; i + 1 < argc; ++i)
        if (std::string(argv[i]) == "--render-every") render_every = std::stoi(argv[i + 1]);

    cv::VideoCapture cap(video_path);
    if (!cap.isOpened()) {
        std::cerr << "无法打开视频: " << video_path << '\n';
        return 1;
    }
    const int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    const int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    const int total_frames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));

    std::ifstream in(txt_path);
    if (!in) {
        std::cerr << "无法打开结果文件: " << txt_path << '\n';
        return 1;
    }

    std::vector<FrameRecord> records;
    std::vector<std::string> errors;
    std::string line;
    int line_no = 0;
    std::map<std::string, int> number_hist;

    while (std::getline(in, line)) {
        ++line_no;
        std::smatch m;
        if (!std::regex_match(line, m, kLineRe)) {
            errors.push_back("line " + std::to_string(line_no) + ": 无法解析: " + line);
            continue;
        }
        FrameRecord rec;
        rec.frame = std::stoi(m[1].str());
        const int declared = std::stoi(m[2].str());
        const std::string rest = m[3].str();

        for (auto it = std::sregex_iterator(rest.begin(), rest.end(), kArmorRe);
             it != std::sregex_iterator(); ++it) {
            ArmorRecord armor;
            armor.number = (*it)[1].str();
            for (int k = 0; k < 4; ++k)
                armor.points.emplace_back(std::stof((*it)[2 + 2 * k].str()),
                                          std::stof((*it)[3 + 2 * k].str()));
            ++number_hist[armor.number];
            static const std::vector<std::string> kAllowed = {
                "1", "2", "3", "4", "outpost", "guard", "base", "unknown"};
            if (std::find(kAllowed.begin(), kAllowed.end(), armor.number) == kAllowed.end())
                errors.push_back("line " + std::to_string(line_no) + ": number 不在允许集合内: " +
                                 armor.number);
            rec.armors.push_back(armor);
        }

        if (static_cast<int>(rec.armors.size()) != declared)
            errors.push_back("line " + std::to_string(line_no) + ": count=" +
                             std::to_string(declared) + " 但解析到 " +
                             std::to_string(rec.armors.size()) + " 块装甲板");
        records.push_back(rec);
    }

    if (static_cast<int>(records.size()) != total_frames)
        errors.push_back("行数 " + std::to_string(records.size()) + " != 视频帧数 " +
                         std::to_string(total_frames));

    std::size_t total_armors = 0;
    std::size_t frames_with_armor = 0;
    int max_count = 0;
    int longest_zero = 0;
    int current_zero = 0;
    int zero_start = -1;
    int longest_zero_start = -1;
    int max_jump = 0;
    int max_jump_frame = -1;

    for (std::size_t i = 0; i < records.size(); ++i) {
        const auto& rec = records[i];
        if (i > 0) {
            const int jump = std::abs(static_cast<int>(rec.armors.size()) -
                                      static_cast<int>(records[i - 1].armors.size()));
            if (jump > max_jump) {
                max_jump = jump;
                max_jump_frame = rec.frame;
            }
        }
        if (rec.frame != static_cast<int>(i))
            errors.push_back("第 " + std::to_string(i + 1) + " 行的帧号是 " +
                             std::to_string(rec.frame) + "，应为 " + std::to_string(i));
        if (!rec.armors.empty()) ++frames_with_armor;
        total_armors += rec.armors.size();
        max_count = std::max(max_count, static_cast<int>(rec.armors.size()));

        if (rec.armors.empty()) {
            if (current_zero == 0) zero_start = rec.frame;
            ++current_zero;
            if (current_zero > longest_zero) {
                longest_zero = current_zero;
                longest_zero_start = zero_start;
            }
        } else {
            current_zero = 0;
        }

        for (const auto& armor : rec.armors) {
            if (armor.points.size() != 4)
                errors.push_back("帧 " + std::to_string(rec.frame) + ": 点数不是 4");
            for (const auto& p : armor.points) {
                if (p.x < 0 || p.y < 0 || p.x > width || p.y > height)
                    errors.push_back("帧 " + std::to_string(rec.frame) + ": 坐标越界 (" +
                                     std::to_string(p.x) + "," + std::to_string(p.y) + ")");
            }
            const auto& p = armor.points;
            const double area = std::fabs((p[0].x * p[1].y - p[1].x * p[0].y) +
                                          (p[1].x * p[2].y - p[2].x * p[1].y) +
                                          (p[2].x * p[3].y - p[3].x * p[2].y) +
                                          (p[3].x * p[0].y - p[0].x * p[3].y)) * 0.5;
            if (area < 1.0)
                errors.push_back("帧 " + std::to_string(rec.frame) + ": 四点退化（面积 0）");
        }
    }

    std::cout << "file=" << txt_path << " video=" << video_path << '\n'
              << "video_frames=" << total_frames << " lines=" << records.size() << '\n'
              << "frames_with_armor=" << frames_with_armor << " ("
              << (records.empty() ? 0.0 : 100.0 * frames_with_armor / records.size()) << "%)\n"
              << "total_armors=" << total_armors << " mean_count="
              << (records.empty() ? 0.0 : static_cast<double>(total_armors) / records.size())
              << " max_count=" << max_count << '\n'
              << "longest_zero_run=" << longest_zero << " (from frame " << longest_zero_start
              << ")\n"
              << "max_adjacent_count_jump=" << max_jump << " (at frame " << max_jump_frame
              << ")\n";

    std::cout << "numbers:";
    for (const auto& kv : number_hist) std::cout << ' ' << kv.first << '=' << kv.second;
    std::cout << '\n';

    if (render_every > 0) {
        std::filesystem::create_directories("verify");
        const std::string stem = std::filesystem::path(video_path).stem().string();
        int saved = 0;
        for (const auto& rec : records) {
            // 空帧也渲染，这样漏检在抽检图里同样可见
            if (rec.frame % render_every != 0) continue;
            cap.set(cv::CAP_PROP_POS_FRAMES, rec.frame);
            cv::Mat img;
            if (!cap.read(img)) continue;
            const cv::Mat original = img.clone();
            // 角点颜色：0 红(左下) 1 绿(左上) 2 蓝(右上) 3 黄(右下)
            const cv::Scalar point_colors[4] = {cv::Scalar(0, 0, 255), cv::Scalar(0, 255, 0),
                                                cv::Scalar(255, 0, 0), cv::Scalar(0, 255, 255)};
            int inset_x = 10;
            for (const auto& armor : rec.armors) {
                std::vector<cv::Point> poly;
                for (const auto& p : armor.points) poly.emplace_back(cv::Point(cvRound(p.x), cvRound(p.y)));
                cv::polylines(img, poly, true, cv::Scalar(0, 255, 0), 2);
                for (int k = 0; k < 4; ++k) cv::circle(img, poly[k], 4, point_colors[k], -1);
                cv::putText(img, armor.number, poly[1] + cv::Point(0, -8), cv::FONT_HERSHEY_SIMPLEX,
                            0.8, cv::Scalar(0, 255, 0), 2);

                // 左上角贴一张 4 倍放大的局部图，便于核对贴合度与角点顺序
                cv::Rect box = cv::boundingRect(poly);
                const int pad = std::max(box.width, box.height) / 2 + 10;
                box = cv::Rect(box.x - pad, box.y - pad, box.width + 2 * pad, box.height + 2 * pad) &
                      cv::Rect(0, 0, original.cols, original.rows);
                if (box.area() <= 0) continue;
                cv::Mat crop = original(box).clone();
                const double scale = 4.0;
                cv::resize(crop, crop, cv::Size(), scale, scale, cv::INTER_LINEAR);
                std::vector<cv::Point> local;
                for (const auto& p : armor.points)
                    local.emplace_back(cvRound((p.x - box.x) * scale), cvRound((p.y - box.y) * scale));
                cv::polylines(crop, local, true, cv::Scalar(0, 255, 0), 1);
                for (int k = 0; k < 4; ++k) cv::circle(crop, local[k], 5, point_colors[k], -1);
                if (inset_x + crop.cols > img.cols || 60 + crop.rows > img.rows) continue;
                crop.copyTo(img(cv::Rect(inset_x, 60, crop.cols, crop.rows)));
                cv::rectangle(img, cv::Rect(inset_x, 60, crop.cols, crop.rows), cv::Scalar(255, 255, 255), 1);
                inset_x += crop.cols + 10;
            }
            char header[96];
            std::snprintf(header, sizeof(header), "frame %d  count=%zu", rec.frame, rec.armors.size());
            cv::putText(img, header, cv::Point(10, 40), cv::FONT_HERSHEY_SIMPLEX, 1.0,
                        cv::Scalar(255, 255, 255), 2);
            char name[128];
            std::snprintf(name, sizeof(name), "verify/%s_f%05d.png", stem.c_str(), rec.frame);
            cv::imwrite(name, img);
            ++saved;
        }
        std::cout << "rendered=" << saved << " -> verify/\n";
    }

    if (errors.empty()) {
        std::cout << "RESULT: PASS\n";
        return 0;
    }
    std::cout << "RESULT: FAIL (" << errors.size() << " errors)\n";
    for (std::size_t i = 0; i < errors.size() && i < 20; ++i) std::cout << "  " << errors[i] << '\n';
    return 1;
}
