"""Backgrounds of the third tale, "la musica perduta" (0.10.0): the map of the
Valle della Musica (with its music, and grey-blue and quiet under the ogre's
spell) and the five places: the land of sweets, the mushroom forest, the lily
pond, the circus, and the ogre's bedroom (the last duel and the lullaby).

The map yields its anchors with the prefix "m3_": where Deva stands at each
place (m3_loc0..5) and the circle that gets its music back when the place is
freed (m3_reg1..5 centre, m3_rad1..5 radius).

Same layout as the other places: the sky and the far things above y ~150, the
ground from y ~196 (Deva's feet at x ~72, the monster's at x ~238, y ~214),
the middle of the screen calm.
"""
import numpy as np

from backgrounds import W, H, canvas, gradient, poly_mask, stars, hexrgb
from pixel import dilate4, rgb
from scenery import (YY, XX, LIGHT_UL, tone_fill, lobe, ellipse, rect, outline, paint, glow, vfade, speckle,
                     line, cloud, puff_row, round_tree, mushroom_glow, firefly, grass_tufts, moon, moon_face,
                     lily_pad, cattail)

INK = '#3b1f4a'


# ------------------------------------------------------------------ pieces
def lollipop(img, cx, base, h, r, cols, stick='#fff4fa', ink=INK):
    """A lollipop tree: a white stick and a round swirl of two colours."""
    st = rect(cx - 1, base - h, cx + 1, base)
    outline(img, st, ink)
    paint(img, st, stick)
    paint(img, st & (XX == cx + 1), '#e4d8ec')
    cy = base - h - r + 2
    disc = ellipse(cx, cy, r, r)
    outline(img, disc, ink)
    a = np.arctan2(YY + 0.5 - cy, XX + 0.5 - cx)
    d = np.sqrt((XX + 0.5 - cx) ** 2 + (YY + 0.5 - cy) ** 2)
    swirl = (np.floor((a / (2 * np.pi) + d / (r * 0.55)) * 2) % 2) == 0
    paint(img, disc & swirl, cols[0])
    paint(img, disc & ~swirl, cols[1])
    glow(img, cx - r * 0.35, cy - r * 0.4, r * 0.5, r * 0.4, '#ffffff', 0.5, mask=disc)
    return disc | st


def candy_cane(img, x, base, h, ink=INK):
    """A striped cane with its hook turned right."""
    pts = [(x, base), (x, base - h + 6), (x + 1.5, base - h + 2), (x + 4.5, base - h), (x + 7.5, base - h + 2),
           (x + 9, base - h + 6)]
    m = np.zeros((H, W), bool)
    for (x0, y0), (x1, y1) in zip(pts[:-1], pts[1:]):
        n = int(max(abs(x1 - x0), abs(y1 - y0)) * 2) + 1
        for t in np.linspace(0, 1, n):
            m |= ellipse(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, 2.1, 2.1)
    outline(img, m, ink)
    paint(img, m, '#fff4fa')
    paint(img, m & (((XX + YY) // 3) % 2 == 0), '#e5395a')
    return m


def gumdrop(img, cx, base, rx, ry, tones):
    m = ellipse(cx, base, rx, ry) & (YY <= base)
    outline(img, m, INK)
    tone_fill(img, m, 0.25 + 0.6 * (1 - (YY - (base - ry)) / max(1.0, ry)) - (XX - cx) / rx * 0.15, tones)
    speckle(img, m, '#ffffff', 0.08, int(cx * 7 + base))
    return m


def cupcake(img, cx, base, s, frost, cup='#ff93c6'):
    cupm = poly_mask([(cx - 9 * s, base - 11 * s), (cx + 9 * s, base - 11 * s), (cx + 6.5 * s, base), (cx - 6.5 * s, base)])
    outline(img, cupm, INK)
    paint(img, cupm, cup)
    for k in range(-3, 4):
        line(img, [(cx + k * 2.4 * s, base - 11 * s), (cx + k * 1.8 * s, base)], '#e0609e', 0.3, cupm)
    top = ellipse(cx, base - 13 * s, 10 * s, 6 * s) | ellipse(cx, base - 18 * s, 7 * s, 5 * s) | \
        ellipse(cx, base - 22 * s, 4 * s, 3.5 * s)
    outline(img, top, INK)
    tone_fill(img, top, 0.5 - (YY - (base - 24 * s)) / (14 * s) * 0.4 + 0.3, frost)
    cherry = ellipse(cx + 1 * s, base - 26 * s, 2.6 * s, 2.6 * s)
    outline(img, cherry, INK)
    paint(img, cherry, '#e5395a')
    paint(img, ellipse(cx, base - 27 * s, 0.8 * s, 0.8 * s), '#ffcbd6')
    rng = np.random.default_rng(int(cx))
    for _ in range(int(10 * s)):   # sprinkles
        x, y = cx + rng.uniform(-8, 8) * s, base - rng.uniform(12, 21) * s
        if top[int(y), int(x)]:
            paint(img, rect(int(x), int(y), int(x) + 1, int(y)), ['#74b8ff', '#ffd23f', '#4fd6c0', '#b376ec'][rng.integers(0, 4)])


def big_mushroom(img, cx, base, h, capr, caps, spots='#fff4fa', stem=('#d8cfc4', '#efe6da', '#fff8ee'), glow_c=None):
    """A giant mushroom: a stem with a skirt and a wide spotted cap, lit from the upper left."""
    stm = poly_mask([(cx - capr * 0.18, base), (cx - capr * 0.13, base - h), (cx + capr * 0.13, base - h),
                     (cx + capr * 0.2, base)])
    cap = ellipse(cx, base - h, capr, capr * 0.55) & (YY <= base - h + capr * 0.12)
    outline(img, stm | cap, INK)
    tone_fill(img, stm, 0.55 - (XX + 0.5 - cx) / max(1.0, capr * 0.2) * 0.35, list(stem))
    lobe(img, cx, base - h, capr, capr * 0.55, list(caps), L=LIGHT_UL, clip=cap, gamma=0.9)
    rng = np.random.default_rng(int(cx * 3 + base))
    for _ in range(int(capr / 2.5)):
        a = rng.uniform(np.pi * 1.05, np.pi * 1.95)
        d = rng.uniform(0.25, 0.8)
        x, y = cx + np.cos(a) * capr * d, base - h + np.sin(a) * capr * 0.5 * d
        rr = rng.uniform(1.6, 3.2) * capr / 20
        sp = ellipse(x, y, rr * 1.2, rr) & cap
        paint(img, sp, spots)
    # the gills under the cap
    gills = rect(cx - capr + 3, base - h + int(capr * 0.1) - 1, cx + capr - 3, base - h + int(capr * 0.12)) & \
        ellipse(cx, base - h, capr, capr * 0.6)
    paint(img, gills, stem[0])
    if glow_c:
        glow(img, cx, base - h, capr * 1.4, capr * 0.9, glow_c, 0.22)
    return stm | cap


def bunting(img, x0, y0, x1, y1, sag, cols, size=5):
    """A string of little flags between two points, sagging in the middle."""
    n = int(np.hypot(x1 - x0, y1 - y0) / (size * 2.2))
    pts = [(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t + sag * 4 * t * (1 - t)) for t in np.linspace(0, 1, n * 4 + 1)]
    line(img, pts, INK, 0.4)
    for k in range(n):
        t = (k + 0.5) / n
        x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t + sag * 4 * t * (1 - t)
        f = poly_mask([(x - size, y), (x + size, y), (x, y + size * 1.6)])
        outline(img, f, INK)
        paint(img, f, cols[k % len(cols)])


def bulbs(img, pts, on='#fff3a6', rim='#f0a030'):
    for (x, y) in pts:
        b = ellipse(x, y, 1.8, 1.8)
        paint(img, dilate4(b) & ~b, rim)
        paint(img, b, on)
        glow(img, x, y, 6, 6, '#fff3a6', 0.25)


def sun(img, cx, cy, r):
    """A kawaii sun: rays, a round face, pink cheeks, closed happy eyes."""
    glow(img, cx, cy, r * 3, r * 3, '#fff4c2', 0.3)
    for k in range(10):
        a = k * np.pi / 5
        ray = poly_mask([(cx + np.cos(a - 0.14) * r * 1.15, cy + np.sin(a - 0.14) * r * 1.15),
                         (cx + np.cos(a) * r * 1.6, cy + np.sin(a) * r * 1.6),
                         (cx + np.cos(a + 0.14) * r * 1.15, cy + np.sin(a + 0.14) * r * 1.15)])
        paint(img, ray, '#ffd23f')
    m = lobe(img, cx, cy, r, r, ['#f0a030', '#ffd23f', '#ffe066', '#fff3a6'], L=(-0.4, -0.5, 0.8), lo=0.2)
    outline(img, m, '#e08a20')
    moon_face(img, cx, cy, r, '#a0561a')
    return m


def note_deco(img, x, y, c, s=1.0):
    """A little musical note painted on the scenery (the valley sings)."""
    head = ellipse(x, y, 2.6 * s, 2.0 * s)
    stem = rect(int(x + 2 * s), int(y - 8 * s), int(x + 2 * s + 1), int(y))
    flag = poly_mask([(x + 2 * s, y - 8 * s), (x + 6 * s, y - 5 * s), (x + 3 * s, y - 5 * s)])
    m = head | stem | flag
    outline(img, m, INK)
    paint(img, m, c)
    return m


def striped_tent(img, cx, base, w, h, cols=('#e5395a', '#fff4fa'), top=None):
    """A circus tent: striped walls, a cone roof, a flag."""
    wall = rect(cx - w // 2, base - h // 2, cx + w // 2, base)
    roof = poly_mask([(cx - w / 2 - 4, base - h / 2 + 1), (cx + w / 2 + 4, base - h / 2 + 1), (cx, base - h)])
    outline(img, wall | roof, INK)
    for m, per in ((wall, 6), (roof, 6)):
        st = ((XX - cx + 1000) // per) % 2 == 0
        paint(img, m & st, cols[0])
        paint(img, m & ~st, cols[1])
    # the roof stripes fan out from the top
    ang = np.arctan2(XX + 0.5 - cx, YY + 0.5 - (base - h))
    fan = (np.floor(ang * 6) % 2) == 0
    paint(img, roof & fan, cols[0])
    paint(img, roof & ~fan, cols[1])
    door = poly_mask([(cx - w * 0.16, base), (cx, base - h * 0.34), (cx + w * 0.16, base)])
    paint(img, door, '#5c1733')
    line(img, [(cx, base - h), (cx, base - h - 8)], INK, 0.4)
    paint(img, poly_mask([(cx + 1, base - h - 8), (cx + 7, base - h - 6), (cx + 1, base - h - 4)]), top or '#ffd23f')
    return wall | roof


def crooked_house(img, cx, base, s=1.0, lit=True):
    """The ogre's house: crooked walls, a big round window, a leaning chimney."""
    walls = poly_mask([(cx - 18 * s, base), (cx - 16 * s, base - 26 * s), (cx + 17 * s, base - 28 * s), (cx + 19 * s, base)])
    roof = poly_mask([(cx - 23 * s, base - 24 * s), (cx + 22 * s, base - 27 * s), (cx + 3 * s, base - 48 * s)])
    chim = poly_mask([(cx + 9 * s, base - 38 * s), (cx + 15 * s, base - 39 * s), (cx + 16 * s, base - 50 * s),
                      (cx + 10 * s, base - 49 * s)])
    outline(img, walls | roof | chim, INK)
    tone_fill(img, walls, 0.5 - (XX - cx) / (20 * s) * 0.3, ['#8a6a4a', '#a8845e', '#c49e72'])
    tone_fill(img, roof, 0.6 - (YY - (base - 48 * s)) / (26 * s) * 0.4, ['#3f5a8a', '#5277b0', '#74a0d6'])
    tone_fill(img, chim, 0.5, ['#6e4e3a', '#8a6a4a'])
    win = ellipse(cx - 3 * s, base - 16 * s, 6 * s, 6 * s)
    outline(img, win, INK)
    paint(img, win, '#fff3a6' if lit else '#6e7fa8')
    paint(img, rect(int(cx - 3 * s), int(base - 22 * s), int(cx - 3 * s), int(base - 10 * s)) & win, '#8a6a4a')
    paint(img, rect(int(cx - 9 * s), int(base - 16 * s), int(cx + 3 * s), int(base - 16 * s)) & win, '#8a6a4a')
    door = rect(int(cx + 7 * s), int(base - 13 * s), int(cx + 13 * s), int(base))
    outline(img, door, INK)
    paint(img, door, '#5c3a22')
    return walls | roof | chim


# ------------------------------------------------------------------ the map of the Valle della Musica
# Deva's feet at each place, and the circle that gets its music back; the monster waiting at the next
# place stands at loc + 10 (64 px tall, the ogre 88), so every place has room above it
LOCS3 = [(28, 214), (150, 216), (58, 154), (176, 152), (96, 96), (262, 94)]
REGS3 = [None, (152, 198, 52), (62, 130, 50), (212, 140, 54), (98, 70, 52), (268, 62, 60)]


def stream(img, pts, w=3.0):
    """A little brook: dark banks, blue water, a few glints."""
    m = np.zeros((H, W), bool)
    for (x0, y0), (x1, y1) in zip(pts[:-1], pts[1:]):
        n = int(max(abs(x1 - x0), abs(y1 - y0)) * 2) + 1
        for t in np.linspace(0, 1, n):
            m |= ellipse(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, w, w * 0.8)
    outline(img, m, '#3f7a8a')
    paint(img, m, '#74b8ff')
    paint(img, m & (((XX * 3 + YY * 5) % 13) == 0), '#dff2ff')
    return m


def flowers(img, n, area, seed, cols=('#ffcbe3', '#fff3a6', '#ffffff', '#c4b0ff'), avoid=None):
    rng = np.random.default_rng(seed)
    x0, y0, x1, y1 = area
    for _ in range(n):
        x, y = int(rng.integers(x0, x1)), int(rng.integers(y0, y1))
        if avoid is not None and avoid[y, x]:
            continue
        c = cols[int(rng.integers(0, len(cols)))]
        paint(img, rect(x - 1, y, x + 1, y) | rect(x, y - 1, x, y + 1), c)
        img[y, x, :3] = rgb('#ffd23f')


def mappa3(with_music=True):
    img = canvas()
    gradient(img, 0, 64, ['#8fd0ff', '#afdeff', '#cfeeff', '#fff4e0'])
    sun(img, 206, 22, 9)
    cloud(img, puff_row(10, 96, 20, 7, 3), ['#d8e8ff', '#eef5ff', '#ffffff'])
    cloud(img, puff_row(236, 290, 14, 6, 4), ['#d8e8ff', '#eef5ff', '#ffffff'])
    # far hills, the meadow
    far = poly_mask([(0, 64)] + [(x, 50 + 6 * np.sin(x / 26.0) + 3 * np.sin(x / 9.0)) for x in range(0, W + 8, 8)]
                    + [(W, 64)])
    tone_fill(img, far, 0.6 + (YY - 50) / 20.0 * 0.3, ['#8cc8a0', '#a2d8b0', '#b8e8c0'])
    gradient(img, 60, H, ['#b2e69a', '#a2dc8c', '#92d07e', '#84c472', '#76b868'])
    speckle(img, YY >= 64, '#a8e094', 0.05, 31)
    speckle(img, YY >= 64, '#6fae62', 0.02, 32)
    # the ogre's hill, top right, and its path of stones
    hill5 = ellipse(274, 112, 60, 38) & (YY <= 118)
    outline(img, hill5, '#5f9150')
    tone_fill(img, hill5, 0.35 + (-(XX - 274) / 60 - (YY - 80) / 38) * 0.3, ['#6fa35e', '#80b46a', '#94c87a', '#a8d88c'])
    # the brook from the hill to the pond, and out to the right
    stream(img, [(318, 96), (300, 110), (290, 124), (268, 134), (258, 142)], 2.4)
    stream(img, [(252, 158), (270, 172), (300, 178), (322, 186)], 2.6)
    # the pond (3)
    pond = ellipse(222, 146, 36, 15)
    outline(img, pond, '#2b6a8a')
    tone_fill(img, pond, 0.35 + (YY - 146) / 15 * 0.35, ['#9fd8ff', '#74b8ff', '#5aa4ea', '#4f96e0'][::-1])
    paint(img, pond & (((XX * 5 + YY * 11) % 23) == 0), '#dff2ff')
    for (x, y, fl) in ((204, 142, True), (232, 150, False), (242, 140, True), (214, 154, False)):
        pad = ellipse(x, y, 5, 2.2)
        paint(img, pad, '#4cc25a')
        paint(img, pad & (YY < y), '#7cd86a')
        if fl:
            paint(img, rect(x - 1, y - 2, x + 1, y - 1), '#ff93c6')
    for (x, h, lean) in ((186, 18, -1), (192, 14, 1), (258, 16, 1)):
        cattail(img, x, 150, h, lean)
    # the path, dotted, from place to place
    for i in range(len(LOCS3) - 1):
        (x0, y0), (x1, y1) = LOCS3[i], LOCS3[i + 1]
        y0 -= 2
        y1 -= 2
        n = int(np.hypot(x1 - x0, y1 - y0) / 7)
        for k in range(1, n):
            t = k / n
            x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
            d = ellipse(x, y, 2.2, 1.6)
            paint(img, dilate4(d) & ~d, '#b08a50')
            paint(img, d, '#fff0c8')
    # trees and flowers here and there
    greens = ['#2d6440', '#46844f', '#6aa962', '#8cc876', '#b0e090']
    trunks = ['#3f2e3e', '#574353', '#6e5a64', '#8a7480']
    for (cx, base, r, sd) in ((150, 118, 11, 1), (170, 108, 8, 2), (304, 150, 12, 3), (8, 104, 12, 4),
                              (196, 76, 9, 5)):
        round_tree(img, cx, base, r, greens, trunks, L=LIGHT_UL, seed=sd, ink='#2f5a3a')
    flowers(img, 70, (0, 70, W, 236), 5, avoid=pond)
    # 0: the theatre tent (home)
    striped_tent(img, 28, 206, 30, 44, cols=('#ff93c6', '#fff4fa'))
    # 1: the land of sweets
    for (x, b, hh, r, c) in ((116, 204, 16, 7, ('#ff4d6d', '#fff4fa')), (186, 206, 20, 8, ('#b376ec', '#fff4fa')),
                             (206, 196, 12, 6, ('#4fd6c0', '#fff4fa')), (128, 180, 10, 5, ('#ffa860', '#fff4fa'))):
        lollipop(img, x, b, hh, r, c)
    gumdrop(img, 130, 226, 8, 7, ['#e0609e', '#ff93c6', '#ffcbe3'])
    gumdrop(img, 176, 228, 7, 6, ['#2a8a45', '#4cc25a', '#a8f08a'])
    gumdrop(img, 196, 222, 6, 5, ['#c07a20', '#ffb040', '#ffe08a'])
    candy_cane(img, 100, 228, 22)
    # 2: the mushroom forest
    for (x, b, hh, r, caps) in ((44, 128, 20, 12, ('#3a2a7a', '#5a3aa8', '#7d5fd0', '#a88af0')),
                                (88, 124, 18, 11, ('#8a1f33', '#c21f45', '#e53935', '#ff7a8a')),
                                (20, 152, 14, 10, ('#8a1f33', '#c21f45', '#e53935', '#ff7a8a')),
                                (104, 150, 12, 8, ('#1d6f73', '#2fb3a8', '#7ff0dc', '#d8fff4'))):
        big_mushroom(img, x, b, hh, r, caps)
    # 4: the circus
    striped_tent(img, 98, 78, 44, 52, cols=('#e5395a', '#fff4fa'))
    for x in (64, 132):
        line(img, [(x, 82), (x, 52)], INK, 0.4)
        paint(img, ellipse(x, 51, 1.5, 1.5), '#ffd23f')
    bunting(img, 64, 54, 78, 50, 3, ['#ffd23f', '#74b8ff', '#4cc25a'], 3)
    bunting(img, 118, 50, 132, 54, 3, ['#ff93c6', '#b376ec', '#ffd23f'], 3)
    # 5: the ogre's crooked house on the hill
    crooked_house(img, 272, 86, 1.0, lit=with_music)
    if with_music:   # the valley sings: notes in the air
        for (x, y, c) in ((160, 176, '#ff4d6d'), (32, 100, '#ff7b2e'), (248, 116, '#74b8ff'), (132, 40, '#4cc25a'),
                          (238, 40, '#ffd23f'), (306, 64, '#b376ec'), (58, 180, '#4fd6c0'), (180, 60, '#ff93c6')):
            note_deco(img, x, y, c)
    else:   # the ogre's chimney smokes a grey, sleepy smoke
        for (x, y, r) in ((290, 30, 5), (296, 22, 4), (300, 15, 3)):
            cloud(img, [(x, y, r, r * 0.8)], ['#8e97aa', '#a9b1c2', '#c2c9d6'])
    anchors = {}
    for i, (x, y) in enumerate(LOCS3):
        anchors['m3_loc%d' % i] = (x, y)
    for i in range(1, 6):
        cx, cy, r = REGS3[i]
        anchors['m3_reg%d' % i] = (cx, cy)
        anchors['m3_rad%d' % i] = (r, 0)
    return img, anchors


def zitta(img):
    """Under the ogre's spell: the same valley, quiet, pale and misty blue-grey."""
    f = img[..., :3].astype(np.float32)
    lum = f[..., 0] * 0.3 + f[..., 1] * 0.55 + f[..., 2] * 0.15
    g = np.stack([lum] * 3, axis=-1) * 0.6 + f * 0.15
    tint = np.array([0x8c, 0x98, 0xb8], np.float32)
    g = g * 0.72 + tint * 0.3
    out = img.copy()
    out[..., :3] = np.clip(g, 0, 255).astype(np.uint8)
    vfade(out, YY >= 0, '#c4cce0', 0.18, 0.05, 0, H)   # a sleepy mist
    return out


# ------------------------------------------------------------------ the land of sweets (Caramellone)
def dolci():
    img = canvas()
    gradient(img, 0, 196, ['#ffc4e4', '#ffd6ec', '#ffe6f2', '#fff0f4', '#ffe8d8'])
    cloud(img, puff_row(20, 120, 40, 9, 5), ['#f4b8dc', '#ffd6ec', '#fff4fa'])
    cloud(img, puff_row(190, 300, 30, 10, 6), ['#b8d8f4', '#d6ecff', '#f4faff'])
    cloud(img, puff_row(120, 170, 70, 6, 7), ['#f4b8dc', '#ffd6ec', '#fff4fa'])
    # far gumdrop hills with sugar
    for (cx, b, rx, ry, t) in ((30, 170, 50, 36, ['#9fd08a', '#b8e8a0', '#d8f8c4']),
                               (120, 176, 60, 30, ['#c9a0f0', '#e0c4ff', '#f0e0ff']),
                               (220, 172, 56, 38, ['#ffb070', '#ffd09a', '#ffe8c8']),
                               (300, 176, 44, 28, ['#9fd8ff', '#c4e6ff', '#e4f4ff'])):
        gumdrop(img, cx, b, rx, ry, t)
    # a gingerbread house far away
    gh = rect(236, 148, 268, 176)
    roof = poly_mask([(230, 150), (274, 150), (252, 128)])
    outline(img, gh | roof, INK)
    paint(img, gh, '#c98f58')
    tone_fill(img, roof, 0.5, ['#8a5a30', '#a8703e'])
    line(img, [(231, 150), (252, 129), (273, 150)], '#fff4fa', 0.7)
    for x in range(234, 272, 6):
        paint(img, ellipse(x, 151, 1.6, 1.6), ['#ff4d6d', '#4fd6c0', '#ffd23f'][(x // 6) % 3])
    paint(img, rect(248, 160, 256, 176), '#8a5a30')
    paint(img, ellipse(242, 158, 3, 3) | ellipse(262, 158, 3, 3), '#fff3a6')
    # the chocolate river, far
    river = (YY >= 180 + 2 * np.sin(XX / 14.0)) & (YY <= 191 + 2 * np.sin(XX / 17.0 + 1))
    paint(img, river, '#7a4a26')
    paint(img, river & (((XX * 3 + YY * 7) % 17) == 0), '#a8703e')
    outline(img, river, '#4a2a14')
    # lollipop trees and canes
    for (x, b, hh, r, c) in ((18, 196, 64, 16, ('#ff4d6d', '#fff4fa')), (108, 192, 40, 10, ('#b376ec', '#fff4fa')),
                             (206, 192, 48, 12, ('#4fd6c0', '#fff4fa')), (306, 196, 60, 15, ('#ffa860', '#fff4fa'))):
        lollipop(img, x, b, hh, r, c)
    candy_cane(img, 132, 198, 30)
    candy_cane(img, 180, 198, 26)
    # the ground: pink frosting with sprinkles, a biscuit path
    gradient(img, 194, H, ['#ffb8d8', '#ffc4e0', '#ffd0e6'])
    wave = YY < 196 + 2 * np.sin(XX / 9.0)
    paint(img, (YY >= 192) & wave & (YY < 199), '#ffc4e0')
    rng = np.random.default_rng(8)
    for _ in range(150):
        x, y = int(rng.integers(0, W - 2)), int(rng.integers(200, H - 1))
        c = ['#74b8ff', '#ffd23f', '#4fd6c0', '#b376ec', '#ff4d6d', '#fff4fa'][rng.integers(0, 6)]
        if rng.integers(0, 2):
            paint(img, rect(x, y, x + 1, y), c)
        else:
            paint(img, rect(x, y, x, y + 1), c)
    # cupcakes at the edges, close to us
    cupcake(img, 26, 238, 1.4, ['#ff93c6', '#ffcbe3', '#fff4fa'])
    cupcake(img, 296, 236, 1.3, ['#9fd8ff', '#c4e6ff', '#f4faff'], cup='#b376ec')
    return img


# ------------------------------------------------------------------ the mushroom forest (Fungone)
def funghi():
    img = canvas()
    gradient(img, 0, 196, ['#2a3a6a', '#3a4a86', '#5a5aa0', '#7a6ab0', '#9a86c0'])
    rng = np.random.default_rng(12)
    stars(img, rng, 50, (0, 0, W, 90))
    # far mushrooms in the mist
    for (x, b, hh, r) in ((20, 168, 60, 26), (96, 164, 74, 30), (170, 166, 56, 22), (250, 162, 80, 34),
                          (318, 170, 58, 24)):
        big_mushroom(img, x, b, hh, r, ('#3a3a7a', '#4a4a90', '#5a5aa6', '#6c6cba'), spots='#8a8ad0',
                     stem=('#5a5a8a', '#6a6a9a', '#7a7aaa'))
    vfade(img, (YY > 60) & (YY < 196), '#9a86c0', 0.05, 0.4, 60, 190)
    # the near ones: red with white spots, blue with glowing spots
    big_mushroom(img, 30, 200, 70, 30, ('#8a1f33', '#c21f45', '#e53935', '#ff7a8a'))
    big_mushroom(img, 292, 200, 84, 36, ('#1d6f73', '#2fb3a8', '#7ff0dc', '#d8fff4'), spots='#fff4fa',
                 glow_c='#7ff0dc')
    big_mushroom(img, 118, 192, 40, 16, ('#5a3aa8', '#7d5fd0', '#a88af0', '#d0c0ff'))
    big_mushroom(img, 214, 190, 34, 14, ('#8a1f33', '#c21f45', '#e53935', '#ff7a8a'))
    # ground: moss, a path, little glowing mushrooms
    gradient(img, 194, H, ['#2d5a3a', '#244a30', '#1c3c26'])
    path = poly_mask([(132, 240), (190, 240), (180, 214), (170, 200), (160, 196), (150, 200), (142, 214)])
    tone_fill(img, path, 0.3 + (YY - 196) / 60.0 * 0.5, ['#5a4a3a', '#6e5a44', '#826a50'])
    speckle(img, (YY >= 196) & ~path, '#3e7a4c', 0.08, 13)
    grass_tufts(img, 200, 238, ['#3e7a4c', '#4e9058', '#2e6a40'], 14, n=60, avoid=path)
    for (cx, by, s) in ((64, 222, 1.0), (96, 206, 0.8), (230, 218, 1.1), (262, 206, 0.8), (18, 230, 0.9)):
        mushroom_glow(img, cx, by, s, cap=('#6a2a8a', '#9a4ac0', '#d08af0', '#f4d8ff'), halo='#d08af0')
    fl = np.random.default_rng(15)
    for _ in range(12):   # floating spores
        firefly(img, int(fl.integers(10, W - 10)), int(fl.integers(70, 190)), '#c8f0ff')
    return img


# ------------------------------------------------------------------ the lily pond (Ranocchione)
def lago():
    img = canvas()
    gradient(img, 0, 150, ['#7ec8ff', '#9fd8ff', '#bfe6ff', '#dff2ff'])
    sun(img, 262, 34, 14)
    cloud(img, puff_row(20, 130, 34, 8, 2), ['#d8e8ff', '#eef5ff', '#ffffff'])
    # the far shore: hills, willows
    hills = poly_mask([(0, 150)] + [(x, 128 + 6 * np.sin(x / 30.0) + 4 * np.sin(x / 11.0)) for x in range(0, W + 8, 8)]
                      + [(W, 150)])
    tone_fill(img, hills, 0.5 + (YY - 128) / 30.0 * 0.3, ['#6fb462', '#80c46e', '#94d47c'])
    greens = ['#2d6440', '#46844f', '#6aa962', '#8cc876', '#b0e090']
    trunks = ['#3f2e3e', '#574353', '#6e5a64', '#8a7480']
    for (cx, base, r, s) in ((40, 150, 16, 1), (120, 146, 13, 2), (206, 148, 15, 3), (300, 150, 18, 4)):
        round_tree(img, cx, base, r, greens, trunks, L=LIGHT_UL, seed=s, ink='#1e3a2a')
    # the water
    water = (YY >= 150) & (YY < 200)
    gradient(img, 150, 200, ['#8fd0ff', '#6ab8f0', '#4f9ee0', '#3f86d0'])
    paint(img, water & (((XX * 5 + YY * 11) % 37) == 0), '#dff2ff')
    for y in range(154, 198, 6):   # ripples
        for x in range((y * 13) % 40, W, 40):
            line(img, [(x, y), (x + 8, y)], '#b0e0ff', 0.3)
    for (x, y, rx, ry, fl) in ((40, 170, 14, 5, ('#ffcbe3', '#ff93c6')), (96, 186, 12, 4, None),
                               (150, 166, 10, 4, ('#fff4fa', '#ffcbe3')), (208, 184, 14, 5, ('#ffcbe3', '#ff93c6')),
                               (268, 168, 12, 4, None), (300, 190, 10, 4, ('#fff4fa', '#ffcbe3'))):
        lily_pad(img, x, y, rx, ry, flower=fl)
    # a little wooden jetty on the right
    jet = rect(250, 186, 320, 192)
    outline(img, jet, INK)
    tone_fill(img, jet, 0.5, ['#8a6a4a', '#a8845e', '#c49e72'])
    for x in range(252, 320, 8):
        paint(img, rect(x, 186, x, 192), '#6e4e3a')
        paint(img, rect(x + 3, 193, x + 4, 200), '#6e4e3a')
    # the near shore: grass, reeds
    shore = YY >= 198 + 2 * np.sin(XX / 13.0)
    gradient(img, 196, H, ['#80c46e', '#6fb462', '#5fa456'])
    img[~shore & (YY >= 196) & (YY < 202), :3] = rgb('#4f9ee0')
    outline(img, shore & (YY >= 196), '#3f7a3a')
    speckle(img, shore, '#94d47c', 0.06, 21)
    grass_tufts(img, 202, 238, ['#4e9058', '#5fa456', '#3e7a4c'], 22, n=70)
    for (x, h, lean) in ((8, 40, -1), (16, 34, 1), (304, 38, 1), (312, 44, -2), (296, 30, 0)):
        cattail(img, x, 214, h, lean)
    fl = np.random.default_rng(23)
    for _ in range(18):   # meadow flowers
        x, y = int(fl.integers(4, W - 4)), int(fl.integers(206, 236))
        c = ['#ffcbe3', '#fff3a6', '#c4e6ff'][int(fl.integers(0, 3))]
        paint(img, rect(x - 1, y - 1, x + 1, y - 1) | rect(x, y - 2, x, y), c)
    return img


# ------------------------------------------------------------------ the circus (Trombone, and the party)
def circo(party=False):
    img = canvas()
    # the inside of the big top: soft stripes fanning from the top middle, lit from below
    ang = np.arctan2(XX + 0.5 - 160, YY + 30.0)
    fan = (np.floor(ang * 9) % 2) == 0
    tent = YY < 196
    lit = np.clip(1.0 - np.abs(XX - 160) / 260.0 - np.abs(YY - 130) / 260.0, 0, 1)
    tone_fill(img, tent & fan, 0.25 + lit * 0.7, ['#6a1830', '#8a2240', '#a82c4c', '#c23a58'])
    tone_fill(img, tent & ~fan, 0.25 + lit * 0.7, ['#a8808c', '#c8a4ac', '#e4ccc8', '#f6e6dc'])
    vfade(img, tent, '#2a0c24', 0.5, 0.0, 0, 70)      # the dark of the top
    # strings of bulbs hanging in arcs
    for (y0, sag) in ((34, 14), (58, 16)):
        pts = [(x, y0 + sag * 4 * (x / 320.0) * (1 - x / 320.0)) for x in range(0, 321, 4)]
        line(img, pts, INK, 0.3)
        bulbs(img, [(x, y0 + sag * 4 * (x / 320.0) * (1 - x / 320.0) + 2) for x in range(8, 320, 16)])
    bunting(img, 0, 80, 160, 70, 10, ['#ffd23f', '#74b8ff', '#4cc25a', '#ff93c6', '#b376ec'], 5)
    bunting(img, 160, 70, 320, 80, 10, ['#ff93c6', '#b376ec', '#ffd23f', '#74b8ff', '#4cc25a'], 5)
    # a big star sign at the back
    from objects import star_mask_px
    sm = np.zeros((H, W), bool)
    s = star_mask_px(33, 33, 16.5, 16.5, 16, 7)
    sm[92:125, 144:177] = s
    outline(img, sm, INK)
    tone_fill(img, sm, 0.3 + (-(XX - 160) - (YY - 108)) / 32.0 * 0.4 + 0.3, ['#f0a030', '#ffd23f', '#fff3a6'])
    bulbs(img, [(160 + 22 * np.cos(np.deg2rad(a)), 108 + 22 * np.sin(np.deg2rad(a))) for a in range(0, 360, 30)])
    # the stands in the half-light: two rows of round heads (the audience), a low wall
    rng = np.random.default_rng(7)
    for (y, off) in ((158, 0), (166, 6)):
        for x in range(off, W + 8, 12):
            c = ['#5a2e5e', '#6a3a70', '#4e2a56', '#74406e'][int(rng.integers(0, 4))]
            hx = x + int(rng.integers(-2, 3))
            head = ellipse(hx, y, 4.2, 4.0) | ellipse(hx, y + 7, 6.5, 4.5)
            paint(img, head & (YY < 174), c)
    paint(img, rect(0, 174, W - 1, 196), '#4a1a3a')
    for x in range(0, W, 20):
        paint(img, rect(x, 174, x + 1, 196), '#3a1030')
    paint(img, rect(0, 174, W - 1, 175), '#ffd23f')
    # the ring: sawdust floor, red and gold rim
    floor = YY >= 196
    gradient(img, 196, H, ['#e8c890', '#dcb878', '#d0aa68'])
    speckle(img, floor, '#f4dca8', 0.08, 31)
    speckle(img, floor, '#b89050', 0.05, 32)
    rim = floor & (YY <= 200)
    paint(img, rim, '#c21f45')
    paint(img, rim & (((XX // 8) % 2) == 0), '#ffd23f')
    outline(img, rim, INK)
    # two drum pedestals at the sides
    for (cx, top) in ((20, 176), (300, 176)):
        body = rect(cx - 16, top, cx + 16, 214)
        outline(img, body, INK)
        paint(img, body, '#74b8ff')
        for k in range(5):
            line(img, [(cx - 16 + k * 8, top + 2), (cx - 12 + k * 8, 212)], '#ffd23f', 0.5, body)
        lid = ellipse(cx, top, 16, 4)
        outline(img, lid, INK)
        paint(img, lid, '#fff4fa')
    if party:   # confetti in the air
        rng = np.random.default_rng(41)
        for _ in range(90):
            x, y = int(rng.integers(0, W - 2)), int(rng.integers(0, 190))
            paint(img, rect(x, y, x + 1, y + 1), ['#ffd23f', '#74b8ff', '#4cc25a', '#ff93c6', '#b376ec'][rng.integers(0, 5)])
    return img


# ------------------------------------------------------------------ the ogre's bedroom (the last duel, the lullaby)
def casa_orco():
    img = canvas()
    # wooden walls, big planks
    wall = YY < 196
    for x0 in range(0, W, 22):
        plank = wall & (XX >= x0) & (XX < x0 + 22)
        t = 0.45 + ((x0 // 22) % 3) * 0.12
        tone_fill(img, plank, t + (YY / 196.0) * 0.1, ['#6e4a30', '#845a3a', '#9a6c46', '#ae7e54'])
        paint(img, wall & (XX == x0), '#4a2f1b')
    for (x, y) in ((40, 30), (106, 90), (192, 40), (260, 120), (300, 60), (150, 150)):   # knots
        paint(img, ellipse(x, y, 2.4, 1.6), '#5c3a22')
    # the big round window with the moon and the hills of the valley
    win = ellipse(160, 62, 36, 36)
    glass = ellipse(160, 62, 32, 32)
    outline(img, win, INK)
    tone_fill(img, win & ~glass, 0.5 + (-(XX - 160) - (YY - 62)) / 72.0, ['#5c3a22', '#7a4a26', '#9a6c46', '#b8885c'])
    tmp = canvas()
    gradient(tmp, 26, 98, ['#141a44', '#1f2660', '#2b2f70', '#3a3a80'])
    img[glass, :3] = tmp[glass, :3]
    rng = np.random.default_rng(5)
    stars(img, rng, 16, (132, 32, 190, 80), avoid=~glass)
    moon(img, 172, 50, 9, halo=False, face=True)
    hills = glass & (YY > 78 + 4 * np.sin(XX / 9.0))
    paint(img, hills, '#2b5c47')
    paint(img, glass & (XX == 160), '#5c3a22')
    paint(img, glass & (YY == 62), '#5c3a22')
    # a cuckoo clock on the left and a shelf with a candle on the right
    clock = rect(52, 40, 80, 76)
    roofc = poly_mask([(48, 42), (84, 42), (66, 28)])
    outline(img, clock | roofc, INK)
    tone_fill(img, clock, 0.5, ['#8a5a30', '#a8703e', '#c98f58'])
    tone_fill(img, roofc, 0.5, ['#5c3a22', '#7a4a26'])
    face = ellipse(66, 56, 9, 9)
    outline(img, face, INK)
    paint(img, face, '#fff4e0')
    line(img, [(66, 56), (66, 50)], INK, 0.3)
    line(img, [(66, 56), (70, 58)], INK, 0.3)
    for (x, y) in ((58, 84), (74, 88)):   # the weights on chains
        line(img, [(x, 77), (x, y)], INK, 0.3)
        paint(img, rect(x - 2, y, x + 2, y + 6), '#ffd23f')
    shelf = rect(236, 70, 300, 74)
    outline(img, shelf, INK)
    paint(img, shelf, '#a8703e')
    candle = rect(250, 56, 256, 70)
    outline(img, candle, INK)
    paint(img, candle, '#fff4e0')
    paint(img, ellipse(253, 52, 2, 3.4), '#ffd23f')
    paint(img, ellipse(253, 53, 1, 1.6), '#fff3a6')
    glow(img, 253, 54, 26, 22, '#fff3a6', 0.3)
    for (x, c) in ((270, '#74b8ff'), (282, '#ff93c6'), (292, '#4cc25a')):   # books
        b = rect(x, 58, x + 7, 70)
        outline(img, b, INK)
        paint(img, b, c)
    # the giant bed at the back right, with a patchwork quilt and a huge pillow
    frame = rect(196, 132, 318, 196)
    headb = rect(292, 104, 318, 196)
    outline(img, frame | headb, INK)
    tone_fill(img, headb, 0.5, ['#5c3a22', '#7a4a26', '#9a6c46'])
    tone_fill(img, frame, 0.4, ['#5c3a22', '#7a4a26'])
    quilt = rect(198, 140, 290, 186)
    outline(img, quilt, INK)
    for yq in range(140, 187, 12):
        for xq in range(198, 291, 12):
            c = ['#ff93c6', '#74b8ff', '#ffd23f', '#4cc25a', '#b376ec', '#fff4fa'][((xq // 12) + (yq // 12) * 2) % 6]
            paint(img, rect(xq, yq, xq + 11, yq + 11) & quilt, c)
    line(img, [(198, 152), (290, 152)], '#fff4fa', 0.3)
    pillow = ellipse(276, 136, 18, 9)
    outline(img, pillow, INK)
    tone_fill(img, pillow, 0.5 + (-(XX - 276) - (YY - 136)) / 30.0, ['#d8d0e8', '#ece6f6', '#ffffff'])
    # the floor, a round rug
    floor = YY >= 196
    for y0 in range(196, H, 11):
        k = ((y0 - 196) // 11) % 2
        img[(YY >= y0) & (YY < y0 + 11), :3] = hexrgb('#5c3a22') if k else hexrgb('#4a2f1b')
    for x in range(0, W, 32):
        paint(img, floor & (XX == x + ((YY // 11) % 2) * 16), '#3a2414')
    paint(img, rect(0, 196, W - 1, 196), INK)
    rug = ellipse(150, 222, 110, 16)
    outline(img, rug, INK)
    paint(img, rug, '#b376ec')
    paint(img, ellipse(150, 222, 96, 12), '#d0a8f0')
    paint(img, ellipse(150, 222, 80, 9), '#b376ec')
    vfade(img, wall, '#1a0e24', 0.3, 0.0, 0, 120)   # it is night: dimmer up high
    return img


def all_backgrounds():
    lit, _ = mappa3(True)
    quiet, _ = mappa3(False)
    return {'bg_mappa3': lit, 'bg_mappa3_zitta': zitta(quiet), 'bg_dolci': dolci(), 'bg_funghi': funghi(),
            'bg_lago': lago(), 'bg_circo': circo(), 'bg_casa_orco': casa_orco()}


def anchors():
    return mappa3(True)[1]
