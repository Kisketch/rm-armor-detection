# 4

| strategy | shots | verifiable | hits | hit rate | hits/s |
|---|---|---|---|---|---|
| direct (aim at current plate) | 2927 | 2840 | 1611 | 56.7% | 11.8 |
| point KF (predict plate centre) | 1138 | 1096 | 472 | 43.1% | 3.5 |
| robot EKF (whole-robot model) | 1956 | 1914 | 1303 | 68.1% | 9.5 |
| oracle (cheats: aims at the future plate) | 3229 | 3198 | 3001 | 93.8% | 22.0 |

| prediction | horizon | samples | mean px | median px | p90 px |
|---|---|---|---|---|---|
| no prediction | 0.12 s | 3623 | 52.9 | 50.9 | 76.5 |
| point KF | 0.12 s | 3623 | 103.1 | 73.5 | 216.3 |
| robot EKF | 0.12 s | 3623 | 8.6 | 7.0 | 16.1 |
| no prediction | 0.20 s | 3547 | 39.7 | 23.1 | 100.6 |
| point KF | 0.20 s | 3547 | 142.9 | 91.2 | 472.7 |
| robot EKF | 0.20 s | 3547 | 12.8 | 10.4 | 25.6 |
| no prediction | 0.32 s | 3605 | 46.4 | 44.5 | 83.0 |
| point KF | 0.32 s | 3605 | 288.7 | 85.1 | 747.8 |
| robot EKF | 0.32 s | 3605 | 17.9 | 14.8 | 36.3 |

tracking ratio 0.924, frames with armor 0.979, median |w| 8.284 rad/s, r1 0.238 m, r2 0.214 m, dz 0.046 m
