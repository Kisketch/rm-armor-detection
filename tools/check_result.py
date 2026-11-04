#!/usr/bin/env python3
"""结果文件自检：检查行数、帧号连续性、格式、坐标合法性与统计信息。

用法:
    python3 tools/check_result.py <结果.txt> <视频> [--frame-base N] [--max-errors N]

输出每一项检查的 PASS/FAIL（任务书 §8.1 的 7 项），最后给出汇总统计。
退出码 0 表示全部通过，1 表示存在错误（打印前 N 条错误原文）。

依赖：opencv-python（cv2）。其余只用标准库。
"""

from __future__ import annotations

import argparse
import re
import sys
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path

import cv2

# 允许的数字类别（与 model/label.txt 一致，unknown 用于未启用分类的情况）
ALLOWED_NUMBERS = {"1", "2", "3", "4", "outpost", "guard", "base", "unknown"}

# 一行： "<frame>, count=<N>" 后可跟若干装甲板
LINE_RE = re.compile(r"^(?P<frame>\d+), count=(?P<count>\d+)(?P<rest>.*)$")
# 单个装甲板： "armor1: number=3, points=(x,y),(x,y),(x,y),(x,y)"
ARMOR_RE = re.compile(
    r"armor(?P<idx>\d+): number=(?P<number>[^,\s]+), points="
    r"\((?P<x0>-?\d+(?:\.\d+)?),(?P<y0>-?\d+(?:\.\d+)?)\),"
    r"\((?P<x1>-?\d+(?:\.\d+)?),(?P<y1>-?\d+(?:\.\d+)?)\),"
    r"\((?P<x2>-?\d+(?:\.\d+)?),(?P<y2>-?\d+(?:\.\d+)?)\),"
    r"\((?P<x3>-?\d+(?:\.\d+)?),(?P<y3>-?\d+(?:\.\d+)?)\)"
)


@dataclass
class Armor:
    number: str
    points: list[tuple[float, float]]


@dataclass
class Frame:
    index: int
    declared_count: int
    armors: list[Armor] = field(default_factory=list)
    raw: str = ""


def shoelace_area(points: list[tuple[float, float]]) -> float:
    """四边形面积（鞋带公式），用于判断四点是否退化。"""
    area = 0.0
    for i in range(len(points)):
        x1, y1 = points[i]
        x2, y2 = points[(i + 1) % len(points)]
        area += x1 * y2 - x2 * y1
    return abs(area) * 0.5


def parse_result(path: Path, frame_base: int, max_errors: int):
    frames: list[Frame] = []
    errors: list[str] = []
    number_hist: Counter[str] = Counter()
    bad_parse = 0
    bad_leftover = 0

    with path.open("r", encoding="utf-8", errors="replace") as fh:
        for line_no, raw in enumerate(fh, start=1):
            text = raw.rstrip("\r\n")
            m = LINE_RE.match(text)
            if not m:
                bad_parse += 1
                if len(errors) < max_errors:
                    errors.append(f"第 {line_no} 行无法解析: {text!r}")
                continue

            frame_index = int(m.group("frame"))
            declared = int(m.group("count"))
            rest = m.group("rest")
            armors: list[Armor] = []

            for am in ARMOR_RE.finditer(rest):
                pts = [
                    (float(am.group(f"x{k}")), float(am.group(f"y{k}"))) for k in range(4)
                ]
                armors.append(Armor(number=am.group("number"), points=pts))
                number_hist[am.group("number")] += 1

            # 行内是否还有未被 ARMOR_RE 吃掉的内容（分隔符本身不算）
            leftover = ARMOR_RE.sub("", rest)
            leftover = leftover.replace("|", "").replace(",", "").strip()
            if leftover:
                bad_leftover += 1
                if len(errors) < max_errors:
                    errors.append(f"第 {line_no} 行存在无法解析的片段: {leftover!r}")

            frames.append(
                Frame(index=frame_index, declared_count=declared, armors=armors, raw=text)
            )

    return frames, errors, number_hist, bad_parse, bad_leftover


def main() -> int:
    ap = argparse.ArgumentParser(description="装甲板结果文件自检")
    ap.add_argument("result", type=Path, help="结果 txt，例如 3.txt")
    ap.add_argument("video", type=Path, help="对应视频，例如 video/3.avi")
    ap.add_argument("--frame-base", type=int, default=0, help="帧号基数，默认 0")
    ap.add_argument("--max-errors", type=int, default=20, help="最多打印多少条错误")
    ap.add_argument("--json", type=Path, default=None, help="可选：把统计结果写成 JSON")
    args = ap.parse_args()

    if not args.result.is_file():
        print(f"找不到结果文件: {args.result}", file=sys.stderr)
        return 1
    cap = cv2.VideoCapture(str(args.video))
    if not cap.isOpened():
        print(f"无法打开视频: {args.video}", file=sys.stderr)
        return 1
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    cap.release()

    frames, errors, number_hist, bad_parse, bad_leftover = parse_result(
        args.result, args.frame_base, args.max_errors
    )
    checks: list[tuple[str, bool, str]] = []

    # 1. 行数 == 视频帧数
    checks.append(
        (
            "行数 == 视频帧数",
            len(frames) == total_frames,
            f"lines={len(frames)} video_frames={total_frames}",
        )
    )

    # 2. 帧号从 frame_base 起严格 +1
    frame_seq_ok = True
    seq_detail = ""
    for i, fr in enumerate(frames):
        expect = args.frame_base + i
        if fr.index != expect:
            frame_seq_ok = False
            seq_detail = f"第 {i + 1} 行帧号 {fr.index}，应为 {expect}"
            errors.append(seq_detail)
            break
    checks.append(
        (
            f"帧号从 {args.frame_base} 起严格 +1",
            frame_seq_ok,
            seq_detail or f"range=[{args.frame_base}, {args.frame_base + len(frames) - 1}]",
        )
    )

    # 3. 每行可完整解析
    checks.append(
        (
            "每行可完整解析",
            bad_parse == 0 and bad_leftover == 0,
            f"unparsable_lines={bad_parse} stray_fragments={bad_leftover}",
        )
    )

    # 4. count 与条目数一致；每块装甲板恰好 4 点
    count_mismatch = [fr for fr in frames if fr.declared_count != len(fr.armors)]
    points_ok = all(len(a.points) == 4 for fr in frames for a in fr.armors)
    checks.append(
        (
            "count 与条目数一致",
            not count_mismatch,
            f"mismatch_lines={len(count_mismatch)}"
            + (f" first=frame {count_mismatch[0].index}" if count_mismatch else ""),
        )
    )
    checks.append(
        (
            "每块装甲板恰好 4 点",
            points_ok,
            "正则保证 4 点" if points_ok else "存在非 4 点条目",
        )
    )

    # 5. 坐标在画面内 + 四点不退化
    out_of_bounds = 0
    degenerate = 0
    for fr in frames:
        for armor in fr.armors:
            for x, y in armor.points:
                if not (0.0 <= x <= width and 0.0 <= y <= height):
                    out_of_bounds += 1
                    if len(errors) < args.max_errors:
                        errors.append(f"帧 {fr.index}: 坐标越界 ({x}, {y}) 画面 {width}x{height}")
            if shoelace_area(armor.points) < 1.0:
                degenerate += 1
                if len(errors) < args.max_errors:
                    errors.append(f"帧 {fr.index}: 四点退化（面积≈0）")
    checks.append(
        (
            "坐标在画面范围内",
            out_of_bounds == 0,
            f"out_of_bounds={out_of_bounds} frame={width}x{height}",
        )
    )
    checks.append(("四点不退化", degenerate == 0, f"degenerate={degenerate}"))

    # 6. number 合法
    illegal = {k: v for k, v in number_hist.items() if k not in ALLOWED_NUMBERS}
    checks.append(
        (
            "number 在允许集合内",
            not illegal,
            f"illegal={illegal}" if illegal else f"classes={sorted(number_hist)}",
        )
    )

    # 统计
    n = len(frames)
    total_armors = sum(len(fr.armors) for fr in frames)
    frames_with_armor = sum(1 for fr in frames if fr.armors)
    max_count = max((len(fr.armors) for fr in frames), default=0)

    longest_zero = current_zero = 0
    longest_zero_start = zero_start = -1
    for fr in frames:
        if not fr.armors:
            if current_zero == 0:
                zero_start = fr.index
            current_zero += 1
            if current_zero > longest_zero:
                longest_zero, longest_zero_start = current_zero, zero_start
        else:
            current_zero = 0

    max_jump, max_jump_frame = 0, -1
    for prev, cur in zip(frames, frames[1:]):
        jump = abs(len(cur.armors) - len(prev.armors))
        if jump > max_jump:
            max_jump, max_jump_frame = jump, cur.index

    ratio = 100.0 * frames_with_armor / n if n else 0.0
    mean_count = total_armors / n if n else 0.0

    print(f"file={args.result} video={args.video}")
    for name, ok, detail in checks:
        print(f"[{'PASS' if ok else 'FAIL'}] {name}  ({detail})")
    print()
    print(f"video_frames={total_frames} lines={n}")
    print(f"frames_with_armor={frames_with_armor} ({ratio:.2f}%)")
    print(f"total_armors={total_armors} mean_count={mean_count:.4f} max_count={max_count}")
    print(f"longest_zero_run={longest_zero} (from frame {longest_zero_start})")
    print(f"max_adjacent_count_jump={max_jump} (at frame {max_jump_frame})")
    print("numbers: " + " ".join(f"{k}={v}" for k, v in sorted(number_hist.items())))

    ok = all(item[1] for item in checks)
    if errors:
        print()
        for err in errors[: args.max_errors]:
            print(f"  ERROR {err}")
        if len(errors) > args.max_errors:
            print(f"  ... 其余 {len(errors) - args.max_errors} 条省略")
    print()
    print("RESULT: PASS" if ok else "RESULT: FAIL")

    if args.json:
        import json

        args.json.write_text(
            json.dumps(
                {
                    "file": str(args.result),
                    "video": str(args.video),
                    "video_frames": total_frames,
                    "lines": n,
                    "frames_with_armor": frames_with_armor,
                    "frames_with_armor_ratio": ratio,
                    "total_armors": total_armors,
                    "mean_count": mean_count,
                    "max_count": max_count,
                    "longest_zero_run": longest_zero,
                    "longest_zero_run_from": longest_zero_start,
                    "max_adjacent_count_jump": max_jump,
                    "numbers": dict(number_hist),
                    "checks": [{"name": c[0], "pass": c[1], "detail": c[2]} for c in checks],
                    "pass": ok,
                },
                ensure_ascii=False,
                indent=2,
            ),
            encoding="utf-8",
        )
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
