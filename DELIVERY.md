# 交付报告 — RoboMaster 装甲板检测（自瞄考核）

| 项 | 内容 |
| --- | --- |
| 交付日期 | 2026-11-04 |
| 评审人 | 研一控制 李宇宁 |
| 代码仓库 | <https://github.com/Kisketch/rm-armor-detection>（public，`main`） |
| 提交包 | `提交包_自瞄考核_v3.zip`（73 个文件，约 30 MB，不含视频与构建产物） |
| 仓库提交 | `main` 分支，含完整开发记录；交付版本以仓库最新提交为准 |
| 实测环境 | Ubuntu 26.04.1 + g++ 15.2.0 + OpenCV 4.10.0；Windows 11 + MSVC 14.51 + OpenCV 4.14.0 |

## 1. 任务与范围

用给定的 C++17/OpenCV 模板实现自瞄的**灯条查找**与**装甲板匹配**，接通数字提取与分类，把 `3.avi`、`4.avi` 每一帧识别到的装甲板（数字 + 4 个角点）写入文本文件。

- 本人负责：`FindLight`、`FindArmor`、结果输出格式、自检/反渲染工具、文档与验证。
- 未改动：`ArmorTypes.h` 结构体、公有函数签名、`ExtractNumbers` / `Classify` 的判定逻辑、`model/`、`io/`。
- 不在范围：ROS rviz 里程计圆周运动（队长明确分给另一位同学）。

## 2. 交付物

| ID | 交付物 | 位置 | 状态 |
| --- | --- | --- | --- |
| D-01 | `FindLight` / `FindArmor` / `WriteFrameResult` | `Armor/src/ArmorDetector.cpp` | 完成 |
| D-02 | `start.cpp`（恢复数字识别、`--headless` 等参数） | `start.cpp` | 完成 |
| D-03 | 两条视频的逐帧结果 | `3.txt`（2209 行）、`4.txt`（3416 行） | 完成 |
| D-04 | 结果自检脚本 | `tools/check_result.py`（另有等价 C++ 版） | 完成 |
| D-05 | 反渲染抽检图 | `verify/`（46 帧 + 2 张数字拼图，共 48 张） | 完成 |
| D-06 | 说明文档 | `README.md` | 完成 |
| D-07 | 结果统计 | `RESULTS.md` | 完成 |
| D-08 | 提交包 / 仓库 | `提交包_自瞄考核_v3.zip`、GitHub 仓库 | 完成 |
| 附加 | 备用输出格式 | `results_alt/3_literal.txt`、`4_literal.txt` | 完成 |

## 3. 验收自检

| ID | 判据 | 实测 | 结论 |
| --- | --- | --- | --- |
| A-01 | 两条视频跑完不崩溃，行数 = 帧数 | 2209 / 3416，重跑与提交版逐字节一致 | PASS |
| A-02 | 格式合法、每块 4 点、坐标在画面内、count 一致、帧号连续 | `check_result.py` 8/8 项 PASS（两条视频 × 两种格式，共 4 组） | PASS |
| A-03 | 抽检 ≥20 帧，框贴合、角点顺序正确 | 46 帧反渲染图（每 5 秒 1 帧），重新渲染与提交版逐字节相同；抽查双目标帧角点顺序正确 | PASS |
| A-04 | 数字正确率 ≥80% | 247 张数字图（96+151）全部为正立“3”，置信度≈1.00 | PASS（仅“3”有样本） |
| A-05 | 相邻帧 count 跳变 ≤5，无长时间全 0 | 最大跳变 2；最长全 0 段 8 帧（0.32 s） | PASS |
| A-06 | 单帧 ≤40 ms | 实测 6.8–18.0 ms/帧（55–148 fps，区间说明见 `RESULTS.md` §5） | PASS |
| A-07 | 无显示器环境可跑完 | `env -u DISPLAY -u WAYLAND_DISPLAY` 自动 headless，跑通 | PASS |
| A-08 | 干净目录一次构建通过 | 全新 `build/`：0 error、0 warning | PASS |

自检工具本身做过 6 组破坏性测试（删行、改帧号、删点、坐标越界、非法数字、count 不符），**6/6 全部被判 FAIL**，证明检查不是“永远 PASS”。

GitHub Actions 每次推送自动执行：Ubuntu 构建（强制 0 warning）→ 合成视频跑通 canonical / literal 两种格式 → 非法参数必须报错；最近 3 次运行全部 success。

## 4. 关键指标

| 视频 | 帧数 | 有目标帧 | 装甲板总数 | 平均 count | 最大 count | 最长全 0 段 |
| --- | --- | --- | --- | --- | --- | --- |
| 3.avi | 2209 | 2122（96.06%） | 2372 | 1.0738 | 2 | 8 帧 |
| 4.avi | 3416 | 3323（97.28%） | 3869 | 1.1326 | 2 | 8 帧 |

跨平台一致性：同一份源码在 Ubuntu（OpenCV 4.10）与 Windows（OpenCV 4.14）各跑一遍，5625 行结果**逐行数值完全一致**，坐标最大差 0.000000 px，仅行尾 CRLF/LF 不同。

## 5. 算法要点

1. **灯条**：BGR 通道差（红 `R-B`、蓝 `B-R`）二值化（阈值 70）→ 3×3 开/闭运算 → `findContours` + `minAreaRect` → 按长宽比、面积、填充率、倾角过滤。
2. **配对**：两灯条角度差 ≤12°、长度比 ≤1.7、中心 y 差/灯长 ≤0.6、内间距/灯长 0.15–3.2、整体宽高比 1.1–5.0；四点取两条灯条长轴端点，固定为“左下→左上→右上→右下”。
3. **数字**：沿用模板的 `ExtractNumbers` 透视变换 + ONNX 分类（阈值 0.7，忽略 `negative`）。
4. **输出**：默认 `帧号, count=N, armor1: number=3, points=(x,y)×4`；另提供 `--format literal` 输出贴近作业原句的写法（`0，装甲板1: 3, points: (…)×4`），两套结果都已生成。

## 6. 复现

```bash
sudo apt install -y build-essential cmake pkg-config libopencv-dev python3-opencv
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j"$(nproc)"
./build/start video/3.avi red --headless && ./build/start video/4.avi red --headless
python3 tools/check_result.py 3.txt video/3.avi
```

提交包不含视频，运行前把 `3.avi` / `4.avi` 放进 `video/`。

## 7. 结果文件校验

| 文件 | 行数 | 大小 | SHA-256 |
| --- | --- | --- | --- |
| `3.txt` | 2209 | 243,546 B | `eeba6148511193abd661771aaa78c2f040dbcdceaab486fe0305affb436efa45` |
| `4.txt` | 3416 | 395,470 B | `01073921f6b3baab464713420091bd45ed7b45c632842977eac5b64edfdf49e1` |

## 8. 已知限制与风险

1. **数字样本只有“3”**：两条视频里只出现 3 号步兵，A-04 的 100% 只对“3”成立，其它类别没有样本。
2. **单灯条漏检**：机器人正侧对相机时每块装甲板只露一条灯条，无法配对；两条视频最长连续漏检均为 8 帧（约 0.3 s）。
3. **参数按本批暗光视频标定**：换场地/曝光需重调阈值与几何判据。
4. **倾角限制**：灯条与竖直方向夹角超过 ±40° 不检测。
5. **性能随宿主波动**：同一台 VM 不同时段实测 6.8–18.0 ms/帧，最慢仍低于 40 ms 预算；波动来自宿主 CPU 频率与大小核调度，不影响结果（重跑逐字节一致）。
6. **输出格式的残余不确定性**：作业原句“帧号，装甲板1:armor_number2.points:4个点”有笔误且无官方样例，因此同时提供 canonical 与 literal 两种格式；若评审要求第三种写法，只需改 `WriteFrameResult` 一处。

## 9. 版本记录

| 版本 | 内容 |
| --- | --- |
| 2026-11-03 | 完成 `FindLight` / `FindArmor`，Windows 下跑通 |
| 2026-11-04 | 修复四点取法（漏检 92%→96%、88%→97%）；补 Python 自检/反渲染；Linux 实测；输出格式双版本；CI 与文档完善 |
