#!/usr/bin/env python3
"""Rebuild two-across Screensaver and Easter-egg galleries in the CLOCK ODT manual.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import copy
import shutil
import tempfile
import zipfile
from pathlib import Path

from lxml import etree

ROOT = Path(__file__).resolve().parents[1]
MANUAL = ROOT / "docs/manual/clock-user-manual.odt"
ASSETS = ROOT / "docs/manual/assets"
IMAGE_WIDTH_IN = 1.43
IMAGE_HEIGHT_IN = 0.715
TABLE_WIDTH_IN = 3.19

SCREENSAVER_ROWS = (
    (("screensaver-clock.png", "CLOCK - scrolling clock traces"), ("screensaver-plug.png", "PLUG - damped plucked string")),
    (("screensaver-heartbeat.png", "HEARTBEAT - pulsing trace"), ("screensaver-acid.png", "ACID - bouncing smiley")),
    (("screensaver-spectrum.png", "SPECTRUM - synthetic 16-band display"), ("screensaver-field.png", "FIELD - moving contour field")),
    (("screensaver-blox.png", "BLOX - falling procedural bodies"), ("screensaver-matrix.png", "MATRIX - monochrome digital rain")),
    (("screensaver-cube-cover.png", "CUBE COVER - tiled cube sweep"), ("screensaver-fractal.png", "FRACTAL - curated fern crop")),
    (("screensaver-orbit.png", "ORBIT - sparse orbital motion"), ("screensaver-make-music.png", "MAKE MUSIC - progressive text stream")),
    (("screensaver-labyrinth.png", "LABYRINTH - generated maze"), ("screensaver-starfield.png", "STARFIELD - three-depth parallax")),
    (("screensaver-fireworks.png", "FIREWORKS - pixel rocket and burst"), ("power-off.png", "OLED OFF - final protection stage")),
)

EASTER_ROWS = (
    (("pixel-raid.png", "Pixel Raid - move and TAP to fire"), ("formula-1.png", "Formula 1 - steer through the course")),
    (("breakout.png", "Breakout - paddle, ball and modifiers"), ("egg-journey.png", "Egg Journey - jump through incoming hazards")),
    (("beatknecht.png", "BEATKNECHT - default eight-gate rhythm egg"), ("arcade-top-100.png", "Top 100 - ranked arcade leaderboards")),
)

NS = {
    "office": "urn:oasis:names:tc:opendocument:xmlns:office:1.0",
    "style": "urn:oasis:names:tc:opendocument:xmlns:style:1.0",
    "fo": "urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0",
    "text": "urn:oasis:names:tc:opendocument:xmlns:text:1.0",
    "table": "urn:oasis:names:tc:opendocument:xmlns:table:1.0",
    "draw": "urn:oasis:names:tc:opendocument:xmlns:drawing:1.0",
    "xlink": "http://www.w3.org/1999/xlink",
    "svg": "urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0",
    "manifest": "urn:oasis:names:tc:opendocument:xmlns:manifest:1.0",
}


def q(prefix: str, local: str) -> str:
    return f"{{{NS[prefix]}}}{local}"


def direct_text(element: etree._Element) -> str:
    return " ".join("".join(element.itertext()).split())


def ensure_styles(root: etree._Element) -> None:
    automatic = root.xpath("//office:automatic-styles", namespaces=NS)[0]
    existing = {element.get(q("style", "name")) for element in automatic if element.tag == q("style", "style")}

    def add_style(name: str, family: str, child_name: str, attributes: dict[str, str]) -> None:
        if name in existing:
            return
        style = etree.SubElement(automatic, q("style", "style"), {q("style", "name"): name, q("style", "family"): family})
        etree.SubElement(style, q("style", child_name), attributes)

    add_style(
        "GalleryTableStyle",
        "table",
        "table-properties",
        {q("style", "width"): f"{TABLE_WIDTH_IN:.3f}in", q("fo", "margin-left"): "0.0014in", q("table", "align"): "left", q("style", "writing-mode"): "page"},
    )
    table_style = root.xpath('//style:style[@style:name="GalleryTableStyle"]', namespaces=NS)[0]
    table_props = table_style.xpath('./style:table-properties', namespaces=NS)[0]
    table_props.set(q("style", "width"), f"{TABLE_WIDTH_IN:.3f}in")
    table_props.set(q("fo", "margin-left"), "0.0014in")
    table_props.set(q("table", "align"), "left")
    add_style(
        "GalleryColumnStyle",
        "table-column",
        "table-column-properties",
        {q("style", "column-width"): f"{TABLE_WIDTH_IN / 2.0:.3f}in"},
    )
    add_style(
        "GalleryRowStyle",
        "table-row",
        "table-row-properties",
        {q("fo", "keep-together"): "always"},
    )
    add_style(
        "GalleryCellStyle",
        "table-cell",
        "table-cell-properties",
        {
            q("style", "vertical-align"): "top",
            q("fo", "padding-left"): "0.05in",
            q("fo", "padding-right"): "0.05in",
            q("fo", "padding-top"): "0.04in",
            q("fo", "padding-bottom"): "0.07in",
            q("fo", "border"): "none",
        },
    )
    if "GalleryCaptionStyle" not in existing:
        style = etree.SubElement(
            automatic,
            q("style", "style"),
            {q("style", "name"): "GalleryCaptionStyle", q("style", "family"): "paragraph", q("style", "parent-style-name"): "Standard"},
        )
        etree.SubElement(
            style,
            q("style", "paragraph-properties"),
            {q("fo", "text-align"): "center", q("fo", "margin-top"): "0.03in", q("fo", "margin-bottom"): "0in", q("fo", "line-height"): "100%"},
        )
        etree.SubElement(style, q("style", "text-properties"), {q("fo", "font-size"): "8pt"})


def image_paragraph(template: etree._Element, href: str, name: str) -> etree._Element:
    paragraph = copy.deepcopy(template)
    image = paragraph.xpath(".//draw:image", namespaces=NS)[0]
    frame = paragraph.xpath(".//draw:frame", namespaces=NS)[0]
    image.set(q("xlink", "href"), href)
    frame.set(q("draw", "name"), name)
    frame.set(q("svg", "width"), f"{IMAGE_WIDTH_IN:.3f}in")
    frame.set(q("svg", "height"), f"{IMAGE_HEIGHT_IN:.3f}in")
    return paragraph


def gallery_table(
    rows: tuple[tuple[tuple[str, str], tuple[str, str]], ...],
    image_template: etree._Element,
    name: str,
    embedded: dict[str, bytes],
) -> etree._Element:
    table = etree.Element(q("table", "table"), {q("table", "name"): name, q("table", "style-name"): "GalleryTableStyle"})
    etree.SubElement(table, q("table", "table-column"), {q("table", "style-name"): "GalleryColumnStyle", q("table", "number-columns-repeated"): "2"})
    for row_number, row_items in enumerate(rows, 1):
        row = etree.SubElement(table, q("table", "table-row"), {q("table", "style-name"): "GalleryRowStyle"})
        for column_number, (filename, caption) in enumerate(row_items, 1):
            cell = etree.SubElement(
                row,
                q("table", "table-cell"),
                {q("table", "style-name"): "GalleryCellStyle", q("office", "value-type"): "string"},
            )
            archive_name = f"Pictures/gallery_{name.lower()}_{row_number:02d}_{column_number:02d}.png"
            embedded[archive_name] = (ASSETS / filename).read_bytes()
            cell.append(image_paragraph(image_template, archive_name, f"{name}Image{row_number:02d}{column_number:02d}"))
            caption_p = etree.SubElement(cell, q("text", "p"), {q("text", "style-name"): "GalleryCaptionStyle"})
            caption_p.text = caption
    return table


def heading_paragraph(template: etree._Element, text: str) -> etree._Element:
    paragraph = copy.deepcopy(template)
    for child in list(paragraph):
        paragraph.remove(child)
    paragraph.text = text
    return paragraph


def rebuild_odt() -> None:
    # The previous composite-gallery experiment is not part of the source contract.
    for old in ASSETS.glob("gallery-*.png"):
        old.unlink()

    with zipfile.ZipFile(MANUAL, "r") as archive:
        content = etree.fromstring(archive.read("content.xml"))
        manifest = etree.fromstring(archive.read("META-INF/manifest.xml"))
        ensure_styles(content)
        body = content.xpath("//office:body/office:text", namespaces=NS)[0]

        # Remove generated gallery tables/labels from an earlier run.
        for element in list(body):
            if element.tag == q("table", "table") and (element.get(q("table", "name")) or "").startswith("ManualGallery"):
                body.remove(element)
            elif direct_text(element) in ("Screensaver gallery", "Easter-egg gallery"):
                body.remove(element)
            else:
                hrefs = element.xpath(".//draw:image/@xlink:href", namespaces=NS)
                if any(str(href).startswith("Pictures/gallery-") for href in hrefs):
                    body.remove(element)

        image_template = next(
            p for p in body if p.xpath(".//draw:image[@xlink:href='Pictures/manual_rework_017.png']", namespaces=NS)
        )
        screensaver_heading = next(p for p in body if direct_text(p) == "Available animations")
        easter_heading = next(p for p in body if direct_text(p) == "Arcade flow")
        embedded: dict[str, bytes] = {}

        # Insert screensaver gallery after the mode table.
        children = list(body)
        heading_index = next(i for i, p in enumerate(children) if direct_text(p) == "Available animations")
        mode_table = children[heading_index + 1]
        insert_at = body.index(mode_table) + 1
        body.insert(insert_at, heading_paragraph(screensaver_heading, "Screensaver gallery"))
        body.insert(insert_at + 1, gallery_table(SCREENSAVER_ROWS, image_template, "ManualGalleryScreensavers", embedded))

        # Insert Easter-egg gallery directly before the Arcade flow explanation.
        children = list(body)
        arcade_index = next(i for i, p in enumerate(children) if direct_text(p) == "Arcade flow")
        body.insert(arcade_index, heading_paragraph(easter_heading, "Easter-egg gallery"))
        body.insert(arcade_index + 1, gallery_table(EASTER_ROWS, image_template, "ManualGalleryEasterEggs", embedded))

        # Drop stale gallery manifest entries and add the current embedded screenshots.
        path_attr = q("manifest", "full-path")
        for entry in list(manifest):
            full_path = entry.get(path_attr) or ""
            if full_path.startswith("Pictures/gallery-") or full_path.startswith("Pictures/gallery_"):
                manifest.remove(entry)
        for archive_name in sorted(embedded):
            etree.SubElement(
                manifest,
                q("manifest", "file-entry"),
                {path_attr: archive_name, q("manifest", "media-type"): "image/png"},
            )

        replacements = {
            "content.xml": etree.tostring(content, encoding="UTF-8", xml_declaration=True),
            "META-INF/manifest.xml": etree.tostring(manifest, encoding="UTF-8", xml_declaration=True),
            **embedded,
        }
        with tempfile.NamedTemporaryFile(suffix=".odt", delete=False, dir=MANUAL.parent) as temp:
            temp_path = Path(temp.name)
        try:
            with zipfile.ZipFile(temp_path, "w") as output:
                mimetype = archive.getinfo("mimetype")
                output.writestr(mimetype, archive.read("mimetype"), compress_type=zipfile.ZIP_STORED)
                for info in archive.infolist():
                    if info.filename == "mimetype":
                        continue
                    if info.filename.startswith("Pictures/gallery-") or info.filename.startswith("Pictures/gallery_"):
                        continue
                    payload = replacements.get(info.filename, archive.read(info.filename))
                    output.writestr(info, payload)
                for archive_name, payload in embedded.items():
                    output.writestr(archive_name, payload, compress_type=zipfile.ZIP_DEFLATED)
            shutil.move(temp_path, MANUAL)
        finally:
            temp_path.unlink(missing_ok=True)


if __name__ == "__main__":
    rebuild_odt()
    print(MANUAL)
