#!/usr/bin/env python3
"""Validate the CLOCK V1 HIL qualification ledger and RC release gate.

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
ALLOWED_STATUSES = {"PENDING", "BLOCKED", "PASS", "FAIL"}


def current_version() -> str:
    """Read the repository firmware version."""
    match = VERSION_RE.search(VERSION.read_text(encoding="utf-8"))
    if match is None:
        raise ValueError("src/version.h does not define CLOCK_FIRMWARE_VERSION")
    return match.group(1)


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
            errors.append(f"{test_id}: RC/1.0 gate requires PASS, found {status}")

    if actual_ids != expected_ids:
        errors.append(
            "qualification ledger test IDs/order must exactly match docs/HIL_TEST_PLAN.md: "
            f"expected {expected_ids!r}, got {actual_ids!r}"
        )
    return errors


def main() -> int:
    """Validate ledger structure and optionally enforce the RC/1.0 all-PASS gate."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--require-pass", action="store_true", help="require every HIL test to be PASS")
    parser.add_argument(
        "--require-pass-if-rc",
        action="store_true",
        help="require all PASS automatically for 1.0.0-rc.* and stable 1.0.0",
    )
    args = parser.parse_args()
    version = current_version()
    auto_gate = args.require_pass_if_rc and (
        version.startswith("1.0.0-rc.") or version == "1.0.0"
    )
    try:
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
    status = "RC gate PASS" if args.require_pass or auto_gate else "ledger structure PASS"
    print(f"HIL qualification: {status} ({version})")
    print("  " + ", ".join(f"{name}={counts[name]}" for name in ("PASS", "PENDING", "BLOCKED", "FAIL")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
