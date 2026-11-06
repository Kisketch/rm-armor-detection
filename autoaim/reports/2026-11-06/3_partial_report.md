# 3

| strategy | shots | verifiable | hits | hit rate | hits/s |
|---|---|---|---|---|---|
| direct (aim at current plate) | 1943 | 1856 | 658 | 35.5% | 7.4 |
| point KF (predict plate centre) | 1743 | 1653 | 489 | 29.6% | 5.5 |
| robot EKF (whole-robot model) | 1439 | 1388 | 938 | 67.6% | 10.6 |
| oracle (cheats: aims at the future plate) | 2010 | 1990 | 1977 | 99.3% | 22.4 |

Rate-cap replay (first eligible fire decision; no heat/recoil feedback; virtual hits):

| max shots/s | strategy | shots | hits | unknown | hits/s | hits/all shots |
|---:|---|---:|---:|---:|---:|---:|
| 5.0 | direct (aim at current plate) | 423 | 136 | 21 | 1.5 | 32.2% |
| 5.0 | robot EKF (whole-robot model) | 350 | 228 | 13 | 2.6 | 65.1% |
| 10.0 | direct (aim at current plate) | 692 | 228 | 29 | 2.6 | 32.9% |
| 10.0 | robot EKF (whole-robot model) | 547 | 349 | 22 | 3.9 | 63.8% |
| 15.0 | direct (aim at current plate) | 1019 | 338 | 41 | 3.8 | 33.2% |
| 15.0 | robot EKF (whole-robot model) | 776 | 495 | 29 | 5.6 | 63.8% |

| prediction | horizon | samples | mean px | median px | p90 px |
|---|---|---|---|---|---|
| no prediction | 0.12 s | 2235 | 36.8 | 27.4 | 88.2 |
| point KF | 0.12 s | 2235 | 47.5 | 9.7 | 121.0 |
| robot EKF | 0.12 s | 2235 | 6.3 | 5.0 | 13.5 |
| no prediction | 0.20 s | 2236 | 43.1 | 44.1 | 77.1 |
| point KF | 0.20 s | 2236 | 80.3 | 29.7 | 130.5 |
| robot EKF | 0.20 s | 2236 | 11.4 | 8.5 | 24.3 |
| no prediction | 0.32 s | 2223 | 41.8 | 39.0 | 79.6 |
| point KF | 0.32 s | 2223 | 459.7 | 66.4 | 154.1 |
| robot EKF | 0.32 s | 2223 | 18.5 | 14.3 | 40.8 |

tracking ratio 0.933, frames with armor 0.965, median |w| 2.690 rad/s, r1 0.215 m, r2 0.232 m, dz -0.050 m
