#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

if ! command -v c++ >/dev/null 2>&1 || ! command -v cmake >/dev/null 2>&1 || ! command -v pkg-config >/dev/null 2>&1 || ! pkg-config --exists opencv4; then
    if ! command -v apt-get >/dev/null 2>&1; then
        echo "缺少 C++ 编译器、CMake、pkg-config 或 OpenCV 4；请先通过系统包管理器安装。" >&2
        exit 1
    fi
    if (( EUID == 0 )); then
        apt-get update
        apt-get install -y build-essential cmake pkg-config libopencv-dev
    else
        sudo apt-get update
        sudo apt-get install -y build-essential cmake pkg-config libopencv-dev
    fi
fi

cmake -S "$project_dir" -B "$project_dir/build"
cmake --build "$project_dir/build" -j "$(nproc)"
echo "完成。运行: cd \"$project_dir\" && ./build/start video/3.avi"
echo "或在 build 目录运行: ./start ../video/3.avi red"
