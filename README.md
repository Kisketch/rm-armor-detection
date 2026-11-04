# 装甲板检测（RM 自瞄考核）

读取视频，检测敌方装甲板：灯条查找 → 装甲板匹配 → 数字识别，逐帧写入结果文件。

- 输入：`video/3.avi`（2209 帧）、`video/4.avi`（3416 帧），1440×1080 @25fps
- 输出：`3.txt`、`4.txt`（每帧一行）
- 实测性能：约 140 fps（7 ms/帧，单线程 CPU），满足 25 fps 实时要求

## 目录结构

| 路径 | 作用 |
| --- | --- |
| `start.cpp` | 主流程：读帧 → 检测 → 分类 → 写文件 → 可视化 |
| `Armor/src/ArmorDetector.cpp` | **本次实现**：`FindLight`、`FindArmor`、`WriteFrameResult` |
| `Armor/include/ArmorTypes.h` | 灯条 / 装甲板数据结构（未改动） |
| `io/` | `VideoReader` 封装（未改动） |
| `model/mlp.onnx`、`model/label.txt` | 数字分类模型与标签（未改动） |
| `tools/check_result.cpp` | 结果自检 + 反渲染工具 |
| `verify/` | 由 txt 反渲染出的抽检图 |
| `build/` | 构建产物（自动生成） |

## 环境

实测通过的环境：

| 项 | 版本 |
| --- | --- |
| 系统 | Windows（VS 18 2026 Community） |
| 编译器 | MSVC 14.51.36231（x64） |
| 构建 | CMake 4.3.1（VS 自带），生成器 `Visual Studio 18 2026` |
| OpenCV | 4.14.0（官方 Windows 预编译包，`build/x64/vc16`） |

Linux 同样可用（代码无平台相关 API）：Ubuntu 20.04/22.04 + `build-essential cmake pkg-config libopencv-dev`。

## 构建

### Windows

```powershell
# 1) 解压 OpenCV 4.14.0 到 C:\opencv（得到 C:\opencv\opencv\build）
#    https://github.com/opencv/opencv/releases/download/4.14.0/opencv-4.14.0-windows.exe
$cmake = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DOpenCV_DIR=C:/opencv/opencv/build
& $cmake --build build --config Release -j

# 2) 运行时 DLL
$env:Path = "C:\opencv\opencv\build\x64\vc16\bin;" + $env:Path
```

### Linux

```bash
sudo apt update && sudo apt install -y build-essential cmake pkg-config libopencv-dev
cmake -S . -B build && cmake --build build -j"$(nproc)"
```

## 运行

```bash
# 用法
./start <视频> [red|blue] [--headless] [--save-every N] [--save-list 帧号,...] [--max-frames N] [--verbose]

# 生成结果（在仓库根目录执行，输出 3.txt / 4.txt）
./start video/3.avi red --headless
./start video/4.avi red --headless
```

| 参数 | 说明 |
| --- | --- |
| `red` / `blue` | 敌方颜色，默认 `red` |
| `--headless` | 不开窗口、不 `waitKey`，用于批量/无显示器环境 |
| `--save-every N` | 每 N 帧保存一张带框的调试图到 `debug/` |
| `--save-list a,b,c` | 仅保存指定帧号的调试图 |
| `--max-frames N` | 只处理前 N 帧（调试用） |
| `--verbose` | 每 25 帧打印灯条数/匹配数/最终数 |

调试开关（环境变量）：

- `ARMOR_SAVE_MASK=<起始帧>`：保存该帧起 6 张灯条掩码到 `debug/`。

## 算法

### 1. 灯条查找（`FindLight`）

1. 分离 BGR 通道，取通道差：红灯 `R-B`，蓝灯 `B-R`。
   不用 HSV 的原因：灯条中心过曝成白/黄（实测 R=G=255、B≈100），色相会漂到 40° 以上，而 B 通道仍然很低，差值稳定。
2. 阈值二值化（默认 70）→ 3×3 开运算去噪 → 3×3 闭运算 2 次连接断裂。
3. `findContours` + `minAreaRect`，按下列判据过滤：

| 判据 | 取值 |
| --- | --- |
| 长边 | ≥ 8 px |
| 长宽比 | 1.6 ~ 12 |
| 轮廓面积 / 外接矩形面积 | ≥ 0.40 |
| 长轴与水平夹角 | 45° ~ 135°（相对竖直 ±40°） |
| 灯条数量 | 超过 40 条时按长度截断 |

### 2. 装甲板匹配（`FindArmor`）

两两配对，全部满足才认为是装甲板：

| 判据 | 取值 |
| --- | --- |
| 两灯条长轴夹角 | ≤ 12° |
| 长度比 | ≤ 1.7 |
| 中心 y 差 / 平均灯条长 | ≤ 0.6 |
| 内间距 / 平均灯条长 | 0.15 ~ 3.2 |
| 装甲板宽高比 | 1.1 ~ 5.0 |
| 宽高比 ≥ 3.0 | 判为 LARGE，否则 SMALL |

四点取两块灯条的**外侧角点**，顺序固定为 **左下 → 左上 → 右上 → 右下**（`ExtractNumbers` 的透视变换依赖该顺序，写反会导致数字镜像）。同帧重复框按中心距去重。

### 3. 数字识别

沿用模板提供的 `ExtractNumbers` + `Classify`（ONNX，阈值 0.7，忽略 `negative`），未改动判定逻辑。

## 输出格式

```
<帧号>, count=<N>[, armor1: number=<数字>, points=(x,y),(x,y),(x,y),(x,y)][ | armor2: ...]
```

- 一帧一行，行数等于视频帧数；无目标帧为 `<帧号>, count=0`。
- 坐标保留 2 位小数，原点为左上角。
- 示例：

```
0, count=0
1, count=1, armor1: number=3, points=(612.31,704.28),(610.02,651.77),(668.44,652.10),(670.90,705.02)
```

## 结果

见 `RESULTS.md`。两条视频均通过自检：

```bash
./build/Release/check_result.exe 3.txt video/3.avi --render-every 300
./build/Release/check_result.exe 4.txt video/4.avi --render-every 400
```

## 相对模板的改动

| 文件 | 改动 |
| --- | --- |
| `Armor/src/ArmorDetector.cpp` | 实现 `FindLight`、`FindArmor`；重写 `WriteFrameResult`；阈值 180 → 70 |
| `start.cpp` | 恢复 `ExtractNumbers`/`Classify` 调用；新增 `--headless` 等参数；输出统计 |
| `CMakeLists.txt` | 增加 `imgcodecs` 组件、MSVC `/utf-8`、`check_result` 目标 |
| `tools/check_result.cpp` | 新增自检/反渲染工具 |
| `README.md` | 重写 |

`ArmorTypes.h`、`io/`、`model/`、`ExtractNumbers`、`Classify` 均未改动。

## 已知限制

1. 数字模型只有 8 类，`outpost` 在极小/模糊目标上偶尔误判：两条视频共 17 帧（占 0.33%）。
2. 一侧灯条被遮挡时无法配对，会漏检（两条视频最长连续漏检均为 10 帧，约 0.4 s）。
3. 判别阈值按本批视频的暗光场景标定；换场地/曝光需要重调 `threshold_` 与灯条/配对参数。
4. 装甲板大幅侧倾（滚转超过约 40°）时四点排序依赖 x 排序，可能失效。
