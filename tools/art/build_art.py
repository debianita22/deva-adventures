#!/usr/bin/env python3
"""Build all graphics for Deva's Awesome Adventures.

  build_art.py draw   -> regenerate editable sprite PNGs in art/png/ (from code)
  build_art.py pack   -> pack art/png/*.png into data/deva_adventures/gfx/atlas.{png,txt}
  build_art.py        -> both, plus backgrounds

art/png/ is the editable source: tweak a PNG (Aseprite, LibreSprite...) and run
`build_art.py pack`. Offsets live in art/png/offsets.txt.
atlas.txt format:  sprite <name> <x> <y> <w> <h> <ox> <oy>
                   anchor <name> <x> <y>
"""
import os
import sys

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
PNG_DIR = os.path.join(ROOT, 'art', 'png')
GFX_DIR = os.path.join(ROOT, 'data', 'deva_adventures', 'gfx')

from pngpal import save_png  # noqa: E402  (1.0: lossless indexed PNGs, a third of the bytes)
import hero as M            # noqa: E402
import objects as O         # noqa: E402
import ui as U              # noqa: E402
import backgrounds as B     # noqa: E402
import stories as ST        # noqa: E402
import tale as TL           # noqa: E402
import tale_bg as TB        # noqa: E402
import menus as MN          # noqa: E402
import font as FN           # noqa: E402
import games3 as G3         # noqa: E402
import tale2 as T2          # noqa: E402
import tale2_bg as TB2      # noqa: E402
import sparkle as SP        # noqa: E402
import tale3 as T3          # noqa: E402
import tale3_bg as TB3      # noqa: E402
import games4 as G4         # noqa: E402
import tale4 as T4          # noqa: E402
import tale4_bg as TB4      # noqa: E402
import games5 as G5         # noqa: E402
import games6 as G6         # noqa: E402
import blink as BL          # noqa: E402
from pixel import save      # noqa: E402


def draw():
    os.makedirs(PNG_DIR, exist_ok=True)
    offsets = {}
    for name, (img, off) in M.parts().items():
        save(img, os.path.join(PNG_DIR, name + '.png'))
        offsets[name] = off
    small, _, words_small = O.render_all(1.0)   # 24 px: dense grids, sequence rows
    big, icons, words = O.render_all(1.5)        # 36 px: scattered objects, cards
    for name, img in small.items():
        save(img, os.path.join(PNG_DIR, 'obj_' + name + '.png'))
    for name, img in big.items():
        save(img, os.path.join(PNG_DIR, 'objb_' + name + '.png'))
    for name, img in icons.items():
        save(img, os.path.join(PNG_DIR, 'rw_' + name + '.png'))
    # word pictures for "Parole" (36 px) and sequence items (24 px)
    alias = {'stella': 'stelle', 'cuore': 'cuori', 'farfalla': 'farfalle', 'fiore': 'fiori',
             'diamante': 'diamanti', 'scarpetta': 'scarpette', 'rossetto': 'rossetti',
             'fiocco': 'fiocchi', 'microfono': 'microfoni', 'smalto': 'smalti'}
    for word, obj in alias.items():
        save(big[obj], os.path.join(PNG_DIR, 'pic_' + word + '.png'))
        save(small[obj], os.path.join(PNG_DIR, 'pics_' + word + '.png'))
    save(icons['coroncina'], os.path.join(PNG_DIR, 'pic_corona.png'))
    save(icons['guance'], os.path.join(PNG_DIR, 'pic_pennello.png'))
    for word, img in words.items():
        save(img, os.path.join(PNG_DIR, 'pic_' + word + '.png'))
        save(words_small[word], os.path.join(PNG_DIR, 'pics_' + word + '.png'))
    for name, img in U.ui_sprites().items():
        save(img, os.path.join(PNG_DIR, name + '.png'))
    for name, img in ST.all_sprites().items():
        save(img, os.path.join(PNG_DIR, name + '.png'))
    for name, img in TL.all_sprites().items():
        save(img, os.path.join(PNG_DIR, name + '.png'))
    for name, img in MN.all_sprites().items():
        save(img, os.path.join(PNG_DIR, name + '.png'))
    for name, img in FN.all_sprites().items():     # the small font of the grown-up screens
        save(img, os.path.join(PNG_DIR, name + '.png'))
    for name, img in G3.all_sprites().items():     # gym, make-up salon, shapes (0.6)
        save(img, os.path.join(PNG_DIR, name + '.png'))
    for name, img in T2.all_sprites().items():     # the second tale (0.7)
        save(img, os.path.join(PNG_DIR, name + '.png'))
    for name, img in SP.all_sprites().items():     # twinkles, hearts, puffs... (0.9)
        save(img, os.path.join(PNG_DIR, name + '.png'))
    for name, img in T3.all_sprites().items():     # the third tale: monsters, the ogre, the notes (0.10)
        save(img, os.path.join(PNG_DIR, name + '.png'))
    for name, img in G4.all_sprites().items():     # letters, shadows, the stones path (0.10)
        save(img, os.path.join(PNG_DIR, name + '.png'))
    for name, img in T4.all_sprites().items():     # the fourth tale: toy monsters, the king, the keys (0.11)
        save(img, os.path.join(PNG_DIR, name + '.png'))
    for name, img in G5.all_sprites().items():     # the shop, the measures (0.11)
        save(img, os.path.join(PNG_DIR, name + '.png'))
    for name, img in G6.all_sprites().items():     # the parrot, the toy train, the tiles of the sign (0.12)
        save(img, os.path.join(PNG_DIR, name + '.png'))
    for name, img in BL.all_sprites().items():     # the characters with their eyes closed: they blink (0.13)
        save(img, os.path.join(PNG_DIR, name + '.png'))
    mo = M.mirror_offsets()
    with open(os.path.join(PNG_DIR, 'offsets.txt'), 'w') as f:
        f.write('# sprite offsets (ox oy) on the 44x68 hero canvas; anchors\n')
        for name in sorted(offsets):
            f.write('offset %s %d %d\n' % (name, offsets[name][0], offsets[name][1]))
        for name, (x, y) in sorted(mo.items()):
            f.write('anchor %s %d %d\n' % (name, x, y))
        f.write('anchor feet 22 %d\n' % M.FEET_Y)
        x0, y0, x1, y1 = B.PANEL
        f.write('anchor panel0 %d %d\n' % (x0, y0))
        f.write('anchor panel1 %d %d\n' % (x1, y1))
        for name, (x, y) in sorted(TB.anchors().items()):   # the map of the tale
            f.write('anchor %s %d %d\n' % (name, x, y))
        for name, (x, y) in sorted(TB2.anchors().items()):  # the night map of the second tale
            f.write('anchor %s %d %d\n' % (name, x, y))
        for name, (x, y) in sorted(TB3.anchors().items()):  # the valley of music, third tale
            f.write('anchor %s %d %d\n' % (name, x, y))
        for name, (x, y) in sorted(TB4.anchors().items()):  # the kingdom of toys, fourth tale
            f.write('anchor %s %d %d\n' % (name, x, y))
    print('drew sprites into', PNG_DIR)


def pack(width=512):
    offsets, anchors = {}, []
    with open(os.path.join(PNG_DIR, 'offsets.txt')) as f:
        for line in f:
            t = line.split()
            if not t or t[0].startswith('#'):
                continue
            if t[0] == 'offset':
                offsets[t[1]] = (int(t[2]), int(t[3]))
            elif t[0] == 'anchor':
                anchors.append((t[1], int(t[2]), int(t[3])))
    items = []
    for fn in sorted(os.listdir(PNG_DIR)):
        if fn.endswith('.png'):
            img = np.array(Image.open(os.path.join(PNG_DIR, fn)).convert('RGBA'))
            img[..., 3] = np.where(img[..., 3] >= 128, 255, 0)   # 1-bit alpha
            items.append((fn[:-4], img))
    # shelf packing, tallest first, 1 px gutter
    items.sort(key=lambda t: (-t[1].shape[0], -t[1].shape[1], t[0]))
    x = y = shelf_h = 0
    placed = []
    for name, img in items:
        h, w = img.shape[:2]
        if x + w > width:
            x, y, shelf_h = 0, y + shelf_h + 1, 0
        placed.append((name, x, y, img))
        x += w + 1
        shelf_h = max(shelf_h, h)
    height = y + shelf_h
    sheet = np.zeros((height, width, 4), dtype=np.uint8)
    for name, px, py, img in placed:
        h, w = img.shape[:2]
        sheet[py:py + h, px:px + w] = img
    os.makedirs(GFX_DIR, exist_ok=True)
    save_png(Image.fromarray(sheet, 'RGBA'), os.path.join(GFX_DIR, 'atlas.png'))
    with open(os.path.join(GFX_DIR, 'atlas.txt'), 'w') as f:
        f.write('# generated by tools/art/build_art.py - do not edit, edit art/png instead\n')
        for name, px, py, img in sorted(placed):
            h, w = img.shape[:2]
            ox, oy = offsets.get(name, (0, 0))
            f.write('sprite %s %d %d %d %d %d %d\n' % (name, px, py, w, h, ox, oy))
        for name, ax, ay in anchors:
            f.write('anchor %s %d %d\n' % (name, ax, ay))
    print('atlas %dx%d, %d sprites' % (width, height, len(placed)))


def backgrounds():
    os.makedirs(GFX_DIR, exist_ok=True)
    for name, img in (list(B.all_backgrounds().items()) + list(TB.all_backgrounds().items()) +
                      list(G3.all_backgrounds().items()) + list(TB2.all_backgrounds().items()) +
                      list(TB3.all_backgrounds().items()) + list(G4.all_backgrounds().items()) +
                      list(TB4.all_backgrounds().items()) + list(G5.all_backgrounds().items()) +
                      list(G6.all_backgrounds().items())):
        save_png(Image.fromarray(img[..., :3], 'RGB'), os.path.join(GFX_DIR, name + '.png'))
    save_png(Image.fromarray(B.curtain_panel()[..., :3], 'RGB'), os.path.join(GFX_DIR, 'sipario.png'))
    print('backgrounds written to', GFX_DIR)


if __name__ == '__main__':
    cmd = sys.argv[1] if len(sys.argv) > 1 else 'all'
    if cmd in ('draw', 'all'):
        draw()
    if cmd in ('pack', 'all'):
        pack()
    if cmd in ('bg', 'all'):
        backgrounds()
