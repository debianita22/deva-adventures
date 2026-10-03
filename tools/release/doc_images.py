#!/usr/bin/env python3
"""Deva's Awesome Adventures - the screenshots of the manual, from the automated playthroughs.

  make test                                   (writes build/test/<playthrough>/out/*.png)
  python3 tools/release/doc_images.py         -> docs/img/*.png
  python3 tools/release/html2pdf.py docs/manuale.html

Each picture is the game's own 320x240 frame, doubled with nearest-neighbour (crisp pixels in the PDF
too, where viewers smooth what they enlarge) and saved losslessly by tools/art/pngpal.py.
"""
import os
import sys

from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "art"))
from pngpal import save_png  # noqa: E402

SCALE = 2
SHOTS = {  # docs/img/<name>.png <- build/test/<playthrough>/out/<shot>.png
    "menu": "menu/t0_menu_principale",
    "prova": "demo/02_prova_croce",
    "giochi": "demo/02_zmenu",
    "conta": "demo/10_conta_domanda1",
    "aiuto": "demo/27_parole_aiuto",
    "sentiero": "demo/i0_sentiero_domanda1",
    "negozio": "demo/j0_negozio_domanda1",
    "racconto": "demo/r_prologo_1_1",
    "mappa": "demo/m0_mappa_1",
    "sfida": "demo/s1_sfida_ciuffone",
    "premio": "demo/80_premio_scelta",
    "camerino": "menu/c0_camerino",
    "album": "menu/a0_album_storia",
    "grandi": "menu/t3_menu_tieni",
    "opzioni": "menu/o0_opzioni",
    "salvataggi": "menu/s0_salvataggi",
    "pausa": "menu/p0_pausa",
}


def main():
    out_dir = os.path.join(ROOT, "docs", "img")
    os.makedirs(out_dir, exist_ok=True)
    missing = 0
    for name, shot in sorted(SHOTS.items()):
        run, frame = shot.split("/")
        src = os.path.join(ROOT, "build", "test", run, "out", frame + ".png")
        if not os.path.exists(src):
            print("missing %s (run make test first)" % os.path.relpath(src, ROOT))
            missing += 1
            continue
        im = Image.open(src).convert("RGB")
        im = im.resize((im.width * SCALE, im.height * SCALE), Image.NEAREST)
        dst = os.path.join(out_dir, name + ".png")
        save_png(im, dst)
        print("%-12s %-32s %7d bytes" % (name, shot, os.path.getsize(dst)))
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
