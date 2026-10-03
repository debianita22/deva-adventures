"""Art of the third wave of games (0.6): Ginnastica, Trucca i mostri, Forme e colori.

Backgrounds (320x240): the gym, the make-up salon with its big mirror, the
wizard's laboratory. Sprites: the four gym moves as pictograms, a trampoline,
the menu icons, make-up drawn on the monsters' faces, the shapes as masks
(the runtime paints them in any colour), the cauldron and a speech bubble.
All drawn from scratch with the palette of pixel.py.
"""
import math

import numpy as np

import backgrounds as B
from objects import poly_mask, star_mask_px
from pixel import (blank, blit, ellipse_mask, rect_mask, capsule_mask, shaded_part, rgb, dilate4,
                   crop)

W, H = 320, 240


def put(img, mask, base, light, shadow, outline='k'):
    blit(img, shaded_part(mask, base, light, shadow, outline), 0, 0)


def paint(img, mask, c):
    img[mask, :3] = rgb(c) if isinstance(c, str) else c
    img[mask, 3] = 255


# ------------------------------------------------------------------ backgrounds
def _wall(img, y1, top, bottom):
    B.gradient(img, 0, y1, [top, bottom])


def _wood_floor(img, y0, c1, c2, line):
    for y in range(y0, H):
        k = ((y - y0) // 7) % 2
        img[y, :, :3] = B.hexrgb(c1 if k == 0 else c2)
        if (y - y0) % 7 == 6:
            img[y, :, :3] = B.hexrgb(line)
    for row in range((H - y0) // 7 + 1):
        y = y0 + row * 7
        off = 0 if row % 2 == 0 else 26
        for x in range(off, W, 52):
            img[y:min(y + 6, H), x, :3] = B.hexrgb(line)
    img[y0, :, :3] = rgb('k')


def palestra_bg():
    """The gym: lilac walls, wall bars, a window, pennants, a blue mat."""
    img = B.canvas()
    _wall(img, 172, '#efe6ff', '#d8c8f6')
    yy, xx = np.mgrid[0:H, 0:W]
    # skirting board
    img[164:172, :, :3] = B.hexrgb('#c9b3ef')
    img[164, :, :3] = B.hexrgb('#b39ae4')
    # pennant garland across the top
    cols = ['h', 'Y', 'T', 'v', 'P', 'B']
    for i, x in enumerate(range(6, W, 22)):
        tri = poly_mask(W, H, [(x, 6 + (1 if i % 2 else 0)), (x + 16, 6 + (1 if i % 2 else 0)), (x + 8, 20)])
        put(img, tri, cols[i % len(cols)], 'w', cols[i % len(cols)])
    for x in range(0, W):
        y = 6 + (1 if (x // 11) % 2 else 0)
        img[y - 1, x, :3] = rgb('k')
    # wall bars (spalliera) on the left
    for px_ in (14, 70):
        post = rect_mask(W, H, px_, 34, px_ + 5, 166)
        put(img, post, 'J', 'j', 'q')
    for y in range(44, 160, 12):
        rung = rect_mask(W, H, 20, y, 69, y + 2)
        put(img, rung, 'j', 'u', 'J')
    # window with sky and a cloud
    win = rect_mask(W, H, 232, 32, 302, 98, radius=4)
    put(img, win, 'W', 'w', 's')
    glass = rect_mask(W, H, 237, 37, 297, 93, radius=2)
    B.gradient(img, 37, 94, ['#8fd0ff', '#c4e6ff'], 237, 298)
    img[~glass & rect_mask(W, H, 237, 37, 297, 93), :3] = rgb('W')
    cloud = (ellipse_mask(W, H, 256, 62, 9, 6) | ellipse_mask(W, H, 266, 58, 10, 8) |
             ellipse_mask(W, H, 277, 63, 9, 6)) & glass
    paint(img, cloud, 'w')
    sun = ellipse_mask(W, H, 286, 46, 6, 6) & glass
    paint(img, sun, 'y')
    img[64:66, 237:298, :3] = rgb('W')          # window cross
    img[37:94, 266:268, :3] = rgb('W')
    # rings hanging from the ceiling (right of the window)
    for rx in (214,):
        img[0:40, rx, :3] = rgb('x')
        ring = ellipse_mask(W, H, rx + 0.5, 46.5, 6, 6) & ~ellipse_mask(W, H, rx + 0.5, 46.5, 4, 4)
        put(img, ring, 'Y', 'y', 'o')
    # floor and the big blue mat
    _wood_floor(img, 172, '#f6d7a8', '#f0cb95', '#d9a870')
    mat = poly_mask(W, H, [(58, 182), (262, 182), (282, 214), (38, 214)])
    put(img, mat, 'B', 'b', 'n')
    for x in range(78, 250, 34):              # seams of the mat
        for y in range(184, 213):
            t = (y - 182) / 32.0
            xs = int(round(x + (x - 160) * 0.12 * t))
            if mat[y, xs]:
                img[y, xs, :3] = rgb('n')
    return img


def trucco_bg():
    """The make-up salon: striped wallpaper, a big mirror with bulbs where the
    client sits (the quiz panel), the vanity table for the cards."""
    img = B.canvas()
    yy, xx = np.mgrid[0:H, 0:W]
    stripe = (xx // 12) % 2 == 0
    img[stripe, :3] = B.hexrgb('#ffe0ef')
    img[~stripe, :3] = B.hexrgb('#fff4f9')
    for y in range(10, 176, 22):             # little stars on the wallpaper
        for x in range(8, W, 24):
            off = 12 if (y // 22) % 2 else 0
            sx = x + off
            if sx + 3 < W and y + 3 < H:
                img[y + 1, sx:sx + 3, :3] = B.hexrgb('#ffc2dc')
                img[y:y + 3, sx + 1, :3] = B.hexrgb('#ffc2dc')
    x0, y0, x1, y1 = B.PANEL
    frame = rect_mask(W, H, x0 - 5, y0 - 5, x1 + 5, y1 + 5, radius=12)
    glass = rect_mask(W, H, x0, y0, x1, y1, radius=8)
    put(img, frame, 'Y', 'y', 'o')
    B.gradient(img, y0, y1 + 1, ['#e3f1ff', '#cfe6ff'], x0, x1 + 1)
    img[~glass & rect_mask(W, H, x0, y0, x1, y1), :3] = rgb('Y')
    shine = glass & ((((xx - yy) % 56) < 5) | (((xx - yy) % 56) == 8))
    B.blend(img, shine, B.hexrgb('#ffffff'), 0.55)
    img[dilate4(glass) & ~glass & frame, :3] = rgb('O')
    bulbs = [(x, y0 - 3) for x in range(x0 + 10, x1 - 4, 22)] + \
            [(x0 - 3, y) for y in range(y0 + 14, y1 - 4, 22)] + [(x1 + 3, y) for y in range(y0 + 14, y1 - 4, 22)]
    for (bx, by) in bulbs:
        b = ellipse_mask(W, H, bx + 0.5, by + 0.5, 3.2, 3.2)
        paint(img, b, 'y')
        img[dilate4(b) & ~b, :3] = rgb('o')
        img[by - 1, bx - 1, :3] = rgb('w')
    # vanity table
    img[178:H, :, :3] = B.hexrgb('#f6c89f')
    img[178:181, :, :3] = B.hexrgb('#ffe0c4')
    img[181, :, :3] = B.hexrgb('#d99a6c')
    for y in range(192, H, 12):
        img[y, :, :3] = B.hexrgb('#ebb58a')
    img[177, :, :3] = rgb('k')
    # a pot of brushes and a perfume on the left
    pot = rect_mask(W, H, 44, 160, 58, 177, radius=2)
    for i, (bx, col) in enumerate(((47, 'h'), (51, 'T'), (55, 'Y'))):
        stick = rect_mask(W, H, bx, 146 + i * 3, bx + 1, 162)
        paint(img, stick, 'J')
        tip = ellipse_mask(W, H, bx + 1, 145 + i * 3, 2, 3)
        put(img, tip, col, 'w', col)
    put(img, pot, 'v', 'L', 'V')
    bottle = rect_mask(W, H, 18, 158, 32, 177, radius=3)
    put(img, bottle, 'b', 'w', 'B')
    cap = rect_mask(W, H, 22, 152, 28, 157)
    put(img, cap, 'Y', 'y', 'o')
    return img


def laboratorio_bg():
    """The wizard's laboratory: stone walls, shelves of potions, a round window
    with the moon, and a parchment board (the quiz panel)."""
    img = B.canvas()
    B.gradient(img, 0, 190, ['#2a1a4a', '#3a2466', '#4a2e7a'])
    # stone blocks
    for row, y in enumerate(range(0, 190, 14)):
        off = 0 if row % 2 == 0 else 18
        img[y, :, :3] = B.hexrgb('#24163f')
        for x in range(off, W, 36):
            img[y:y + 14, x, :3] = B.hexrgb('#24163f')
    rng = np.random.default_rng(3)
    B.stars(img, rng, 26, (4, 4, 78, 26))
    # round window with the moon (top left)
    win = ellipse_mask(W, H, 38.5, 22.5, 18, 18)
    sky_ = B.canvas()
    B.gradient(sky_, 4, 42, ['#16103a', '#2c2070'], 20, 58)
    img[win, :3] = sky_[win, :3]
    moon = ellipse_mask(W, H, 44, 18, 7, 7) & ~ellipse_mask(W, H, 47, 15, 6, 6)
    paint(img, moon, 'y')
    ring = dilate4(win) & ~win
    img[ring, :3] = rgb('J')
    img[dilate4(ring) & ~ring & ~win, :3] = rgb('q')
    # shelves with potion bottles (left column)
    for sy in (72, 124):
        shelf = rect_mask(W, H, 4, sy, 74, sy + 4)
        put(img, shelf, 'J', 'j', 'q')
        for i, (bx, col) in enumerate(((10, 'G'), (26, 'h'), (42, 'B'), (58, 'Y'))):
            hgt = 14 + (i * 5) % 9
            bottle = ellipse_mask(W, H, bx + 5, sy - 6, 6, 6) | rect_mask(W, H, bx + 3, sy - hgt, bx + 7, sy - 6)
            put(img, bottle, col, 'w', col)
            cork = rect_mask(W, H, bx + 3, sy - hgt - 3, bx + 7, sy - hgt - 1)
            put(img, cork, 'O', 'o', 'q')
    # parchment board
    x0, y0, x1, y1 = B.PANEL
    m = rect_mask(W, H, x0, y0, x1, y1, radius=6)
    wood = rect_mask(W, H, x0 - 4, y0 - 4, x1 + 4, y1 + 4, radius=8)
    put(img, wood, 'J', 'j', 'q')
    paint(img, m, B.hexrgb('#fdf1d6'))
    yy, xx = np.mgrid[0:H, 0:W]
    speck = m & (((xx * 7 + yy * 13) % 29) == 0)
    img[speck, :3] = B.hexrgb('#f1dcb0')
    edge = m & ~rect_mask(W, H, x0 + 2, y0 + 2, x1 - 2, y1 - 2, radius=5)
    img[edge, :3] = B.hexrgb('#e8cf9c')
    # stone floor
    img[190:H, :, :3] = B.hexrgb('#5a4a78')
    for y in range(190, H, 10):
        img[y, :, :3] = B.hexrgb('#46396a')
    for row, y in enumerate(range(190, H, 10)):
        off = 0 if row % 2 == 0 else 20
        for x in range(off, W, 40):
            img[y:y + 10, x, :3] = B.hexrgb('#46396a')
    img[190, :, :3] = rgb('k')
    # hanging stars from the ceiling on the right of the board
    for (sx, ln) in ((310, 30), (8, 0)):
        if ln:
            img[0:ln, sx, :3] = rgb('S')
            star = star_mask_px(W, H, sx + 0.5, ln + 5, 5, 2.3)
            put(img, star, 'Y', 'y', 'o')
    return img


def all_backgrounds():
    import places13 as P13   # 0.13.0: the three rooms painted again (the old ones stay above for reference)
    return {'bg_palestra': P13.palestra(), 'bg_trucco': P13.trucco(), 'bg_laboratorio': P13.laboratorio()}


# ------------------------------------------------------------------ gym: decorations of the routine cards
# (the moves themselves are Deva's own poses, drawn and turned at runtime)
def _arc_mask(w, h, cx, cy, rad, a0, a1, thick=1.4):
    m = np.zeros((h, w), dtype=bool)
    steps = max(8, int(abs(a1 - a0) / 6))
    pts = [(cx + rad * math.cos(math.radians(a0 + (a1 - a0) * i / steps)),
            cy + rad * math.sin(math.radians(a0 + (a1 - a0) * i / steps))) for i in range(steps + 1)]
    for p, q in zip(pts, pts[1:]):
        m |= capsule_mask(w, h, p, q, thick)
    ex, ey = pts[-1]
    a = math.radians(a1)
    d = 1 if a1 > a0 else -1
    tx, ty = -math.sin(a) * d, math.cos(a) * d          # tangent at the end
    nx, ny = math.cos(a), math.sin(a)                   # normal
    m |= poly_mask(w, h, [(ex + tx * 5, ey + ty * 5), (ex + nx * 4, ey + ny * 4), (ex - nx * 4, ey - ny * 4)])
    return m


def gm_giro():
    """Round arrow around the curled-up Deva: the forward roll."""
    return shaded_part(_arc_mask(40, 40, 20, 20, 16, 150, 480), 'Y', 'y', 'o')


def gm_arco():
    """Arched arrow over the upside-down Deva: the cartwheel."""
    return shaded_part(_arc_mask(46, 22, 23, 24, 19, 195, 345), 'Y', 'y', 'o')


def gm_scia():
    """Speed lines under the feet: she is up in the air."""
    img = blank(22, 8)
    for x, h in ((3, 6), (10, 7), (17, 6)):
        m = capsule_mask(22, 8, (x + 0.5, 1), (x + 0.5, h), 0.9)
        img[m, :3] = rgb('M')
        img[m, 3] = 255
    return img


def trampolino():
    img = blank(64, 20)
    for lx in (8, 54):
        leg = rect_mask(64, 20, lx, 6, lx + 2, 19)
        put(img, leg, 'S', 's', 'x')
    frame = ellipse_mask(64, 20, 32, 6.5, 30, 5.5)
    put(img, frame, 'B', 'b', 'n')
    bed = ellipse_mask(64, 20, 32, 6.5, 26, 3.5)
    paint(img, bed, 'K')
    img[5, 12:52, :3] = rgb('x')
    return img


# ------------------------------------------------------------------ menu icons (44x44)
def menu_ginnastica():
    """Rhythmic gymnastics: a ribbon swirling from its stick, and a ball."""
    img = blank(44, 44)
    ball = ellipse_mask(44, 44, 12, 33, 8, 8)
    put(img, ball, 'T', 't', 'e')
    img[29:31, 8:10, :3] = rgb('w')
    m = np.zeros((44, 44), dtype=bool)
    pts = []
    for i in range(70):                 # a ribbon waving away from the stick
        t = i / 69.0
        x = 36 - 33 * t
        y = 13 + 8.5 * math.sin(2.6 * math.pi * t) * (1 - 0.25 * t) - 3 * t
        pts.append((x, y))
    pts = pts[::-1]
    for p, q in zip(pts, pts[1:]):
        m |= capsule_mask(44, 44, p, q, 1.6)
    put(img, m, 'h', 'P', 'H')
    stick = capsule_mask(44, 44, pts[-1], (41, 30), 1.2)
    put(img, stick, 'J', 'j', 'q')
    star = star_mask_px(44, 44, 33, 38, 5, 2.2)
    put(img, star, 'Y', 'y', 'o')
    return img


def menu_trucco():
    import tale as TL
    img = TL.face_icon(TL.ciuffone('b'))
    # a lipstick in the corner, and lipstick on its mouth
    tube = rect_mask(44, 44, 34, 30, 41, 42, radius=1)
    put(img, tube, 'Y', 'y', 'o')
    bullet = poly_mask(44, 44, [(35, 30.5), (35, 23), (40.6, 19.5), (40.6, 30.5)])
    put(img, bullet, 'r', 'P', 'R')
    return img


def menu_forme():
    img = blank(44, 44)
    circ = ellipse_mask(44, 44, 13, 13, 9.5, 9.5)
    put(img, circ, 'X', 'r', 'R')
    sq = rect_mask(44, 44, 23, 5, 39, 21, radius=2)
    put(img, sq, 'n', 'B', 'N')
    tri = poly_mask(44, 44, [(22, 22), (38, 40), (6, 40)])
    put(img, tri, 'Y', 'y', 'o')
    star = star_mask_px(44, 44, 35.5, 33, 6.2, 2.8)
    put(img, star, 'G', 'g', 'd')
    return img


# ------------------------------------------------------------------ make-up on the monsters
SHADOW_COLS = {'rosa': ('P', 'p', 'h'), 'azzurro': ('B', 'b', 'n'), 'viola': ('v', 'L', 'V'), 'oro': ('Y', 'y', 'o')}
LIP_COLS = {'rosso': ('X', 'r', 'R'), 'rosa': ('P', 'p', 'h'), 'viola': ('v', 'L', 'V')}


def _crescent(w, h):
    """Eyeshadow above an eye: a crescent w x h, drawn with a 1 px margin."""
    W_, H_ = w + 2, h + 2
    outer = ellipse_mask(W_, H_, W_ / 2, h + 1.2, w / 2, h + 0.2)
    inner = ellipse_mask(W_, H_, W_ / 2, h + 1.6, w / 2 - 1.6, h - 1.4)
    return outer & ~inner


def _lips(w, h):
    W_, H_ = w + 2, h + 2
    cx = W_ / 2
    upper = ellipse_mask(W_, H_, cx - w * 0.22, h * 0.45 + 1, w * 0.3, h * 0.33) | \
        ellipse_mask(W_, H_, cx + w * 0.22, h * 0.45 + 1, w * 0.3, h * 0.33)
    lower = ellipse_mask(W_, H_, cx, h * 0.52 + 1, w / 2, h * 0.52)
    m = upper | lower
    return m


def makeup_sprites():
    out = {}
    for name, col in SHADOW_COLS.items():
        out['mk_ombretto_' + name] = shaded_part(_crescent(10, 3), *col, outline='k')
        out['mk_ombrettog_' + name] = shaded_part(_crescent(20, 5), *col, outline='k')
    for name, col in LIP_COLS.items():
        lips = shaded_part(_lips(14, 6), *col, outline='k')
        mid = lips.shape[0] // 2
        lips[mid, 3:-3, :3] = rgb(col[2])        # the line between the lips
        out['mk_rossetto_' + name] = lips
        small = shaded_part(_lips(9, 4), *col, outline='k')
        out['mk_rossettos_' + name] = small
    g = blank(8, 6)
    m = ellipse_mask(8, 6, 4, 3, 3.6, 2.6)
    paint(g, m, 'c')
    paint(g, ellipse_mask(8, 6, 4, 3, 1.8, 1.2), 'P')
    out['mk_guance'] = g
    out['mk_adesivo_stella'] = shaded_part(star_mask_px(9, 9, 4.5, 4.8, 4.3, 1.9), 'Y', 'y', 'o')
    heart = ellipse_mask(9, 9, 3, 3.5, 2.3, 2.3) | ellipse_mask(9, 9, 6, 3.5, 2.3, 2.3) | \
        poly_mask(9, 9, [(0.9, 4.2), (8.1, 4.2), (4.5, 8.2)])
    out['mk_adesivo_cuore'] = shaded_part(heart, 'r', 'P', 'R')
    return out


# ------------------------------------------------------------------ shapes (masks painted at runtime)
SH = 36


def shape_mask(name):
    s = SH
    if name == 'cerchio':
        return ellipse_mask(s, s, 18, 18, 14.5, 14.5)
    if name == 'quadrato':
        return rect_mask(s, s, 5, 5, 30, 30, radius=2)
    if name == 'triangolo':
        return poly_mask(s, s, [(18, 3), (33, 31), (3, 31)])
    if name == 'rettangolo':
        return rect_mask(s, s, 2, 9, 33, 26, radius=2)
    if name == 'stella':
        return star_mask_px(s, s, 18, 19, 16, 7.2)
    if name == 'cuore':
        return ellipse_mask(s, s, 11.5, 12.5, 8.5, 8.2) | ellipse_mask(s, s, 24.5, 12.5, 8.5, 8.2) | \
            poly_mask(s, s, [(3.3, 15), (32.7, 15), (18, 32.5)])
    raise ValueError(name)


SHAPES = ['cerchio', 'quadrato', 'triangolo', 'rettangolo', 'stella', 'cuore']


def _mask_sprite(mask):
    img = blank(mask.shape[1], mask.shape[0])
    img[mask, :3] = rgb('w')
    img[mask, 3] = 255
    return img


def shape_sprites():
    out = {}
    for name in SHAPES:
        m = shape_mask(name)
        up = np.zeros_like(m)
        up[1:, :] = m[:-1, :]
        left = np.zeros_like(m)
        left[:, 1:] = m[:, :-1]
        dn = np.zeros_like(m)
        dn[:-1, :] = m[1:, :]
        rt = np.zeros_like(m)
        rt[:, :-1] = m[:, 1:]
        light = m & (~up | ~left) & ~(~dn | ~rt)
        dark = m & (~dn | ~rt)
        ring = dilate4(m) & ~m
        out['fs_' + name] = _mask_sprite(m)
        out['fs_' + name + '_o'] = _mask_sprite(ring)
        out['fs_' + name + '_l'] = _mask_sprite(light)
        out['fs_' + name + '_d'] = _mask_sprite(dark)
    return out


# ------------------------------------------------------------------ cauldron, bubble
def calderone():
    img = blank(64, 46)
    for lx in (14, 46):                          # legs
        leg = rect_mask(64, 46, lx, 36, lx + 3, 45)
        put(img, leg, 'x', 'S', 'K')
    pot = ellipse_mask(64, 46, 32, 24, 27, 19) & rect_mask(64, 46, 0, 9, 63, 45)
    put(img, pot, 'x', 'S', 'K')
    rim = ellipse_mask(64, 46, 32, 10, 29, 6)
    put(img, rim, 'S', 's', 'x')
    inside = ellipse_mask(64, 46, 32, 10, 25, 4)
    paint(img, inside, 'K')
    return img


def pozione():
    """The potion's surface: a mask painted in the potion's colour."""
    m = ellipse_mask(52, 10, 26, 5, 24.5, 3.6)
    return _mask_sprite(m)


def bolla():
    m = ellipse_mask(7, 7, 3.5, 3.5, 3.2, 3.2) & ~ellipse_mask(7, 7, 3.5, 3.5, 2, 2)
    img = _mask_sprite(m)
    img[2, 2, :3] = rgb('w')
    img[2, 2, 3] = 255
    return img


def fumetto():
    """A speech bubble (the client's wish), 50x46 with the tail at the bottom left."""
    img = blank(50, 46)
    body = ellipse_mask(50, 46, 26, 19, 23, 18)
    tail = poly_mask(50, 46, [(8, 30), (18, 33), (3, 45)])
    m = body | tail
    put(img, m, 'W', 'w', 's')
    return img


def all_sprites():
    out = {
        'gm_giro': gm_giro(), 'gm_arco': gm_arco(), 'gm_scia': gm_scia(),
        'trampolino': __import__('props').trampolino(),   # redrawn in 0.8.0
        'menu_ginnastica': menu_ginnastica(), 'menu_trucco': menu_trucco(), 'menu_forme': menu_forme(),
        'calderone': calderone(), 'pozione': pozione(), 'bolla': bolla(), 'fumetto': fumetto(),
    }
    out.update(makeup_sprites())
    out.update(shape_sprites())
    return out
