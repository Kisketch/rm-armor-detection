# 结果报告

生成时间：2026-11-04　｜　单线程 CPU　｜　两套环境实测：

- 环境 A（主）：Ubuntu 26.04.1 LTS，g++ 15.2.0，CMake 4.2.3，OpenCV 4.10.0，Python 3.14.4 + cv2 4.10.0
- 环境 B（对照）：Windows 11，MSVC 14.51.36231，CMake 4.3.1，OpenCV 4.14.0

## 1. 总览

| 视频 | 帧数 | 输出行数 | 有目标帧占比 | 平均 count | 最大 count | 最长全 0 段 | 处理耗时/帧 | 数字正确率（抽检） |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 3.avi | 2209 | 2209 | 96.06%（2122 帧） | 1.0738 | 2 | 8 帧（937–944） | 6.8–7.8 ms（128–148 fps） | 96/96 = 100% |
| 4.avi | 3416 | 3416 | 97.28%（3323 帧） | 1.1326 | 2 | 8 帧（1479–1486） | 6.8–7.8 ms（128–147 fps） | 151/151 = 100% |

- 装甲板总数：3.avi 2372 块，4.avi 3869 块；全部识别为 `3`。
- 相邻帧 count 最大跳变：2（目标机器人转向时两块装甲板同时出现/消失）。
- 耗时为整段视频（含解码、检测、分类、写文件，`--headless`）的实测平均值，远低于 40 ms 预算。
- Windows 对照：7.0 ms/帧（143–144 fps），行数、装甲板总数与本表完全一致（见 §4）。

## 2. 自检输出（原文，`tools/check_result.py`）

```
$ python3 tools/check_result.py 3.txt video/3.avi
file=3.txt video=video/3.avi format=canonical
[PASS] 行数 == 视频帧数  (lines=2209 video_frames=2209)
[PASS] 帧号从 0 起严格 +1  (range=[0, 2208])
[PASS] 每行可完整解析  (unparsable_lines=0 stray_fragments=0)
[PASS] count 与条目数一致  (mismatch_lines=0)
[PASS] 每块装甲板恰好 4 点  (正则保证 4 点)
[PASS] 坐标在画面范围内  (out_of_bounds=0 frame=1440x1080)
[PASS] 四点不退化  (degenerate=0)
[PASS] number 在允许集合内  (classes=['3'])

video_frames=2209 lines=2209
frames_with_armor=2122 (96.06%)
total_armors=2372 mean_count=1.0738 max_count=2
longest_zero_run=8 (from frame 937)
max_adjacent_count_jump=2 (at frame 60)
numbers: 3=2372

RESULT: PASS

$ python3 tools/check_result.py 4.txt video/4.avi
file=4.txt video=video/4.avi format=canonical
[PASS] 行数 == 视频帧数  (lines=3416 video_frames=3416)
[PASS] 帧号从 0 起严格 +1  (range=[0, 3415])
[PASS] 每行可完整解析  (unparsable_lines=0 stray_fragments=0)
[PASS] count 与条目数一致  (mismatch_lines=0)
[PASS] 每块装甲板恰好 4 点  (正则保证 4 点)
[PASS] 坐标在画面范围内  (out_of_bounds=0 frame=1440x1080)
[PASS] 四点不退化  (degenerate=0)
[PASS] number 在允许集合内  (classes=['3'])

video_frames=3416 lines=3416
frames_with_armor=3323 (97.28%)
total_armors=3869 mean_count=1.1326 max_count=2
longest_zero_run=8 (from frame 1479)
max_adjacent_count_jump=2 (at frame 21)
numbers: 3=3869

RESULT: PASS
```

## 2.1 双格式与回归验证

`Homework.md` 的格式描述（“帧号，装甲板1:armor_number2.points:4个点”）有笔误、且无官方样例，因此程序支持两种输出，默认 canonical：

| 格式 | 触发 | 一行示例 |
| --- | --- | --- |
| canonical（默认） | 不加参数 | `1, count=1, armor1: number=3, points=(520.53,647.13),(522.30,625.94),(571.50,626.00),(571.50,647.00)` |
| literal | `--format literal` | `0，装甲板1: 3, points: (520.19,647.09),(521.81,625.91),(571.50,626.00),(571.50,647.00)` |

验证：

- 加 `--format` 后，默认输出**逐字节不变**：Linux 上 `cmp` 与旧 `3.txt`/`4.txt` 完全一致；Windows 上重跑后去掉 CR 也完全一致。
- literal 两条视频各 2209 / 3416 行，`check_result.py` 自动识别格式后同样 **PASS**（literal 无 `count` 字段，该项标注为跳过）。
- literal 结果存放在 `results_alt/3_literal.txt`、`4_literal.txt`，主结果仍是根目录 `3.txt`/`4.txt`。
- 两个 Python 工具都能自动识别格式；`render_check.py` 对两种格式渲染出的 PNG 逐字节相同（9 帧抽样比对），说明两种文件承载的数据完全一致。
- C++ 版 `check_result` 只解析 canonical，遇到 literal 会直接提示改用 Python 版，不会静默误判。

## 3. 抽检

### 3.1 框的贴合度与角点顺序（A-03）

`verify/3_f*.png`（18 张）、`verify/4_f*.png`（28 张）：全部由 **txt 内容** 经 `tools/render_check.py` 反渲染，每 125 帧（5 秒）1 张，共 46 帧、54 块装甲板。
角点按顺序着色（0 红=左下、1 绿=左上、2 蓝=右上、3 黄=右下），左上角附 4 倍放大图。

- 46/46 帧的框都贴着两条灯条，角点顺序正确，没有镜像/旋转；
- 抽检帧中没有误检（没有框在装饰灯或其它物体上）；
- 抽检帧中没有“装甲板完整可见却 count=0”的情况。

### 3.2 数字（A-04）

`verify/3_numbers.png`（96 张）、`verify/4_numbers.png`（151 张）：每 25 帧收集一次 `ExtractNumbers` 输出的 28×28 数字图，标注帧号/分类结果/置信度。
全部 247 张都是正立、不镜像的“3”，分类结果全部为 `3`，绝大多数置信度为 1.00。

## 4. 跨平台一致性

同一份源码、同一批视频，在两套环境中各跑一遍，逐行比对：

| 项 | 3.avi | 4.avi | 合计 |
| --- | --- | --- | --- |
| 行数 | 2209 = 2209 | 3416 = 3416 | 5625 = 5625 |
| 装甲板总数 | 2372 = 2372 | 3869 = 3869 | 6241 = 6241 |
| 逐行数字差异 | 0 行 | 0 行 | 0 行 |
| 坐标最大绝对差 | 0.000000 px | 0.000000 px | 0.000000 px |

唯一的差异是行尾：Windows 写 CRLF，Linux 写 LF（`cmp` 在第 101 字节处 0x0D/0x0A）。仓库内的 `3.txt`/`4.txt` 为 Linux 生成的 LF 版本，坐标与 Windows 版逐字节相同。

## 5. Linux 构建与 headless 验证

```bash
$ g++ --version | head -1
g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0
$ cmake --version | head -1
cmake version 4.2.3
$ pkg-config --modversion opencv4
4.10.0

$ rm -rf build && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j8
-- Found OpenCV: /usr (found version "4.10.0") found components: core highgui videoio imgproc imgcodecs dnn
-- Configuring done (0.5s)
-- Generating done (0.0s)
[100%] Linking CXX executable start
[100%] Built target start
[100%] Built target check_result
```

无显示器分支（A-07，故意不传 `--headless`）：

```bash
$ env -u DISPLAY -u WAYLAND_DISPLAY ./build/start video/3.avi red --max-frames 200
未检测到 DISPLAY，自动启用 --headless
video=3 frames=200 armors=216 elapsed=1.43162s fps=139.702
```

全量运行：

```bash
$ /usr/bin/time -f "wall=%es maxrss=%MKB" ./build/start video/3.avi red --headless
video=3 frames=2209 armors=2372 elapsed=14.9556s fps=147.704
wall=15.02s maxrss=121684KB

$ /usr/bin/time -f "wall=%es maxrss=%MKB" ./build/start video/4.avi red --headless
video=4 frames=3416 armors=3869 elapsed=23.3026s fps=146.593
wall=23.36s maxrss=121864KB
```

同一台 VM 在负载较高时复跑（干净目录、README 原命令）：3.avi 17.13 s（7.75 ms/帧）、4.avi 26.68 s（7.81 ms/帧），仍远低于 40 ms 预算；两次运行的 `3.txt`/`4.txt` 逐字节一致，说明耗时波动不影响结果。

## 6. 自检工具的负向测试

为确认 `check_result.py` 不是“永远 PASS”，对 3.txt 做 6 种定向破坏，每种都应 FAIL：

| 破坏 | 期望命中的检查 | 实测 |
| --- | --- | --- |
| 删掉最后一行 | 行数 == 视频帧数 | FAIL（lines=2208 video_frames=2209） |
| 第 5 行帧号 4→6 | 帧号严格 +1 | FAIL（第 5 行帧号 6，应为 4） |
| 第 1 行删掉 `points=(...)` | 每行可完整解析 | FAIL（stray_fragments=1） |
| 第 1 行坐标改 9999 | 坐标在画面范围内 | FAIL（out_of_bounds=1） |
| 第 1 行 number=3→9 | number 在允许集合内 | FAIL（illegal={'9': 1}） |
| 第 1 行 count=1→2 | count 与条目数一致 | FAIL（mismatch_lines=1，frame 0） |

六项全部按预期 FAIL，退出码均为 1。

## 7. 本轮修复：四点取法（2026-11-04）

上一版（2026-11-03）用“两条灯条的 8 个角点按 x 排序，取最左两个和最右两个”作为四点。
抽检发现 3.avi 第 875 帧、4.avi 第 2625 帧两块装甲板都清楚可见，输出却是 `count=0`。
调试（`--verbose` + `--dump-numbers` + `--save-list`）显示：几何匹配到了 2 块，但都被 `Classify` 删除；其中一块的数字图是扭曲的乱码。
原因：灯条细、稍有倾斜时，最左两个角点可能都是灯条**上端**的两个角，四边形自交，透视变换出乱码。

修复：改用 `LightBlob` 已有的长轴端点 `top/bottom`，`points = {left.bottom, left.top, right.top, right.bottom}`。

| 指标 | 修复前 | 修复后 |
| --- | --- | --- |
| 3.avi 有目标帧 | 2035（92.12%） | **2122（96.06%）** |
| 4.avi 有目标帧 | 2999（87.79%） | **3323（97.28%）** |
| 装甲板总数（两条视频） | 5128 | **6241** |
| `outpost` 误判 | 17 | **0** |
| 最长连续漏检 | 10 帧 | **8 帧** |

修复只改了 `FindArmor` 里取四点的几行；`ExtractNumbers`/`Classify` 未改动。

## 8. 典型现象

1. **同一帧 2 块装甲板**（3.avi 250 帧、4.avi 546 帧）：机器人斜对相机，正面和侧面两块装甲板同时可见，两块都识别为 3，是正确检测（见 `verify/3_f00875.png`、`verify/4_f02625.png`）。
2. **连续漏检 8 帧**：机器人正侧对相机，两块装甲板各只露出一条灯条，无法配对（已抽 3.avi 第 940 帧确认）。
3. **全 0 帧**：3.avi 87 帧、4.avi 93 帧，集中在上述转向区段。

## 9. 已知限制

1. 视频里只有 3 号，其它数字没有样本可验证。
2. 单灯条（另一条被遮挡或侧对）无法定位装甲板。
3. 参数按本批暗光视频标定，换场景需重调。
4. 灯条倾角限制在竖直 ±40° 以内。
5. Ubuntu 20.04/22.04 自带的 OpenCV 4.2/4.5 未实测，只验证了 4.10 与 4.14。
