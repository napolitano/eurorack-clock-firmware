#!/usr/bin/env python3
"""Parse GNU size output and enforce firmware Flash/RAM budgets.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""

from __future__ import annotations

import argparse
import math
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class MemoryUsage:
    """Program memory usage derived from GNU size's Berkeley-format columns."""

    text_bytes: int
    data_bytes: int
    bss_bytes: int

    @property
    def flash_bytes(self) -> int:
        """Return bytes stored in non-volatile program Flash."""
        return self.text_bytes + self.data_bytes

    @property
    def ram_bytes(self) -> int:
        """Return statically allocated RAM bytes."""
        return self.data_bytes + self.bss_bytes


def parse_size_output(output: str) -> MemoryUsage:
    """Parse one GNU ``size`` Berkeley-format report.

    Expected data line format is ``text data bss dec hex filename``. Whitespace is
    deliberately flexible so the parser works across Windows, macOS, and Linux hosts.
    """
    for line in output.splitlines():
        match = re.match(r"^\s*(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+[0-9A-Fa-f]+\s+.+$", line)
        if match:
            return MemoryUsage(*(int(value) for value in match.groups()))
    raise ValueError("Could not find a GNU size data row in tool output")


def budget_bytes(capacity_bytes: int, maximum_percent: float) -> int:
    """Return the inclusive byte budget for a percentage of a capacity."""
    if capacity_bytes <= 0:
        raise ValueError("Memory capacity must be greater than zero")
    if not 0.0 < maximum_percent <= 100.0:
        raise ValueError("Memory budget percentage must be in the range (0, 100]")
    return math.floor(capacity_bytes * maximum_percent / 100.0)


def enforce_memory_budget(
    usage: MemoryUsage,
    flash_capacity_bytes: int,
    ram_capacity_bytes: int,
    maximum_percent: float,
) -> None:
    """Raise RuntimeError when Flash or static RAM exceeds the configured budget."""
    flash_budget = budget_bytes(flash_capacity_bytes, maximum_percent)
    ram_budget = budget_bytes(ram_capacity_bytes, maximum_percent)
    failures: list[str] = []

    if usage.flash_bytes > flash_budget:
        failures.append(
            f"Flash {usage.flash_bytes} B exceeds {flash_budget} B "
            f"({maximum_percent:.1f}% of {flash_capacity_bytes} B)"
        )
    if usage.ram_bytes > ram_budget:
        failures.append(
            f"RAM {usage.ram_bytes} B exceeds {ram_budget} B "
            f"({maximum_percent:.1f}% of {ram_capacity_bytes} B)"
        )
    if failures:
        raise RuntimeError("; ".join(failures))


def format_report(
    usage: MemoryUsage,
    flash_capacity_bytes: int,
    ram_capacity_bytes: int,
    maximum_percent: float,
) -> str:
    """Return a concise human-readable memory report for CI and local builds."""
    flash_budget = budget_bytes(flash_capacity_bytes, maximum_percent)
    ram_budget = budget_bytes(ram_capacity_bytes, maximum_percent)
    return "\n".join(
        [
            "Firmware memory budget",
            f"  Flash: {usage.flash_bytes:6d} / {flash_capacity_bytes:6d} B "
            f"({usage.flash_bytes * 100.0 / flash_capacity_bytes:5.1f}%, limit {flash_budget} B)",
            f"  RAM:   {usage.ram_bytes:6d} / {ram_capacity_bytes:6d} B "
            f"({usage.ram_bytes * 100.0 / ram_capacity_bytes:5.1f}%, limit {ram_budget} B)",
        ]
    )


def inspect_elf(
    size_tool: str,
    elf_path: Path,
    flash_capacity_bytes: int,
    ram_capacity_bytes: int,
    maximum_percent: float,
) -> MemoryUsage:
    """Run GNU size on an ELF, print usage, and enforce the configured limits."""
    completed = subprocess.run(
        [size_tool, str(elf_path)],
        check=True,
        capture_output=True,
        text=True,
    )
    usage = parse_size_output(completed.stdout)
    print(format_report(usage, flash_capacity_bytes, ram_capacity_bytes, maximum_percent))
    enforce_memory_budget(usage, flash_capacity_bytes, ram_capacity_bytes, maximum_percent)
    return usage


def main() -> int:
    """CLI entry point used for manual ELF inspection outside PlatformIO."""
    parser = argparse.ArgumentParser()
    parser.add_argument("elf", type=Path)
    parser.add_argument("--size-tool", default="arm-none-eabi-size")
    parser.add_argument("--flash", type=int, required=True)
    parser.add_argument("--ram", type=int, required=True)
    parser.add_argument("--percent", type=float, default=90.0)
    args = parser.parse_args()
    inspect_elf(args.size_tool, args.elf, args.flash, args.ram, args.percent)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
