#!/usr/bin/env python3
"""Enforce repository architecture and maintainability constraints.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP_ROOTS = (ROOT / "src", ROOT / "lib", ROOT / "test", ROOT / "sim")
HARDWARE_ALLOWED_PATHS = (
    Path("src/hal"),
    Path("src/pin_map.h"),
    Path("test/support/fake_framework"),
    Path("test/test_host_firmware"),
    Path("test/test_sync_behavior"),
    Path("test/test_swing"),
    Path("test/test_humanize"),
    Path("test/test_controls"),
    Path("sim/framework"),
    Path("sim/simulator_runtime.cpp"),
)
FRAMEWORK_INCLUDE_ALLOWED_PATHS = HARDWARE_ALLOWED_PATHS + (Path("src/main.cpp"),)
MAX_MAIN_LINES = 80
MAX_CPP_LINES = 450

# Deliberate implementation splits may share the public class header instead of
# introducing fake one-to-one headers that expose no additional interface.
IMPLEMENTATION_COMPANION_HEADERS = {
    "persistent_state_codec.cpp": "persistent_state_service.h",
    "persistent_state_migration.cpp": "persistent_state_service.h",
    "oled_display_transport.cpp": "oled_display.h",
    "screensaver_spectrum.cpp": "screensaver_renderer.h",
    "screensaver_field.cpp": "screensaver_renderer.h",
    "screensaver_blox.cpp": "screensaver_renderer.h",
    "screensaver_matrix.cpp": "screensaver_renderer.h",
    "screensaver_cube_cover.cpp": "screensaver_renderer.h",
    "moon_buggy_render.cpp": "moon_buggy_game.h",
    "clock_engine_timing.cpp": "clock_engine.h",
    "clock_engine_sync.cpp": "clock_engine.h",
    "ui_controller_navigation.cpp": "ui_controller.h",
    "ui_controller_display.cpp": "ui_controller.h",
    "ui_controller_presets.cpp": "ui_controller.h",
    "menu_model_sync.cpp": "menu_model.h",
    "settings_editor_sync.cpp": "settings_editor.h",
    "simulator_runtime_inputs.cpp": "simulator_runtime.h",
}

TEXT_METADATA_MARKERS = (
    "Author: Axel Napolitano",
    "License: PolyForm-Noncommercial-1.0.0",
)

REQUIRED_CPP_HEADER_MARKERS = (
    "@file",
    "@brief",
    "@author Axel Napolitano",
    "@license PolyForm-Noncommercial-1.0.0",
)

HARDWARE_INCLUDE_PATTERN = re.compile(
    r'^\s*#\s*include\s*[<"](?:Arduino|Wire|HardwareTimer|SPI|Adafruit_[^>"]+)',
    re.MULTILINE,
)

DIRECT_HARDWARE_API_PATTERN = re.compile(
    r"\b(?:digitalWrite|digitalRead|pinMode|millis|micros|delay|noInterrupts|interrupts)\s*\(",
)

# Static strings beginning with a letter are assumed to be user-visible when
# they appear in implementation code. Format-only literals such as "%u" are
# deliberately excluded. All UI prose belongs in src/ui_text.h.
STATIC_UI_LITERAL_PATTERN = re.compile(r'"[A-Za-z][A-Za-z0-9 /:%+>_.-]*"')

EXECUTABLE_INLINE_PATTERN = re.compile(
    r"^\s*inline(?:\s+constexpr)?\s+(?!constexpr\b)[^;=]+\([^;]*\)\s*\{",
    re.MULTILINE,
)


CPP_COMMENT_OR_LITERAL_PATTERN = re.compile(
    r'//.*?$|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
    re.MULTILINE | re.DOTALL,
)

HEAP_ALLOCATION_PATTERNS = (
    (re.compile(r"\bmalloc\s*\("), "malloc"),
    (re.compile(r"\bcalloc\s*\("), "calloc"),
    (re.compile(r"\brealloc\s*\("), "realloc"),
    (re.compile(r"\bfree\s*\("), "free"),
    (re.compile(r"\bnew\s+(?:[A-Za-z_:]|\[)"), "operator new"),
    (re.compile(r"\bdelete\s*(?:\[\s*\])?\s*[A-Za-z_]"), "operator delete"),
    (re.compile(r"\bstd::(?:vector|string|map|unordered_map|function|shared_ptr|unique_ptr|make_shared|make_unique)\b"), "dynamic STL facility"),
)


def repository_relative(path: Path) -> Path:
    """Return a stable repository-relative path for diagnostics and policy checks."""
    return path.resolve().relative_to(ROOT.resolve())


def is_allowed_path(path: Path, allowed_paths: tuple[Path, ...]) -> bool:
    """Return whether a file belongs to one of the supplied architecture paths."""
    relative_path = repository_relative(path)
    return any(
        relative_path == allowed or allowed in relative_path.parents
        for allowed in allowed_paths
    )


def is_hardware_allowed_path(path: Path) -> bool:
    """Return whether direct framework hardware APIs are allowed for this file."""
    return is_allowed_path(path, HARDWARE_ALLOWED_PATHS)


def is_framework_include_allowed_path(path: Path) -> bool:
    """Return whether framework headers may be included by this boundary file."""
    return is_allowed_path(path, FRAMEWORK_INCLUDE_ALLOWED_PATHS)


def iter_cpp_files() -> list[Path]:
    """Collect all project-owned C++ source and header files in deterministic order."""
    files: list[Path] = []
    for root in CPP_ROOTS:
        if not root.exists():
            continue
        files.extend(root.rglob("*.h"))
        files.extend(root.rglob("*.hpp"))
        files.extend(root.rglob("*.cpp"))
    return sorted(set(files))


def check_cpp_headers(errors: list[str]) -> None:
    """Require explicit Doxygen file, brief, author, and license metadata in every C++ file."""
    for path in iter_cpp_files():
        text = path.read_text(encoding="utf-8")
        for marker in REQUIRED_CPP_HEADER_MARKERS:
            if marker not in text[:1200]:
                errors.append(f"{repository_relative(path)}: missing header marker {marker!r}")


def check_text_metadata(errors: list[str]) -> None:
    """Require ownership/license metadata in project-owned textual repository files."""
    candidates: list[Path] = []
    candidates.extend(ROOT.glob("*.md"))
    candidates.extend(ROOT.glob("*.ini"))
    candidates.extend(ROOT.glob(".gitignore"))
    candidates.extend(ROOT.glob("Doxyfile"))
    candidates.extend(ROOT.glob(".editorconfig"))
    candidates.extend(ROOT.glob(".clang-format"))
    candidates.extend((ROOT / "docs").rglob("*.md"))
    candidates.extend((ROOT / "test").rglob("*.md"))
    candidates.extend((ROOT / "scripts").rglob("*.py"))
    candidates.extend((ROOT / ".github").rglob("*.yml"))
    candidates.extend((ROOT / ".github").rglob("*.yaml"))

    for path in sorted(set(candidates)):
        if path.name == "LICENSE.md":
            continue
        text = path.read_text(encoding="utf-8")
        for marker in TEXT_METADATA_MARKERS:
            if marker not in text[:1200]:
                errors.append(
                    f"{repository_relative(path)}: missing repository metadata marker {marker!r}"
                )



def check_api_briefs(errors: list[str]) -> None:
    """Require concise Doxygen briefs on production function and method declarations."""
    header_paths = sorted(list((ROOT / "src").rglob("*.h")) + list((ROOT / "lib").rglob("*.h")))
    name_pattern = re.compile(r"(?:~?[A-Za-z_]\w*|operator\s*[^\s]+)$")
    for path in header_paths:
        lines = path.read_text(encoding="utf-8").splitlines()
        in_comment = False
        statement: list[str] = []
        start_line = 0
        for index, line in enumerate(lines):
            stripped = line.strip()
            if in_comment:
                if "*/" in stripped:
                    in_comment = False
                continue
            if stripped.startswith("/*"):
                if "*/" not in stripped:
                    in_comment = True
                continue
            if not stripped or stripped.startswith(("//", "#")):
                continue

            if not statement:
                if "(" not in stripped:
                    continue
                statement = [stripped]
                start_line = index
            else:
                statement.append(stripped)

            combined = " ".join(statement)
            if ";" not in stripped:
                if "{" in stripped:
                    statement = []
                continue
            if "{" in combined:
                statement = []
                continue

            prefix = combined.split("(", 1)[0].strip()
            if prefix.startswith(("if ", "for ", "while ", "switch ", "return ", "static_assert", "using ")):
                statement = []
                continue
            if "=" in prefix and "operator=" not in prefix:
                statement = []
                continue
            if prefix in {"static_cast", "reinterpret_cast", "const_cast", "dynamic_cast", "sizeof", "alignof", "decltype"}:
                statement = []
                continue
            if name_pattern.search(prefix) is None:
                statement = []
                continue
            if " " not in prefix and "::" in prefix:
                statement = []
                continue

            previous = start_line - 1
            while previous >= 0 and not lines[previous].strip():
                previous -= 1
            block = ""
            if previous >= 0 and "*/" in lines[previous]:
                block_start = previous
                while block_start >= 0 and "/**" not in lines[block_start]:
                    block_start -= 1
                if block_start >= 0:
                    block = "\n".join(lines[block_start:previous + 1])
            if "@brief" not in block:
                errors.append(
                    f"{repository_relative(path)}:{start_line + 1}: function/method declaration requires a Doxygen @brief"
                )
            statement = []

def check_hardware_boundaries(errors: list[str]) -> None:
    """Keep framework headers and direct GPIO/time APIs inside HAL or pin mapping."""
    for path in iter_cpp_files():
        text = path.read_text(encoding="utf-8")
        if HARDWARE_INCLUDE_PATTERN.search(text) and not is_framework_include_allowed_path(path):
            errors.append(
                f"{repository_relative(path)}: framework include is only allowed in HAL/pin_map or src/main.cpp"
            )
        if DIRECT_HARDWARE_API_PATTERN.search(text) and not is_hardware_allowed_path(path):
            errors.append(
                f"{repository_relative(path)}: direct framework hardware API is only allowed in HAL"
            )


def check_header_colocation(errors: list[str]) -> None:
    """Require implementation headers to live beside their corresponding .cpp files."""
    entry_point_sources = {
        "main.cpp",
        "native_smoke_main.cpp",
        "headless_main.cpp",
        "simulator_tests.cpp",
    }
    implementation_roots = (ROOT / "src", ROOT / "sim")
    for implementation_root in implementation_roots:
        for source in sorted(implementation_root.rglob("*.cpp")):
            if source.name in entry_point_sources:
                continue
            expected_header = source.with_suffix(".h")
            companion_name = IMPLEMENTATION_COMPANION_HEADERS.get(source.name)
            if companion_name is not None:
                expected_header = source.with_name(companion_name)
            if not expected_header.is_file():
                errors.append(
                    f"{repository_relative(source)}: expected colocated header {repository_relative(expected_header)}"
                )


def check_ui_text_centralization(errors: list[str]) -> None:
    """Reject static user-facing text embedded outside the central language catalog."""
    candidates = list((ROOT / "src/ui").rglob("*.cpp"))
    candidates += [ROOT / "src/domain/clock_labels.cpp", ROOT / "src/services/template_service.cpp"]
    for path in sorted(set(candidates)):
        lines = path.read_text(encoding="utf-8").splitlines()
        for line_number, line in enumerate(lines, start=1):
            stripped = line.strip()
            if stripped.startswith("#include") or stripped.startswith("//") or "static_assert" in stripped:
                continue
            if STATIC_UI_LITERAL_PATTERN.search(line):
                errors.append(
                    f"{repository_relative(path)}:{line_number}: static UI text must come from src/ui_text.h"
                )


def check_external_library_policy(errors: list[str]) -> None:
    """Prevent accidental reintroduction of third-party display libraries."""
    platformio = (ROOT / "platformio.ini").read_text(encoding="utf-8")
    if "Adafruit" in platformio or "lib_deps" in platformio:
        errors.append("platformio.ini: third-party lib_deps are not permitted without an explicit architecture decision")


def check_header_executable_logic(errors: list[str]) -> None:
    """Keep executable functions out of headers so function coverage remains measurable."""
    for path in iter_cpp_files():
        relative_path = repository_relative(path)
        if relative_path.parts and relative_path.parts[0] == "test":
            continue
        if Path("sim/framework") in relative_path.parents:
            continue
        if path.suffix not in {".h", ".hpp"}:
            continue
        text = path.read_text(encoding="utf-8")
        if EXECUTABLE_INLINE_PATTERN.search(text):
            errors.append(
                f"{repository_relative(path)}: executable inline/header-only functions are not permitted; move logic to a colocated .cpp file"
            )


def count_lines(path: Path) -> int:
    """Count logical source lines using the repository's UTF-8 convention."""
    return len(path.read_text(encoding="utf-8").splitlines())


def check_heap_free_firmware(errors: list[str]) -> None:
    """Reject dynamic allocation primitives from embedded production code."""
    production_files = sorted(
        list((ROOT / "src").rglob("*.h")) +
        list((ROOT / "src").rglob("*.cpp")) +
        list((ROOT / "lib").rglob("*.h")) +
        list((ROOT / "lib").rglob("*.cpp"))
    )
    for path in production_files:
        text = path.read_text(encoding="utf-8")
        code = CPP_COMMENT_OR_LITERAL_PATTERN.sub(" ", text)
        for pattern, label in HEAP_ALLOCATION_PATTERNS:
            if pattern.search(code):
                errors.append(
                    f"{repository_relative(path)}: embedded firmware must remain heap-free; found {label}"
                )


def check_source_sizes(errors: list[str]) -> None:
    """Guard against future monolithic entry points and implementation files."""
    main_path = ROOT / "src/main.cpp"
    if main_path.exists() and count_lines(main_path) > MAX_MAIN_LINES:
        errors.append(
            f"src/main.cpp: {count_lines(main_path)} lines exceeds the {MAX_MAIN_LINES}-line entry-point limit"
        )

    implementation_files = list((ROOT / "src").rglob("*.cpp")) + list((ROOT / "sim").rglob("*.cpp"))
    for path in sorted(implementation_files):
        line_count = count_lines(path)
        if line_count > MAX_CPP_LINES:
            errors.append(
                f"{repository_relative(path)}: {line_count} lines exceeds the {MAX_CPP_LINES}-line implementation guideline"
            )


def main() -> int:
    """Run all architecture checks and return a shell-friendly status code."""
    errors: list[str] = []
    check_cpp_headers(errors)
    check_api_briefs(errors)
    check_text_metadata(errors)
    check_hardware_boundaries(errors)
    check_header_colocation(errors)
    check_ui_text_centralization(errors)
    check_external_library_policy(errors)
    check_header_executable_logic(errors)
    check_heap_free_firmware(errors)
    check_source_sizes(errors)

    if errors:
        print("Architecture check failed:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    print("Architecture check passed.")
    print(f"  C++ files checked: {len(iter_cpp_files())}")
    print("  Hardware APIs: HAL/pin_map only; Arduino entry declaration allowed in src/main.cpp")
    print("  Static UI text: centralized in src/ui_text.h")
    print("  Third-party PlatformIO libraries: none")
    print("  Embedded heap allocation: prohibited in src/ and lib/")
    print("  Headers: explicit @file/@brief metadata and production API briefs; colocated implementations; executable header-only logic prohibited")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
