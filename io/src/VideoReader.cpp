#include "VideoReader.h"

VideoReader::VideoReader(const std::string& path) : capture_(path) {}

bool VideoReader::isOpened() const {
    return capture_.isOpened();
}

bool VideoReader::read(cv::Mat& frame) {
    return capture_.read(frame);
}
