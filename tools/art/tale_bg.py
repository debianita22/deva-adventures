"""Backgrounds of the tale: the map of the Regno delle Stelle (in colour and
under the grey spell) and the five places of the duels.

The map also yields its anchors: where Deva stands at each place (loc0..5)
and the circle that gets its colours back when the place is freed
(reg1..5 centre, rad1..5 radius).
"""
import numpy as np

from backgrounds import (W, H, canvas, gradient, blend, poly_mask, stars, hexrgb)
from pixel import ellipse_mask, rect_mask, dilate4, rgb, shaded_part, blit, PALETTE
import tale as T

YY, XX = np.mgrid[0:H, 0:W]


def E(cx, cy, rx, ry):
    return ellipse_mask(W, H, cx, cy, rx, ry)


def Rm(x0, y0, x1, y1, rad=0):
    return rect_mask(W, H, x0, y0, x1, y1, rad)


def paint(img, m, c):
    img[m, :3] = rgb(c) if isinstance(c, str) and len(c) == 1 else (hexrgb(c) if isinstance(c, str) else c)


def part(img, m, base, light, shadow, outline='k'):
    """Shaded part straight onto a background (outline included)."""
    ring = dilate4(m) & ~m
    img[ring, :3] = rgb(outline)
    sp = shaded_part(m, base, light, shadow, outline)
    a = sp[..., 3] > 0
    img[a, :3] = sp[a, :3]


def dither(m, level):
    """Ordered-dither subset of a mask (level 0..1)."""
    B = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0
    return m & (B[YY % 4, XX % 4] < level)


def tree(img, cx, base_y, r, dark=False):
    trunk = Rm(cx - 2, base_y - r - 2, cx + 2, base_y)
    part(img, trunk, *(('1', '2', '0') if dark else ('J', 'j', 'q')))
    crown = E(cx, base_y - r - 6, r, r * 0.9) | E(cx - r * 0.5, base_y - r - 2, r * 0.6, r * 0.55) | \
        E(cx + r * 0.5, base_y - r - 2, r * 0.6, r * 0.55)
    part(img, crown, *(('4', '5', '0') if dark else ('G', 'g', 'd')))


def mushroom(img, cx, base_y, glow=False):
    stem = Rm(cx - 1, base_y - 4, cx + 1, base_y)
    part(img, stem, 'W', 'w', 's')
    cap = E(cx, base_y - 5, 4, 2.6) & Rm(0, 0, W - 1, base_y - 4)
    part(img, cap, *(('T', 't', 'e') if glow else ('r', 'P', 'R')))
    img[base_y - 6, cx - 1, :3] = rgb('w')
    img[base_y - 5, cx + 2, :3] = rgb('w')


def sparkle_dots(img, rng, n, area, colors=('7', 'y')):
    x0, y0, x1, y1 = area
    for _ in range(n):
        x, y = rng.integers(x0, x1), rng.integers(y0, y1)
        img[y, x, :3] = rgb(colors[rng.integers(0, len(colors))])


# ------------------------------------------------------------------ the map
LOCS = [(44, 214), (106, 176), (170, 214), (242, 176), (186, 118), (270, 90)]   # Deva's feet
REGS = [None, (100, 150, 52), (170, 204, 50), (244, 150, 54), (180, 76, 58), (272, 50, 60)]


def mappa():
    """0.12: the Regno delle Stelle painted again with light and volume, like the later maps: the
    same places in the same spots (the anchors do not move)."""
    import scenery as S
    import tale3_bg as T3
    INK = '#3b1f4a'
    img = canvas()
    gradient(img, 0, 66, ['#8fd0ff', '#a8dcff', '#c4e8ff', '#e2f4ff', '#fff4e0'])
    T3.sun(img, 30, 22, 9)
    S.cloud(img, S.puff_row(62, 128, 18, 7, 11), ['#d8e8ff', '#eef5ff', '#ffffff'])
    S.cloud(img, S.puff_row(4, 40, 52, 5, 12), ['#d8e8ff', '#eef5ff', '#ffffff'])
    for (x, y) in ((112, 40), (226, 10), (134, 8), (58, 6)):   # the kingdom of the stars: little stars by day
        S.paint(img, S.rect(x, y - 1, x, y + 1) | S.rect(x - 1, y, x + 1, y), '#fff3a6')
    # far hills, the meadow
    far = poly_mask([(0, 72)] + [(x, 58 + 6 * np.sin(x / 30.0) + 3 * np.sin(x / 11.0)) for x in range(0, W + 8, 8)]
                    + [(W, 72)])
    S.tone_fill(img, far, 0.55 + (S.YY - 54) / 20.0 * 0.3, ['#8cc8a0', '#a2d8b0', '#b8e8c0'])
    meadow = S.YY >= 66
    gradient(img, 66, H, ['#b2e69a', '#a6de8e', '#98d482', '#8aca76', '#7cbe6a'])
    S.speckle(img, meadow, '#c0ecaa', 0.022, 41)
    S.speckle(img, meadow, '#6fae62', 0.025, 42)
    # the river, from the mountain down to the bottom of the map, wider as it goes
    pts = [(150, 64), (140, 90), (150, 120), (138, 150), (120, 188), (126, 240)]
    river = np.zeros((H, W), bool)
    for (x0, y0), (x1, y1) in zip(pts[:-1], pts[1:]):
        for t in np.linspace(0, 1, 60):
            x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
            river |= S.ellipse(x, y, 3.2 + y / 55.0, 3.4)
    S.outline(img, river, '#3f7a8a')
    S.tone_fill(img, river, 0.45 + np.sin(S.XX / 3.0 + S.YY / 5.0) * 0.25, ['#5aa4ea', '#74b8ff', '#9fd0ff'])
    S.paint(img, river & (((S.XX * 3 + S.YY * 5) % 17) == 0), '#e8f6ff')
    # the path, dotted, from place to place, and a little wooden bridge where it crosses the river
    bridge = poly_mask([(113, 180), (131, 189), (128, 196), (110, 187)])
    S.outline(img, bridge, '#5c3a22')
    S.tone_fill(img, bridge, 0.5, ['#a8703e', '#c98f58', '#e0ad76'])
    S.paint(img, bridge & ((((S.XX * 2 - S.YY) % 5) + 5) % 5 == 0), '#8a5a30')
    for i in range(len(LOCS) - 1):
        (x0, y0), (x1, y1) = LOCS[i], LOCS[i + 1]
        y0 -= 2
        y1 -= 2
        n = int(np.hypot(x1 - x0, y1 - y0) / 7)
        for k in range(1, n):
            t = k / n
            x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
            d = S.ellipse(x, y, 2.2, 1.6)
            S.paint(img, dilate4(d) & ~d, '#b08a50')
            S.paint(img, d, '#fff0c8')
    greens = ['#2d6440', '#46844f', '#6aa962', '#8cc876', '#b0e090']
    trunks = ['#3f2e3e', '#574353', '#6e5a64', '#8a7480']
    # 4: the mountain of clouds, lilac, lit from the left, its snow, the clouds round its waist
    mount = poly_mask([(140, 124), (174, 52), (186, 40), (200, 56), (232, 124)])
    S.outline(img, mount, '#4a3a70')
    side = (S.XX - 186) / 46.0
    S.tone_fill(img, mount, 0.62 - side * 0.38 - (S.YY - 40) / 84.0 * 0.15, ['#7a6aa8', '#9888c8', '#b4a6e0', '#d2c8f4'])
    snow = poly_mask([(166, 68), (174, 52), (186, 40), (200, 56), (206, 70), (196, 64), (188, 72), (180, 62), (172, 70)])
    S.tone_fill(img, snow & mount, 0.7 - side * 0.3, ['#c8d4f0', '#e8eeff', '#ffffff'])
    for (cx, cy, r) in ((150, 76, 10), (164, 70, 12), (214, 80, 11), (226, 86, 9), (194, 98, 10), (206, 94, 8),
                        (178, 102, 8)):
        S.cloud(img, [(cx, cy, r, r * 0.7)], ['#d8e2ff', '#eef3ff', '#ffffff'], ink='#9aa6d0')
    # 5: the witch's castle on its hill: stones lit from the left, violet roofs, lit windows, flags
    hill5 = S.ellipse(272, 100, 48, 16)
    S.outline(img, hill5, '#5f9150')
    S.tone_fill(img, hill5, 0.4 + (-(S.XX - 272) / 48.0 - (S.YY - 88) / 16.0) * 0.3,
                ['#6fa35e', '#80b46a', '#94c87a', '#a8d88c'])
    wall = S.rect(242, 52, 302, 92)
    towers = [(244, 26, 9), (272, 12, 11), (300, 26, 9)]
    body = wall.copy()
    for x in range(242, 302, 8):
        body |= S.rect(x, 48, x + 3, 51)
    for (cx, top, w) in towers:
        body |= S.rect(cx - w // 2, top + 12, cx + w // 2, 92)
    S.outline(img, body, INK)
    S.tone_fill(img, body, 0.55 - (S.XX - 272) / 40.0 * 0.25, ['#5e5280', '#776a9c', '#9286b8', '#ada2d0'])
    S.paint(img, body & ((S.YY % 7) == 0) & (S.XX > 240), '#5e5280')   # rows of stones
    S.paint(img, body & ((S.YY % 7) != 0) & (((S.XX + (S.YY // 7) * 4) % 9) == 0), '#5e5280')
    for (cx, top, w) in towers:
        roof = poly_mask([(cx - w // 2 - 3, top + 13), (cx + w // 2 + 3, top + 13), (cx, top - 5)])
        S.outline(img, roof, INK)
        S.tone_fill(img, roof, 0.6 - (S.XX - cx) / (w + 3.0) * 0.4, ['#6a3aa8', '#8a5ad0', '#b07af0', '#d0a8ff'])
        win = S.rect(cx - 1, top + 18, cx + 1, top + 22)
        S.paint(img, win, '#fff3a6')
        S.glow(img, cx, top + 20, 6, 6, '#fff3a6', 0.3)
        S.line(img, [(cx, top - 5), (cx, top - 12)], INK, 0.4)
        S.paint(img, poly_mask([(cx + 1, top - 12), (cx + 7, top - 10), (cx + 1, top - 8)]),
                '#ff7ab8' if cx == 272 else '#ffd23f')
    gate = S.ellipse(272, 90, 8, 12) & S.rect(0, 0, W - 1, 92)
    S.outline(img, gate, INK)
    S.paint(img, gate, '#3a2a5a')
    S.paint(img, gate & (((S.XX - 264) % 4) == 0), '#2a1e44')
    # 1: the enchanted forest (round trees lit from the left), red mushrooms
    for (cx, by, r, sd) in ((100, 146, 14, 1), (78, 160, 11, 2), (122, 156, 12, 3), (86, 176, 9, 4), (124, 178, 9, 5)):
        S.round_tree(img, cx, by, r, greens, trunks, L=S.LIGHT_UL, ink='#2f5a3a', seed=sd)
    for (cx, by) in ((70, 186), (132, 190), (94, 188)):
        mushroom(img, cx, by)
    # 2: the frog swamp: a pond with lily pads and flowers, bulrushes
    pond = S.ellipse(172, 206, 34, 12)
    S.outline(img, pond, '#2b6a8a')
    S.tone_fill(img, pond, 0.3 + (S.YY - 194) / 24.0 * 0.4, ['#4f96e0', '#5aa4ea', '#74b8ff', '#9fd8ff'][::-1])
    S.paint(img, pond & (((S.XX * 5 + S.YY * 11) % 23) == 0), '#dff2ff')
    for (cx, cy, fl) in ((158, 204, ('#ff93c6', '#ffcbe3')), (184, 210, None), (176, 200, ('#fff3a6', '#ffffff'))):
        S.lily_pad(img, cx, cy, 5.5, 2.6, fl)
    for (x, h, lean) in ((140, 18, -1), (145, 14, 0), (202, 16, 1), (207, 20, 1)):
        S.cattail(img, x, 202, h, lean)
    # 3: the crystal cave: a rocky hill, a dark mouth, pink crystals glowing at the door
    hill = poly_mask([(206, 176), (216, 138), (236, 122), (262, 128), (282, 154), (286, 176)])
    S.outline(img, hill, '#4a4260')
    S.tone_fill(img, hill, 0.6 - (S.XX - 246) / 40.0 * 0.3 - (S.YY - 122) / 54.0 * 0.15,
                ['#7a7090', '#958aac', '#b0a6c6', '#cac2dc'])
    for (x0, y0, x1, y1) in ((220, 150, 230, 148), (258, 138, 270, 142), (236, 132, 246, 131), (272, 160, 280, 158)):
        S.line(img, [(x0, y0), (x1, y1)], '#7a7090', 0.4, hill)   # cracks in the rock
    mouth = S.ellipse(246, 170, 14, 16) & S.rect(0, 0, W - 1, 176)
    S.outline(img, mouth, '#2a2238')
    S.tone_fill(img, mouth, (S.YY - 154) / 22.0, ['#1a1428', '#2a2040', '#3a2a58'])
    for (cx, s_) in ((226, 0.42), (268, 0.38)):
        S.crystal_cluster(img, cx, 176, s_, ('#c0306a', '#ff5a9a', '#ff9ac6', '#ffffff'), '#ff9ac6')
    # 0: the theatre tent (home)
    T3.striped_tent(img, 32, 206, 34, 46, cols=('#ff93c6', '#fff4fa'))
    # flowers in the grass, away from the pond and the river
    T3.flowers(img, 80, (0, 110, W, 236), 7, avoid=pond | river | dilate4(dilate4(bridge)))
    anchors = {}
    for i, (x, y) in enumerate(LOCS):
        anchors['loc%d' % i] = (x, y)
    for i in range(1, 6):
        cx, cy, r = REGS[i]
        anchors['reg%d' % i] = (cx, cy)
        anchors['rad%d' % i] = (r, 0)
    return img, anchors


def grigio(img):
    """The grey spell: luminance, tinted towards a cold lilac grey, a bit darker."""
    f = img[..., :3].astype(np.float32)
    lum = f[..., 0] * 0.3 + f[..., 1] * 0.55 + f[..., 2] * 0.15
    g = np.stack([lum] * 3, axis=-1)
    tint = np.array([0x9d, 0x94, 0xb8], np.float32)
    g = (g * 0.72 + tint * 0.28) * 0.86
    out = img.copy()
    out[..., :3] = np.clip(g, 0, 255).astype(np.uint8)
    return out


# ------------------------------------------------------------------ duel places
def ground(img, y0, colors, speckle=None):
    gradient(img, y0, H, colors)
    if speckle:
        paint(img, (YY >= y0) & (((XX * 5 + YY * 11) % 23) == 0), speckle)


def fog(img, y0, y1, color='%', level=0.35):
    """A soft band of mist: denser in the middle, fading at both edges."""
    mid = (y0 + y1) / 2.0
    for y in range(y0, y1):
        t = 1.0 - abs(y - mid) / ((y1 - y0) / 2.0)
        row = (YY == y) & dither(np.ones((H, W), bool), 0.25 + 0.5 * t)
        blend(img, row, rgb(color), level * t)


def bosco():
    img = canvas()
    gradient(img, 0, 196, ['#0f1030', '#1b1d4a', '#2a2c63', '#3b3a78'])
    rng = np.random.default_rng(3)
    stars(img, rng, 90, (0, 0, W, 120))
    moon = E(250, 44, 20, 20)
    paint(img, moon, 'y')
    paint(img, dilate4(moon) & ~moon, 'o')
    paint(img, E(244, 38, 4, 3) & moon, '#f5e27a')
    paint(img, E(256, 52, 3, 2) & moon, '#f5e27a')
    for (cx, by, r) in ((50, 176, 16), (150, 172, 18), (206, 176, 15), (260, 172, 17)):   # far row
        tree(img, cx, by, r, dark=True)
    blend(img, (YY < 196) & (YY > 100), hexrgb('#1b1d4a'), 0.35)
    for (cx, by, r) in ((20, 190, 26), (70, 186, 20), (120, 192, 24), (176, 188, 22), (230, 190, 26),
                        (290, 186, 24)):
        tree(img, cx, by, r, dark=True)
    for (cx, by, r) in ((-6, 206, 34), (326, 206, 34)):                                   # frame
        tree(img, cx, by, r, dark=True)
    ground(img, 196, ['#1f3b2f', '#183126', '#12261d'], '#2d5a40')
    for (cx, by) in ((30, 214), (58, 208), (292, 212), (268, 206), (150, 216)):
        mushroom(img, cx, by, glow=True)
    sparkle_dots(img, rng, 26, (0, 100, W, 196))
    fog(img, 176, 200)
    return img


def palude():
    img = canvas()
    gradient(img, 0, 190, ['#1c2a26', '#27392f', '#35503d', '#48664b'])
    rng = np.random.default_rng(4)
    moon = E(64, 40, 14, 14)
    paint(img, moon, '#e8f2c0')
    paint(img, dilate4(moon) & ~moon, '#9fb87a')
    # dead trees
    for (x, top) in ((24, 70), (286, 60), (230, 96)):
        trunk = Rm(x - 3, top, x + 3, 196)
        part(img, trunk, '1', '2', '0')
        for (dx, dy, l) in ((-1, 20, -14), (1, 34, 16), (-1, 50, -10)):
            for k in range(abs(l)):
                xx = x + dx * 3 + (k if l > 0 else -k)
                yy = top + dy - k // 2
                paint(img, Rm(xx, yy, xx + 1, yy + 1), '1')
    gradient(img, 190, H, ['#23382c', '#1b2d23'])
    pond = E(170, 214, 120, 20)
    paint(img, pond, '#2c5f5a')
    paint(img, dilate4(pond) & ~pond, '#1a3f3b')
    paint(img, pond & (((XX + 2 * YY) % 17) == 0), '#4f8f86')
    for (cx, cy) in ((112, 212), (150, 222), (206, 210), (240, 224), (186, 226)):
        part(img, E(cx, cy, 8, 3.2), '5', '6', '4')
    for x in range(8, W, 23):
        h = 14 + (x * 7) % 10
        paint(img, Rm(x, 196 - h, x, 196), '4')
        paint(img, Rm(x - 1, 196 - h - 4, x + 1, 196 - h + 1, 1), 'q')
    sparkle_dots(img, rng, 20, (0, 90, W, 190), ('7', '6'))
    fog(img, 160, 200, '%', 0.4)
    return img


def grotta():
    img = canvas()
    gradient(img, 0, H, ['#140e24', '#1f1636', '#2a1d45', '#1d1433'])
    paint(img, dither(np.ones((H, W), bool), 0.12) & (((XX // 3 + YY // 5) % 7) == 0), '#35285a')
    # stalactites
    for x in range(6, W, 26):
        h = 16 + (x * 13) % 22
        m = poly_mask([(x - 7, 0), (x + 7, 0), (x, h)])
        part(img, m, '1', '2', '0')
    # glowing crystal clusters
    for (cx, by, s, c) in ((22, 200, 1.2, ('P', 'p', 'h')), (52, 204, 0.8, ('v', 'L', 'V')),
                           (292, 198, 1.3, ('T', 't', 'e')), (262, 206, 0.9, ('P', 'p', 'h')),
                           (160, 208, 0.7, ('v', 'L', 'V'))):
        for (dx, hh, w) in ((-6, 22, 5), (0, 30, 6), (7, 18, 5)):
            m = poly_mask([(cx + dx - w * s, by), (cx + dx, by - hh * s), (cx + dx + w * s, by)])
            part(img, m, *c)
        blend(img, E(cx, by - 10, 26 * s, 16 * s) & ~dilate4(E(cx, by - 10, 1, 1)), rgb(c[0]), 0.10)
    ground(img, 206, ['#2a2140', '#211a34', '#18122a'], '#3b2f5c')
    return img


def nuvole():
    img = canvas()
    gradient(img, 0, H, ['#262a40', '#343a55', '#474f6d', '#5a6386'])
    rng = np.random.default_rng(6)
    for (cx, cy, r) in ((30, 30, 26), (80, 20, 30), (150, 26, 28), (220, 18, 32), (290, 30, 26),
                        (60, 70, 18), (270, 76, 20)):
        cl = E(cx, cy, r * 1.3, r * 0.8)
        part(img, cl, '2', '3', '1')
    # rain
    for _ in range(140):
        x, y = rng.integers(0, W), rng.integers(40, 200)
        for k in range(4):
            if 0 <= y + k < H and 0 <= x - k // 2 < W:
                img[y + k, x - k // 2, :3] = hexrgb('#8e9ac4')
    # the rocky summit
    peak = poly_mask([(0, 200), (40, 186), (110, 192), (170, 184), (240, 190), (320, 182), (320, 240), (0, 240)])
    part(img, peak, '2', '3', '1')
    paint(img, peak & (YY < 196) & (((XX * 3 + YY) % 5) != 0), 's')
    ground(img, 206, ['#5c5870', '#4a475c', '#3b384a'], '#6e6a84')
    return img


def castello():
    img = canvas()
    # stone wall
    gradient(img, 0, 196, ['#2a2140', '#342a4f', '#3e3360'])
    for y in range(0, 196, 10):
        off = 0 if (y // 10) % 2 == 0 else 12
        img[y, :, :3] = hexrgb('#221a36')
        for x in range(off, W, 24):
            img[y:y + 10, x, :3] = hexrgb('#221a36')
    # the big window with the moon and a storm outside
    win = Rm(128, 18, 192, 120, 0) | E(160, 18, 32, 32) & Rm(0, 0, W - 1, 30)
    paint(img, win, '#141033')
    moon = E(172, 40, 10, 10) & ~E(178, 36, 9, 9)
    paint(img, moon & win, 'y')
    rng = np.random.default_rng(8)
    for _ in range(18):
        x, y = rng.integers(130, 190), rng.integers(10, 118)
        if win[y, x]:
            img[y, x, :3] = rgb('w')
    frame = dilate4(dilate4(win)) & ~win
    paint(img, frame, '1')
    paint(img, dilate4(frame) & ~frame & ~win, 'k')
    paint(img, Rm(159, 0, 160, 120) & win, '1')
    paint(img, Rm(128, 64, 192, 65) & win, '1')
    # banners with a moon
    for x0 in (40, 250):
        ban = poly_mask([(x0, 20), (x0 + 30, 20), (x0 + 30, 100), (x0 + 15, 90), (x0, 100)])
        part(img, ban, 'V', 'v', '$')
        m = E(x0 + 15, 50, 7, 7) & ~E(x0 + 18, 47, 6, 6)
        paint(img, m, 'Y')
    # torches with green flames
    for x in (100, 220):
        paint(img, Rm(x - 2, 90, x + 2, 110), '1')
        flame = E(x, 84, 4, 7)
        part(img, flame, '7', 'y', '5')
        blend(img, E(x, 84, 24, 22) & ~flame, rgb('7'), 0.08)
    # cobwebs in the corners
    for (cx, cy, sx, sy) in ((0, 0, 1, 1), (W - 1, 0, -1, 1)):
        for r in (10, 18, 26):
            for a in np.linspace(0, np.pi / 2, 20):
                x = int(cx + sx * r * np.cos(a))
                y = int(cy + sy * r * np.sin(a))
                if 0 <= x < W and 0 <= y < H:
                    img[y, x, :3] = hexrgb('#8a80a8')
        for a in np.linspace(0, np.pi / 2, 4):
            for r in range(0, 28):
                x = int(cx + sx * r * np.cos(a))
                y = int(cy + sy * r * np.sin(a))
                if 0 <= x < W and 0 <= y < H:
                    img[y, x, :3] = hexrgb('#8a80a8')
    # chequered floor
    for y in range(196, H):
        for x in range(0, W):
            k = ((x + (y - 196) * 2) // 16 + (y - 196) // 8) % 2
            img[y, x, :3] = hexrgb('#4a3b6e') if k else hexrgb('#2e2447')
    img[196, :, :3] = rgb('k')
    # a bubbling cauldron in the corner
    cau = E(292, 210, 18, 12)
    part(img, cau, '1', '2', '0')
    brew = E(292, 200, 15, 4)
    paint(img, brew, '7')
    for (x, y) in ((286, 194), (296, 190), (290, 186)):
        b = E(x, y, 2, 2)
        paint(img, dilate4(b) & ~b, '6')
    return img


def all_backgrounds():
    import scenery as S   # the places painted with volume (0.8.0); the old flat ones stay above for reference
    import places13 as P13   # 0.13.0: the castle painted again
    m, _ = mappa()
    return {'bg_mappa': m, 'bg_mappa_grigia': grigio(m), 'bg_bosco': S.bosco(), 'bg_palude': S.palude(),
            'bg_grotta': S.grotta(), 'bg_nuvole': S.nuvole(), 'bg_castello': P13.castello()}


def anchors():
    return mappa()[1]
