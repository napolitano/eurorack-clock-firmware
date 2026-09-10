#!/usr/bin/env python3
"""Analyze CLOCK HIL edge captures and apply objective timing checks.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import statistics
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


@dataclass(frozen=True)
class Edge:
    """One timestamped digital transition from a HIL capture."""

    time_us: float
    signal: str
    level: int


def parse_capture(path: Path) -> list[Edge]:
    """Read the canonical long-form HIL CSV and validate ordering and levels."""
    edges: list[Edge] = []
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        required = {"time_us", "signal", "level"}
        if reader.fieldnames is None or set(reader.fieldnames) != required:
            raise ValueError(
                f"{path}: expected CSV columns time_us,signal,level exactly; got {reader.fieldnames!r}"
            )
        previous_time = -math.inf
        for line_number, row in enumerate(reader, start=2):
            try:
                time_us = float(row["time_us"])
                level = int(row["level"])
            except (TypeError, ValueError) as exc:
                raise ValueError(f"{path}:{line_number}: invalid numeric field") from exc
            signal = row["signal"].strip()
            if not signal:
                raise ValueError(f"{path}:{line_number}: signal must not be empty")
            if level not in (0, 1):
                raise ValueError(f"{path}:{line_number}: level must be 0 or 1")
            if not math.isfinite(time_us) or time_us < 0.0:
                raise ValueError(f"{path}:{line_number}: time_us must be finite and >= 0")
            if time_us < previous_time:
                raise ValueError(f"{path}:{line_number}: timestamps must be globally monotonic")
            previous_time = time_us
            edges.append(Edge(time_us=time_us, signal=signal, level=level))
    if not edges:
        raise ValueError(f"{path}: capture contains no edges")
    return edges


def signal_edges(edges: Iterable[Edge], signal: str) -> list[Edge]:
    """Return all transitions for one signal."""
    return [edge for edge in edges if edge.signal == signal]


def rising_times(edges: Iterable[Edge], signal: str) -> list[float]:
    """Return rising-edge timestamps for one signal."""
    return [edge.time_us for edge in edges if edge.signal == signal and edge.level == 1]


def pulse_widths(edges: Iterable[Edge], signal: str) -> list[float]:
    """Return complete HIGH pulse widths and reject malformed transition sequences."""
    transitions = signal_edges(edges, signal)
    widths: list[float] = []
    rise: float | None = None
    last_level: int | None = None
    for edge in transitions:
        if last_level == edge.level:
            raise ValueError(f"{signal}: duplicate level {edge.level} at {edge.time_us:g} us")
        last_level = edge.level
        if edge.level == 1:
            if rise is not None:
                raise ValueError(f"{signal}: rising edge while already HIGH at {edge.time_us:g} us")
            rise = edge.time_us
        elif rise is not None:
            widths.append(edge.time_us - rise)
            rise = None
    if rise is not None:
        raise ValueError(f"{signal}: capture ends while HIGH")
    return widths


def periods(edges: Iterable[Edge], signal: str) -> list[float]:
    """Return rising-edge-to-rising-edge periods for one signal."""
    rises = rising_times(edges, signal)
    return [later - earlier for earlier, later in zip(rises, rises[1:])]


def stats(values: list[float]) -> dict[str, float | int]:
    """Return deterministic descriptive statistics for a non-empty sample."""
    if not values:
        raise ValueError("cannot calculate statistics for an empty sample")
    ordered = sorted(values)
    return {
        "count": len(values),
        "min": ordered[0],
        "max": ordered[-1],
        "mean": statistics.fmean(values),
        "median": statistics.median(values),
        "stdev": statistics.pstdev(values),
        "peak_to_peak": ordered[-1] - ordered[0],
    }


def maximum_absolute_error(values: list[float], expected: float) -> float:
    """Return the largest absolute deviation from an expected value."""
    if not values:
        raise ValueError("cannot calculate error for an empty sample")
    return max(abs(value - expected) for value in values)


def maximum_median_deviation(values: list[float]) -> float:
    """Return the largest absolute deviation from the sample median."""
    if not values:
        raise ValueError("cannot calculate deviation for an empty sample")
    median = statistics.median(values)
    return max(abs(value - median) for value in values)


def interchannel_skews(
    edges: Iterable[Edge],
    signals: list[str],
    match_window_us: float,
) -> list[float]:
    """Match simultaneous rising edges to the first signal and return group skews."""
    if len(signals) < 2:
        raise ValueError("skew analysis requires at least two signals")
    reference = rising_times(edges, signals[0])
    if not reference:
        raise ValueError(f"{signals[0]}: no rising edges")
    others = [rising_times(edges, signal) for signal in signals[1:]]
    if any(not values for values in others):
        missing = signals[1:][next(index for index, values in enumerate(others) if not values)]
        raise ValueError(f"{missing}: no rising edges")

    cursors = [0 for _ in others]
    result: list[float] = []
    for ref_time in reference:
        group = [ref_time]
        matched = True
        for channel_index, times in enumerate(others):
            cursor = cursors[channel_index]
            while cursor < len(times) and times[cursor] < ref_time - match_window_us:
                cursor += 1
            candidates: list[tuple[float, int]] = []
            for candidate_index in (cursor, cursor + 1):
                if candidate_index < len(times):
                    delta = abs(times[candidate_index] - ref_time)
                    if delta <= match_window_us:
                        candidates.append((delta, candidate_index))
            if not candidates:
                matched = False
                cursors[channel_index] = cursor
                break
            _, selected_index = min(candidates)
            group.append(times[selected_index])
            cursors[channel_index] = selected_index + 1
        if matched:
            result.append(max(group) - min(group))
    if not result:
        raise ValueError("no complete simultaneous edge groups found inside match window")
    return result


def emit(result: dict[str, object], passed: bool | None = None) -> int:
    """Print one machine-readable result and map pass/fail to the process status."""
    if passed is not None:
        result["status"] = "PASS" if passed else "FAIL"
    print(json.dumps(result, indent=2, sort_keys=True))
    if passed is None or passed:
        return 0
    return 2


def require_minimum(count: int, minimum: int, what: str) -> None:
    """Reject captures that contain too little evidence for the requested check."""
    if count < minimum:
        raise ValueError(f"need at least {minimum} {what}; capture contains {count}")


def command_summary(args: argparse.Namespace) -> int:
    """Summarize all signals without applying a release threshold."""
    edges = parse_capture(args.capture)
    signals = sorted({edge.signal for edge in edges})
    result: dict[str, object] = {"capture": str(args.capture), "signals": {}}
    signal_result: dict[str, object] = {}
    for signal in signals:
        rises = rising_times(edges, signal)
        signal_periods = periods(edges, signal)
        entry: dict[str, object] = {
            "edge_count": len(signal_edges(edges, signal)),
            "rising_edge_count": len(rises),
        }
        if signal_periods:
            entry["period_us"] = stats(signal_periods)
        try:
            widths = pulse_widths(edges, signal)
        except ValueError as exc:
            entry["pulse_error"] = str(exc)
        else:
            if widths:
                entry["high_width_us"] = stats(widths)
        signal_result[signal] = entry
    result["signals"] = signal_result
    return emit(result)


def command_pulse_width(args: argparse.Namespace) -> int:
    """Check physical HIGH pulse width against an expected value and tolerance."""
    edges = parse_capture(args.capture)
    values = pulse_widths(edges, args.signal)
    require_minimum(len(values), args.min_pulses, "complete pulses")
    error = maximum_absolute_error(values, args.expected_us)
    passed = error <= args.tolerance_us
    return emit(
        {
            "check": "pulse-width",
            "capture": str(args.capture),
            "signal": args.signal,
            "expected_us": args.expected_us,
            "tolerance_us": args.tolerance_us,
            "maximum_absolute_error_us": error,
            "observed_us": stats(values),
        },
        passed,
    )


def command_period(args: argparse.Namespace) -> int:
    """Check rising-edge period against an expected value and tolerance."""
    edges = parse_capture(args.capture)
    values = periods(edges, args.signal)
    require_minimum(len(values), args.min_periods, "periods")
    error = maximum_absolute_error(values, args.expected_us)
    passed = error <= args.tolerance_us
    return emit(
        {
            "check": "period",
            "capture": str(args.capture),
            "signal": args.signal,
            "expected_us": args.expected_us,
            "tolerance_us": args.tolerance_us,
            "maximum_absolute_error_us": error,
            "observed_us": stats(values),
        },
        passed,
    )


def command_skew(args: argparse.Namespace) -> int:
    """Check simultaneous output edge skew across two or more channels."""
    edges = parse_capture(args.capture)
    signals = [part.strip() for part in args.signals.split(",") if part.strip()]
    values = interchannel_skews(edges, signals, args.match_window_us)
    require_minimum(len(values), args.min_groups, "complete edge groups")
    maximum = max(values)
    return emit(
        {
            "check": "interchannel-skew",
            "capture": str(args.capture),
            "signals": signals,
            "match_window_us": args.match_window_us,
            "maximum_allowed_skew_us": args.max_skew_us,
            "observed_skew_us": stats(values),
        },
        maximum <= args.max_skew_us,
    )


def command_compare(args: argparse.Namespace) -> int:
    """Compare idle and stress captures for event loss and added period instability."""
    baseline_edges = parse_capture(args.baseline)
    stress_edges = parse_capture(args.stress)
    baseline_rises = rising_times(baseline_edges, args.signal)
    stress_rises = rising_times(stress_edges, args.signal)
    baseline_periods = periods(baseline_edges, args.signal)
    stress_periods = periods(stress_edges, args.signal)
    require_minimum(len(baseline_periods), args.min_periods, "baseline periods")
    require_minimum(len(stress_periods), args.min_periods, "stress periods")

    baseline_deviation = maximum_median_deviation(baseline_periods)
    stress_deviation = maximum_median_deviation(stress_periods)
    added_deviation = max(0.0, stress_deviation - baseline_deviation)
    count_delta = len(stress_rises) - len(baseline_rises)
    count_ok = count_delta == 0 if args.require_same_event_count else True
    timing_ok = added_deviation <= args.max_added_deviation_us
    return emit(
        {
            "check": "display-stress-comparison",
            "baseline": str(args.baseline),
            "stress": str(args.stress),
            "signal": args.signal,
            "baseline_rising_edges": len(baseline_rises),
            "stress_rising_edges": len(stress_rises),
            "event_count_delta": count_delta,
            "require_same_event_count": args.require_same_event_count,
            "baseline_max_median_deviation_us": baseline_deviation,
            "stress_max_median_deviation_us": stress_deviation,
            "added_max_median_deviation_us": added_deviation,
            "maximum_allowed_added_deviation_us": args.max_added_deviation_us,
            "baseline_period_us": stats(baseline_periods),
            "stress_period_us": stats(stress_periods),
        },
        count_ok and timing_ok,
    )


def build_parser() -> argparse.ArgumentParser:
    """Construct the command-line interface."""
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    summary = subparsers.add_parser("summary", help="summarize all captured signals")
    summary.add_argument("capture", type=Path)
    summary.set_defaults(handler=command_summary)

    width = subparsers.add_parser("pulse-width", help="validate HIGH pulse width")
    width.add_argument("capture", type=Path)
    width.add_argument("--signal", required=True)
    width.add_argument("--expected-us", type=float, required=True)
    width.add_argument("--tolerance-us", type=float, required=True)
    width.add_argument("--min-pulses", type=int, default=10)
    width.set_defaults(handler=command_pulse_width)

    period_parser = subparsers.add_parser("period", help="validate rising-edge period")
    period_parser.add_argument("capture", type=Path)
    period_parser.add_argument("--signal", required=True)
    period_parser.add_argument("--expected-us", type=float, required=True)
    period_parser.add_argument("--tolerance-us", type=float, required=True)
    period_parser.add_argument("--min-periods", type=int, default=10)
    period_parser.set_defaults(handler=command_period)

    skew = subparsers.add_parser("skew", help="validate simultaneous output skew")
    skew.add_argument("capture", type=Path)
    skew.add_argument("--signals", required=True, help="comma-separated signal names")
    skew.add_argument("--match-window-us", type=float, default=100.0)
    skew.add_argument("--max-skew-us", type=float, required=True)
    skew.add_argument("--min-groups", type=int, default=10)
    skew.set_defaults(handler=command_skew)

    compare = subparsers.add_parser("compare", help="compare idle and display-stress timing")
    compare.add_argument("baseline", type=Path)
    compare.add_argument("stress", type=Path)
    compare.add_argument("--signal", required=True)
    compare.add_argument("--max-added-deviation-us", type=float, required=True)
    compare.add_argument("--min-periods", type=int, default=10)
    compare.add_argument("--require-same-event-count", action="store_true")
    compare.set_defaults(handler=command_compare)
    return parser


def main() -> int:
    """Run the selected analysis command with stable diagnostic exit codes."""
    parser = build_parser()
    args = parser.parse_args()
    try:
        return int(args.handler(args))
    except (OSError, ValueError) as exc:
        print(f"HIL capture error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
