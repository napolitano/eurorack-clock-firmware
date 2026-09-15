#!/usr/bin/env python3
"""Regression tests for deterministic GitHub Wiki generation.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""

from __future__ import annotations

import importlib.util
import tempfile
import unittest
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / 'scripts/build_wiki.py'
spec = importlib.util.spec_from_file_location('build_wiki', SCRIPT)
assert spec is not None and spec.loader is not None
build_wiki = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = build_wiki
spec.loader.exec_module(build_wiki)


class WikiGenerationTests(unittest.TestCase):
    """Keep the Wiki page contract, navigation, links, and manual download stable."""

    def test_build_generates_complete_curated_page_set(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / 'wiki'
            version = build_wiki.build(output, 'owner/clock', 'deadbeef', 'https://github.com')
            expected = {f'{page.slug}.md' for page in build_wiki.PAGES} | {'_Sidebar.md'}
            self.assertEqual(expected, {path.name for path in output.glob('*.md')})
            self.assertIn(version, (output / '_Sidebar.md').read_text(encoding='utf-8'))

    def test_every_content_page_has_exactly_one_munich_footer(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / 'wiki'
            build_wiki.build(output, 'owner/clock', 'deadbeef', 'https://github.com')
            for page in build_wiki.PAGES:
                text = (output / f'{page.slug}.md').read_text(encoding='utf-8')
                self.assertEqual(1, text.count(build_wiki.CANONICAL_FOOTER), page.slug)
                self.assertTrue(text.rstrip().endswith(build_wiki.CANONICAL_FOOTER), page.slug)

    def test_manual_page_links_exact_version_frozen_odt(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / 'wiki'
            version = build_wiki.build(output, 'owner/clock', 'deadbeef', 'https://github.com')
            text = (output / 'Manual-and-Downloads.md').read_text(encoding='utf-8')
            expected = f'clock-user-manual.{version}.odt'
            self.assertIn(expected, text)
            self.assertIn(f'https://github.com/owner/clock/raw/deadbeef/docs/manual/releases/{version}/{expected}', text)

    def test_home_also_offers_current_odt_download(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / 'wiki'
            version = build_wiki.build(output, 'owner/clock', 'deadbeef', 'https://github.com')
            text = (output / 'Home.md').read_text(encoding='utf-8')
            self.assertIn(f'clock-user-manual.{version}.odt', text)

    def test_mapped_markdown_links_become_wiki_links(self) -> None:
        rendered = build_wiki.rewrite_links(
            '[Timing](docs/TIMING.md) and [User Guide](docs/USER_GUIDE.md#controls)',
            Path('README.md'),
            'owner/clock',
            'deadbeef',
            'https://github.com',
        )
        self.assertIn('[Timing](Timing)', rendered)
        self.assertIn('[User Guide](User-Guide#controls)', rendered)

    def test_html_asset_links_are_rewritten(self) -> None:
        rendered = build_wiki.rewrite_links(
            '<img src="docs/manual/assets/front-panel-anatomy.svg">',
            Path('README.md'),
            'owner/clock',
            'deadbeef',
            'https://github.com',
        )
        self.assertIn('src="https://github.com/owner/clock/raw/deadbeef/docs/manual/assets/front-panel-anatomy.svg"', rendered)

    def test_non_wiki_asset_links_stay_on_exact_repository_commit(self) -> None:
        rendered = build_wiki.rewrite_links(
            '![Panel](docs/manual/assets/front-panel-anatomy.svg)',
            Path('README.md'),
            'owner/clock',
            'deadbeef',
            'https://github.com',
        )
        self.assertIn('https://github.com/owner/clock/raw/deadbeef/docs/manual/assets/front-panel-anatomy.svg', rendered)


if __name__ == '__main__':
    unittest.main()
