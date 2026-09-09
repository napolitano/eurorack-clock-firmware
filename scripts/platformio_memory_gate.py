"""PlatformIO post-link hook enforcing the project Flash/RAM headroom policy.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""

from __future__ import annotations

import sys
from pathlib import Path

Import("env")  # type: ignore[name-defined]  # Provided by PlatformIO/SCons.

if not env.IsIntegrationDump():  # type: ignore[name-defined]
    scripts_directory = Path(env.subst("$PROJECT_DIR")) / "scripts"  # type: ignore[name-defined]
    sys.path.insert(0, str(scripts_directory))

    from check_memory_budget import inspect_elf

    def check_memory(target, source, env) -> None:  # noqa: ANN001
        """Inspect the linked ELF and fail the build when the 90% policy is exceeded."""
        del source
        elf_path = Path(str(target[0]))
        board = env.BoardConfig()
        flash_capacity = int(board.get("upload.maximum_size"))
        ram_capacity = int(board.get("upload.maximum_ram_size"))
        maximum_percent = float(env.GetProjectOption("custom_memory_budget_percent", "90"))
        size_tool = env.subst("$SIZETOOL")
        inspect_elf(size_tool, elf_path, flash_capacity, ram_capacity, maximum_percent)

    env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", check_memory)  # type: ignore[name-defined]
