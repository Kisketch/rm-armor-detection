<!-- 由 autoaim_video / autoaim_sim 生成，请勿手改；环境与复现方式见仓库根目录 BASELINE.md -->
<!-- generated: 2026-11-04 | commit: edfd512 | OS: Windows 11 10.0.26300 | OpenCV: 4.14.0 | CPU: Intel Core Ultra 9 275HX -->
# 3

| strategy | shots | verifiable | hits | hit rate | hits/s |
|---|---|---|---|---|---|
| direct (aim at current plate) | 1943 | 1856 | 658 | 35.5% | 7.4 |
| point KF (predict plate centre) | 1743 | 1653 | 489 | 29.6% | 5.5 |
| robot EKF (whole-robot model) | 1331 | 1279 | 799 | 62.5% | 9.0 |
| oracle (cheats: aims at the future plate) | 2010 | 1990 | 1977 | 99.3% | 22.4 |

| prediction | horizon | samples | mean px | median px | p90 px |
|---|---|---|---|---|---|
| no prediction | 0.12 s | 2234 | 36.8 | 27.4 | 88.3 |
| point KF | 0.12 s | 2234 | 47.6 | 9.6 | 121.0 |
| robot EKF | 0.12 s | 2234 | 6.1 | 5.1 | 12.8 |
| no prediction | 0.20 s | 2234 | 43.2 | 44.1 | 77.2 |
| point KF | 0.20 s | 2234 | 80.6 | 29.4 | 130.5 |
| robot EKF | 0.20 s | 2234 | 11.5 | 9.1 | 24.1 |
| no prediction | 0.32 s | 2220 | 41.8 | 39.0 | 79.4 |
| point KF | 0.32 s | 2220 | 460.9 | 66.6 | 154.4 |
| robot EKF | 0.32 s | 2220 | 19.9 | 16.2 | 42.8 |

tracking ratio 0.932, frames with armor 0.965, median |w| 2.570 rad/s, r1 0.243 m, r2 0.208 m, dz 0.050 m
