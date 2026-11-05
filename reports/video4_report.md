<!-- 由 autoaim_video / autoaim_sim 生成，请勿手改；环境与复现方式见仓库根目录 BASELINE.md -->
<!-- generated: 2026-11-04 | commit: edfd512 | OS: Windows 11 10.0.26300 | OpenCV: 4.14.0 | CPU: Intel Core Ultra 9 275HX -->
# 4

| strategy | shots | verifiable | hits | hit rate | hits/s |
|---|---|---|---|---|---|
| direct (aim at current plate) | 2927 | 2840 | 1611 | 56.7% | 11.8 |
| point KF (predict plate centre) | 1138 | 1096 | 472 | 43.1% | 3.5 |
| robot EKF (whole-robot model) | 1914 | 1877 | 1119 | 59.6% | 8.2 |
| oracle (cheats: aims at the future plate) | 3229 | 3198 | 3001 | 93.8% | 22.0 |

| prediction | horizon | samples | mean px | median px | p90 px |
|---|---|---|---|---|---|
| no prediction | 0.12 s | 3604 | 52.9 | 50.9 | 76.5 |
| point KF | 0.12 s | 3604 | 103.2 | 73.6 | 217.1 |
| robot EKF | 0.12 s | 3604 | 9.8 | 8.1 | 18.6 |
| no prediction | 0.20 s | 3529 | 39.6 | 23.1 | 100.5 |
| point KF | 0.20 s | 3529 | 143.1 | 91.2 | 472.7 |
| robot EKF | 0.20 s | 3529 | 15.5 | 12.8 | 30.5 |
| no prediction | 0.32 s | 3585 | 46.5 | 44.5 | 83.0 |
| point KF | 0.32 s | 3585 | 289.0 | 85.2 | 746.0 |
| robot EKF | 0.32 s | 3585 | 22.7 | 19.3 | 43.3 |

tracking ratio 0.918, frames with armor 0.979, median |w| 8.267 rad/s, r1 0.239 m, r2 0.213 m, dz 0.046 m
