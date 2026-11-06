# 开源基础筛选与橙灯四角点模型抽样（2026-11-06）

## 结论

当前 `autoaim/` 的 C++17 + OpenCV 检测、PnP、整车跟踪、弹道链路继续作为可回退的基线。接下来最值得验证的算法改动，是把**四角点装甲板检测器**作为可选输入接入现有位姿与跟踪流程；不替换已有检测器。深圳大学公开模型已经在本项目橙灯视频的抽样帧里检出了相邻两块装甲板，但还没有人工标注的检测准确率、完整视频火控评估或实车证据。

系统实现参考以同济 `sp_vision_25` 为主，重点是相机/IMU 时间对齐、手眼标定、记录回放和实机命令链。其代码不直接覆盖本项目。此前已有的 `chenjunnn/rm_auto_aim`、`FYT2024_vision` 保留为本项目检测几何、PnP 和整车估计的历史参考。

## 已核验的仓库

仓库状态和许可证按 2026-11-06 的 GitHub 仓库元数据核验；提交固定在下表，避免以后 `main` 变化后把结论归到新代码。

| 优先级 | 仓库与快照 | 许可证 | 实际可用内容 | 对本项目的决定 |
| --- | --- | --- | --- | --- |
| 1：实机系统参考 | [TongjiSuperPower/sp_vision_25 @ bd9f5e7](https://github.com/TongjiSuperPower/sp_vision_25/tree/bd9f5e798fa3c6dd3b483ae6627796afb41c608d) | MIT | [无 ROS 主链路](https://github.com/TongjiSuperPower/sp_vision_25/blob/bd9f5e798fa3c6dd3b483ae6627796afb41c608d/src/standard.cpp)、[相机/IMU 四元数插值](https://github.com/TongjiSuperPower/sp_vision_25/blob/bd9f5e798fa3c6dd3b483ae6627796afb41c608d/io/cboard.cpp)、相机与手眼标定、视频+姿态记录、独立调试程序 | 借鉴时间戳和回放契约；实车到手后核对相机曝光时刻、IMU、外参和 MCU 协议。项目需 OpenVINO、Eigen、fmt 等，且附带相机 SDK 二进制，不整体移植。README 的国赛命中率是该队设备上的报告，不能外推到本项目。 |
| 1：检测候选 | [SZURPVision/26_NNDeployment_Lib_and_Detection_Models @ ccb5dfe](https://github.com/SZURPVision/26_NNDeployment_Lib_and_Detection_Models/tree/ccb5dfe09696edeea445ffb8f99adaa7c383ec3a) | MIT | 随仓 [Infantry-v8n 四角点 ONNX](https://github.com/SZURPVision/26_NNDeployment_Lib_and_Detection_Models/tree/ccb5dfe09696edeea445ffb8f99adaa7c383ec3a/%E6%89%80%E6%9C%89%E6%A8%A1%E5%9E%8B/onnx)、[解码与 NMS](https://github.com/SZURPVision/26_NNDeployment_Lib_and_Detection_Models/blob/ccb5dfe09696edeea445ffb8f99adaa7c383ec3a/src/core/algorithm/NNdeployment/src/postprocess/network_postprocess.cpp)、测试视频与推理接口。输入 `1×3×480×640`，输出 `1×21×6300`。 | 先做离线对比，再决定是否添加可选推理后端。模型按红/蓝/白等类别训练，视频里的橙灯被判为红类；这不等于已验证橙/绿跨场景鲁棒性。 |
| 2：火控与同步参考 | [Alliance-Algorithm/rmcs_auto_aim_v2 @ 4f43409](https://github.com/Alliance-Algorithm/rmcs_auto_aim_v2/tree/4f4340956a989b0d3ecd4fbd94b1e72a301293cd) | MIT | [IMU 环形缓冲与时间插值](https://github.com/Alliance-Algorithm/rmcs_auto_aim_v2/blob/4f4340956a989b0d3ecd4fbd94b1e72a301293cd/src/io/capturer/imu_snapshot_buffer.hpp)、[开火角窗口](https://github.com/Alliance-Algorithm/rmcs_auto_aim_v2/blob/4f4340956a989b0d3ecd4fbd94b1e72a301293cd/src/module/fire_control/shoot_evaluator.cpp)、回放和 PnP 测试 | 可参考边界处理和测试案例；[CMake](https://github.com/Alliance-Algorithm/rmcs_auto_aim_v2/blob/4f4340956a989b0d3ecd4fbd94b1e72a301293cd/CMakeLists.txt)要求 C++23、ROS2/RMCS、OpenVINO、海康 SDK，当前独立 C++17 工程不以它为底座。 |
| 3：框架组织参考 | [SZURPVision/RP-26AutoAim-Frame](https://github.com/SZURPVision/RP-26AutoAim-Frame) | MIT | 插件、线程组与 Context 通信 | [README](https://github.com/SZURPVision/RP-26AutoAim-Frame/blob/main/README.md)明确公开插件是**不含算法的最小示例**；不能把它当作现成装甲检测器。 |
| 3：同济衍生实现 | [SPR-Algorithm/spr_vision_26](https://github.com/SPR-Algorithm/spr_vision_26) | MIT | 2026 赛季的相机、自瞄和调试流程 | [README](https://github.com/SPR-Algorithm/spr_vision_26/blob/main/readme.md)明确建立在 `sp_vision_25` 上。可查其新增改动，但不作为独立证据叠加同济的效果。 |
| 3：ROS2 组合参考 | [SMBU-PolarBear-Robotics-Team/pb2025_rm_vision](https://github.com/SMBU-PolarBear-Robotics-Team/pb2025_rm_vision) | Apache-2.0 | 装甲检测、整车估计、弹道 ROS2 包的集成 | [README](https://github.com/SMBU-PolarBear-Robotics-Team/pb2025_rm_vision)说明其模块主要来自既有开源项目；适合检查接口，不适合作为当前无 ROS 工程的重构起点。 |

原指导材料中的 `robomaster-oss/rm_vision` 在此次 GitHub API 查询返回 404；它可能已更名、移走或不可公开访问，暂不列为可用依赖。仓库文档只作为线索，本表以实际公开仓库、文件和构建配置为准。

## 对本地橙灯视频的初筛

使用上述模型仓库的 `Infantry-v8n-fp16-20260726-D1.8w-B16.onnx`，文件大小 6,429,469 字节，SHA256 为 `8b735d0e68faf330e351e353be9f792954512cfaf64f054123c466f755a6fe1e`。模型只放在 `/tmp/rm_reference_models/`，未提交到项目。视频是原始 `3.avi`（2209 帧）和 `4.avi`（3416 帧），均为 1440×1080、25 fps。每隔 50 帧抽一帧，共 114 帧。

实验用 ONNX Runtime 1.31.1 CPU、OpenCV 4.10.0，`confidence=0.5`、NMS IoU `0.2`（与模型仓默认一致）；颜色 `3` 按原解码器过滤，但保留其余颜色用于观察。模型输出的橙灯板均被归为颜色 ID `1`（红）、装甲 ID `3`。当前基线由**本分支当次重新编译**的 `autoaim_video --redetect` 生成；两条视频的基线虚拟命中分别复现为 67.6% 和 68.6%。[抽样原始 JSON](../reports/2026-11-06/open_source_probe.json) 保留了运行参数和全部计数。**下表是未经人工标注的候选数，不是准确率、召回率或命中率。**

| 视频 | 抽样帧 | 当前基线：0 / 1 / 2 块的帧数 | 开源模型：0 / 1 / 2 块的帧数 | 候选总数：基线 → 模型 | 模型推理中位 / P95 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `3.avi` | 45 | 2 / 37 / 6 | 1 / 25 / 19 | 49 → 63 | 11.79 / 15.17 ms |
| `4.avi` | 69 | 1 / 58 / 10 | 0 / 34 / 35 | 78 → 104 | 11.61 / 14.07 ms |
| 合计 | 114 | 3 / 95 / 16 | 1 / 59 / 54 | 127 → 167 | — |

在 `3.avi` 第 1600 帧，当前基线框出左侧板，开源模型分开框出了左右两块；[局部并排图](OPEN_SOURCE_3_F1600_COMPARE.png) 只用于展示这一帧。抽样里模型比基线多 40 个候选，可能包含真实补检，也可能包含误检。没有逐帧真值时不能计算提升百分比。第 950 帧模型没有检出；画面中的亮条当时侧向/遮挡，人工观察也未见清晰正面板。

OpenCV 4.10 DNN 在本机加载这个 FP16 ONNX 时于首个 `Conv` 节点报错，因此这次使用独立虚拟环境里的 ONNX Runtime。上述耗时只计 `session.run()`，不含视频解码、预处理、NMS、PnP、跟踪、弹道和通讯；硬件为本机 VM 可见的 8 vCPU（Intel Core Ultra 9 275HX）。重复运行时中位值曾在 13.0–13.5 ms、P95 在 18.6–18.7 ms，说明该小样本耗时受环境负载影响。不能据此声称实车全链路可达相同帧率。

### 复现

```bash
cd autoaim  # 从 rm-armor-detection 仓库根目录进入
python3 -m venv --system-site-packages /tmp/rm_reference_models/venv
/tmp/rm_reference_models/venv/bin/python -m pip install onnxruntime
curl -L --fail 'https://raw.githubusercontent.com/SZURPVision/26_NNDeployment_Lib_and_Detection_Models/ccb5dfe09696edeea445ffb8f99adaa7c383ec3a/%E6%89%80%E6%9C%89%E6%A8%A1%E5%9E%8B/onnx/Infantry-v8n-fp16-20260726-D1.8w-B16.onnx' -o /tmp/rm_reference_models/Infantry-v8n.onnx
cmake -S . -B /tmp/rm_reference_models/current-build -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/rm_reference_models/current-build -j4
/tmp/rm_reference_models/current-build/autoaim_video /home/lgnatius/RM/3.avi --out /tmp/rm_reference_models/current_3 --redetect
/tmp/rm_reference_models/current-build/autoaim_video /home/lgnatius/RM/4.avi --out /tmp/rm_reference_models/current_4 --redetect
python3 tools/probe_open_model.py \
  --model /tmp/rm_reference_models/Infantry-v8n.onnx \
  --video /home/lgnatius/RM/3.avi --video /home/lgnatius/RM/4.avi \
  --cache /tmp/rm_reference_models/current_3/3_detections.txt \
  --cache /tmp/rm_reference_models/current_4/4_detections.txt
```

最后两条 `--cache` 仅作本机基线比较；在其他机器上可省略，或用现有 `autoaim_video` 先生成检测缓存。脚本不修改模型、视频、缓存或当前默认配置。

## 实施顺序与验收

1. 建立人工标注的小型测试集：从两条视频按距离、侧角、单/双板、遮挡和无目标画面抽帧，标出每块独立装甲板的四角点与 ID。统计精确率、召回率、相邻两板误合并率、四角点误差和无目标误报。橙灯优先；绿色等新场景取得视频后按同一协议测。
2. 做**可选**神经检测输入：明确模型四点定义是否与当前 `ArmorDetection::corners` 的灯条端点一致，检查角点顺序、物理尺寸和 PnP 重投影；不通过就不送入现有 PoseSolver。推理运行时单独封装，保留传统检测后端和原 MLP 路径。
3. 在完整 `3.avi` / `4.avi` 上比较检测、PnP、跟踪、虚拟命中、每秒有效命中及 P95 全链路延迟；虚拟射击只用于离线排序。候选数增多但误配、预测或火控变差时不启用新后端。
4. 拿到实车相机后，完成真实内参与手眼标定、曝光时间戳/IMU 同步、MCU 协议和实测延迟/弹速；按 [RMUC 2026 目标说明](RMUC_2026_TARGET_REVIEW.md) 验证实际可击打面。真标定和实测未通过前保持 draft PR，不合并到 `main`。
