#ifndef ARMOR_DETECTOR_H
#define ARMOR_DETECTOR_H

#include "ArmorTypes.h"
#include <opencv2/dnn.hpp>
#include <fstream>
#include <mutex>
#include <ostream>

class ArmorDetector {
public:
    ArmorDetector(
        const std::string &model_path, const std::string &label_path, const double threshold,
        const std::vector<std::string> &ignore_classes = {});

    // 五个阶段可分别调用，调用顺序见 start.cpp。
    LightBlobs FindLight(Mat &src, int enemy_color);
    Armors FindArmor(Mat &src, LightBlobs &light_blobs);
    void ExtractNumbers(const Mat &src, Armors &armors);
    void Classify(Armors &armors);
    void WriteFrameResult(std::ostream &out, std::size_t frame_number, const Armors &armors) const;

private:

    // 检测参数
    int enemy_color_;
    int threshold_;

    // 分类参数
    cv::dnn::Net net_;
    std::vector<std::string> class_names_;
    std::vector<std::string> ignore_classes_;
    double classify_threshold_;
    std::mutex mutex_;
};

#endif
