#!/usr/bin/env python3
"""Validate the CLOCK HIL qualification ledger and optional release gate.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PLAN = ROOT / "docs/HIL_TEST_PLAN.md"
LEDGER = ROOT / "docs/qualification/v1_qualification.json"
VERSION = ROOT / "src/version.h"
ID_RE = re.compile(r"^###\s+(HIL-[A-Z]+-\d{3})\s+—", re.MULTILINE)
VERSION_RE = re.compile(r'^#define\s+CLOCK_FIRMWARE_VERSION\s+"([^"]+)"\s*$', re.MULTILINE)
SEMVER_RE = re.compile(
    r"^(?P<major>0|[1-9]\d*)\.(?P<minor>0|[1-9]\d*)\.(?P<patch>0|[1-9]\d*)"
    r"(?:-(?P<prerelease>[0-9A-Za-z.-]+))?(?:\+[0-9A-Za-z.-]+)?$"
)
ALLOWED_STATUSES = {"PENDING", "BLOCKED", "PASS", "FAIL"}
DEFAULT_ENFORCE_FROM = "1.5.0"


def current_version() -> str:
    """Read the repository firmware version."""
    match = VERSION_RE.search(VERSION.read_text(encoding="utf-8"))
    if match is None:
        raise ValueError("src/version.h does not define CLOCK_FIRMWARE_VERSION")
    return match.group(1)


def parse_semver(version: str) -> tuple[tuple[int, int, int], str | None]:
    """Return SemVer core and prerelease string, rejecting malformed versions."""
    match = SEMVER_RE.fullmatch(version)
    if match is None:
        raise ValueError(f"invalid semantic version: {version!r}")
    core = (int(match.group("major")), int(match.group("minor")), int(match.group("patch")))
    return core, match.group("prerelease")


def should_require_pass(version: str, enforce_from: str = DEFAULT_ENFORCE_FROM) -> bool:
    """Enforce HIL evidence for stable/RC releases at or beyond the threshold.

    Alpha and beta builds remain advisory even after the threshold so development
    snapshots are never blocked by unfinished bench work. Release candidates and
    stable releases at/after the threshold require a complete PASS ledger.
    """
    core, prerelease = parse_semver(version)
    threshold_core, threshold_prerelease = parse_semver(enforce_from)
    if threshold_prerelease is not None:
        raise ValueError("HIL enforcement threshold must be a stable SemVer core")
    if core < threshold_core:
        return False
    if prerelease is None:
        return True
    first_identifier = prerelease.split(".", 1)[0].lower()
    return first_identifier == "rc"


def plan_ids() -> list[str]:
    """Extract ordered HIL test IDs from the normative test plan."""
    ids = ID_RE.findall(PLAN.read_text(encoding="utf-8"))
    if not ids:
        raise ValueError("docs/HIL_TEST_PLAN.md contains no HIL test IDs")
    if len(ids) != len(set(ids)):
        raise ValueError("docs/HIL_TEST_PLAN.md contains duplicate HIL test IDs")
    return ids


def validate(require_pass: bool) -> list[str]:
    """Return ledger validation errors; PASS entries require concrete evidence paths."""
    errors: list[str] = []
    data = json.loads(LEDGER.read_text(encoding="utf-8"))
    if data.get("schema") != 1:
        errors.append("qualification ledger schema must be 1")
    if data.get("product") != "South Signal Lab CLOCK":
        errors.append("qualification ledger product identity mismatch")
    if data.get("baseline") != current_version():
        errors.append(f"qualification ledger baseline must match firmware version {current_version()}")
    if data.get("scheduler_tick_us") != 50:
        errors.append("qualification ledger must record the 50 us V1 scheduler quantum")
    tests = data.get("tests")
    if not isinstance(tests, list):
        return errors + ["qualification ledger tests must be a list"]

    expected_ids = plan_ids()
    actual_ids: list[str] = []
    for index, entry in enumerate(tests):
        if not isinstance(entry, dict):
            errors.append(f"tests[{index}] must be an object")
            continue
        test_id = entry.get("id")
        status = entry.get("status")
        evidence = entry.get("evidence")
        actual_ids.append(str(test_id))
        if status not in ALLOWED_STATUSES:
            errors.append(f"{test_id}: invalid status {status!r}")
        if not isinstance(evidence, list):
            errors.append(f"{test_id}: evidence must be a list")
            evidence = []
        for evidence_path in evidence:
            if not isinstance(evidence_path, str) or not evidence_path.strip():
                errors.append(f"{test_id}: evidence entries must be non-empty strings")
            elif evidence_path.startswith("/") or ".." in Path(evidence_path).parts:
                errors.append(f"{test_id}: evidence path must be repository-relative")
        if status == "PASS" and not evidence:
            errors.append(f"{test_id}: PASS requires at least one evidence artifact")
        if status == "PASS":
            for evidence_path in evidence:
                if isinstance(evidence_path, str) and evidence_path.strip():
                    resolved = ROOT / evidence_path
                    if not resolved.is_file():
                        errors.append(f"{test_id}: PASS evidence does not exist: {evidence_path}")
        if require_pass and status != "PASS":
            errors.append(f"{test_id}: enforced HIL gate requires PASS, found {status}")

    if actual_ids != expected_ids:
        errors.append(
            "qualification ledger test IDs/order must exactly match docs/HIL_TEST_PLAN.md: "
            f"expected {expected_ids!r}, got {actual_ids!r}"
        )
    return errors


def main() -> int:
    """Validate ledger structure and conditionally enforce the all-PASS gate."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--require-pass", action="store_true", help="require every HIL test to be PASS")
    parser.add_argument(
        "--require-pass-from",
        default=None,
        metavar="VERSION",
        help=(
            "automatically require all PASS for release candidates and stable releases "
            "at or beyond VERSION; alpha/beta builds remain advisory"
        ),
    )
    args = parser.parse_args()
    version = current_version()
    try:
        auto_gate = bool(args.require_pass_from) and should_require_pass(version, args.require_pass_from)
        errors = validate(args.require_pass or auto_gate)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"HIL qualification validation failed: {exc}", file=sys.stderr)
        return 1
    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 2
    data = json.loads(LEDGER.read_text(encoding="utf-8"))
    counts = {status: 0 for status in sorted(ALLOWED_STATUSES)}
    for entry in data.get("tests", []):
        if isinstance(entry, dict) and entry.get("status") in counts:
            counts[entry["status"]] += 1
    if args.require_pass or auto_gate:
        status = "enforced gate PASS"
    elif args.require_pass_from:
        status = f"advisory ledger PASS (hard gate starts at {args.require_pass_from})"
    else:
        status = "ledger structure PASS"
    print(f"HIL qualification: {status} ({version})")
    print("  " + ", ".join(f"{name}={counts[name]}" for name in ("PASS", "PENDING", "BLOCKED", "FAIL")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
