"""The tale of the grey spell: Mago Pistacchio, Strega Grisella, the four
monsters (under the spell and freed), the magic wand, the spell orbs and the
colour gems.

Monsters are drawn on a 64x64 canvas, the witch and the wizard on 52x76 like
Deva (feet on the bottom rows). Every character comes in its spell colours
(grey, dark, glowing eyes: "un pizzico di brivido") and in its own colours
once the spell is broken.
"""
import numpy as np

import objects as O
from pixel import (blank, blit, ellipse_mask, rect_mask, shaded_part, rgb, dilate4, crop, flip_h)


# ------------------------------------------------------------------ helpers
def E(w, h, cx, cy, rx, ry):
    return ellipse_mask(w, h, cx, cy, rx, ry)


def R(w, h, x0, y0, x1, y1, rad=0):
    return rect_mask(w, h, x0, y0, x1, y1, rad)


def P(w, h, pts):
    return O.poly_mask(w, h, pts)


def put(img, mask, base, light, shadow, outline='k'):
    blit(img, shaded_part(mask, base, light, shadow, outline), 0, 0)


def paint(img, mask, c):
    img[mask, :3] = rgb(c)
    img[mask, 3] = 255


def px(img, x, y, c):
    if 0 <= y < img.shape[0] and 0 <= x < img.shape[1]:
        img[y, x, :3] = rgb(c)
        img[y, x, 3] = 255


def line(img, x0, y0, x1, y1, c, thick=1):
    n = int(max(abs(x1 - x0), abs(y1 - y0))) + 1
    for i in range(n + 1):
        t = i / max(n, 1)
        x = int(round(x0 + (x1 - x0) * t))
        y = int(round(y0 + (y1 - y0) * t))
        for dx in range(thick):
            px(img, x + dx, y, c)


def shaggy(w, h, cx, cy, rx, ry, n=26, tuft=3.5, top_only=False, seed=1):
    """A furry blob: an ellipse whose outline alternates in and out."""
    rng = np.random.RandomState(seed)
    pts = []
    for i in range(n * 2):
        a = np.pi * i / n
        out = (i % 2 == 0)
        k = 1.0 + (tuft / max(rx, ry)) * (1 if out else -0.2) * (0.8 + 0.4 * rng.rand())
        if top_only and np.sin(a) > 0.35:     # smooth underside
            k = 1.0
        pts.append((cx + rx * k * np.cos(a), cy + ry * k * np.sin(a)))
    return P(w, h, pts)


def eye_round(img, cx, cy, r=4, look=(0, 0)):
    """A big friendly eye: white, dark pupil, sparkle."""
    w, h = img.shape[1], img.shape[0]
    m = E(w, h, cx, cy, r, r + 0.5)
    put(img, m, 'w', 'w', 's')
    pu = E(w, h, cx + look[0] + 0.3, cy + look[1] + 0.6, r * 0.55, r * 0.65)
    paint(img, pu & m, 'K')
    px(img, int(cx + look[0] - 1), int(cy + look[1] - 1), 'w')


def blush(img, cx, cy):
    for dx in (-1, 0, 1):
        px(img, cx + dx, cy, 'c')
    px(img, cx, cy - 1, 'c')


def smile(img, cx, y, half=4, tongue=True, c='k'):
    for dx in range(-half, half + 1):
        yy = y + (1 if abs(dx) < half - 1 else 0)
        px(img, cx + dx, yy, c)
    if tongue:
        for dx in (-1, 0, 1):
            px(img, cx + dx, y + 2, 'h')


# ------------------------------------------------------------------ monsters (64x64)
MW = MH = 64


def ciuffone(form):
    """Forest monster: a big shaggy ball with horns."""
    img = blank(MW, MH)
    bad = form != 'b'
    body_c = ('V', 'v', 'K') if bad else ('L', 'l', 'm')
    horn_c = ('S', 's', 'x') if bad else ('Y', 'y', 'o')
    # horns behind the fur
    for sx in (-1, 1):   # curved horns, wide at the base
        horn = P(MW, MH, [(32 + sx * 6, 21), (32 + sx * 14, 12), (32 + sx * 17, 2), (32 + sx * 20, 9),
                          (32 + sx * 19, 17), (32 + sx * 15, 23)])
        put(img, horn, *horn_c)
    # feet
    for fx_ in (22, 42):
        put(img, E(MW, MH, fx_, 58.5, 6, 3.5), *(('M', 'm', 'K') if bad else ('m', 'L', 'M')))
    body = shaggy(MW, MH, 32, 36, 24, 21, n=24, tuft=3.8, seed=3)
    put(img, body, *body_c)
    # fur strands
    rng = np.random.RandomState(5)
    for _ in range(26):
        x, y = rng.randint(12, 52), rng.randint(20, 54)
        if body[y, x] and body[y + 2, x + 1] and not (22 <= y <= 50 and 20 <= x <= 44):
            px(img, x, y, body_c[2] if bad else 'm')
            px(img, x + 1, y + 1, body_c[2] if bad else 'm')
    # little arms (tufts)
    for sx in (-1, 1):
        arm = shaggy(MW, MH, 32 + sx * 24, 42, 5, 6, n=8, tuft=1.6, seed=7 + sx)
        put(img, arm, *body_c)
    if bad:
        # glowing angry eyes, heavy brows
        for sx in (-1, 1):
            cx = 32 + sx * 8
            eye = E(MW, MH, cx, 30, 4.5, 3.6)
            put(img, eye, '7', 'w', 'Y')
            paint(img, R(MW, MH, cx, 28, cx, 32), 'k')     # slit pupil
            for i in range(7):                           # brow slanting down to the middle
                x = cx - 3 + i if sx < 0 else cx + 3 - i
                px(img, x, 24 + i // 2, 'K')
                px(img, x, 25 + i // 2, 'K')
        if form == 'r':   # roaring: mouth wide open
            mouth = E(MW, MH, 32, 45, 10, 7) & R(MW, MH, 0, 40, 63, 63)
            put(img, mouth, '9', 'H', '9')
            paint(img, E(MW, MH, 32, 49, 5, 2.5) & mouth, 'h')
        else:
            mouth = E(MW, MH, 32, 42, 10, 5) & R(MW, MH, 0, 42, 63, 63)
            put(img, mouth, '9', '9', '9')
        for fx_ in (26, 38):                            # fangs
            fang = P(MW, MH, [(fx_ - 2, 42), (fx_ + 2, 42), (fx_, 47)])
            paint(img, fang, 'w')
            px(img, fx_, 47, 's')
    else:
        eye_round(img, 24, 30, 5, (1, 0))
        eye_round(img, 40, 30, 5, (-1, 0))
        blush(img, 18, 38)
        blush(img, 46, 38)
        mouth = E(MW, MH, 32, 41, 7, 5) & R(MW, MH, 0, 41, 63, 63)
        put(img, mouth, '9', '9', '9')
        paint(img, E(MW, MH, 32, 45, 3.5, 1.8) & mouth, 'h')
        # a flower on the head
        for i in range(5):
            a = np.deg2rad(i * 72 - 90)
            paint(img, E(MW, MH, 40 + 3 * np.cos(a), 12 + 3 * np.sin(a), 2, 2), 'P')
        paint(img, E(MW, MH, 40, 12, 1.4, 1.4), 'Y')
    return img


def melmoso(form):
    """Swamp monster: a slime dome with one big eye and drips."""
    img = blank(MW, MH)
    bad = form != 'b'
    c = ('5', '6', '4') if bad else ('g', 'w', 'G')
    body = (E(MW, MH, 32, 40, 27, 24) & R(MW, MH, 0, 0, 63, 55)) | E(MW, MH, 32, 54, 25, 4)
    for dx, ln in ((12, 5), (22, 3), (40, 7), (50, 4)):
        body |= R(MW, MH, dx - 1, 54, dx + 1, 55 + ln) | E(MW, MH, dx, 56 + ln, 2, 2)
    put(img, body, *c)
    # bubbles
    for (x, y, r) in ((14, 38, 2), (50, 34, 1.6), (46, 48, 1.4), (18, 50, 1.2)):
        m = E(MW, MH, x, y, r, r)
        paint(img, dilate4(m) & ~m & body, c[1])
    if bad:
        # one big glowing eye with a slit, and a heavy brow
        eye = E(MW, MH, 32, 29, 9, 8)
        put(img, eye, '7', 'w', 'Y')
        iris = E(MW, MH, 32, 30, 4.5, 5.5)
        paint(img, iris, '@')
        paint(img, R(MW, MH, 31, 26, 32, 34), 'k')
        for i in range(19):
            x = 23 + i
            y = 19 + (0 if abs(x - 32) > 5 else 1) + (1 if abs(x - 32) <= 2 else 0)
            px(img, x, y, 'k')
            px(img, x, y + 1, 'k')
        if form == 'r':
            mouth = E(MW, MH, 32, 45, 9, 5)
            put(img, mouth, '9', 'H', '9')
            for fx_ in (27, 37):
                paint(img, P(MW, MH, [(fx_ - 1.5, 41), (fx_ + 1.5, 41), (fx_, 44.5)]), 'w')
        else:
            for x in range(23, 42):                     # a wavy grumpy mouth
                y = 45 + (1 if (x // 3) % 2 else 0)
                px(img, x, y, 'k')
            paint(img, P(MW, MH, [(35, 46), (38, 46), (36.5, 49)]), 'w')
    else:
        eye = E(MW, MH, 32, 29, 9, 8.5)
        put(img, eye, 'w', 'w', 's')
        paint(img, E(MW, MH, 32, 30.5, 5, 5.5), 'N')
        paint(img, E(MW, MH, 32, 31, 2.8, 3.2), 'K')
        px(img, 30, 27, 'w')
        px(img, 31, 27, 'w')
        px(img, 30, 28, 'w')
        smile(img, 32, 44, 6)
        blush(img, 18, 40)
        blush(img, 46, 40)
        # a lily pad hat with a pink flower
        pad = E(MW, MH, 32, 16, 10, 3) & ~P(MW, MH, [(32, 16), (36, 12), (40, 13)])
        put(img, pad, 'G', 'g', 'd')
        for i in range(5):
            a = np.deg2rad(i * 72 - 90)
            paint(img, E(MW, MH, 28 + 2.6 * np.cos(a), 12 + 2.6 * np.sin(a), 1.8, 1.8), 'P')
        paint(img, E(MW, MH, 28, 12, 1.2, 1.2), 'Y')
    return img


def rocciolo(form):
    """Cave monster: a boulder with crystals on its back."""
    img = blank(MW, MH)
    bad = form != 'b'
    c = ('2', '3', '1') if bad else ('F', 'f', 'O')
    cr = ('V', 'v', '$') if bad else ('P', 'p', 'h')
    for (x0, top, x1) in ((14, 8, 24), (25, 2, 37), (38, 6, 48)):   # crystals
        m = P(MW, MH, [(x0, 22), ((x0 + x1) / 2, top), (x1, 22)])
        put(img, m, *cr)
        line(img, (x0 + x1) / 2, top + 3, (x0 + x1) / 2, 20, cr[1])
    for sx in (-1, 1):                                               # arms
        put(img, E(MW, MH, 32 + sx * 25, 44, 6, 7), *c)
    body = P(MW, MH, [(10, 26), (22, 16), (42, 16), (54, 26), (57, 44), (50, 58), (14, 58), (7, 44)])
    put(img, body, *c)
    for (x0, y0, x1, y1) in ((14, 34, 18, 38), (46, 30, 50, 35), (44, 50, 49, 53), (16, 50, 20, 54)):
        line(img, x0, y0, x1, y1, c[2])                              # cracks
    for fx_ in (22, 42):                                             # feet
        put(img, R(MW, MH, fx_ - 6, 56, fx_ + 6, 61, 2), *c)
    if bad:
        brow = R(MW, MH, 16, 25, 48, 29, 2)
        put(img, brow, '1', '2', '0')
        for sx in (-1, 1):
            cx = 32 + sx * 8
            paint(img, R(MW, MH, cx - 3, 31, cx + 3, 33), '0')
            paint(img, R(MW, MH, cx - 2, 31, cx + 2, 32), '@')
        h = 8 if form == 'r' else 5
        mouth = R(MW, MH, 22, 42, 42, 42 + h)
        put(img, mouth, '9', '9', '9')
        for tx in (24, 30, 36):
            paint(img, R(MW, MH, tx, 42, tx + 3, 44), 's')
        if form == 'r':
            for tx in (27, 33):
                paint(img, R(MW, MH, tx, 48, tx + 3, 50), 's')
    else:
        eye_round(img, 24, 31, 4)
        eye_round(img, 40, 31, 4)
        blush(img, 17, 39)
        blush(img, 47, 39)
        smile(img, 32, 42, 6)
    return img


def tuonello(form):
    """Cloud monster: a storm cloud with a lightning bolt (a rainbow once freed)."""
    img = blank(MW, MH)
    bad = form != 'b'
    c = ('2', '3', '1') if bad else ('W', 'w', 's')
    if bad:
        bolt = P(MW, MH, [(30, 44), (38, 44), (33, 52), (38, 52), (26, 63), (30, 54), (25, 54)])
        put(img, bolt, 'Y', 'y', 'o')
        if form == 'r':
            bolt2 = P(MW, MH, [(12, 42), (18, 42), (14, 49), (18, 49), (9, 58), (12, 51), (8, 51)])
            put(img, bolt2, 'Y', 'y', 'o')
        for (x, y) in ((46, 50), (52, 54), (42, 58), (16, 52)):
            line(img, x, y, x - 1, y + 3, 'n')
    else:
        for i, col in enumerate(('r', 'Y', 'G', 'B', 'v')):
            band = E(MW, MH, 32, 62, 26 - i * 3, 16 - i * 3) & ~E(MW, MH, 32, 62, 23 - i * 3, 13 - i * 3)
            paint(img, band & R(MW, MH, 0, 40, 63, 63), col)
    cloud = (E(MW, MH, 19, 30, 12, 11) | E(MW, MH, 32, 22, 15, 14) | E(MW, MH, 45, 30, 12, 11) |
             E(MW, MH, 12, 40, 10, 8) | E(MW, MH, 52, 40, 10, 8) | E(MW, MH, 32, 39, 24, 10))
    put(img, cloud, *c)
    if bad:
        for sx in (-1, 1):
            cx = 32 + sx * 8
            put(img, E(MW, MH, cx, 29, 4, 3.5), 'w', 'w', 's')
            paint(img, E(MW, MH, cx - sx * 0.5, 30, 1.8, 2), 'K')
            for i in range(7):
                x = cx - 3 + i if sx < 0 else cx + 3 - i
                px(img, x, 23 + i // 2, 'K')
                px(img, x, 24 + i // 2, 'K')
        if form == 'r':
            mouth = E(MW, MH, 32, 39, 6, 5)
            put(img, mouth, '9', 'H', '9')
        else:
            put(img, R(MW, MH, 25, 37, 39, 40, 1), 'w', 'w', 's')
            for x in (28, 32, 36):
                px(img, x, 37, 'k')
                px(img, x, 38, 'k')
                px(img, x, 39, 'k')
    else:
        for cx in (24, 40):   # happy closed eyes
            for (dx, dy) in ((-2, 1), (-1, 0), (0, -1), (1, -1), (2, 0), (3, 1)):
                px(img, cx + dx, 30 + dy, 'k')
        blush(img, 18, 35)
        blush(img, 46, 35)
        smile(img, 32, 36, 5)
    return img


MONSTERS = {'ciuffone': ciuffone, 'melmoso': melmoso, 'rocciolo': rocciolo, 'tuonello': tuonello}


# ------------------------------------------------------------------ witch and wizard (52x76)
CW, CH = 52, 76


def _robe(img, pts, c):
    put(img, P(CW, CH, pts), *c)


def strega(form):
    """Strega Grisella. form: 'c' spell (grey), 'cp' spell talking, 'pianto'
    crying, 'b' good, 'bp' good talking, 'balla' still grey but happy, dancing
    with Deva at the end (0.12)."""
    img = blank(CW, CH)
    good = form in ('b', 'bp')
    hat = ('V', 'v', 'M') if good else ('1', '2', '0')
    hair = ('L', 'l', 'm') if good else ('x', '%', 'K')
    skin = ('a', 'i', 'A') if good else ('8', 'w', '3')
    dress = ('P', 'p', 'h') if good else ('2', '3', '1')
    cape = ('v', 'L', 'V') if good else ('$', 'x', '0')
    # cape and hair behind
    _robe(img, [(15, 36), (37, 36), (47, 72), (5, 72)], cape)
    put(img, E(CW, CH, 26, 33, 14, 13) | R(CW, CH, 12, 32, 40, 50, 5), *hair)
    # dress with a ragged hem
    hem = []
    for i in range(9):
        x = 9 + i * 4.25
        hem.append((x, 70 if i % 2 == 0 else 66 + (0 if good else 1)))
    _robe(img, [(19, 40), (33, 40), (43, 68)] + hem[::-1] + [(9, 68)], dress)
    if good:   # little stars on the dress
        for (x, y) in ((20, 52), (30, 58), (24, 63), (34, 50)):
            px(img, x, y, 'Y')
            px(img, x - 1, y, 'y')
            px(img, x + 1, y, 'y')
            px(img, x, y - 1, 'y')
            px(img, x, y + 1, 'y')
    # shoes
    for fx_ in (20, 32):
        put(img, E(CW, CH, fx_, 72.5, 4, 2), 'K', 'x', 'K')
    # arms and hands; the right hand holds her wand up
    put(img, P(CW, CH, [(19, 41), (14, 54), (11, 53), (16, 40)]), *dress)
    put(img, E(CW, CH, 12, 55, 2.5, 2.5), *skin)
    put(img, P(CW, CH, [(33, 41), (41, 32), (43, 34), (36, 43)]), *dress)
    put(img, E(CW, CH, 42.5, 31.5, 2.5, 2.5), *skin)
    wand_c = 'J' if good else 'K'
    line(img, 43, 30, 46, 18, wand_c, thick=2)
    star = O.star_mask_px(9, 9, 4.5, 4.7, 4.4, 2.0)
    blit(img, shaded_part(star, *(('P', 'p', 'h') if good else ('V', 'v', '$'))), 42, 11)
    # face, a bit bigger than Deva's so her expressions read at 1x
    face = E(CW, CH, 26, 32, 10, 10.5)
    put(img, face, *skin)
    if form == 'pianto':
        for sx in (-1, 1):
            cx = 26 + sx * 5
            for (dx, dy) in ((-2, 1), (-1, 0), (0, 0), (1, 0), (2, 1)):
                px(img, cx + dx, 30 + dy, 'k')
            for dy in range(4):   # tears
                px(img, cx + sx * 2, 32 + dy, 'B')
            px(img, cx + sx * 2, 36, 'b')
        for x in range(22, 31):   # wobbly frown
            px(img, x, 39 - (1 if 23 <= x <= 29 else 0), 'k')
        paint(img, P(CW, CH, [(25, 31), (27, 31), (26, 35)]), skin[2])
    elif form == 'balla':   # eyes closed in a smile, the long nose, a big open smile
        for sx in (-1, 1):
            cx = 26 + sx * 5
            for (dx, dy) in ((-2, 1), (-1, 0), (0, 0), (1, 0), (2, 1)):
                px(img, cx + dx, 30 + dy, 'k')
        blush(img, 19, 35)
        blush(img, 33, 35)
        paint(img, P(CW, CH, [(25, 30), (27.5, 30), (28.5, 36.5), (24.5, 35)]), skin[2])
        px(img, 27, 36, '8')
        smile(img, 26, 38, 4, tongue=True)
    elif good:
        for sx in (-1, 1):
            cx = 26 + sx * 5
            paint(img, R(CW, CH, cx - 1, 29, cx + 1, 32), 'k')
            px(img, cx - 1, 29, 'w')
            px(img, cx - 2 * sx, 28, 'k')   # lashes
        blush(img, 19, 35)
        blush(img, 33, 35)
        paint(img, P(CW, CH, [(25.5, 32), (26.5, 32), (26, 34.5)]), skin[2])
        if form == 'bp':
            paint(img, E(CW, CH, 26, 38.5, 2.4, 2), '9')
        else:
            smile(img, 26, 37, 4, tongue=True)
    else:
        for sx in (-1, 1):      # narrow, sly eyes with a lime glow
            cx = 26 + sx * 5
            paint(img, R(CW, CH, cx - 2, 30, cx + 2, 30), 'k')
            paint(img, R(CW, CH, cx - 1, 31, cx + 1, 31), '7')
            px(img, cx - 2, 31, 'k')
            px(img, cx + 2, 31, 'k')
            line(img, cx - 3 * sx, 26, cx + 2 * sx, 28, 'K', thick=1)   # brows down to the middle
            line(img, cx - 3 * sx, 27, cx + 2 * sx, 29, 'K', thick=1)
        # the long pointed nose of a fairy-tale witch
        paint(img, P(CW, CH, [(25, 30), (27.5, 30), (28.5, 36.5), (24.5, 35)]), skin[2])
        px(img, 27, 36, '8')
        if form == 'cp':
            paint(img, E(CW, CH, 26, 39, 3.2, 2.2), '9')
            px(img, 25, 38, 'w')
        else:   # crooked grin with one tooth
            for x in range(21, 32):
                px(img, x, 39 - (1 if x >= 28 else 0) - (1 if x <= 22 else 0), 'k')
            px(img, 25, 40, 'w')
    # hair fringe over the forehead
    fringe = E(CW, CH, 26, 24.5, 10.5, 4) & R(CW, CH, 0, 0, 51, 25)
    put(img, fringe, *hair)
    # hat: wide brim and a bent cone
    cone = P(CW, CH, [(16, 22), (36, 22), (32, 11), (40, 3), (30, 6), (24, 13)])
    put(img, cone, *hat)
    band = R(CW, CH, 18, 18, 34, 21)
    paint(img, band & cone, 'Y' if good else '2')
    if good:
        px(img, 26, 19, 'w')
    else:
        paint(img, R(CW, CH, 24, 18, 27, 21) & cone, 'S')
    put(img, E(CW, CH, 26, 23, 17, 3.5), *hat)
    return img


def strega_volo():
    """The witch on her broom, for the flight over the map."""
    base = strega('c')
    img = blank(64, 76)
    broom = blank(64, 76)
    stick = R(64, 76, 2, 63, 50, 65)
    blit(broom, shaded_part(stick, 'J', 'j', 'q'), 0, 0)
    bristles = P(64, 76, [(48, 60), (62, 56), (63, 72), (48, 68)])
    blit(broom, shaded_part(bristles, 'o', 'Y', 'O'), 0, 0)
    for y in range(59, 70, 3):
        line(broom, 50, 64, 62, y, 'O')
    body = base.copy()
    body[70:, :, 3] = 0          # no feet: she sits on the broom
    blit(img, body, 6, -4)
    blit(img, broom, 0, 0)
    return crop(img)


def mago(form):
    """Mago Pistacchio. form: '' or 'p' (talking)."""
    img = blank(CW, CH)
    robe = ('G', 'g', 'd')
    # staff behind, on his left
    line(img, 6, 16, 6, 74, 'J', thick=2)
    line(img, 7, 16, 7, 74, 'q')
    star = O.star_mask_px(11, 11, 5.5, 5.8, 5.4, 2.4)
    blit(img, shaded_part(star, 'Y', 'y', 'o'), 1, 7)
    # robe
    _robe(img, [(17, 38), (35, 38), (46, 73), (6, 73)], robe)
    for (x, y) in ((14, 62), (36, 58), (24, 68), (40, 68)):
        px(img, x, y, 'Y')
        px(img, x - 1, y, 'y')
        px(img, x + 1, y, 'y')
        px(img, x, y - 1, 'y')
        px(img, x, y + 1, 'y')
    moon = E(CW, CH, 30, 64, 3, 3) & ~E(CW, CH, 31.5, 63, 2.6, 2.6)
    paint(img, moon, 'y')
    # sleeves and hands
    put(img, P(CW, CH, [(18, 40), (8, 50), (8, 54), (20, 46)]), *robe)
    put(img, E(CW, CH, 8, 52, 2.6, 2.6), 'a', 'i', 'A')
    put(img, P(CW, CH, [(34, 40), (44, 50), (43, 54), (32, 46)]), *robe)
    put(img, E(CW, CH, 43.5, 52, 2.6, 2.6), 'a', 'i', 'A')
    # shoes
    for fx_ in (20, 32):
        put(img, E(CW, CH, fx_, 73, 4, 2), 'J', 'j', 'q')
    # face
    put(img, E(CW, CH, 26, 31, 9, 9), 'a', 'i', 'A')
    blush(img, 19, 33)
    blush(img, 33, 33)
    # beard, long and wavy
    pts = [(17, 31), (35, 31), (37, 42), (34, 52)]
    for i in range(7):
        pts.append((33 - i * 2.3, 56 - (i % 2) * 3))
    pts += [(18, 52), (15, 42)]
    beard = P(CW, CH, pts)
    put(img, beard, 'W', 'w', 's')
    for (x0, y0) in ((22, 40), (27, 44), (31, 40), (24, 48)):
        line(img, x0, y0, x0 + 1, y0 + 3, 's')
    # moustache and mouth
    must = E(CW, CH, 22.5, 36, 4, 1.8) | E(CW, CH, 29.5, 36, 4, 1.8)
    put(img, must, 'W', 'w', 's')
    if form == 'p':
        paint(img, E(CW, CH, 26, 39, 2, 1.5), '9')
    # nose
    put(img, E(CW, CH, 26, 33, 2.2, 2), 'a', 'i', 'A')
    # round glasses
    for sx in (-1, 1):
        cx = 26 + sx * 5
        ring = E(CW, CH, cx, 29, 3.4, 3.2)
        paint(img, dilate4(ring) & ~ring, 'k')
        paint(img, ring, 'b')
        paint(img, E(CW, CH, cx, 29.4, 1.2, 1.4), 'k')
        px(img, cx - 2, 28, 'w')
    line(img, 25, 28, 27, 28, 'k')
    # bushy white eyebrows
    for sx in (-1, 1):
        cx = 26 + sx * 5
        paint(img, R(CW, CH, cx - 3, 24, cx + 3, 25), 'W')
        paint(img, R(CW, CH, cx - 3, 26, cx + 3, 26) & R(CW, CH, cx - 3 + (1 if sx < 0 else 0), 26, cx + 2, 26), 's')
    # tall hat with stars
    cone = P(CW, CH, [(15, 23), (37, 23), (29, 6), (22, 0), (24, 8)])
    put(img, cone, 'g', 'w', 'G')
    for (x, y) in ((24, 15), (30, 19), (25, 7)):
        px(img, x, y, 'Y')
        px(img, x - 1, y, 'y')
        px(img, x + 1, y, 'y')
        px(img, x, y - 1, 'y')
        px(img, x, y + 1, 'y')
    put(img, E(CW, CH, 26, 23.5, 16, 3.2), 'g', 'w', 'G')
    return img


# ------------------------------------------------------------------ props
def bacchetta():
    """Deva's wand: a candy stick and a gold star (7x18)."""
    img = blank(9, 20)
    stick = R(9, 20, 3, 7, 5, 19)
    blit(img, shaded_part(stick, 'W', 'w', 'S'), 0, 0)
    for y in (9, 12, 15, 18):
        img[y, 4:6, :3] = rgb('h')
    star = O.star_mask_px(9, 9, 4.5, 4.6, 4.3, 1.9)
    blit(img, shaded_part(star, 'Y', 'y', 'o'), 0, 0)
    return img


def gemma(col):
    c = {'verde': ('G', 'g', 'd'), 'azzurra': ('B', 'b', 'n'), 'rosa': ('P', 'p', 'h'),
         'gialla': ('Y', 'y', 'o'), 'viola': ('v', 'L', 'V')}[col]
    w, h = 20, 18
    img = blank(w, h)
    top = P(w, h, [(5, 1), (15, 1), (19, 6), (1, 6)])
    bottom = P(w, h, [(1, 6), (19, 6), (10, 17)])
    put(img, top | bottom, *c)
    paint(img, P(w, h, [(6, 2), (10, 2), (8, 5)]), c[1])
    paint(img, P(w, h, [(3, 7), (8, 7), (10, 15)]) & bottom, c[1])
    paint(img, P(w, h, [(12, 7), (18, 7), (10, 15)]) & bottom, c[2])
    img[2, 6, :3] = rgb('w')
    img[3, 5, :3] = rgb('w')
    return img


def sfera(broken=False):
    w = h = 14
    img = blank(w, h)
    if not broken:
        # a glowing spell orb: light rim so it shows on the dark places
        m = E(w, h, 7, 7, 5.6, 5.6)
        put(img, m, 'v', 'L', 'V', outline='p')
        core = E(w, h, 7.5, 7.5, 2.6, 2.6)
        paint(img, core, 'h')
        img[4, 5, :3] = rgb('w')
        img[5, 4, :3] = rgb('w')
        return img
    for (pts) in ([(2, 3), (6, 1), (5, 6)], [(8, 2), (12, 5), (9, 7)], [(3, 9), (7, 8), (6, 12)], [(9, 10), (12, 9), (11, 13)]):
        put(img, P(w, h, pts), 'v', 'L', 'V')
    return img


def bolla():
    """A magic soap bubble (20x20, 0.12): the spell around the monster of a duel. Its middle is
    empty (the monster shows through), the rim shimmers pink, lilac and sky blue."""
    w = h = 20
    img = blank(w, h)
    yy, xx = np.mgrid[0:h, 0:w]
    d = np.sqrt((xx - 9.5) ** 2 + (yy - 9.5) ** 2)
    rim = (d <= 9.4) & (d >= 7.6)
    ang = np.arctan2(yy - 9.5, xx - 9.5)
    cols = ['#ff93c6', '#d8b4ff', '#8fd8ff', '#b4f0e8']
    k = ((ang + np.pi) / (2 * np.pi) * 4).astype(int) % 4
    for i, c in enumerate(cols):
        paint(img, rim & (k == i), c)
    paint(img, (d <= 9.4) & (d > 8.7), '#7a5aa8')                      # the edge, for the dark places
    inner = (d < 7.6) & (d >= 6.8) & (yy > xx)                          # a faint inner sheen, lower right
    paint(img, inner, '#e8f4ff')
    glint = ((xx - 6) ** 2 + (yy - 6) ** 2 <= 4.5) & ~((xx - 7) ** 2 + (yy - 7) ** 2 <= 2.2)
    paint(img, glint, 'w')
    paint(img, (xx == 12) & (yy == 5), 'w')
    return img


def pannello():
    """The question panel of the duels, drawn over the story backgrounds."""
    x0, y0, x1, y1 = 80, 28, 302, 166
    w, h = x1 - x0 + 3, y1 - y0 + 3
    img = blank(w, h)
    m = R(w, h, 1, 1, w - 2, h - 2, 8)
    ring = dilate4(m) & ~m
    paint(img, m, 'W')
    paint(img, ring, 'k')
    inner = R(w, h, 3, 3, w - 4, h - 4, 6)
    paint(img, m & ~inner, 'p')
    yy, xx = np.mgrid[0:h, 0:w]
    dots = inner & (((xx + x0 - 1) % 16) == 8) & (((yy + y0 - 1) % 16) == 8)
    img[dots, :3] = (0xff, 0xe3, 0xef)
    return img


def face_icon(img, box=44):
    """A menu icon from a character: the top of the sprite, centred."""
    c = crop(img)
    h, w = c.shape[:2]
    out = blank(box, box)
    x = (box - w) // 2
    if w > box:
        c = c[:, (w - box) // 2:(w - box) // 2 + box]
        x = 0
    blit(out, c[:box], x, max(0, (box - min(h, box)) // 2))
    return out


def all_sprites():
    out = {}
    for name, fn in MONSTERS.items():
        for form in ('c', 'r', 'b'):
            out['mo_%s_%s' % (name, form)] = fn(form)
        out['menu_sfida_' + name] = face_icon(fn('c'))
        out['amico_' + name] = face_icon(fn('b'))          # the album: friends set free
    for form in ('c', 'cp', 'pianto', 'b', 'bp', 'balla'):
        out['strega_' + form] = strega(form)
    out['menu_sfida_grisella'] = face_icon(strega('c')[:48])
    out['amico_grisella'] = face_icon(strega('b')[:48])
    out['strega_volo'] = strega_volo()
    out['mago'] = mago('')
    out['mago_p'] = mago('p')
    out['bacchetta'] = bacchetta()
    for col in ('verde', 'azzurra', 'rosa', 'gialla', 'viola'):
        out['gemma_' + col] = gemma(col)
    out['sfera'] = sfera()
    out['bolla_magica'] = bolla()
    out['sfera_rotta'] = sfera(True)
    out['pannello'] = pannello()
    return out
