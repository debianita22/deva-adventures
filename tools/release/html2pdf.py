#!/usr/bin/env python3
"""Deva's Awesome Adventures - the PDFs of the documentation, from their HTML.

  python3 tools/release/html2pdf.py docs/manuale.html docs/playtest/scheda_osservazione.html

Each page is printed by Chromium (Playwright) next to its HTML, with the same name and .pdf: A4 or the
@page size of its CSS, backgrounds on, the print media and the light theme. Needs
`pip install playwright` and a Chromium for it (`python3 -m playwright install chromium`, or one
found through PLAYWRIGHT_BROWSERS_PATH).
"""
import os
import re
import sys

from playwright.sync_api import sync_playwright


def pages_of(pdf_path):
    """The page count, from the page tree of the PDF Chromium wrote."""
    with open(pdf_path, "rb") as f:
        data = f.read()
    counts = [int(n) for n in re.findall(rb"/Type\s*/Pages\b[^>]*?/Count\s+(\d+)", data)]
    return max(counts) if counts else 0


def main(argv):
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__.strip())
        return 0 if argv else 2
    with sync_playwright() as p:
        browser = p.chromium.launch()
        page = browser.new_page(color_scheme="light")
        for src in argv:
            src = os.path.abspath(src)
            out = os.path.splitext(src)[0] + ".pdf"
            page.goto("file://" + src, wait_until="load")
            page.emulate_media(media="print", color_scheme="light")
            page.pdf(path=out, format="A4", print_background=True, prefer_css_page_size=True)
            print("%s: %d pages, %d bytes" % (os.path.relpath(out), pages_of(out), os.path.getsize(out)))
        browser.close()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
