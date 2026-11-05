fps 25, corner noise 0.70 px, dropout 5%, 20 s per scenario, fx 2400, latency 60 ms, bullet 25 m/s, seed 42

| scenario | direct hit% | point KF hit% | robot EKF hit% | robot hits/s | centre err cm | |w| err rad/s | radius err cm | tracking % |
|---|---|---|---|---|---|---|---|---|
| spin 2.1 rad/s at 5 m (like the videos) | 42.3 | 97.5 | 99.7 | 16.95 | 3.9 | 0.55 | 0.9 | 96 |
| spin 6 rad/s + strafe 0.5 m at 5 m | 53.6 | 14.6 | 43.2 | 5.75 | 3.5 | 0.97 | 0.3 | 96 |
| fast spin 10 rad/s at 4 m | 43.2 | 37.0 | 99.3 | 14.20 | 2.1 | 0.38 | 0.2 | 96 |
| no spin, strafe 0.8 m / 2 s at 6 m | 14.0 | 24.5 | 14.6 | 2.05 | 10.7 | 1.68 | 1.6 | 93 |
| spin 4 rad/s approaching 7 m -> 3 m | 51.8 | 44.4 | 97.4 | 13.20 | 3.0 | 0.49 | 0.5 | 95 |
| variable spin 0.5..4.5 rad/s at 6 m | 69.9 | 73.2 | 94.4 | 12.60 | 4.4 | 0.53 | 0.5 | 96 |
| swing back and forth (+-3 rad/s, 4 s) at 6 m | 64.4 | 88.7 | 96.2 | 16.45 | 4.2 | 0.55 | 0.4 | 97 |
| spin 3 rad/s + gentle strafe 0.3 m / 3 s at 6 m | 74.9 | 67.0 | 79.3 | 11.30 | 4.4 | 0.57 | 0.4 | 95 |
