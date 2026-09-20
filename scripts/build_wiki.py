#!/usr/bin/env python3
"""Build the generated GitHub Wiki tree from canonical repository Markdown.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from urllib.parse import quote

ROOT = Path(__file__).resolve().parents[1]
STABLE_VERSION_RE = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+$")
VERSION_RE = re.compile(r'#define\s+CLOCK_FIRMWARE_VERSION\s+"([^"]+)"')
LINK_RE = re.compile(r'(!?)\[([^\]]+)\]\(([^)]+)\)')
TARGET_RE = re.compile(r'\]\(([^)]+)\)')
HTML_ATTR_RE = re.compile(r'(?P<prefix>\b(?:href|src)=")(?P<target>[^"]+)(?P<suffix>")')
FOOTER_RE = re.compile(
    r'\n*<h6\s+align="center">From Munich with (?:&#9829;|♥)</h6>\s*$',
    re.IGNORECASE,
)
CANONICAL_FOOTER = '<h6 align="center">From Munich with &#9829;</h6>'


@dataclass(frozen=True)
class WikiPage:
    """Describe one generated Wiki page and its canonical repository source."""

    source: str
    slug: str
    title: str
    group: str


PAGES = (
    WikiPage('README.md', 'Home', 'Home', 'Start here'),
    WikiPage('docs/USER_GUIDE.md', 'User-Guide', 'User Guide', 'Start here'),
    WikiPage('docs/manual-source/README.md', 'Manual-and-Downloads', 'Manual & Downloads', 'Start here'),
    WikiPage('docs/ROADMAP.md', 'Roadmap', 'Roadmap', 'Start here'),
    WikiPage('docs/ARCHITECTURE.md', 'Architecture', 'Architecture', 'Engineering'),
    WikiPage('docs/TIMING.md', 'Timing', 'Timing', 'Engineering'),
    WikiPage('docs/CONFIGURATION.md', 'Configuration', 'Configuration', 'Engineering'),
    WikiPage('docs/SIMULATOR.md', 'Simulator', 'Simulator', 'Engineering'),
    WikiPage('test/README.md', 'Native-Tests', 'Native Tests', 'Quality'),
    WikiPage('docs/TEST_COVERAGE.md', 'Test-Coverage', 'Test Coverage', 'Quality'),
    WikiPage('docs/HIL_TEST_PLAN.md', 'HIL-Qualification', 'HIL Qualification', 'Quality'),
    WikiPage('docs/V1_FORWARD_COMPATIBILITY.md', 'V1-Forward-Compatibility', 'V1 Forward Compatibility', 'Quality'),
    WikiPage('docs/DEVELOPER_README.md', 'Developer-Setup', 'Developer Setup', 'Development'),
    WikiPage('docs/DEVELOPMENT.md', 'Development-Workflow', 'Development Workflow', 'Development'),
    WikiPage('docs/DEPENDENCIES.md', 'Dependencies', 'Dependencies', 'Development'),
    WikiPage('docs/README.md', 'Documentation-Index', 'Documentation Index', 'Development'),
    WikiPage('docs/WIKI.md', 'Wiki-Publication', 'Wiki Publication', 'Development'),
    WikiPage('docs/LICENSING.md', 'Licensing', 'Licensing', 'Project'),
    WikiPage('docs/PROJECT_IDENTITY.md', 'Project-Identity', 'Project Identity', 'Project'),
    WikiPage('CONTRIBUTING.md', 'Contributing', 'Contributing', 'Project'),
    WikiPage('.github/SECURITY.md', 'Security', 'Security', 'Project'),
)

PAGE_BY_SOURCE = {PurePosixPath(page.source): page for page in PAGES}
WIKI_SLUGS = {page.slug for page in PAGES}


def firmware_version() -> str:
    """Read the firmware version from the repository's single source of truth."""
    text = (ROOT / 'src/version.h').read_text(encoding='utf-8')
    match = VERSION_RE.search(text)
    if match is None:
        raise SystemExit('CLOCK_FIRMWARE_VERSION not found in src/version.h')
    return match.group(1)


def repository_url(server_url: str, repository: str, ref: str, path: PurePosixPath, raw: bool = False) -> str:
    """Return a stable source-repository URL for an unmapped file or asset."""
    mode = 'raw' if raw else 'blob'
    safe_path = '/'.join(quote(part) for part in path.parts)
    return f'{server_url.rstrip("/")}/{repository}/{mode}/{ref}/{safe_path}'


def strip_repository_footer(text: str) -> str:
    """Remove an existing repository footer so the Wiki adds exactly one footer."""
    return FOOTER_RE.sub('', text.rstrip())


def resolve_target(source: PurePosixPath, target: str) -> tuple[PurePosixPath, str]:
    """Resolve a relative Markdown target against its source document."""
    path_part, separator, anchor = target.partition('#')
    if not path_part:
        return source, f'#{anchor}' if separator else ''
    combined = source.parent / path_part
    normalized: list[str] = []
    for part in combined.parts:
        if part in ('', '.'):
            continue
        if part == '..':
            if normalized:
                normalized.pop()
            continue
        normalized.append(part)
    return PurePosixPath(*normalized), f'#{anchor}' if separator else ''


def rewrite_links(text: str, source: PurePosixPath, repository: str, ref: str, server_url: str) -> str:
    """Rewrite repository-relative links so they remain useful in the separate Wiki repository."""
    output: list[str] = []
    fenced = False

    def rewrite_target(target: str, image: bool = False) -> str:
        clean_target = target.strip()
        slug, _, _anchor = clean_target.partition('#')
        if slug in WIKI_SLUGS:
            return target
        if clean_target.startswith(('http://', 'https://', 'mailto:', '#', 'file-upload://')):
            return target
        if clean_target.startswith('<') and clean_target.endswith('>'):
            clean_target = clean_target[1:-1]
        resolved, anchor = resolve_target(source, clean_target)
        mapped = PAGE_BY_SOURCE.get(resolved)
        if mapped is not None:
            return f'{mapped.slug}{anchor}'
        local_path = ROOT / Path(*resolved.parts)
        raw = image or local_path.suffix.lower() in {
            '.png', '.jpg', '.jpeg', '.gif', '.svg', '.webp', '.odt', '.pdf'
        }
        return f'{repository_url(server_url, repository, ref, resolved, raw=raw)}{anchor}'

    def replace_markdown(match: re.Match[str]) -> str:
        image_marker, label, target = match.groups()
        return f'{image_marker}[{label}]({rewrite_target(target, image=bool(image_marker))})'

    def replace_target(match: re.Match[str]) -> str:
        return f']({rewrite_target(match.group(1))})'

    def replace_html(match: re.Match[str]) -> str:
        target = match.group('target')
        image = match.group('prefix').startswith('src=')
        return f"{match.group('prefix')}{rewrite_target(target, image=image)}{match.group('suffix')}"

    for line in text.splitlines(keepends=True):
        if line.lstrip().startswith('```'):
            fenced = not fenced
            output.append(line)
            continue
        if fenced:
            output.append(line)
            continue
        rewritten = LINK_RE.sub(replace_markdown, line)
        # A second target pass catches outer links in nested badge syntax such as
        # [![badge](absolute)](relative.md), which a simple Markdown regex cannot
        # consume correctly in one match. Already absolute targets are unchanged.
        rewritten = TARGET_RE.sub(replace_target, rewritten)
        rewritten = HTML_ATTR_RE.sub(replace_html, rewritten)
        output.append(rewritten)
    return ''.join(output)


def generated_notice(page: WikiPage, repository: str, ref: str, server_url: str) -> str:
    """Build the source-of-truth notice displayed at the top of each generated page."""
    source_url = repository_url(server_url, repository, ref, PurePosixPath(page.source))
    return (
        '> [!NOTE]\n'
        f'> This Wiki page is generated from [{page.source}]({source_url}). '
        'Edit the repository source rather than the Wiki directly.\n\n'
    )


def manual_download_notice(version: str, repository: str, ref: str, server_url: str) -> str:
    """Build the prominent current ODT-manual download block for the Wiki manual page."""
    relative = PurePosixPath(
        f'docs/manual/clock-user-manual.{version}.odt'
    )
    source = ROOT / Path(*relative.parts)
    if not source.is_file():
        if STABLE_VERSION_RE.fullmatch(version):
            raise SystemExit(
                f'Frozen stable-release Wiki manual source missing: {relative}. '
                'Run scripts/prepare_release_manual.py before publishing the Wiki.'
            )
        return (
            '> [!NOTE]\n'
            f'> CLOCK {version} is a prerelease development line. '
            'Prerelease manuals are generated as publication artifacts and are not kept in '
            '`docs/manual/`; use the repository User Guide for the live development state.\n\n'
        )
    url = repository_url(server_url, repository, ref, relative, raw=True)
    return (
        '> [!TIP]\n'
        f'> **Download the current CLOCK {version} ODT manual:** '
        f'[clock-user-manual.{version}.odt]({url})\n\n'
    )


def render_page(page: WikiPage, version: str, repository: str, ref: str, server_url: str) -> str:
    """Render one canonical repository document into its Wiki representation."""
    source = ROOT / page.source
    if not source.is_file():
        raise SystemExit(f'Wiki source missing: {page.source}')
    body = strip_repository_footer(source.read_text(encoding='utf-8'))
    body = rewrite_links(body, PurePosixPath(page.source), repository, ref, server_url)
    prefix = generated_notice(page, repository, ref, server_url)
    if page.slug in {'Home', 'Manual-and-Downloads'}:
        prefix += manual_download_notice(version, repository, ref, server_url)
    return f'{prefix}{body.rstrip()}\n\n{CANONICAL_FOOTER}\n'


def render_sidebar(version: str) -> str:
    """Render stable navigation for the generated Wiki."""
    lines = [
        '<!-- Generated by scripts/build_wiki.py; do not edit in the Wiki. -->',
        '',
        f'**CLOCK {version}**',
        '',
    ]
    group_order = ('Start here', 'Engineering', 'Quality', 'Development', 'Project')
    for group in group_order:
        lines.extend((f'### {group}', ''))
        for page in PAGES:
            if page.group == group:
                lines.append(f'- [{page.title}]({page.slug})')
        lines.append('')
    return '\n'.join(lines).rstrip() + '\n'


def validate_output(output: Path, version: str) -> None:
    """Reject incomplete or structurally inconsistent generated Wiki trees."""
    expected = {f'{page.slug}.md' for page in PAGES} | {'_Sidebar.md'}
    actual = {path.name for path in output.glob('*.md')}
    missing = sorted(expected - actual)
    unexpected = sorted(actual - expected)
    if missing:
        raise SystemExit(f'Generated Wiki missing pages: {", ".join(missing)}')
    if unexpected:
        raise SystemExit(f'Generated Wiki contains unexpected pages: {", ".join(unexpected)}')
    for page in PAGES:
        text = (output / f'{page.slug}.md').read_text(encoding='utf-8')
        if not text.rstrip().endswith(CANONICAL_FOOTER):
            raise SystemExit(f'{page.slug}.md: canonical Munich footer is missing')
        if 'generated from' not in text[:1200]:
            raise SystemExit(f'{page.slug}.md: generated-source notice is missing')
    manual = (output / 'Manual-and-Downloads.md').read_text(encoding='utf-8')
    if STABLE_VERSION_RE.fullmatch(version):
        if f'clock-user-manual.{version}.odt' not in manual or '/raw/' not in manual:
            raise SystemExit('Manual-and-Downloads.md: current stable ODT download link is missing')
    elif 'prerelease development line' not in manual:
        raise SystemExit('Manual-and-Downloads.md: prerelease manual policy notice is missing')
    sidebar = (output / '_Sidebar.md').read_text(encoding='utf-8')
    if f'CLOCK {version}' not in sidebar:
        raise SystemExit('_Sidebar.md: current firmware version is missing')


def build(output: Path, repository: str, ref: str, server_url: str) -> str:
    """Generate the complete Wiki tree and return the firmware version used."""
    version = firmware_version()
    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)
    for page in PAGES:
        (output / f'{page.slug}.md').write_text(
            render_page(page, version, repository, ref, server_url),
            encoding='utf-8',
            newline='\n',
        )
    (output / '_Sidebar.md').write_text(render_sidebar(version), encoding='utf-8', newline='\n')
    validate_output(output, version)
    return version


def main() -> int:
    """CLI entry point for local previews and GitHub Actions publication."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', default='build/wiki')
    parser.add_argument('--repository', default=os.environ.get('GITHUB_REPOSITORY', 'napolitano/eurorack-clock-firmware'))
    parser.add_argument('--ref', default=os.environ.get('GITHUB_SHA', 'main'))
    parser.add_argument('--server-url', default=os.environ.get('GITHUB_SERVER_URL', 'https://github.com'))
    args = parser.parse_args()

    output = Path(args.output)
    if not output.is_absolute():
        output = ROOT / output
    version = build(output, args.repository, args.ref, args.server_url)
    print(f'Generated {len(PAGES)} Wiki pages for CLOCK {version}: {output}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
