#pragma once

#include <opencv2/core/mat.hpp>
#include <opencv2/videoio.hpp>
#include <string>

class VideoReader {
public:
    explicit VideoReader(const std::string& path);

    bool isOpened() const;
    bool read(cv::Mat& frame);

private:
    cv::VideoCapture capture_;
};
