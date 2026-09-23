"""Contract tests for the experimental VCV Rack Trial-First port.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""

from __future__ import annotations

import json
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class VcvManifestTests(unittest.TestCase):
    def test_manifest_uses_stable_unique_identity_and_rack2_major(self) -> None:
        manifest = json.loads((ROOT / "vcv/plugin.json").read_text(encoding="utf-8"))
        self.assertEqual(manifest["slug"], "SouthSignalLab-CLOCK")
        self.assertEqual(manifest["brand"], "South Signal Lab")
        self.assertEqual(manifest["modules"][0]["slug"], "CLOCK")
        self.assertTrue(str(manifest["version"]).startswith("2."))
        self.assertIn("Clock generator", manifest["modules"][0]["tags"])

    def test_makefile_compiles_production_sources_in_cpp17_mode(self) -> None:
        makefile = (ROOT / "vcv/Makefile").read_text(encoding="utf-8")
        self.assertIn("../src", makefile)
        self.assertIn("../lib/clock_core/src", makefile)
        self.assertIn("CLOCK_HOST_TEST=1", makefile)
        self.assertIn("CLOCK_SIMULATOR=1", makefile)
        self.assertIn("EXTRA_CXXFLAGS += -std=c++17", makefile)
        self.assertIn("-fno-unsafe-math-optimizations", makefile)
        self.assertNotIn("ClockEngine.cpp", makefile)

    def test_plugin_shell_does_not_implement_musical_scheduler(self) -> None:
        shell = (ROOT / "vcv/src/CLOCK.cpp").read_text(encoding="utf-8")
        bridge = (ROOT / "vcv/clock_vcv_runtime.cpp").read_text(encoding="utf-8")
        self.assertIn("ClockVcvRuntime", shell)
        self.assertIn("runtime_.advanceMicroseconds(config::kSchedulerTickUs)", bridge)
        self.assertNotRegex(shell, r"bpm|euclid|grooveAmount|sequencerStep")


class VcvWorkflowTests(unittest.TestCase):
    def test_workflow_uses_node24_generation_actions_and_pinned_sdk(self) -> None:
        workflow = (ROOT / ".github/workflows/vcv.yml").read_text(encoding="utf-8")
        self.assertIn("actions/checkout@v5", workflow)
        self.assertIn("actions/upload-artifact@v7", workflow)
        self.assertIn("RACK_SDK_VERSION: '2.6.6'", workflow)
        self.assertIn("make -C vcv", workflow)
        self.assertIn("tar --zstd -tf", workflow)
        self.assertIn("make -C vcv install", workflow)
        self.assertIn("plugins-lin-x64", workflow)
        self.assertNotRegex(workflow, r"actions/(checkout|upload-artifact|cache)@v[1-4]\b")

    def test_vcv_docs_disclose_single_instance_limit(self) -> None:
        readme = (ROOT / "vcv/README.md").read_text(encoding="utf-8")
        docs = (ROOT / "docs/VCV_RACK.md").read_text(encoding="utf-8")
        self.assertIn("one CLOCK instance", readme)
        self.assertIn("one active CLOCK instance", docs)
        self.assertIn("0/+5 V", readme)
        self.assertIn("Trial First", readme)


if __name__ == "__main__":
    unittest.main()
