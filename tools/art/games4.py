"""Art of the fourth wave of games (0.10.0): Le lettere, Le ombre, Il sentiero.

Backgrounds (320x240): the library of Mago Pistacchio (a parchment board for
the pictures and the words), the shadow theatre (a lit sheet where the lamp
throws the shadows), the meadow of stepping stones seen from above (the plank
at the bottom holds the program). Sprites: the three menu icons, the little
Deva that walks the stones (stand, step, hooray, oh), a rock, a flower to
pick, small arrows for the duel cards.
All drawn from scratch with the palette of pixel.py.
"""
import math

import numpy as np

import backgrounds as B
from objects import poly_mask, star_mask_px
from pixel import (blank, blit, ellipse_mask, rect_mask, capsule_mask, shaded_part, rgb, dilate4,
                   ascii_sprite)
from scenery import (YY, XX, LIGHT_UL, tone_fill, lobe, ellipse, rect, outline, paint as spaint, glow, vfade,
                     speckle, line, grass_tufts, firefly, moon_face)

W, H = 320, 240
INK = '#3b1f4a'


def put(img, mask, base, light, shadow, ol='k'):
    blit(img, shaded_part(mask, base, light, shadow, ol), 0, 0)


def paint(img, mask, c):
    img[mask, :3] = rgb(c) if isinstance(c, str) else c
    img[mask, 3] = 255


def with_outline(img, c='k'):
    """A 1 px outline around the opaque pixels (the sprite gets a 1 px margin)."""
    h, w = img.shape[:2]
    out = blank(w + 2, h + 2)
    out[1:-1, 1:-1] = img
    a = out[..., 3] > 0
    ring = dilate4(a) & ~a
    out[ring, :3] = rgb(c)
    out[ring, 3] = 255
    return out


# ------------------------------------------------------------------ the little Deva of the stones (20x27)
# honey-brown hair with a lighter streak, brown eyes, pink cheeks, the pink-and-white striped tee, the lilac
# tulle skirt, pink shoes: the same girl as the big Deva, as a pawn of a board game
MINI = [
    "......JJJJJJ......",   # 0
    "....JJjjjjjjJJ....",   # 1
    "...JjuujjjjjjJ....",   # 2
    "..JjuujjjjjjjjjJ..",   # 3
    ".JjjjjjjjjjjjjjjJ.",   # 4
    ".JjjjjjjjjjjjjjjJ.",   # 5
    ".JjJaaJaaaaJaaJjJ.",   # 6 the fringe
    ".Jjaaaaaaaaaaaa" + "jJ.",   # 7
    ".Jj" + "aawDaaaawDaa" + "jJ.",   # 8 eyes
    ".Jj" + "aaDDaaaaDDaa" + "jJ.",   # 9
    ".Jj" + "aaDDaaaaDDaa" + "jJ.",   # 10
    ".Jj" + "ccaaaaaaaacc" + "jJ.",   # 11 cheeks
    ".Jjj" + "aaaaHHaaaa" + "jjJ.",   # 12 smile
    "JjjjjJ" + "aaaaaa" + "JjjjjJ",   # 13 chin
    "Jjjjjq" + "qhhhhq" + "qjjjjJ",   # 14 the pink choker
    "Jjq" + "PPPPPPPPPPPP" + "qjJ",   # 15 sleeves
    "Jjq" + "aaWWWWWWWWaa" + "qjJ",   # 16 stripes, arms
    "uju" + "aaPPPPPPPPaa" + "uju",   # 17 the sun-kissed tips
    "..." + "aaWWWWWWWWaa" + "...",   # 18 hands
    "...." + "LLLLLLLLLL" + "....",   # 19 the skirt
    "..." + "lLLLLLLLLLLm" + "...",   # 20
    "..." + "mLLmmLLmmLLm" + "...",   # 21 tulle
    "......" + "aa" + ".." + "aa" + "......",   # 22 legs
    "......" + "aa" + ".." + "aa" + "......",   # 23
    "....." + "hhh" + ".." + "hhh" + ".....",   # 24 shoes
]
MINI[2] = "..." + "JjuujjjjjjjJ" + "..."   # the crown of the head, symmetric


def _edit(rows, changes):
    g = [list(r) for r in rows]
    for (y, x), c in changes.items():
        g[y][x] = c
    return [''.join(r) for r in g]


def mini_frames():
    for r in MINI:
        assert len(r) == 18, (r, len(r))
    base = MINI
    step = _edit(base, {(23, 6): '.', (23, 7): '.', (24, 5): '.', (24, 6): '.', (24, 7): '.',
                        (23, 5): 'h'})
    step = _edit(step, {(22, 6): 'a', (22, 7): 'a', (23, 6): 'h', (23, 7): 'h'})
    happy = {}
    for (y, x) in ((8, 5), (8, 6), (8, 11), (8, 12), (9, 4), (9, 7), (9, 10), (9, 13)):
        happy[(y, x)] = 'D'
    for (y, x) in ((9, 5), (9, 6), (9, 11), (9, 12), (10, 5), (10, 6), (10, 11), (10, 12)):
        happy[(y, x)] = 'a'
    for x in (7, 8, 9, 10):
        happy[(12, x)] = '9'
    happy[(13, 8)] = '9'
    happy[(13, 9)] = '9'
    for (y, x) in ((16, 3), (16, 4), (17, 3), (17, 4), (16, 13), (16, 14), (17, 13), (17, 14)):
        happy[(y, x)] = 'q'
    for (y, x) in ((18, 3), (18, 4), (18, 13), (18, 14)):
        happy[(y, x)] = '.'
    for (y, x) in ((10, 0), (11, 0), (11, 1), (12, 1), (13, 1), (13, 2), (14, 2),
                   (10, 17), (11, 17), (11, 16), (12, 16), (13, 16), (13, 15), (14, 15)):
        happy[(y, x)] = 'a'
    happy[(14, 3)] = 'P'
    happy[(14, 14)] = 'P'
    hooray = _edit(base, happy)
    oh = _edit(base, {(12, 8): '9', (12, 9): '9', (13, 8): '9', (13, 9): '9', (12, 7): 'a', (12, 10): 'a',
                      (2, 15): 'b', (3, 16): 'b', (4, 16): 'B', (4, 17): 'b', (5, 16): 'B', (5, 17): 'B'})
    out = {}
    for name, rows in (('deva_mini', base), ('deva_mini_passo', step), ('deva_mini_evviva', hooray),
                       ('deva_mini_oh', oh)):
        out[name] = with_outline(ascii_sprite(rows))
    return out


# ------------------------------------------------------------------ the stones game: rock, flower, small arrows
def _lit(w, h, cx, cy, rx, ry, L=(-0.55, -0.62, 0.56)):
    yy, xx = np.mgrid[0:h, 0:w]
    nx, ny = (xx + 0.5 - cx) / rx, (yy + 0.5 - cy) / ry
    r2 = nx * nx + ny * ny
    nz = np.sqrt(np.clip(1 - r2, 0, 1))
    lv = np.array(L) / np.linalg.norm(L)
    return r2 <= 1.0, np.clip(nx * lv[0] + ny * lv[1] + nz * lv[2], 0, 1)


def _ramp(img, mask, t, tones):
    b4 = B.BAYER4
    h, w = mask.shape
    yy, xx = np.mgrid[0:h, 0:w]
    n = len(tones)
    v = np.clip(t, 0, 0.9999) * (n - 1)
    i = np.floor(v).astype(int)
    i = np.clip(i + ((v - i) > b4[yy % 4, xx % 4]), 0, n - 1)
    for k, c in enumerate(tones):
        m = mask & (i == k)
        img[m, :3] = rgb(c)
        img[m, 3] = 255


ROCK_TONES = ['#4a4458', '#6b6480', '#8e87a3', '#b3adc4', '#d6d1e2']


def sasso(w=32, h=24, small=False):
    """A round boulder lit from the upper left, with a cushion of moss on top."""
    img = blank(w, h)
    lobes = [(w * 0.5, h * 0.6, w * 0.42, h * 0.38), (w * 0.34, h * 0.5, w * 0.26, h * 0.3),
             (w * 0.66, h * 0.52, w * 0.24, h * 0.3)]
    mask = np.zeros((h, w), bool)
    for (cx, cy, rx, ry) in sorted(lobes, key=lambda l: l[1]):
        m, d = _lit(w, h, cx, cy, rx, ry)
        _ramp(img, m & ~mask, 0.1 + d * 0.9, ROCK_TONES)
        mask |= m
    mask &= np.mgrid[0:h, 0:w][0] < h - 1
    img[~mask] = 0
    # a crack and a few specks
    yy, xx = np.mgrid[0:h, 0:w]
    crack = [(w * 0.58, h * 0.4), (w * 0.62, h * 0.55), (w * 0.56, h * 0.7)]
    for (x0, y0), (x1, y1) in zip(crack[:-1], crack[1:]):
        for t in np.linspace(0, 1, 12):
            x, y = int(x0 + (x1 - x0) * t), int(y0 + (y1 - y0) * t)
            if mask[y, x]:
                img[y, x, :3] = rgb('#4a4458')
    # moss on the top
    moss = mask & ellipse_mask(w, h, w * 0.4, h * 0.26, w * 0.2, h * 0.14)
    _ramp(img, moss, 0.3 + (1 - (yy - h * 0.12) / (h * 0.3)) * 0.6, ['#2a8a45', '#4cc25a', '#a8f08a'])
    if not small:   # a tiny white flower in the moss
        fx, fy = int(w * 0.4), int(h * 0.18)
        for (dx, dy) in ((0, -1), (-1, 0), (1, 0), (0, 1)):
            img[fy + dy, fx + dx, :3] = rgb('w')
            img[fy + dy, fx + dx, 3] = 255
        img[fy, fx, :3] = rgb('Y')
    out = blank(w + 2, h + 2)
    out[1:-1, 1:-1] = img
    a = out[..., 3] > 0
    ring = dilate4(a) & ~a
    out[ring, :3] = rgb('#241a30')
    out[ring, 3] = 255
    return out


def fiorellino(s=1.0):
    """A pink five-petal flower with a smiling yellow heart, a stem and two leaves."""
    w, h = int(round(18 * s)), int(round(22 * s))
    img = blank(w, h)
    cx, cy = w / 2, 8.5 * s
    stem = capsule_mask(w, h, (cx, cy + 3 * s), (cx, h - 1.5), max(0.6, 0.9 * s))
    leaf_l = ellipse_mask(w, h, cx - 3.4 * s, h - 5 * s, 3.2 * s, 1.6 * s)
    leaf_r = ellipse_mask(w, h, cx + 3.4 * s, h - 7 * s, 3.2 * s, 1.6 * s)
    put(img, stem | leaf_l | leaf_r, 'G', 'g', 'd')
    petals = np.zeros((h, w), bool)
    for k in range(5):
        a = -math.pi / 2 + k * 2 * math.pi / 5
        petals |= ellipse_mask(w, h, cx + math.cos(a) * 4.6 * s, cy + math.sin(a) * 4.6 * s, 3.6 * s, 3.6 * s)
    put(img, petals, 'P', 'p', 'h')
    heart = ellipse_mask(w, h, cx, cy, 2.9 * s, 2.9 * s)
    put(img, heart, 'Y', 'y', 'o', ol='o')
    if s >= 1.0:   # a tiny happy face: two dots and a little smile
        ix, iy = int(cx), int(cy)
        for (y, x) in ((iy - 1, ix - 2), (iy - 1, ix + 1), (iy + 1, ix - 1), (iy + 1, ix), (iy, ix - 2), (iy, ix + 1)):
            img[y, x, :3] = rgb('k')
        img[iy - 1, ix - 2, :3] = rgb('k')
        img[iy, ix - 2, :3] = rgb('o')
        img[iy, ix + 1, :3] = rgb('o')
        img[iy + 1, ix - 2, :3] = rgb('k')
        img[iy + 1, ix + 1, :3] = rgb('k')
        img[iy + 2, ix - 1, :3] = rgb('k')
        img[iy + 2, ix, :3] = rgb('k')
        img[iy + 1, ix - 1, :3] = rgb('y')
        img[iy + 1, ix, :3] = rgb('y')
    return img


def small_arrow(color, light, dark, n=12):
    """The arrows of the duel cards, n x n (12), pointing up (rotate for the others)."""
    k = n / 12.0
    m = poly_mask(n, n, [(6 * k, 0.8 * k), (11.2 * k, 6 * k), (8.2 * k, 6 * k), (8.2 * k, 11.2 * k),
                         (3.8 * k, 11.2 * k), (3.8 * k, 6 * k), (0.8 * k, 6 * k)])
    return shaded_part(m, color, light, dark)


def small_arrows():
    return {'sarrow_up': small_arrow('Y', 'y', 'o'),
            'sarrow_down': np.rot90(small_arrow('T', 't', 'e'), 2).copy(),
            'sarrow_left': np.rot90(small_arrow('h', 'P', 'H'), 1).copy(),
            'sarrow_right': np.rot90(small_arrow('v', 'L', 'V'), -1).copy()}


# ------------------------------------------------------------------ menu icons (44x44)
def _letter_a(w, h, x0, y0, x1, y1, r=1.3):
    xm = (x0 + x1) / 2
    m = capsule_mask(w, h, (x0, y1), (xm, y0), r) | capsule_mask(w, h, (xm, y0), (x1, y1), r)
    m |= capsule_mask(w, h, (x0 + (xm - x0) * 0.45, y0 + (y1 - y0) * 0.62),
                      (x1 - (x1 - xm) * 0.45, y0 + (y1 - y0) * 0.62), r * 0.85)
    return m


def _letter_b(w, h, x0, y0, x1, y1, r=1.3):
    ym = (y0 + y1) / 2
    m = capsule_mask(w, h, (x0, y0), (x0, y1), r)
    for (ya, yb, xr) in ((y0, ym, x1 - 1.2), (ym, y1, x1)):
        cyl = (ya + yb) / 2
        ry = (yb - ya) / 2
        yy, xx = np.mgrid[0:h, 0:w]
        rx = xr - x0
        d = ((xx + 0.5 - x0) / rx) ** 2 + ((yy + 0.5 - cyl) / ry) ** 2
        ring = (d <= 1.0) & (d >= 0.45) & (xx + 0.5 >= x0)
        m |= ring
    return m


def _block(img, x, y, s, face, top, side, letter_fn, ink='W'):
    """An alphabet block: front face s x s, the top and the right side in perspective."""
    w, h = img.shape[1], img.shape[0]
    d = max(3, s // 4)
    topm = poly_mask(w, h, [(x, y), (x + d, y - d), (x + s + d, y - d), (x + s, y)])
    sidem = poly_mask(w, h, [(x + s, y), (x + s + d, y - d), (x + s + d, y + s - d), (x + s, y + s)])
    frontm = rect_mask(w, h, x, y, x + s - 1, y + s - 1)
    put(img, topm, top[0], top[1], top[2])
    put(img, sidem, side[0], side[1], side[2])
    put(img, frontm, face[0], face[1], face[2])
    inner = rect_mask(w, h, x + 2, y + 2, x + s - 3, y + s - 3)
    ring = inner & ~rect_mask(w, h, x + 3, y + 3, x + s - 4, y + s - 4)
    paint(img, ring, face[1])
    lm = letter_fn(w, h, x + s * 0.24, y + s * 0.2, x + s * 0.76, y + s * 0.8)
    put(img, lm, ink, 'w', 's')


def menu_lettere():
    """An alphabet block with the A, and a little bee: "A... ape!"."""
    import objects as O
    img = blank(44, 44)
    _block(img, 4, 18, 22, ('h', 'P', 'H'), ('P', 'p', 'h'), ('H', 'h', 'H'), _letter_a)
    _, _, words = O.render_all(0.85)
    bee = words['ape']
    blit(img, bee, 44 - bee.shape[1], 0)
    for (x, y) in ((31, 26), (34, 30), (36, 34)):   # its little flight path, dotted
        img[y, x, :3] = rgb('k')
        img[y, x, 3] = 255
    return img


def _bunny_mask(w, h, cx, cy, s=1.0):
    """A sitting bunny seen from the side: the silhouette of the shadow."""
    m = ellipse_mask(w, h, cx - 2 * s, cy + 5 * s, 8 * s, 6.5 * s)             # the body
    m |= ellipse_mask(w, h, cx + 5 * s, cy - 1 * s, 4.6 * s, 4.2 * s)          # the head
    m |= poly_mask(w, h, [(cx + 1.2 * s, cy - 3 * s), (cx + 0.8 * s, cy - 11 * s), (cx + 2.6 * s, cy - 14 * s),
                          (cx + 4.2 * s, cy - 11 * s), (cx + 4.4 * s, cy - 3 * s)])     # the ears, apart
    m |= poly_mask(w, h, [(cx + 5.6 * s, cy - 3 * s), (cx + 7.4 * s, cy - 11 * s), (cx + 9.6 * s, cy - 13 * s),
                          (cx + 10.2 * s, cy - 9.6 * s), (cx + 8.4 * s, cy - 2 * s)])
    m |= ellipse_mask(w, h, cx - 10 * s, cy + 3 * s, 2.4 * s, 2.4 * s)        # the tail
    m |= ellipse_mask(w, h, cx + 3 * s, cy + 10.5 * s, 3.4 * s, 1.4 * s)       # the paws
    return m


def menu_ombre():
    img = blank(44, 44)
    frame = rect_mask(44, 44, 3, 3, 40, 37, radius=2)
    put(img, frame, 'J', 'j', 'q')
    sheet = rect_mask(44, 44, 6, 6, 37, 34, radius=1)
    paint(img, sheet, 'y')
    yy, xx = np.mgrid[0:44, 0:44]
    d = np.sqrt(((xx - 21.5) / 16) ** 2 + ((yy - 20) / 14) ** 2)
    paint(img, sheet & (d < 0.62), 'W')
    paint(img, sheet & (d > 1.0), 'f')
    bunny = _bunny_mask(44, 44, 20, 21, 0.95)
    paint(img, bunny & sheet, '$')
    img[19, 26, :3] = rgb('f')   # the eye, lit through
    # the two feet of the frame, and a little lamp glowing on the floor
    for x in (6, 34):
        put(img, rect_mask(44, 44, x, 38, x + 3, 41), 'q', 'J', 'Q')
    lamp = ellipse_mask(44, 44, 21.5, 40, 4, 2.4) | rect_mask(44, 44, 20, 36, 23, 39)
    put(img, lamp, 'Y', 'y', 'o')
    return img


def menu_sentiero():
    """Stepping stones with a corner, a little arrow on each stone, the star waiting at the end."""
    img = blank(44, 44)
    stones = ((7, 38), (20, 38), (33, 38), (33, 25))
    for (cx, cy) in stones:
        st = ellipse_mask(44, 44, cx, cy, 6.4, 4.8)
        put(img, st, 'f', 'W', 'F')
    right = np.rot90(small_arrow('v', 'L', 'V', 9), -1).copy()
    up = small_arrow('Y', 'y', 'o', 9)
    for (cx, cy), a in zip(stones, (right, right, up, up)):
        blit(img, a, cx - 4, cy - 5)
    star = star_mask_px(44, 44, 33, 10, 8, 3.6)
    put(img, star, 'Y', 'y', 'o')
    return img


# ------------------------------------------------------------------ backgrounds
def _canvas():
    return B.canvas()


def _board(img, x0, y0, x1, y1, paper='#fdf1d6', wood=('J', 'j', 'q')):
    """The parchment board of the quiz panel in a wooden frame (as in the laboratory)."""
    m = rect_mask(W, H, x0, y0, x1, y1, radius=6)
    frame = rect_mask(W, H, x0 - 4, y0 - 4, x1 + 4, y1 + 4, radius=8)
    put(img, frame, *wood)
    paint(img, m, B.hexrgb(paper))
    speck = m & (((XX * 7 + YY * 13) % 29) == 0)
    img[speck, :3] = B.hexrgb('#f1dcb0')
    edge = m & ~rect_mask(W, H, x0 + 2, y0 + 2, x1 - 2, y1 - 2, radius=5)
    img[edge, :3] = B.hexrgb('#e8cf9c')


BOOKS = ['#e5395a', '#ff93c6', '#74b8ff', '#4cc25a', '#ffd23f', '#b376ec', '#4fd6c0', '#ffa860', '#fff4fa']


def _books(img, x0, x1, base, rng, hmin=18, hmax=30):
    x = x0
    while x < x1 - 3:
        bw = int(rng.integers(4, 8))
        if x + bw > x1:
            bw = x1 - x
        bh = int(rng.integers(hmin, hmax + 1))
        c = BOOKS[int(rng.integers(0, len(BOOKS)))]
        lean = rng.random() < 0.12 and x + bw + 4 < x1
        if lean:   # a book leaning on the next one
            m = poly_mask(W, H, [(x, base), (x + bw, base), (x + bw + 4, base - bh + 2), (x + 4, base - bh)])
        else:
            m = rect(x, base - bh, x + bw - 1, base - 1)
        outline(img, m, INK)
        tone_fill(img, m, 0.55 - (XX - x) / max(1, bw) * 0.3, [c, c])
        spaint(img, m & (XX == x), '#ffffff')
        band = m & (YY >= base - bh + 3) & (YY <= base - bh + 4)
        spaint(img, band, '#ffd23f' if c != '#ffd23f' else '#f0a030')
        x += bw + (5 if lean else 1)


def _owl(img, cx, base):
    """A little sleepy owl sitting on the bookcase (the library's keeper)."""
    body = ellipse(cx, base - 8, 7, 8.5)
    ears = poly_mask(W, H, [(cx - 6, base - 13), (cx - 5, base - 19), (cx - 2, base - 15)]) | \
        poly_mask(W, H, [(cx + 6, base - 13), (cx + 5, base - 19), (cx + 2, base - 15)])
    outline(img, body | ears, INK)
    tone_fill(img, body | ears, 0.3 + (-(XX - cx) / 8.0 - (YY - base + 8) / 9.0) * 0.3 + 0.2,
              ['#8a5a30', '#a8703e', '#c98f58'])
    belly = ellipse(cx, base - 5, 4.5, 5)
    spaint(img, belly, '#f3dcb4')
    for k in range(3):
        spaint(img, belly & (YY == base - 7 + k * 2) & ((XX + k) % 2 == 0), '#c98f58')
    for ex in (cx - 3, cx + 3):   # closed, sleepy eyes
        e = ellipse(ex, base - 12, 2.6, 2.6)
        spaint(img, e, '#fff4e0')
        spaint(img, rect(ex - 1.5, base - 12, ex + 1, base - 12), INK)
    spaint(img, poly_mask(W, H, [(cx - 1, base - 11), (cx + 1, base - 11), (cx, base - 8.5)]), '#f0a030')
    for x in (cx - 3, cx + 2):
        spaint(img, rect(x, base - 1, x + 1, base), '#f0a030')


def biblioteca_bg():
    img = _canvas()
    # the wall: soft lilac paper with little stars, wooden wainscot
    B.gradient(img, 0, 196, ['#d9c8ee', '#e3d4f2', '#ecdff6'])
    stars_m = (((XX // 16) + (YY // 16)) % 2 == 0) & (XX % 16 == 8) & (YY % 16 == 8)
    for (y, x) in zip(*np.where(stars_m)):
        spaint(img, rect(x - 1, y, x + 1, y) | rect(x, y - 1, x, y + 1), '#f7eefc')
    wains = (YY >= 150) & (YY < 196)
    tone_fill(img, wains, 0.4 + (YY - 150) / 46.0 * 0.3, ['#8a5a30', '#a8703e', '#c98f58'])
    for x in range(0, W, 24):
        spaint(img, wains & (XX == x), '#6e4526')
    spaint(img, rect(0, 150, W - 1, 151), '#6e4526')
    # the big bookcase on the left
    case = rect(0, 26, 76, 196)
    outline(img, case, INK)
    tone_fill(img, case, 0.5, ['#6e4526', '#8a5a30'])
    rng = np.random.default_rng(12)
    for (y0, y1) in ((32, 64), (70, 102), (108, 140), (146, 176)):
        back = rect(4, y0, 72, y1 - 1)
        spaint(img, back, '#4a2f1b')
        _books(img, 5, 72, y1, rng, hmin=(y1 - y0) - 14, hmax=(y1 - y0) - 3)
        shelf = rect(0, y1, 76, y1 + 4)
        outline(img, shelf, INK)
        tone_fill(img, shelf, 0.6 - (YY - y1) / 5.0 * 0.4, ['#8a5a30', '#a8703e', '#c98f58'])
    spaint(img, rect(4, 181, 72, 196), '#4a2f1b')
    _owl(img, 58, 26)
    # a little potted plant beside the owl
    pot = poly_mask(W, H, [(14, 26), (26, 26), (24, 17), (16, 17)])
    outline(img, pot, INK)
    spaint(img, pot, '#e0609e')
    spaint(img, rect(15, 17, 25, 18), '#ff93c6')
    for (x0, y0, x1, y1) in ((20, 17, 14, 8), (20, 17, 20, 5), (20, 17, 26, 8), (20, 17, 11, 13), (20, 17, 29, 12)):
        line(img, [(x0, y0), (x1, y1)], '#2a8a45', 0.5)
        spaint(img, ellipse(x1, y1, 2.2, 1.6), '#4cc25a')
    # a thin case on the right edge
    case2 = rect(306, 30, 319, 196)
    outline(img, case2, INK)
    tone_fill(img, case2, 0.5, ['#6e4526', '#8a5a30'])
    for (y0, y1) in ((36, 70), (76, 110), (116, 150), (156, 190)):
        spaint(img, rect(309, y0, 319, y1 - 1), '#4a2f1b')
        _books(img, 309, 320, y1, rng, hmin=(y1 - y0) - 12, hmax=(y1 - y0) - 3)
        spaint(img, rect(306, y1, 319, y1 + 3), '#a8703e')
    # the parchment board (the quiz panel)
    _board(img, *B.PANEL)
    # a hanging lamp between the board and the right case
    line(img, [(304, 0), (304, 10)], INK, 0.4)
    lamp = poly_mask(W, H, [(298, 18), (310, 18), (307, 10), (301, 10)])
    outline(img, lamp, INK)
    spaint(img, lamp, '#ffd23f')
    spaint(img, ellipse(304, 19, 2.2, 1.6), '#fff3a6')
    glow(img, 304, 24, 18, 14, '#fff3a6', 0.3)
    # the floor: warm boards and a round rug with letters' colours
    floor = YY >= 196
    for y0 in range(196, H, 11):
        k = ((y0 - 196) // 11) % 2
        img[(YY >= y0) & (YY < y0 + 11), :3] = B.hexrgb('#b8885c') if k else B.hexrgb('#a8784e')
    for x in range(0, W, 30):
        spaint(img, floor & (XX == x + ((YY - 196) // 11 % 2) * 15), '#8a5a30')
    spaint(img, rect(0, 196, W - 1, 196), INK)
    rug = ellipse(170, 222, 130, 15)
    outline(img, rug, INK)
    spaint(img, rug, '#74b8ff')
    for k, c in enumerate(['#ffd23f', '#ff93c6', '#4cc25a']):
        spaint(img, ellipse(170, 222, 124 - k * 12, 13 - k * 1.8) & ~ellipse(170, 222, 118 - k * 12, 11.4 - k * 1.8), c)
    return img


def ombre_bg():
    img = _canvas()
    # the dark little theatre: violet night with faint stars
    B.gradient(img, 0, 196, ['#1c1236', '#261846', '#302056', '#3a2866'])
    rng = np.random.default_rng(8)
    B.stars(img, rng, 40, (0, 0, W, 120))
    x0, y0, x1, y1 = B.PANEL
    # the lit sheet (the quiz panel), warm in the middle
    sheet = rect(x0, y0, x1, y1)
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    d = np.sqrt(((XX - cx) / ((x1 - x0) / 2)) ** 2 + ((YY - cy) / ((y1 - y0) / 2)) ** 2)
    tone_fill(img, sheet, np.clip(1.05 - d * 0.55, 0, 1), ['#e8cfa4', '#f3dfb8', '#fbeccc', '#fff6e0'])
    for x in range(int(x0) + 18, int(x1), 36):   # soft folds of the cloth
        spaint(img, sheet & (XX == x) & ((YY % 3) != 0), '#f0dcb0')
    glow(img, cx, y1 + 30, 150, 60, '#fff3c4', 0.12)
    # the wooden frame and two feet
    frame = rect(x0 - 5, y0 - 5, x1 + 5, y1 + 5) & ~sheet
    outline(img, frame | sheet, INK)
    tone_fill(img, frame, 0.55 + (-(XX - cx) / 300.0 - (YY - cy) / 200.0), ['#6e4526', '#8a5a30', '#a8703e', '#c98f58'])
    # velvet curtains tied at the sides, a valance on top
    for side in (-1, 1):
        xe = x0 - 5 if side < 0 else x1 + 5
        pts = [(xe, y0 - 8), (xe + side * 26, y0 - 8), (xe + side * 12, y0 + 60), (xe + side * 20, y1 + 30),
               (xe, y1 + 30)]
        cur = poly_mask(W, H, pts)
        outline(img, cur, INK)
        folds = np.cos((XX - xe) * 0.9) * 0.3 + 0.5
        tone_fill(img, cur, folds, ['#6a1830', '#8a2240', '#b8324e', '#d44c66'])
        tie = ellipse(xe + side * 12, y0 + 62, 5, 3)
        outline(img, tie, INK)
        spaint(img, tie, '#ffd23f')
    val = rect(x0 - 14, y0 - 12, x1 + 14, y0 - 2)
    for k in range(int((x1 - x0 + 28) / 14) + 1):
        val |= ellipse(x0 - 14 + 7 + k * 14, y0 - 2, 7, 5)
    val &= YY <= y0 + 3
    outline(img, val, INK)
    tone_fill(img, val, 0.4 + np.cos((XX - x0) * 0.45) * 0.3, ['#6a1830', '#8a2240', '#b8324e', '#d44c66'])
    spaint(img, val & (YY == y0 - 11), '#ffd23f')
    # the stage and its lamp
    stage = rect(0, 170, W - 1, 196)
    tone_fill(img, stage, 0.35 + (YY - 170) / 26.0 * 0.4, ['#6e4526', '#8a5a30', '#a8703e'])
    for x in range(0, W, 26):
        spaint(img, stage & (XX == x), '#5a3820')
    spaint(img, rect(0, 170, W - 1, 170), INK)
    for (lx, ly) in ((92, 168), (290, 168)):
        base = rect(lx - 4, ly - 4, lx + 4, ly)
        outline(img, base, INK)
        spaint(img, base, '#ffd23f')
        glass = ellipse(lx, ly - 9, 4, 5.5)
        outline(img, glass, INK)
        spaint(img, glass, '#fff3a6')
        spaint(img, ellipse(lx, ly - 9, 1.4, 2.4), '#ffa860')
        glow(img, lx, ly - 9, 20, 18, '#fff3a6', 0.3)
    # the floor of the audience
    B.gradient(img, 196, H, ['#2a1a40', '#24163a', '#1e1234'])
    spaint(img, rect(0, 196, W - 1, 196), INK)
    return img


def prato_bg():
    """The meadow of the stones seen from above: a calm lawn in the middle, bushes and flowers around,
    a pond in a corner, the plank of the program at the bottom."""
    img = _canvas()
    # lawn: two greens in soft mowing stripes, dithered
    stripes = ((XX + YY // 2) // 20) % 2
    t = 0.45 + stripes * 0.18 + (YY / H) * 0.12
    tone_fill(img, YY >= 0, t, ['#5fa456', '#6fb462', '#80c46e', '#8fd07a'])
    speckle(img, YY >= 0, '#94d47c', 0.04, 3)
    speckle(img, YY >= 0, '#5a9a50', 0.03, 4)
    calm = rect(36, 36, 284, 172)   # the stones go here: nothing but lawn
    rng = np.random.default_rng(5)
    for _ in range(60):   # tiny flowers seen from above
        x, y = int(rng.integers(2, W - 3)), int(rng.integers(2, 186))
        if calm[y, x]:
            continue
        c = ['#ffffff', '#fff3a6', '#ffcbe3', '#c4b0ff'][int(rng.integers(0, 4))]
        spaint(img, rect(x - 1, y, x + 1, y) | rect(x, y - 1, x, y + 1), c)
        img[y, x, :3] = rgb('#ffd23f')
    # bushes along the top, round crowns seen from above
    greens = ['#2d6440', '#3e7a4c', '#4e9058', '#6aa962', '#8cc876']
    for (cx, cy, r) in ((6, 10, 16), (36, 4, 12), (64, 12, 13), (256, 6, 12), (286, 12, 15), (316, 2, 14),
                        (0, 60, 14), (320, 70, 16), (4, 150, 12), (318, 150, 12)):
        for (dx, dy, rr) in ((0, 0, 1.0), (-r * 0.5, r * 0.3, 0.6), (r * 0.5, r * 0.25, 0.62), (0, -r * 0.4, 0.55)):
            lobe(img, cx + dx, cy + dy, r * rr, r * rr * 0.9, greens, L=LIGHT_UL, gamma=0.9)
        for k in range(3):
            a = k * 2.1 + cx
            fx, fy = int(cx + math.cos(a) * r * 0.5), int(cy + math.sin(a) * r * 0.45)
            if 0 <= fx < W and 0 <= fy < H:
                spaint(img, rect(fx - 1, fy, fx + 1, fy) | rect(fx, fy - 1, fx, fy + 1), '#ff93c6')
    # a little pond in the lower right corner (lily pads, a frog-green glint)
    pond = ellipse(306, 176, 22, 12)
    outline(img, pond, '#2b6a8a')
    tone_fill(img, pond, 0.4 + (YY - 176) / 12 * 0.3, ['#4f96e0', '#6ab8f0', '#8fd0ff'][::-1])
    for (x, y) in ((298, 172), (312, 180)):
        spaint(img, ellipse(x, y, 3.5, 2), '#4cc25a')
    # the plank of the program at the bottom
    x0, x1, y0, y1 = 22, 298, 190, 232
    plank = rect(x0, y0, x1, y1) | ellipse(x0, (y0 + y1) / 2, 6, (y1 - y0) / 2) | ellipse(x1, (y0 + y1) / 2, 6, (y1 - y0) / 2)
    shadow = rect(x0 + 2, y0 + 3, x1 + 2, y1 + 3)
    spaint(img, shadow & ~plank, '#4e8a46')
    outline(img, plank, INK)
    tone_fill(img, plank, 0.55 - (YY - y0) / (y1 - y0) * 0.35, ['#8a5a30', '#a8703e', '#c98f58', '#dca872'])
    for y in (y0 + 14, y0 + 28):
        spaint(img, plank & (YY == y), '#8a5a30')
    for (x, y) in ((x0 + 4, y0 + 5), (x1 - 4, y0 + 5), (x0 + 4, y1 - 5), (x1 - 4, y1 - 5)):   # nails
        spaint(img, rect(x, y, x, y), '#5a3820')
    return img


def all_backgrounds():
    return {'bg_biblioteca': biblioteca_bg(), 'bg_ombre': ombre_bg(), 'bg_prato': prato_bg()}


def all_sprites():
    out = {'menu_lettere': menu_lettere(), 'menu_ombre': menu_ombre(), 'menu_sentiero': menu_sentiero(),
           'sasso': sasso(), 'sasso_s': sasso(22, 16, small=True), 'fiorellino': fiorellino(1.0),
           'fiorellino_s': fiorellino(0.55)}
    out.update(small_arrows())
    out.update(mini_frames())
    return out
