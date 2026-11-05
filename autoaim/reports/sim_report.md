<!-- 由 autoaim_video / autoaim_sim 生成，请勿手改；环境与复现方式见仓库根目录 BASELINE.md -->
<!-- generated: 2026-11-04 | commit: edfd512 | OS: Windows 11 10.0.26300 | OpenCV: 4.14.0 | CPU: Intel Core Ultra 9 275HX -->
fps 25, corner noise 0.70 px, dropout 5%, 20 s per scenario, fx 2400, latency 60 ms, bullet 25 m/s

| scenario | direct hit% | point KF hit% | robot EKF hit% | robot hits/s | centre err cm | |w| err rad/s | radius err cm | tracking % |
|---|---|---|---|---|---|---|---|---|
| spin 2.1 rad/s at 5 m (like the videos) | 42.3 | 97.5 | 99.4 | 16.95 | 4.5 | 0.62 | 0.9 | 96 |
| spin 6 rad/s + strafe 0.5 m at 5 m | 53.6 | 14.6 | 45.0 | 5.45 | 3.5 | 0.80 | 0.3 | 96 |
| fast spin 10 rad/s at 4 m | 43.2 | 37.0 | 98.7 | 14.65 | 2.4 | 0.41 | 0.2 | 96 |
| no spin, strafe 0.8 m / 2 s at 6 m | 14.0 | 24.5 | 17.1 | 2.50 | 6.9 | 1.31 | 0.8 | 93 |
| spin 4 rad/s approaching 7 m -> 3 m | 51.8 | 44.4 | 98.9 | 13.40 | 3.9 | 0.54 | 0.5 | 95 |
| variable spin 0.5..4.5 rad/s at 6 m | 69.9 | 73.2 | 97.4 | 13.00 | 5.2 | 0.59 | 0.5 | 96 |
| swing back and forth (+-3 rad/s, 4 s) at 6 m | 64.4 | 88.7 | 98.2 | 16.60 | 5.1 | 0.61 | 0.4 | 97 |
| spin 3 rad/s + gentle strafe 0.3 m / 3 s at 6 m | 74.9 | 67.0 | 85.2 | 11.80 | 4.9 | 0.60 | 0.5 | 95 |
