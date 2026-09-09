#!/usr/bin/env python3
"""Build, execute, and aggregate full host-side firmware coverage.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""

from __future__ import annotations

import argparse
import gzip
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD_ROOT = ROOT / "build" / "host_tests"
COVERAGE_DIR = ROOT / "coverage"
FAKE_INCLUDE = ROOT / "test" / "support" / "fake_framework"

WARNING_FLAGS = [
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-Wshadow",
    "-Wconversion",
    "-Wsign-conversion",
    "-Werror",
]

COVERAGE_FLAGS = [
    "-std=c++17",
    "-O0",
    "-g",
    "--coverage",
    *WARNING_FLAGS,
]

SANITIZER_FLAGS = [
    "-std=c++17",
    "-O1",
    "-g",
    "-fno-omit-frame-pointer",
    "-fsanitize=address,undefined",
    *WARNING_FLAGS,
]


def run(
    command: list[str],
    cwd: Path = ROOT,
    environment: dict[str, str] | None = None,
) -> None:
    """Run one subprocess and fail immediately when it returns non-zero."""
    print("+", " ".join(command))
    subprocess.run(command, cwd=cwd, env=environment, check=True)


def production_sources() -> list[Path]:
    """Return all firmware implementation units except the native-smoke entry point."""
    sources = sorted((ROOT / "src").rglob("*.cpp"))
    sources = [path for path in sources if path.name != "native_smoke_main.cpp"]
    sources.extend(sorted((ROOT / "lib" / "clock_core" / "src").glob("*.cpp")))
    return sources


def compile_host_firmware_variant(name: str, defines: list[str]) -> Path:
    """Compile the complete firmware against deterministic fake framework headers."""
    build_dir = BUILD_ROOT / name
    build_dir.mkdir(parents=True, exist_ok=True)
    executable = build_dir / "tests"
    command = [
        os.environ.get("CXX", "g++"),
        *COVERAGE_FLAGS,
        "-DCLOCK_HOST_TEST=1",
        *defines,
        "-I", str(ROOT / "src"),
        "-I", str(ROOT / "lib" / "clock_core" / "src"),
        "-I", str(FAKE_INCLUDE),
        *[str(path) for path in production_sources()],
        str(ROOT / "test" / "host_firmware" / "test_main.cpp"),
        "-o", str(executable),
    ]
    run(command)
    run([str(executable)], cwd=build_dir)
    return build_dir


def compile_core_suite() -> Path:
    """Compile and execute the exhaustive deterministic-core suite without PlatformIO."""
    build_dir = BUILD_ROOT / "core"
    build_dir.mkdir(parents=True, exist_ok=True)
    executable = build_dir / "tests"
    run([
        os.environ.get("CXX", "g++"),
        *COVERAGE_FLAGS,
        "-I", str(ROOT / "src"),
        "-I", str(ROOT / "lib" / "clock_core" / "src"),
        "-I", str(FAKE_INCLUDE),
        str(ROOT / "lib" / "clock_core" / "src" / "clock_core.cpp"),
        str(ROOT / "test" / "test_clock_core" / "test_main.cpp"),
        "-o", str(executable),
    ])
    run([str(executable)], cwd=build_dir)
    return build_dir


def realtime_sources() -> list[Path]:
    """Return only production units needed by the focused real-time integration suite."""
    relative = [
        "lib/clock_core/src/clock_core.cpp",
        "src/domain/default_configuration.cpp",
        "src/engine/clock_engine.cpp",
        "src/engine/clock_engine_sync.cpp",
        "src/engine/clock_engine_timing.cpp",
        "src/engine/output_mode_resolver.cpp",
        "src/hal/external_input_capture.cpp",
        "src/hal/gate_output_driver.cpp",
        "src/hal/interrupt_lock.cpp",
        "src/hal/system_clock.cpp",
        "src/services/external_sync_controller.cpp",
    ]
    return [ROOT / item for item in relative]


def compile_realtime_suite(
    name: str = "realtime",
    defines: list[str] | None = None,
) -> Path:
    """Compile timing/SYNC/RST stress tests as a fast, focused integration suite."""
    build_dir = BUILD_ROOT / name
    build_dir.mkdir(parents=True, exist_ok=True)
    executable = build_dir / "tests"
    run([
        os.environ.get("CXX", "g++"),
        *COVERAGE_FLAGS,
        "-DCLOCK_HOST_TEST=1",
        *(defines or []),
        "-I", str(ROOT / "src"),
        "-I", str(ROOT / "lib" / "clock_core" / "src"),
        "-I", str(FAKE_INCLUDE),
        *[str(path) for path in realtime_sources()],
        str(ROOT / "test" / "realtime" / "test_main.cpp"),
        "-o", str(executable),
    ])
    run([str(executable)], cwd=build_dir)
    return build_dir


def compile_sanitized_realtime_suite(
    name: str = "realtime",
    defines: list[str] | None = None,
) -> None:
    """Run timing/SYNC/RST stress tests under ASan and UBSan."""
    build_dir = BUILD_ROOT / f"sanitizer_{name}"
    build_dir.mkdir(parents=True, exist_ok=True)
    executable = build_dir / "tests"
    run([
        os.environ.get("CXX", "g++"),
        *SANITIZER_FLAGS,
        "-DCLOCK_HOST_TEST=1",
        *(defines or []),
        "-I", str(ROOT / "src"),
        "-I", str(ROOT / "lib" / "clock_core" / "src"),
        "-I", str(FAKE_INCLUDE),
        *[str(path) for path in realtime_sources()],
        str(ROOT / "test" / "realtime" / "test_main.cpp"),
        "-o", str(executable),
    ])
    run_sanitized_executable(executable, build_dir)


def compile_native_smoke() -> Path:
    """Execute the PlatformIO native-build entry point under coverage as test-support code."""
    build_dir = BUILD_ROOT / "native_smoke"
    build_dir.mkdir(parents=True, exist_ok=True)
    executable = build_dir / "smoke"
    run([
        os.environ.get("CXX", "g++"),
        *COVERAGE_FLAGS,
        "-DCLOCK_NATIVE_SMOKE_BUILD=1",
        str(ROOT / "src" / "native_smoke_main.cpp"),
        "-o", str(executable),
    ])
    run([str(executable)], cwd=build_dir)
    return build_dir


def compile_sanitized_core_suite() -> None:
    """Run the deterministic core suite under AddressSanitizer and UndefinedBehaviorSanitizer."""
    build_dir = BUILD_ROOT / "sanitizer_core"
    build_dir.mkdir(parents=True, exist_ok=True)
    executable = build_dir / "tests"
    run([
        os.environ.get("CXX", "g++"),
        *SANITIZER_FLAGS,
        "-I", str(ROOT / "src"),
        "-I", str(ROOT / "lib" / "clock_core" / "src"),
        "-I", str(FAKE_INCLUDE),
        str(ROOT / "lib" / "clock_core" / "src" / "clock_core.cpp"),
        str(ROOT / "test" / "test_clock_core" / "test_main.cpp"),
        "-o", str(executable),
    ])
    run_sanitized_executable(executable, build_dir)


def compile_sanitized_firmware_variant(name: str, defines: list[str]) -> None:
    """Run one complete firmware configuration under ASan and UBSan."""
    build_dir = BUILD_ROOT / f"sanitizer_{name}"
    build_dir.mkdir(parents=True, exist_ok=True)
    executable = build_dir / "tests"
    run([
        os.environ.get("CXX", "g++"),
        *SANITIZER_FLAGS,
        "-DCLOCK_HOST_TEST=1",
        *defines,
        "-I", str(ROOT / "src"),
        "-I", str(ROOT / "lib" / "clock_core" / "src"),
        "-I", str(FAKE_INCLUDE),
        *[str(path) for path in production_sources()],
        str(ROOT / "test" / "host_firmware" / "test_main.cpp"),
        "-o", str(executable),
    ])
    run_sanitized_executable(executable, build_dir)


def run_sanitized_executable(executable: Path, build_dir: Path) -> None:
    """Execute one sanitizer binary with fail-fast diagnostics enabled."""
    environment = os.environ.copy()
    environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    run([str(executable)], cwd=build_dir, environment=environment)


def run_sanitizer_matrix() -> None:
    """Execute core, I2C, SPI, and configured-reset host binaries under ASan/UBSan."""
    compile_sanitized_core_suite()
    compile_sanitized_realtime_suite()
    compile_sanitized_realtime_suite(
        "realtime_external_gpio",
        ["-DCLOCK_EXTERNAL_SYNC_PIN=PB3", "-DCLOCK_EXTERNAL_RESET_PIN=PB4"],
    )
    compile_sanitized_firmware_variant("firmware_i2c", [])
    compile_sanitized_firmware_variant(
        "firmware_spi_ssd1306", ["-DCLOCK_DISPLAY_USE_SPI=1", "-DCLOCK_DISPLAY_CONTROLLER=1306"])
    compile_sanitized_firmware_variant(
        "firmware_spi_ssd1315", ["-DCLOCK_DISPLAY_USE_SPI=1", "-DCLOCK_DISPLAY_CONTROLLER=1315"])
    compile_sanitized_firmware_variant(
        "firmware_i2c_fixed_reset",
        ["-DCLOCK_DISPLAY_I2C_ADDRESS=0x3C", "-DCLOCK_DISPLAY_RESET_PIN=0xA06"],
    )


def normalize_source_path(filename: str) -> str | None:
    """Normalize gcov source paths and keep only project-owned production/test-glue code."""
    path = Path(filename)
    try:
        if path.is_absolute():
            path = path.resolve().relative_to(ROOT.resolve())
    except ValueError:
        return None
    normalized = path.as_posix()
    if normalized.startswith("src/") or normalized.startswith("lib/clock_core/src/"):
        return normalized
    return None


def collect_coverage(build_dirs: list[Path]) -> dict[str, object]:
    """Aggregate GCC JSON coverage across I2C, SPI, reset-pin, core, and smoke variants."""
    line_counts: dict[tuple[str, int], int] = {}
    function_counts: dict[tuple[str, str, int], int] = {}
    branch_counts: dict[tuple[str, int, int, bool, bool], int] = {}
    gcov = os.environ.get("GCOV", "gcov")

    for build_dir in build_dirs:
        for old_json in build_dir.glob("*.gcov.json.gz"):
            old_json.unlink()
        for notes_file in build_dir.glob("*.gcno"):
            subprocess.run(
                [gcov, "-j", "-b", str(notes_file)],
                cwd=build_dir,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=True,
            )
        for report_path in build_dir.glob("*.gcov.json.gz"):
            with gzip.open(report_path, "rt", encoding="utf-8") as handle:
                report = json.load(handle)
            for file_report in report.get("files", []):
                source = normalize_source_path(file_report["file"])
                if source is None:
                    continue
                for line in file_report.get("lines", []):
                    line_key = (source, int(line["line_number"]))
                    line_counts[line_key] = line_counts.get(line_key, 0) + int(line.get("count", 0))
                    for branch_index, branch in enumerate(line.get("branches", [])):
                        branch_key = (
                            source,
                            int(line["line_number"]),
                            branch_index,
                            bool(branch.get("fallthrough", False)),
                            bool(branch.get("throw", False)),
                        )
                        branch_counts[branch_key] = branch_counts.get(branch_key, 0) + int(branch.get("count", 0))
                for function in file_report.get("functions", []):
                    function_key = (
                        source,
                        function.get("demangled_name", function["name"]),
                        int(function["start_line"]),
                    )
                    function_counts[function_key] = (
                        function_counts.get(function_key, 0) + int(function.get("execution_count", 0))
                    )

    covered_lines = sum(count > 0 for count in line_counts.values())
    covered_functions = sum(count > 0 for count in function_counts.values())
    covered_compiler_branches = sum(count > 0 for count in branch_counts.values())
    decision_branch_counts = {
        key: count for key, count in branch_counts.items() if not key[4]
    }
    covered_decision_branches = sum(count > 0 for count in decision_branch_counts.values())

    def percent(covered: int, total: int) -> float:
        return 100.0 if total == 0 else covered * 100.0 / total

    uncovered_lines = [
        {"file": filename, "line": line_number}
        for (filename, line_number), count in sorted(line_counts.items())
        if count == 0
    ]
    uncovered_functions = [
        {"file": filename, "line": line_number, "name": name}
        for (filename, name, line_number), count in sorted(function_counts.items())
        if count == 0
    ]
    uncovered_decision_branches = [
        {"file": filename, "line": line_number, "branch": branch_index}
        for (filename, line_number, branch_index, _fallthrough, _throw), count
        in sorted(decision_branch_counts.items())
        if count == 0
    ]

    return {
        "lines": {
            "covered": covered_lines,
            "total": len(line_counts),
            "percent": percent(covered_lines, len(line_counts)),
            "uncovered": uncovered_lines,
        },
        "functions": {
            "covered": covered_functions,
            "total": len(function_counts),
            "percent": percent(covered_functions, len(function_counts)),
            "uncovered": uncovered_functions,
        },
        "decision_branches": {
            "covered": covered_decision_branches,
            "total": len(decision_branch_counts),
            "percent": percent(covered_decision_branches, len(decision_branch_counts)),
            "uncovered": uncovered_decision_branches,
        },
        "compiler_branches": {
            "covered": covered_compiler_branches,
            "total": len(branch_counts),
            "percent": percent(covered_compiler_branches, len(branch_counts)),
        },
    }


def write_coverage_report(report: dict[str, object]) -> None:
    """Write machine-readable JSON and concise text reports for CI artifacts."""
    COVERAGE_DIR.mkdir(parents=True, exist_ok=True)
    (COVERAGE_DIR / "full_coverage.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    lines = report["lines"]
    functions = report["functions"]
    decision_branches = report["decision_branches"]
    compiler_branches = report["compiler_branches"]
    text = (
        "Full firmware host coverage\n"
        f"Lines:              {lines['covered']}/{lines['total']} ({lines['percent']:.2f}%)\n"
        f"Functions:          {functions['covered']}/{functions['total']} ({functions['percent']:.2f}%)\n"
        f"Decision branches:  {decision_branches['covered']}/{decision_branches['total']} "
        f"({decision_branches['percent']:.2f}%)\n"
        f"Compiler branches:  {compiler_branches['covered']}/{compiler_branches['total']} "
        f"({compiler_branches['percent']:.2f}%, informational)\n"
    )
    (COVERAGE_DIR / "full_coverage.txt").write_text(text, encoding="utf-8")
    print(text, end="")


def enforce_thresholds(
    report: dict[str, object],
    minimum_line: float,
    minimum_function: float,
    minimum_branch: float,
) -> None:
    """Fail CI when repository coverage falls below the configured quality gates."""
    lines = report["lines"]
    functions = report["functions"]
    decision_branches = report["decision_branches"]
    failures: list[str] = []
    if lines["percent"] < minimum_line:
        failures.append(
            f"line coverage is {lines['percent']:.2f}% below {minimum_line:.2f}%"
        )
    if functions["percent"] < minimum_function:
        failures.append(
            f"function coverage is {functions['percent']:.2f}% below {minimum_function:.2f}%"
        )
    if decision_branches["percent"] < minimum_branch:
        failures.append(
            f"decision-branch coverage is {decision_branches['percent']:.2f}% "
            f"below {minimum_branch:.2f}%"
        )
    if failures:
        for item in lines["uncovered"]:
            print(f"UNCOVERED LINE: {item['file']}:{item['line']}", file=sys.stderr)
        for item in functions["uncovered"]:
            print(
                f"UNCOVERED FUNCTION: {item['file']}:{item['line']} {item['name']}",
                file=sys.stderr,
            )
        for item in decision_branches["uncovered"]:
            print(
                f"UNCOVERED DECISION BRANCH: {item['file']}:{item['line']} "
                f"branch {item['branch']}",
                file=sys.stderr,
            )
        raise RuntimeError("; ".join(failures))


def main() -> int:
    """Execute all host suites and enforce repository coverage/sanitizer policy."""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--minimum-line",
        type=float,
        default=95.0,
        help="Minimum executable-line coverage percentage (default: 95).",
    )
    parser.add_argument(
        "--minimum-function",
        type=float,
        default=95.0,
        help="Minimum function coverage percentage (default: 95).",
    )
    parser.add_argument(
        "--minimum-branch",
        type=float,
        default=90.0,
        help="Minimum non-throw decision-branch coverage percentage (default: 90).",
    )
    parser.add_argument(
        "--skip-sanitizers",
        action="store_true",
        help="Skip ASan/UBSan execution after coverage.",
    )
    parser.add_argument(
        "--sanitizers-only",
        action="store_true",
        help="Run only the ASan/UBSan matrix and skip coverage collection.",
    )
    args = parser.parse_args()

    if BUILD_ROOT.exists():
        shutil.rmtree(BUILD_ROOT)
    if args.sanitizers_only:
        run_sanitizer_matrix()
        return 0
    build_dirs = [
        compile_core_suite(),
        compile_realtime_suite(),
        compile_realtime_suite(
            "realtime_external_gpio",
            ["-DCLOCK_EXTERNAL_SYNC_PIN=PB3", "-DCLOCK_EXTERNAL_RESET_PIN=PB4"],
        ),
        compile_host_firmware_variant("firmware_i2c", []),
        compile_host_firmware_variant(
            "firmware_spi_ssd1306",
            ["-DCLOCK_DISPLAY_USE_SPI=1", "-DCLOCK_DISPLAY_CONTROLLER=1306"],
        ),
        compile_host_firmware_variant(
            "firmware_spi_ssd1315",
            ["-DCLOCK_DISPLAY_USE_SPI=1", "-DCLOCK_DISPLAY_CONTROLLER=1315"],
        ),
        compile_host_firmware_variant(
            "firmware_spi_custom_wiring",
            [
                "-DCLOCK_DISPLAY_USE_SPI=1",
                "-DCLOCK_DISPLAY_CONTROLLER=1315",
                "-DCLOCK_DISPLAY_SPI_SCK_PIN=0xA08",
                "-DCLOCK_DISPLAY_SPI_MOSI_PIN=0xA09",
                "-DCLOCK_DISPLAY_SPI_MISO_PIN=0xA0A",
                "-DCLOCK_DISPLAY_SPI_CS_PIN=0xB00",
                "-DCLOCK_DISPLAY_SPI_DC_PIN=0xB01",
                "-DCLOCK_DISPLAY_RESET_PIN=0xB05",
            ],
        ),
        compile_host_firmware_variant(
            "firmware_i2c_fixed_reset",
            ["-DCLOCK_DISPLAY_I2C_ADDRESS=0x3C", "-DCLOCK_DISPLAY_RESET_PIN=0xA06"],
        ),
        compile_native_smoke(),
    ]
    report = collect_coverage(build_dirs)
    write_coverage_report(report)
    enforce_thresholds(report, args.minimum_line, args.minimum_function, args.minimum_branch)
    if not args.skip_sanitizers:
        run_sanitizer_matrix()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
