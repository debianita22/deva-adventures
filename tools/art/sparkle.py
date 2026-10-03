"""Sprites of the sparkling bits (0.9.0): twinkles in four sizes, hearts, petals,
bubbles, snowflakes, glows for fireflies and embers, the little cloud puff of a
wrong answer. They are drawn by fx.c (particles), amb.c (the ambient layers of
every place) and trans.c (the edges of the transitions).
"""
import numpy as np

from pixel import ascii_sprite, blank, ellipse_mask, dilate4, rgb

TWINKLE = [
    [".w.",
     "wyw",
     ".w."],
    ["..w..",
     "..w..",
     "wwyww",
     "..w..",
     "..w.."],
    ["...w...",
     "...w...",
     "..wyw..",
     "wwyWyww",
     "..wyw..",
     "...w...",
     "...w..."],
    ["....w....",
     "....w....",
     ".w..w..w.",
     "..wwyww..",
     "wwwyyywww",
     "..wwyww..",
     ".w..w..w.",
     "....w....",
     "....w...."],
]

HEART_S = [
    ".HH.HH.",
    "HpPHPPH",
    "HPPPPPH",
    ".HPPPH.",
    "..HPH..",
    "...H...",
]

HEART_M = [
    ".HHH.HHH.",
    "HwpPHPPPH",
    "HpPPPPPPH",
    "HPPPPPPhH",
    ".HPPPPhH.",
    "..HPPhH..",
    "...HhH...",
    "....H....",
]

PETAL = [
    ".PP..",
    "PpPP.",
    ".PPPh",
    "..hh.",
]

FLAKE_S = [
    ".w.",
    "wbw",
    ".w.",
]

FLAKE = [
    "w.w.w",
    ".wbw.",
    "wbwbw",
    ".wbw.",
    "w.w.w",
]


def bubble(r=4.5):
    s = int(r * 2 + 1)
    img = blank(s, s)
    m = ellipse_mask(s, s, s / 2, s / 2, r, r)
    ring = m & ~ellipse_mask(s, s, s / 2, s / 2, r - 1, r - 1)
    img[ring, :3] = rgb('b')
    img[ring, 3] = 255
    for (x, y) in ((3, 3), (4, 3), (3, 4)):   # the shine, inside the rim
        img[y, x, :3] = rgb('w')
        img[y, x, 3] = 255
    return img


def glow(core, mid, halo):
    """7x7: a bright core, a ring, a dithered halo (1-bit alpha: the halo is a checker)."""
    img = blank(7, 7)
    yy, xx = np.mgrid[0:7, 0:7]
    d = np.sqrt((xx - 3) ** 2 + (yy - 3) ** 2)
    halo_m = (d <= 3.2) & (d > 1.6) & (((xx + yy) % 2) == 0)
    mid_m = (d <= 1.6) & (d > 0.5)
    core_m = d <= 0.5
    for m, c in ((halo_m, halo), (mid_m, mid), (core_m, core)):
        img[m, :3] = rgb(c)
        img[m, 3] = 255
    return img


PUFF_BUMPS = ((0.25, 0.62, 0.2, 0.3), (0.5, 0.42, 0.26, 0.38), (0.75, 0.62, 0.2, 0.3))


def puff(w, h, face=False):
    """A soft little cloud (a wrong answer: "poof", the card goes to sleep):
    three bumps on a flat base, cream with a lilac rim; the big frame has a
    sleepy face with pink cheeks."""
    img = blank(w, h)
    yy, xx = np.mgrid[0:h, 0:w]
    m = np.zeros((h, w), bool)
    for (fx, fy, rx, ry) in PUFF_BUMPS:
        m |= ellipse_mask(w, h, w * fx, h * fy, w * rx, h * ry)
    m |= (yy >= h * 0.55) & (yy <= h - 2) & (xx >= w * 0.14) & (xx <= w * 0.86)
    inner = m & ~dilate4(~m)
    img[m, :3] = rgb('m')
    img[inner, :3] = rgb('W')
    img[inner & (yy > h * 0.72), :3] = rgb('l')
    if face:   # sleepy: two little closed eyes, a tiny mouth, pink cheeks
        ey, c = int(h * 0.58), w // 2
        for ex in (c - 4, c + 3):
            img[ey, ex:ex + 2, :3] = rgb('M')
        img[ey + 2, c, :3] = rgb('M')
        img[ey + 1, c - 6, :3] = rgb('c')
        img[ey + 1, c + 6, :3] = rgb('c')
    img[m, 3] = 255
    return img


def puff_break(w, h):
    """The last frame of the puff: it comes apart in little round bits."""
    img = blank(w, h)
    m = np.zeros((h, w), bool)
    for (cx, cy, r) in ((0.2, 0.62, 2.6), (0.5, 0.35, 3.2), (0.8, 0.6, 2.6), (0.42, 0.8, 2.0), (0.66, 0.85, 1.8)):
        m |= ellipse_mask(w, h, w * cx, h * cy, r * 1.1, r)
    inner = m & ~dilate4(~m)
    img[m, :3] = rgb('m')
    img[inner, :3] = rgb('W')
    img[m, 3] = 255
    return img


# 0.13: life in the backgrounds: a little bird flapping across the sky, butterflies of three colours
BIRD = [
    ["k.....k",
     ".k...k.",
     "..kkk.."],
    [".......",
     "..kkk..",
     ".k...k."],
]
BUTTERFLY = [
    ["k.....k",
     "xxk.kxx",
     "xwxkxwx",
     ".xxkxx.",
     "xx.k.xx",
     "......."],
    ["...k...",
     "..xkx..",
     "..wkx..",
     "..xkx..",
     "...k...",
     "......."],
]
BUTTERFLY_COLS = {'rosa': 'P', 'gialla': 'Y', 'azzurra': 'B'}

# 0.13: little clouds that drift across the day skies (the tones of the painted clouds)
CLOUDS = [
    (30, 12, ((7, 8, 6, 4), (14, 5.5, 7, 5), (22, 7, 6, 4.5), (27, 9, 3.5, 3))),
    (42, 15, ((7, 10, 6, 4.5), (15, 7, 8, 6), (25, 5.5, 9, 5.5), (34, 8, 7, 5), (39, 11, 3.5, 3))),
    (20, 9, ((5, 6, 4.5, 3), (10, 4.2, 5.5, 4), (15, 6, 4.5, 3))),
]


def cloud_sprite(w, h, lobes):
    """Lobes on a flat base, lit from above: white on top, a pale blue underneath."""
    img = blank(w, h)
    yy, xx = np.mgrid[0:h, 0:w]
    m = np.zeros((h, w), bool)
    for (cx, cy, rx, ry) in sorted(lobes, key=lambda l: -l[1]):
        e = ellipse_mask(w, h, cx, cy, rx, ry)
        ny = (yy + 0.5 - cy) / ry
        img[e & (ny >= 0.35), :3] = rgb('#d8e8ff')
        img[e & (ny < 0.35), :3] = rgb('#eef5ff')
        img[e & (ny < -0.3), :3] = rgb('#ffffff')
        m |= e
    xs = [l[0] for l in lobes]
    base = (yy >= h - 5) & (yy <= h - 1) & (xx >= min(xs)) & (xx <= max(xs))
    img[base & ~m, :3] = rgb('#d8e8ff')
    m |= base
    img[m & (yy == h - 1), :3] = rgb('#d8e8ff')
    img[m, 3] = 255
    return img


def all_sprites():
    out = {}
    for i, rows in enumerate(BIRD):
        out['uccello_%d' % i] = ascii_sprite(rows)
    for name, c in BUTTERFLY_COLS.items():
        for i, rows in enumerate(BUTTERFLY):
            out['farfalla_%s_%d' % (name, i)] = ascii_sprite([r.replace('x', c) for r in rows])
    for i, (w, h, lobes) in enumerate(CLOUDS):
        out['nuvola_%d' % i] = cloud_sprite(w, h, lobes)
    for i, rows in enumerate(TWINKLE):
        out['tw_%d' % i] = ascii_sprite(rows)
    out['heart_s'] = ascii_sprite(HEART_S)
    out['heart_m'] = ascii_sprite(HEART_M)
    out['petal'] = ascii_sprite(PETAL)
    out['flake_s'] = ascii_sprite(FLAKE_S)
    out['flake'] = ascii_sprite(FLAKE)
    out['bubble'] = bubble()
    out['glow_lime'] = glow('w', '7', '6')     # fireflies
    out['glow_gold'] = glow('w', 'y', 'Y')     # lanterns, a warm light
    out['glow_ember'] = glow('y', '@', 'R')    # the volcano's embers
    out['glow_cyan'] = glow('w', 't', 'T')     # glow-worms, crystals
    out['puff_0'] = puff(13, 9)
    out['puff_1'] = puff(21, 14, face=True)
    out['puff_2'] = puff_break(23, 15)
    return out
