# 3

| strategy | shots | verifiable | hits | hit rate | hits/s |
|---|---|---|---|---|---|
| direct (aim at current plate) | 1943 | 1856 | 658 | 35.5% | 7.4 |
| point KF (predict plate centre) | 1743 | 1653 | 489 | 29.6% | 5.5 |
| robot EKF (whole-robot model) | 1394 | 1341 | 901 | 67.2% | 10.2 |
| oracle (cheats: aims at the future plate) | 2010 | 1990 | 1977 | 99.3% | 22.4 |

| prediction | horizon | samples | mean px | median px | p90 px |
|---|---|---|---|---|---|
| no prediction | 0.12 s | 2234 | 36.8 | 27.4 | 88.2 |
| point KF | 0.12 s | 2234 | 47.4 | 9.7 | 120.9 |
| robot EKF | 0.12 s | 2234 | 5.9 | 4.9 | 12.4 |
| no prediction | 0.20 s | 2235 | 43.2 | 44.1 | 77.2 |
| point KF | 0.20 s | 2235 | 80.0 | 29.7 | 130.5 |
| robot EKF | 0.20 s | 2235 | 10.9 | 8.3 | 23.3 |
| no prediction | 0.32 s | 2223 | 41.9 | 39.1 | 79.6 |
| point KF | 0.32 s | 2223 | 454.3 | 66.6 | 154.1 |
| robot EKF | 0.32 s | 2223 | 18.2 | 14.2 | 40.6 |

tracking ratio 0.932, frames with armor 0.965, median |w| 2.647 rad/s, r1 0.243 m, r2 0.209 m, dz 0.050 m
