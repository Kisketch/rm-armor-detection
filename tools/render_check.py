#!/usr/bin/env python3
"""把结果 txt 里的四点反渲染回原视频帧，生成人工抽检图。

用法:
    python3 tools/render_check.py <结果.txt> <视频> [--every N] [--frames 0,125,250] [--out verify]

每个抽样帧保存为 <out>/<视频名>_f00000.png：
- 绿框标出装甲板，角点按顺序着色（0 红=左下，1 绿=左上，2 蓝=右上，3 黄=右下）
- 左上角附 4 倍放大图与帧号/count/数字，用于核对贴合度、角点顺序与分类结果
- 空帧也会渲染（便于发现漏检）

依赖：opencv-python（cv2）。
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

import cv2
import numpy as np

LINE_RE = re.compile(r"^(?P<frame>\d+), count=(?P<count>\d+)(?P<rest>.*)$")
ARMOR_RE = re.compile(
    r"number=(?P<number>[^,\s]+), points="
    r"\((?P<x0>-?\d+(?:\.\d+)?),(?P<y0>-?\d+(?:\.\d+)?)\),"
    r"\((?P<x1>-?\d+(?:\.\d+)?),(?P<y1>-?\d+(?:\.\d+)?)\),"
    r"\((?P<x2>-?\d+(?:\.\d+)?),(?P<y2>-?\d+(?:\.\d+)?)\),"
    r"\((?P<x3>-?\d+(?:\.\d+)?),(?P<y3>-?\d+(?:\.\d+)?)\)"
)

# 角点颜色（BGR）：0 红(左下) 1 绿(左上) 2 蓝(右上) 3 黄(右下)
POINT_COLORS = [
    (0, 0, 255),
    (0, 255, 0),
    (255, 0, 0),
    (0, 255, 255),
]
BOX_COLOR = (0, 255, 0)
TEXT_COLOR = (255, 255, 255)


@dataclass
class Armor:
    number: str
    points: list[tuple[float, float]]


@dataclass
class Frame:
    index: int
    armors: list[Armor]


def parse(path: Path) -> dict[int, Frame]:
    frames: dict[int, Frame] = {}
    with path.open("r", encoding="utf-8", errors="replace") as fh:
        for line_no, raw in enumerate(fh, start=1):
            text = raw.rstrip("\r\n")
            m = LINE_RE.match(text)
            if not m:
                print(f"警告: 第 {line_no} 行无法解析，已跳过: {text!r}", file=sys.stderr)
                continue
            armors: list[Armor] = []
            for am in ARMOR_RE.finditer(m.group("rest")):
                pts = [(float(am.group(f"x{k}")), float(am.group(f"y{k}"))) for k in range(4)]
                armors.append(Armor(number=am.group("number"), points=pts))
            frames[int(m.group("frame"))] = Frame(index=int(m.group("frame")), armors=armors)
    return frames


def draw(frame: np.ndarray, rec: Frame, stem: str) -> np.ndarray:
    img = frame.copy()
    original = frame
    inset_x = 10

    for armor in rec.armors:
        poly = np.array([[int(round(x)), int(round(y))] for x, y in armor.points], np.int32)
        cv2.polylines(img, [poly], True, BOX_COLOR, 2)
        for k in range(4):
            cv2.circle(img, tuple(poly[k]), 4, POINT_COLORS[k], -1)
        cv2.putText(
            img,
            armor.number,
            (poly[1][0], poly[1][1] - 8),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.8,
            BOX_COLOR,
            2,
        )

        # 左上角贴 4 倍放大局部图
        x, y, w, h = cv2.boundingRect(poly)
        pad = max(w, h) // 2 + 10
        x0, y0 = max(x - pad, 0), max(y - pad, 0)
        x1 = min(x + w + pad, original.shape[1])
        y1 = min(y + h + pad, original.shape[0])
        if x1 <= x0 or y1 <= y0:
            continue
        crop = cv2.resize(
            original[y0:y1, x0:x1], None, fx=4.0, fy=4.0, interpolation=cv2.INTER_LINEAR
        )
        local = np.array(
            [[int(round((px - x0) * 4)), int(round((py - y0) * 4))] for px, py in armor.points],
            np.int32,
        )
        cv2.polylines(crop, [local], True, BOX_COLOR, 1)
        for k in range(4):
            cv2.circle(crop, tuple(local[k]), 5, POINT_COLORS[k], -1)
        if inset_x + crop.shape[1] > img.shape[1] or 60 + crop.shape[0] > img.shape[0]:
            continue
        img[60 : 60 + crop.shape[0], inset_x : inset_x + crop.shape[1]] = crop
        cv2.rectangle(
            img,
            (inset_x, 60),
            (inset_x + crop.shape[1], 60 + crop.shape[0]),
            TEXT_COLOR,
            1,
        )
        inset_x += crop.shape[1] + 10

    header = f"{stem} frame {rec.index}  count={len(rec.armors)}"
    cv2.putText(img, header, (10, 40), cv2.FONT_HERSHEY_SIMPLEX, 1.0, TEXT_COLOR, 2)
    return img


def main() -> int:
    ap = argparse.ArgumentParser(description="把结果 txt 反渲染回视频帧")
    ap.add_argument("result", type=Path, help="结果 txt，例如 3.txt")
    ap.add_argument("video", type=Path, help="对应视频，例如 video/3.avi")
    ap.add_argument("--every", type=int, default=125, help="每 N 帧抽一张，默认 125（5 秒）")
    ap.add_argument("--frames", default="", help="指定帧号，逗号分隔；给出时忽略 --every")
    ap.add_argument("--out", type=Path, default=Path("verify"), help="输出目录，默认 verify/")
    args = ap.parse_args()

    frames = parse(args.result)
    if not frames:
        print(f"结果文件为空或无法解析: {args.result}", file=sys.stderr)
        return 1

    if args.frames.strip():
        wanted = sorted({int(x) for x in args.frames.replace(" ", "").split(",") if x})
    else:
        wanted = sorted(f for f in frames if f % max(args.every, 1) == 0)

    cap = cv2.VideoCapture(str(args.video))
    if not cap.isOpened():
        print(f"无法打开视频: {args.video}", file=sys.stderr)
        return 1

    args.out.mkdir(parents=True, exist_ok=True)
    stem = args.video.stem
    saved = 0
    for idx in wanted:
        rec = frames.get(idx)
        if rec is None:
            print(f"警告: 结果里没有帧 {idx}，跳过", file=sys.stderr)
            continue
        cap.set(cv2.CAP_PROP_POS_FRAMES, idx)
        ok, frame = cap.read()
        if not ok:
            print(f"警告: 无法读取视频帧 {idx}", file=sys.stderr)
            continue
        out_path = args.out / f"{stem}_f{idx:05d}.png"
        cv2.imwrite(str(out_path), draw(frame, rec, stem))
        saved += 1
    cap.release()

    print(f"rendered={saved} -> {args.out}/")
    return 0 if saved else 1


if __name__ == "__main__":
    sys.exit(main())
