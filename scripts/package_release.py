#!/usr/bin/env python3
"""Create deterministic release filenames and SHA-256 checksums from a PlatformIO build.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import hashlib
import shutil
from pathlib import Path


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def read_version(header: Path) -> str:
    prefix = '#define CLOCK_FIRMWARE_VERSION "'
    for line in header.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line.startswith(prefix) and line.endswith('"'):
            return line[len(prefix):-1]
    raise SystemExit(f"CLOCK_FIRMWARE_VERSION not found in {header}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default=".pio/build/blackpill_f401cc")
    parser.add_argument("--out-dir", default="dist")
    parser.add_argument("--version-file", default="src/version.h")
    parser.add_argument("--license-file", default="LICENSE.md")
    parser.add_argument("--notice-file", default="NOTICE.txt")
    parser.add_argument("--third-party-notice-file", default="THIRD_PARTY_NOTICES.md")
    parser.add_argument("--third-party-dir", default="third_party")
    parser.add_argument("--citation-file", default="CITATION.cff")
    parser.add_argument("--codemeta-file", default="codemeta.json")
    parser.add_argument(
        "--acknowledge-lgpl-static-link",
        action="store_true",
        help=(
            "Explicitly acknowledge that publishing the generated static BIN/ELF "
            "requires a separately reviewed LGPL compliance package."
        ),
    )
    args = parser.parse_args()

    if not args.acknowledge_lgpl_static_link:
        raise SystemExit(
            "Binary packaging is intentionally disabled while the target statically "
            "links STM32duino LGPL components. Use --acknowledge-lgpl-static-link "
            "only for a compliance-reviewed distribution."
        )

    build_dir = Path(args.build_dir)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    version = read_version(Path(args.version_file))
    stem = f"clock-v{version}-stm32f401cc"

    expected = {
        "firmware.bin": out_dir / f"{stem}.bin",
        "firmware.elf": out_dir / f"{stem}.elf",
    }

    produced: list[Path] = []
    for source_name, destination in expected.items():
        source = build_dir / source_name
        if not source.is_file():
            raise SystemExit(f"Build artifact missing: {source}")
        shutil.copy2(source, destination)
        produced.append(destination)

    license_source = Path(args.license_file)
    if not license_source.is_file():
        raise SystemExit(f"License file missing: {license_source}")
    license_destination = out_dir / "LICENSE.md"
    shutil.copy2(license_source, license_destination)
    produced.append(license_destination)

    notice_source = Path(args.notice_file)
    if not notice_source.is_file():
        raise SystemExit(f"Required notice file missing: {notice_source}")
    notice_destination = out_dir / "NOTICE.txt"
    shutil.copy2(notice_source, notice_destination)
    produced.append(notice_destination)

    third_party_notice_source = Path(args.third_party_notice_file)
    if not third_party_notice_source.is_file():
        raise SystemExit(f"Third-party notice file missing: {third_party_notice_source}")
    third_party_notice_destination = out_dir / "THIRD_PARTY_NOTICES.md"
    shutil.copy2(third_party_notice_source, third_party_notice_destination)
    produced.append(third_party_notice_destination)

    metadata_sources = [
        (Path(args.citation_file), out_dir / "CITATION.cff", "Citation metadata"),
        (Path(args.codemeta_file), out_dir / "codemeta.json", "CodeMeta metadata"),
    ]
    for source, destination, label in metadata_sources:
        if not source.is_file():
            raise SystemExit(f"{label} file missing: {source}")
        shutil.copy2(source, destination)
        produced.append(destination)

    third_party_dir = Path(args.third_party_dir)
    if not third_party_dir.is_dir():
        raise SystemExit(f"Third-party license directory missing: {third_party_dir}")
    third_party_output = out_dir / "third_party"
    third_party_output.mkdir(parents=True, exist_ok=True)
    license_files = sorted(path for path in third_party_dir.iterdir() if path.is_file())
    if not license_files:
        raise SystemExit(f"No third-party license files found in: {third_party_dir}")
    for source in license_files:
        destination = third_party_output / source.name
        shutil.copy2(source, destination)
        produced.append(destination)

    checksum_path = out_dir / "SHA256SUMS.txt"
    checksum_path.write_text(
        "".join(
            f"{sha256(path)}  {path.relative_to(out_dir).as_posix()}\n"
            for path in produced
        ),
        encoding="utf-8",
    )
    print("\n".join(str(p) for p in [*produced, checksum_path]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
