"""Art of the fifth wave of games (0.11.0): Il negozio and Le misure.

Il negozio: the coins (copper 1, silver 2, golden 5, the value written on
them), the price sign on its easel, a dot for the prices of level 1, the toy
till (closed and open), the purse, the toys for sale (36x36 like the word
pictures: trenino, orsetto, aquilone, trottola, robot, dinosauro, tamburo) and
the shop (bg_negozio: the counter is the quiz panel, things stand on its back
edge at y 107 and coins lie on it around y 139).

Le misure: five sizes of each thing (mis_<kind>_<0..4>, smallest first):
big/small (pallone, orsetto, regalo: the height grows), long/short (pastello,
serpente, bruco: the width grows, the height stays), tall/low (albero,
girasole, castello) and full/empty (bicchiere, barattolo, acquario: 32x40, what
is inside reaches row 36 - 7 * v); the things to weigh (pic_piuma ... 36x36);
the bubble icons, the parts of the spring scales, the shelf, and the playroom
(bg_misure).
"""
import numpy as np

import backgrounds as B
from objects import poly_mask, star_mask_px
from pixel import blank, blit, ellipse_mask, rect_mask, shaded_part, rgb, dilate4, ascii_sprite
from scenery import (YY, XX, LIGHT_UL, tone_fill, lobe, ellipse, rect, outline, paint as spaint, glow, vfade,
                     speckle, line, cloud, puff_row)

W, H = 320, 240
INK = '#3b1f4a'


def put(img, mask, base, light, shadow, ol='k'):
    blit(img, shaded_part(mask, base, light, shadow, ol), 0, 0)


def paint(img, mask, c):
    img[mask, :3] = rgb(c) if isinstance(c, str) else c
    img[mask, 3] = 255


def px(img, x, y, c):
    if 0 <= y < img.shape[0] and 0 <= x < img.shape[1]:
        img[y, x, :3] = rgb(c)
        img[y, x, 3] = 255


def E(w, h, cx, cy, rx, ry):
    return ellipse_mask(w, h, cx, cy, rx, ry)


def R(w, h, x0, y0, x1, y1, r=0):
    return rect_mask(w, h, int(round(x0)), int(round(y0)), int(round(x1)), int(round(y1)), int(r))


def P(w, h, pts):
    return poly_mask(w, h, pts)


def tline(img, x0, y0, x1, y1, c, thick=1):
    n = int(max(abs(x1 - x0), abs(y1 - y0))) + 1
    for i in range(n + 1):
        t = i / max(n, 1)
        x = int(round(x0 + (x1 - x0) * t))
        y = int(round(y0 + (y1 - y0) * t))
        for d in range(thick):
            px(img, x + d, y, c)


def worm(w, h, pts, r0, r1, steps=10):
    m = np.zeros((h, w), bool)
    segs = len(pts) - 1
    for i in range(segs):
        (x0, y0), (x1, y1) = pts[i], pts[i + 1]
        for k in range(steps + 1):
            t = k / steps
            u = (i + t) / segs
            r = r0 + (r1 - r0) * u
            m |= E(w, h, x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, r, r)
    return m


# ------------------------------------------------------------------ coins
DIGITS = {
    '1': [".k.", "kk.", ".k.", ".k.", ".k.", "kkk"],
    '2': [".kk.", "k..k", "..k.", ".k..", "k...", "kkkk"],
    '5': ["kkkk", "k...", "kkk.", "...k", "k..k", ".kk."],
}


def _digit(img, ch, x0, y0, c):
    for y, row in enumerate(DIGITS[ch]):
        for x, v in enumerate(row):
            if v == 'k':
                px(img, x0 + x, y0 + y, c)


def coin(value):
    """A coin: copper 1 (11 px), silver 2 (13 px), gold 5 (15 px), its value on it."""
    size, tones, ink = {1: (11, ('#d07a3a', '#f4b070', '#9a5022'), '#6a3414'),
                        2: (13, ('#c8c4d8', '#f6f4fc', '#8e8aa6'), '#2e2a44'),
                        5: (15, ('#ffc93a', '#fff3a6', '#d99a1c'), '#8a5a10')}[value]
    img = blank(size, size)
    c = (size - 1) / 2.0 + 0.5
    m = E(size, size, c, c, c - 1, c - 1)
    put(img, m, *tones)
    if value > 1:   # the raised rim
        ring = E(size, size, c, c, c - 2.6, c - 2.6) & ~E(size, size, c, c, c - 3.6, c - 3.6)
        paint(img, ring, tones[2])
    if value == 5:   # little stars on the rim of the golden one
        for (x, y) in ((3, 3), (11, 3), (3, 11), (11, 11)):
            px(img, x, y, '#ffffff')
    dw = len(DIGITS[str(value)][0])
    _digit(img, str(value), (size - dw) // 2, (size - 6) // 2, ink)
    px(img, 2 if size < 14 else 3, 2 if size < 14 else 3, '#ffffff')   # a glint
    return img


def pallino():
    img = blank(6, 6)
    put(img, E(6, 6, 3, 3, 2.2, 2.2), 'Y', 'y', 'o')
    return img


def cartello():
    """The price sign on a little easel (44x50): the board rows 0..37, the number
    drawn by the game at row 18 (13 when the coins are drawn too, at row 26)."""
    w, h = 44, 50
    img = blank(w, h)
    for (x0, x1) in ((9, 5), (34, 38)):   # the legs of the easel
        put(img, worm(w, h, [(x0, 34), (x1, 48)], 1.6, 1.6), '#a8703e', '#c98f58', '#7a4a26')
    board = R(w, h, 1, 1, 42, 37, 3)
    put(img, board, '#a8703e', '#c98f58', '#7a4a26')
    inner = R(w, h, 3, 3, 40, 35, 2)
    paint(img, inner, '#fff6e0')
    paint(img, inner & (np.mgrid[0:h, 0:w][0] == 3), '#ffffff')
    put(img, E(w, h, 22, 2.5, 2.4, 2.4), 'r', 'P', 'R')   # a red pin
    return img


def cassa(open_):
    """A toy till (48x40), standing on its bottom row: buttons, a little display; open: the drawer
    is out with coins in it."""
    w, h = 48, 40
    img = blank(w, h)
    # the display on its stand, at the back
    put(img, R(w, h, 30, 6, 33, 12), '#c8c4d8', '#f6f4fc', '#8e8aa6')
    disp = R(w, h, 24, 1, 40, 8, 2)
    put(img, disp, 'h', 'P', 'H')
    paint(img, R(w, h, 26, 3, 38, 6), '#bdf5ea')
    paint(img, E(w, h, 32, 4.5, 1.8, 1.6), '#ffc93a')   # a little coin on the screen
    px(img, 32, 4, '#d99a1c')
    # the sloping body with keys
    body = P(w, h, [(4, 28), (44, 28), (40, 10), (8, 10)])
    put(img, body, 'h', 'P', 'H')
    keys = ['#ffd23f', '#74b8ff', '#4cc25a', '#ff7b2e', '#ffffff', '#b376ec', '#ff4d6d', '#4fd6c0', '#ffd23f']
    for j in range(3):
        for i in range(3):
            kx, ky = 13 + i * 6 + j * -1, 14 + j * 5
            m = E(w, h, kx, ky, 2.2, 1.7)
            paint(img, dilate4(m) & ~m & body, 'H')
            paint(img, m, keys[j * 3 + i])
    put(img, R(w, h, 33, 13, 40, 25, 1), '#ffd23f', '#fff3a6', '#f0a030')   # the big key
    # the drawer
    if open_:
        tray = R(w, h, 6, 28, 42, 32)
        put(img, tray, '#7a4a26', '#a8703e', '#5a3820')
        for (x, c) in ((10, '#ffc93a'), (15, '#c8c4d8'), (20, '#d07a3a'), (25, '#ffc93a'), (30, '#c8c4d8'), (35, '#ffc93a')):
            paint(img, E(w, h, x, 30, 1.8, 1.4), c)
        front = R(w, h, 5, 33, 43, 39, 1)
    else:
        front = R(w, h, 5, 28, 43, 38, 1)
    put(img, front, 'P', 'p', 'h')
    fy = 36 if open_ else 33
    paint(img, R(w, h, 20, fy, 28, fy + 1), '#ffd23f')   # the handle
    return img


def borsellino():
    """An open purse (36x30) that coins jump out of."""
    w, h = 36, 30
    img = blank(w, h)
    put(img, E(w, h, 18, 8, 9, 4), '#3b1f4a', '#5a3a6a', '#24122f')   # the dark opening
    body = E(w, h, 18, 19, 15, 10) | R(w, h, 4, 9, 32, 19, 4)
    put(img, body, 'h', 'P', 'H')
    paint(img, E(w, h, 18, 20, 11, 6) & body, 'P')
    for x in (13, 23):   # the clasp
        put(img, E(w, h, x, 7, 2.4, 2.4), 'Y', 'y', 'o')
    paint(img, R(w, h, 5, 9, 31, 10) & body, 'Y')
    put(img, E(w, h, 18, 6, 3, 2.4), 'Y', 'y', 'o')   # a coin peeping out
    return img


# ------------------------------------------------------------------ toys for sale (36x36)
S36 = 36


def pic_trenino():
    img = blank(S36, S36)
    w = h = S36
    put(img, R(w, h, 2, 15, 14, 26, 1), 'B', 'b', 'n')            # the wagon
    for (x, c) in ((4, ('Y', 'y', 'o')), (9, ('G', 'g', 'd'))):   # its cargo of blocks
        put(img, R(w, h, x, 10, x + 4, 15), *c)
    tline(img, 14, 22, 17, 22, 'k')
    put(img, R(w, h, 17, 7, 24, 26, 1), 'X', 'r', 'R')              # the cab
    paint(img, R(w, h, 19, 10, 22, 14), 'b')
    put(img, R(w, h, 16, 5, 25, 7), 'R', 'r', 'k')                  # the roof
    put(img, R(w, h, 24, 14, 34, 26, 2), 'X', 'r', 'R')             # the boiler
    put(img, R(w, h, 28, 6, 31, 14), 'x', 'S', 'k')                 # the chimney
    put(img, E(w, h, 29.5, 5, 3, 1.6), 'x', 'S', 'k')
    put(img, R(w, h, 33, 18, 35, 22), 'Y', 'y', 'o')                # the lamp
    for x in (6, 11, 20, 29):
        put(img, E(w, h, x, 28, 3.2, 3.2), 'k', 'x', 'K')
        px(img, x, 28, 'S')
    return img


def orsetto_mask(w, h, H_, cx, base):
    """A teddy bear of height H_ standing on `base`, centred on cx: the masks of its parts."""
    k = H_ / 42.0
    top = base - H_
    parts = {
        'ears': E(w, h, cx - 9 * k, top + 5 * k, 4 * k + 0.6, 4 * k + 0.6) | E(w, h, cx + 9 * k, top + 5 * k, 4 * k + 0.6, 4 * k + 0.6),
        'legs': E(w, h, cx - 6.5 * k, base - 4 * k, 5 * k + 0.5, 4 * k + 0.5) | E(w, h, cx + 6.5 * k, base - 4 * k, 5 * k + 0.5, 4 * k + 0.5),
        'body': E(w, h, cx, top + 28 * k, 11 * k + 0.5, 12 * k + 0.5),
        'arms': E(w, h, cx - 12 * k, top + 25 * k, 4 * k + 0.5, 6 * k + 0.5) | E(w, h, cx + 12 * k, top + 25 * k, 4 * k + 0.5, 6 * k + 0.5),
        'head': E(w, h, cx, top + 12.5 * k, 11 * k + 0.6, 10 * k + 0.6),
    }
    return parts, k, top


def draw_orsetto(img, H_, cx, base, fur=('#c98f58', '#e0ad76', '#a8703e')):
    h, w = img.shape[:2]
    p, k, top = orsetto_mask(w, h, H_, cx, base)
    for name in ('ears', 'legs', 'body', 'arms', 'head'):
        put(img, p[name], *fur)
    for sx in (-1, 1):
        paint(img, E(w, h, cx + sx * 9 * k, top + 5 * k, 2 * k + 0.3, 2 * k + 0.3) & p['ears'], '#f2cc98')
    paint(img, E(w, h, cx, top + 30 * k, 7 * k + 0.3, 7 * k + 0.3) & p['body'], '#f2cc98')   # the belly
    paint(img, E(w, h, cx, top + 16 * k, 4.6 * k + 0.4, 3.4 * k + 0.4) & p['head'], '#f2cc98')   # the muzzle
    ey = int(round(top + 11 * k))
    for sx in (-1, 1):
        ex = int(round(cx + sx * 4.5 * k))
        px(img, ex, ey, 'k')
        if k > 0.6:
            px(img, ex, ey + 1, 'k')
    nx, ny = int(round(cx)), int(round(top + 14.5 * k))
    px(img, nx, ny, 'k')
    if k > 0.5:
        px(img, nx - 1, ny, 'k')
        px(img, nx, ny + 2, 'k')
        px(img, nx - 1, ny + 2, 'k')
    if k > 0.6:   # a red bow at the neck
        for sx in (-1, 1):
            paint(img, P(w, h, [(cx, top + 22 * k), (cx + sx * 4.5 * k, top + 20 * k), (cx + sx * 4.5 * k, top + 24 * k)]), 'r')
        paint(img, E(w, h, cx, top + 22 * k, 1.3, 1.3), 'R')


def pic_orsetto():
    img = blank(S36, S36)
    draw_orsetto(img, 34, 18, 35)
    return img


def pic_aquilone():
    img = blank(S36, S36)
    w = h = S36
    cx, cy = 21, 12
    pts = [(cx, cy - 11), (cx + 9, cy), (cx, cy + 13), (cx - 9, cy)]
    kite = P(w, h, pts)
    put(img, kite, 'X', 'r', 'R')
    cols = ['X', 'Y', 'B', 'G']
    for i, (a, b) in enumerate(zip(pts, pts[1:] + pts[:1])):
        tri = P(w, h, [(cx, cy), a, b])
        paint(img, tri & kite & ~(dilate4(~kite)), cols[i])
    tline(img, cx, cy - 10, cx, cy + 12, 'W')
    tline(img, cx - 8, cy, cx + 8, cy, 'W')
    tail = [(cx, cy + 13)] + [(cx - 3 - k * 3.4, cy + 15 + 2.5 * np.sin(k * 1.3) + k * 1.6) for k in range(6)]
    for (a, b) in zip(tail, tail[1:]):
        tline(img, a[0], a[1], b[0], b[1], 'k')
    for k in (2, 4):
        x, y = tail[k]
        bow = P(w, h, [(x - 2.5, y - 2), (x + 2.5, y + 2), (x + 2.5, y - 2), (x - 2.5, y + 2)])
        put(img, bow, ('P' if k == 2 else 'B'), 'p', 'h')
    return img


def draw_trottola(img, cx, top, s, cols=('P', 'Y', 'T', 'L'), face=False):
    h, w = img.shape[:2]
    put(img, R(w, h, cx - 1.5 * s, top, cx + 1.5 * s, top + 4 * s, 1), 'X', 'r', 'R')
    upper = P(w, h, [(cx - 10 * s, top + 12 * s), (cx + 10 * s, top + 12 * s), (cx + 5 * s, top + 4 * s), (cx - 5 * s, top + 4 * s)])
    lower = P(w, h, [(cx - 10 * s, top + 12 * s), (cx + 10 * s, top + 12 * s), (cx + 1.5 * s, top + 21 * s), (cx - 1.5 * s, top + 21 * s)])
    body = upper | lower
    put(img, body, 'W', 'w', 'f')
    yy, xx = np.mgrid[0:h, 0:w]
    band = np.floor((xx - cx + (yy - top - 12 * s) * 0.9) / (3.0 * s)).astype(int) % 4
    inner = body & ~(dilate4(~body))
    for k, c in enumerate(cols):
        paint(img, inner & (band == k), c)
    put(img, E(w, h, cx, top + 12 * s, 10.5 * s, 1.8 * s), 'h', 'P', 'H')
    put(img, R(w, h, cx - 1, top + 20 * s, cx + 1, top + 23 * s), 'S', 's', 'x')


def pic_trottola():
    img = blank(S36, S36)
    draw_trottola(img, 18, 3, 1.4)
    return img


def pic_robot():
    img = blank(S36, S36)
    w = h = S36
    tline(img, 18, 1, 18, 4, 'k')
    put(img, E(w, h, 18, 2, 1.8, 1.8), 'X', 'r', 'R')
    for sx in (-1, 1):   # arms
        put(img, worm(w, h, [(18 + sx * 8, 20), (18 + sx * 12, 24), (18 + sx * 12, 27)], 1.8, 1.8), 'S', 's', 'x')
        put(img, E(w, h, 18 + sx * 12, 28, 2.2, 2), 'B', 'b', 'n')
    for x in (14, 22):   # legs
        put(img, R(w, h, x - 2, 29, x + 2, 34), 'S', 's', 'x')
    head = R(w, h, 9, 4, 27, 15, 3)
    put(img, head, 'B', 'b', 'n')
    body = R(w, h, 10, 16, 26, 30, 2)
    put(img, body, 'B', 'b', 'n')
    for ex in (14, 22):
        put(img, E(w, h, ex, 9, 2.6, 2.6), 'w', 'w', 's')
        paint(img, R(w, h, ex, 9, ex + 1, 10), 'k')
    paint(img, R(w, h, 15, 12, 21, 12), 'k')
    for (x, c) in ((14, 'Y'), (18, 'X'), (22, 'G')):
        paint(img, E(w, h, x, 22, 1.4, 1.4), c)
    return img


def pic_dinosauro():
    img = blank(S36, S36)
    w = h = S36
    for (x, y) in ((9, 14), (13, 11), (17, 10), (21, 11)):   # yellow spikes on the back
        put(img, P(w, h, [(x - 2.5, y + 3), (x, y - 2), (x + 2.5, y + 3)]), 'Y', 'y', 'o')
    put(img, worm(w, h, [(15, 22), (8, 25), (2, 22)], 5, 1.5), 'G', 'g', 'd')   # the tail
    for x in (14, 21):
        put(img, R(w, h, x - 2, 26, x + 2, 33, 1), 'G', 'g', 'd')
    put(img, E(w, h, 17, 20, 9, 8), 'G', 'g', 'd')            # the body
    paint(img, E(w, h, 18, 23, 5, 4), 'y')
    put(img, worm(w, h, [(22, 14), (25, 8)], 4, 4), 'G', 'g', 'd')   # the neck
    put(img, E(w, h, 28, 8, 7, 5.5), 'G', 'g', 'd')            # the head
    put(img, E(w, h, 29, 6, 2.2, 2.4), 'w', 'w', 's')
    paint(img, R(w, h, 30, 6, 30, 7), 'k')
    tline(img, 29, 11, 33, 10, 'k')
    px(img, 26, 10, 'c')
    put(img, worm(w, h, [(24, 18), (27, 20)], 1.4, 1.4), 'G', 'g', 'd')   # a little arm
    return img


def pic_tamburo():
    img = blank(S36, S36)
    w = h = S36
    for (a, b) in (((6, 3), (17, 12)), ((30, 3), (19, 12))):   # the sticks
        put(img, worm(w, h, [a, b], 1.2, 1.2), 'F', 'f', 'O')
        put(img, E(w, h, a[0], a[1], 2.2, 2.2), 'W', 'w', 's')
    body = R(w, h, 5, 16, 31, 32, 2) | E(w, h, 18, 32, 13, 3)
    put(img, body, 'X', 'r', 'R')
    top = E(w, h, 18, 16, 13, 4)
    put(img, top, 'W', 'w', 's')
    for k in range(5):   # the zig-zag cords
        x0 = 7 + k * 5.5
        tline(img, x0, 20, x0 + 2.8, 29, 'Y')
        tline(img, x0 + 2.8, 29, x0 + 5.5, 20, 'Y')
    paint(img, R(w, h, 5, 18, 31, 19) & body, 'Y')
    paint(img, R(w, h, 5, 30, 31, 31) & body, 'Y')
    return img


# ------------------------------------------------------------------ things to weigh (36x36)
def pic_piuma():
    img = blank(S36, S36)
    w = h = S36
    vane = worm(w, h, [(8, 31), (14, 22), (21, 13), (29, 5)], 1.5, 4.0, steps=12)
    vane |= worm(w, h, [(12, 25), (18, 17), (25, 9)], 4.5, 3.5, steps=12)
    put(img, vane, 'p', 'W', 'P')
    for k in range(7):   # the barbs
        x, y = 11 + k * 2.6, 26 - k * 3.0
        tline(img, x, y, x + 3, y + 2, 'P')
    tline(img, 5, 34, 28, 6, 'h')   # the quill
    return img


def pic_foglia():
    img = blank(S36, S36)
    w = h = S36
    leaf = P(w, h, [(5, 30)] + [(5 + 26 * t + 7 * np.sin(np.pi * t), 30 - 26 * t + 7 * np.sin(np.pi * t)) for t in np.linspace(0, 1, 12)]
             + [(5 + 26 * t - 7 * np.sin(np.pi * t), 30 - 26 * t - 7 * np.sin(np.pi * t)) for t in np.linspace(1, 0, 12)])
    put(img, leaf, 'G', 'g', 'd')
    tline(img, 3, 33, 30, 5, 'd')
    for k in range(4):   # veins
        x, y = 10 + k * 5, 25 - k * 5
        tline(img, x, y, x + 4, y + 1, 'd')
        tline(img, x, y, x - 1, y - 4, 'd')
    return img


def pic_libro():
    img = blank(S36, S36)
    w = h = S36
    pages = P(w, h, [(8, 12), (30, 8), (30, 26), (8, 31)])
    put(img, pages, 'W', 'w', 's')
    for y in range(13, 30, 3):
        tline(img, 9, y + 1, 29, y - 3 + 1, 's')
    cover = P(w, h, [(4, 10), (26, 6), (26, 25), (4, 29)])
    put(img, cover, 'B', 'b', 'n')
    spine = P(w, h, [(4, 10), (7, 10), (7, 30), (4, 29)])
    put(img, spine, 'n', 'B', 'N')
    put(img, star_mask_px(w, h, 16, 17, 6, 2.6), 'Y', 'y', 'o')
    return img


def pic_anguria():
    img = blank(S36, S36)
    w = h = S36
    m = E(w, h, 18, 19, 16.5, 13)
    put(img, m, 'G', 'g', 'd')
    yy, xx = np.mgrid[0:h, 0:w]
    for k in range(-3, 4):   # dark wavy stripes
        x0 = 18 + k * 5
        stripe = np.abs(xx - x0 - 1.4 * np.sin(yy * 0.9)) < 1.2
        paint(img, m & stripe & ~(dilate4(~m)), 'd')
    paint(img, E(w, h, 11, 12, 4, 2.6) & m, 'g')
    put(img, R(w, h, 17, 4, 19, 7), 'O', 'o', 'k')
    return img


def pic_masso():
    img = blank(S36, S36)
    w = h = S36
    rock = P(w, h, [(3, 32), (2, 22), (7, 12), (15, 6), (25, 7), (32, 14), (34, 24), (32, 32)])
    put(img, rock, '2', '3', '1')
    paint(img, P(w, h, [(8, 14), (15, 8), (22, 9), (13, 15)]), '3')
    paint(img, P(w, h, [(24, 22), (31, 25), (30, 31), (22, 31)]), '1')
    for (a, b) in (((12, 20), (17, 25)), ((17, 25), (16, 30)), ((24, 12), (27, 17))):
        tline(img, a[0], a[1], b[0], b[1], '1')
    paint(img, E(w, h, 9, 30, 4, 2) & rock, '6')   # a little moss
    return img


def pic_zucca():
    img = blank(S36, S36)
    w = h = S36
    for (x, rx) in ((10, 8), (26, 8), (18, 9)):   # the ribs
        put(img, E(w, h, x, 21, rx, 12), '@', 'F', 'O')
    paint(img, E(w, h, 14, 15, 2.4, 3) & E(w, h, 18, 21, 9, 12), 'F')
    put(img, worm(w, h, [(18, 10), (19, 6), (22, 4)], 2.2, 1.6), 'G', 'g', 'd')
    paint(img, E(w, h, 25, 8, 3.4, 2), 'g')
    return img


# ------------------------------------------------------------------ the things to measure
SIZES = [12, 19, 26, 33, 40]         # big/small: the height (the width follows)
LENGTHS = [12, 20, 28, 36, 44]       # long/short
HEIGHTS = [12, 20, 28, 36, 44]       # tall/low
FILL_TOP, FILL_BOT = 8, 36           # the containers (32x40): what is inside reaches FILL_BOT - 7 * v


def mis_pallone(v):
    d = SIZES[v]
    img = blank(d, d)
    c = d / 2.0
    m = E(d, d, c, c, c - 1, c - 1)
    put(img, m, 'r', 'P', 'R')
    yy, xx = np.mgrid[0:d, 0:d]
    a = np.arctan2(yy + 0.5 - c, xx + 0.5 - c)
    seg = np.floor((a + np.pi) / (np.pi / 3)).astype(int) % 3
    inner = m & ~(dilate4(~m))
    paint(img, inner & (seg == 0), 'X')
    paint(img, inner & (seg == 1), 'W')
    paint(img, inner & (seg == 2), 'B')
    paint(img, E(d, d, c, c, max(1.2, d * 0.12), max(1.2, d * 0.12)), 'Y')
    paint(img, E(d, d, c - d * 0.22, c - d * 0.24, max(0.8, d * 0.08), max(0.8, d * 0.06)) & inner, 'w')
    return img


def mis_orsetto(v):
    h = SIZES[v] + 2
    w = int(round(h * 0.86)) + 2
    img = blank(w, h)
    draw_orsetto(img, h - 2, w / 2.0, h - 1)
    return img


def mis_regalo(v):
    S = SIZES[v]
    w = int(round(S * 0.92)) + 1
    img = blank(w, S)
    box = R(w, S, 1.5, S * 0.36, w - 2.5, S - 2, 1)
    put(img, box, 'h', 'P', 'H')
    lid = R(w, S, 0.5, S * 0.24, w - 1.5, S * 0.4, 1)
    put(img, lid, 'P', 'p', 'h')
    rib = R(w, S, w / 2 - max(1, S * 0.06), S * 0.24, w / 2 + max(1, S * 0.06) - 1, S - 2)
    paint(img, rib & (box | lid), 'Y')
    for sx in (-1, 1):
        loop = E(w, S, w / 2 + sx * S * 0.16, S * 0.15, S * 0.15 + 0.4, S * 0.11 + 0.4)
        put(img, loop, 'Y', 'y', 'o')
    put(img, E(w, S, w / 2, S * 0.19, S * 0.07 + 0.5, S * 0.07 + 0.5), 'o', 'Y', 'O')
    return img


def mis_pastello(v):
    L, h = LENGTHS[v], 9
    img = blank(L, h)
    tip = P(L, h, [(L - 7, 1), (L - 1, 4.5), (L - 7, 8)])
    put(img, tip, 'F', 'f', 'O')
    paint(img, P(L, h, [(L - 3.5, 3), (L - 1, 4.5), (L - 3.5, 6)]), 'n')
    body = R(L, h, 1, 1, L - 7, 7)
    put(img, body, 'B', 'b', 'n')
    paper = R(L, h, 3, 1, max(4, L - 10), 7)
    paint(img, paper & body & ~(dilate4(~body)), 'n')
    for x in (4, max(5, L - 11)):
        paint(img, R(L, h, x, 2, x, 6), 'b')
    return img


def mis_serpente(v):
    L, h = LENGTHS[v], 12
    img = blank(L, h)
    pts = [(1.5 + t * (L - 8), 6 + 2.2 * np.sin(t * (L - 8) / 4.5)) for t in np.linspace(0, 1, max(4, L // 3))]
    body = worm(L, h, pts, 1.6, 2.6, steps=6)
    put(img, body, 'G', 'g', 'd')
    for (x, y) in pts[1::2]:
        paint(img, E(L, h, x, y, 1.1, 1.1) & body & ~(dilate4(~body)), 'Y')
    hx, hy = L - 5, pts[-1][1]
    head = E(L, h, hx, hy, 3.6, 3.4)
    put(img, head, 'G', 'g', 'd')
    px(img, int(hx + 1), int(hy - 1), 'k')
    px(img, int(hx + 3), int(hy + 2), 'r')
    return img


def mis_bruco(v):
    L, h = [18, 24, 31, 37, 44][v], 13
    img = blank(L, h)
    n = v + 1
    step = (L - 12) / max(1, n)
    for k in range(n):   # the body, tail first
        x = 5 + k * step
        seg = E(L, h, x, 8, 4.4, 4.2)
        put(img, seg, 'G' if k % 2 else 'g', 'W', 'd')
        paint(img, R(L, h, x - 1, 12, x, 12), 'd')
    hx = L - 6.5
    head = E(L, h, hx, 7, 5.4, 5.2)
    put(img, head, 'Y', 'y', 'o')
    for (dx, c) in ((-2, 'k'), (2, 'k')):
        px(img, int(hx + dx), 6, c)
    px(img, int(hx), 9, 'k')
    px(img, int(hx - 4), 8, 'c')
    px(img, int(hx + 4), 8, 'c')
    for sx in (-1, 1):   # antennae
        tline(img, hx + sx * 2, 2, hx + sx * 3, 0, 'k')
    return img


def mis_albero(v):
    h = HEIGHTS[v]
    c = min(22, max(8, int(h * 0.55)))
    w = c + 2
    img = blank(w, h)
    trunk = R(w, h, w / 2 - max(1, c * 0.12), c * 0.7, w / 2 + max(1, c * 0.12), h - 1)
    put(img, trunk, 'J', 'j', 'q')
    crown = E(w, h, w / 2, c / 2.0 + 0.5, c / 2.0, c / 2.0)
    put(img, crown, 'G', 'g', 'd')
    if c > 12:
        for (dx, dy) in ((-0.2, -0.15), (0.18, 0.1), (-0.05, 0.25)):
            paint(img, E(w, h, w / 2 + dx * c, c / 2 + dy * c, 1.2, 1.2), 'r')
    return img


def mis_girasole(v):
    h = HEIGHTS[v]
    w = 14
    img = blank(w, h)
    stem = R(w, h, 6, 10, 7, h - 1)
    put(img, stem, 'G', 'g', 'd')
    if h > 20:
        for (y, sx) in ((int(h * 0.6), -1), (int(h * 0.78), 1)):
            leaf = E(w, h, 6.5 + sx * 3.4, y, 3, 1.6)
            put(img, leaf, 'G', 'g', 'd')
    for k in range(10):   # petals
        a = k * np.pi / 5
        put(img, E(w, h, 7 + np.cos(a) * 4, 6 + np.sin(a) * 4, 2, 2), 'Y', 'y', 'o')
    put(img, E(w, h, 7, 6, 2.6, 2.6), 'J', 'j', 'q')
    return img


BLOCK_COLS = [('X', 'r', 'R'), ('Y', 'y', 'o'), ('B', 'b', 'n'), ('G', 'g', 'd'), ('P', 'p', 'h')]


def mis_castello(v):
    n = v + 1
    h, w = 8 * n + 4, 12
    img = blank(w, h)
    for k in range(n):   # the blocks, from the bottom
        y1 = h - 1 - k * 8
        put(img, R(w, h, 1, y1 - 7, 10, y1), *BLOCK_COLS[k % 5])
        px(img, 2, y1 - 6, 'w')
    for x in (1, 5, 8):   # crenellations
        put(img, R(w, h, x, 0, x + 2, 3), *BLOCK_COLS[(n + x) % 5])
    return img


def _level(v):
    return FILL_BOT - (FILL_BOT - FILL_TOP) * v // 4


def mis_bicchiere(v):
    w, h = 32, 40
    img = blank(w, h)
    glass = P(w, h, [(5, 4), (27, 4), (24.5, 39), (7.5, 39)])
    put(img, glass, 'b', 'w', 'B', ol='n')
    inner = P(w, h, [(7, 5), (25, 5), (23, 36), (9, 36)])
    paint(img, inner, '#eef8ff')
    yy = np.mgrid[0:h, 0:w][0]
    juice = inner & (yy >= _level(v))
    if v > 0:
        paint(img, juice, '#ffa040')
        paint(img, juice & (yy == _level(v)), '#ffd080')
        if v >= 2:   # a straw
            tline(img, 20, 1, 15, _level(v) + 6, 'P', 2)
    paint(img, R(w, h, 9, 8, 10, 30) & inner, 'w')   # the glint of the glass
    return img


def mis_barattolo(v):
    w, h = 32, 40
    img = blank(w, h)
    jar = R(w, h, 4, 6, 27, 38, 4)
    put(img, jar, 'b', 'w', 'B', ol='n')
    inner = R(w, h, 6, 7, 25, 36, 3)
    paint(img, inner, '#eef8ff')
    lid = R(w, h, 5, 1, 26, 6, 1)
    put(img, lid, 'X', 'r', 'R')
    if v > 0:   # round candies, in rows, up to the level
        cols = ['h', 'Y', 'B', 'G', 'v', 'r', 'T']
        lvl = _level(v)
        row = 0
        y = 33
        while y - 2 >= lvl:
            for i, x in enumerate(range(9 + (row % 2) * 2, 24, 5)):
                c = E(w, h, x + 0.5, y + 0.5, 2.7, 2.5) & inner
                paint(img, c, cols[(i * 3 + row * 2) % len(cols)])
            y -= 4
            row += 1
    paint(img, R(w, h, 8, 10, 8, 30) & inner, 'w')
    return img


def mis_acquario(v):
    w, h = 32, 40
    img = blank(w, h)
    bowl = E(w, h, 16, 24, 15, 15) | R(w, h, 7, 6, 25, 12)
    bowl &= np.mgrid[0:h, 0:w][0] >= 6
    put(img, bowl, 'b', 'w', 'B', ol='n')
    inner = (E(w, h, 16, 24, 13, 13) | R(w, h, 9, 7, 23, 12)) & (np.mgrid[0:h, 0:w][0] >= 7)
    paint(img, inner, '#eef8ff')
    yy = np.mgrid[0:h, 0:w][0]
    if v > 0:
        water = inner & (yy >= _level(v))
        paint(img, water, '#8fd0ff')
        paint(img, water & (yy == _level(v)), '#cdeeff')
        if v >= 2:   # a little fish
            fy = max(_level(v) + 5, 26)
            fish = E(w, h, 15, fy, 4, 2.6) | P(w, h, [(19, fy), (23, fy - 3), (23, fy + 3)])
            put(img, fish, '@', 'F', 'O')
            px(img, 13, fy - 1, 'k')
    put(img, R(w, h, 12, 36, 20, 39, 1), 'S', 's', 'x')   # the stand
    paint(img, R(w, h, 7, 16, 7, 26) & inner, 'w')
    return img


MIS_KINDS = {'pallone': mis_pallone, 'orsetto': mis_orsetto, 'regalo': mis_regalo, 'pastello': mis_pastello,
             'serpente': mis_serpente, 'bruco': mis_bruco, 'albero': mis_albero, 'girasole': mis_girasole,
             'castello': mis_castello, 'bicchiere': mis_bicchiere, 'barattolo': mis_barattolo,
             'acquario': mis_acquario}


# ------------------------------------------------------------------ the bubble icons (30x24)
def _icon():
    return blank(30, 24)


def _hollow(img, m):
    ring = dilate4(m) & ~m
    paint(img, ring, 'x')
    paint(img, m & ~(dilate4(~m)), 'S')


def ico_cerchi(big_wanted):
    img = _icon()
    w, h = 30, 24
    small, big = E(w, h, 7, 16, 3.4, 3.4), E(w, h, 20, 12, 8.4, 8.4)
    want, other = (big, small) if big_wanted else (small, big)
    _hollow(img, other)
    put(img, want, 'h', 'P', 'H')
    return img


def ico_barre(long_wanted):
    img = _icon()
    w, h = 30, 24
    short, long_ = R(w, h, 3, 4, 13, 8, 2), R(w, h, 3, 14, 26, 18, 2)
    want, other = (long_, short) if long_wanted else (short, long_)
    _hollow(img, other)
    put(img, want, 'h', 'P', 'H')
    return img


def ico_torri(tall_wanted):
    img = _icon()
    w, h = 30, 24
    low, tall = R(w, h, 5, 14, 11, 21, 2), R(w, h, 17, 2, 23, 21, 2)
    want, other = (tall, low) if tall_wanted else (low, tall)
    _hollow(img, other)
    put(img, want, 'h', 'P', 'H')
    paint(img, R(w, h, 2, 22, 27, 22), 'S')   # the floor line
    return img


def ico_bicchiere(full):
    img = _icon()
    w, h = 30, 24
    glass = P(w, h, [(8, 2), (22, 2), (20, 22), (10, 22)])
    put(img, glass, 'b', 'w', 'B', ol='n')
    inner = P(w, h, [(9.5, 3), (20.5, 3), (19, 21), (11, 21)])
    paint(img, inner, '#eef8ff')
    if full:
        paint(img, inner & (np.mgrid[0:h, 0:w][0] >= 4), 'h')
        paint(img, inner & (np.mgrid[0:h, 0:w][0] == 4), 'P')
    return img


def ico_peso(heavy):
    img = _icon()
    w, h = 30, 24
    if heavy:
        put(img, E(w, h, 15, 7, 4.5, 4) & ~E(w, h, 15, 7, 2, 1.6), 'x', 'S', 'k')
        put(img, P(w, h, [(7, 22), (23, 22), (20, 9), (10, 9)]), 'x', 'S', 'k')
    else:
        feather = worm(w, h, [(7, 21), (15, 12), (24, 3)], 1.5, 3.4)
        put(img, feather, 'p', 'W', 'P')
        tline(img, 5, 23, 24, 3, 'h')
    return img


# ------------------------------------------------------------------ the spring scales and the shelf
def spira():
    w, h = 14, 3
    img = blank(w, h)
    paint(img, E(w, h, 7, 1.5, 7, 1.5) & ~E(w, h, 7, 1.0, 5, 0.6), 'S')
    paint(img, R(w, h, 2, 2, 11, 2) & E(w, h, 7, 1.5, 7, 1.5), 'x')
    paint(img, R(w, h, 3, 0, 10, 0) & E(w, h, 7, 1.5, 7, 1.5), 's')
    return img


def gancio():
    w, h = 6, 7
    img = blank(w, h)
    for (x, y) in ((3, 0), (3, 1), (3, 2), (3, 3), (2, 4), (1, 5), (2, 6), (3, 6), (4, 5), (4, 4)):
        px(img, x, y, 'x')
    return img


def piatto():
    w, h = 40, 5
    img = blank(w, h)
    put(img, E(w, h, 20, 2.5, 19, 1.8), 'Y', 'y', 'o')
    return img


def mensola():
    w, h = 200, 8
    img = blank(w, h)
    plank = R(w, h, 0, 0, w - 1, 4, 1)
    put(img, plank, '#c98f58', '#e0ad76', '#8a5a30')
    for x in (20, w - 26):   # brackets
        put(img, P(w, h, [(x, 4), (x + 6, 4), (x + 3, 7)]), '#a8703e', '#c98f58', '#7a4a26')
    return img


# ------------------------------------------------------------------ menu icons (44x44)
def menu_negozio():
    img = blank(44, 44)
    w = h = 44
    counter = R(w, h, 4, 30, 39, 40, 1)
    put(img, counter, '#a8703e', '#c98f58', '#7a4a26')
    paint(img, R(w, h, 5, 30, 38, 31), '#e0ad76')
    awn = R(w, h, 2, 4, 41, 12)
    for k in range(5):   # the scalloped edge
        awn |= E(w, h, 6 + k * 8, 12, 4, 3)
    put(img, awn, 'h', 'P', 'H')
    stripes = (np.floor((np.mgrid[0:h, 0:w][1] - 2) / 4) % 2) == 0
    paint(img, awn & stripes & ~(dilate4(~awn)), 'W')
    for x in (6, 37):   # the posts
        paint(img, R(w, h, x, 15, x + 1, 30), 'q')
    blit(img, coin(5), 9, 17)
    blit(img, pic_mini_teddy(), 24, 13)
    return img


def pic_mini_teddy():
    img = blank(16, 18)
    draw_orsetto(img, 16, 8, 17)
    return img


def menu_misure():
    img = blank(44, 44)
    w = h = 44
    ruler = R(w, h, 2, 34, 41, 40, 1)
    put(img, ruler, 'Y', 'y', 'o')
    for x in range(5, 40, 3):
        tline(img, x, 34, x, 35 + (1 if (x - 5) % 6 else 3), 'O')
    for k in range(3):   # a tall tower
        put(img, R(w, h, 24, 25 - k * 9, 32, 33 - k * 9), *BLOCK_COLS[k])
    put(img, R(w, h, 9, 25, 17, 33), *BLOCK_COLS[3])   # a short one
    for (x0, y0, x1, y1) in ((36, 6, 36, 32),):   # the measuring arrow
        tline(img, x0, y0, x1, y1, 'h')
        tline(img, x0 - 2, y0 + 2, x0, y0, 'h')
        tline(img, x0 + 2, y0 + 2, x0, y0, 'h')
        tline(img, x0 - 2, y1 - 2, x0, y1, 'h')
        tline(img, x0 + 2, y1 - 2, x0, y1, 'h')
    return img


# ------------------------------------------------------------------ backgrounds
def _canvas():
    return B.canvas()


def _shelf_toys(img, x0, x1, base, rng):
    """A row of little toys on a shelf: blocks, balls, teddies, cars."""
    x = x0
    while x < x1 - 8:
        kind = int(rng.integers(0, 4))
        c = ['#e5395a', '#ffc93a', '#5a9cf0', '#4cc25a', '#ff7ab8', '#a06ae0'][int(rng.integers(0, 6))]
        if kind == 0:   # a block
            b = rect(x, base - 8, x + 7, base - 1)
            outline(img, b, INK)
            spaint(img, b, c)
            spaint(img, rect(x, base - 8, x + 7, base - 8), '#ffffff')
            x += 10
        elif kind == 1:   # a ball
            b = ellipse(x + 4, base - 4, 4, 4)
            outline(img, b, INK)
            spaint(img, b, c)
            spaint(img, ellipse(x + 3, base - 6, 1.2, 1), '#ffffff')
            x += 10
        elif kind == 2:   # a teddy
            t = ellipse(x + 4, base - 3, 4, 3.4) | ellipse(x + 4, base - 9, 3.4, 3) | ellipse(x + 1.5, base - 12, 1.4, 1.4) | \
                ellipse(x + 6.5, base - 12, 1.4, 1.4)
            outline(img, t, INK)
            spaint(img, t, '#c98f58')
            x += 11
        else:   # a little car
            car = rect(x, base - 6, x + 11, base - 3) | rect(x + 3, base - 9, x + 8, base - 6)
            outline(img, car, INK)
            spaint(img, car, c)
            for wx in (x + 2, x + 9):
                spaint(img, ellipse(wx, base - 2, 1.6, 1.6), INK)
            x += 15


def negozio_bg():
    img = _canvas()
    # the wall: soft yellow wallpaper with stripes
    stripes = (XX // 10) % 2
    tone_fill(img, YY < 196, 0.5 + stripes * 0.25 + (YY / 196.0) * 0.15, ['#fbe6b0', '#fdeec4', '#fff6dc'])
    # shelves of toys at the sides
    rng = np.random.default_rng(21)
    for (sx0, sx1) in ((0, 70), (306, 320)):
        case = rect(sx0, 18, sx1, 104)
        outline(img, case, INK)
        tone_fill(img, case, 0.5, ['#7a4a26', '#8a5a30'])
        for (y0, y1) in ((22, 44), (48, 72), (76, 100)):
            spaint(img, rect(sx0 + 3, y0, sx1 - 3, y1), '#f4dca8')
            _shelf_toys(img, sx0 + 4, sx1 - 2, y1, rng)
            sh = rect(sx0, y1, sx1, y1 + 3)
            outline(img, sh, INK)
            tone_fill(img, sh, 0.6, ['#a8703e', '#c98f58'])
    # the awning of the shop across the top
    awn = rect(0, 0, W - 1, 12)
    for k in range(0, W + 16, 16):
        awn |= ellipse(k + 8, 12, 8, 5)
    outline(img, awn, INK)
    st = ((XX // 8) % 2) == 0
    spaint(img, awn & st, '#ff7ab8')
    spaint(img, awn & ~st, '#fff4fa')
    # the counter: its top (the quiz panel) and its front
    top = rect(64, 104, W - 1, 170)
    outline(img, top, INK)
    tone_fill(img, top, 0.45 + (YY - 104) / 66.0 * 0.4, ['#d8a26a', '#e6b67e', '#f2cc98', '#f8dcb0'])
    for y in range(110, 170, 9):   # the grain of the wood
        line(img, [(66, y), (W - 1, y + 1)], '#e0ad76', 0.3, mask=top)
    spaint(img, rect(64, 104, W - 1, 105), '#c98f58')
    felt = rect(92, 122, 292, 160)   # a green felt mat where the coins go
    outline(img, felt, '#2a6a3a')
    tone_fill(img, felt, 0.5 + (YY - 122) / 38.0 * 0.2, ['#3e9a5a', '#4cad66', '#58bb72'])
    speckle(img, felt, '#6ac884', 0.04, 5)
    for x in range(96, 292, 6):
        spaint(img, rect(x, 124, x + 2, 124) | rect(x, 158, x + 2, 158), '#8ad8a0')
    front = rect(64, 171, W - 1, 196)
    outline(img, front, INK)
    tone_fill(img, front, 0.5, ['#8a5a30', '#a8703e'])
    for x in range(70, W, 42):   # panels on the front
        p = rect(x, 175, x + 34, 192)
        outline(img, p, '#6e4526')
        tone_fill(img, p, 0.55, ['#a8703e', '#c98f58'])
    spaint(img, rect(64, 171, W - 1, 172), '#ffd23f')
    # the floor: chequered pink tiles
    floor = YY >= 196
    rows = (YY - 196) // 11
    k = ((XX + rows * 12) // 24 + rows) % 2
    spaint(img, floor & (k == 0), '#ffd6e6')
    spaint(img, floor & (k == 1), '#ffeef4')
    spaint(img, floor & (((YY - 196) % 11) == 10), '#f0b8cc')
    spaint(img, rect(0, 196, W - 1, 196), INK)
    # a hanging sign with a coin, over the left shelves
    return img


def _giraffe_chart(img, x0, y0, y1):
    """The growth chart on the wall: a tall friendly giraffe with marks on its neck."""
    neck = rect(x0 + 4, y0 + 18, x0 + 14, y1)
    head = ellipse(x0 + 12, y0 + 12, 10, 8)
    ears = ellipse(x0 + 4, y0 + 4, 3, 2) | ellipse(x0 + 20, y0 + 4, 3, 2)
    horns = rect(x0 + 8, y0, x0 + 9, y0 + 5) | rect(x0 + 15, y0, x0 + 16, y0 + 5)
    m = neck | head | ears | horns
    outline(img, m, INK)
    spaint(img, m, '#ffd23f')
    for (cx, cy, r) in ((x0 + 7, y0 + 40, 2.4), (x0 + 11, y0 + 62, 2.8), (x0 + 7, y0 + 90, 2.2), (x0 + 11, y0 + 118, 2.6),
                        (x0 + 8, y0 + 146, 2.4), (x0 + 12, y0 + 10, 2)):
        spaint(img, ellipse(cx, cy, r, r) & m, '#e09a2a')
    for y in range(y0 + 26, y1, 6):   # the marks
        spaint(img, rect(x0 + 12 if (y - y0) % 24 else x0 + 9, y, x0 + 14, y), '#8a5a30')
    for ex in (x0 + 9, x0 + 15):
        spaint(img, rect(ex, y0 + 10, ex, y0 + 11), INK)
    spaint(img, rect(x0 + 10, y0 + 16, x0 + 14, y0 + 16), INK)
    spaint(img, ellipse(x0 + 18, y0 + 14, 1.4, 1), '#ff93c6')


def misure_bg():
    img = _canvas()
    # the playroom wall: mint with little dots, a wainscot of soft panels
    tone_fill(img, YY < 196, 0.45 + (YY / 196.0) * 0.3, ['#cfeee4', '#d9f3ea', '#e4f7f0'])
    dots = (((XX // 12) + (YY // 12)) % 2 == 0) & (XX % 12 == 6) & (YY % 12 == 6) & (YY < 150)
    spaint(img, dots, '#bfe6da')
    wains = (YY >= 168) & (YY < 196)
    tone_fill(img, wains, 0.5 + (YY - 168) / 28.0 * 0.3, ['#ffcce2', '#ffd8e8', '#ffe6f0'])
    spaint(img, rect(0, 168, W - 1, 169), '#e8a8c8')
    for x in range(0, W, 32):
        spaint(img, wains & (XX == x), '#f0b8d0')
    # the giraffe growth chart on the left
    _giraffe_chart(img, 12, 24, 190)
    # a window with clouds on the right, and a shelf with toys under it
    win = rect(282, 20, 318, 76)
    glass = rect(286, 24, 314, 72)
    outline(img, win, INK)
    tone_fill(img, win & ~glass, 0.6, ['#a8703e', '#c98f58', '#e0ad76'])
    tmp = _canvas()
    B.gradient(tmp, 24, 73, ['#8fd0ff', '#afdeff', '#cfeeff'])
    img[glass, :3] = tmp[glass, :3]
    cloud(img, [(294, 44, 5, 3.5), (301, 41, 6, 4.5), (308, 45, 4, 3)], ['#e6f2ff', '#ffffff'])
    spaint(img, glass & ((XX == 300) | (YY == 48)), '#a8703e')
    sh = rect(280, 120, 320, 123)
    outline(img, sh, INK)
    tone_fill(img, sh, 0.6, ['#a8703e', '#c98f58'])
    _shelf_toys(img, 284, 318, 120, np.random.default_rng(31))
    # the floor: warm boards and a round rug
    floor = YY >= 196
    for y0 in range(196, H, 11):
        k = ((y0 - 196) // 11) % 2
        img[(YY >= y0) & (YY < y0 + 11), :3] = B.hexrgb('#c8986a') if k else B.hexrgb('#b8885c')
    for x in range(0, W, 30):
        spaint(img, floor & (XX == x + ((YY - 196) // 11 % 2) * 15), '#9a6a40')
    spaint(img, rect(0, 196, W - 1, 196), INK)
    rug = ellipse(170, 222, 130, 15)
    outline(img, rug, INK)
    spaint(img, rug, '#b376ec')
    for k, c in enumerate(['#ffd23f', '#4fd6c0', '#ff93c6']):
        spaint(img, ellipse(170, 222, 124 - k * 12, 13 - k * 1.8) & ~ellipse(170, 222, 118 - k * 12, 11.4 - k * 1.8), c)
    return img


def all_backgrounds():
    import places13 as P13   # 0.13.0: the room of the measures painted again
    return {'bg_negozio': negozio_bg(), 'bg_misure': P13.misure()}


def all_sprites():
    out = {'menu_negozio': menu_negozio(), 'menu_misure': menu_misure(),
           'coin_1': coin(1), 'coin_2': coin(2), 'coin_5': coin(5), 'neg_pallino': pallino(),
           'neg_cartello': cartello(), 'neg_cassa': cassa(False), 'neg_cassa_aperta': cassa(True),
           'neg_borsellino': borsellino(),
           'pic_trenino': pic_trenino(), 'pic_orsetto': pic_orsetto(), 'pic_aquilone': pic_aquilone(),
           'pic_trottola': pic_trottola(), 'pic_robot': pic_robot(), 'pic_dinosauro': pic_dinosauro(),
           'pic_tamburo': pic_tamburo(),
           'pic_piuma': pic_piuma(), 'pic_foglia': pic_foglia(), 'pic_libro': pic_libro(),
           'pic_anguria': pic_anguria(), 'pic_masso': pic_masso(), 'pic_zucca': pic_zucca(),
           'mis_spira': spira(), 'mis_gancio': gancio(), 'mis_piatto': piatto(), 'mis_mensola': mensola(),
           'mis_ico_grande': ico_cerchi(True), 'mis_ico_piccolo': ico_cerchi(False),
           'mis_ico_lungo': ico_barre(True), 'mis_ico_corto': ico_barre(False),
           'mis_ico_alto': ico_torri(True), 'mis_ico_basso': ico_torri(False),
           'mis_ico_pieno': ico_bicchiere(True), 'mis_ico_vuoto': ico_bicchiere(False),
           'mis_ico_pesante': ico_peso(True), 'mis_ico_leggero': ico_peso(False)}
    for name, fn in MIS_KINDS.items():
        for v in range(5):
            out['mis_%s_%d' % (name, v)] = fn(v)
    return out
