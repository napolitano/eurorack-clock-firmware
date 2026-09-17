#!/usr/bin/env python3
"""Build persistence-safe DfuSe firmware images and finalize CLOCK release assets.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import hashlib
import os
import re
import shutil
import struct
import subprocess
import tempfile
import zipfile
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
TOOLCHAIN_VERSION = "7.2.1"
TOOLCHAIN_PACKAGE = "toolchain-gccarmnoneeabi"
REPOSITORY_URL = "https://github.com/napolitano/eurorack-clock-firmware"
FIRMWARE_FLAVORS = (
    ("default", "BEATKNECHT"),
    ("pixel-raid", "Pixel Raid"),
    ("formula-1", "Formula 1"),
    ("breakout", "Breakout"),
    ("egg-journey", "Egg Journey"),
)
STATIC_LICENSE_FILES = (
    "LICENSE-Apache-2.0.txt",
    "LICENSE-BSD-3-Clause.txt",
    "LICENSE-SDL-zlib.txt",
)


def sha256(path: Path) -> str:
    """Return the lowercase SHA-256 digest for one file."""
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def md5(path: Path) -> str:
    """Return the lowercase MD5 digest for one release-integrity manifest."""
    h = hashlib.md5(usedforsecurity=False)
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
        0,
        1,
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
    flavor = (variant or "default").strip() or "default"
    known = {name for name, _ in FIRMWARE_FLAVORS}
    if flavor not in known:
        raise ValueError(f"Unknown firmware flavor {flavor!r}; expected one of {sorted(known)}")
    return f"eurorack-clock-firmware-{flavor}-{version}.dfu"


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


def resolve_toolchain_dir(explicit: str | None) -> Path:
    if explicit:
        path = Path(explicit).expanduser().resolve()
    else:
        path = (Path.home() / ".platformio" / "packages" / TOOLCHAIN_PACKAGE).resolve()
    if not path.is_dir():
        raise SystemExit(f"Pinned GNU Arm toolchain package missing: {path}")
    return path


def gcc_executable(toolchain_dir: Path) -> Path:
    candidates = (
        toolchain_dir / "bin" / "arm-none-eabi-gcc",
        toolchain_dir / "bin" / "arm-none-eabi-gcc.exe",
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise SystemExit(f"arm-none-eabi-gcc missing from pinned toolchain: {toolchain_dir}")


def toolchain_version_text(toolchain_dir: Path) -> str:
    result = subprocess.run(
        [str(gcc_executable(toolchain_dir)), "--version"],
        text=True,
        capture_output=True,
        check=True,
    )
    first = result.stdout.splitlines()[0] if result.stdout else result.stderr.splitlines()[0]
    if TOOLCHAIN_VERSION not in first:
        raise SystemExit(
            f"Expected GNU Arm Embedded Toolchain {TOOLCHAIN_VERSION}; got {first!r}"
        )
    return first.strip()


def is_license_candidate(path: Path) -> bool:
    name = path.name.lower()
    if re.match(r"^(copying|license|copyright)([._-].*)?$", name):
        return True
    parts = [part.lower() for part in path.parts]
    return name in {"license.txt", "license.html", "license.rst"} or (
        "share" in parts and "doc" in parts and "license" in name
    )


def collect_toolchain_licenses(args: argparse.Namespace) -> int:
    """Archive license material from the exact pinned ARM toolchain package used by CI."""
    toolchain_dir = resolve_toolchain_dir(args.toolchain_dir)
    version_line = toolchain_version_text(toolchain_dir)
    candidates = sorted(
        path for path in toolchain_dir.rglob("*")
        if path.is_file() and is_license_candidate(path)
    )
    if not candidates:
        raise SystemExit(f"No license material found in pinned toolchain: {toolchain_dir}")

    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    output = out_dir / f"GNU-ARM-EMBEDDED-{TOOLCHAIN_VERSION}-LICENSES.zip"
    manifest = [
        f"GNU Arm Embedded Toolchain: {version_line}",
        f"Source package: {toolchain_dir}",
        "",
        "Files captured from the exact installed PlatformIO toolchain package:",
    ]
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for source in candidates:
            relative = source.relative_to(toolchain_dir).as_posix()
            archive.write(source, f"toolchain/{relative}")
            manifest.append(f"{sha256(source)}  toolchain/{relative}")
        archive.writestr("MANIFEST.txt", "\n".join(manifest) + "\n")
    print(output)
    return 0


def parse_map_argument(value: str) -> tuple[str, Path]:
    if "=" not in value:
        raise argparse.ArgumentTypeError("--map expects FLAVOR=PATH")
    flavor, raw_path = value.split("=", 1)
    flavor = flavor.strip()
    known = {name for name, _ in FIRMWARE_FLAVORS}
    if flavor not in known:
        raise argparse.ArgumentTypeError(f"Unknown firmware flavor: {flavor}")
    return flavor, Path(raw_path).expanduser()


def linked_archives(map_text: str) -> list[str]:
    """Extract unique static-library paths referenced by one GNU ld map."""
    matches = re.findall(r"([^\s()]+\.a)(?:\([^\n)]*\))?", map_text)
    return sorted(dict.fromkeys(matches))


def build_info(args: argparse.Namespace) -> int:
    """Generate release build provenance from linker maps and the pinned ARM toolchain."""
    version = read_version(Path(args.version_file).resolve())
    toolchain_dir = resolve_toolchain_dir(args.toolchain_dir)
    compiler = toolchain_version_text(toolchain_dir)
    supplied = dict(args.maps)
    expected = {name for name, _ in FIRMWARE_FLAVORS}
    if set(supplied) != expected:
        missing = sorted(expected - set(supplied))
        extra = sorted(set(supplied) - expected)
        raise SystemExit(f"Linker-map contract mismatch; missing={missing}, extra={extra}")

    revision = args.source_revision or os.environ.get("GITHUB_SHA") or "local/unknown"
    lines = [
        "CLOCK firmware build provenance",
        f"Firmware version: {version}",
        f"Source revision: {revision}",
        f"Repository: {REPOSITORY_URL}",
        f"Compiler: {compiler}",
        "",
        "Each flavor below was built independently. The linked archive list is derived",
        "from that build's GNU ld map and is intended for release-license auditing.",
        "",
    ]
    for flavor, title in FIRMWARE_FLAVORS:
        path = supplied[flavor].resolve()
        if not path.is_file():
            raise SystemExit(f"Linker map missing for {flavor}: {path}")
        text = path.read_text(encoding="utf-8", errors="replace")
        archives = linked_archives(text)
        lines.extend([
            f"[{flavor}] {title}",
            f"Map SHA-256: {sha256(path)}",
            "Linked static archives:",
        ])
        if archives:
            lines.extend(f"  - {archive}" for archive in archives)
        else:
            lines.append("  - none reported by map parser")
        lines.append("")

    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    output = out_dir / "BUILD-INFO.txt"
    output.write_text("\n".join(lines), encoding="utf-8")
    print(output)
    return 0


def copy_required(source: Path, destination: Path, label: str) -> None:
    """Copy one required release artifact or fail with a precise diagnostic."""
    if not source.is_file():
        raise SystemExit(f"{label} missing: {source}")
    if source.resolve() != destination.resolve():
        shutil.copy2(source, destination)


def required_firmware_paths(out_dir: Path, version: str) -> list[Path]:
    return [out_dir / firmware_filename(version, flavor) for flavor, _ in FIRMWARE_FLAVORS]


def finalize_release(args: argparse.Namespace) -> int:
    """Enforce the complete release contract and generate integrity manifests."""
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    version = read_version(Path(args.version_file).resolve())

    missing_firmware = [path.name for path in required_firmware_paths(out_dir, version) if not path.is_file()]
    if missing_firmware:
        raise SystemExit(f"Required firmware images missing: {', '.join(missing_firmware)}")

    manual_odt = Path(args.manual_odt).resolve()
    manual_pdf = Path(args.manual_pdf).resolve()
    copy_required(manual_odt, out_dir / manual_odt.name, "Manual ODT")
    copy_required(manual_pdf, out_dir / manual_pdf.name, "Manual PDF")
    copy_required(Path(args.changelog).resolve(), out_dir / "CHANGELOG.md", "Changelog")
    copy_required(Path(args.summary).resolve(), out_dir / "RELEASE_SUMMARY.md", "Release summary")
    copy_required(Path(args.release_notes).resolve(), out_dir / "RELEASE_NOTES.md", "Release notes")

    copy_required(ROOT / "LICENSE.md", out_dir / "LICENSE.md", "Firmware license")
    copy_required(ROOT / "NOTICE.txt", out_dir / "NOTICE.txt", "Required notice")
    copy_required(ROOT / "THIRD_PARTY_NOTICES.md", out_dir / "THIRD_PARTY_NOTICES.md", "Third-party notices")
    copy_required(ROOT / "docs" / "manual" / "LICENSE.md", out_dir / "MANUAL-LICENSE.md", "Manual license")
    for filename in STATIC_LICENSE_FILES:
        copy_required(ROOT / "third_party" / filename, out_dir / filename, filename)

    toolchain_license = out_dir / f"GNU-ARM-EMBEDDED-{TOOLCHAIN_VERSION}-LICENSES.zip"
    if not toolchain_license.is_file():
        raise SystemExit(f"Pinned toolchain license archive missing: {toolchain_license}")
    build_info_path = out_dir / "BUILD-INFO.txt"
    if not build_info_path.is_file():
        raise SystemExit(f"Build provenance missing: {build_info_path}")

    checksum_names = {"SHA256SUMS.txt", "MD5SUMS.txt"}
    assets = sorted(
        path for path in out_dir.rglob("*")
        if path.is_file() and path.name not in checksum_names
    )
    (out_dir / "SHA256SUMS.txt").write_text(
        "".join(f"{sha256(path)}  {path.relative_to(out_dir).as_posix()}\n" for path in assets),
        encoding="utf-8",
    )
    (out_dir / "MD5SUMS.txt").write_text(
        "".join(f"{md5(path)}  {path.relative_to(out_dir).as_posix()}\n" for path in assets),
        encoding="utf-8",
    )
    print(out_dir / "SHA256SUMS.txt")
    print(out_dir / "MD5SUMS.txt")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    firmware = subparsers.add_parser("firmware", help="Package one PlatformIO build as sparse DfuSe")
    firmware.add_argument("--build-dir", default=".pio/build/release_default")
    firmware.add_argument("--out-dir", default="dist")
    firmware.add_argument("--version-file", default="src/version.h")
    firmware.add_argument("--variant", default="default")
    firmware.add_argument("--objcopy")
    firmware.add_argument("--boot-bin")
    firmware.add_argument("--app-bin")
    firmware.set_defaults(func=package_firmware)

    licenses = subparsers.add_parser(
        "toolchain-licenses",
        help="Capture license material from the exact pinned GNU Arm toolchain package",
    )
    licenses.add_argument("--out-dir", default="dist")
    licenses.add_argument("--toolchain-dir")
    licenses.set_defaults(func=collect_toolchain_licenses)

    info = subparsers.add_parser("build-info", help="Generate build provenance from GNU ld maps")
    info.add_argument("--out-dir", default="dist")
    info.add_argument("--version-file", default="src/version.h")
    info.add_argument("--toolchain-dir")
    info.add_argument("--source-revision")
    info.add_argument("--map", dest="maps", action="append", type=parse_map_argument, default=[])
    info.set_defaults(func=build_info)

    finalize = subparsers.add_parser("finalize", help="Finalize release assets and checksums")
    finalize.add_argument("--out-dir", default="dist")
    finalize.add_argument("--version-file", default="src/version.h")
    finalize.add_argument("--manual-odt", required=True)
    finalize.add_argument("--manual-pdf", required=True)
    finalize.add_argument("--summary", required=True)
    finalize.add_argument("--release-notes", required=True)
    finalize.add_argument("--changelog", default="CHANGELOG.md")
    finalize.set_defaults(func=finalize_release)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
