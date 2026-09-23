#!/usr/bin/env python3
"""Build and enforce focused coverage for the Rack-independent VCV runtime adapter.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import gzip
import json
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build" / "vcv-coverage"
TARGET_SOURCE = (ROOT / "vcv" / "clock_vcv_runtime.cpp").resolve()
COVERAGE_DIR = ROOT / "coverage"


def run(command: list[str], cwd: Path = ROOT) -> None:
    print("+", " ".join(command))
    subprocess.run(command, cwd=cwd, check=True)


def collect_report(notes_file: Path) -> tuple[float, float, float, int, int, int, int, int, int]:
    object_dir = notes_file.parent
    for stale in object_dir.glob("*.gcov.json.gz"):
        stale.unlink()
    run(["gcov", "-j", "-b", notes_file.name], cwd=object_dir)

    for report_path in object_dir.glob("*.gcov.json.gz"):
        with gzip.open(report_path, "rt", encoding="utf-8") as handle:
            report = json.load(handle)
        for file_report in report.get("files", []):
            source = Path(file_report["file"]).resolve()
            if source != TARGET_SOURCE:
                continue
            lines = file_report.get("lines", [])
            functions = file_report.get("functions", [])
            covered_lines = sum(int(line.get("count", 0)) > 0 for line in lines)
            covered_functions = sum(
                int(function.get("execution_count", 0)) > 0 for function in functions
            )
            decision_branches = [
                branch
                for line in lines
                for branch in line.get("branches", [])
                if not bool(branch.get("throw", False))
            ]
            covered_decision_branches = sum(
                int(branch.get("count", 0)) > 0 for branch in decision_branches
            )
            line_percent = 100.0 if not lines else covered_lines * 100.0 / len(lines)
            function_percent = (
                100.0 if not functions else covered_functions * 100.0 / len(functions)
            )
            branch_percent = (
                100.0
                if not decision_branches
                else covered_decision_branches * 100.0 / len(decision_branches)
            )
            return (
                line_percent,
                function_percent,
                branch_percent,
                covered_lines,
                len(lines),
                covered_functions,
                len(functions),
                covered_decision_branches,
                len(decision_branches),
            )
    raise RuntimeError("gcov report did not contain vcv/clock_vcv_runtime.cpp")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--minimum-line", type=float, default=95.0)
    parser.add_argument("--minimum-function", type=float, default=95.0)
    parser.add_argument("--minimum-branch", type=float, default=90.0)
    args = parser.parse_args()

    if BUILD.exists():
        shutil.rmtree(BUILD)
    run([
        "cmake",
        "-S", ".",
        "-B", str(BUILD.relative_to(ROOT)),
        "-DCLOCK_SIMULATOR_WITH_SDL=OFF",
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_CXX_FLAGS=--coverage -O0",
        "-DCMAKE_EXE_LINKER_FLAGS=--coverage",
    ])
    run([
        "cmake", "--build", str(BUILD.relative_to(ROOT)),
        "--target", "clock-vcv-runtime-tests", "-j2",
    ])
    run([
        "ctest", "--test-dir", str(BUILD.relative_to(ROOT)),
        "--output-on-failure", "-R", "vcv_runtime_adapter_tests",
    ])

    notes = next(BUILD.rglob("clock_vcv_runtime.cpp.gcno"), None)
    if notes is None:
        raise RuntimeError("VCV runtime coverage notes file was not produced")
    (
        line_percent,
        function_percent,
        branch_percent,
        covered_lines,
        total_lines,
        covered_functions,
        total_functions,
        covered_branches,
        total_branches,
    ) = collect_report(notes)

    report_text = (
        "VCV runtime adapter coverage\n"
        f"Lines:              {covered_lines}/{total_lines} ({line_percent:.2f}%)\n"
        f"Functions:          {covered_functions}/{total_functions} ({function_percent:.2f}%)\n"
        f"Decision branches:  {covered_branches}/{total_branches} ({branch_percent:.2f}%)\n"
    )
    print(report_text, end="")
    COVERAGE_DIR.mkdir(parents=True, exist_ok=True)
    (COVERAGE_DIR / "vcv_runtime_coverage.txt").write_text(report_text, encoding="utf-8")
    (COVERAGE_DIR / "vcv_runtime_coverage.json").write_text(
        json.dumps(
            {
                "source": "vcv/clock_vcv_runtime.cpp",
                "lines": {"covered": covered_lines, "total": total_lines, "percent": line_percent},
                "functions": {
                    "covered": covered_functions,
                    "total": total_functions,
                    "percent": function_percent,
                },
                "decision_branches": {
                    "covered": covered_branches,
                    "total": total_branches,
                    "percent": branch_percent,
                },
            },
            indent=2,
            sort_keys=True,
        ) + "\n",
        encoding="utf-8",
    )
    failures: list[str] = []
    if line_percent < args.minimum_line:
        failures.append(
            f"line coverage is {line_percent:.2f}% below {args.minimum_line:.2f}%"
        )
    if function_percent < args.minimum_function:
        failures.append(
            f"function coverage is {function_percent:.2f}% below {args.minimum_function:.2f}%"
        )
    if branch_percent < args.minimum_branch:
        failures.append(
            f"decision-branch coverage is {branch_percent:.2f}% below "
            f"{args.minimum_branch:.2f}%"
        )
    if failures:
        raise RuntimeError("; ".join(failures))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
