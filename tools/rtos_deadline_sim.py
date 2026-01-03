#!/usr/bin/env python3
"""RTOS deadline simulation (host-side, no hardware required).

Goal
- Provide a *useful approximation* of deadline miss risk for fixed-priority periodic tasks.
- This is not cycle-accurate. It answers: "Given assumed WCETs, will 20ms/50ms deadlines be missed?"

Model
- Fixed-priority preemptive scheduler (FreeRTOS-like).
- Periodic tasks release jobs at fixed periods.
- CPU time is consumed in quanta (default 100us).

What it does NOT model
- Precise STM32 peripheral timing, DMA, flash wait-states, cache effects
- ISR storms or priority inversion beyond what you encode in WCETs

Usage
- python tools/rtos_deadline_sim.py
- Adjust the constants below to reflect conservative WCET assumptions.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from typing import Any


# Periodic tasks
# Priorities: higher number = higher priority
@dataclass
class TaskCfg:
    name: str
    period_ms: int
    deadline_ms: int
    wcet_us: int
    priority: int

def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Fixed-priority RTOS deadline simulator (host-side approximation)",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    p.add_argument("--sim-ms", type=int, default=2000, help="Simulation time in ms")
    p.add_argument("--quantum-us", type=int, default=100, help="CPU time quantum in us")

    p.add_argument("--rt-period-ms", type=int, default=20, help="RealTime task period")
    p.add_argument("--rt-deadline-ms", type=int, default=20, help="RealTime task deadline")
    p.add_argument("--rt-wcet-us", type=int, default=4000, help="RealTime assumed WCET in us")

    p.add_argument("--sensor-period-ms", type=int, default=100, help="Sensor task period")
    p.add_argument("--sensor-deadline-ms", type=int, default=100, help="Sensor task deadline")
    p.add_argument("--sensor-wcet-us", type=int, default=8000, help="Sensor assumed WCET in us")

    p.add_argument("--system-period-ms", type=int, default=1000, help="System task period")
    p.add_argument("--system-deadline-ms", type=int, default=1000, help="System task deadline")
    p.add_argument("--system-wcet-us", type=int, default=6000, help="System assumed WCET in us")

    p.add_argument("--isr-tax-period-ms", type=int, default=0, help="Optional periodic ISR tax period in ms")
    p.add_argument("--isr-tax-us", type=int, default=0, help="Optional periodic ISR tax cost in us")

    p.add_argument(
        "--sweep-rt-wcet",
        action="store_true",
        help="Sweep RealTime WCET over a range and report max safe threshold(s)",
    )
    p.add_argument("--sweep-start-us", type=int, default=8000, help="Sweep start WCET for RealTime (us)")
    p.add_argument("--sweep-end-us", type=int, default=20000, help="Sweep end WCET for RealTime (us)")
    p.add_argument("--sweep-step-us", type=int, default=1000, help="Sweep step WCET for RealTime (us)")
    p.add_argument(
        "--sweep-criteria",
        choices=["rt", "all"],
        default="all",
        help="Primary pass/fail criteria for sweep output (still prints both thresholds)",
    )

    return p.parse_args()


@dataclass
class Job:
    task: TaskCfg
    release_ms: int
    deadline_ms: int
    remaining_us: int
    started_ms: int | None = None
    finished_ms: int | None = None
    missed: bool = False


@dataclass
class SimResult:
    misses: dict[str, int]
    worst_lateness_ms: dict[str, int]
    max_response_ms: dict[str, int]


def simulate(
    *,
    sim_ms: int,
    quantum_us: int,
    tasks: list[TaskCfg],
    isr_tax_period_ms: int,
    isr_tax_us: int,
) -> SimResult:
    steps_per_ms = 1000 // quantum_us
    total_steps = sim_ms * steps_per_ms

    ready: list[Job] = []

    misses = {t.name: 0 for t in tasks}
    worst_lateness_ms = {t.name: 0 for t in tasks}
    max_response_ms = {t.name: 0 for t in tasks}

    def release_jobs(now_ms: int) -> None:
        for t in tasks:
            if now_ms % t.period_ms == 0:
                ready.append(Job(task=t, release_ms=now_ms, deadline_ms=now_ms + t.deadline_ms, remaining_us=t.wcet_us))
        if isr_tax_period_ms and (now_ms % isr_tax_period_ms == 0):
            # Represent ISR tax as a highest-priority pseudo-job.
            pseudo = TaskCfg(name="ISR_TAX", period_ms=isr_tax_period_ms, deadline_ms=isr_tax_period_ms, wcet_us=isr_tax_us, priority=999)
            ready.append(Job(task=pseudo, release_ms=now_ms, deadline_ms=now_ms + isr_tax_period_ms, remaining_us=isr_tax_us))

    # Sim loop
    for step in range(total_steps):
        now_us = step * quantum_us
        now_ms = now_us // 1000

        # release at the start of each ms boundary
        if (now_us % 1000) == 0:
            release_jobs(int(now_ms))

        # Count deadline misses for any job that has exceeded its deadline, even if it never finishes.
        for j in ready:
            if j.remaining_us <= 0 or j.task.name == "ISR_TAX" or j.missed:
                continue
            if int(now_ms) > j.deadline_ms:
                j.missed = True
                misses[j.task.name] += 1
                lateness = int(now_ms) - j.deadline_ms
                if lateness > worst_lateness_ms[j.task.name]:
                    worst_lateness_ms[j.task.name] = lateness

        # pick highest-priority ready job with remaining time
        run_queue = [j for j in ready if j.remaining_us > 0]
        if run_queue:
            run_queue.sort(key=lambda j: (j.task.priority, -j.release_ms), reverse=True)
            j = run_queue[0]
            if j.started_ms is None:
                j.started_ms = int(now_ms)
            j.remaining_us -= quantum_us
            if j.remaining_us <= 0:
                j.finished_ms = int((now_us + quantum_us + 999) // 1000)  # ceil to ms
                # stats (ignore ISR_TAX)
                if j.task.name != "ISR_TAX":
                    response = j.finished_ms - j.release_ms
                    if response > max_response_ms[j.task.name]:
                        max_response_ms[j.task.name] = response
                    lateness = j.finished_ms - j.deadline_ms
                    if lateness > 0:
                        if not j.missed:
                            misses[j.task.name] += 1
                            j.missed = True
                        if lateness > worst_lateness_ms[j.task.name]:
                            worst_lateness_ms[j.task.name] = lateness

        # drop completed jobs from ready list occasionally
        if (step % 100) == 0:
            ready = [j for j in ready if j.remaining_us > 0]

    # End-of-simulation: account for any unfinished jobs that have already missed their deadlines.
    end_ms = int(sim_ms)
    for j in ready:
        if j.task.name == "ISR_TAX" or j.remaining_us <= 0:
            continue
        # Track response lower-bound as how long it's been pending so far.
        pending = end_ms - j.release_ms
        if pending > max_response_ms.get(j.task.name, 0):
            max_response_ms[j.task.name] = pending

        lateness = end_ms - j.deadline_ms
        if lateness > 0 and not j.missed:
            j.missed = True
            misses[j.task.name] += 1
            if lateness > worst_lateness_ms[j.task.name]:
                worst_lateness_ms[j.task.name] = lateness

    return SimResult(misses=misses, worst_lateness_ms=worst_lateness_ms, max_response_ms=max_response_ms)


def main() -> int:
    args = parse_args()

    quantum_us = args.quantum_us
    if quantum_us <= 0 or quantum_us > 1000 or (1000 % quantum_us) != 0:
        print("ERROR: --quantum-us must be a divisor of 1000 in range 1..1000")
        return 2

    base_tasks = [
        TaskCfg(name="RealTime", period_ms=args.rt_period_ms, deadline_ms=args.rt_deadline_ms, wcet_us=args.rt_wcet_us, priority=5),
        TaskCfg(name="Sensor", period_ms=args.sensor_period_ms, deadline_ms=args.sensor_deadline_ms, wcet_us=args.sensor_wcet_us, priority=3),
        TaskCfg(name="System", period_ms=args.system_period_ms, deadline_ms=args.system_deadline_ms, wcet_us=args.system_wcet_us, priority=1),
    ]

    def print_single_run(tasks: list[TaskCfg], result: SimResult) -> None:
        print("RTOS deadline simulation (approx)")
        print(f"- Sim time: {args.sim_ms} ms")
        print(f"- Quantum: {args.quantum_us} us")
        print()
        print("Task assumptions")
        for t in tasks:
            print(f"- {t.name}: period={t.period_ms}ms deadline={t.deadline_ms}ms wcet={t.wcet_us}us prio={t.priority}")
        if args.isr_tax_period_ms:
            print(f"- ISR_TAX: every {args.isr_tax_period_ms}ms cost={args.isr_tax_us}us")
        print()
        print("Results")
        for t in tasks:
            print(
                f"- {t.name}: misses={result.misses[t.name]} "
                f"worst_lateness={result.worst_lateness_ms[t.name]}ms "
                f"max_response={result.max_response_ms[t.name]}ms"
            )
        print()
        print("How to use")
        print("- If RealTime misses > 0: reduce its WCET or reduce interference (lower other WCETs/ISR tax).")
        print("- If Sensor/System miss: OK unless they block shared resources needed by RealTime.")

    def tasks_with_rt_wcet(rt_wcet_us: int) -> list[TaskCfg]:
        out: list[TaskCfg] = []
        for t in base_tasks:
            if t.name == "RealTime":
                out.append(TaskCfg(name=t.name, period_ms=t.period_ms, deadline_ms=t.deadline_ms, wcet_us=rt_wcet_us, priority=t.priority))
            else:
                out.append(t)
        return out

    if args.sweep_rt_wcet:
        start = args.sweep_start_us
        end = args.sweep_end_us
        step = args.sweep_step_us
        if step <= 0:
            print("ERROR: --sweep-step-us must be > 0")
            return 2
        if end < start:
            print("ERROR: --sweep-end-us must be >= --sweep-start-us")
            return 2

        # Track thresholds.
        max_rt_only_ok: int | None = None
        max_all_ok: int | None = None

        print("RTOS RealTime WCET sweep")
        print(f"- Range: {start}..{end} us step {step} us")
        print(f"- Sim time: {args.sim_ms} ms, quantum: {args.quantum_us} us")
        if args.isr_tax_period_ms:
            print(f"- ISR tax: every {args.isr_tax_period_ms}ms cost={args.isr_tax_us}us")
        print()
        print("wcet_us  rt_miss  sens_miss  sys_miss  rt_max_resp_ms")

        wcet = start
        while wcet <= end:
            tcfg = tasks_with_rt_wcet(wcet)
            res = simulate(
                sim_ms=args.sim_ms,
                quantum_us=quantum_us,
                tasks=tcfg,
                isr_tax_period_ms=args.isr_tax_period_ms,
                isr_tax_us=args.isr_tax_us,
            )

            rt_miss = res.misses.get("RealTime", 0)
            sens_miss = res.misses.get("Sensor", 0)
            sys_miss = res.misses.get("System", 0)
            rt_resp = res.max_response_ms.get("RealTime", 0)
            print(f"{wcet:6d}  {rt_miss:7d}  {sens_miss:9d}  {sys_miss:8d}  {rt_resp:13d}")

            if rt_miss == 0:
                max_rt_only_ok = wcet
                if sens_miss == 0 and sys_miss == 0:
                    max_all_ok = wcet

            wcet += step

        print()
        print("Thresholds")
        if max_rt_only_ok is None:
            print("- Max WCET with RealTime misses=0: none in range")
        else:
            print(f"- Max WCET with RealTime misses=0: {max_rt_only_ok} us")
        if max_all_ok is None:
            print("- Max WCET with RealTime+Sensor+System misses=0: none in range")
        else:
            print(f"- Max WCET with RealTime+Sensor+System misses=0: {max_all_ok} us")

        if args.sweep_criteria == "rt" and max_rt_only_ok is None:
            return 1
        if args.sweep_criteria == "all" and max_all_ok is None:
            return 1
        return 0

    # Single-run mode
    result = simulate(
        sim_ms=args.sim_ms,
        quantum_us=quantum_us,
        tasks=base_tasks,
        isr_tax_period_ms=args.isr_tax_period_ms,
        isr_tax_us=args.isr_tax_us,
    )
    print_single_run(base_tasks, result)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
