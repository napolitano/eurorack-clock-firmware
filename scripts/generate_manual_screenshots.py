#!/usr/bin/env python3
"""Build the headless simulator renderer and regenerate manual OLED screenshots.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""

from __future__ import annotations

import argparse
import binascii
import struct
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "docs" / "manual" / "assets"
BUILD_DIR = ROOT / "build" / "simulator-headless"
GENERATOR_BASENAME = "clock-manual-screenshot-generator"
MANIFEST_NAME = "manual-screenshots.tsv"


def run(command: list[str]) -> None:
    """Execute one subprocess and fail immediately on errors."""
    print("+", " ".join(command))
    subprocess.run(command, cwd=ROOT, check=True)


def generator_path() -> Path:
    """Return the configured generator executable path for this host platform."""
    suffix = ".exe" if sys.platform.startswith("win") else ""
    return BUILD_DIR / f"{GENERATOR_BASENAME}{suffix}"


def read_pgm(path: Path) -> tuple[int, int, bytes]:
    """Read the generator's binary P5 grayscale image without third-party packages."""
    data = path.read_bytes()
    offset = 0

    def token() -> bytes:
        nonlocal offset
        while offset < len(data):
            if data[offset:offset + 1] == b"#":
                newline = data.find(b"\n", offset)
                if newline < 0:
                    raise ValueError(f"Malformed PGM comment in {path}")
                offset = newline + 1
                continue
            if not data[offset:offset + 1].isspace():
                break
            offset += 1
        start = offset
        while offset < len(data) and not data[offset:offset + 1].isspace():
            offset += 1
        return data[start:offset]

    if token() != b"P5":
        raise ValueError(f"Unsupported PGM format in {path}")
    width = int(token())
    height = int(token())
    maximum = int(token())
    if maximum != 255:
        raise ValueError(f"Unsupported PGM range in {path}: {maximum}")
    while offset < len(data) and data[offset:offset + 1].isspace():
        offset += 1
    pixels = data[offset:]
    expected = width * height
    if len(pixels) != expected:
        raise ValueError(f"PGM pixel length mismatch in {path}: {len(pixels)} != {expected}")
    return width, height, pixels


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    """Encode one PNG chunk with its CRC."""
    checksum = binascii.crc32(kind)
    checksum = binascii.crc32(payload, checksum) & 0xFFFFFFFF
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", checksum)


def write_scaled_png(path: Path, width: int, height: int, pixels: bytes, scale: int) -> None:
    """Write a grayscale PNG using integer nearest-neighbour scaling only."""
    scaled_width = width * scale
    scaled_height = height * scale
    raw = bytearray()
    for source_y in range(height):
        row = pixels[source_y * width:(source_y + 1) * width]
        scaled_row = b"".join(bytes((value,)) * scale for value in row)
        for _ in range(scale):
            raw.append(0)  # PNG filter type: None
            raw.extend(scaled_row)

    ihdr = struct.pack(">IIBBBBB", scaled_width, scaled_height, 8, 0, 0, 0, 0)
    encoded = (
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", ihdr)
        + png_chunk(b"IDAT", zlib.compress(bytes(raw), level=9))
        + png_chunk(b"IEND", b"")
    )
    path.write_bytes(encoded)


def parse_manifest(path: Path) -> list[tuple[str, str]]:
    """Read the ordered screenshot catalog emitted by the native generator."""
    entries: list[tuple[str, str]] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        name, description = line.split("\t", 1)
        entries.append((name, description))
    return entries


def remove_previous_outputs(output_dir: Path) -> None:
    """Delete only files listed in the previous generated manifest."""
    previous_manifest = output_dir / MANIFEST_NAME
    if not previous_manifest.exists():
        return
    for name, _ in parse_manifest(previous_manifest):
        candidate = output_dir / f"{name}.png"
        if candidate.exists():
            candidate.unlink()


def generate(output_dir: Path, scale: int) -> int:
    """Build the generator, render the catalog, and publish deterministic PNG files."""
    if scale < 1:
        raise ValueError("scale must be at least 1")

    run(["cmake", "--preset", "simulator-headless"])
    run(["cmake", "--build", "--preset", "simulator-headless", "--target", GENERATOR_BASENAME])

    executable = generator_path()
    if not executable.exists():
        raise FileNotFoundError(f"Screenshot generator not found: {executable}")

    output_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="clock-manual-screenshots-") as temporary:
        temporary_dir = Path(temporary)
        run([str(executable), str(temporary_dir)])
        entries = parse_manifest(temporary_dir / "manifest.tsv")
        remove_previous_outputs(output_dir)

        for name, _ in entries:
            pgm_path = temporary_dir / f"{name}.pgm"
            width, height, pixels = read_pgm(pgm_path)
            if (width, height) != (128, 64):
                raise ValueError(f"Unexpected OLED dimensions for {name}: {width}x{height}")
            write_scaled_png(output_dir / f"{name}.png", width, height, pixels, scale)

        manifest_text = "".join(f"{name}\t{description}\n" for name, description in entries)
        (output_dir / MANIFEST_NAME).write_text(manifest_text, encoding="utf-8", newline="\n")

    try:
        shown_output = output_dir.relative_to(ROOT)
    except ValueError:
        shown_output = output_dir
    print(f"Generated {len(entries)} screenshots in {shown_output} at {scale}x nearest-neighbour scale.")
    return len(entries)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help="Destination directory (default: docs/manual/assets)",
    )
    parser.add_argument(
        "--scale",
        type=int,
        default=4,
        help="Integer nearest-neighbour enlargement factor (default: 4)",
    )
    args = parser.parse_args()
    generate(args.output.resolve(), args.scale)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
