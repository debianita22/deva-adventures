"""Deva, the heroine: a chibi girl with long wavy honey-brown hair, sun-kissed
tips, a braided strand of coloured threads, brown eyes, a pink choker, a
pink-and-white striped tee and a lilac tulle skirt.

Parts are exported as separate sprites with their offsets on a 44x68 canvas
(feet on row 63, centre x = 22); the runtime layers them:
hair_back, body, head, eyeshadow, eyes, blush, mouth, sticker, hair_front,
head accessory, glitter.
"""
import numpy as np
from pixel import (ascii_sprite, blank, blit, flip_h, ellipse_mask, capsule_mask,
                   rect_mask, shaded_part, recolor, rgb, dilate4)

CANVAS_W, CANVAS_H = 44, 68
FEET_Y = 63
W, H = CANVAS_W, CANVAS_H
YY, XX = np.mgrid[0:H, 0:W]
PX, PY = XX + 0.5, YY + 0.5

EYE_POS = ((13, 25), (25, 25))    # canvas, top-left of each eye
MOUTH_POS = (19, 34)
BLUSH_POS = ((11, 32), (29, 32))
SHADOW_POS = ((13, 24), (25, 24))
STICKER_POS = (27, 31)
TIARA_POS = (15, 4)
FLOWER_POS = (6, 13)


def outline(mask, color='k'):
    img = blank(W, H)
    ring = dilate4(mask) & ~mask
    img[ring, :3] = rgb(color)
    img[ring, 3] = 255
    return img


def paint(img, mask, c):
    img[mask, :3] = rgb(c)
    img[mask, 3] = 255


# ------------------------------------------------------------------ hair
RNG = np.random.default_rng(4)
_TIP_JITTER = RNG.uniform(-0.8, 0.8, 16)


def _dome(cx=22.0, cy=20.0, rx=15.0, ry=15.0):
    return ((PX - cx) / rx) ** 2 + ((PY - cy) / ry) ** 2 <= 1.0


def _back_mask():
    dy = PY - 20.0
    half = 15.0 + 1.1 * np.sin(dy * 0.42) + 0.075 * np.clip(dy, 0, None)
    col = np.clip(((PX - 5.0) / 34.0 * 15).astype(int), 0, 15)
    hem = 44.6 + 2.3 * np.abs(np.cos(np.pi * (PX - 5.5) / 5.1)) + _TIP_JITTER[col]
    body = (np.abs(PX - 22.0) <= half) & (PY >= 20.0) & (PY <= hem)
    return _dome() | body


def hair_back_sprite():
    m = _back_mask()
    img = blank(W, H)
    paint(img, m, 'J')
    dx = np.abs(PX - 22.0)
    paint(img, m & (dx < 12) & (PY > 31), 'q')          # shadow behind the neck
    paint(img, m & (PY > 39), 'j')                       # lighter lengths
    paint(img, m & (PY > 42.5), 'u')                     # sun-kissed tips
    # soft wave lines following the hair flow
    for x0 in (7.5, 10.5, 33.5, 36.5):
        for y in range(21, 43):
            x = int(round(x0 + 0.9 * np.sin(y * 0.42 + (0 if x0 < 22 else 1.2))))
            if m[y, x] and not (dx[y, x] < 12 and y > 31):
                img[y, x, :3] = rgb('q' if y < 39 else 'J')
    for y in range(H):                                   # rim light on the left
        xs = np.where(m[y])[0]
        if len(xs) and y > 12:
            img[y, xs[0], :3] = rgb('j' if y < 39 else 'u')
    blit(img, outline(m), 0, 0)
    return img


def hairline(px):
    """Top edge of the visible face: middle part, hair sweeping to the temples."""
    d = np.minimum(np.abs(px - 22.0), 11.5)
    return 16.5 + 7.0 * (d / 11.5) ** 1.8


def _lock(side):
    """A face-framing lock from the temple to just below the chin."""
    sgn = -1 if side == 'l' else 1
    m = np.zeros((H, W), dtype=bool)
    for y in range(17, 40):
        t = (y - 17) / 22.0
        cx = 22 + sgn * (11.6 - 1.2 * t + 0.7 * np.sin(y * 0.5))
        half = 2.1 - 0.6 * t
        m[y] |= np.abs(PX[y] - cx) <= half
    return m


def hair_front_sprite():
    cap = _dome(cy=20.0, rx=15.4, ry=15.4) & (PY < hairline(PX))
    lock_l, lock_r = _lock('l'), _lock('r')
    m = cap | lock_l | lock_r
    img = blank(W, H)
    paint(img, m, 'J')
    dx = PX - 22.0
    # middle part and the shadow along the hairline
    for y in range(6, 11):
        img[y, 21, :3] = rgb('q')
    edge = cap & ~((PY + 1) < hairline(PX))
    paint(img, edge & (np.abs(dx) > 2), 'q')
    # glossy crown band, curved like the head
    band = _dome(cy=20.0, rx=12.6, ry=12.4) & ~_dome(cy=20.0, rx=11.2, ry=11.0) & (PY < 12.5) & cap
    paint(img, band, 'j')
    paint(img, band & (np.abs(dx) > 3.5) & (np.abs(dx) < 7.5), 'u')
    # locks: inner shadow, lighter tips
    for lk, sgn in ((lock_l, -1), (lock_r, 1)):
        for y in range(17, 40):
            xs = np.where(lk[y])[0]
            if len(xs):
                inner = xs[-1] if sgn < 0 else xs[0]
                img[y, inner, :3] = rgb('q')
                if y > 35:
                    img[y, xs, :3] = rgb('j')
    # braid of coloured threads inside her left lock (viewer's right): a twisted wrap
    cols = ['h', 'Y', 'G']
    for y in range(23, 39):
        xs = np.where(lock_r[y])[0]
        if len(xs) >= 2:
            c = xs[len(xs) // 2]
            k = (y // 2) % 3
            img[y, c - 1, :3] = rgb(cols[k])
            img[y, c, :3] = rgb(cols[(k + 1) % 3])
    tassel = [(39, 0), (40, 0), (40, -1), (41, -1)]
    xs = np.where(lock_r[38])[0]
    cx = xs[len(xs) // 2] if len(xs) else 33
    m2 = img[..., 3] > 0
    for yy, ddx in tassel:
        img[yy, cx + ddx, :3] = rgb('h')
        img[yy, cx + ddx, 3] = 255
        m2[yy, cx + ddx] = True
    blit(img, outline(m2), 0, 0)
    return img


# ------------------------------------------------------------------ face
def head_sprite():
    face = (((PX - 22.0) / 11.2) ** 2 + ((PY - 27.0) / 11.2) ** 2 <= 1.0) | \
           (((PX - 22.0) / 10.2) ** 2 + ((PY - 31.0) / 7.6) ** 2 <= 1.0)
    img = blank(W, H)
    paint(img, face, 'a')
    # soft shading: right and bottom rim, shadow under the fringe
    for y in range(H):
        xs = np.where(face[y])[0]
        if len(xs):
            img[y, xs[-1], :3] = rgb('A')
    for x in range(W):
        ys = np.where(face[:, x])[0]
        if len(ys):
            img[ys[-1], x, :3] = rgb('A')
    under = face & (PY < hairline(PX) + 1.6) & (PY > hairline(PX))
    paint(img, under, 'A')
    # eyebrows (thin, hair colour) and tiny nose
    for x in range(14, 18):
        img[22 if x in (15, 16) else 23, x, :3] = rgb('q')
    for x in range(26, 30):
        img[22 if x in (27, 28) else 23, x, :3] = rgb('q')
    img[32, 21, :3] = rgb('A')
    img[32, 22, :3] = rgb('A')
    for (x, y) in ((13, 32), (14, 32), (29, 32), (30, 32)):
        img[y, x, :3] = rgb('c')
    blit(img, outline(face), 0, 0)
    # neck (drawn with the head so it moves with it)
    neck = (PX >= 19.5) & (PX <= 24.5) & (PY >= 37) & (PY <= 41)
    paint(img, neck & ~face, 'A')
    return img


EYE_OPEN = [
    ".kkkk.",
    "kkkkkk",
    "kwwCCk",
    "kwCCCk",
    "kCCDCk",
    "kCDDwk",
    ".kkkk.",
]
EYE_CLOSED = [
    "......",
    "......",
    "......",
    "k....k",
    ".kkkk.",
    "......",
    "......",
]
EYE_HAPPY = [
    "......",
    "......",
    "..kk..",
    ".k..k.",
    "k....k",
    "......",
    "......",
]
# expressions for "Le emozioni di Deva"
# left eye; the right one is its mirror (hero_eye_*_r): lids slanting up to the
# middle (sad) or down to the middle (angry)
EYE_SAD = ["......", "....kk", "..kkkk", "kkkCwk", "kCDDCk", "kCDDCk", ".kkkk."]
EYE_ANGRY = ["......", "kk....", "kkkk..", "kwCkkk", "kCDDCk", "kCDDwk", ".kkkk."]
EYE_WIDE = [".kkkk.", "kwwwwk", "kwCCwk", "kwDDwk", "kwCCwk", "kwwwwk", ".kkkk."]
MOUTH_FROWN = [".kkkk.", "k....k"]
MOUTH_GRIT = ["kkkkkk", "kwwwwk", "kkkkkk"]
MOUTH_WIDE = [".kkkk.", "khhhhk", "khhhhk", ".kkkk."]
TEAR = [".b.", "bBb", "bBb", ".b."]
TEAR_POS = (14, 31)
# scared: a drop of sweat on the temple (drawn over the hair) and a wobbly mouth
SWEAT = ["..b.", ".bb.", ".bBb", "bBwb", "bBBb", ".bb."]
SWEAT_POS = (29, 20)
MOUTH_WAVY = [".k..k.", "k.kk.k"]
BROW_POS = (13, 21)             # the brow box: x 13..30, y 21..24
# left brow in its 6x4 box ('q' hair colour); the right one is the mirror
BROWS = {
    'sad':   ["....qq", "..qq..", "qq....", "......"],
    'angry': ["......", "qq....", "..qq..", "....qq"],
    'up':    ["..qq..", ".q..q.", "......", "......"],
}

MOUTH_SMILE = ["k....k", ".kkkk."]
MOUTH_OPEN = ["kkkkkk", "kwwwwk", "khhhhk", ".kkkk."]
MOUTH_OH = ["..kk..", ".khhk.", "..kk.."]
BLUSH = [".cc.", "cccc"]
EYESHADOW = [".zzzz.", "z....z"]

STICKER_STAR = ["..k..", ".kYk.", "kYyYk", ".kYk.", "..k.."]
STICKER_HEART = [".k.k.", "khkhk", "khhhk", ".khk.", "..k.."]
TIARA = [
    "..k...k...k..",
    ".kYk.kYk.kYk.",
    "kYyYkYhYkYyYk",
    "kYYYYYYYYYYYk",
    "kYBYYYhYYYBYk",
    ".kkkkkkkkkkk.",
]
FLOWER = [
    "..kk.kk..",
    ".kPPkPPk.",
    "kPPpPpPPk",
    ".kPpYpPk.",
    "kPPpPpPPk",
    ".kPPkPPk.",
    "..kk.kk..",
]
SPARKLE = ["..w..", "..y..", "wy.yw", "..y..", "..w.."]
HAND = [".kk.", "kaak", "kaAk", ".kk."]

EYESHADOW_COLORS = {'rosa': 'P', 'azzurro': 'B', 'viola': 'v', 'oro': 'Y'}
LIPSTICK_COLORS = {'rosso': 'X', 'rosa': 'h', 'viola': 'V'}


# ------------------------------------------------------------------ body
def _limb(canvas, p0, p1, r, base, light, shadow):
    m = capsule_mask(W, H, p0, p1, r)
    blit(canvas, shaded_part(m, base, light, shadow), 0, 0)


def _blob(canvas, cx, cy, rx, ry, base, light, shadow):
    m = ellipse_mask(W, H, cx, cy, rx, ry)
    blit(canvas, shaded_part(m, base, light, shadow), 0, 0)


def _shirt(canvas, dy=0):
    m = rect_mask(W, H, 16, 38 + dy, 27, 48 + dy, radius=3)
    part = shaded_part(m, 'h', None, None)
    for y in range(H):
        if ((y - dy) // 2) % 2 == 1:
            part[y][m[y]] = (*rgb('W'), 255)
    # round neckline + pink choker
    for x in range(19, 25):
        part[38 + dy, x, :3] = rgb('a')
    blit(canvas, part, 0, 0)
    for x in range(20, 24):
        canvas[39 + dy, x, :3] = rgb('P' if x % 2 else 'h')
        canvas[39 + dy, x, 3] = 255


def _skirt(canvas, dy=0):
    """Lilac tulle skirt, A-line, wavy hem, tiny sparkles."""
    xr = PX - 22.0
    top, bottom = 46.0 + dy, 54.2 + dy
    t = np.clip((PY - top) / (bottom - top), 0, 1)
    half = 7.2 + (13.6 - 7.2) * np.sqrt(t)
    hem = bottom + 0.9 * np.cos(2 * np.pi * np.abs(xr) / 4.5)
    m = (PY >= top) & (PY <= hem) & (np.abs(xr) <= half)
    part = shaded_part(m, 'L', 'l', 'm')
    # layered tulle: a lighter top layer
    top_layer = m & (PY < top + 3.2)
    for y, x in zip(*np.where(top_layer)):
        if part[y, x, :3].tolist() == list(rgb('L')):
            part[y, x, :3] = rgb('l')
    for (x, y) in ((14, 50), (19, 52), (25, 49), (29, 52), (22, 53), (12, 53)):
        if m[y + dy, x] and part[y + dy, x, :3].tolist() != list(rgb('k')):
            part[y + dy, x, :3] = rgb('w')
    blit(canvas, part, 0, 0)


POSES = {
    #         left hand     right hand    left foot       right foot      knees   torso dy
    'idle':  dict(lh=(12, 48), rh=(32, 48), lf=(18.5, 61.5), rf=(25.5, 61.5)),
    'idle2': dict(lh=(12, 49), rh=(32, 49), lf=(18.5, 61.5), rf=(25.5, 61.5)),
    'up':    dict(lh=(4, 31),  rh=(40, 31), lf=(18.5, 61.5), rf=(25.5, 61.5)),
    'step':  dict(lh=(4, 31),  rh=(37, 45), lf=(20.5, 58.0), rf=(25.5, 61.5), lk=(13.5, 56.0)),
    'jump':  dict(lh=(4, 31),  rh=(40, 31), lf=(16.5, 61.5), rf=(27.5, 61.5)),
    'giu':   dict(lh=(15, 52), rh=(29, 52), lf=(16.5, 61.5), rf=(27.5, 61.5), lk=(13.0, 57.0), rk=(31.0, 57.0),
                  dy=4),
    # 0.13: walking towards us (one foot up, the other arm swinging forward), clapping her hands
    'walk_l': dict(lh=(12, 50), rh=(29, 46), lf=(19.0, 58.5), rf=(25.5, 61.5), lk=(16.5, 56.5)),
    'walk_r': dict(lh=(15, 46), rh=(32, 50), lf=(18.5, 61.5), rf=(25.0, 58.5), rk=(27.5, 56.5)),
    'clap_open': dict(lh=(15, 45), rh=(29, 45), lf=(18.5, 61.5), rf=(25.5, 61.5)),
    'clap':  dict(lh=(20.5, 44), rh=(23.5, 44), lf=(18.5, 61.5), rf=(25.5, 61.5)),
}
HEAD_DY = {'idle': 0, 'idle2': 1, 'up': 0, 'step': 0, 'jump': 0, 'giu': 4, 'walk_l': 0, 'walk_r': 0,
           'clap_open': 0, 'clap': 0}


def body_sprite(pose):
    p = POSES[pose]
    dy = p.get('dy', 0)
    c = blank(W, H)
    for hip, foot, knee in (((19.8, 53.0 + dy), p['lf'], p.get('lk')),
                            ((24.2, 53.0 + dy), p['rf'], p.get('rk'))):
        end = (foot[0] + (0.5 if foot[0] < 22 else -0.5), foot[1] - 1.6)
        if knee:
            _limb(c, hip, knee, 1.6, 'a', 'i', 'A')
            _limb(c, knee, end, 1.6, 'a', 'i', 'A')
        else:
            _limb(c, hip, end, 1.6, 'a', 'i', 'A')
    for foot in (p['lf'], p['rf']):
        _blob(c, foot[0], foot[1], 2.8, 1.7, 'P', 'p', 'h')
    _shirt(c, dy)
    _skirt(c, dy)
    for sh, hand in (((17.2, 41.0 + dy), p['lh']), ((26.8, 41.0 + dy), p['rh'])):
        _limb(c, sh, hand, 1.5, 'a', 'i', 'A')
        # short striped sleeve over the shoulder
        v = np.array(hand) - np.array(sh)
        v = v / (np.linalg.norm(v) or 1)
        _limb(c, sh, (sh[0] + v[0] * 2.0, sh[1] + v[1] * 2.0), 1.8, 'h', 'P', 'H')
        blit(c, ascii_sprite(HAND), int(round(hand[0] - 2)), int(round(hand[1] - 2)))
    return c


# ------------------------------------------------------------------ parts
def mouth_sprite(kind, lip=None):
    rows = {'smile': MOUTH_SMILE, 'open': MOUTH_OPEN, 'oh': MOUTH_OH, 'frown': MOUTH_FROWN,
            'grit': MOUTH_GRIT, 'wide': MOUTH_WIDE, 'wavy': MOUTH_WAVY}[kind]
    img = ascii_sprite(rows)
    if lip:
        img = recolor(img, {'k': LIPSTICK_COLORS[lip]})
    return img


def parts():
    """name -> (sprite, (ox, oy)) on the 44x68 canvas."""
    out = {}

    def crop_full(full):
        ys, xs = np.where(full[..., 3] > 0)
        return full[ys.min():ys.max() + 1, xs.min():xs.max() + 1], (int(xs.min()), int(ys.min()))

    for pose in POSES:
        out['hero_body_' + pose] = crop_full(body_sprite(pose))
    out['hero_hair_back'] = crop_full(hair_back_sprite())
    out['hero_head'] = crop_full(head_sprite())
    out['hero_hair_front'] = crop_full(hair_front_sprite())
    for n, rows in (('open', EYE_OPEN), ('closed', EYE_CLOSED), ('happy', EYE_HAPPY), ('sad', EYE_SAD),
                    ('angry', EYE_ANGRY), ('wide', EYE_WIDE)):
        out['hero_eye_' + n] = (ascii_sprite(rows), EYE_POS[0])
        if n in ('sad', 'angry'):
            out['hero_eye_%s_r' % n] = (flip_h(ascii_sprite(rows)), EYE_POS[1])
    # brows: an 18x4 patch that repaints the skin over the default brows
    for n, left in BROWS.items():
        rows = []
        for r in left:
            right = r[::-1]
            rows.append(r.replace('.', 'a') + '.' * 6 + right.replace('.', 'a'))
        out['hero_brow_' + n] = (ascii_sprite(rows), BROW_POS)
    out['hero_tear'] = (ascii_sprite(TEAR), TEAR_POS)
    out['hero_sweat'] = (ascii_sprite(SWEAT), SWEAT_POS)
    for kind in ('smile', 'open', 'oh', 'frown', 'grit', 'wide', 'wavy'):
        out['hero_mouth_' + kind] = (mouth_sprite(kind), MOUTH_POS)
        for lip in LIPSTICK_COLORS:
            out['hero_mouth_%s_%s' % (kind, lip)] = (mouth_sprite(kind, lip), MOUTH_POS)
    out['hero_blush'] = (ascii_sprite(BLUSH), BLUSH_POS[0])
    for col, c in EYESHADOW_COLORS.items():
        out['hero_shadow_' + col] = (ascii_sprite([r.replace('z', c) for r in EYESHADOW]), SHADOW_POS[0])
    out['hero_sticker_stella'] = (ascii_sprite(STICKER_STAR), STICKER_POS)
    out['hero_sticker_cuore'] = (ascii_sprite(STICKER_HEART), STICKER_POS)
    out['hero_tiara'] = (ascii_sprite(TIARA), TIARA_POS)
    out['hero_flower'] = (ascii_sprite(FLOWER), FLOWER_POS)
    out['sparkle'] = (ascii_sprite(SPARKLE), (0, 0))
    return out


def mirror_offsets():
    return {'eye_r': EYE_POS[1], 'blush_r': BLUSH_POS[1], 'shadow_r': SHADOW_POS[1]}


def compose(p, pose='idle', eyes='open', mouth='smile', lip=None, shadow=None, blush=False,
            sticker=None, tiara=False, flower=False, flip=False, brow=None, tear=0, sweat=False):
    """Reference composition, mirrors the C renderer."""
    c = blank(W, H)
    mo = mirror_offsets()
    hd = HEAD_DY[pose]

    def put(name, dy=0, at=None):
        img, (ox, oy) = p[name]
        if at:
            ox, oy = at
        blit(c, img, ox, oy + dy)

    put('hero_hair_back', hd)
    put('hero_body_' + pose)
    put('hero_head', hd)
    if brow:
        put('hero_brow_' + brow, hd)
    if shadow:
        put('hero_shadow_' + shadow, hd)
        put('hero_shadow_' + shadow, hd, mo['shadow_r'])
    put('hero_eye_' + eyes, hd)
    put('hero_eye_' + eyes + ('_r' if eyes in ('sad', 'angry') else ''), hd, mo['eye_r'])
    if tear:
        put('hero_tear', hd + tear - 1)
    if blush:
        put('hero_blush', hd)
        put('hero_blush', hd, mo['blush_r'])
    put('hero_mouth_' + mouth + ('_' + lip if lip else ''), hd)
    if sticker:
        put('hero_sticker_' + sticker, hd)
    put('hero_hair_front', hd)
    if sweat:
        put('hero_sweat', hd)
    if tiara:
        put('hero_tiara', hd)
    if flower:
        put('hero_flower', hd)
    return flip_h(c) if flip else c
