#!/usr/bin/env python3
"""Analyze ESP32 telemetry receiver logs.

This script is intentionally dependency-free (stdlib only).

Expected input: CSV with a header row.
Recommended columns (you can rename via CLI flags):
  - rx_t_us: local monotonic receive timestamp in microseconds (ESP32 esp_timer_get_time() / Arduino micros())
  - seq: telemetry frame sequence number (uint16)
  - crc_ok: 1 if frame CRC/parse ok, else 0 (optional)

Example CSV row:
  rx_t_us,seq,crc_ok,frame_ts_ms,reserved1_us,reserved4_100us
  123456789,42,1,98765,3200,18

The script computes inter-arrival (dt) statistics and estimated drops from seq gaps.
"""

from __future__ import annotations

import argparse
import csv
import math
import statistics
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional


@dataclass
class Stats:
    count: int
    duration_s: float
    mean_ms: float
    stdev_ms: float
    min_ms: float
    p50_ms: float
    p90_ms: float
    p99_ms: float
    max_ms: float


def _percentile(sorted_values: List[float], p: float) -> float:
    if not sorted_values:
        return float("nan")
    if p <= 0:
        return sorted_values[0]
    if p >= 100:
        return sorted_values[-1]
    k = (len(sorted_values) - 1) * (p / 100.0)
    f = math.floor(k)
    c = math.ceil(k)
    if f == c:
        return sorted_values[int(k)]
    d0 = sorted_values[f] * (c - k)
    d1 = sorted_values[c] * (k - f)
    return d0 + d1


def _compute_stats(dt_ms: List[float], duration_s: float) -> Stats:
    dt_sorted = sorted(dt_ms)
    mean_ms = statistics.fmean(dt_ms) if dt_ms else float("nan")
    stdev_ms = statistics.pstdev(dt_ms) if len(dt_ms) >= 2 else 0.0
    return Stats(
        count=len(dt_ms),
        duration_s=duration_s,
        mean_ms=mean_ms,
        stdev_ms=stdev_ms,
        min_ms=(min(dt_ms) if dt_ms else float("nan")),
        p50_ms=_percentile(dt_sorted, 50),
        p90_ms=_percentile(dt_sorted, 90),
        p99_ms=_percentile(dt_sorted, 99),
        max_ms=(max(dt_ms) if dt_ms else float("nan")),
    )


def _histogram(dt_ms: List[float], bin_ms: float) -> Dict[int, int]:
    hist: Dict[int, int] = {}
    if bin_ms <= 0:
        return hist
    for v in dt_ms:
        b = int(math.floor(v / bin_ms))
        hist[b] = hist.get(b, 0) + 1
    return dict(sorted(hist.items(), key=lambda kv: kv[0]))


def _read_rows(path: Path) -> List[Dict[str, str]]:
    with path.open("r", newline="") as f:
        reader = csv.DictReader(f)
        if reader.fieldnames is None:
            raise SystemExit("CSV must have a header row")
        return list(reader)


def _parse_int(row: Dict[str, str], key: str) -> Optional[int]:
    if key not in row:
        return None
    s = (row.get(key) or "").strip()
    if s == "":
        return None
    return int(s, 0)


def main() -> int:
    ap = argparse.ArgumentParser(description="Analyze telemetry receiver inter-arrival times and drops")
    ap.add_argument("--csv", required=True, help="Input CSV path (must have header)")

    ap.add_argument("--time-col", default="rx_t_us", help="Receive timestamp column name")
    ap.add_argument("--time-unit", choices=["us", "ms"], default="us", help="Unit of time-col")

    ap.add_argument("--seq-col", default="seq", help="Sequence column name")
    ap.add_argument("--seq-mod", type=int, default=65536, help="Sequence wrap modulus (default: 65536 for uint16)")

    ap.add_argument("--crc-col", default="crc_ok", help="CRC ok column name (optional)")
    ap.add_argument("--only-ok", action="store_true", help="Only analyze rows with crc_ok==1 (if column exists)")

    ap.add_argument("--expected-period-ms", type=float, default=20.0, help="Expected frame period (ms)")
    ap.add_argument("--hist-bin-ms", type=float, default=1.0, help="Histogram bin width (ms)")
    ap.add_argument("--out-hist-csv", default="", help="Optional output histogram CSV path")

    args = ap.parse_args()

    path = Path(args.csv)
    try:
        rows = _read_rows(path)
    except FileNotFoundError:
        print(f"error: file not found: {path}")
        return 2
    except Exception as e:
        print(f"error: failed to read CSV: {path}: {e}")
        return 2

    rx_t_us: List[int] = []
    seq: List[int] = []

    has_crc = False
    for row in rows:
        crc_ok = _parse_int(row, args.crc_col)
        if crc_ok is not None:
            has_crc = True
            if args.only_ok and crc_ok != 1:
                continue

        t = _parse_int(row, args.time_col)
        s = _parse_int(row, args.seq_col)
        if t is None or s is None:
            continue

        if args.time_unit == "ms":
            t = int(t * 1000)

        rx_t_us.append(int(t))
        seq.append(int(s))

    if len(rx_t_us) < 2:
        raise SystemExit("Not enough valid rows (need at least 2 with timestamp+seq)")

    # Ensure monotonic order by timestamp.
    pairs = sorted(zip(rx_t_us, seq), key=lambda p: p[0])
    rx_t_us = [p[0] for p in pairs]
    seq = [p[1] for p in pairs]

    dt_ms: List[float] = []
    for i in range(1, len(rx_t_us)):
        dt_us = rx_t_us[i] - rx_t_us[i - 1]
        if dt_us <= 0:
            continue
        dt_ms.append(dt_us / 1000.0)

    duration_s = (rx_t_us[-1] - rx_t_us[0]) / 1_000_000.0
    stats = _compute_stats(dt_ms, duration_s)

    # Drop estimation from seq gaps.
    drops = 0
    seq_mod = int(args.seq_mod)
    if seq_mod <= 1:
        seq_mod = 65536

    out_of_order = 0
    for i in range(1, len(seq)):
        prev = seq[i - 1] % seq_mod
        cur = seq[i] % seq_mod
        diff = (cur - prev) % seq_mod
        if diff == 0:
            out_of_order += 1
            continue
        if diff > 1:
            drops += (diff - 1)

    expected = float(args.expected_period_ms)
    est_rate_hz = (1.0 / (stats.mean_ms / 1000.0)) if stats.mean_ms and stats.mean_ms > 0 else float("nan")

    print(f"file: {path}")
    print(f"rows_used: {len(seq)}" + (" (filtered ok-only)" if args.only_ok and has_crc else ""))
    print(f"duration_s: {stats.duration_s:.3f}")
    print(
        "dt_ms: "
        f"mean={stats.mean_ms:.3f} stdev={stats.stdev_ms:.3f} "
        f"min={stats.min_ms:.3f} p50={stats.p50_ms:.3f} p90={stats.p90_ms:.3f} p99={stats.p99_ms:.3f} max={stats.max_ms:.3f}"
    )
    print(f"rate_hz: {est_rate_hz:.2f} (expected {1000.0/expected:.2f})")
    print(f"seq_drops_est: {drops}")
    if out_of_order:
        print(f"seq_out_of_order_or_duplicates: {out_of_order}")

    hist = _histogram(dt_ms, args.hist_bin_ms)
    if hist:
        print(f"histogram (bin={args.hist_bin_ms}ms):")
        # Print a compact view around expected period.
        center = int(math.floor(expected / args.hist_bin_ms))
        for b in range(max(0, center - 10), center + 11):
            lo = b * args.hist_bin_ms
            hi = (b + 1) * args.hist_bin_ms
            cnt = hist.get(b, 0)
            if cnt:
                print(f"  [{lo:6.1f},{hi:6.1f}) ms: {cnt}")

    if args.out_hist_csv:
        out_path = Path(args.out_hist_csv)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        with out_path.open("w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["bin_index", "bin_lo_ms", "bin_hi_ms", "count"])
            for b, cnt in hist.items():
                lo = b * args.hist_bin_ms
                hi = (b + 1) * args.hist_bin_ms
                w.writerow([b, f"{lo:.3f}", f"{hi:.3f}", cnt])
        print(f"wrote: {out_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
