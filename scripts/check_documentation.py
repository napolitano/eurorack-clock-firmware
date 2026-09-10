#!/usr/bin/env python3
"""Validate CLOCK repository documentation without network access.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""

from __future__ import annotations

import json
import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[1]
README_FOOTER = '<h6 align="center">From Munich with &#9829;</h6>'
MARKDOWN_LINK_RE = re.compile(r'!?(?:\[[^\]]*\])\(([^)]+)\)')
HTML_LINK_RE = re.compile(r'(?:href|src)="([^"]+)"')
VERSION_RE = re.compile(r'^#define\s+CLOCK_FIRMWARE_VERSION\s+"([^"]+)"\s*$', re.MULTILINE)

CANONICAL_PROJECT_NAME = 'South Signal Lab CLOCK'
CANONICAL_AUTHOR = 'Axel Napolitano'
CANONICAL_PUBLISHER = 'South Signal Lab'
CANONICAL_LICENSE = 'PolyForm-Noncommercial-1.0.0'
CODEMETA_CONTEXT = 'https://w3id.org/codemeta/3.0'
UPDATE_URL = 'https://github.com/napolitano'



def markdown_files() -> list[Path]:
    """Return project-owned Markdown files that participate in documentation validation."""
    roots = [ROOT, ROOT / 'docs', ROOT / 'test', ROOT / '.github']
    files: set[Path] = set()
    for root in roots:
        if not root.exists():
            continue
        if root == ROOT:
            files.update(root.glob('*.md'))
        else:
            files.update(root.rglob('*.md'))
    return sorted(files)


def local_link_target(source: Path, raw_target: str) -> Path | None:
    """Resolve one local Markdown/HTML link target, ignoring URLs and fragment-only links."""
    target = raw_target.strip()
    if not target or target.startswith(('#', 'http://', 'https://', 'mailto:', 'data:')):
        return None
    target = target.split('#', 1)[0].split('?', 1)[0]
    if not target:
        return None
    return (source.parent / unquote(target)).resolve()


def check_links(errors: list[str]) -> None:
    """Require every repository-local Markdown and HTML link target to exist."""
    for path in markdown_files():
        text = path.read_text(encoding='utf-8')
        targets = MARKDOWN_LINK_RE.findall(text) + HTML_LINK_RE.findall(text)
        for raw_target in targets:
            target = local_link_target(path, raw_target)
            if target is None:
                continue
            try:
                target.relative_to(ROOT.resolve())
            except ValueError:
                errors.append(f'{path.relative_to(ROOT)}: link escapes repository: {raw_target}')
                continue
            if not target.exists():
                errors.append(f'{path.relative_to(ROOT)}: missing local link target: {raw_target}')


def check_fences(errors: list[str]) -> None:
    """Require balanced triple-backtick code fences in every Markdown document."""
    for path in markdown_files():
        lines = path.read_text(encoding='utf-8').splitlines()
        fence_lines = [line for line in lines if line.lstrip().startswith('```')]
        if len(fence_lines) % 2 != 0:
            errors.append(f'{path.relative_to(ROOT)}: unbalanced triple-backtick code fence')


def check_readme_footers(errors: list[str]) -> None:
    """Require the canonical centered text-heart footer on every README-style document."""
    for path in sorted(ROOT.rglob('*.md')):
        if 'readme' not in path.name.lower():
            continue
        text = path.read_text(encoding='utf-8').rstrip()
        if not text.endswith(README_FOOTER):
            errors.append(f'{path.relative_to(ROOT)}: missing canonical README footer')


def check_manual_svgs(errors: list[str]) -> None:
    """Require manual SVG assets to be valid, scalable, and accessibility-labelled."""
    assets = ROOT / 'docs' / 'manual' / 'assets'
    if not assets.is_dir():
        errors.append('docs/manual/assets: directory missing')
        return
    svg_paths = sorted(assets.glob('*.svg'))
    if not svg_paths:
        errors.append('docs/manual/assets: no SVG illustrations found')
        return
    namespace = {'svg': 'http://www.w3.org/2000/svg'}
    for path in svg_paths:
        try:
            root = ET.parse(path).getroot()
        except ET.ParseError as exc:
            errors.append(f'{path.relative_to(ROOT)}: invalid SVG/XML: {exc}')
            continue
        if 'viewBox' not in root.attrib:
            errors.append(f'{path.relative_to(ROOT)}: SVG requires viewBox')
        if root.attrib.get('role') != 'img':
            errors.append(f'{path.relative_to(ROOT)}: SVG requires role="img"')
        title = root.find('svg:title', namespace)
        desc = root.find('svg:desc', namespace)
        if title is None or not ''.join(title.itertext()).strip():
            errors.append(f'{path.relative_to(ROOT)}: SVG requires a non-empty <title>')
        if desc is None or not ''.join(desc.itertext()).strip():
            errors.append(f'{path.relative_to(ROOT)}: SVG requires a non-empty <desc>')



def check_generated_front_panel(errors: list[str]) -> None:
    """Keep the manual front-panel illustration synchronized with simulator geometry."""
    command = [sys.executable, str(ROOT / 'scripts' / 'generate_front_panel_illustration.py'), '--check']
    result = subprocess.run(command, text=True, capture_output=True)
    if result.returncode != 0:
        detail = (result.stderr or result.stdout).strip()
        errors.append(f'docs/manual/assets/front-panel-anatomy.svg: generated illustration is stale ({detail})')


def check_version_identity(errors: list[str]) -> None:
    """Keep current project identity/version synchronized across primary documentation metadata."""
    version_header = (ROOT / 'src' / 'version.h').read_text(encoding='utf-8')
    match = VERSION_RE.search(version_header)
    if match is None:
        errors.append('src/version.h: firmware version define missing')
        return
    version = match.group(1)
    for path in (ROOT / 'README.md', ROOT / 'docs/USER_GUIDE.md', ROOT / 'docs/TEST_COVERAGE.md'):
        text = path.read_text(encoding='utf-8')
        if version not in text:
            errors.append(f'{path.relative_to(ROOT)}: current firmware version {version} is not documented')

    readme = (ROOT / 'README.md').read_text(encoding='utf-8')
    expected_ci_badge = 'actions/workflows/ci.yml/badge.svg'
    if expected_ci_badge not in readme:
        errors.append('README.md: CI badge must use the live GitHub Actions workflow status')

    if 'PROJECT_NAME           = "South Signal Lab CLOCK Firmware"' not in (ROOT / 'Doxyfile').read_text(encoding='utf-8'):
        errors.append('Doxyfile: PROJECT_NAME must use canonical South Signal Lab CLOCK identity')


def check_v1_scope_contract(errors: list[str]) -> None:
    """Keep the deliberate no-general-CV V1 boundary and migration audit visible."""
    required = {
        ROOT / 'README.md': ('Why no general CV modulation?', 'no general parameter-CV inputs'),
        ROOT / 'docs/USER_GUIDE.md': ('Product boundary: timing rather than analog CV', 'not a missing V1 feature'),
        ROOT / 'docs/CONFIGURATION.md': ('Analog/CV product boundary', '16-bit DAC-class'),
        ROOT / 'docs/ROADMAP.md': ('V1 feature freeze', 'digital timing, gate, and trigger'),
        ROOT / 'docs/V1_FORWARD_COMPATIBILITY.md': ('2,504 bytes', '63 bytes', 'schema v7'),
    }
    for path, needles in required.items():
        if not path.is_file():
            errors.append(f'{path.relative_to(ROOT)}: required V1 scope document is missing')
            continue
        text = path.read_text(encoding='utf-8')
        for needle in needles:
            if needle not in text:
                errors.append(f'{path.relative_to(ROOT)}: missing V1 scope contract text {needle!r}')


def check_project_metadata(errors: list[str]) -> None:
    """Require citation/discovery metadata to match the canonical project identity and version."""
    version_header = (ROOT / 'src' / 'version.h').read_text(encoding='utf-8')
    match = VERSION_RE.search(version_header)
    if match is None:
        return
    version = match.group(1)

    citation_path = ROOT / 'CITATION.cff'
    if not citation_path.is_file():
        errors.append('CITATION.cff: required project citation metadata is missing')
    else:
        citation = citation_path.read_text(encoding='utf-8')
        required_lines = {
            'cff-version: 1.2.0': 'Citation File Format must be 1.2.0',
            f'title: "{CANONICAL_PROJECT_NAME}"': 'canonical project title mismatch',
            f'version: {version}': f'citation version must be {version}',
            f'license: {CANONICAL_LICENSE}': 'SPDX license identifier mismatch',
            'given-names: Axel': 'author given name missing',
            'family-names: Napolitano': 'author family name missing',
        }
        for needle, message in required_lines.items():
            if needle not in citation:
                errors.append(f'CITATION.cff: {message}')
        if re.search(r'^doi\s*:', citation, re.MULTILINE):
            doi_match = re.search(r'^doi\s*:\s*(\S+)', citation, re.MULTILINE)
            doi_value = '' if doi_match is None else doi_match.group(1).strip('\"\'')
            if not doi_value.startswith('10.'):
                errors.append('CITATION.cff: DOI must be a real DOI value when present')

    codemeta_path = ROOT / 'codemeta.json'
    if not codemeta_path.is_file():
        errors.append('codemeta.json: required software discovery metadata is missing')
    else:
        try:
            metadata = json.loads(codemeta_path.read_text(encoding='utf-8'))
        except json.JSONDecodeError as exc:
            errors.append(f'codemeta.json: invalid JSON: {exc}')
            metadata = {}
        if metadata.get('@context') != CODEMETA_CONTEXT:
            errors.append('codemeta.json: CodeMeta 3.0 context mismatch')
        if metadata.get('@type') != 'SoftwareSourceCode':
            errors.append('codemeta.json: @type must be SoftwareSourceCode')
        if metadata.get('name') != CANONICAL_PROJECT_NAME:
            errors.append('codemeta.json: canonical project name mismatch')
        if metadata.get('softwareVersion') != version:
            errors.append(f'codemeta.json: softwareVersion must be {version}')
        if metadata.get('author', {}).get('name') != CANONICAL_AUTHOR:
            errors.append('codemeta.json: canonical author mismatch')
        if metadata.get('publisher', {}).get('name') != CANONICAL_PUBLISHER:
            errors.append('codemeta.json: publisher must be South Signal Lab')
        expected_license = f'https://spdx.org/licenses/{CANONICAL_LICENSE}.html'
        if metadata.get('license') != expected_license:
            errors.append('codemeta.json: SPDX license URL mismatch')

    update_source = (ROOT / 'src' / 'ui' / 'settings_renderer.cpp').read_text(encoding='utf-8')
    if f'QR payload for exactly: {UPDATE_URL}' not in update_source:
        errors.append('settings_renderer.cpp: update QR canonical URL comment mismatch')
    user_guide = (ROOT / 'docs' / 'USER_GUIDE.md').read_text(encoding='utf-8')
    if UPDATE_URL not in user_guide:
        errors.append('docs/USER_GUIDE.md: canonical update URL mismatch')

    readme = (ROOT / 'README.md').read_text(encoding='utf-8')
    for required in (CANONICAL_PROJECT_NAME, '(CITATION.cff)', '(codemeta.json)'):
        if required not in readme:
            errors.append(f'README.md: missing project identity metadata reference {required!r}')


def main() -> int:
    """Run all documentation quality checks and return a shell-friendly status code."""
    errors: list[str] = []
    check_links(errors)
    check_fences(errors)
    check_readme_footers(errors)
    check_manual_svgs(errors)
    check_generated_front_panel(errors)
    check_version_identity(errors)
    check_v1_scope_contract(errors)
    check_project_metadata(errors)
    if errors:
        print('Documentation check failed:', file=sys.stderr)
        for error in errors:
            print(f'  - {error}', file=sys.stderr)
        return 1
    print('Documentation check passed.')
    print(f'  Markdown files checked: {len(markdown_files())}')
    print(f'  Manual SVGs checked: {len(list((ROOT / "docs/manual/assets").glob("*.svg")))}')
    print('  README footer, local links, fences, SVG accessibility, version identity, and citation metadata: PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
