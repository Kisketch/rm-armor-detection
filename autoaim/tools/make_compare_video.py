#!/usr/bin/env python3
"""Build the "original vs detection+aim" comparison clips and stitch them.

Left half: the raw video frame. Right half: the autoaim HUD (detection boxes,
modelled plates, aim point, hit marker, top view). Both halves also get a
burned-in frame number so a still can be traced back to the source.
"""

import subprocess
import sys
from pathlib import Path

import cv2
import numpy as np

FFMPEG = r"C:\Users\Lgnatius\AppData\Local\Programs\yt-dlp\ffmpeg.exe"
ROOT = Path(r"C:\Users\Lgnatius\Desktop\RM\autoaim-takeover")
OUT = Path(r"C:\Users\Lgnatius\Desktop\RM\光点与自瞄对比_2026-11-05")
TMP = ROOT / "runs" / "cmp_build"

PANEL_W, PANEL_H = 960, 720
BAR_H = 34
FONT = cv2.FONT_HERSHEY_SIMPLEX
GAMMA = 1.6  # display-only lift; both panels get the same curve, detection untouched


def lift(img: np.ndarray) -> np.ndarray:
    lut = np.array([((i / 255.0) ** (1.0 / GAMMA)) * 255 for i in range(256)], np.uint8)
    return cv2.LUT(img, lut)


def compose(original: Path, overlay: Path, out_avi: Path, start_frame: int) -> int:
    cap_o = cv2.VideoCapture(str(original))
    cap_a = cv2.VideoCapture(str(overlay))
    if not cap_o.isOpened() or not cap_a.isOpened():
        raise SystemExit(f"cannot open {original} or {overlay}")
    fps = cap_a.get(cv2.CAP_PROP_FPS) or 25.0
    writer = cv2.VideoWriter(str(out_avi), cv2.VideoWriter_fourcc(*"MJPG"), fps,
                             (PANEL_W * 2, PANEL_H + BAR_H))
    n = 0
    while True:
        ok_o, fo = cap_o.read()
        ok_a, fa = cap_a.read()
        if not (ok_o and ok_a):
            break
        lo = lift(cv2.resize(fo, (PANEL_W, PANEL_H), interpolation=cv2.INTER_AREA))
        ra = lift(cv2.resize(fa, (PANEL_W, PANEL_H), interpolation=cv2.INTER_AREA))
        bar = np.full((BAR_H, PANEL_W * 2, 3), 24, np.uint8)
        cv2.putText(bar, "ORIGINAL  (no overlay)", (10, 24), FONT, 0.7, (210, 210, 210), 2)
        cv2.putText(bar, "DETECTION + AIM  (green=armor, red O=aim, x=impact)", (PANEL_W + 10, 24),
                    FONT, 0.7, (120, 255, 120), 2)
        idx = start_frame + n
        stamp = f"f{idx}  t={idx / fps:5.2f}s  gamma {GAMMA:g}"
        cv2.putText(bar, stamp, (PANEL_W - 290, 24), FONT, 0.62, (90, 200, 255), 2)
        frame = np.vstack([bar, np.hstack([lo, ra])])
        # thin divider between the two panels
        cv2.line(frame, (PANEL_W, BAR_H), (PANEL_W, BAR_H + PANEL_H), (60, 60, 60), 2)
        writer.write(frame)
        n += 1
    cap_o.release()
    cap_a.release()
    writer.release()
    return n


def encode(avi: Path, mp4: Path) -> None:
    subprocess.run([FFMPEG, "-y", "-loglevel", "error", "-i", str(avi),
                    "-c:v", "libx264", "-preset", "medium", "-crf", "22",
                    "-pix_fmt", "yuv420p", "-movflags", "+faststart", str(mp4)], check=True)


def concat(parts: list[Path], out: Path) -> None:
    lst = TMP / "concat.txt"
    lst.write_text("".join(f"file '{p.as_posix()}'\n" for p in parts), encoding="utf-8")
    subprocess.run([FFMPEG, "-y", "-loglevel", "error", "-f", "concat", "-safe", "0",
                    "-i", str(lst), "-c", "copy", "-movflags", "+faststart", str(out)], check=True)


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    TMP.mkdir(parents=True, exist_ok=True)
    full = "--full" in sys.argv
    if full:
        jobs = [
            ("3", ROOT / "../template/template/video/3.avi", ROOT / "runs/cmp3/3_overlay_0_2208.avi",
             0, "06_全片_3号_原画vs自瞄.mp4"),
            ("4", ROOT / "../template/template/video/4.avi", ROOT / "runs/cmp4/4_overlay_0_3415.avi",
             0, "07_全片_4号_原画vs自瞄.mp4"),
        ]
    else:
        jobs = [
            ("3", ROOT / "../template/template/video/3.avi", ROOT / "runs/cmp3/3_overlay_850_1000.avi",
             850, "01_3号视频_原画vs自瞄.mp4"),
            ("4", ROOT / "../template/template/video/4.avi", ROOT / "runs/cmp4/4_overlay_2625_2775.avi",
             2625, "02_4号视频_原画vs自瞄.mp4"),
        ]
    made = []
    for tag, original, overlay, start, name in jobs:
        avi = TMP / f"cmp_{tag}.avi"
        frames = compose(original.resolve(), overlay, avi, start)
        mp4 = OUT / name
        encode(avi, mp4)
        size = mp4.stat().st_size / 1024
        print(f"{name}: {frames} frames, {size / 1024:.1f} MB")
        made.append(mp4)
    if full:
        combined = OUT / "08_全片合集_3号+4号_原画vs自瞄.mp4"
        concat(made, combined)
        print(f"{combined.name}: {combined.stat().st_size / 1024 / 1024:.1f} MB")
    else:
        combined = OUT / "03_合集_3号+4号_原画vs自瞄.mp4"
        concat(made, combined)
        print(f"{combined.name}: {combined.stat().st_size / 1024:.0f} KB")
    return 0


if __name__ == "__main__":
    sys.exit(main())
