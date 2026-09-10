"""Regression tests for CLOCK HIL capture and qualification tooling.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import importlib.util
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PYTHON = sys.executable


def load_module(name: str, path: Path):
    """Load a repository script as a testable Python module."""
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


ANALYZER = load_module("analyze_hil_capture", ROOT / "scripts/analyze_hil_capture.py")
QUAL = load_module("check_hil_qualification", ROOT / "scripts/check_hil_qualification.py")


class HilCaptureToolTests(unittest.TestCase):
    """Exercise the machine-readable HIL timing analyzer."""

    def write_capture(self, directory: Path, name: str, rows: list[tuple[float, str, int]]) -> Path:
        """Write one canonical long-form edge capture."""
        path = directory / name
        lines = ["time_us,signal,level"]
        lines.extend(f"{time:.3f},{signal},{level}" for time, signal, level in sorted(rows))
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")
        return path

    def periodic_rows(
        self,
        signal: str,
        count: int,
        period_us: float,
        width_us: float,
        offset_us: float = 0.0,
        period_jitter_us: float = 0.0,
    ) -> list[tuple[float, str, int]]:
        """Build deterministic pulse rows with alternating period perturbation."""
        rows: list[tuple[float, str, int]] = []
        time = offset_us
        for index in range(count):
            rows.append((time, signal, 1))
            rows.append((time + width_us, signal, 0))
            if index + 1 < count:
                time += period_us + (period_jitter_us if index % 2 == 0 else -period_jitter_us)
        return rows

    def test_parse_rejects_non_monotonic_capture(self) -> None:
        """Global timestamp ordering is part of the exchange contract."""
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "bad.csv"
            path.write_text("time_us,signal,level\n10,OUT1,1\n9,OUT1,0\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "monotonic"):
                ANALYZER.parse_capture(path)

    def test_pulse_width_command_passes_and_fails_objective_tolerance(self) -> None:
        """The same capture must produce deterministic PASS/FAIL at the threshold boundary."""
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            path = self.write_capture(
                directory,
                "width.csv",
                self.periodic_rows("OUT1", 12, 500000.0, 10025.0),
            )
            pass_result = subprocess.run(
                [PYTHON, str(ROOT / "scripts/analyze_hil_capture.py"), "pulse-width", str(path),
                 "--signal", "OUT1", "--expected-us", "10000", "--tolerance-us", "50"],
                text=True, capture_output=True, check=False,
            )
            self.assertEqual(pass_result.returncode, 0, pass_result.stderr)
            self.assertEqual(json.loads(pass_result.stdout)["status"], "PASS")

            fail_result = subprocess.run(
                [PYTHON, str(ROOT / "scripts/analyze_hil_capture.py"), "pulse-width", str(path),
                 "--signal", "OUT1", "--expected-us", "10000", "--tolerance-us", "20"],
                text=True, capture_output=True, check=False,
            )
            self.assertEqual(fail_result.returncode, 2)
            self.assertEqual(json.loads(fail_result.stdout)["status"], "FAIL")

    def test_skew_analysis_matches_all_eight_outputs(self) -> None:
        """Eight-channel simultaneous events are matched to OUT1 without hiding worst-case skew."""
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            rows: list[tuple[float, str, int]] = []
            offsets = [0.0, 2.0, 4.0, 7.0, 9.0, 12.0, 15.0, 18.0]
            for index, offset in enumerate(offsets, start=1):
                rows += self.periodic_rows(f"OUT{index}", 12, 500000.0, 10000.0, offset)
            path = self.write_capture(directory, "skew.csv", rows)
            result = subprocess.run(
                [PYTHON, str(ROOT / "scripts/analyze_hil_capture.py"), "skew", str(path),
                 "--signals", ",".join(f"OUT{i}" for i in range(1, 9)),
                 "--max-skew-us", "50", "--min-groups", "10"],
                text=True, capture_output=True, check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            payload = json.loads(result.stdout)
            self.assertEqual(payload["status"], "PASS")
            self.assertAlmostEqual(payload["observed_skew_us"]["max"], 18.0)

    def test_display_stress_comparison_detects_event_loss(self) -> None:
        """Equal mean frequency must not hide a missing event under display load."""
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            baseline_rows = self.periodic_rows("OUT1", 20, 500000.0, 10000.0)
            stress_rows = self.periodic_rows("OUT1", 19, 500000.0, 10000.0)
            baseline = self.write_capture(directory, "idle.csv", baseline_rows)
            stress = self.write_capture(directory, "stress.csv", stress_rows)
            result = subprocess.run(
                [PYTHON, str(ROOT / "scripts/analyze_hil_capture.py"), "compare", str(baseline), str(stress),
                 "--signal", "OUT1", "--max-added-deviation-us", "50", "--require-same-event-count"],
                text=True, capture_output=True, check=False,
            )
            self.assertEqual(result.returncode, 2)
            payload = json.loads(result.stdout)
            self.assertEqual(payload["status"], "FAIL")
            self.assertEqual(payload["event_count_delta"], -1)

    def test_display_stress_comparison_detects_added_timing_excursion(self) -> None:
        """Display correlation is checked against worst period deviation, not only the average."""
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            baseline = self.write_capture(
                directory, "idle.csv", self.periodic_rows("OUT1", 20, 500000.0, 10000.0, period_jitter_us=5.0)
            )
            stress = self.write_capture(
                directory, "stress.csv", self.periodic_rows("OUT1", 20, 500000.0, 10000.0, period_jitter_us=80.0)
            )
            result = subprocess.run(
                [PYTHON, str(ROOT / "scripts/analyze_hil_capture.py"), "compare", str(baseline), str(stress),
                 "--signal", "OUT1", "--max-added-deviation-us", "50", "--require-same-event-count"],
                text=True, capture_output=True, check=False,
            )
            self.assertEqual(result.returncode, 2)
            self.assertGreater(json.loads(result.stdout)["added_max_median_deviation_us"], 50.0)


class HilQualificationLedgerTests(unittest.TestCase):
    """Protect the release-gating semantics of the physical qualification ledger."""

    def test_repository_ledger_matches_every_normative_hil_test(self) -> None:
        """Beta is allowed to be incomplete, but the ledger may not omit a HIL test."""
        self.assertEqual(QUAL.validate(require_pass=False), [])

    def test_pass_without_evidence_is_rejected(self) -> None:
        """A green status without evidence must never satisfy the release process."""
        original = QUAL.LEDGER
        with tempfile.TemporaryDirectory() as tmp:
            temp_ledger = Path(tmp) / "ledger.json"
            data = json.loads(original.read_text(encoding="utf-8"))
            data["tests"][0]["status"] = "PASS"
            data["tests"][0]["evidence"] = []
            temp_ledger.write_text(json.dumps(data), encoding="utf-8")
            QUAL.LEDGER = temp_ledger
            try:
                errors = QUAL.validate(require_pass=False)
            finally:
                QUAL.LEDGER = original
        self.assertTrue(any("PASS requires" in error for error in errors))

    def test_explicit_all_pass_gate_rejects_pending_physical_tests(self) -> None:
        """An explicitly requested all-PASS gate still rejects incomplete HIL evidence."""
        errors = QUAL.validate(require_pass=True)
        self.assertTrue(any("requires PASS" in error for error in errors))

    def test_hil_hard_gate_is_advisory_before_1_5(self) -> None:
        """Stable and RC releases below 1.5 must not be blocked by incomplete HIL."""
        for version in ("1.0.0-rc.1", "1.0.0", "1.4.9-rc.3", "1.4.9"):
            with self.subTest(version=version):
                self.assertFalse(QUAL.should_require_pass(version, "1.5.0"))

    def test_hil_hard_gate_starts_with_1_5_release_candidates(self) -> None:
        """From 1.5, release candidates and stable releases require all-PASS HIL."""
        for version in ("1.5.0-rc.1", "1.5.0", "1.6.0-rc.2", "2.0.0"):
            with self.subTest(version=version):
                self.assertTrue(QUAL.should_require_pass(version, "1.5.0"))

    def test_alpha_and_beta_remain_advisory_after_threshold(self) -> None:
        """Development prereleases remain usable while bench qualification is incomplete."""
        for version in ("1.5.0-alpha.1", "1.5.0-beta.4", "2.0.0-beta.1"):
            with self.subTest(version=version):
                self.assertFalse(QUAL.should_require_pass(version, "1.5.0"))

    def test_pass_with_missing_evidence_file_is_rejected(self) -> None:
        """PASS evidence must resolve to a real repository file rather than a decorative path."""
        original = QUAL.LEDGER
        with tempfile.TemporaryDirectory() as tmp:
            temp_ledger = Path(tmp) / "ledger.json"
            data = json.loads(original.read_text(encoding="utf-8"))
            data["tests"][0]["status"] = "PASS"
            data["tests"][0]["evidence"] = ["docs/qualification/evidence/does-not-exist.csv"]
            temp_ledger.write_text(json.dumps(data), encoding="utf-8")
            QUAL.LEDGER = temp_ledger
            try:
                errors = QUAL.validate(require_pass=False)
            finally:
                QUAL.LEDGER = original
        self.assertTrue(any("does not exist" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
