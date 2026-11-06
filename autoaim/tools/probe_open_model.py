#!/usr/bin/env python3
"""Sample an external four-corner armor model on recorded videos.

This is a detector probe, not a precision/recall or hit-rate evaluation. It
deliberately stays outside the production C++ detector and never alters caches.

Example:
  python tools/probe_open_model.py --model /tmp/Infantry-v8n.onnx \
      --video ../3.avi --video ../4.avi \
      --cache /path/to/3_detections.txt --cache /path/to/4_detections.txt
"""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import time

import cv2
import numpy as np
import onnxruntime as ort


def prepare(frame):
    height, width = frame.shape[:2]
    scale = min(480 / height, 640 / width)
    new_width = min(640, round(width * scale))
    new_height = min(480, round(height * scale))
    pad_x = (640 - new_width) // 2
    pad_y = (480 - new_height) // 2
    resized = cv2.resize(frame, (new_width, new_height))
    padded = cv2.copyMakeBorder(
        resized, pad_y, 480 - new_height - pad_y,
        pad_x, 640 - new_width - pad_x,
        cv2.BORDER_CONSTANT, value=(124, 124, 124),
    )
    rgb = cv2.cvtColor(padded, cv2.COLOR_BGR2RGB)
    blob = rgb.astype(np.float32).transpose(2, 0, 1)[None] / 255.0
    return blob, scale, pad_x, pad_y


def decode(output, shape, scale, pad_x, pad_y, confidence):
    if output.shape != (1, 21, 6300):
        raise ValueError(f"expected model output (1, 21, 6300), got {output.shape}")
    height, width = shape[:2]
    data = output[0]
    scores = data[4:13].max(axis=0)
    boxes, accepted_scores, labels = [], [], []
    for index in np.flatnonzero(scores >= confidence):
        color = int(data[:4, index].argmax())
        if color == 3:  # The reference decoder discards its fourth color class.
            continue
        armor_id = int(data[4:13, index].argmax())
        points = data[13:21, index].reshape(4, 2).copy()
        points[:, 0] = (points[:, 0] - pad_x) / scale
        points[:, 1] = (points[:, 1] - pad_y) / scale
        if not np.isfinite(points).all():
            continue
        if (points[:, 0] < 0).any() or (points[:, 0] > width).any():
            continue
        if (points[:, 1] < 0).any() or (points[:, 1] > height).any():
            continue
        boxes.append(cv2.boundingRect(points.astype(np.float32)))
        accepted_scores.append(float(scores[index]))
        labels.append((color, armor_id))
    if not boxes:
        return []
    kept = np.asarray(cv2.dnn.NMSBoxes(boxes, accepted_scores, confidence, 0.2)).reshape(-1)
    return [labels[int(index)] for index in kept]


def cache_counts(path):
    if path is None:
        return None
    counts = []
    with path.open(encoding="utf-8") as handle:
        for index, line in enumerate(handle):
            tokens = line.split(maxsplit=3)
            if len(tokens) < 3 or int(tokens[0]) != index:
                raise ValueError(f"invalid detection cache at frame {index}: {path}")
            counts.append(int(tokens[2]))
    return counts


def probe(session, video, baseline, interval_seconds, confidence):
    cap = cv2.VideoCapture(str(video))
    if not cap.isOpened():
        raise ValueError(f"cannot open video: {video}")
    fps = cap.get(cv2.CAP_PROP_FPS)
    total = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    if not np.isfinite(fps) or fps <= 0 or total <= 0:
        raise ValueError(f"invalid video FPS/frame count: {video}")
    step = max(1, round(fps * interval_seconds))
    count_hist, color_hist, class_hist = Counter(), Counter(), Counter()
    baseline_hist = Counter()
    missed = []
    inference_ms = []
    input_name = session.get_inputs()[0].name
    baseline_counts = cache_counts(baseline)
    if baseline_counts is not None and len(baseline_counts) != total:
        raise ValueError(f"baseline frame count mismatch: {baseline}")
    sampled = 0
    for frame_index in range(0, total, step):
        cap.set(cv2.CAP_PROP_POS_FRAMES, frame_index)
        ok, frame = cap.read()
        if not ok:
            raise ValueError(f"cannot read frame {frame_index}: {video}")
        blob, scale, pad_x, pad_y = prepare(frame)
        start = time.perf_counter()
        output = session.run(None, {input_name: blob})[0]
        inference_ms.append((time.perf_counter() - start) * 1000)
        labels = decode(output, frame.shape, scale, pad_x, pad_y, confidence)
        count_hist[len(labels)] += 1
        if not labels:
            missed.append(frame_index)
        for color, armor_id in labels:
            color_hist[color] += 1
            class_hist[armor_id] += 1
        if baseline_counts is not None:
            baseline_hist[baseline_counts[frame_index]] += 1
        sampled += 1
    cap.release()
    result = {
        "video": str(video), "frames": total, "fps": fps,
        "sample_interval_frames": step, "sampled_frames": sampled,
        "model_detection_counts": dict(sorted(count_hist.items())),
        "model_color_ids": dict(sorted(color_hist.items())),
        "model_armor_ids": dict(sorted(class_hist.items())),
        "model_no_detection_frames": missed,
        "inference_only_median_ms": round(float(np.median(inference_ms)), 2),
        "inference_only_p95_ms": round(float(np.percentile(inference_ms, 95)), 2),
    }
    if baseline_counts is not None:
        result["baseline_cache"] = str(baseline)
        result["baseline_detection_counts"] = dict(sorted(baseline_hist.items()))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", type=Path, required=True)
    parser.add_argument("--video", action="append", type=Path, required=True)
    parser.add_argument("--cache", action="append", type=Path, default=[])
    parser.add_argument("--interval-seconds", type=float, default=2.0)
    parser.add_argument("--confidence", type=float, default=0.5)
    args = parser.parse_args()
    if args.cache and len(args.cache) != len(args.video):
        parser.error("provide one --cache for every --video, in the same order")
    if args.interval_seconds <= 0 or not 0 < args.confidence < 1:
        parser.error("interval must be positive and confidence must be within (0, 1)")
    model_bytes = args.model.read_bytes()
    session = ort.InferenceSession(str(args.model), providers=["CPUExecutionProvider"])
    model_input = session.get_inputs()[0]
    if model_input.shape != [1, 3, 480, 640] or model_input.type != "tensor(float)":
        raise ValueError(f"unexpected model input: {model_input.shape}, {model_input.type}")
    result = {
        "model_sha256": hashlib.sha256(model_bytes).hexdigest(),
        "opencv": cv2.__version__, "onnxruntime": ort.__version__,
        "confidence": args.confidence, "nms_iou": 0.2,
        "videos": [
            probe(session, video, args.cache[index] if args.cache else None,
                  args.interval_seconds, args.confidence)
            for index, video in enumerate(args.video)
        ],
    }
    print(json.dumps(result, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
