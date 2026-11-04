#include "VideoReader.h"
#include "ArmorDetector.h"

#include <opencv2/highgui.hpp>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    const auto fail = [](const std::string& message) {
        std::cerr << message << '\n';
        return 1;
    };

    std::string path;
    std::string color = "red";
    bool headless = false;
    bool verbose = false;
    int save_every = 0;
    long max_frames = -1;
    std::vector<std::size_t> save_list;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--headless") {
            headless = true;
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--save-list" && i + 1 < argc) {
            std::string list = argv[++i];
            std::size_t pos = 0;
            while (pos <= list.size()) {
                const std::size_t comma = list.find(',', pos);
                const std::string token = list.substr(pos, comma - pos);
                if (!token.empty()) save_list.push_back(static_cast<std::size_t>(std::stoul(token)));
                if (comma == std::string::npos) break;
                pos = comma + 1;
            }
        } else if (arg == "--save-every" && i + 1 < argc) {
            save_every = std::stoi(argv[++i]);
        } else if (arg == "--max-frames" && i + 1 < argc) {
            max_frames = std::stol(argv[++i]);
        } else if (arg == "red" || arg == "blue") {
            color = arg;
        } else if (path.empty()) {
            path = arg;
        } else {
            return fail("无法识别的参数: " + arg);
        }
    }
    if (path.empty())
        return fail(std::string("用法: ") + argv[0] +
                    " <视频路径> [red|blue] [--headless] [--save-every N] [--max-frames N]\n"
                    "示例: ./start video/3.avi red --headless");

    VideoReader reader(path);
    if (!reader.isOpened()) return fail("无法打开视频: " + path);

    ArmorDetector detector(std::string(MODEL_DIR) + "/mlp.onnx",
                           std::string(MODEL_DIR) + "/label.txt", 0.7, {"negative"});

    const std::string stem = std::filesystem::path(path).stem().string();
    const std::string result_path = stem + ".txt";
    std::ofstream result_file(result_path, std::ios::trunc);
    if (!result_file) return fail("无法创建结果文件: " + result_path);

    if (save_every > 0) std::filesystem::create_directories("debug");

    const auto started = std::chrono::steady_clock::now();
    cv::Mat frame;
    std::size_t frame_number = 0;
    std::size_t total_armors = 0;

    while (reader.read(frame)) {
        auto lights = detector.FindLight(frame, color == "red" ? ENEMY_RED : ENEMY_BLUE);
        auto armors = detector.FindArmor(frame, lights);
        const std::size_t matched = armors.size();
        detector.ExtractNumbers(frame, armors);
        detector.Classify(armors);
        if (verbose && frame_number % 25 == 0)
            std::cerr << "frame=" << frame_number << " lights=" << lights.size()
                      << " matched=" << matched << " final=" << armors.size() << '\n';
        detector.WriteFrameResult(result_file, frame_number, armors);
        total_armors += armors.size();

        for (const auto& armor : armors) {
            for (std::size_t i = 0; i < armor.points.size(); ++i) {
                cv::line(frame, armor.points[i], armor.points[(i + 1) % armor.points.size()],
                         cv::Scalar(0, 255, 0), 2);
            }
            // 四个角点用不同颜色标出，便于核对顺序：0 红 1 绿 2 蓝 3 黄
            const cv::Scalar point_colors[4] = {
                cv::Scalar(0, 0, 255), cv::Scalar(0, 255, 0), cv::Scalar(255, 0, 0),
                cv::Scalar(0, 255, 255)};
            for (std::size_t i = 0; i < armor.points.size() && i < 4; ++i)
                cv::circle(frame, armor.points[i], 3, point_colors[i], -1);

            if (!armor.points.empty()) {
                const std::string label =
                    armor.number.empty() ? ARMOR_TYPE_STR[static_cast<int>(armor.type)] : armor.number;
                cv::putText(frame, label, armor.points[1], cv::FONT_HERSHEY_SIMPLEX, 0.8,
                            cv::Scalar(0, 255, 0), 2);
            }
        }

        if (save_every > 0 && frame_number % static_cast<std::size_t>(save_every) == 0) {
            char name[128];
            std::snprintf(name, sizeof(name), "debug/%s_f%05zu.png", stem.c_str(), frame_number);
            cv::imwrite(name, frame);
        }
        if (!save_list.empty() &&
            std::find(save_list.begin(), save_list.end(), frame_number) != save_list.end()) {
            char name[128];
            std::snprintf(name, sizeof(name), "debug/%s_f%05zu.png", stem.c_str(), frame_number);
            cv::imwrite(name, frame);
        }

        ++frame_number;
        if (frame_number % 100 == 0) std::cerr << "  processed " << frame_number << " frames\r";

        if (!headless) {
            cv::imshow("video", frame);
            if (cv::waitKey(1) == 27) break;  // Esc 退出
        }
        if (max_frames > 0 && frame_number >= static_cast<std::size_t>(max_frames)) break;
    }

    if (!result_file) return fail("写入结果文件失败");
    result_file.close();
    if (!headless) cv::destroyAllWindows();

    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started);
    std::cerr << "\n";
    std::cout << "video=" << stem << " frames=" << frame_number << " armors=" << total_armors
              << " elapsed=" << elapsed.count() << "s"
              << " fps=" << (frame_number / elapsed.count()) << std::endl;
    return 0;
}
