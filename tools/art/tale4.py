"""The fourth tale, "i giocattoli del Re Capriccio" (0.11.0): the little king
and his four toy monsters (Robottone, Saltamolla, Dinozzo, Trottolina), the
five wind-up keys (rossa, arancione, azzurra, verde, d'oro) and the toy train
that runs on the map.

Same conventions as tale.py: monsters on a 64x64 canvas in the forms 'c'
(under the spell), 'r' (under the spell, shouting) and 'b' (freed); the king
on 48x68, a child: smaller than Deva, feet on the bottom rows. Under the
spell the toys are grey and dusty with glowing eyes; freed, they are bright
toys again. The king is never scary: a spoilt little boy with a crown too big
for him, who learns to take turns.
"""
import numpy as np

from pixel import blank, blit, crop
from tale import E, R, P, put, paint, px, line, eye_round, blush, smile, face_icon, MW, MH
from tale2 import worm, angry_eyes, fangs_mouth, happy_face

LIME = ('7', '7', '6')   # the glowing eyes of the spell


def _rivets(img, pts, c):
    for (x, y) in pts:
        px(img, x, y, c)


# ------------------------------------------------------------------ the monsters (64x64)
def robottone(form):
    """A big tin robot with a wind-up key on its back. Under the spell it is a
    grey, rusty robot with a gauge on its chest and a grill of teeth."""
    img = blank(MW, MH)
    bad = form != 'b'
    tin = ('2', '3', '1') if bad else ('B', 'b', 'n')
    trim = ('1', '2', '0') if bad else ('S', 's', 'x')
    gold = ('3', 's', '2') if bad else ('Y', 'y', 'o')
    # the wind-up key, sticking out of its back on the right
    put(img, R(MW, MH, 48, 39, 56, 42), *gold)
    for (cy_, ry) in ((34.5, 4.6), (46.5, 4.6)):
        put(img, E(MW, MH, 58.5, cy_, 3.6, ry), *gold)
    put(img, R(MW, MH, 55, 36, 59, 45, 1), *gold)
    for cy_ in (34.5, 46.5):
        paint(img, E(MW, MH, 58.5, cy_, 1.2, 1.8), 'k' if not bad else '1')
    # legs and feet
    for fx_ in (24, 40):
        put(img, R(MW, MH, fx_ - 4, 53, fx_ + 4, 59), *trim)
        put(img, R(MW, MH, fx_ - 7, 58, fx_ + 7, 62, 2), *tin)
    # arms: down, or up shouting (r); a claw at the end
    up = form == 'r'
    for sx in (-1, 1):
        sh = (32 + sx * 18, 36)
        pts = [sh, (32 + sx * 23, 30), (32 + sx * 25, 20)] if up else [sh, (32 + sx * 23, 42), (32 + sx * 24, 49)]
        put(img, worm(MW, MH, pts, 3.4, 3.0), *trim)
        hx, hy = pts[-1]
        claw = E(MW, MH, hx, hy + (-1 if up else 2), 4, 3.4) & ~E(MW, MH, hx, hy + (-3 if up else 4), 1.6, 2.2)
        put(img, claw, *tin)
    # the body: a tin box with rivets
    body = R(MW, MH, 14, 31, 50, 54, 5)
    put(img, body, *tin)
    paint(img, R(MW, MH, 16, 33, 48, 34) & body, tin[1])
    _rivets(img, ((17, 34), (47, 34), (17, 51), (47, 51)), 'k' if not bad else '1')
    if bad:   # a gauge, the needle in the red
        put(img, E(MW, MH, 32, 43, 7, 6.5), '3', 's', '2')
        paint(img, E(MW, MH, 32, 43, 5, 4.5) & (np.mgrid[0:MH, 0:MW][0] < 43), 'r')
        line(img, 32, 44, 36, 39, 'k')
        px(img, 32, 44, 'k')
        for x in (21, 43):   # warning lights
            put(img, E(MW, MH, x, 37, 1.8, 1.8), '@', '@', 'R')
    else:     # a heart window and two little lights
        heart = E(MW, MH, 29, 41, 3.6, 3.4) | E(MW, MH, 35, 41, 3.6, 3.4) | \
            P(MW, MH, [(25.6, 42), (38.4, 42), (32, 49)])
        put(img, heart, 'X', 'r', 'R')
        px(img, 28, 40, 'p')
        px(img, 29, 39, 'p')
        put(img, E(MW, MH, 20, 37, 1.8, 1.8), 'Y', 'y', 'o')
        put(img, E(MW, MH, 44, 37, 1.8, 1.8), 'G', 'g', 'd')
    # neck, head, ear bolts, antenna
    put(img, R(MW, MH, 27, 28, 37, 31), *trim)
    for sx in (-1, 1):
        put(img, E(MW, MH, 32 + sx * 16, 18, 3, 4), *trim)
    head = R(MW, MH, 17, 7, 47, 29, 5)
    put(img, head, *tin)
    paint(img, R(MW, MH, 19, 9, 45, 10) & head, tin[1])
    line(img, 32, 2, 32, 7, 'k')
    put(img, E(MW, MH, 32, 3, 2.6, 2.6), *(('7', 'w', '6') if bad else ('X', 'r', 'R')))
    if bad:
        angry_eyes(img, 17, dx=7, iris=LIME, rx=3.6, ry=3)
        roar = form == 'r'
        grill = R(MW, MH, 23, 21 if roar else 22, 41, 27 if roar else 25, 1)
        put(img, grill, '9', '9', '9')
        for x in range(25, 40, 3):   # zigzag teeth
            paint(img, P(MW, MH, [(x - 1.3, grill.nonzero()[0].min()), (x + 1.3, grill.nonzero()[0].min()),
                                  (x, grill.nonzero()[0].min() + 2.5)]), 'w')
        if roar:   # steam from the ear bolts
            for (x, y, r) in ((9, 10, 2.6), (6, 5, 2.0), (55, 10, 2.6), (58, 5, 2.0)):
                put(img, E(MW, MH, x, y, r, r * 0.8), 's', 'W', '3')
    else:
        eye_round(img, 25, 17, 4, (1, 0))
        eye_round(img, 39, 17, 4, (-1, 0))
        blush(img, 20, 24)
        blush(img, 44, 24)
        smile(img, 32, 24, 4, tongue=True)
    return img


def saltamolla(form):
    """A jack-in-the-box: a puppet with a jester's cap on a spring, out of a
    box with diamonds. Under the spell the box is grey and the puppet grins."""
    img = blank(MW, MH)
    bad = form != 'b'
    box = ('2', '3', '1') if bad else ('Y', 'y', 'o')
    deco = ('1', '2', '0') if bad else ('B', 'b', 'n')
    face = ('3', 's', '2') if bad else ('W', 'w', 'f')
    capl = ('V', 'v', 'K') if bad else ('X', 'r', 'R')
    capr = ('1', '2', '0') if bad else ('B', 'b', 'n')
    bell = ('3', 's', '2') if bad else ('Y', 'y', 'o')
    lift = -3 if form == 'r' else 0   # shouting: the spring stretches
    # the lid, open at the back: only its edge shows
    put(img, P(MW, MH, [(14, 44), (50, 44), (52, 40), (16, 40)]), *deco)
    # the spring
    for k in range(4):
        y = 41 - k * 3 + (lift * k) // 3
        coil = E(MW, MH, 32, y, 6.5, 1.8)
        put(img, coil & ~E(MW, MH, 32, y - 0.6, 4.2, 0.9), 's', 'w', 'S')
    # the box with its diamonds
    boxm = R(MW, MH, 13, 43, 51, 62, 2)
    put(img, boxm, *box)
    paint(img, R(MW, MH, 13, 43, 51, 45) & boxm, box[1])
    for cx_ in (22, 32, 42):
        dia = P(MW, MH, [(cx_, 47), (cx_ + 4.5, 52.5), (cx_, 58), (cx_ - 4.5, 52.5)])
        put(img, dia, *deco)
    if not bad:
        for cx_ in (22, 42):
            px(img, cx_, 52, 'w')
        paint(img, E(MW, MH, 32, 52.5, 1.6, 1.6), 'X')
    # white gloves on the rim of the box
    for sx in (-1, 1):
        put(img, E(MW, MH, 32 + sx * 15, 43, 4.2, 3.2), *(('3', 's', '2') if bad else ('W', 'w', 's')))
    # the ruff
    hy = 19 + lift
    ruff = E(MW, MH, 32, hy + 14, 12, 3.6)
    for k in range(10):   # scallops
        ruff |= E(MW, MH, 21 + k * 2.4, hy + 16.5, 1.6, 1.6)
    put(img, ruff, *(('3', 's', '2') if bad else ('p', 'W', 'P')))
    # the head
    headm = E(MW, MH, 32, hy, 13, 12)
    put(img, headm, *face)
    # the jester's cap: a band and two floppy points with bells
    for sx, col in ((-1, capl), (1, capr)):
        pts = [(32, hy - 14), (32 + sx * 5, hy - 15), (32 + sx * 15, hy - 19), (32 + sx * 23, hy - 14),
               (32 + sx * 20, hy - 9), (32 + sx * 12, hy - 7), (32, hy - 6)]
        put(img, P(MW, MH, pts), *col)
        put(img, E(MW, MH, 32 + sx * 23, hy - 12, 2.8, 2.8), *bell)
    put(img, R(MW, MH, 20, hy - 9, 44, hy - 6, 1), *bell)
    if bad:
        angry_eyes(img, hy + 1, dx=6, iris=LIME, rx=3.2, ry=2.6)
        mouth = E(MW, MH, 32, hy + 6, 6.5 if form != 'r' else 7, 2.6 if form != 'r' else 4.2) & \
            R(MW, MH, 0, hy + 6, MW - 1, MH - 1)
        put(img, mouth, '9', 'H', '9')
        for fx_ in (29, 35):
            paint(img, P(MW, MH, [(fx_ - 1.3, hy + 6), (fx_ + 1.3, hy + 6), (fx_, hy + 8.5)]), 'w')
        put(img, E(MW, MH, 32, hy + 3, 2, 1.6), '1', '2', '0')   # a grey button nose
    else:
        eye_round(img, 26, hy, 3.6, (1, 0))
        eye_round(img, 38, hy, 3.6, (-1, 0))
        blush(img, 21, hy + 5)
        blush(img, 43, hy + 5)
        put(img, E(MW, MH, 32, hy + 3, 2, 1.6), 'X', 'r', 'R')   # a red button nose
        smile(img, 32, hy + 6, 3, tongue=True)
    return img


def dinozzo(form):
    """A toy dinosaur, soft and round, with spikes made of building blocks.
    Under the spell it is a muddy grey-green with dull blocks."""
    img = blank(MW, MH)
    bad = form != 'b'
    skin = ('5', '6', '4') if bad else ('G', 'g', 'd')
    belly = ('6', '6', '5') if bad else ('y', 'W', 'u')
    blocks = (('2', '3', '1'), ('1', '2', '0'), ('3', 's', '2')) if bad else \
        (('X', 'r', 'R'), ('Y', 'y', 'o'), ('B', 'b', 'n'))
    # the tail, curling out to the right behind the body
    put(img, worm(MW, MH, [(40, 52), (50, 54), (57, 50), (60, 43)], 6, 2.2), *skin)
    # the block spikes along the back, peeping out around the head
    for i, a in enumerate((-160, -128, -90, -52, -20)):
        r = np.deg2rad(a)
        x, y, s = 32 + 19 * np.cos(r), 22 + 15.5 * np.sin(r), 8 if i == 2 else 7
        x0, y0 = int(round(x - s / 2)), int(round(y - s / 2))
        put(img, R(MW, MH, x0, y0, x0 + s - 1, y0 + s - 1), *blocks[i % 3])
        if not bad:
            px(img, x0 + 1, y0 + 1, 'w')
    # legs with three toes
    for fx_ in (23, 41):
        put(img, E(MW, MH, fx_, 56, 7, 6), *skin)
        for t in (-3, 0, 3):
            px(img, fx_ + t, 61, 'w' if not bad else '3')
    # body and belly
    body = E(MW, MH, 32, 45, 16, 13)
    put(img, body, *skin)
    paint(img, E(MW, MH, 32, 47, 9.5, 9) & body, belly[0])
    for y in (43, 47, 51):   # belly stripes
        paint(img, E(MW, MH, 32, 47, 9.5, 9) & body & (np.mgrid[0:MH, 0:MW][0] == y), belly[2])
    # tiny arms
    for sx in (-1, 1):
        put(img, worm(MW, MH, [(32 + sx * 13, 40), (32 + sx * 17, 44), (32 + sx * 18, 47)], 2.8, 2.3), *skin)
    # the big round head
    headm = E(MW, MH, 32, 22, 17, 13.5)
    put(img, headm, *skin)
    paint(img, E(MW, MH, 32, 29, 10, 4.5) & headm, skin[1])   # the lighter snout
    for (x, y) in ((28, 27), (36, 27)):   # nostrils
        px(img, x, y, skin[2])
    if bad:
        angry_eyes(img, 19, dx=8, iris=LIME, rx=4, ry=3.2)
        fangs_mouth(img, 31, half=8, roar=form == 'r', fangs=(26, 38))
    else:
        eye_round(img, 25, 18, 4.4, (1, 0))
        eye_round(img, 39, 18, 4.4, (-1, 0))
        blush(img, 19, 26)
        blush(img, 45, 26)
        smile(img, 32, 30, 5, tongue=True)
        for x in (28, 36):   # two little blunt teeth
            px(img, x, 31, 'w')
    return img


def trottolina(form):
    """A spinning top with a little face, a bow and a handle on her head.
    Under the spell her stripes are grey; shouting, she spins (motion lines)."""
    img = blank(MW, MH)
    bad = form != 'b'
    stripes = ['2', '3', '1', '3'] if bad else ['P', 'Y', 'T', 'L']
    rim = ('1', '2', '0') if bad else ('h', 'P', 'H')
    face = ('3', 's', '2') if bad else ('W', 'w', 'f')
    # the spinning lines (shouting)
    if form == 'r':
        for (y, x0, x1) in ((36, 2, 9), (44, 1, 8), (52, 4, 10)):
            line(img, x0, y, x1, y - 1, 'k')
            line(img, 63 - x1, y - 1, 63 - x0, y, 'k')
    # the body of the top: the upper cone, the wide rim, the lower cone and its point
    upper = P(MW, MH, [(11, 44), (53, 44), (43, 31), (21, 31)])
    lower = P(MW, MH, [(11, 44), (53, 44), (35, 61), (29, 61)])
    tip = R(MW, MH, 30, 59, 34, 63, 1)
    put(img, tip, *(('1', '2', '0') if bad else ('S', 's', 'x')))
    bodym = upper | lower
    put(img, bodym, *face)
    yy, xx = np.mgrid[0:MH, 0:MW]
    swirl = np.floor((xx - 32 + (yy - 44) * 0.9) / 6.0).astype(int) % 4   # slanted stripes
    for k, c in enumerate(stripes):
        paint(img, bodym & (swirl == k) & ~(dilate_ring(bodym)), c)
    put(img, E(MW, MH, 32, 44, 22, 3.6), *rim)
    if not bad:
        for x in (16, 24, 32, 40, 48):   # little stars on the rim
            px(img, x, 44, 'y')
    # little arms up, dancing
    for sx in (-1, 1):
        put(img, worm(MW, MH, [(32 + sx * 12, 34), (32 + sx * 18, 29), (32 + sx * 20, 23)], 2.4, 2.2), *face)
        put(img, E(MW, MH, 32 + sx * 20, 22, 2.8, 2.8), *face)
    # the head and the handle of the top
    put(img, R(MW, MH, 29, 3, 35, 13, 1), *(('1', '2', '0') if bad else ('X', 'r', 'R')))
    put(img, E(MW, MH, 32, 4, 3.2, 3.2), *(('1', '2', '0') if bad else ('X', 'r', 'R')))
    headm = E(MW, MH, 32, 21, 11, 10)
    put(img, headm, *face)
    # a bow on the handle (freed) or a crumpled grey one
    for sx in (-1, 1):
        put(img, P(MW, MH, [(32, 10), (32 + sx * 8, 6), (32 + sx * 8, 14)]), *(('2', '3', '1') if bad else ('P', 'p', 'h')))
    put(img, E(MW, MH, 32, 10, 2.2, 2.2), *(('2', '3', '1') if bad else ('h', 'P', 'H')))
    if bad:
        angry_eyes(img, 21, dx=5, iris=LIME, rx=3, ry=2.6)
        mouth = E(MW, MH, 32, 26, 5, 2.2 if form != 'r' else 3.6) & R(MW, MH, 0, 26, MW - 1, MH - 1)
        put(img, mouth, '9', 'H', '9')
        for fx_ in (30, 34):
            paint(img, P(MW, MH, [(fx_ - 1.2, 26), (fx_ + 1.2, 26), (fx_, 28.5)]), 'w')
    else:
        eye_round(img, 27, 20, 3.4, (1, 0))
        eye_round(img, 37, 20, 3.4, (-1, 0))
        for sx in (-1, 1):   # long lashes
            px(img, 32 + sx * 5 + sx * 4, 17, 'k')
            px(img, 32 + sx * 5 + sx * 3, 16, 'k')
        blush(img, 23, 25)
        blush(img, 41, 25)
        smile(img, 32, 26, 3, tongue=True)
    return img


def dilate_ring(m):
    from pixel import dilate4
    inner = ~dilate4(~m)
    return m & ~inner


MONSTERS4 = {'robottone': robottone, 'saltamolla': saltamolla, 'dinozzo': dinozzo, 'trottolina': trottolina}


# ------------------------------------------------------------------ il Re Capriccio (48x68)
KW, KH = 48, 68
KSKIN = ('a', 'i', 'A')
HAIR = ('J', 'j', 'q')
CAPE = ('X', 'r', 'R')
TUNIC = ('B', 'b', 'n')


def _crown(img, cx, top, tilt=0):
    """A crown a little too big for him; tilt moves the right side up (+) or down (-)."""
    pts = [(cx - 12, top + 11 + tilt), (cx + 12, top + 11 - tilt), (cx + 13, top + 2 - tilt), (cx + 7, top + 6 - tilt),
           (cx + 3, top - tilt * 0.3), (cx, top + 5), (cx - 3, top + tilt * 0.3), (cx - 7, top + 6 + tilt),
           (cx - 13, top + 2 + tilt)]
    crown = P(KW, KH, pts)
    put(img, crown, 'Y', 'y', 'o')
    band = P(KW, KH, [(cx - 12, top + 11 + tilt), (cx + 12, top + 11 - tilt), (cx + 12, top + 8 - tilt),
                      (cx - 12, top + 8 + tilt)])
    paint(img, band & crown, 'o')
    for (x, c) in ((cx - 7, 'r'), (cx, 'B'), (cx + 7, 'G')):   # gems on the band
        y = top + 9.5 + tilt * (cx - x) / 12.0
        paint(img, E(KW, KH, x, y, 1.4, 1.2), c)
    for (x, y) in ((cx - 13, top + 2 + tilt), (cx + 3, top - tilt * 0.3), (cx + 13, top + 2 - tilt)):
        paint(img, E(KW, KH, x, y, 1.3, 1.3), 'W')


def re(form):
    """form: 'c' cross and pouting, 'cp' shouting "è mio!", 'piange' crying,
    'gioca' arms out to catch the ball, 'b' happy, 'bp' happy talking."""
    img = blank(KW, KH)
    cx = 24
    good = form in ('b', 'bp', 'gioca')
    # the cape behind him
    cape = P(KW, KH, [(cx - 11, 37), (cx + 11, 37), (cx + 18, 63), (cx - 18, 63)])
    put(img, cape, *CAPE)
    paint(img, P(KW, KH, [(cx - 9, 40), (cx - 5, 40), (cx - 10, 61), (cx - 15, 61)]) & cape, CAPE[1])
    # legs in white tights, golden shoes
    for fx_ in (cx - 4, cx + 4):
        put(img, R(KW, KH, fx_ - 2, 55, fx_ + 2, 63), 'W', 'w', 's')
        put(img, E(KW, KH, fx_ + (1 if fx_ > cx else -1), 64.5, 4, 2.6), 'Y', 'y', 'o')
    # arms behind the body (depending on the form)
    def arm(pts, hand_at):
        put(img, worm(KW, KH, pts, 2.8, 2.6), *TUNIC)
        put(img, E(KW, KH, hand_at[0], hand_at[1], 2.6, 2.6), *KSKIN)
    sceptre = None
    if form == 'cp':   # the right arm up with the sceptre, the left on the hip
        arm([(cx + 8, 42), (cx + 14, 36), (cx + 16, 29)], (cx + 16, 28))
        arm([(cx - 8, 42), (cx - 13, 47), (cx - 10, 51)], (cx - 9, 51))
        sceptre = ((cx + 16, 34), (cx + 17, 17))
    elif form == 'piange':   # both fists at the eyes
        for sx in (-1, 1):
            arm([(cx + sx * 8, 42), (cx + sx * 13, 36), (cx + sx * 9, 29)], (cx + sx * 8, 28))
    elif form == 'gioca':   # both arms out in front, ready to catch
        for sx in (-1, 1):
            arm([(cx + sx * 8, 42), (cx + sx * 14, 44), (cx + sx * 18, 40)], (cx + sx * 18, 39))
    elif form in ('b', 'bp'):   # arms open, glad; the sceptre in the right hand
        arm([(cx + 8, 42), (cx + 14, 40), (cx + 18, 34)], (cx + 18, 33))
        arm([(cx - 8, 42), (cx - 14, 40), (cx - 18, 34)], (cx - 18, 33))
        sceptre = ((cx + 18, 38), (cx + 20, 22))
    # the tunic, a golden belt
    tunic = E(KW, KH, cx, 47, 10, 10.5) | R(KW, KH, cx - 8, 46, cx + 8, 56, 2)
    put(img, tunic, *TUNIC)
    paint(img, R(KW, KH, cx - 9, 50, cx + 9, 51) & tunic, 'Y')
    paint(img, R(KW, KH, cx - 1, 49, cx + 1, 52) & tunic, 'o')
    if form == 'c':   # arms crossed on the tunic
        for sx in (-1, 1):
            put(img, worm(KW, KH, [(cx + sx * 9, 41), (cx + sx * 3, 46), (cx - sx * 5, 47)], 2.8, 2.6), *TUNIC)
        for sx in (-1, 1):
            put(img, E(KW, KH, cx - sx * 6, 47, 2.4, 2.2), *KSKIN)
    # the ermine collar
    collar = R(KW, KH, cx - 11, 35, cx + 11, 41, 3)
    put(img, collar, 'W', 'w', 's')
    for (x, y) in ((cx - 7, 38), (cx - 1, 37), (cx + 5, 38), (cx + 9, 37), (cx - 10, 37)):
        px(img, x, y, 'k')
    # the sceptre with a star
    if sceptre:
        (x0, y0), (x1, y1) = sceptre
        line(img, x0, y0, x1, y1, 'o', thick=2)
        from objects import star_mask_px
        put(img, star_mask_px(KW, KH, x1 + 0.5, y1 - 2, 5, 2.2), 'Y', 'y', 'o')
    # the head: curls, the face, the ears
    hy = 23
    for (x, y, r) in ((cx - 11, hy - 3, 3.4), (cx + 11, hy - 3, 3.4)):   # short curls above the ears
        put(img, E(KW, KH, x, y, r, r), *HAIR)
    for sx in (-1, 1):   # the ears
        put(img, E(KW, KH, cx + sx * 11.5, hy + 3, 2.4, 3), *KSKIN)
    facem = E(KW, KH, cx, hy + 2, 11.5, 11)
    put(img, facem, *KSKIN)
    fringe = E(KW, KH, cx, hy - 8, 12, 5) & facem
    for (x, r) in ((cx - 7, 3.6), (cx - 2, 3.8), (cx + 3, 3.8), (cx + 8, 3.6)):   # curls on the forehead
        fringe |= E(KW, KH, x, hy - 6, r, 3) & facem
    put(img, fringe, *HAIR)
    ey = hy + 2
    if form == 'piange':
        for sx in (-1, 1):   # eyes squeezed shut, tears
            ex = cx + sx * 5
            line(img, ex - 2, ey - 1, ex + 2, ey + 1 if sx < 0 else ey - 1, 'k')
            line(img, ex - 2, ey + 1 if sx < 0 else ey - 1, ex + 2, ey - 1, 'k')
            paint(img, R(KW, KH, ex + sx * 1 - 1, ey + 2, ex + sx * 1, ey + 8), 'B')
            paint(img, R(KW, KH, ex + sx * 1 - 1, ey + 2, ex + sx * 1 - 1, ey + 4), 'b')
        mouth = E(KW, KH, cx, ey + 7, 3.6, 3) & R(KW, KH, 0, ey + 5, KW - 1, KH - 1)
        put(img, mouth, '9', 'H', '9')
        paint(img, E(KW, KH, cx, ey + 9, 1.6, 0.8) & mouth, 'h')
        for sx in (-1, 1):
            paint(img, E(KW, KH, cx + sx * 8, ey + 4, 2.2, 1.4), 'c')
    else:
        look = (1, 0) if form in ('gioca',) else (0, 0)
        for sx in (-1, 1):
            ex = cx + sx * 5
            white = E(KW, KH, ex, ey, 2.6, 3)
            put(img, white, 'w', 'w', 's')
            iris = R(KW, KH, ex - 1 + look[0], ey - 2, ex + 1 + look[0], ey + 2) & white
            paint(img, iris, 'C')
            paint(img, R(KW, KH, ex + look[0], ey - 1, ex + 1 + look[0], ey + 1) & white, 'D')
            px(img, ex - 1 + look[0], ey - 1, 'w')
            if form in ('c', 'cp'):   # angry brows, down to the middle
                line(img, ex - 3 * sx, ey - 6, ex + 2 * sx, ey - 4, 'q')
            else:
                line(img, ex - 2, ey - 5, ex + 2, ey - 5, 'q')
        if form == 'c':   # the pout, puffed cheeks
            for x in range(cx - 3, cx + 4):
                px(img, x, ey + 7 + (1 if abs(x - cx) == 3 else 0) - (1 if x == cx else 0), 'k')
            for sx in (-1, 1):
                paint(img, E(KW, KH, cx + sx * 8, ey + 4, 2.6, 1.8), 'r')
        elif form == 'cp':   # "è mio!"
            put(img, E(KW, KH, cx, ey + 7, 3.4, 3), '9', 'H', '9')
            paint(img, E(KW, KH, cx, ey + 8.5, 1.8, 0.9), 'h')
            for sx in (-1, 1):
                paint(img, E(KW, KH, cx + sx * 8, ey + 4, 2.6, 1.8), 'r')
        elif form == 'bp':
            put(img, E(KW, KH, cx, ey + 7, 3.2, 2.6) & R(KW, KH, 0, ey + 6, KW - 1, KH - 1), '9', 'H', '9')
            paint(img, E(KW, KH, cx, ey + 8.5, 1.6, 0.8), 'h')
            for sx in (-1, 1):
                paint(img, E(KW, KH, cx + sx * 8, ey + 4, 2.2, 1.4), 'c')
        else:   # happy, or hopeful (gioca)
            smile(img, cx, ey + 6, 3 if form != 'gioca' else 2, tongue=form == 'b')
            for sx in (-1, 1):
                paint(img, E(KW, KH, cx + sx * 8, ey + 4, 2.2, 1.4), 'c')
    # the crown: tilted when he is upset
    _crown(img, cx, hy - 19, tilt={'piange': 2, 'c': 1, 'cp': -1}.get(form, 0))
    return img


# ------------------------------------------------------------------ the wind-up keys (20x18)
KEY_COLS = {'rossa': ('X', 'r', 'R'), 'arancione': ('@', 'F', 'O'), 'azzurra': ('B', 'b', 'n'),
            'verde': ('G', 'g', 'd'), 'oro': ('Y', 'y', 'o')}


def chiave(col):
    """A wind-up key: a handle of two round lobes with holes, and a shaft."""
    w, h = 20, 18
    img = blank(w, h)
    c = KEY_COLS[col]
    shaft = R(w, h, 8, 9, 11, 15) | R(w, h, 6, 13, 13, 16, 1)
    put(img, shaft, *c)
    lobes = E(w, h, 5.5, 6, 4.8, 5) | E(w, h, 14.5, 6, 4.8, 5) | R(w, h, 7, 3, 12, 9)
    put(img, lobes, *c)
    for x in (5.5, 14.5):
        paint(img, E(w, h, x, 6, 1.6, 2), 'k')
    paint(img, R(w, h, 3, 3, 4, 3), 'w')   # the shine
    px(img, 2, 4, 'w')
    paint(img, R(w, h, 9, 5, 10, 7), c[2])   # the hub
    return img


# ------------------------------------------------------------------ the toy train of the map (30x16)
def trenino():
    w, h = 30, 16
    img = blank(w, h)
    # the wagon (behind, on the left) with a little cargo of blocks
    put(img, R(w, h, 1, 6, 11, 12, 1), 'Y', 'y', 'o')
    put(img, R(w, h, 2, 3, 5, 6), 'B', 'b', 'n')
    put(img, R(w, h, 6, 2, 9, 6), 'G', 'g', 'd')
    line(img, 11, 10, 13, 10, 'k')   # the hook
    # the engine: cab, boiler, chimney
    put(img, R(w, h, 13, 2, 19, 12, 1), 'X', 'r', 'R')
    paint(img, R(w, h, 15, 4, 17, 6), 'b')   # the window
    put(img, R(w, h, 19, 6, 28, 12, 2), 'X', 'r', 'R')
    put(img, R(w, h, 23, 1, 26, 6), 'x', 'S', 'k')
    put(img, R(w, h, 27, 8, 29, 11), 'Y', 'y', 'o')   # the lamp
    # wheels
    for x in (4, 9, 16, 24):
        put(img, E(w, h, x, 13.5, 2.4, 2.4), 'k', 'x', 'K')
        px(img, x, 13, 'S')
    return img


# ------------------------------------------------------------------ make-up anchors (scene_trucco.c)
# measured on the drawings above: eyes (centre x, top y), mouth, cheeks, the
# crown's lower edge, the flower beside the head
TRUCCO = {
    'robottone': dict(eyes=2, eye=((25, 13), (39, 13)), mouth=(32, 25), cheek=((20, 24), (44, 24)), top=(32, 7), flower=(46, 9)),
    'saltamolla': dict(eyes=2, eye=((27, 18), (37, 18)), mouth=(32, 27), cheek=((23, 26), (41, 26)), top=(32, 13), flower=(43, 15)),
    'dinozzo': dict(eyes=2, eye=((25, 14), (39, 14)), mouth=(32, 31), cheek=((19, 26), (45, 26)), top=(32, 9), flower=(46, 12)),
    'trottolina': dict(eyes=2, eye=((27, 17), (37, 17)), mouth=(32, 27), cheek=((23, 25), (41, 25)), top=(32, 12), flower=(42, 13)),
    're': dict(eyes=2, eye=((19, 22), (29, 22)), mouth=(24, 31), cheek=((16, 29), (32, 29)), top=(24, 15), flower=(34, 16)),
}


def all_sprites():
    out = {}
    for name, fn in MONSTERS4.items():
        for form in ('c', 'r', 'b'):
            out['mo_%s_%s' % (name, form)] = fn(form)
        cut = 40 if name == 'dinozzo' else MH   # (the tip of the dinosaur's tail stays out of the icon)
        out['menu_sfida_' + name] = face_icon(fn('c')[:cut])
        out['amico_' + name] = face_icon(fn('b')[:cut])
    for form in ('c', 'cp', 'piange', 'gioca', 'b', 'bp'):
        out['re_' + form] = re(form)
    out['menu_sfida_re'] = face_icon(re('c')[:44])
    out['amico_re'] = face_icon(re('b')[:44])
    for col in KEY_COLS:
        out['chiave_' + col] = chiave(col)
    out['trenino'] = trenino()
    return out
