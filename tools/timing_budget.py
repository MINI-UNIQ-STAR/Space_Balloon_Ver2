#!/usr/bin/env python3
"""Static worst-case timing budget estimator (no hardware needed).

This script scans the workspace for:
- UART baud rates (Core/Src/usart.c)
- Telemetry frame max size (Core/Src/services/telemetry_service.c)
- Driver timeout constants (Core/Src/drivers/*.c)

It then prints conservative upper bounds for blocking time per 20ms slot.

Notes:
- This is NOT cycle-accurate simulation.
- It is intended to catch obvious deadline risks (e.g., blocking UART TX).
"""

from __future__ import annotations

import pathlib
import re
import sys
from dataclasses import dataclass
from typing import Iterable


ROOT = pathlib.Path(__file__).resolve().parents[1]


@dataclass(frozen=True)
class UartConfig:
    name: str
    baud: int


def read_text(path: pathlib.Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def find_uart_baud_rates(usart_c: pathlib.Path) -> list[UartConfig]:
    txt = read_text(usart_c)

    # Pattern: huart3.Instance = USART3; ... huart3.Init.BaudRate = 115200;
    results: list[UartConfig] = []
    for uart in ("huart1", "huart2", "huart3", "huart4", "huart5"):
        m_inst = re.search(rf"\b{re.escape(uart)}\.Instance\s*=\s*(USART\d+)\s*;", txt)
        m_baud = re.search(rf"\b{re.escape(uart)}\.Init\.BaudRate\s*=\s*(\d+)\s*;", txt)
        if m_inst and m_baud:
            results.append(UartConfig(name=m_inst.group(1), baud=int(m_baud.group(1))))
    return results


def find_telemetry_frame_cap(telemetry_service_c: pathlib.Path) -> int | None:
    txt = read_text(telemetry_service_c)

    # Pattern: uint8_t frame[128];
    m = re.search(r"\buint8_t\s+frame\s*\[(\d+)\]\s*;", txt)
    if not m:
        return None
    return int(m.group(1))


def iter_driver_sources() -> Iterable[pathlib.Path]:
    d = ROOT / "Core" / "Src" / "drivers"
    if not d.exists():
        return []
    return sorted(d.glob("*.c"))


def scan_timeouts_in_file(path: pathlib.Path) -> dict[str, int]:
    """Return {symbol_name: value_ms} for *_TIMEOUT_MS enum constants and raw HAL timeouts."""
    txt = read_text(path)

    out: dict[str, int] = {}

    # enum { FOO_TIMEOUT_MS = 10u, };
    for m in re.finditer(r"\b([A-Z0-9_]+TIMEOUT_MS)\s*=\s*(\d+)\s*[uU]?\b", txt):
        out[m.group(1)] = int(m.group(2))

    # Also catch direct literals in HAL calls: ..., timeout)
    # Very rough: HAL_I2C_* (..., <number> )
    for m in re.finditer(r"HAL_I2C_[A-Za-z0-9_]+\([^;]*?,\s*(\d+)\s*\)", txt):
        # Don’t name these; just track as literals
        out[f"{path.name}:HAL_I2C_timeout_literal_ms@{m.start()}"] = int(m.group(1))

    # UART Transmit blocking literal: HAL_UART_Transmit(..., timeout_ms)
    for m in re.finditer(r"HAL_UART_Transmit\([^;]*?,\s*(\d+)\s*\)", txt):
        out[f"{path.name}:HAL_UART_timeout_literal_ms@{m.start()}"] = int(m.group(1))

    return out


def uart_tx_time_ms(baud: int, n_bytes: int) -> float:
    # 8N1 => 10 bits per byte
    bits = n_bytes * 10
    return (bits / baud) * 1000.0


def main() -> int:
    usart_c = ROOT / "Core" / "Src" / "usart.c"
    telemetry_c = ROOT / "Core" / "Src" / "services" / "telemetry_service.c"

    if not usart_c.exists():
        print(f"ERROR: not found: {usart_c}")
        return 2
    if not telemetry_c.exists():
        print(f"ERROR: not found: {telemetry_c}")
        return 2

    uarts = find_uart_baud_rates(usart_c)
    frame_cap = find_telemetry_frame_cap(telemetry_c)

    print("Worst-case timing budget (static analysis)")
    print(f"- Workspace: {ROOT}")
    print()

    print("UART configs (from Core/Src/usart.c)")
    for u in uarts:
        print(f"- {u.name}: {u.baud} bps")
    print()

    if frame_cap is not None:
        print("Telemetry frame capacity (from telemetry_service.c)")
        print(f"- frame buffer cap: {frame_cap} bytes")
        print()

        uart3 = next((u for u in uarts if u.name == "USART3"), None)
        if uart3:
            t = uart_tx_time_ms(uart3.baud, frame_cap)
            print("If telemetry were BLOCKING UART TX (8N1), worst-case TX time")
            print(f"- USART3 {uart3.baud} bps, {frame_cap} bytes => ~{t:.2f} ms")
            print("  (This was a major 20ms deadline risk; now mitigated if TX is non-blocking.)")
            print()

    # Driver timeouts summary
    timeout_values: list[tuple[str, int]] = []
    for p in iter_driver_sources():
        for k, v in scan_timeouts_in_file(p).items():
            # Filter out huge spam of position-tagged keys unless value seems relevant
            if k.endswith("TIMEOUT_MS"):
                timeout_values.append((k, v))

    timeout_values.sort(key=lambda x: x[1], reverse=True)

    print("Driver timeout constants (descending)")
    if not timeout_values:
        print("- (none found)")
    else:
        for name, ms in timeout_values[:20]:
            print(f"- {name}: {ms} ms")
        if len(timeout_values) > 20:
            print(f"- ... ({len(timeout_values) - 20} more)")
    print()

    print("Rule of thumb for 50Hz RealTime task")
    print("- Keep any single blocking call << 20ms, ideally < 2ms")
    print("- Prefer try-lock (0ms) on shared resources")
    print("- Any remaining >5ms timeouts should live only in low/medium priority tasks")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
