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

    def test_wiki_exposes_all_24_manual_aligned_user_chapters(self) -> None:
        chapters = [page for page in build_wiki.PAGES if page.group == 'Using CLOCK']
        self.assertEqual(24, len(chapters))
        self.assertEqual('01 Start here', chapters[0].title)
        self.assertEqual('24 Technical status and specifications', chapters[-1].title)
        self.assertTrue(all(page.source.startswith('docs/user-guide/') for page in chapters))

    def test_wiki_exposes_vcv_overview_and_plugin_developer_guide(self) -> None:
        pages = {page.slug: page for page in build_wiki.PAGES}
        self.assertIn('VCV-Rack-Trial-First', pages)
        self.assertEqual('docs/VCV_RACK.md', pages['VCV-Rack-Trial-First'].source)
        self.assertEqual('Engineering', pages['VCV-Rack-Trial-First'].group)
        self.assertIn('VCV-Plugin-Development', pages)
        self.assertEqual('docs/VCV_DEVELOPMENT.md', pages['VCV-Plugin-Development'].source)
        self.assertEqual('Development', pages['VCV-Plugin-Development'].group)

    def test_sidebar_keeps_manual_aligned_user_chapters_in_order(self) -> None:
        sidebar = build_wiki.render_sidebar(build_wiki.firmware_version())
        self.assertLess(sidebar.index('01 Start here'), sidebar.index('15 Settings map'))
        self.assertLess(sidebar.index('15 Settings map'), sidebar.index('24 Technical status and specifications'))

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
            self.assertIn(f'https://github.com/owner/clock/raw/deadbeef/docs/manual/{expected}', text)

    def test_home_also_offers_current_odt_download(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / 'wiki'
            version = build_wiki.build(output, 'owner/clock', 'deadbeef', 'https://github.com')
            text = (output / 'Home.md').read_text(encoding='utf-8')
            self.assertIn(f'clock-user-manual.{version}.odt', text)

    def test_prerelease_manual_notice_does_not_require_archive_snapshot(self) -> None:
        text = build_wiki.manual_download_notice(
            '1.1.0-beta.1', 'owner/clock', 'deadbeef', 'https://github.com'
        )
        self.assertIn('prerelease development line', text)
        self.assertNotIn('clock-user-manual.1.1.0-beta.1.odt](', text)

    def test_mapped_markdown_links_become_wiki_links(self) -> None:
        rendered = build_wiki.rewrite_links(
            '[Timing](docs/TIMING.md) and [User Guide](docs/user-guide/README.md)',
            Path('README.md'),
            'owner/clock',
            'deadbeef',
            'https://github.com',
        )
        self.assertIn('[Timing](Timing)', rendered)
        self.assertIn('[User Guide](User-Guide)', rendered)

    def test_html_asset_links_are_rewritten(self) -> None:
        rendered = build_wiki.rewrite_links(
            '<img src="docs/manual-source/assets/front-panel-anatomy.svg">',
            Path('README.md'),
            'owner/clock',
            'deadbeef',
            'https://github.com',
        )
        self.assertIn('src="https://github.com/owner/clock/raw/deadbeef/docs/manual-source/assets/front-panel-anatomy.svg"', rendered)

    def test_non_wiki_asset_links_stay_on_exact_repository_commit(self) -> None:
        rendered = build_wiki.rewrite_links(
            '![Panel](docs/manual-source/assets/front-panel-anatomy.svg)',
            Path('README.md'),
            'owner/clock',
            'deadbeef',
            'https://github.com',
        )
        self.assertIn('https://github.com/owner/clock/raw/deadbeef/docs/manual-source/assets/front-panel-anatomy.svg', rendered)


if __name__ == '__main__':
    unittest.main()
