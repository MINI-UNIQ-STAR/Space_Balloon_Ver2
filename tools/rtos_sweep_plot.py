#!/usr/bin/env python3
"""Plot RT WCET safety margin vs ISR tax (no-hardware).

Outputs
- CSV: tools/out/rtos_wcet_vs_isr.csv
- SVG: tools/out/rtos_wcet_vs_isr.svg

Why SVG?
- Avoids extra deps (matplotlib) and works in any environment.

This script imports the simulator from tools/rtos_deadline_sim.py.
"""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path
import sys
from typing import Iterable


TOOLS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS_DIR))

import rtos_deadline_sim  # noqa: E402


@dataclass(frozen=True)
class SweepPoint:
    isr_tax_us: int
    isr_percent: float
    max_rt_only_ok_us: int | None
    max_all_ok_us: int | None


def parse_int_list(s: str) -> list[int]:
    out: list[int] = []
    for part in s.split(","):
        part = part.strip()
        if not part:
            continue
        out.append(int(part))
    if not out:
        raise ValueError("empty list")
    return out


def compute_thresholds(
    *,
    sim_ms: int,
    quantum_us: int,
    rt_period_ms: int,
    rt_deadline_ms: int,
    sensor_period_ms: int,
    sensor_deadline_ms: int,
    sensor_wcet_us: int,
    system_period_ms: int,
    system_deadline_ms: int,
    system_wcet_us: int,
    sweep_start_us: int,
    sweep_end_us: int,
    sweep_step_us: int,
    isr_tax_period_ms: int,
    isr_tax_us: int,
) -> tuple[int | None, int | None]:
    base_tasks = [
        rtos_deadline_sim.TaskCfg(
            name="RealTime",
            period_ms=rt_period_ms,
            deadline_ms=rt_deadline_ms,
            wcet_us=sweep_start_us,
            priority=5,
        ),
        rtos_deadline_sim.TaskCfg(
            name="Sensor",
            period_ms=sensor_period_ms,
            deadline_ms=sensor_deadline_ms,
            wcet_us=sensor_wcet_us,
            priority=3,
        ),
        rtos_deadline_sim.TaskCfg(
            name="System",
            period_ms=system_period_ms,
            deadline_ms=system_deadline_ms,
            wcet_us=system_wcet_us,
            priority=1,
        ),
    ]

    def tasks_with_rt_wcet(rt_wcet_us: int) -> list[rtos_deadline_sim.TaskCfg]:
        out: list[rtos_deadline_sim.TaskCfg] = []
        for t in base_tasks:
            if t.name == "RealTime":
                out.append(
                    rtos_deadline_sim.TaskCfg(
                        name=t.name,
                        period_ms=t.period_ms,
                        deadline_ms=t.deadline_ms,
                        wcet_us=rt_wcet_us,
                        priority=t.priority,
                    )
                )
            else:
                out.append(t)
        return out

    max_rt_only_ok: int | None = None
    max_all_ok: int | None = None

    wcet = sweep_start_us
    while wcet <= sweep_end_us:
        res = rtos_deadline_sim.simulate(
            sim_ms=sim_ms,
            quantum_us=quantum_us,
            tasks=tasks_with_rt_wcet(wcet),
            isr_tax_period_ms=isr_tax_period_ms,
            isr_tax_us=isr_tax_us,
        )
        rt_miss = res.misses.get("RealTime", 0)
        sens_miss = res.misses.get("Sensor", 0)
        sys_miss = res.misses.get("System", 0)

        if rt_miss == 0:
            max_rt_only_ok = wcet
            if sens_miss == 0 and sys_miss == 0:
                max_all_ok = wcet

        wcet += sweep_step_us

    return max_rt_only_ok, max_all_ok


def svg_escape(s: str) -> str:
    return (
        s.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
        .replace("'", "&#39;")
    )


def write_svg(
    *,
    out_path: Path,
    points: list[SweepPoint],
    title: str,
    y_max_us: int,
) -> None:
    width, height = 900, 520
    margin_l, margin_r, margin_t, margin_b = 70, 20, 55, 70
    plot_w = width - margin_l - margin_r
    plot_h = height - margin_t - margin_b

    xs = [p.isr_percent for p in points]
    x_min = min(xs)
    x_max = max(xs)
    if x_max == x_min:
        x_max = x_min + 1.0

    y_min = 0
    y_max = y_max_us

    def x_to_px(x: float) -> float:
        return margin_l + (x - x_min) * plot_w / (x_max - x_min)

    def y_to_px(y: float) -> float:
        return margin_t + (y_max - y) * plot_h / (y_max - y_min)

    def polyline(points_xy: Iterable[tuple[float, float]]) -> str:
        return " ".join(f"{x_to_px(x):.2f},{y_to_px(y):.2f}" for x, y in points_xy)

    # Build data series (us). Skip None values for polylines.
    rt_only_xy = [(p.isr_percent, float(p.max_rt_only_ok_us)) for p in points if p.max_rt_only_ok_us is not None]
    all_ok_xy = [(p.isr_percent, float(p.max_all_ok_us)) for p in points if p.max_all_ok_us is not None]

    # Ticks
    x_ticks = sorted(set(xs))
    y_ticks = [0, 5000, 10000, 15000, 20000]
    y_ticks = [t for t in y_ticks if t <= y_max_us]
    if y_max_us not in y_ticks:
        y_ticks.append(y_max_us)

    lines: list[str] = []
    lines.append(f"<svg xmlns='http://www.w3.org/2000/svg' width='{width}' height='{height}' viewBox='0 0 {width} {height}'>")
    lines.append("<rect x='0' y='0' width='100%' height='100%' fill='white' />")

    # Title
    lines.append(
        f"<text x='{width/2:.0f}' y='28' text-anchor='middle' font-family='sans-serif' font-size='16'>"
        f"{svg_escape(title)}"
        "</text>"
    )

    # Plot area
    x0, y0 = margin_l, margin_t
    lines.append(f"<rect x='{x0}' y='{y0}' width='{plot_w}' height='{plot_h}' fill='none' stroke='#333' stroke-width='1' />")

    # Grid + y labels
    for yt in y_ticks:
        y = y_to_px(float(yt))
        lines.append(f"<line x1='{x0}' y1='{y:.2f}' x2='{x0 + plot_w}' y2='{y:.2f}' stroke='#e5e5e5' stroke-width='1' />")
        lines.append(
            f"<text x='{x0 - 8}' y='{y + 4:.2f}' text-anchor='end' font-family='sans-serif' font-size='12' fill='#333'>"
            f"{yt/1000:.1f}ms"
            "</text>"
        )

    # X labels
    for xt in x_ticks:
        x = x_to_px(float(xt))
        lines.append(f"<line x1='{x:.2f}' y1='{y0}' x2='{x:.2f}' y2='{y0 + plot_h}' stroke='#f0f0f0' stroke-width='1' />")
        lines.append(
            f"<text x='{x:.2f}' y='{y0 + plot_h + 22}' text-anchor='middle' font-family='sans-serif' font-size='12' fill='#333'>"
            f"{xt:.1f}%"
            "</text>"
        )

    # Axis labels
    lines.append(
        f"<text x='{x0 + plot_w/2:.0f}' y='{height - 22}' text-anchor='middle' font-family='sans-serif' font-size='13' fill='#333'>"
        "ISR load (approx, % CPU)"
        "</text>"
    )
    # Y label (rotated)
    lines.append(
        f"<text x='18' y='{y0 + plot_h/2:.0f}' text-anchor='middle' font-family='sans-serif' font-size='13' fill='#333' transform='rotate(-90 18 {y0 + plot_h/2:.0f})'>"
        "Max safe RealTime WCET"
        "</text>"
    )

    # Lines
    if all_ok_xy:
        lines.append(
            f"<polyline fill='none' stroke='#1f77b4' stroke-width='2.5' points='{polyline(all_ok_xy)}' />"
        )
    if rt_only_xy:
        lines.append(
            f"<polyline fill='none' stroke='#ff7f0e' stroke-width='2.5' points='{polyline(rt_only_xy)}' />"
        )

    # Markers
    def draw_markers(series: list[tuple[float, float]], color: str) -> None:
        for x, y in series:
            lines.append(f"<circle cx='{x_to_px(x):.2f}' cy='{y_to_px(y):.2f}' r='3.2' fill='{color}' />")

    draw_markers(all_ok_xy, "#1f77b4")
    draw_markers(rt_only_xy, "#ff7f0e")

    # Legend
    lx, ly = x0 + 12, y0 + 12
    lines.append(f"<rect x='{lx}' y='{ly}' width='260' height='52' fill='white' stroke='#ddd' />")
    lines.append(f"<line x1='{lx+12}' y1='{ly+18}' x2='{lx+42}' y2='{ly+18}' stroke='#1f77b4' stroke-width='3' />")
    lines.append(f"<text x='{lx+52}' y='{ly+22}' font-family='sans-serif' font-size='12' fill='#333'>All tasks misses=0</text>")
    lines.append(f"<line x1='{lx+12}' y1='{ly+38}' x2='{lx+42}' y2='{ly+38}' stroke='#ff7f0e' stroke-width='3' />")
    lines.append(f"<text x='{lx+52}' y='{ly+42}' font-family='sans-serif' font-size='12' fill='#333'>RealTime only misses=0</text>")

    lines.append("</svg>")

    out_path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    p = argparse.ArgumentParser(
        description="Generate SVG graph: max safe RealTime WCET vs ISR tax",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    p.add_argument("--sim-ms", type=int, default=5000)
    p.add_argument("--quantum-us", type=int, default=100)

    p.add_argument("--rt-period-ms", type=int, default=20)
    p.add_argument("--rt-deadline-ms", type=int, default=20)

    p.add_argument("--sensor-period-ms", type=int, default=100)
    p.add_argument("--sensor-deadline-ms", type=int, default=100)
    p.add_argument("--sensor-wcet-us", type=int, default=8000)

    p.add_argument("--system-period-ms", type=int, default=1000)
    p.add_argument("--system-deadline-ms", type=int, default=1000)
    p.add_argument("--system-wcet-us", type=int, default=6000)

    p.add_argument("--sweep-start-us", type=int, default=8000)
    p.add_argument("--sweep-end-us", type=int, default=20000)
    p.add_argument("--sweep-step-us", type=int, default=1000)

    p.add_argument("--isr-tax-period-ms", type=int, default=1)
    p.add_argument(
        "--isr-tax-us-list",
        type=str,
        default="0,50,100,150,200",
        help="Comma-separated ISR tax cost in us per isr-tax-period-ms (e.g. 100us every 1ms = 10% CPU)",
    )

    p.add_argument("--out-dir", type=str, default=str(TOOLS_DIR / "out"))

    args = p.parse_args()

    if args.quantum_us <= 0 or args.quantum_us > 1000 or (1000 % args.quantum_us) != 0:
        print("ERROR: --quantum-us must be a divisor of 1000 in range 1..1000")
        return 2

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    try:
        tax_us_list = parse_int_list(args.isr_tax_us_list)
    except Exception as e:
        print(f"ERROR: invalid --isr-tax-us-list: {e}")
        return 2

    points: list[SweepPoint] = []

    for tax_us in tax_us_list:
        # percent CPU ~= tax_us / (period_ms * 1000us)
        isr_percent = (tax_us / (args.isr_tax_period_ms * 1000.0)) * 100.0 if args.isr_tax_period_ms else 0.0

        max_rt_only, max_all = compute_thresholds(
            sim_ms=args.sim_ms,
            quantum_us=args.quantum_us,
            rt_period_ms=args.rt_period_ms,
            rt_deadline_ms=args.rt_deadline_ms,
            sensor_period_ms=args.sensor_period_ms,
            sensor_deadline_ms=args.sensor_deadline_ms,
            sensor_wcet_us=args.sensor_wcet_us,
            system_period_ms=args.system_period_ms,
            system_deadline_ms=args.system_deadline_ms,
            system_wcet_us=args.system_wcet_us,
            sweep_start_us=args.sweep_start_us,
            sweep_end_us=args.sweep_end_us,
            sweep_step_us=args.sweep_step_us,
            isr_tax_period_ms=args.isr_tax_period_ms,
            isr_tax_us=tax_us,
        )

        points.append(
            SweepPoint(
                isr_tax_us=tax_us,
                isr_percent=isr_percent,
                max_rt_only_ok_us=max_rt_only,
                max_all_ok_us=max_all,
            )
        )

    points.sort(key=lambda p: p.isr_percent)

    csv_path = out_dir / "rtos_wcet_vs_isr.csv"
    with csv_path.open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["isr_tax_us", "isr_percent", "max_rt_only_ok_us", "max_all_ok_us"])
        for pnt in points:
            w.writerow([
                pnt.isr_tax_us,
                f"{pnt.isr_percent:.3f}",
                "" if pnt.max_rt_only_ok_us is None else pnt.max_rt_only_ok_us,
                "" if pnt.max_all_ok_us is None else pnt.max_all_ok_us,
            ])

    svg_path = out_dir / "rtos_wcet_vs_isr.svg"
    title = (
        f"Max safe RealTime WCET vs ISR tax (sim={args.sim_ms}ms, quantum={args.quantum_us}us, "
        f"Sensor={args.sensor_wcet_us/1000:.1f}ms@{args.sensor_period_ms}ms, System={args.system_wcet_us/1000:.1f}ms@{args.system_period_ms}ms)"
    )
    write_svg(out_path=svg_path, points=points, title=title, y_max_us=args.sweep_end_us)

    print("Wrote:")
    print(f"- {csv_path}")
    print(f"- {svg_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
