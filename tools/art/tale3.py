"""The third tale, "la musica perduta" (0.10.0): l'Orco Brontolone and his four
monsters (Caramellone, Fungone, Ranocchione, Trombone) and the five magic notes
(do rossa, re arancione, mi azzurra, fa verde, sol d'oro).

Same conventions as tale.py and tale2.py: monsters on a 64x64 canvas in the
forms 'c' (under the spell), 'r' (under the spell, roaring or talking) and 'b'
(freed); the ogre on 52x76 like Deva, feet on the bottom rows. The ogre is big
and grumpy but never scary: pyjamas, a night cap, fluffy slippers.
"""
import numpy as np

from pixel import blank, blit, shaded_part, crop
import tale as T
from tale import E, R, P, put, paint, px, line, eye_round, blush, smile, face_icon, MW, MH, CW, CH
from tale2 import worm, angry_eyes, fangs_mouth, happy_face
from props import Canvas, STAR_RAMPS


# ------------------------------------------------------------------ the monsters (64x64)
def caramellone(form):
    """Candy monster: a big gummy bear with a lollipop. Under the spell it is a
    dark sour candy with a broken candy cane."""
    img = blank(MW, MH)
    bad = form != 'b'
    gum = ('H', 'h', '9') if bad else ('P', 'p', 'h')
    # the lollipop (good) or the broken candy cane (spell), held on the right
    if bad:
        cane = worm(MW, MH, [(52, 56), (54, 30), (52, 22), (47, 21)], 2.2, 2.0)
        put(img, cane, 'S', 's', 'x')
        for y in range(24, 56, 5):
            paint(img, cane & (np.mgrid[0:MH, 0:MW][0] >= y) & (np.mgrid[0:MH, 0:MW][0] < y + 2), '9')
    else:
        line(img, 53, 22, 53, 50, 'W', thick=2)
        pop = E(MW, MH, 53, 16, 8, 8)
        put(img, pop, 'X', 'r', 'R')
        for k in range(60):   # the white swirl
            a = k * 0.33
            r = 7 - k * 0.11
            if r < 0.8:
                break
            px(img, int(round(53 + r * np.cos(a))), int(round(16 + r * np.sin(a))), 'w')
    # legs and ears first, the body over them
    for fx_ in (23, 41):
        put(img, E(MW, MH, fx_, 58, 7, 4.5), *gum)
    for sx in (-1, 1):
        put(img, E(MW, MH, 32 + sx * 13, 8, 5, 5), *gum)
        paint(img, E(MW, MH, 32 + sx * 13, 8.5, 2.2, 2.2), 'c' if not bad else 'h')
    body = E(MW, MH, 32, 45, 16, 14)
    put(img, body, *gum)
    paint(img, E(MW, MH, 32, 47, 9, 8), gum[1])   # the lighter belly of a gummy
    for sx in (-1, 1):   # little arms
        put(img, E(MW, MH, 32 + sx * 16, 42, 4, 6), *gum)
    head = E(MW, MH, 32, 22, 17, 15)
    put(img, head, *gum)
    # the shine of a gummy sweet
    for (x, y, r) in ((22, 13, 2.2), (25, 11, 1.2), (21, 38, 1.6)):
        paint(img, E(MW, MH, x, y, r, r), 'w')
    if bad:
        angry_eyes(img, 21, 7, iris=('7', '7', '6'), rx=4, ry=3.2)
        fangs_mouth(img, 29, 7, roar=form == 'r', fangs=(28, 36))
        for (x, y) in ((16, 30), (48, 30), (26, 52), (40, 54)):   # sour sugar grains
            px(img, x, y, 's')
    else:
        happy_face(img, 21, 28, 7, r=4)
        # a candy bow on the head
        for sx in (-1, 1):
            put(img, P(MW, MH, [(32, 6), (32 + sx * 8, 2), (32 + sx * 8, 10)]), 'Y', 'y', 'o')
        put(img, E(MW, MH, 32, 6, 2.5, 2.5), 'o', 'Y', 'O')
    return img


def fungone(form):
    """Mushroom monster: a big spotted cap over a round stem with a face."""
    img = blank(MW, MH)
    bad = form != 'b'
    cap = ('M', 'm', '$') if bad else ('X', 'r', 'R')
    stem = ('3', 's', '2') if bad else ('W', 'w', 's')
    yy, xx = np.mgrid[0:MH, 0:MW]
    # feet and the stem-body
    for fx_ in (24, 40):
        put(img, E(MW, MH, fx_, 59, 6.5, 3.5), *stem)
    body = R(MW, MH, 18, 22, 46, 58, 9)
    put(img, body, *stem)
    for sx in (-1, 1):   # little arms
        put(img, E(MW, MH, 32 + sx * 15, 42, 3.5, 5.5), *stem)
    # the cap: a wide dome with a flat underside and gills
    dome = E(MW, MH, 32, 22, 28, 18) & (yy <= 24)
    put(img, dome, *cap)
    gills = R(MW, MH, 10, 24, 54, 26, 1)
    put(img, gills, *(('2', '3', '1') if bad else ('s', 'W', 'S')))
    for x in range(13, 52, 3):
        px(img, x, 25, '2' if bad else 'S')
    # spots on the cap
    for (x, y, r) in ((18, 15, 3.2), (31, 9, 3.6), (44, 14, 3.0), (25, 20, 2.0), (39, 21, 2.2), (51, 20, 1.6),
                      (11, 21, 1.4)):
        paint(img, E(MW, MH, x, y, r, r * 0.9) & dome, '7' if bad else 'W')
    paint(img, E(MW, MH, 20, 8, 2.2, 1.4) & dome, 'm' if bad else 'r')   # a shine
    if bad:
        angry_eyes(img, 36, 6, iris=('7', '7', '6'), rx=3.4, ry=2.8)
        fangs_mouth(img, 45, 6, roar=form == 'r', fangs=(29, 35))
        if form == 'r':   # a puff of spores
            for (x, y, r) in ((8, 36, 2.0), (5, 30, 1.6), (56, 34, 1.8), (59, 28, 1.4), (10, 44, 1.2)):
                put(img, E(MW, MH, x, y, r, r), '6', '7', '5')
    else:
        happy_face(img, 37, 44, 6, r=3.6, cheeks=False)
        blush(img, 24, 43)
        blush(img, 40, 43)
        # a little sprout on the cap
        line(img, 31, 2, 31, 5, 'd')
        put(img, E(MW, MH, 28, 2, 3, 1.8), 'G', 'g', 'd')
        put(img, E(MW, MH, 35, 2, 3, 1.8), 'G', 'g', 'd')
    return img


def ranocchione(form):
    """Frog monster: round and wide, the eyes up on top. Under the spell its
    croak blows up a big throat pouch."""
    img = blank(MW, MH)
    bad = form != 'b'
    skin = ('5', '6', '4') if bad else ('G', 'g', 'd')
    belly = ('6', '6', '5') if bad else ('u', 'y', 'j')
    # back legs folded at the sides, front feet
    for sx in (-1, 1):
        put(img, E(MW, MH, 32 + sx * 22, 50, 9, 8), *skin)
        put(img, E(MW, MH, 32 + sx * 26, 59, 7, 3), *skin)
    for fx_ in (24, 40):
        put(img, E(MW, MH, fx_, 60, 5, 3), *skin)
    body = E(MW, MH, 32, 40, 24, 19)
    put(img, body, *skin)
    paint(img, E(MW, MH, 32, 47, 15, 10) & body, belly[1])
    for (x, y) in ((22, 30), (44, 28), (38, 34), (16, 42), (48, 44)):   # spots
        paint(img, E(MW, MH, x, y, 1.8, 1.4) & body, skin[2])
    # the eyes on top
    for sx in (-1, 1):
        put(img, E(MW, MH, 32 + sx * 12, 21, 9, 8), *skin)
    if bad:
        for sx in (-1, 1):
            ex = 32 + sx * 12
            put(img, E(MW, MH, ex, 21, 5.5, 4.5), '7', '7', '6')
            paint(img, R(MW, MH, ex - 2, 21, ex + 2, 22), 'k')
            for i in range(8):   # heavy brows sloping to the middle
                bx = ex - 4 + i if sx < 0 else ex + 4 - i
                px(img, bx, 14 + i // 2, 'K')
                px(img, bx, 15 + i // 2, 'K')
        if form == 'r':   # croak: the throat pouch
            pouch = E(MW, MH, 32, 50, 12, 9)
            put(img, pouch, 'H', 'h', '9')
            paint(img, E(MW, MH, 28, 47, 3, 2), 'p')
            for x in range(20, 45):
                px(img, x, 40 + (1 if x in (20, 21, 43, 44) else 0), 'k')
        else:
            fangs_mouth(img, 38, 12, roar=False, fangs=(25, 39))
    else:
        for sx in (-1, 1):
            eye_round(img, 32 + sx * 12, 21, 5, (-sx, 0))
        blush(img, 17, 36)
        blush(img, 47, 36)
        for x in range(21, 44):   # a wide happy smile
            px(img, x, 37 + (1 if 24 <= x <= 40 else 0) + (1 if 28 <= x <= 36 else 0), 'k')
        for dx in (-1, 0, 1):
            px(img, 32 + dx, 40, 'h')
        # a little pink flower between the eyes
        for i in range(5):
            a = np.deg2rad(i * 72 - 90)
            paint(img, E(MW, MH, 32 + 2.6 * np.cos(a), 13 + 2.6 * np.sin(a), 1.8, 1.8), 'P')
        paint(img, E(MW, MH, 32, 13, 1.2, 1.2), 'Y')
    return img


def trombone(form):
    """Circus monster: a little elephant whose trunk ends in a golden trumpet,
    with a red fez. Under the spell the trumpet is dull and the trunk droops."""
    img = blank(MW, MH)
    bad = form != 'b'
    skin = ('2', '3', '1') if bad else ('L', 'l', 'm')
    ear_in = ('x', '2', '1') if bad else ('p', 'c', 'P')
    brass = ('S', 's', 'x') if bad else ('Y', 'y', 'o')
    # big ears at the sides
    for sx in (-1, 1):
        ear = E(MW, MH, 32 + sx * 20, 26, 12, 14)
        put(img, ear, *skin)
        paint(img, E(MW, MH, 32 + sx * 21, 27, 7, 9) & ear, ear_in[1])
    # feet and body
    for fx_ in (23, 41):
        put(img, R(MW, MH, fx_ - 6, 52, fx_ + 6, 62, 3), *skin)
        for tx in (-3, 0, 3):
            px(img, fx_ + tx, 61, 'w' if not bad else '3')
    body = E(MW, MH, 32, 46, 16, 12)
    put(img, body, *skin)
    head = E(MW, MH, 32, 26, 15, 14)
    put(img, head, *skin)
    # the trunk: up and curled (good, or trumpeting), down (spell, quiet); a trumpet bell at its end
    if bad and form != 'r':
        trunk = worm(MW, MH, [(32, 32), (33, 42), (37, 49), (42, 52)], 4.2, 3.0)
        flare = P(MW, MH, [(41, 49.5), (43, 55), (52, 60), (54, 50)])
        mouth_c, mouth_r = (53, 55), (2.6, 5.2)
    else:
        trunk = worm(MW, MH, [(32, 32), (34, 40), (42, 40), (47, 32), (49, 26)], 4.2, 3.0)
        flare = P(MW, MH, [(46, 27), (52, 28), (61, 16), (53, 11)])
        mouth_c, mouth_r = (57, 13.5), (4.8, 3.2)
    put(img, trunk, *skin)
    for (x, y) in ((33, 37), (35, 41), (40, 41)) if not (bad and form != 'r') else ((33, 38), (34, 43), (37, 47)):
        px(img, x, y, skin[2])   # the wrinkles of the trunk
    put(img, flare | E(MW, MH, mouth_c[0], mouth_c[1], mouth_r[0], mouth_r[1]), *brass)
    paint(img, E(MW, MH, mouth_c[0], mouth_c[1], mouth_r[0] - 1.6, mouth_r[1] - 1.6), brass[2])
    bell_c = mouth_c
    if form == 'r':   # the trumpet blows: sound lines
        for (dx, dy) in ((4, -8), (7, -4), (7, 1)):
            line(img, bell_c[0] + dx - 2, bell_c[1] + dy, bell_c[0] + dx, bell_c[1] + dy - 2, 'k')
    if bad:
        angry_eyes(img, 24, 7, iris=('7', '7', '6'), rx=3.2, ry=2.6)
        if form == 'r':
            paint(img, E(MW, MH, 25, 34, 3, 2.4), '9')
    else:
        eye_round(img, 25, 24, 3.6, (1, 0))
        eye_round(img, 39, 24, 3.6, (-1, 0))
        blush(img, 21, 31)
        blush(img, 43, 31)
        smile(img, 26, 34, 2, tongue=False)
    # the fez, with a golden tassel
    fez = P(MW, MH, [(25, 13), (39, 13), (37, 4), (27, 4)])
    put(img, fez, *(('1', '2', '0') if bad else ('X', 'r', 'R')))
    line(img, 32, 4, 37, 8, 'o' if not bad else '3')
    put(img, E(MW, MH, 38, 10, 1.6, 2.2), *(brass if not bad else ('3', 's', '2')))
    return img


MONSTERS3 = {'caramellone': caramellone, 'fungone': fungone, 'ranocchione': ranocchione, 'trombone': trombone}


# ------------------------------------------------------------------ l'Orco Brontolone (64x88)
SKIN = ('#9fd08a', '#c6e8b0', '#6fa35e')      # mint green, never scary
PYJAMA = ('b', 'w', 'n')
OW, OH = 64, 88   # bigger than Deva: an ogre (feet on the bottom rows, like hers)


def _eye(img, ex, ey, lid=0, look=0):
    """An ogre eye: white, a dark pupil, a glint; lid = rows covered by the eyelid."""
    white = E(OW, OH, ex, ey, 2.6, 3.2)
    put(img, white, 'w', 'w', 's')
    paint(img, R(OW, OH, ex - 1 + look, ey - 1, ex + look, ey + 1) & white, 'k')
    px(img, ex - 1 + look, ey - 1, 'w')
    if lid:
        paint(img, white & R(OW, OH, 0, 0, OW - 1, ey - 4 + lid), SKIN[2])
        line(img, ex - 3, ey - 3 + lid, ex + 3, ey - 3 + lid, 'k')


def orco(form):
    """form: 'c' grumpy, 'cp' grumpy shouting, 'soffia' blowing the music away,
    'brontola' stamping and puffing (the last question of the duel), 'stanco'
    tired, 'stanco_p' yawning, 'assonnato' falling asleep to the lullaby (0.12),
    'dorme' asleep, 'b' happy, 'bp' happy talking."""
    img = blank(OW, OH)
    good = form in ('b', 'bp')
    cx = 32 + (2 if form == 'dorme' else 0)   # asleep, the head leans
    # fluffy slippers and legs
    for fx_ in (21, 43):
        put(img, R(OW, OH, fx_ - 6, 68, fx_ + 6, 82, 4), *PYJAMA)
        put(img, E(OW, OH, fx_, 84, 9, 4), 'p', 'W', 'P')
        paint(img, E(OW, OH, fx_ - 3, 82.5, 1.8, 1.3), 'W')
    # arms hanging behind the belly (tired, asleep, happy talking) or up (stamping)
    if form == 'brontola':
        for sx in (-1, 1):
            arm = worm(OW, OH, [(32 + sx * 18, 50), (32 + sx * 25, 42), (32 + sx * 27, 32)], 5, 4.2)
            put(img, arm, *PYJAMA)
            put(img, E(OW, OH, 32 + sx * 27, 29, 5, 5), *SKIN)
    elif form in ('stanco', 'stanco_p', 'assonnato', 'dorme', 'soffia'):
        for sx in (-1, 1):
            arm = worm(OW, OH, [(32 + sx * 19, 48), (32 + sx * 25, 58), (32 + sx * 26, 66)], 5, 4.2)
            put(img, arm, *PYJAMA)
            put(img, E(OW, OH, 32 + sx * 26, 68, 4.5, 4.5), *SKIN)
    elif good:
        for sx in (-1, 1):   # arms open, glad
            arm = worm(OW, OH, [(32 + sx * 19, 48), (32 + sx * 27, 50), (32 + sx * 30, 42)], 5, 4.2)
            put(img, arm, *PYJAMA)
            put(img, E(OW, OH, 32 + sx * 30, 39, 4.5, 4.5), *SKIN)
    # the big round belly in pyjamas, with little moons
    belly = E(OW, OH, 32, 58, 26, 19)
    put(img, belly, *PYJAMA)
    for (x, y) in ((17, 54), (37, 49), (27, 66), (46, 62), (13, 64), (50, 52)):
        m = E(OW, OH, x, y, 2.6, 2.6) & ~E(OW, OH, x + 1.3, y - 0.9, 2.2, 2.2)
        paint(img, m & belly, 'Y')
    for y in (47, 54, 61, 68):   # buttons
        paint(img, E(OW, OH, 32, y, 1.3, 1.3) & belly, 'n')
    if form in ('c', 'cp'):   # arms crossed on the belly
        for sx in (-1, 1):
            arm = worm(OW, OH, [(32 + sx * 22, 46), (32 + sx * 10, 55), (32 - sx * 8, 57)], 5, 4.6)
            put(img, arm, *PYJAMA)
            put(img, E(OW, OH, 32 - sx * 10, 57, 4.2, 4), *SKIN)
    # ears and head
    for sx in (-1, 1):
        ear = P(OW, OH, [(cx + sx * 15, 26), (cx + sx * 25, 17), (cx + sx * 17, 36)])
        put(img, ear, *SKIN)
        paint(img, P(OW, OH, [(cx + sx * 16, 27), (cx + sx * 22, 21), (cx + sx * 17, 32)]), '#c9a0a8')
    face = E(OW, OH, cx, 31, 18, 16)
    put(img, face, *SKIN)
    # eyes and brows
    if form in ('c', 'cp', 'brontola'):
        for sx in (-1, 1):
            ex = cx + sx * 7
            _eye(img, ex, 27, lid=2)
            line(img, ex - 5 * sx, 20, ex + 3 * sx, 23, 'Q', thick=1)   # bushy brows, down to the middle
            line(img, ex - 5 * sx, 21, ex + 3 * sx, 24, 'Q', thick=1)
    elif form == 'soffia':
        for sx in (-1, 1):   # squeezed shut
            ex = cx + sx * 7
            line(img, ex - 3, 27, ex + 3, 26 if sx > 0 else 28, 'k')
            line(img, ex - 3, 23, ex + 3, 22, 'Q')
    elif form in ('stanco', 'stanco_p'):
        for sx in (-1, 1):   # heavy eyelids and tired rings
            ex = cx + sx * 7
            _eye(img, ex, 27, lid=4)
            line(img, ex - 2, 31, ex + 2, 31, '#7e9e8c')
            line(img, ex - 4, 22, ex + 3, 22, 'Q')
    elif form == 'assonnato':
        for sx in (-1, 1):   # nearly closed: a thin slit under a heavy lid
            ex = cx + sx * 7
            line(img, ex - 4, 26, ex + 3, 26, '#5f9150')
            line(img, ex - 3, 27, ex + 3, 27, 'k')
            px(img, ex, 28, 'k')
            line(img, ex - 4, 22, ex + 3, 22, 'Q')
    elif form == 'dorme':
        for sx in (-1, 1):   # closed, peaceful
            ex = cx + sx * 7
            for dx, dy in ((-3, 0), (-2, 1), (-1, 1), (0, 1), (1, 1), (2, 1), (3, 0)):
                px(img, ex + dx, 27 + dy, 'k')
    else:   # happy
        for sx in (-1, 1):
            ex = cx + sx * 7
            _eye(img, ex, 27, look=-sx)
            line(img, ex - 3, 21, ex + 2, 21, 'q')
    # the big nose
    put(img, E(OW, OH, cx, 34, 5.5, 4.2), '#8cc276', '#b4dc9c', '#5f9150')
    # cheeks
    if form == 'brontola':
        for sx in (-1, 1):
            paint(img, E(OW, OH, cx + sx * 12, 36, 3.5, 2.2), 'r')
    elif form == 'soffia':
        for sx in (-1, 1):   # puffed cheeks
            put(img, E(OW, OH, cx + sx * 11, 38, 6, 5.5), *SKIN)
            paint(img, E(OW, OH, cx + sx * 11, 39, 2.4, 1.6), 'c')
    elif good or form in ('dorme', 'assonnato'):
        for sx in (-1, 1):
            paint(img, E(OW, OH, cx + sx * 12, 37, 2.6, 1.6), 'c')
    # mouth, with two little blunt tusks
    my = 41
    if form in ('cp', 'brontola'):
        m = E(OW, OH, cx, my + 1, 6, 3.5) & R(OW, OH, 0, my - 1, OW - 1, OH - 1)
        put(img, m, '9', 'H', '9')
        paint(img, E(OW, OH, cx, my + 3, 3, 1.2) & m, 'h')
    elif form == 'soffia':
        put(img, E(OW, OH, cx, my + 1, 2.4, 2.4), '9', 'H', '9')
    elif form == 'stanco_p':   # a big yawn
        put(img, E(OW, OH, cx, my + 2, 4.5, 5.5), '9', 'H', '9')
        paint(img, E(OW, OH, cx, my + 5, 2.5, 1.6), 'h')
    elif form == 'bp':
        put(img, E(OW, OH, cx, my + 1, 5.5, 3.6), '9', 'H', '9')
        paint(img, E(OW, OH, cx, my + 3, 2.8, 1.2) & E(OW, OH, cx, my + 1, 5.5, 3.6), 'h')
    elif form == 'assonnato':   # calm now, a small sleepy smile
        smile(img, cx, my, 3, tongue=False)
    elif good or form == 'dorme':
        smile(img, cx, my, 5, tongue=form != 'dorme')
    else:   # the grumpy frown
        for x in range(cx - 6, cx + 7):
            px(img, x, my + (1 if abs(x - cx) >= 5 else 0) - (1 if abs(x - cx) <= 2 else 0), 'k')
    if form not in ('soffia', 'stanco_p'):
        for sx in (-1, 1):
            paint(img, P(OW, OH, [(cx + sx * 4 - 1.2, my + 1), (cx + sx * 4 + 1.2, my + 1), (cx + sx * 4, my - 2.5)]), 'W')
    # the night cap, drooping to one side (asleep: to the other)
    capm = P(OW, OH, [(cx - 16, 20), (cx + 16, 20), (cx + 12, 10), (cx + 4, 4), (cx - 8, 6)])
    tip = (cx + 22, 16) if form != 'dorme' else (cx - 23, 15)
    capm |= worm(OW, OH, [(cx + 3, 5), ((cx + 3 + tip[0]) / 2, 2), tip], 3.8, 1.8)
    put(img, capm, 'B', 'b', 'n')
    for k in range(4):   # stripes
        band = worm(OW, OH, [(cx - 13 + k * 8, 19), (cx - 9 + k * 8, 7)], 1.3, 1.3) & capm
        paint(img, band, 'W')
    put(img, E(OW, OH, tip[0], tip[1] + 1, 3.8, 3.8), 'W', 'w', 's')   # pompom
    put(img, R(OW, OH, cx - 17, 18, cx + 17, 22, 2), 'W', 'w', 's')   # the brim
    if form == 'brontola':   # puffs of anger
        for (x, y, r) in ((7, 12, 3.4), (4, 5, 2.6), (57, 12, 3.4), (60, 5, 2.6)):
            put(img, E(OW, OH, x, y, r, r * 0.8), 's', 'W', '3')
    return img


# ------------------------------------------------------------------ the magic notes
NOTE_RAMPS = {'rossa': STAR_RAMPS['rossa'], 'arancione': ['#7a2a08', '#c24a12', '#ff7b2e', '#ffa860', '#ffe0c0'],
              'azzurra': STAR_RAMPS['azzurra'], 'verde': STAR_RAMPS['verde'], 'oro': STAR_RAMPS['oro']}


def nota(col):
    """A magic note (18x22): an eighth note with a round head that smiles."""
    w, h = 18, 22
    c = Canvas(w, h)
    ramp = NOTE_RAMPS[col]
    head = c.ell(7, 16.5, 6.2, 4.8)
    stem = c.rect(11, 3, 12, 15)
    flag = c.poly([(12, 2), (17, 7), (17, 11), (14, 8), (12, 7)])
    whole = head | stem | flag
    c.ring(whole)
    c.paint(stem | flag, ramp[1])
    c.paint(c.rect(11, 3, 11, 14) & stem, ramp[2])
    c.ball(7, 16.5, 6.2, 4.8, [ramp[1], ramp[2], ramp[3]], band=0.6)
    c.paint(c.rect(4, 13, 5, 13) | c.rect(3, 14, 3, 14), ramp[4])   # the glint
    c.paint(c.rect(5, 16, 5, 17) | c.rect(9, 16, 9, 17), '#3b1f4a')   # eyes
    c.paint(c.rect(6, 19, 8, 19), '#3b1f4a')                          # smile
    c.paint(c.rect(4, 18, 4, 18) | c.rect(10, 18, 10, 18), '#ff93c6')   # cheeks
    return c.img


NOTES = ('rossa', 'arancione', 'azzurra', 'verde', 'oro')


# ------------------------------------------------------------------ make-up anchors (for scene_trucco.c)
# measured on the drawings above: eyes (the lower edge of the eyeshadow), mouth,
# cheeks, the crown's lower edge, the flower
TRUCCO = {
    'caramellone': dict(eyes=2, eye=((25, 18), (39, 18)), mouth=(32, 30), cheek=((21, 27), (43, 27)), top=(32, 6), flower=(44, 9)),
    'fungone': dict(eyes=2, eye=((31, 34), (43, 34)), mouth=(37, 45), cheek=((24, 43), (40, 43)), top=(32, 5), flower=(45, 10)),
    'ranocchione': dict(eyes=2, eye=((20, 17), (44, 17)), mouth=(32, 38), cheek=((17, 36), (47, 36)), top=(32, 13), flower=(46, 9)),
    'trombone': dict(eyes=2, eye=((25, 21), (39, 21)), mouth=(26, 35), cheek=((21, 31), (43, 31)), top=(32, 5), flower=(20, 10)),
    'orco': dict(eyes=2, eye=((20, 19), (32, 19)), mouth=(26, 34), cheek=((16, 30), (36, 30)), top=(26, 4), flower=(35, 9)),
}


def all_sprites():
    out = {}
    for name, fn in MONSTERS3.items():
        for form in ('c', 'r', 'b'):
            out['mo_%s_%s' % (name, form)] = fn(form)
        out['menu_sfida_' + name] = face_icon(fn('c'))
        out['amico_' + name] = face_icon(fn('b'))
    for form in ('c', 'cp', 'soffia', 'brontola', 'stanco', 'stanco_p', 'assonnato', 'dorme', 'b', 'bp'):
        out['orco_' + form] = orco(form)
    out['menu_sfida_orco'] = face_icon(orco('c')[:50])
    out['amico_orco'] = face_icon(orco('b')[:50])
    for col in NOTES:
        out['nota_' + col] = nota(col)
    for i, (fill, light) in enumerate((('P', 'p'), ('B', 'b'), ('Y', 'y'))):
        out['notina_%d' % i] = notina(fill, light)
    return out


NOTINA = [
    "...kk..",
    "...kxk.",
    "...kxxk",
    "...kxkk",
    "...kxk.",
    ".kkkxk.",
    "kxhxxk.",
    "kxxxxk.",
    ".kkkk..",
]


def notina(fill, light):
    """A tiny musical note (7x9) floating over the valley when it sings again."""
    from pixel import ascii_sprite, PALETTE
    pal = dict(PALETTE)
    pal['x'] = PALETTE[fill]
    pal['h'] = PALETTE[light]
    return ascii_sprite(NOTINA, pal)
