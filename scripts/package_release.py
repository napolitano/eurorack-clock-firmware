#!/usr/bin/env python3
"""Build persistence-safe DfuSe firmware images and finalize CLOCK release assets.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import hashlib
import shutil
import struct
import subprocess
import tempfile
import zlib
from pathlib import Path

from upload_preserving_persistence import (
    APP_ADDRESS,
    APP_CAPACITY,
    APP_SECTIONS,
    BOOT_ADDRESS,
    BOOT_CAPACITY,
    BOOT_SECTIONS,
    ensure_region_size,
    objcopy_command,
)

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_USB_VENDOR = 0x0483
DEFAULT_USB_PRODUCT = 0xDF11
DFUSE_VERSION = 1
DFU_SPECIFICATION = 0x011A


def sha256(path: Path) -> str:
    """Return the lowercase SHA-256 digest for one file."""
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def read_version(header: Path) -> str:
    """Read CLOCK_FIRMWARE_VERSION from the canonical firmware header."""
    prefix = '#define CLOCK_FIRMWARE_VERSION "'
    for line in header.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line.startswith(prefix) and line.endswith('"'):
            return line[len(prefix):-1]
    raise SystemExit(f"CLOCK_FIRMWARE_VERSION not found in {header}")


def dfuse_crc(data: bytes) -> int:
    """Return the complemented CRC-32 used by the DfuSe file suffix."""
    return (~zlib.crc32(data)) & 0xFFFFFFFF


def build_dfuse(elements: list[tuple[int, bytes]], *, target_name: str = "CLOCK STM32F401CC") -> bytes:
    """Build one ST DfuSe image containing addressed, non-contiguous Flash elements."""
    if not elements:
        raise ValueError("At least one DfuSe image element is required")
    target_data = bytearray()
    for address, payload in elements:
        if not payload:
            raise ValueError(f"DfuSe element at 0x{address:08X} is empty")
        target_data += struct.pack("<II", address & 0xFFFFFFFF, len(payload))
        target_data += payload

    encoded_name = target_name.encode("ascii", errors="strict")[:255]
    encoded_name = encoded_name + b"\0" * (255 - len(encoded_name))
    target_prefix = struct.pack(
        "<6sBI255sII",
        b"Target",
        0,  # alternate setting 0: internal Flash
        1,  # target name is valid
        encoded_name,
        len(target_data),
        len(elements),
    )
    target = target_prefix + bytes(target_data)
    prefix = struct.pack("<5sBIB", b"DfuSe", DFUSE_VERSION, len(target) + 11, 1)
    body = prefix + target
    suffix_without_crc = struct.pack(
        "<HHHH3sB",
        0xFFFF,
        DEFAULT_USB_PRODUCT,
        DEFAULT_USB_VENDOR,
        DFU_SPECIFICATION,
        b"UFD",
        16,
    )
    crc = dfuse_crc(body + suffix_without_crc)
    return body + suffix_without_crc + struct.pack("<I", crc)


def parse_dfuse(data: bytes) -> list[tuple[int, bytes]]:
    """Validate a DfuSe file and return its addressed image elements."""
    if len(data) < 11 + 274 + 16:
        raise ValueError("DfuSe image is too short")
    if dfuse_crc(data[:-4]) != struct.unpack_from("<I", data, len(data) - 4)[0]:
        raise ValueError("DfuSe CRC mismatch")
    signature, version, image_size, target_count = struct.unpack_from("<5sBIB", data, 0)
    if signature != b"DfuSe" or version != DFUSE_VERSION or target_count != 1:
        raise ValueError("Unexpected DfuSe prefix")
    if image_size != len(data) - 16:
        raise ValueError("DfuSe image-size field does not match the payload")

    offset = 11
    target_signature, alt, named, _name, target_size, element_count = struct.unpack_from(
        "<6sBI255sII", data, offset)
    if target_signature != b"Target" or alt != 0 or named not in (0, 1):
        raise ValueError("Unexpected DfuSe target prefix")
    offset += 274
    target_end = offset + target_size
    elements: list[tuple[int, bytes]] = []
    for _ in range(element_count):
        address, size = struct.unpack_from("<II", data, offset)
        offset += 8
        payload = data[offset:offset + size]
        if len(payload) != size:
            raise ValueError("Truncated DfuSe image element")
        elements.append((address, payload))
        offset += size
    if offset != target_end or target_end != len(data) - 16:
        raise ValueError("DfuSe target-size field does not match its elements")
    return elements


def find_objcopy(explicit: str | None) -> str:
    """Resolve the ARM objcopy executable from an explicit path, PATH, or PlatformIO packages."""
    if explicit:
        candidate = Path(explicit).expanduser()
        if candidate.is_file():
            return str(candidate)
        resolved = shutil.which(explicit)
        if resolved:
            return resolved
        raise FileNotFoundError(f"objcopy not found: {explicit}")

    resolved = shutil.which("arm-none-eabi-objcopy")
    if resolved:
        return resolved
    package_root = Path.home() / ".platformio" / "packages"
    if package_root.is_dir():
        candidates = sorted(package_root.glob("**/arm-none-eabi-objcopy"))
        candidates += sorted(package_root.glob("**/arm-none-eabi-objcopy.exe"))
        if candidates:
            return str(candidates[0])
    raise FileNotFoundError("arm-none-eabi-objcopy is not available")


def extract_regions(build_dir: Path, objcopy: str, temp_dir: Path) -> tuple[Path, Path]:
    """Extract the linker-owned boot and application regions from the sparse ELF."""
    elf = build_dir / "firmware.elf"
    if not elf.is_file():
        raise FileNotFoundError(f"Build artifact missing: {elf}")
    boot = temp_dir / "boot.bin"
    app = temp_dir / "app.bin"
    subprocess.run(objcopy_command(objcopy, elf, boot, BOOT_SECTIONS), check=True)
    subprocess.run(objcopy_command(objcopy, elf, app, APP_SECTIONS), check=True)
    ensure_region_size(boot, BOOT_CAPACITY, "boot")
    ensure_region_size(app, APP_CAPACITY, "application")
    return boot, app


def firmware_filename(version: str, variant: str | None) -> str:
    """Return the public firmware-image filename for one Easter-egg build."""
    stem = f"clock-v{version}-stm32f401cc"
    if variant:
        stem += f"-{variant}"
    return f"{stem}.dfu"


def package_firmware(args: argparse.Namespace) -> int:
    """Create one persistence-safe firmware image for one PlatformIO environment."""
    build_dir = Path(args.build_dir).resolve()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    version = read_version(Path(args.version_file).resolve())
    output = out_dir / firmware_filename(version, args.variant)

    if args.boot_bin or args.app_bin:
        if not args.boot_bin or not args.app_bin:
            raise SystemExit("--boot-bin and --app-bin must be supplied together")
        boot = Path(args.boot_bin).resolve()
        app = Path(args.app_bin).resolve()
        if not boot.is_file() or not app.is_file():
            raise SystemExit("Supplied split firmware image is missing")
        ensure_region_size(boot, BOOT_CAPACITY, "boot")
        ensure_region_size(app, APP_CAPACITY, "application")
        image = build_dfuse([(BOOT_ADDRESS, boot.read_bytes()), (APP_ADDRESS, app.read_bytes())])
    else:
        try:
            objcopy = find_objcopy(args.objcopy)
            with tempfile.TemporaryDirectory(prefix="clock-release-") as tmp:
                boot, app = extract_regions(build_dir, objcopy, Path(tmp))
                image = build_dfuse([(BOOT_ADDRESS, boot.read_bytes()), (APP_ADDRESS, app.read_bytes())])
        except (FileNotFoundError, RuntimeError, subprocess.CalledProcessError) as exc:
            raise SystemExit(str(exc)) from exc

    output.write_bytes(image)
    elements = parse_dfuse(output.read_bytes())
    if [address for address, _ in elements] != [BOOT_ADDRESS, APP_ADDRESS]:
        raise SystemExit("Packaged DfuSe image does not preserve the expected Flash regions")
    print(output)
    return 0


def copy_required(source: Path, destination: Path, label: str) -> None:
    """Copy one required release artifact or fail with a precise diagnostic."""
    if not source.is_file():
        raise SystemExit(f"{label} missing: {source}")
    shutil.copy2(source, destination)


def finalize_release(args: argparse.Namespace) -> int:
    """Copy documentation artifacts and generate checksums for the complete release set."""
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    copy_required(Path(args.manual_odt).resolve(), out_dir / Path(args.manual_odt).name, "Manual ODT")
    copy_required(Path(args.manual_pdf).resolve(), out_dir / Path(args.manual_pdf).name, "Manual PDF")
    copy_required(Path(args.changelog).resolve(), out_dir / "CHANGELOG.md", "Changelog")
    copy_required(Path(args.summary).resolve(), out_dir / "RELEASE_SUMMARY.md", "Release summary")

    assets = sorted(
        path for path in out_dir.rglob("*")
        if path.is_file() and path.name != "SHA256SUMS.txt"
    )
    if not any(path.suffix == ".dfu" for path in assets):
        raise SystemExit("No firmware .dfu images were packaged")
    checksum_path = out_dir / "SHA256SUMS.txt"
    checksum_path.write_text(
        "".join(
            f"{sha256(path)}  {path.relative_to(out_dir).as_posix()}\n"
            for path in assets
        ),
        encoding="utf-8",
    )
    print(checksum_path)
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    firmware = subparsers.add_parser("firmware", help="Package one PlatformIO build as sparse DfuSe")
    firmware.add_argument("--build-dir", default=".pio/build/release_default")
    firmware.add_argument("--out-dir", default="dist")
    firmware.add_argument("--version-file", default="src/version.h")
    firmware.add_argument("--variant", default="")
    firmware.add_argument("--objcopy")
    firmware.add_argument("--boot-bin")
    firmware.add_argument("--app-bin")
    firmware.set_defaults(func=package_firmware)

    finalize = subparsers.add_parser("finalize", help="Finalize documentation assets and checksums")
    finalize.add_argument("--out-dir", default="dist")
    finalize.add_argument("--manual-odt", required=True)
    finalize.add_argument("--manual-pdf", required=True)
    finalize.add_argument("--summary", required=True)
    finalize.add_argument("--changelog", default="CHANGELOG.md")
    finalize.set_defaults(func=finalize_release)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
