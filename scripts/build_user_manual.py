#!/usr/bin/env python3
"""Publish a frozen CLOCK user-manual ODT as versioned ODT + PDF artifacts.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import shutil
import subprocess
import tempfile
import re
from pathlib import Path

import check_user_manual

ROOT = Path(__file__).resolve().parents[1]
VERSION_RE = re.compile(r'^#define\s+CLOCK_FIRMWARE_VERSION\s+"([^"]+)"\s*$', re.MULTILINE)

def current_version() -> str:
    text = (ROOT / "src" / "version.h").read_text(encoding="utf-8")
    match = VERSION_RE.search(text)
    if match is None:
        raise RuntimeError("CLOCK_FIRMWARE_VERSION is missing from src/version.h")
    return match.group(1)

def default_source(version: str) -> Path:
    return ROOT / "docs" / "manual" / "releases" / version / f"clock-user-manual.{version}.odt"


def libreoffice_binary() -> str:
    for candidate in ("libreoffice", "soffice"):
        resolved = shutil.which(candidate)
        if resolved:
            return resolved
    raise RuntimeError("LibreOffice/soffice is required to build the manual PDF")


def build(source: Path, output_dir: Path, version: str, *, require_ubuntu_fonts: bool) -> tuple[Path, Path]:
    check_user_manual.validate_odt(source, version)
    output_dir.mkdir(parents=True, exist_ok=True)
    odt_out = output_dir / f"clock-user-manual.{version}.odt"
    pdf_out = output_dir / f"clock-user-manual.{version}.pdf"
    shutil.copy2(source, odt_out)

    with tempfile.TemporaryDirectory(prefix="clock-lo-profile-") as profile:
        command = [
            libreoffice_binary(),
            "--headless",
            f"-env:UserInstallation=file://{Path(profile).resolve()}",
            "--convert-to",
            "pdf",
            "--outdir",
            str(output_dir),
            str(odt_out),
        ]
        subprocess.run(command, check=True)

    if not pdf_out.is_file() or pdf_out.stat().st_size == 0:
        raise RuntimeError(f"LibreOffice did not produce {pdf_out}")
    check_user_manual.validate_pdf(pdf_out, version, require_ubuntu_fonts=require_ubuntu_fonts)
    return odt_out, pdf_out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", default=current_version())
    parser.add_argument("--source", type=Path)
    parser.add_argument("--output-dir", type=Path, default=Path("dist"))
    parser.add_argument("--require-ubuntu-fonts", action="store_true")
    args = parser.parse_args()
    source = args.source or default_source(args.version)
    odt, pdf = build(
        source.resolve(),
        args.output_dir.resolve(),
        args.version,
        require_ubuntu_fonts=args.require_ubuntu_fonts,
    )
    print(odt)
    print(pdf)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
