cmake_minimum_required(VERSION 3.16)
project(video_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core highgui videoio imgproc imgcodecs dnn)

add_executable(start start.cpp io/src/VideoReader.cpp Armor/src/ArmorDetector.cpp)
target_include_directories(start PRIVATE io/include Armor/include ${OpenCV_INCLUDE_DIRS})
target_link_libraries(start PRIVATE ${OpenCV_LIBS})
target_compile_definitions(start PRIVATE MODEL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/model")

if (MSVC)
    target_compile_options(start PRIVATE /utf-8)
endif()

add_executable(check_result tools/check_result.cpp)
target_include_directories(check_result PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(check_result PRIVATE ${OpenCV_LIBS})
if (MSVC)
    target_compile_options(check_result PRIVATE /utf-8)
endif()
