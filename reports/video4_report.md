# 4

| strategy | shots | verifiable | hits | hit rate | hits/s |
|---|---|---|---|---|---|
| direct (aim at current plate) | 2927 | 2840 | 1611 | 56.7% | 11.8 |
| point KF (predict plate centre) | 1138 | 1096 | 472 | 43.1% | 3.5 |
| robot EKF (whole-robot model) | 1924 | 1886 | 1241 | 65.8% | 9.1 |
| oracle (cheats: aims at the future plate) | 3229 | 3198 | 3001 | 93.8% | 22.0 |

| prediction | horizon | samples | mean px | median px | p90 px |
|---|---|---|---|---|---|
| no prediction | 0.12 s | 3627 | 52.9 | 50.9 | 76.5 |
| point KF | 0.12 s | 3627 | 103.0 | 73.5 | 216.3 |
| robot EKF | 0.12 s | 3627 | 9.2 | 7.5 | 17.4 |
| no prediction | 0.20 s | 3550 | 39.7 | 23.1 | 100.6 |
| point KF | 0.20 s | 3550 | 142.7 | 91.0 | 471.5 |
| robot EKF | 0.20 s | 3550 | 14.2 | 11.7 | 28.4 |
| no prediction | 0.32 s | 3608 | 46.4 | 44.5 | 83.0 |
| point KF | 0.32 s | 3608 | 288.2 | 84.9 | 745.7 |
| robot EKF | 0.32 s | 3608 | 19.7 | 16.7 | 39.1 |

tracking ratio 0.924, frames with armor 0.979, median |w| 8.272 rad/s, r1 0.239 m, r2 0.214 m, dz 0.046 m
