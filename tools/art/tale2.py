"""The second tale, "la notte senza stelle": lo Stregone Mezzanotte and his four
monsters (Polpone, Lumacone, Nevone, Fumino), the five big stars, the magic
lantern, the sack of stars, and Grisella (good now) flying on her broom.

Same conventions as tale.py: monsters on a 64x64 canvas in the forms 'c'
(under the spell), 'r' (under the spell, roaring or talking) and 'b' (freed);
the wizard on 52x76 like Deva, feet on the bottom rows.
"""
import numpy as np

import objects as O
from pixel import blank, blit, shaded_part, rgb, dilate4, crop
import tale as T
from tale import E, R, P, put, paint, px, line, eye_round, blush, smile, face_icon, shaggy, MW, MH, CW, CH


def worm(w, h, pts, r0, r1, steps=10):
    """A tapering tube along a polyline (tentacles, tails, eye stalks)."""
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


def star(w, h, cx, cy, R_, r_):
    return O.star_mask_px(w, h, cx, cy, R_, r_)


def angry_eyes(img, y, dx=8, cx=32, iris=('Y', 'y', 'o'), slit=False, rx=4.5, ry=3.6):
    """Glowing eyes under brows slanting down to the middle (the spell)."""
    w, h = img.shape[1], img.shape[0]
    for sx in (-1, 1):
        ex = cx + sx * dx
        eye = E(w, h, ex, y, rx, ry)
        put(img, eye, *iris)
        if slit:
            paint(img, R(w, h, ex - 2, y, ex + 2, y), 'k')
        else:
            paint(img, R(w, h, ex, y - 2, ex, y + 2), 'k')
        for i in range(7):
            x = ex - 3 + i if sx < 0 else ex + 3 - i
            px(img, x, y - 6 + i // 2, 'K')
            px(img, x, y - 5 + i // 2, 'K')


def fangs_mouth(img, cy, half=9, roar=False, fangs=(26, 38)):
    w, h = img.shape[1], img.shape[0]
    if roar:
        mouth = E(w, h, 32, cy + 3, half, 6.5) & R(w, h, 0, cy - 1, w - 1, h - 1)
        put(img, mouth, '9', 'H', '9')
        paint(img, E(w, h, 32, cy + 7, half * 0.5, 2.3) & mouth, 'h')
    else:
        mouth = E(w, h, 32, cy, half, 4.5) & R(w, h, 0, cy, w - 1, h - 1)
        put(img, mouth, '9', '9', '9')
    for fx_ in fangs:
        paint(img, P(w, h, [(fx_ - 2, cy), (fx_ + 2, cy), (fx_, cy + 4)]), 'w')


def happy_face(img, ey, my, dx=8, cx=32, r=5, cheeks=True):
    eye_round(img, cx - dx, ey, r, (1, 0))
    eye_round(img, cx + dx, ey, r, (-1, 0))
    if cheeks:
        blush(img, cx - dx - 6, ey + 8)
        blush(img, cx + dx + 6, ey + 8)
    w, h = img.shape[1], img.shape[0]
    mouth = E(w, h, cx, my, 6, 4.5) & R(w, h, 0, my, w - 1, h - 1)
    put(img, mouth, '9', '9', '9')
    paint(img, E(w, h, cx, my + 3, 3, 1.6) & mouth, 'h')


# ------------------------------------------------------------------ the monsters (64x64)
def polpone(form):
    """Beach monster: a big octopus. Under the spell it waves two arms up."""
    img = blank(MW, MH)
    bad = form != 'b'
    body = ('H', 'h', '9') if bad else ('c', 'p', 'h')
    # arms first, the head covers their roots
    if bad:
        arms = [[(17, 34), (9, 29), (5, 19), (8, 11), (13, 11), (13, 15)],
                [(47, 34), (55, 29), (59, 19), (56, 11), (51, 11), (51, 15)]]
    else:   # one arm waves hello, the other rests
        arms = [[(17, 36), (9, 32), (5, 23), (7, 15), (11, 13)],
                [(47, 38), (55, 44), (59, 51), (57, 56), (53, 55)]]
    arms += [[(22, 38), (17, 48), (11, 54), (6, 58), (8, 62), (12, 61)],
             [(28, 40), (27, 50), (23, 58), (25, 62), (29, 61)],
             [(36, 40), (37, 50), (41, 58), (39, 62), (35, 61)]]
    if bad:
        arms.append([(42, 38), (47, 48), (53, 54), (58, 58), (56, 62), (52, 61)])
    else:
        arms.append([(42, 39), (45, 49), (48, 57), (46, 62), (42, 61)])
    masks = [worm(MW, MH, a, 4.2, 1.6) for a in arms]
    for m in masks:
        put(img, m, *body)
    for a in arms:   # suckers along the arms
        for (x, y) in a[1:-1]:
            px(img, int(x), int(y) + 1, 'p' if bad else 'W')
    head = E(MW, MH, 32, 24, 19, 18) | E(MW, MH, 32, 36, 13, 7)
    put(img, head, *body)
    for (x, y, r) in ((22, 12, 2.2), (41, 10, 1.8), (46, 20, 1.6), (18, 22, 1.4), (35, 7, 1.3)):
        paint(img, E(MW, MH, x, y, r, r) & head, body[2] if bad else 'P')
    if bad:
        angry_eyes(img, 25, 8, slit=True)
        fangs_mouth(img, 35, 8, roar=form == 'r', fangs=(28, 36))
    else:
        happy_face(img, 24, 34, 8)
        # a little starfish on the head
        sf = star(MW, MH, 43, 9, 5.5, 2.4)
        put(img, sf, 'o', 'Y', 'O')
        px(img, 42, 8, 'k')
        px(img, 44, 8, 'k')
    return img


def lumacone(form):
    """Garden monster: a giant snail, eyes on stalks, a big spiral shell."""
    img = blank(MW, MH)
    bad = form != 'b'
    skin = ('5', '6', '4') if bad else ('g', 'w', 'G')
    shell = ('C', 'J', 'D') if bad else ('o', 'Y', 'O')
    # the foot, long and low
    foot = E(MW, MH, 30, 56, 27, 6) | R(MW, MH, 6, 52, 56, 60, 4)
    put(img, foot, *skin)
    # the shell on the back (right)
    sh = E(MW, MH, 40, 36, 19, 19)
    put(img, sh, *shell)
    for k in range(80):   # the spiral line
        a = k * 0.19
        r = 16 - k * 0.19
        if r < 1.5:
            break
        x, y = 40 + r * np.cos(a), 36 + r * np.sin(a)
        px(img, int(round(x)), int(round(y)), shell[2] if bad else 'O')
    if not bad:   # a flower on the shell
        for i in range(5):
            a = np.deg2rad(i * 72 - 90)
            paint(img, E(MW, MH, 48 + 3 * np.cos(a), 21 + 3 * np.sin(a), 2, 2), 'P')
        paint(img, E(MW, MH, 48, 21, 1.4, 1.4), 'Y')
    # head and neck rising at the front (left)
    neck = E(MW, MH, 15, 42, 9, 13) | E(MW, MH, 15, 50, 10, 6)
    put(img, neck, *skin)
    # eye stalks
    for (x0, x1) in ((11, 7), (19, 22)):
        st = worm(MW, MH, [(x0, 32), (x1, 21), (x1, 14)], 1.8, 1.4)
        put(img, st, *skin)
    if bad:
        for (x, y, sx) in ((7, 12, -1), (22, 12, 1)):
            put(img, E(MW, MH, x, y, 4.6, 4.2), 'Y', 'y', 'o')
            paint(img, R(MW, MH, x - 2, y, x + 2, y + 1), 'k')
            for i in range(8):   # a heavy brow slanting down towards the head
                bx = x - 4 + i if sx < 0 else x + 4 - i
                px(img, bx, 6 + i // 2, 'K')
                px(img, bx, 7 + i // 2, 'K')
        if form == 'r':
            m = E(MW, MH, 15, 43, 5.5, 4.5)
            put(img, m, '9', 'H', '9')
            paint(img, E(MW, MH, 15, 45, 2.5, 1.5) & m, 'h')
        else:
            for x in range(10, 21):
                px(img, x, 43 + (1 if x in (10, 11, 19, 20) else 0), 'k')
            paint(img, P(MW, MH, [(13, 43), (15, 43), (14, 46)]), 'w')
    else:
        eye_round(img, 7, 12, 3.4, (1, 0))
        eye_round(img, 22, 12, 3.4, (-1, 0))
        blush(img, 9, 42)
        blush(img, 22, 42)
        smile(img, 15, 42, 3, tongue=True)
    return img


def nevone(form):
    """Ice monster: a tall snowy furball with long arms and icy horns."""
    img = blank(MW, MH)
    bad = form != 'b'
    fur = ('b', 'w', 'n') if bad else ('W', 'w', 's')
    face_c = ('n', 'b', 'N') if bad else ('b', 'w', 'n')
    # horns of ice
    for sx in (-1, 1):
        horn = P(MW, MH, [(32 + sx * 8, 12), (32 + sx * 15, 1), (32 + sx * 15, 11), (32 + sx * 13, 16)])
        put(img, horn, 'T', 't', 'e') if bad else put(img, horn, 't', 'w', 'T')
    # feet
    for fx_ in (22, 42):
        put(img, E(MW, MH, fx_, 59, 7, 3.5), *(('n', 'b', 'N') if bad else ('s', 'w', 'b')))
    # long arms hanging down
    for sx in (-1, 1):
        arm = shaggy(MW, MH, 32 + sx * 22, 40, 5, 12, n=9, tuft=1.6, seed=11 + sx)
        put(img, arm, *fur)
    body = shaggy(MW, MH, 32, 34, 20, 24, n=26, tuft=3.2, seed=9)
    put(img, body, *fur)
    face = E(MW, MH, 32, 28, 12, 10)
    put(img, face, *face_c)
    if bad:
        angry_eyes(img, 26, 6, iris=('T', 't', 'e'), rx=3.6, ry=3)
        fangs_mouth(img, 33, 6, roar=form == 'r', fangs=(29, 35))
        for (x, top) in ((20, 50), (27, 54), (37, 54), (44, 50)):   # icicles on the fur
            paint(img, P(MW, MH, [(x - 1.5, top), (x + 1.5, top), (x, top + 5)]), 't')
    else:
        happy_face(img, 26, 32, 5, r=3.6, cheeks=False)
        blush(img, 22, 33)
        blush(img, 42, 33)
        # a red scarf
        scarf = R(MW, MH, 18, 39, 46, 43, 2)
        put(img, scarf, 'X', 'r', 'R')
        put(img, R(MW, MH, 38, 42, 42, 52, 1), 'X', 'r', 'R')
        for y in (45, 48, 51):
            px(img, 40, y, 'w')
    return img


def fumino(form):
    """Volcano monster: a little dragon; under the spell it puffs black smoke."""
    img = blank(MW, MH)
    bad = form != 'b'
    skin = ('R', 'r', '9') if bad else ('G', 'g', 'd')
    belly = ('o', 'Y', 'O') if bad else ('u', 'y', 'j')
    wing = ('9', 'H', 'K') if bad else ('T', 't', 'e')
    # tail curling on the right
    tail = worm(MW, MH, [(40, 50), (52, 52), (59, 44), (57, 36), (52, 38)], 4.5, 1.8)
    put(img, tail, *skin)
    put(img, P(MW, MH, [(50, 38), (54, 32), (56, 39)]), *wing)
    # wings behind
    for sx in (-1, 1):
        wg = P(MW, MH, [(32 + sx * 8, 30), (32 + sx * 24, 18), (32 + sx * 22, 28), (32 + sx * 26, 32),
                         (32 + sx * 18, 36)])
        put(img, wg, *wing)
    # feet and body
    for fx_ in (24, 40):
        put(img, E(MW, MH, fx_, 59, 6, 3.5), *skin)
    body = E(MW, MH, 32, 46, 14, 14)
    put(img, body, *skin)
    bel = E(MW, MH, 32, 49, 9, 10)
    put(img, bel, *belly)
    for y in (44, 48, 52):
        line(img, 26, y, 38, y, belly[2])
    # arms
    for sx in (-1, 1):
        put(img, E(MW, MH, 32 + sx * 13, 44, 3.2, 5), *skin)
    # head with a snout and spikes
    for x in (24, 32, 40):
        put(img, P(MW, MH, [(x - 3, 12), (x, 4), (x + 3, 12)]), *belly)
    head = E(MW, MH, 32, 22, 15, 12) | E(MW, MH, 32, 29, 11, 6)
    put(img, head, *skin)
    for x in (28, 36):   # nostrils
        paint(img, E(MW, MH, x, 28, 1.2, 1), skin[2])
    if bad:
        angry_eyes(img, 19, 7, rx=3.8, ry=3.2, slit=False)
        if form == 'r':   # roaring, a little fire in the mouth
            m = E(MW, MH, 32, 32, 6, 3.4)
            put(img, m, '9', 'H', '9')
            paint(img, E(MW, MH, 32, 33, 3.2, 1.8) & m, 'o')
            paint(img, E(MW, MH, 32, 33.5, 1.4, 0.8) & m, 'Y')
        else:
            for x in range(26, 39):
                px(img, x, 32 + (1 if x in (26, 27, 37, 38) else 0), 'k')
        for sx in (-1, 1):   # grey smoke rising from the head
            for (dx, y, r) in ((13, 13, 2.8), (17, 8, 2.3), (15, 3, 1.8)):
                put(img, E(MW, MH, 32 + sx * dx, y, r * 1.2, r), '2', '3', '1')
    else:
        happy_face(img, 19, 31, 7, r=3.8, cheeks=False)
        blush(img, 20, 27)
        blush(img, 44, 27)
        paint(img, E(MW, MH, 51, 12, 2.6, 2.4) | E(MW, MH, 55, 12, 2.6, 2.4) |
              P(MW, MH, [(48.6, 13), (57.4, 13), (53, 18)]), 'P')   # a little heart puff
    return img


MONSTERS2 = {'polpone': polpone, 'lumacone': lumacone, 'nevone': nevone, 'fumino': fumino}


# ------------------------------------------------------------------ lo Stregone Mezzanotte (52x76)
def stregone(form):
    """form: 'c' spell, 'cp' spell talking, 'paura' trembling, 'b' good (with the
    lantern), 'bp' good talking."""
    img = blank(CW, CH)
    good = form.startswith('b')
    scared = form == 'paura'
    cloak = ('n', 'b', 'N') if good else ('N', 'n', '$')
    hat = ('n', 'b', 'N') if good else ('$', 'M', '0')
    skin = ('a', 'i', 'A') if good else ('S', 'l', 'x')
    # staff with a crescent moon, behind on his right
    line(img, 45, 14, 45, 74, 'q' if good else 'K', thick=2)
    moon = E(CW, CH, 45, 11, 6, 6) & ~E(CW, CH, 48, 9, 5, 5)
    put(img, moon, 'Y', 'y', 'o') if good else put(img, moon, '7', 'y', '5')
    # long cloak with a high collar
    T._robe(img, [(16, 34), (36, 34), (44, 73), (8, 73)], cloak)
    collar = P(CW, CH, [(13, 26), (19, 36), (33, 36), (39, 26), (36, 38), (16, 38)])
    put(img, collar, *cloak)
    # the stolen stars glitter on his cloak; freed, it shows a calm starry sky
    for (x, y) in ((16, 60), (24, 66), (34, 58), (29, 50), (38, 68), (20, 48)):
        if good or form != 'paura':
            px(img, x, y, 'Y' if good else 'y')
            if good:
                px(img, x - 1, y, 'y')
                px(img, x + 1, y, 'y')
    # arms: the good wizard holds the lantern, the scared one covers his face
    if scared:
        for sx in (-1, 1):
            put(img, P(CW, CH, [(26 + sx * 8, 40), (26 + sx * 4, 30), (26 + sx * 7, 29), (26 + sx * 12, 40)]), *cloak)
            put(img, E(CW, CH, 26 + sx * 5, 29, 2.6, 2.6), *skin)
    else:
        put(img, P(CW, CH, [(17, 38), (8, 48), (10, 51), (19, 44)]), *cloak)
        put(img, E(CW, CH, 9, 50, 2.6, 2.6), *skin)
        put(img, P(CW, CH, [(35, 38), (43, 30), (46, 32), (38, 44)]), *cloak)
        put(img, E(CW, CH, 44.5, 30, 2.6, 2.6), *skin)
        if good:   # the magic lantern hangs from his hand
            line(img, 9, 52, 9, 55, 'k')
            lan = R(CW, CH, 5, 55, 13, 65, 2)
            put(img, lan, 'y', 'w', 'Y', outline='k')
            paint(img, R(CW, CH, 4, 54, 14, 55), 'q')
            paint(img, R(CW, CH, 4, 65, 14, 66), 'q')
            paint(img, E(CW, CH, 9, 60, 1.5, 2.5), 'o')
    # shoes
    for fx_ in (20, 32):
        put(img, E(CW, CH, fx_, 73, 4, 2), 'K', 'x', 'K')
    # a long thin face
    face = E(CW, CH, 26, 30, 8.5, 10.5)
    put(img, face, *skin)
    if scared:
        for sx in (-1, 1):   # wide eyes behind the fingers
            cx = 26 + sx * 4
            put(img, E(CW, CH, cx, 28, 2.4, 2.8), 'w', 'w', 's')
            paint(img, E(CW, CH, cx, 28.5, 1, 1.3), 'k')
        for x in range(22, 31):   # wobbly mouth
            px(img, x, 37 + (1 if x % 2 else 0), 'k')
        for (x, y) in ((14, 24), (38, 24), (12, 34), (40, 34)):   # he trembles
            px(img, x, y, 'w')
            px(img, x, y + 2, 'w')
    elif good:
        for sx in (-1, 1):
            cx = 26 + sx * 4
            paint(img, R(CW, CH, cx - 1, 27, cx + 1, 30), 'k')
            px(img, cx - 1, 27, 'w')
        blush(img, 20, 33)
        blush(img, 32, 33)
        if form == 'bp':
            paint(img, E(CW, CH, 26, 37, 2.4, 2), '9')
        else:
            smile(img, 26, 36, 3, tongue=False)
    else:
        for sx in (-1, 1):   # glowing yellow eyes, big pointed brows
            cx = 26 + sx * 4
            paint(img, E(CW, CH, cx, 28, 2.2, 1.6), 'Y')
            paint(img, R(CW, CH, cx, 27, cx, 29), 'k')
            line(img, cx - 3 * sx, 22, cx + 2 * sx, 26, 'K', thick=1)
            line(img, cx - 4 * sx, 21, cx - 3 * sx, 22, 'K', thick=1)
        if form == 'cp':
            paint(img, E(CW, CH, 26, 37, 3, 2.2), '9')
            px(img, 25, 36, 'w')
        else:   # a sly grin
            for x in range(20, 33):
                px(img, x, 36 - (1 if x in (20, 21, 31, 32) else 0), 'k')
    # a pointy goatee and a curly moustache
    if not scared:
        paint(img, P(CW, CH, [(23, 38), (29, 38), (26, 45)]), 'K' if not good else 'x')
        for sx in (-1, 1):
            line(img, 26, 34, 26 + sx * 5, 34, 'K' if not good else 'x')
            px(img, 26 + sx * 6, 33, 'K' if not good else 'x')
    # the tall hat, bent at the tip, with a crescent moon
    cone = P(CW, CH, [(15, 21), (37, 21), (32, 8), (38, -1), (27, 4), (22, 12)])
    put(img, cone, *hat)
    m2 = E(CW, CH, 25, 14, 3.2, 3.2) & ~E(CW, CH, 26.5, 13, 2.8, 2.8)
    paint(img, m2 & cone, 'Y' if good else '7')
    put(img, E(CW, CH, 26, 21.5, 16, 3.2), *hat)
    return img


def stregone_volo():
    """The wizard riding a crescent moon across the sky, with his sack of stars."""
    base = stregone('cp')
    img = blank(72, 80)
    moon = E(72, 80, 36, 60, 30, 16) & ~E(72, 80, 36, 50, 28, 13)
    blit(img, shaded_part(moon, 'y', 'w', 'o'), 0, 0)
    body = base.copy()
    body[64:, :, 3] = 0   # he sits in the moon
    blit(img, body, 8, -2)
    blit(img, sacco(), 44, 30)
    return crop(img)


def strega_volo_b():
    """Grisella, good now, on her broom (like tale.strega_volo)."""
    base = T.strega('b')
    img = blank(64, 76)
    broom = blank(64, 76)
    stick = R(64, 76, 2, 63, 50, 65)
    blit(broom, shaded_part(stick, 'J', 'j', 'q'), 0, 0)
    bristles = P(64, 76, [(48, 60), (62, 56), (63, 72), (48, 68)])
    blit(broom, shaded_part(bristles, 'o', 'Y', 'O'), 0, 0)
    for y in range(59, 70, 3):
        line(broom, 50, 64, 62, y, 'O')
    body = base.copy()
    body[70:, :, 3] = 0
    blit(img, body, 6, -4)
    blit(img, broom, 0, 0)
    return crop(img)


# ------------------------------------------------------------------ props
STAR_COLS = {'rossa': ('X', 'r', 'R'), 'arancione': ('o', 'Y', 'O'), 'azzurra': ('B', 'b', 'n'),
             'verde': ('G', 'g', 'd'), 'oro': ('Y', 'y', 'o')}


def stella(col):
    """A big star of the sky (22x22), with a shine."""
    w = h = 22
    img = blank(w, h)
    m = star(w, h, 11, 11.5, 10.5, 4.6)
    put(img, m, *STAR_COLS[col])
    img[6, 9, :3] = rgb('w')
    img[7, 8, :3] = rgb('w')
    img[5, 10, :3] = rgb('w')
    return img


def sacco():
    """The wizard's sack, full of stolen stars peeking out (26x26)."""
    w = h = 26
    img = blank(w, h)
    bag = E(w, h, 13, 16, 11, 9) | P(w, h, [(7, 9), (19, 9), (16, 13), (10, 13)])
    put(img, bag, '$', 'M', '0')
    paint(img, R(w, h, 8, 10, 18, 11), 'j')
    for (x, y) in ((9, 5), (15, 4), (12, 7)):
        s = star(w, h, x, y, 3.2, 1.4)
        put(img, s, 'Y', 'y', 'o')
    for (x, y) in ((8, 18), (16, 21), (18, 15)):
        px(img, x, y, 'y')
    return img


def lanterna():
    """The magic lantern of the ending (14x22)."""
    w, h = 14, 22
    img = blank(w, h)
    line(img, 7, 0, 7, 3, 'k')
    ring = E(w, h, 7, 2, 2.5, 2) & ~E(w, h, 7, 2, 1.2, 0.8)
    paint(img, ring, 'k')
    glass = R(w, h, 2, 6, 11, 18, 2)
    put(img, glass, 'y', 'w', 'Y')
    paint(img, R(w, h, 1, 4, 12, 6, 1), 'q')
    paint(img, R(w, h, 1, 18, 12, 20, 1), 'q')
    paint(img, E(w, h, 6.5, 12, 1.8, 3), 'o')
    paint(img, E(w, h, 6.5, 13, 0.8, 1.5), 'Y')
    return img


def tamburo():
    """The monster's drum for the rhythm duel (26x20)."""
    w, h = 26, 20
    img = blank(w, h)
    body = R(w, h, 2, 6, 23, 17, 2)
    put(img, body, 'X', 'r', 'R')
    for i in range(5):   # the gold cord in a zigzag
        x0 = 3 + i * 4
        line(img, x0, 9, x0 + 2, 15, 'Y')
        line(img, x0 + 2, 15, x0 + 4, 9, 'Y')
    paint(img, R(w, h, 2, 16, 23, 17), 'o')
    top = E(w, h, 12.5, 6, 11, 3.6)
    put(img, top, 'W', 'w', 's')
    return img


def all_sprites():
    out = {}
    for name, fn in MONSTERS2.items():
        for form in ('c', 'r', 'b'):
            out['mo_%s_%s' % (name, form)] = fn(form)
        out['menu_sfida_' + name] = face_icon(fn('c'))
        out['amico_' + name] = face_icon(fn('b'))
    for form in ('c', 'cp', 'paura', 'b', 'bp'):
        out['stregone_' + form] = stregone(form)
    out['menu_sfida_mezzanotte'] = face_icon(stregone('c')[:48])
    out['amico_mezzanotte'] = face_icon(stregone('b')[:48])
    out['stregone_volo'] = stregone_volo()
    out['strega_volo_b'] = strega_volo_b()
    for col in STAR_COLS:
        out['stella_' + col] = __import__('props').stella(col)   # faceted, with a smile (0.8.0)
    out['sacco'] = sacco()
    out['lanterna'] = lanterna()
    out['tamburo'] = tamburo()
    return out
