#!/usr/bin/env python3
"""Deva's Awesome Adventures - the icon of the PC program (1.1; Windows and macOS 1.2).

  make test                                  (the title screen of the menu playthrough)
  python3 tools/art/make_icon.py             -> packaging/linux/deva-adventures.png (256x256),
                                                then the two below from it
  python3 tools/art/make_icon.py --derived   -> packaging/windows/deva-adventures.ico (16 to 256),
                                                packaging/macos/deva-adventures.icns (16 to 1024),
                                                packaging/windows/installer.bmp (the installer's side)

Deva on her stage, cut from the game's own title screen, by whole pixels, rounded corners. The
icns is the same picture four times bigger, again by whole pixels (the Mac shows it at 512 and up).
"""
import os
import sys

from PIL import Image, ImageDraw

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "art"))
from pngpal import save_png  # noqa: E402

SHOT = os.path.join(ROOT, "build", "test", "menu", "out", "t0_menu_principale.png")
BOX = (130, 164, 190, 224)  # 60x60 around Deva, in the 320x240 picture: x4 = 240
SIZE, RADIUS, BORDER = 256, 44, 8
INSTALLER_BOX = (238, 166, 238 + 164, 166 + 314)  # in the 640x480 picture of the main menu
PINK, PLUM = (240, 98, 160, 255), (74, 38, 96, 255)


def derived():
    png = os.path.join(ROOT, "packaging", "linux", "deva-adventures.png")
    icon = Image.open(png).convert("RGBA")
    ico = os.path.join(ROOT, "packaging", "windows", "deva-adventures.ico")
    icns = os.path.join(ROOT, "packaging", "macos", "deva-adventures.icns")
    for out in (ico, icns):
        os.makedirs(os.path.dirname(out), exist_ok=True)
    icon.save(ico, sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    icon.resize((SIZE * 4, SIZE * 4), Image.NEAREST).save(icns)
    # the side picture of the installer's first and last page (164x314): the menu's card and Deva on
    # her stage, from the manual's picture of the main menu (two screen pixels per game pixel)
    menu = Image.open(os.path.join(ROOT, "docs", "img", "menu.png")).convert("RGB")
    bmp = os.path.join(ROOT, "packaging", "windows", "installer.bmp")
    menu.crop(INSTALLER_BOX).save(bmp)
    for out in (ico, icns, bmp):
        print("%s %d bytes" % (os.path.relpath(out, ROOT), os.path.getsize(out)))


def main():
    if sys.argv[1:] == ["--derived"]:
        derived()
        return
    if not os.path.exists(SHOT):
        sys.exit("missing %s: run make test first" % os.path.relpath(SHOT, ROOT))
    deva = Image.open(SHOT).convert("RGBA").crop(BOX)
    inner = SIZE - 2 * BORDER
    deva = deva.resize((inner, inner), Image.NEAREST)  # 60 -> 240: whole pixels
    icon = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    mask = Image.new("L", (SIZE, SIZE), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, SIZE - 1, SIZE - 1), RADIUS, fill=255)
    frame = Image.new("RGBA", (SIZE, SIZE), PINK)
    ImageDraw.Draw(frame).rounded_rectangle((3, 3, SIZE - 4, SIZE - 4), RADIUS - 3, outline=PLUM, width=3)
    icon.paste(frame, (0, 0), mask)
    inner_mask = Image.new("L", (inner, inner), 0)
    ImageDraw.Draw(inner_mask).rounded_rectangle((0, 0, inner - 1, inner - 1), RADIUS - BORDER, fill=255)
    icon.paste(deva, (BORDER, BORDER), inner_mask)
    out = os.path.join(ROOT, "packaging", "linux", "deva-adventures.png")
    save_png(icon, out)
    print("%s %dx%d, %d bytes" % (os.path.relpath(out, ROOT), SIZE, SIZE, os.path.getsize(out)))
    derived()


if __name__ == "__main__":
    main()
