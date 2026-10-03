"""Backgrounds of the second tale, "la notte senza stelle": the night map of
the Regno delle Stelle (with its stars, and dark under the wizard's spell)
and the five places of the duels, at night and without stars (he stole them).

The map yields its anchors with the prefix "m2_": where Deva stands at each
place (m2_loc0..5) and the circle that lights up when the place is freed
(m2_reg1..5 centre, m2_rad1..5 radius).
"""
import numpy as np

from backgrounds import (W, H, canvas, gradient, blend, poly_mask, stars, hexrgb)
from pixel import dilate4, rgb
from tale_bg import E, Rm, paint, part, dither, mushroom, sparkle_dots, ground, fog, YY, XX

SKY = ['#0b0e2a', '#141a44', '#1f2660', '#2b2f70']


def masked_gradient(img, mask, y0, y1, stops):
    tmp = canvas()
    gradient(tmp, y0, y1, stops)
    img[mask, :3] = tmp[mask, :3]


def night_sky(img, y1, rng=None, with_stars=False, n=110):
    gradient(img, 0, y1, SKY)
    if with_stars:
        stars(img, rng, n, (0, 0, W, y1 - 4))


def crescent(img, cx, cy, r, c='y', rim='o'):
    m = E(cx, cy, r, r) & ~E(cx + r * 0.45, cy - r * 0.3, r * 0.85, r * 0.85)
    paint(img, dilate4(m) & ~m & ~E(cx + r * 0.45, cy - r * 0.3, r * 0.85, r * 0.85), rim)
    paint(img, m, c)
    if r >= 11:   # big enough for a sleepy face (0.9.0)
        from scenery import crescent_face
        crescent_face(img, cx, cy, r, 'O')
    return m


def big_flower(img, cx, top, h, petal, center='Y', r=10):
    stem = Rm(cx - 2, top, cx + 1, top + h)
    part(img, stem, 'G', 'g', 'd')
    leaf = poly_mask([(cx, top + h * 0.6), (cx - 14, top + h * 0.45), (cx - 4, top + h * 0.7)])
    part(img, leaf, 'G', 'g', 'd')
    for i in range(6):
        a = np.deg2rad(i * 60 + 15)
        pm = E(cx + r * 0.8 * np.cos(a), top + r * 0.8 * np.sin(a), r * 0.62, r * 0.62)
        part(img, pm, *petal)
    part(img, E(cx, top, r * 0.45, r * 0.45), center, 'y', 'o')


def ice_peak(img, x0, x1, top, base, dark=False):
    peak = poly_mask([(x0, base), ((x0 + x1) / 2, top), (x1, base)])
    part(img, peak, *(('3', 's', '2') if dark else ('b', 'w', 'n')))
    snow = poly_mask([((x0 + x1) / 2 - (x1 - x0) * 0.18, top + (base - top) * 0.35), ((x0 + x1) / 2, top),
                      ((x0 + x1) / 2 + (x1 - x0) * 0.18, top + (base - top) * 0.35),
                      ((x0 + x1) / 2 + (x1 - x0) * 0.06, top + (base - top) * 0.28),
                      ((x0 + x1) / 2 - (x1 - x0) * 0.05, top + (base - top) * 0.33)])
    paint(img, snow & peak, 'W')


def shell(img, cx, cy, c=('p', 'W', 'c')):
    m = E(cx, cy, 4, 3) & Rm(0, 0, W - 1, cy + 1)
    m |= poly_mask([(cx - 2, cy + 1), (cx + 2, cy + 1), (cx, cy + 3)])
    part(img, m, *c)
    for dx in (-2, 0, 2):
        paint(img, Rm(cx + dx, cy - 2, cx + dx, cy), c[2])


def starfish(img, cx, cy):
    from objects import star_mask_px
    m = np.zeros((H, W), bool)
    s = star_mask_px(13, 13, 6.5, 6.5, 6, 2.6)
    x0, y0 = int(cx) - 6, int(cy) - 6
    m[y0:y0 + 13, x0:x0 + 13] = s
    part(img, m, 'o', 'Y', 'O')


# ------------------------------------------------------------------ the map
LOCS2 = [(30, 214), (152, 214), (98, 170), (42, 120), (160, 110), (270, 88)]
REGS2 = [None, (164, 206, 54), (98, 150, 48), (44, 84, 54), (162, 76, 50), (272, 56, 60)]


def mappa2(with_stars=True):
    """0.12: the night map painted again with light and volume: a bright, starry, moonlit night when
    the stars are back (with_stars), the same land without them for the dark spell (buio). The
    places stay in the same spots (the anchors do not move)."""
    import scenery as S
    import tale3_bg as T3
    INK = '#1e1838'
    img = canvas()
    rng = np.random.default_rng(21)
    gradient(img, 0, 76, ['#1a1850', '#232766', '#2e347c', '#3c4290', '#4c50a0'])
    if with_stars:
        stars(img, rng, 130, (0, 0, W, 66))
        for (x, y) in ((24, 14), (88, 30), (130, 10), (214, 22), (236, 50), (62, 54), (196, 8)):   # bigger ones
            S.paint(img, S.rect(x, y - 2, x, y + 2) | S.rect(x - 2, y, x + 2, y), '#fff3a6')
            S.paint(img, S.rect(x, y, x, y), '#ffffff')
        S.glow(img, 300, 22, 34, 30, '#fff4c2', 0.22)
    crescent(img, 300, 22, 11)
    # far hills in the moonlight, the meadow
    far = poly_mask([(0, 84)] + [(x, 66 + 7 * np.sin(x / 34.0 + 1) + 3 * np.sin(x / 12.0)) for x in range(0, W + 8, 8)]
                    + [(W, 84)])
    S.tone_fill(img, far, 0.55 + (S.YY - 60) / 24.0 * 0.3, ['#2a5e66', '#336e72', '#3e807e', '#4a928a'])
    meadow = S.YY >= 76
    gradient(img, 76, H, ['#43926c', '#3b8462', '#337658', '#2c6a4e', '#265e46'])
    S.speckle(img, meadow, '#5eac84', 0.022, 51)
    S.speckle(img, meadow, '#22543e', 0.03, 52)
    # the sea and the beach, bottom right, the moon shining on the water
    sea = poly_mask([(176, 240), (184, 222), (206, 206), (246, 198), (320, 194), (320, 240)])
    beach = poly_mask([(112, 240), (120, 222), (146, 204), (196, 194), (250, 188), (320, 184), (320, 240)])
    S.tone_fill(img, beach, 0.75 - (S.YY - 184) / 56.0 * 0.4, ['#b8a274', '#cdb98a', '#dccb9e', '#e8dab0'])
    S.speckle(img, beach, '#f4ead0', 0.06, 53)
    S.tone_fill(img, sea, 0.7 - (S.YY - 194) / 46.0 * 0.5, ['#1a3a7c', '#21488e', '#2a58a2', '#3668b4'])
    for y in range(200, 240, 4):   # the path of the moon on the water
        w = 8 + (y - 200) // 4
        x0 = 268 - w // 2 + (y * 7) % 5
        S.paint(img, S.rect(x0, y, x0 + w, y) & sea & (((S.XX + y) % 4) != 0), '#f5e27a')
    S.paint(img, sea & (((S.XX * 2 + S.YY * 5) % 19) == 0), '#8ab4ff')
    foam = dilate4(sea) & ~sea & beach
    S.paint(img, foam, '#ffffff')
    S.paint(img, dilate4(foam) & ~foam & beach & ((S.XX % 3) == 0), '#eef4ff')
    # the path, dotted, from place to place
    for i in range(len(LOCS2) - 1):
        (x0, y0), (x1, y1) = LOCS2[i], LOCS2[i + 1]
        y0 -= 2
        y1 -= 2
        n = int(np.hypot(x1 - x0, y1 - y0) / 7)
        for k in range(1, n):
            t = k / n
            x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
            d = S.ellipse(x, y, 2.2, 1.6)
            S.paint(img, dilate4(d) & ~d, '#7a5a2e')
            S.paint(img, d, '#f4dca0')
    # 3: the ice mountains, lit by the moon from the right
    for (x0, x1, top, base) in ((-10, 50, 34, 104), (56, 100, 50, 104), (20, 86, 24, 108)):
        peak = poly_mask([(x0, base), ((x0 + x1) / 2, top), (x1, base)])
        S.outline(img, peak, '#2a4a7a')
        side = (S.XX - (x0 + x1) / 2.0) / ((x1 - x0) / 2.0)
        S.tone_fill(img, peak, 0.5 + side * 0.35 - (S.YY - top) / float(base - top) * 0.15,
                    ['#5a86c0', '#7aa6dc', '#a0c8f0', '#c8e4ff'])
        snow = poly_mask([((x0 + x1) / 2 - (x1 - x0) * 0.18, top + (base - top) * 0.35), ((x0 + x1) / 2, top),
                          ((x0 + x1) / 2 + (x1 - x0) * 0.18, top + (base - top) * 0.35),
                          ((x0 + x1) / 2 + (x1 - x0) * 0.06, top + (base - top) * 0.28),
                          ((x0 + x1) / 2 - (x1 - x0) * 0.05, top + (base - top) * 0.33)])
        S.tone_fill(img, snow & peak, 0.6 + side * 0.3, ['#d8e8ff', '#eef6ff', '#ffffff'])
    for (x, top) in ((30, 104), (52, 106), (70, 104)):   # icicles of crystal at their feet
        c = poly_mask([(x - 3, 112), (x, top - 6), (x + 3, 112)])
        S.outline(img, c, '#2a4a7a')
        S.paint(img, c, '#bfeaff')
        S.paint(img, c & (S.XX <= x), '#ffffff')
    # 4: the smoking volcano: dark rock lit from the right, the glowing crater, the lava
    vol = poly_mask([(118, 110), (146, 48), (174, 48), (204, 110)])
    S.outline(img, vol, INK)
    S.tone_fill(img, vol, 0.45 + (S.XX - 160) / 44.0 * 0.3 - (S.YY - 48) / 62.0 * 0.1,
                ['#2e2440', '#3e3256', '#52446c', '#665884'])
    S.glow(img, 160, 50, 30, 14, '#ff9a3c', 0.35)
    S.paint(img, S.ellipse(160, 49, 14, 3.5), '#ff6a2a')
    S.paint(img, S.ellipse(160, 48.5, 9, 2), '#ffd23f')
    for pts in ([(156, 50), (150, 70), (154, 90), (150, 106)], [(166, 50), (170, 68), (168, 84)]):
        S.line(img, pts, '#ff6a2a', 1.3, vol)
        S.line(img, pts, '#ffd23f', 0.4, vol)
    for (cx, cy, r) in ((164, 34, 7), (172, 24, 6), (166, 14, 5)):
        S.cloud(img, [(cx, cy, r * 1.3, r)], ['#6a6488', '#8a84a8', '#aaa4c4'], ink='#3e3256')
    # 5: the Tower of the Moon on its hill, its windows lit
    hill5 = S.ellipse(272, 100, 46, 15)
    S.outline(img, hill5, '#1c4234')
    S.tone_fill(img, hill5, 0.35 + ((S.XX - 272) / 46.0 - (S.YY - 88) / 15.0) * 0.3, ['#2b5c47', '#357058', '#40846a', '#4c987c'])
    tower = S.rect(260, 30, 284, 94)
    S.outline(img, tower, INK)
    S.tone_fill(img, tower, 0.45 + (S.XX - 272) / 12.0 * 0.3, ['#5e5a80', '#76729a', '#8e8ab2', '#a6a2c8'])
    for y in range(36, 94, 8):
        S.paint(img, S.rect(261, y, 283, y) & tower, '#5e5a80')
        S.paint(img, tower & (S.YY > y) & (S.YY < y + 8) & (((S.XX + (y // 8) * 6) % 12) == 0), '#5e5a80')
    roof = poly_mask([(255, 32), (289, 32), (272, 4)])
    S.outline(img, roof, INK)
    S.tone_fill(img, roof, 0.45 + (S.XX - 272) / 17.0 * 0.35, ['#3a3a9a', '#4a50b8', '#6a74d8', '#8e9af0'])
    crescent(img, 274, 2, 4)
    for (x, y) in ((268, 44), (276, 60), (268, 76)):
        S.glow(img, x, y, 8, 8, '#fff3a6', 0.35)
        S.paint(img, S.rect(x - 2, y - 3, x + 2, y + 3), '#fff3a6')
        S.paint(img, S.rect(x, y - 3, x, y + 3) | S.rect(x - 2, y, x + 2, y), '#e8b84a')
    door = S.ellipse(272, 94, 6, 9) & S.rect(0, 0, W - 1, 94)
    S.paint(img, door, '#2a1e44')
    S.glow(img, 272, 92, 10, 8, '#fff3a6', 0.2)
    # 2: the garden of giants: the big flowers and the glowing mushrooms
    for (cx, top, h, pet) in ((70, 128, 40, ('P', 'p', 'h')), (122, 122, 46, ('Y', 'y', 'o')),
                              (98, 116, 50, ('v', 'L', 'V')), (84, 142, 26, ('X', 'r', 'R'))):
        big_flower(img, cx, top, h, pet, r=8)
    for (cx, by, cap) in ((60, 176, ('#1d6f73', '#2fb3a8', '#7ff0dc', '#d8fff4')),
                          (132, 178, ('#5a2a8a', '#8a5ad0', '#c4a0ff', '#f0e4ff')),
                          (110, 180, ('#1d6f73', '#2fb3a8', '#7ff0dc', '#d8fff4'))):
        S.mushroom_glow(img, cx, by, 1.0, cap, cap[2])
    # 1: shells and a starfish on the beach
    for (cx, cy) in ((132, 226), (168, 232), (186, 214)):
        shell(img, cx, cy)
    starfish(img, 150, 228)
    # 0: the theatre tent (home), its door lit
    T3.striped_tent(img, 30, 206, 34, 46, cols=('#ff93c6', '#fff4fa'))
    S.glow(img, 30, 200, 12, 10, '#fff3a6', 0.3)
    S.paint(img, poly_mask([(30 - 5, 206), (30, 191), (30 + 5, 206)]), '#fff3a6')
    if with_stars:   # fireflies and little flowers that shine in the night
        T3.flowers(img, 50, (0, 112, 200, 236), 9, cols=('#ffcbe3', '#fff3a6', '#bfeaff', '#c4b0ff'),
                   avoid=beach | dilate4(S.rect(10, 150, 54, 206)))
        for (x, y) in ((140, 150), (24, 168), (190, 140), (230, 160), (76, 200), (200, 128), (118, 196)):
            S.firefly(img, x, y)
    anchors = {}
    for i, (x, y) in enumerate(LOCS2):
        anchors['m2_loc%d' % i] = (x, y)
    for i in range(1, 6):
        cx, cy, r = REGS2[i]
        anchors['m2_reg%d' % i] = (cx, cy)
        anchors['m2_rad%d' % i] = (r, 0)
    return img, anchors


def buio(img):
    """Under the wizard's spell: the same map, dark and cold, no stars."""
    f = img[..., :3].astype(np.float32)
    lum = f[..., 0] * 0.3 + f[..., 1] * 0.55 + f[..., 2] * 0.15
    g = np.stack([lum] * 3, axis=-1) * 0.35 + f * 0.25
    tint = np.array([0x14, 0x18, 0x3c], np.float32)
    g = g * 0.8 + tint * 0.35
    out = img.copy()
    out[..., :3] = np.clip(g, 0, 255).astype(np.uint8)
    return out


# ------------------------------------------------------------------ the five places, at night
def spiaggia(with_stars=False, party=False):
    img = canvas()
    rng = np.random.default_rng(31)
    night_sky(img, 124, rng, with_stars, 130)
    crescent(img, 66, 40, 16)
    # the sea with the moon's reflection
    gradient(img, 120, 192, ['#10214a', '#152d5e', '#1b3a73'])
    for y in range(124, 190, 3):
        w = 10 + (y - 124) // 5
        x0 = 72 - w // 2 + ((y * 7) % 5)
        seg = Rm(x0, y, x0 + w, y)
        paint(img, dither(seg, 0.6), '#f5e27a')
    for y in (150, 164, 176, 186):
        for x in range(0, W, 26):
            xx = x + (y * 3) % 26
            paint(img, Rm(xx, y, xx + 9, y), '#4f7ec9')
    # foam and sand
    foam = (YY >= 188) & (YY < 194) & (((XX // 6 + YY) % 3) != 0)
    paint(img, foam, '#dfe8f5')
    ground(img, 194, ['#8f7f5c', '#7c6d4f', '#695c43'], '#a8966c')
    for (cx, cy) in ((34, 214), (120, 222), (230, 212), (284, 226), (176, 230)):
        shell(img, cx, cy)
    starfish(img, 70, 226)
    starfish(img, 262, 218)
    # rocks on the left, a sandcastle on the right
    rock = E(10, 196, 26, 16) | E(34, 202, 16, 10)
    part(img, rock, '1', '2', '0')
    castle = Rm(282, 190, 312, 208) | Rm(286, 180, 294, 190) | Rm(302, 182, 310, 190)
    part(img, castle, '#a8966c', '#c9b58a', '#6e5f45')
    for x in (286, 290, 302, 306):
        paint(img, Rm(x, 178, x + 1, 179), '#a8966c')
    paint(img, Rm(295, 198, 299, 208, 2), '#4a3f2c')
    if party:   # a garland of lanterns and everybody's feet in the sand
        for i in range(12):
            x = 12 + i * 27
            y = 16 + int(10 * np.sin(i * 0.9))
            if i:
                px0 = 12 + (i - 1) * 27
                py0 = 16 + int(10 * np.sin((i - 1) * 0.9))
                for t in np.linspace(0, 1, 30):
                    img[int(py0 + (y - py0) * t + 3 * np.sin(np.pi * t)), int(px0 + (x - px0) * t), :3] = rgb('k')
            lan = Rm(x - 3, y + 1, x + 3, y + 8, 2)
            part(img, lan, *(('y', 'w', 'Y') if i % 2 else ('P', 'p', 'h')))
    return img


def giardino():
    img = canvas()
    rng = np.random.default_rng(32)
    night_sky(img, 196)
    # giant flowers far and near
    for (cx, top, h, pet, r) in ((40, 60, 140, ('v', 'L', 'V'), 16), (118, 84, 116, ('P', 'p', 'h'), 13),
                                 (206, 70, 130, ('Y', 'y', 'o'), 15), (286, 56, 144, ('X', 'r', 'R'), 17)):
        big_flower(img, cx, top, h, pet, r=r)
    blend(img, (YY < 196) & (YY > 40), hexrgb('#141a44'), 0.25)
    for (cx, top, h, pet, r) in ((-4, 100, 110, ('P', 'p', 'h'), 20), (326, 96, 110, ('v', 'L', 'V'), 20)):
        big_flower(img, cx, top, h, pet, r=r)
    # a mushroom as big as a house
    stem = Rm(150, 150, 170, 198, 4)
    part(img, stem, 'W', 'w', 's')
    cap = E(160, 150, 34, 16) & Rm(0, 0, W - 1, 154)
    part(img, cap, 'r', 'P', 'R')
    for (x, y, r) in ((146, 142, 4), (166, 140, 5), (182, 147, 3), (136, 149, 3)):
        paint(img, E(x, y, r, r * 0.8) & cap, 'W')
    ground(img, 196, ['#1f4a33', '#183d2a', '#123222'], '#2d6a44')
    for x in range(4, W, 11):   # tall grass
        hgt = 8 + (x * 7) % 9
        paint(img, Rm(x, 196 - hgt, x, 196), '#2d6a44')
    for (cx, by) in ((24, 212), (64, 206), (256, 210), (300, 214), (210, 220)):
        mushroom(img, cx, by, glow=True)
    sparkle_dots(img, rng, 18, (0, 160, W, 196), ('7', 't'))
    fog(img, 180, 204, '%', 0.3)
    return img


def ghiaccio():
    img = canvas()
    night_sky(img, 196)
    # far and near peaks
    for (x0, x1, top, base) in ((-40, 90, 50, 160), (60, 200, 30, 170), (170, 300, 44, 166), (250, 360, 60, 160)):
        ice_peak(img, x0, x1, top, base, dark=True)
    blend(img, (YY < 170), hexrgb('#141a44'), 0.3)
    for (x0, x1, top, base) in ((-20, 70, 110, 200), (240, 340, 104, 200)):
        ice_peak(img, x0, x1, top, base)
    # ice crystals growing from the snow
    for (cx, by, s) in ((30, 206, 1.0), (72, 210, 0.7), (250, 208, 0.9), (296, 204, 1.1), (160, 214, 0.6)):
        for (dx, hh, w) in ((-6, 20, 4), (0, 30, 5), (6, 16, 4)):
            m = poly_mask([(cx + dx - w * s, by), (cx + dx, by - hh * s), (cx + dx + w * s, by)])
            part(img, m, 't', 'w', 'T')
    ground(img, 200, ['#dfe8f5', '#c9d7ee', '#b3c3e2'], '#ffffff')
    paint(img, dither((YY >= 200), 0.1) & (((XX * 3 + YY * 7) % 13) == 0), '#9fb0d4')
    rng = np.random.default_rng(33)
    for _ in range(90):   # falling snow
        x, y = rng.integers(0, W), rng.integers(0, 196)
        img[y, x, :3] = rgb('w')
    return img


def vulcano():
    img = canvas()
    gradient(img, 0, 200, ['#140b1c', '#22101f', '#341522', '#4a1c22'])
    # the volcano behind, with lava
    cone = poly_mask([(40, 200), (130, 70), (190, 70), (290, 200)])
    part(img, cone, '1', '2', '0')
    crater = E(160, 71, 30, 6)
    paint(img, crater, '@')
    paint(img, E(160, 70, 20, 3), 'Y')
    for (pts) in ([(150, 74), (140, 110), (146, 150), (136, 196)], [(172, 74), (182, 120), (176, 160)]):
        for i in range(len(pts) - 1):
            (x0, y0), (x1, y1) = pts[i], pts[i + 1]
            for t in np.linspace(0, 1, 30):
                paint(img, E(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, 2.4, 2.4), '@')
                paint(img, E(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, 0.9, 0.9), 'Y')
    blend(img, E(160, 72, 60, 30) & ~cone, rgb('@'), 0.18)
    for (cx, cy, r) in ((150, 50, 16), (176, 36, 14), (158, 20, 12), (190, 14, 10)):   # smoke
        part(img, E(cx, cy, r * 1.3, r), '2', '3', '1')
    # dark rocks in front
    for (cx, cy, rx, ry) in ((16, 200, 30, 20), (300, 196, 34, 24), (96, 206, 18, 10), (232, 208, 20, 10)):
        part(img, E(cx, cy, rx, ry), '0', '1', 'K')
    ground(img, 206, ['#241a26', '#1d1520', '#16101a'], '#3a2a36')
    for (x0, y0, x1, y1) in ((20, 222, 70, 230), (110, 232, 170, 226), (210, 220, 260, 236), (280, 228, 318, 222)):
        for t in np.linspace(0, 1, 40):   # glowing cracks
            x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t + 2 * np.sin(t * 9)
            img[int(y), int(x), :3] = rgb('@')
    return img


def torre():
    img = canvas()
    # round stone room
    gradient(img, 0, 196, ['#1d2146', '#262b58', '#2f3566'])
    for y in range(0, 196, 10):
        off = 0 if (y // 10) % 2 == 0 else 12
        img[y, :, :3] = hexrgb('#171a38')
        for x in range(off, W, 24):
            img[y:y + 10, x, :3] = hexrgb('#171a38')
    # a big round window: the moon, and a sky without a single star
    win = E(160, 70, 46, 46)
    paint(img, win, '#0b0e2a')
    crescent(img, 172, 56, 14, 'y', 'o')
    frame = dilate4(dilate4(win)) & ~win
    paint(img, frame, '3')
    paint(img, dilate4(frame) & ~frame & ~win, 'k')
    paint(img, (Rm(159, 24, 160, 116) | Rm(114, 69, 206, 70)) & win, '3')
    # a telescope pointing at the window
    tube = poly_mask([(230, 140), (268, 118), (272, 126), (234, 148)])
    part(img, tube, 'J', 'j', 'q')
    paint(img, E(270, 122, 4, 5), 'b')
    for (x0, y0, x1, y1) in ((244, 142, 234, 196), (244, 142, 254, 196), (244, 142, 244, 196)):
        for t in np.linspace(0, 1, 60):
            img[int(y0 + (y1 - y0) * t), int(x0 + (x1 - x0) * t), :3] = rgb('q')
    # shelves with empty jars (the stars were kept here)
    for (sx, sy) in ((20, 80), (20, 130), (262, 60)):
        shelf = Rm(sx, sy, sx + 50, sy + 3)
        part(img, shelf, 'J', 'j', 'q')
        for k in range(3):
            jx = sx + 6 + k * 16
            jar = Rm(jx, sy - 14, jx + 10, sy - 1, 3)
            part(img, jar, '#9fb0d4', '#dfe8f5', '#6e7fa8')
            paint(img, Rm(jx + 2, sy - 16, jx + 8, sy - 14), 'q')
    # a banner with a crescent
    ban = poly_mask([(90, 118), (116, 118), (116, 180), (103, 170), (90, 180)])
    part(img, ban, 'M', 'm', '$')
    crescent(img, 103, 142, 7, 'Y', 'o')
    # wooden floor
    for y in range(196, H):
        for x in range(0, W):
            k = ((x // 32) + (y - 196) // 11) % 2
            img[y, x, :3] = hexrgb('#5c3a22') if k else hexrgb('#4a2f1b')
    img[196, :, :3] = rgb('k')
    return img


def all_backgrounds():
    import scenery as S   # the places painted with volume (0.8.0)
    import places13 as P13   # 0.13.0: the beach, the garden, the tower painted again
    lit, _ = mappa2(True)
    dark, _ = mappa2(False)
    return {'bg_mappa2': lit, 'bg_mappa2_buia': buio(dark), 'bg_spiaggia': P13.spiaggia(),
            'bg_giardino': P13.giardino(), 'bg_ghiaccio': S.ghiaccio(), 'bg_vulcano': S.vulcano(),
            'bg_torre': P13.torre(), 'bg_festa2': P13.spiaggia(True, True)}


def anchors():
    return mappa2(True)[1]
