"""0.13.0: the places painted again with light and volume, like the places of 0.8 and later (the
forest, the swamp, the cave in scenery.py): the witch's castle (Grisella), the wizard's tower
(Mezzanotte), the garden of giants and the beach of the second adventure (and its party), and the
rooms of three games: the gym, the wizard's laboratory, the room of the measures.

Same places as before, in the same spots: the duel keeps the top left calm (where the monster
floats in its ring of bubbles), the tale has Deva on the left (x ~56-92) and the other character
on the right (x ~238), feet at y ~216; the animated details of amb.c (torches, the cauldron,
the window, fireflies...) sit where these drawings put the things they animate.
"""
import numpy as np

from backgrounds import W, H, canvas, gradient, poly_mask, stars
from pixel import dilate4, rgb
from scenery import (YY, XX, LIGHT_UL, tone_fill, lobe, ellipse, rect, outline, paint, glow, vfade, speckle,
                     line, cloud, puff_row, round_tree, mushroom_glow, firefly, grass_tufts, moon, crescent_face,
                     rock_mass)

INK = '#1a1230'


def _blocks(img, mask, y0, y1, bh, bw, tones, mortar, seed, light=None):
    """Stone blocks: rows of bricks, each its own tone, a darker mortar, lit by `light`
    (a function of (x, y) -> 0..1 added to the tone)."""
    rng = np.random.default_rng(seed)
    t = np.zeros((H, W), np.float32)
    for r, y in enumerate(range(y0, y1, bh)):
        off = (bw // 2) * (r % 2)
        for x in range(-bw, W + bw, bw):
            v = rng.uniform(0.25, 0.6)
            t[y:y + bh, max(0, x + off):max(0, min(W, x + off + bw))] = v
    if light is not None:
        t = t + light
    tone_fill(img, mask, np.clip(t, 0, 0.999), tones)
    rows = mask & (((YY - y0) % bh) == 0)
    cols = np.zeros((H, W), bool)
    for r, y in enumerate(range(y0, y1, bh)):
        off = (bw // 2) * (r % 2)
        for x in range(off % bw, W, bw):
            cols[y:y + bh, x] = True
    paint(img, mask & (rows | cols), mortar)
    # the lit upper edge of every block
    paint(img, mask & (((YY - y0) % bh) == 1) & ~cols, tones[-1]) if len(tones) > 3 else None


# ------------------------------------------------------------------ the witch's castle (adventure 1)
def castello():
    img = canvas()
    wall = YY < 196
    # light: the moon from the window in the middle, the two magic torches
    d_win = np.sqrt(((XX - 160) / 120.0) ** 2 + ((YY - 70) / 110.0) ** 2)
    light = np.clip(0.35 - d_win * 0.35, -0.2, 0.35)
    for tx in (100, 220):
        light = light + 0.28 * np.clip(1.0 - np.sqrt(((XX - tx) / 46.0) ** 2 + ((YY - 84) / 52.0) ** 2), 0, 1)
    _blocks(img, wall, 0, 196, 12, 26, ['#160f2a', '#1f1636', '#291e46', '#352858', '#43336c'], '#120c22', 4,
            light=light - 0.1)
    # the two stone pillars that frame the hall
    for (x0, x1) in ((0, 18), (302, 319)):
        p = rect(x0, 0, x1, 196)
        side = (XX - x0) / float(x1 - x0)
        tone_fill(img, p, 0.25 + side * 0.4 if x0 == 0 else 0.65 - side * 0.4, ['#1a1230', '#271c42', '#362a58', '#463873'])
        paint(img, p & (((YY + 6) % 24) == 0), '#130d24')
        outline(img, p, INK)
    # the tall arched window: the night outside, the moon, stars, and its stained-glass rim
    arch = (rect(128, 26, 192, 122)) | (ellipse(160, 28, 32, 26) & (YY <= 28))
    rim = dilate4(dilate4(dilate4(arch))) & ~arch
    gradient_img = canvas()
    gradient(gradient_img, 0, 122, ['#0b0a26', '#14123a', '#221c52', '#30286a'])
    img[arch, :3] = gradient_img[arch, :3]
    rng = np.random.default_rng(8)
    for _ in range(26):
        x, y = int(rng.integers(130, 191)), int(rng.integers(6, 120))
        if arch[y, x]:
            img[y, x, :3] = rgb('#fff3a6' if rng.random() < 0.3 else '#ffffff')
    glow(img, 176, 46, 26, 26, '#fff4c2', 0.3, mask=arch)
    mo = ellipse(176, 46, 11, 11)
    lobe(img, 176, 46, 11, 11, ['#e8c65a', '#f5dc7a', '#fbeaa0', '#fff6cf'], L=(-0.4, -0.5, 0.8), lo=0.15)
    paint(img, ellipse(180, 42, 2, 2) & mo, '#e8c65a')
    paint(img, ellipse(171, 50, 1.5, 1.5) & mo, '#e8c65a')
    # the rim: panes of coloured glass
    ang = np.arctan2(YY - 60.0, XX - 160.0)
    pane = (np.floor(ang * 4) % 3).astype(int)
    for k, c in enumerate(('#8a3ad0', '#2fb3a8', '#d04a8a')):
        paint(img, rim & (pane == k), c)
    paint(img, dilate4(rim) & ~rim & ~arch, INK)
    paint(img, rim & (((XX + YY) % 7) == 0), '#ffffff')
    # the lead of the window: a cross
    paint(img, (rect(159, 6, 160, 122) | rect(128, 72, 192, 73)) & arch, '#2b2346')
    # the moonbeam on the floor, faint
    beam = poly_mask([(132, 122), (188, 122), (230, 240), (96, 240)]) & (YY >= 122)
    vfade(img, beam, '#a8a0e0', 0.16, 0.06, 122, 240)
    # banners: purple with a golden moon, a fringe, the folds in the light
    for x0 in (34, 256):
        ban = poly_mask([(x0, 14), (x0 + 30, 14), (x0 + 30, 104), (x0 + 15, 94), (x0, 104)])
        outline(img, ban, INK)
        fold = np.cos((XX - x0) / 30.0 * np.pi * 2.0)
        tone_fill(img, ban, 0.45 + fold * 0.25 - (YY - 14) / 90.0 * 0.15, ['#4a1f80', '#6a2ea8', '#8a46cc', '#a86ee8'])
        paint(img, rect(x0 - 2, 10, x0 + 32, 14), '#c88a10')
        paint(img, rect(x0 - 2, 10, x0 + 32, 10), '#ffd23f')
        mm = ellipse(x0 + 15, 48, 8, 8) & ~ellipse(x0 + 19, 44, 7, 7)
        paint(img, mm, '#ffd23f')
        paint(img, mm & (XX < x0 + 13), '#fff080')
        for y in range(70, 92, 5):   # little stars down the banner
            paint(img, rect(x0 + 15, y, x0 + 15, y + 2) | rect(x0 + 14, y + 1, x0 + 16, y + 1), '#ffd23f')
    # the torches: a black iron sconce and a lime magic flame (amb.c makes the glow flicker at 100/220, 82)
    for x in (100, 220):
        glow(img, x, 84, 40, 40, '#b8ff6a', 0.22, mask=wall)
        cup = poly_mask([(x - 5, 92), (x + 5, 92), (x + 3, 98), (x - 3, 98)])
        arm = rect(x - 1, 98, x + 1, 112) | rect(x - 5, 110, x + 5, 112)
        outline(img, cup | arm, INK)
        tone_fill(img, cup | arm, 0.4 + (XX - x) / 10.0 * -0.3, ['#2a2436', '#3e3650', '#5a5070'])
        fl = ellipse(x, 86, 4, 7) | poly_mask([(x - 3, 84), (x + 3, 84), (x, 74)])
        outline(img, fl, '#3a6a10')
        tone_fill(img, fl, 0.9 - (YY - 74) / 18.0 * 0.6, ['#4a9a18', '#7ad62a', '#c8ff6a', '#f4ffd0'])
    # cobwebs in the corners of the window arch and of the hall
    for (cx, cy, sx, sy) in ((18, 0, 1, 1), (301, 0, -1, 1)):
        for r in (8, 15, 22):
            pts = [(cx + sx * r * np.cos(a), cy + sy * r * np.sin(a)) for a in np.linspace(0, np.pi / 2, 12)]
            line(img, pts, '#9a90b8', 0.35)
        for a in np.linspace(0, np.pi / 2, 4):
            line(img, [(cx, cy), (cx + sx * 24 * np.cos(a), cy + sy * 24 * np.sin(a))], '#9a90b8', 0.35)
    # the floor: big stone tiles going away, a carpet up to the window
    floor = YY >= 196
    v = (YY - 196) / 44.0
    xc = (XX - 160) / (0.5 + v * 0.9)
    tile = ((np.floor(xc / 22.0) + np.floor((YY - 196) / (5 + v * 9))) % 2).astype(int)
    tone_fill(img, floor, 0.2 + tile * 0.4 + v * 0.2, ['#18112a', '#231a3a', '#30244c', '#3e3060', '#4c3d72'])
    paint(img, floor & (np.abs((XX - 160) % (22 * (0.5 + v * 0.9))) < 0.9), '#140e24')
    paint(img, rect(0, 196, W - 1, 197), INK)
    rug = poly_mask([(140, 197), (180, 197), (214, 240), (106, 240)])
    outline(img, rug, INK)
    tone_fill(img, rug, 0.35 + v * 0.4, ['#5a1f6a', '#7a2e8a', '#9a46aa'])
    edge = rug & ~poly_mask([(144, 199), (176, 199), (206, 240), (114, 240)])
    paint(img, edge, '#ffd23f')
    vfade(img, floor & beam, '#c8c0f0', 0.18, 0.08, 196, 240)
    # the cauldron: black iron on the floor at the right, a green brew, embers under it
    glow(img, 292, 196, 34, 20, '#8aff4a', 0.25)
    pot = ellipse(292, 204, 19, 13) & (YY >= 192)
    rim_ = ellipse(292, 192, 18, 4.5)
    legs = rect(278, 212, 281, 218) | rect(303, 212, 306, 218)
    outline(img, pot | rim_ | legs, INK)
    lobe(img, 292, 204, 19, 13, ['#120e1a', '#221c2e', '#363048', '#4e4664'], L=LIGHT_UL, clip=YY >= 192)
    paint(img, legs, '#221c2e')
    paint(img, rim_, '#363048')
    brew = ellipse(292, 192, 15, 3)
    tone_fill(img, brew, 0.4 + (XX - 277) / 30.0 * 0.4, ['#4a9a18', '#7ad62a', '#c8ff6a'])
    for (bx, by, r) in ((286, 189, 2.2), (296, 187, 1.6), (291, 184, 1.2)):
        b = ellipse(bx, by, r, r)
        outline(img, b, '#3a6a10')
        paint(img, b, '#c8ff6a')
    for (ex, ey) in ((284, 219), (292, 220), (300, 219), (288, 221), (297, 221)):   # embers
        paint(img, rect(ex, ey, ex + 1, ey), '#ff9a3c')
    glow(img, 292, 220, 16, 4, '#ff6a2a', 0.35)
    # a little table with a potion and a spell book, at the left, behind where Deva stands
    tb = rect(22, 168, 58, 172)
    legs2 = rect(26, 172, 28, 196) | rect(52, 172, 54, 196)
    outline(img, tb | legs2, INK)
    tone_fill(img, tb | legs2, 0.5, ['#2e1f2a', '#46303a', '#5e4450'])
    book = rect(28, 160, 44, 167)
    outline(img, book, INK)
    tone_fill(img, book, 0.5, ['#5a1f6a', '#8a2e9a'])
    paint(img, rect(29, 161, 43, 161), '#ffd23f')
    bot = ellipse(51, 162, 4, 5) | rect(50, 154, 52, 157)
    outline(img, bot, INK)
    lobe(img, 51, 162, 4, 5, ['#1d6f73', '#2fb3a8', '#7ff0dc', '#d8fff4'])
    paint(img, rect(50, 152, 52, 153), '#c88a10')
    glow(img, 51, 162, 10, 10, '#7ff0dc', 0.2)
    # the dark of the corners
    vfade(img, YY < 40, '#0a0716', 0.35, 0.0, 0, 40)
    return img


# ------------------------------------------------------------------ the wizard's tower (adventure 2)
def torre():
    img = canvas()
    wall = YY < 194
    # a round room: the wall darker towards the sides (it curves away), lit by the moon in the window
    curve = 1.0 - ((XX - 160) / 170.0) ** 2
    d_win = np.sqrt(((XX - 160) / 110.0) ** 2 + ((YY - 64) / 90.0) ** 2)
    light = curve * 0.35 + np.clip(0.3 - d_win * 0.3, 0, 0.3) - 0.2
    _blocks(img, wall, 0, 194, 11, 22, ['#0f1430', '#151c40', '#1d2752', '#273466', '#33427a'], '#0b0f24', 21,
            light=light)
    # the big round window: the night, the moon with her sleepy face, stars
    win = ellipse(160, 64, 46, 46)
    sky = canvas()
    gradient(sky, 18, 110, ['#090a26', '#10123a', '#1c1c52', '#2a2a6a'])
    img[win, :3] = sky[win, :3]
    rng = np.random.default_rng(31)
    for _ in range(30):
        x, y = int(rng.integers(114, 206)), int(rng.integers(18, 110))
        if win[y, x]:
            img[y, x, :3] = rgb('#fff3a6' if rng.random() < 0.3 else '#ffffff')
    glow(img, 174, 52, 24, 24, '#fff4c2', 0.3, mask=win)
    cm = ellipse(174, 52, 12, 12) & ~ellipse(180, 47, 10, 10)
    paint(img, cm, '#fbeaa0')
    paint(img, cm & (XX < 168), '#fff6cf')
    crescent_face(img, 174, 52, 12, '#b8862a')
    # its frame: wood and brass, the cross of the window
    ring = ellipse(160, 64, 51, 51) & ~win
    outline(img, ring, INK)
    tone_fill(img, ring, 0.55 - (YY - 64) / 51.0 * 0.3, ['#4a2e1e', '#6e4a2e', '#8f6640', '#b88a56'])
    paint(img, ring & ~ellipse(160, 64, 49, 49), '#c88a10')
    paint(img, (rect(159, 18, 160, 110) | rect(114, 63, 206, 64)) & win, '#5a3a24')
    for (bx, by) in ((160, 13), (160, 115), (109, 64), (211, 64)):   # brass studs
        st = ellipse(bx, by, 2.2, 2.2)
        outline(img, st, INK)
        paint(img, st, '#ffd23f')
    # moonlight from the window, on the floor
    beam = poly_mask([(118, 100), (202, 100), (248, 240), (72, 240)]) & (YY >= 100)
    vfade(img, beam, '#b0b8f0', 0.1, 0.06, 100, 240)
    # shelves on the left: books and jars of stars
    for sy in (58, 104):
        sh = rect(22, sy, 92, sy + 3)
        outline(img, sh, INK)
        tone_fill(img, sh, 0.6, ['#5a3a24', '#8f6640', '#b88a56'])
        x = 24
        for i, (w, h, c) in enumerate(((5, 15, '#c0306a'), (4, 13, '#2fb3a8'), (6, 16, '#4a50b8'),
                                        (5, 12, '#ffb43c'), (4, 15, '#8a46cc'))):
            b = rect(x, sy - h, x + w - 1, sy - 1)
            outline(img, b, INK)
            tone_fill(img, b, 0.6 - (XX - x) / float(w) * 0.4, ['#20142a', c, '#ffffff'], band=0.4)
            paint(img, rect(x, sy - h + 2, x + w - 1, sy - h + 2), '#ffd23f')
            x += w + 1
        for k, jx in enumerate((58, 70, 82)):   # jars with a little star inside, glowing
            jar = rect(jx - 4, sy - 12, jx + 4, sy - 1)
            outline(img, jar, INK)
            paint(img, jar, '#2a3a6a')
            paint(img, jar & (XX == jx - 3), '#6a7ab8')
            paint(img, rect(jx - 4, sy - 14, jx + 4, sy - 13), '#8f6640')
            paint(img, rect(jx, sy - 8, jx, sy - 6) | rect(jx - 1, sy - 7, jx + 1, sy - 7),
                   ('#fff3a6', '#7ff0dc', '#ffb4d8')[k])
            glow(img, jx, sy - 7, 9, 9, ('#fff3a6', '#7ff0dc', '#ffb4d8')[k], 0.25)
    # a chart of the stars on the right wall
    ch = rect(236, 20, 292, 64)
    outline(img, ch, INK)
    tone_fill(img, ch, 0.6 + (XX - 236) / 56.0 * 0.2, ['#c8b48a', '#e0cfa4', '#f0e2bc'])
    for (ax, ay, bx2, by2) in ((244, 30, 256, 36), (256, 36, 266, 30), (266, 30, 280, 42), (250, 50, 262, 56),
                               (262, 56, 276, 52)):
        line(img, [(ax, ay), (bx2, by2)], '#8a7a5a', 0.3)
    for (sx_, sy_) in ((244, 30), (256, 36), (266, 30), (280, 42), (250, 50), (262, 56), (276, 52)):
        paint(img, rect(sx_ - 1, sy_, sx_ + 1, sy_) | rect(sx_, sy_ - 1, sx_, sy_ + 1), '#4a50b8')
    # planets hanging from the ceiling, on strings
    for (px, py, r, tones) in ((222, 34, 5, ['#c0306a', '#ff7a9a', '#ffd0dc']), (300, 50, 4, ['#2a8a7a', '#4fd6c0', '#c8fff4']),
                               (30, 26, 4, ['#c07a20', '#ffb43c', '#ffe08a'])):
        line(img, [(px, 0), (px, py - r)], '#8a7ab0', 0.3)
        m = lobe(img, px, py, r, r, tones)
        outline(img, m, INK)
    paint(img, ellipse(222, 34, 9, 1.6) & ~ellipse(222, 34, 5, 5), '#ffd23f')   # a ring
    # the telescope on its tripod, looking at the window (its lens glints: amb.c, at 270, 122)
    for (fx2, fy2) in ((256, 196), (272, 196), (290, 196)):
        line(img, [(272, 136), (fx2, fy2)], '#5a3a24', 0.9)
    tube = poly_mask([(250, 138), (276, 116), (282, 124), (256, 145)])
    outline(img, tube, INK)
    tone_fill(img, tube, 0.6 - (YY - 116) / 30.0 * 0.4, ['#7a4a10', '#c88a10', '#ffd23f', '#fff080'])
    lens = ellipse(279, 120, 3.5, 3.5)
    outline(img, lens, INK)
    paint(img, lens, '#bfeaff')
    paint(img, rect(277, 118, 278, 119), '#ffffff')
    paint(img, ellipse(272, 136, 2.5, 2.5), '#3e2a1a')
    # the wooden floor, its boards going away; a round rug with stars
    floor = YY >= 194
    v = (YY - 194) / 46.0
    board = np.floor((XX - 160) / (14 + 26 * v)).astype(int)
    tone_fill(img, floor, 0.25 + (board % 2) * 0.2 + v * 0.3, ['#2e1f1a', '#3e2a20', '#523828', '#664832'])
    edge = np.floor((XX - 1 - 160) / (14 + 26 * v)).astype(int) != board
    paint(img, floor & edge, '#24170f')
    paint(img, rect(0, 194, W - 1, 195), INK)
    rug = ellipse(160, 216, 92, 16)
    outline(img, rug, INK)
    tone_fill(img, rug, 0.35 + (YY - 200) / 32.0 * 0.3, ['#1d2a6a', '#2a3a8a', '#3a4aa8'])
    paint(img, rug & ~ellipse(160, 216, 86, 13) , '#ffd23f')
    for (rx, ry) in ((120, 212), (160, 220), (200, 212), (140, 224), (182, 224), (160, 208)):
        paint(img, rect(rx, ry - 1, rx, ry + 1) | rect(rx - 1, ry, rx + 1, ry), '#fff3a6')
    vfade(img, floor & beam, '#c8d0ff', 0.14, 0.06, 194, 240)
    vfade(img, YY < 36, '#060818', 0.3, 0.0, 0, 36)
    return img


# ------------------------------------------------------------------ the garden of giants (adventure 2)
def _giant_flower(img, cx, top, h, tones, centre=('#c07a20', '#ffb43c', '#ffe08a'), r=16, lean=0.0, seed=0, lit=0.0):
    """A flower taller than a house: a curved stem, two big leaves, a head of eight lit petals
    (lit = how much the glowing garden lights it from below)."""
    greens = ['#0e2a24', '#164034', '#205a44', '#2e7a58']
    stem_pts = [(cx + lean * 10 * np.sin(t * 2.0) + lean * t * 14, top + h * t) for t in np.linspace(0, 1, 90)]
    stem = np.zeros((H, W), bool)
    for (x, y) in stem_pts:
        stem |= ellipse(x, y, 2.6, 2.6)
    outline(img, stem, INK)
    sx = np.interp(YY, [p[1] for p in stem_pts], [p[0] for p in stem_pts])   # the stem's middle, row by row
    tone_fill(img, stem, 0.55 + (XX - sx) / 2.6 * 0.3, greens)
    for (side, ly) in ((-1, 0.55), (1, 0.72)):   # two leaves, curved, a vein down the middle
        lx, lyy = stem_pts[int(ly * 89)]
        tip = (lx + side * 34, lyy - 14)
        edge_a, edge_b = [], []
        for t in np.linspace(0, 1, 14):
            mx, my = lx + (tip[0] - lx) * t, lyy + (tip[1] - lyy) * t - 6 * np.sin(np.pi * t)
            wdt = 7 * np.sin(np.pi * t ** 0.8)
            edge_a.append((mx, my - wdt))
            edge_b.append((mx, my + wdt))
        leaf = poly_mask(edge_a + edge_b[::-1])
        outline(img, leaf, INK)
        tone_fill(img, leaf, 0.35 + (-(YY - lyy) / 14.0) * 0.35, greens)
        line(img, [(lx + (tip[0] - lx) * t, lyy + (tip[1] - lyy) * t - 6 * np.sin(np.pi * t))
                   for t in np.linspace(0, 0.9, 10)], greens[0], 0.4, leaf)
    head = np.zeros((H, W), bool)
    petals = []
    for i in range(8):
        a = i * np.pi / 4 + seed * 0.3
        px, py = cx + np.cos(a) * r * 0.85, top + np.sin(a) * r * 0.7
        petals.append((px, py))
        head |= ellipse(px, py, r * 0.6, r * 0.5)
    outline(img, head, INK)
    for (px, py) in sorted(petals, key=lambda p: p[1]):
        lobe(img, px, py, r * 0.6, r * 0.5, tones, L=(-0.3, 0.6 if lit else -0.6, 0.6))
    c = ellipse(cx, top, r * 0.42, r * 0.36)
    outline(img, c, INK)
    lobe(img, cx, top, r * 0.42, r * 0.36, list(centre), L=(-0.4, -0.5, 0.7))
    rng = np.random.default_rng(seed + 7)
    for _ in range(int(r)):
        x, y = cx + rng.uniform(-r * 0.3, r * 0.3), top + rng.uniform(-r * 0.25, r * 0.25)
        if c[int(y), int(x)]:
            img[int(y), int(x), :3] = rgb(centre[0])
    if lit:
        glow(img, cx, top + r * 0.6, r * 1.6, r * 0.8, '#7ff0dc', lit, mask=head)
    return head | stem


def giardino():
    img = canvas()
    gradient(img, 0, 196, ['#0c0d2c', '#131646', '#1c205a', '#272b6c', '#33367a'])
    glow(img, 160, 196, 260, 90, '#3a6a8a', 0.35)               # the glow of the garden on the haze
    # far away: giant flowers in the mist, and the hills
    hills = poly_mask([(0, 196)] + [(x, 160 + 7 * np.sin(x / 31.0) + 4 * np.sin(x / 11.0)) for x in range(0, W + 8, 8)]
                      + [(W, 196)])
    tone_fill(img, hills, 0.5, ['#16264a', '#1c2e56', '#223662'])
    for (cx, top, hh) in ((70, 108, 60), (132, 122, 46), (196, 112, 56), (262, 118, 50)):
        st = rect(cx - 1, top, cx + 1, top + hh)
        paint(img, st, '#1e3a52')
        hd = np.zeros((H, W), bool)
        for i in range(8):
            a = i * np.pi / 4
            hd |= ellipse(cx + np.cos(a) * 7, top + np.sin(a) * 6, 5, 4)
        paint(img, hd, '#2c4a72')
        paint(img, ellipse(cx, top, 3, 3), '#4a6a90')
    vfade(img, (YY > 90) & (YY < 196), '#3a5080', 0.0, 0.35, 96, 190)
    # a mushroom as big as a house, far in the middle
    mstem = rect(150, 150, 170, 196)
    mcap = ellipse(160, 152, 34, 15) & (YY <= 156)
    outline(img, mstem | mcap, INK)
    tone_fill(img, mstem, 0.5 + (XX - 150) / 20.0 * -0.3, ['#8a8aa8', '#b0b0c8', '#d4d4e4'])
    lobe(img, 160, 156, 34, 18, ['#6a1a3a', '#9a2a4e', '#c8406a', '#e86a8a'], L=(-0.3, -0.7, 0.6), clip=mcap)
    for (x, y, r) in ((146, 144, 4), (166, 141, 5), (182, 148, 3), (136, 150, 3)):
        paint(img, ellipse(x, y, r, r * 0.8) & mcap, '#f4e4f0')
    door = ellipse(160, 196, 5, 9) & (YY < 196)
    paint(img, door, '#3a2a40')
    glow(img, 160, 190, 12, 10, '#ffd88a', 0.35)
    paint(img, rect(156, 178, 158, 180) | rect(162, 178, 164, 180), '#ffd88a')   # its little windows
    vfade(img, (YY > 130) & (YY < 196) & (XX > 120) & (XX < 200), '#3a5080', 0.0, 0.25, 130, 196)
    # the giants near us, at the sides, lit from below by the glowing garden
    _giant_flower(img, 26, 54, 150, ['#4a1a6a', '#6a2a9a', '#8a46cc', '#b07af0'], r=20, lean=0.3, seed=1, lit=0.25)
    _giant_flower(img, 92, 92, 110, ['#7a1a4a', '#b02a6a', '#e0508a', '#ff8ab4'], r=14, lean=-0.2, seed=2, lit=0.2)
    _giant_flower(img, 236, 84, 120, ['#7a5a10', '#c08a20', '#ffc23c', '#ffe08a'], r=16, lean=0.2, seed=3, lit=0.2)
    _giant_flower(img, 300, 46, 160, ['#6a1220', '#a02030', '#e0404a', '#ff7a7a'], r=21, lean=-0.3, seed=4, lit=0.25)
    # the ground: dark grass, a path of pale pebbles to the mushroom house
    ground = YY >= 194
    gradient(img, 194, H, ['#14332a', '#102a22', '#0c221c'])
    path = poly_mask([(132, 240), (190, 240), (176, 214), (166, 198), (154, 198), (146, 214)])
    tone_fill(img, path, 0.3 + (YY - 196) / 44.0 * 0.4, ['#2a3a40', '#36484e', '#44585c'])
    rng = np.random.default_rng(40)
    for _ in range(26):
        x, y = int(rng.integers(134, 190)), int(rng.integers(198, 238))
        if path[y, x]:
            paint(img, ellipse(x, y, 2.4, 1.4), '#6a7e84')
    speckle(img, ground & ~path, '#1f4a38', 0.07, 41)
    grass_tufts(img, 192, 238, ['#245a40', '#2e6a4c', '#1a4434'], 42, n=80, avoid=path)
    for (cx, by, s) in ((14, 214, 1.1), (44, 204, 0.8), (118, 222, 0.9), (204, 226, 1.0), (276, 206, 0.8),
                        (306, 218, 1.1), (250, 232, 0.9)):
        mushroom_glow(img, cx, by, s)
    for (cx, by, s, cap) in ((70, 214, 0.9, ('#5a2a8a', '#8a5ad0', '#c4a0ff', '#f0e4ff')),
                             (226, 214, 0.8, ('#8a1f33', '#c21f45', '#ff7a8a', '#ffd0dc'))):
        mushroom_glow(img, cx, by, s, cap, cap[2])
    for (x, y) in ((60, 150), (120, 128), (210, 140), (250, 166), (30, 180), (290, 150)):   # dew sparkling
        paint(img, rect(x, y, x, y), '#d8fff4')
    vfade(img, (YY > 184) & (YY < 204), '#7a9ac0', 0.0, 0.16, 184, 204)
    return img


# ------------------------------------------------------------------ the beach at night (adventure 2) and its party
def spiaggia(with_stars=False, party=False):
    img = canvas()
    gradient(img, 0, 126, ['#0a0e2c', '#101840', '#182254', '#222e68', '#2c3a78'])
    rng = np.random.default_rng(31)
    if with_stars:
        stars(img, rng, 120, (0, 0, W, 112))
        for (x, y) in ((140, 20), (208, 44), (300, 30), (24, 70), (250, 82), (118, 60)):
            paint(img, rect(x, y - 2, x, y + 2) | rect(x - 2, y, x + 2, y), '#fff3a6')
            paint(img, rect(x, y, x, y), '#ffffff')
    glow(img, 66, 40, 50, 46, '#fff4c2', 0.25)
    cm = ellipse(66, 40, 16, 16) & ~ellipse(73, 34, 13.5, 13.5)
    outline(img, cm, '#b8862a')
    paint(img, cm, '#fbeaa0')
    paint(img, cm & (XX < 58), '#fff6cf')
    crescent_face(img, 66, 40, 16, '#b8862a')
    # far away: a little island with a lighthouse, its light turning
    isl = ellipse(150, 126, 24, 6) & (YY <= 126)
    paint(img, isl, '#141c3a')
    lh = rect(147, 104, 153, 122)
    outline(img, lh, INK)
    paint(img, lh, '#d8d4e8')
    paint(img, lh & (((YY - 104) // 4) % 2 == 1), '#c0306a')
    paint(img, rect(146, 100, 154, 103), '#3a3a5a')
    paint(img, rect(148, 100, 152, 102), '#fff3a6')
    glow(img, 150, 101, 24, 10, '#fff3a6', 0.35)
    # the sea: bands of moonlit waves, the moon's road on the water
    sea = (YY >= 126) & (YY < 192)
    gradient(img, 126, 192, ['#0e1a44', '#122252', '#162a60', '#1c346e'])
    for y in range(130, 190, 5):
        for x in range((y * 37) % 23, W, 23 + (y % 7)):
            w = 6 + (y - 126) // 8
            paint(img, rect(x, y, x + w, y) & sea, '#3a5aa0')
    for y in range(128, 188, 3):
        w = 6 + (y - 126) // 5
        x0 = 72 - w // 2 + ((y * 7) % 5)
        paint(img, rect(x0, y, x0 + w, y) & sea & (((XX + y) % 4) != 0), '#f5e27a')
    paint(img, rect(0, 126, W - 1, 126), '#3a4a88')
    # the foam, then the sand in the moonlight
    shore = 192 + 2 * np.sin(XX / 13.0)
    foam = (YY >= shore - 3) & (YY < shore + 1)
    paint(img, foam & (((XX // 3) + YY) % 3 != 0), '#e8f0ff')
    paint(img, (YY >= shore - 5) & (YY < shore - 3) & ((XX % 5) < 3), '#8aa8e0')
    sand = YY >= shore + 1
    tone_fill(img, sand, 0.7 - (YY - 194) / 46.0 * 0.4, ['#6a5a40', '#7e6c4e', '#94805e', '#a8946c'])
    speckle(img, sand, '#b8a47c', 0.05, 33)
    speckle(img, sand, '#5a4c36', 0.04, 34)
    for (x, y) in ((96, 214), (104, 220), (112, 214), (120, 220), (128, 214)):   # little footprints
        paint(img, ellipse(x, y, 2, 1.2), '#5e4e38')
    # rocks at the left, a tide pool
    rock_mass(img, [(10, 194, 26, 16), (34, 202, 16, 10), (-4, 210, 18, 12)],
              ['#141428', '#1e1e3a', '#2a2a4e', '#3a3a64', '#4c4c7a'], ink='#0a0a18')
    pool = ellipse(40, 214, 10, 3)
    paint(img, pool, '#2a4a8a')
    paint(img, pool & (XX < 36), '#4a6ab0')
    # the sandcastle at the right: towers, a gate, little flags
    sc = rect(278, 192, 316, 210)
    t1, t2, t3 = rect(280, 180, 290, 192), rect(304, 180, 314, 192), rect(291, 172, 303, 192)
    cast = sc | t1 | t2 | t3
    for tw in (t1, t2, t3):
        x0 = int(np.where(tw)[1].min())
        x1 = int(np.where(tw)[1].max())
        top = int(np.where(tw)[0].min())
        for x in range(x0, x1 + 1, 3):
            cast |= rect(x, top - 2, x + 1, top - 1)
    outline(img, cast, '#3e3220')
    tone_fill(img, cast, 0.6 - (XX - 278) / 38.0 * 0.4, ['#6e5c40', '#8a7654', '#a8926a', '#c4ae84'])
    paint(img, ellipse(297, 210, 4, 7) & (YY < 210), '#3e3220')
    for (fx, fy, c) in ((285, 172, '#e83a4a'), (297, 164, '#ffd23f'), (309, 172, '#4fd6c0')):
        paint(img, rect(fx, fy, fx, fy + 6), '#3e3220')
        paint(img, poly_mask([(fx + 1, fy), (fx + 6, fy + 2), (fx + 1, fy + 4)]), c)
    # a bucket and a spade, shells and starfish
    bk = poly_mask([(240, 206), (252, 206), (250, 218), (242, 218)])
    outline(img, bk, '#3e3220')
    tone_fill(img, bk, 0.6 - (XX - 240) / 12.0 * 0.5, ['#a02a6a', '#e0508a', '#ff8ab4'])
    line(img, [(240, 206), (246, 200), (252, 206)], '#3e3220', 0.4)
    sp_ = poly_mask([(256, 214), (262, 200), (264, 201), (259, 215)])
    outline(img, sp_, '#3e3220')
    paint(img, sp_, '#ffd23f')
    for (sx_, sy_, c) in ((34, 226, '#ffcbe3'), (120, 230, '#fff4fa'), (176, 220, '#ffcbe3'), (214, 232, '#ffe0c0')):
        shell = ellipse(sx_, sy_, 4, 3) & (YY <= sy_ + 1)
        outline(img, shell, '#5e4e38')
        paint(img, shell, c)
        for dx in (-2, 0, 2):
            paint(img, rect(sx_ + dx, sy_ - 2, sx_ + dx, sy_), '#c8a0b0')
    for (stx, sty) in ((70, 226), (262, 226)):
        arms = np.zeros((H, W), bool)
        for i in range(5):
            a = -np.pi / 2 + i * 2 * np.pi / 5
            arms |= poly_mask([(stx + np.cos(a - 0.5) * 2, sty + np.sin(a - 0.5) * 2),
                               (stx + np.cos(a) * 6.5, sty + np.sin(a) * 6.5),
                               (stx + np.cos(a + 0.5) * 2, sty + np.sin(a + 0.5) * 2)]) | ellipse(stx, sty, 2.6, 2.6)
        outline(img, arms, '#7a3a10')
        paint(img, arms, '#ffa040')
        paint(img, ellipse(stx, sty, 1.2, 1.2), '#ffd080')
    if party:   # a garland of lanterns across the sky, swinging from one end to the other
        pts = [(x, 18 + 8 * np.sin(x / 40.0) + 6 * np.sin(np.pi * x / W)) for x in range(-4, W + 8, 4)]
        line(img, pts, '#2a2040', 0.5)
        for i, x in enumerate(range(10, W, 26)):
            y = int(18 + 8 * np.sin(x / 40.0) + 6 * np.sin(np.pi * x / W))
            c = (['#ffd23f', '#fff3a6', '#ffe08a'], ['#ff7ab8', '#ffb4d8', '#ffd0e4'],
                 ['#4fd6c0', '#a0f0e4', '#d8fff8'])[i % 3]
            glow(img, x, y + 6, 12, 12, c[1], 0.3)
            lan = rect(x - 3, y + 2, x + 3, y + 10)
            outline(img, lan, '#2a2040')
            tone_fill(img, lan, 0.6 - (XX - x) / 6.0, c, band=0.5)
            paint(img, rect(x - 3, y + 1, x + 3, y + 1) | rect(x - 3, y + 11, x + 3, y + 11), '#2a2040')
    return img


# ------------------------------------------------------------------ helpers of the rooms
WOOD = ['#6a3e1c', '#8e5a2a', '#b47a3c', '#d49a56', '#ecbe7e', '#f8daa8']


def _rr(x0, y0, x1, y1, r=0):
    from pixel import rect_mask
    return rect_mask(W, H, x0, y0, x1, y1, radius=r)


def _blend(img, mask, color, a):
    """Mix a colour over the mask (a = 0..1), flat."""
    c = np.array(rgb(color), np.float32)
    px = img[mask, :3].astype(np.float32)
    img[mask, :3] = np.clip(px * (1 - a) + c * a, 0, 255).astype(np.uint8)


def _shadow(img, mask, dx, dy, color, a, under=None):
    """The shadow a thing casts on the wall behind it: its mask moved by (dx, dy), mixed in."""
    m = np.roll(np.roll(mask, dy, axis=0), dx, axis=1) & ~mask
    if under is not None:
        m &= under
    _blend(img, m, color, a)


def _planks(img, y0, vx, w0, w1, tones, edge, seed, joint=23, t0=0.3):
    """A wooden floor whose boards run away from us (towards x = vx): every board its own tone,
    darker edges, the joints of the boards staggered."""
    floor = YY >= y0
    v = (YY - y0) / float(H - y0)
    ws = w0 + w1 * v
    board = np.floor((XX - vx) / ws).astype(int)
    var = np.random.default_rng(seed).uniform(-0.07, 0.07, 64)
    t = t0 + (board % 2) * 0.08 + v * 0.35 + var[board % 64]
    tone_fill(img, floor, t, tones)
    edge_m = np.floor((XX - 1 - vx) / ws).astype(int) != board
    paint(img, floor & edge_m, edge)
    joints = floor & ((((YY - y0) + (board * 13) % joint) % joint) == 0) & ~edge_m
    paint(img, joints, edge)
    return floor


def _hull_beam(img, pts, mask, color, a0, a1, y0, y1):
    """A shaft of light: a polygon, fading with height."""
    vfade(img, poly_mask(pts) & mask, color, a0, a1, y0, y1)


# ------------------------------------------------------------------ the gym (Ginnastica)
def palestra():
    """The gym: the same room (wall bars on the left, the window on the right, the blue mat in the
    middle), now with the sun coming in, wood you could climb, a padded wall, a polished floor."""
    img = canvas()
    wall = YY < 172
    d = np.sqrt(((XX - 266) / 170.0) ** 2 + ((YY - 64) / 130.0) ** 2)
    lit = np.clip(0.42 - d * 0.42, 0, 0.42)
    t = 0.5 + lit - np.clip((36 - YY) / 36.0, 0, 1) * 0.22
    tone_fill(img, wall, t, ['#b6a4e0', '#c3b2e8', '#cfc0ef', '#dacef5', '#e5dcf9', '#efe9fc', '#f7f4fe'])
    # a band of colour along the wall
    tone_fill(img, rect(0, 108, W - 1, 112), t - 0.1, ['#d8669a', '#e87aa8', '#f494bc', '#ffb0cf', '#ffc8de'])
    paint(img, rect(0, 108, W - 1, 108), '#ffd6e6')
    tone_fill(img, rect(0, 115, W - 1, 116), t - 0.1, ['#d8a020', '#e8b830', '#f6cc48', '#ffde6a', '#ffeca0'])
    # the padded wall, low, from the wall bars to the right
    for x in range(84, W, 32):
        p = _rr(x + 1, 134, x + 30, 164, 4)
        u = (XX - x - 1) / 29.0
        w_ = (YY - 134) / 30.0
        puff = np.sin(np.pi * np.clip(u, 0, 1)) * np.sin(np.pi * np.clip(w_, 0, 1))
        tone_fill(img, p, 0.2 + 0.35 * puff + 0.25 * (1 - w_) + lit * 0.6,
                  ['#24787a', '#2e8e8e', '#3aa6a2', '#4ebcb6', '#6ad0c8', '#92e2d8', '#bcf0e8'])
        outline(img, p, '#1a5a5e')
        bx, by = x + 15, 149
        paint(img, rect(bx, by, bx + 1, by + 1), '#1a5a5e')
        paint(img, rect(bx, by, bx, by), '#bcf0e8')
    # the skirting board
    tone_fill(img, rect(0, 165, W - 1, 171), 0.9 - (YY - 165) / 6.0 * 0.7 + lit * 0.3,
              ['#8a7ab8', '#a898cc', '#c4b8e0', '#e2dcf2', '#f6f2fc'])
    paint(img, rect(0, 164, W - 1, 164), '#7a6aa8')
    # the wall bars (spalliera): two posts, round rungs, their shadow on the wall
    posts = rect(12, 30, 18, 171) | rect(68, 30, 74, 171)
    caps = ellipse(15.5, 30, 3.6, 2.6) | ellipse(71.5, 30, 3.6, 2.6)
    rungs = np.zeros((H, W), bool)
    for y in range(42, 166, 12):
        rungs |= rect(19, y, 67, y + 2)
    bars = posts | caps | rungs
    _shadow(img, bars, 4, 3, '#5a4a8a', 0.28, under=wall)
    outline(img, bars, INK)
    for x0 in (12, 68):
        tone_fill(img, rect(x0, 28, x0 + 6, 171) & (posts | caps), 0.85 - (XX - x0) / 6.0 * 0.75, WOOD)
    for y in range(42, 166, 12):
        tone_fill(img, rect(19, y, 67, y + 2), 0.95 - (YY - y) / 2.0 * 0.5, WOOD)
        paint(img, rect(19, y + 2, 67, y + 2) & (XX % 7 == 3), WOOD[1])
    # the sunbeam through the window, faint on the wall
    beam_pts = [(237, 37), (297, 37), (297, 93), (262, 182), (236, 212), (160, 212)]
    _hull_beam(img, beam_pts, wall, '#fff6d8', 0.2, 0.08, 37, 172)
    # the window: a deep white frame, the sky, the sun, a hill with a tree, a sill
    frame = _rr(230, 30, 304, 100, 3)
    sill = rect(226, 100, 308, 104)
    glass = rect(237, 37, 297, 93)
    outline(img, frame | sill, INK)
    tone_fill(img, frame, 0.85 - (XX - 230) / 74.0 * 0.25 - (YY - 30) / 70.0 * 0.3,
              ['#9a8cc8', '#bcb0e0', '#dcd4f2', '#f2eefc', '#ffffff'])
    paint(img, rect(235, 35, 299, 95) & ~glass, '#a89ad4')       # the inner lip, in shadow
    paint(img, rect(235, 95, 299, 95), '#e2dcf2')
    sky = canvas()
    gradient(sky, 37, 94, ['#58b0f4', '#7cc4fa', '#a4d8ff', '#c8eaff'], 237, 298)
    img[glass, :3] = sky[glass, :3]
    glow(img, 285, 47, 20, 20, '#fff6c0', 0.45, mask=glass)
    lobe(img, 285, 47, 6, 6, ['#ffc23c', '#ffd860', '#ffec9a', '#fffbe0'], L=(-0.3, -0.4, 0.8), clip=glass)
    far = glass & (YY > 76 + 4 * np.sin((XX - 230) / 12.0))
    paint(img, far, '#9ad6b4')
    hill = glass & (YY > 82 + 4 * np.sin((XX - 237) / 9.0) + 1.5 * np.sin(XX / 3.7))
    tone_fill(img, hill, 0.8 - (YY - 80) / 14.0 * 0.6, ['#3a9a4a', '#52b45a', '#6ccc6a', '#8ee080'])
    paint(img, rect(250, 76, 251, 84) & glass, '#7a5a3a')
    lobe(img, 250.5, 74, 5, 4.5, ['#2e8a3e', '#46a650', '#62c062', '#8ad880'], L=(0.4, -0.6, 0.6))
    cloud(img, [(252, 58, 8, 5), (262, 54, 9, 7), (273, 59, 8, 5)], ['#c4dcf4', '#e2eefc', '#ffffff'])
    shine = glass & ((((XX + YY) % 46) < 3) | (((XX + YY) % 46) == 5))
    _blend(img, shine, '#ffffff', 0.3)
    paint(img, (rect(266, 37, 267, 93) | rect(237, 64, 297, 65)), '#f2eefc')
    paint(img, (rect(268, 37, 268, 93) | rect(237, 66, 297, 66)) & glass, '#bcb0e0')
    tone_fill(img, sill, 0.95 - (YY - 100) / 4.0 * 0.7, ['#9a8cc8', '#bcb0e0', '#dcd4f2', '#f2eefc', '#ffffff'])
    # two rings hanging from the ceiling, on ropes
    for rx in (204, 220):
        rope = rect(rx, 0, rx + 1, 37)
        paint(img, rope, '#c8a070')
        paint(img, rope & (((YY + (XX - rx) * 2) % 4) < 2), '#9a7444')
        ring = ellipse(rx + 1, 45, 6.5, 6.5) & ~ellipse(rx + 1, 45, 4, 4)
        _shadow(img, ring, 3, 2, '#5a4a8a', 0.25, under=wall)
        outline(img, ring, INK)
        tone_fill(img, ring, 0.75 - (YY - 38) / 13.0 * 0.6 - (XX - rx + 5) / 13.0 * 0.15, WOOD[1:])
        strap = rect(rx - 1, 35, rx + 2, 39)
        paint(img, strap, '#3a3060')
        paint(img, rect(rx - 1, 35, rx - 1, 39), '#6a5aa0')
    # the pennants across the top, in two swags
    sag = lambda x: 6 + 7 * np.sin(np.pi * (x % 160) / 160.0)
    cols = (['#c83a78', '#e85a96', '#ff86b4'], ['#d89a10', '#f4be2a', '#ffd860'], ['#1e9a8e', '#36bcae', '#6adcd0'],
            ['#7a3ac0', '#9a5ad8', '#bc86ee'], ['#2a6ad0', '#4a8ae8', '#7aaaf4'], ['#d0602a', '#ec8040', '#ffa868'])
    for i, x in enumerate(range(4, W - 12, 18)):
        a, b = (x, sag(x)), (x + 13, sag(x + 13))
        tip = (x + 6.5, max(a[1], b[1]) + 13)
        tri = poly_mask([a, b, tip])
        _shadow(img, tri, 2, 3, '#5a4a8a', 0.2, under=wall)
        c = cols[i % len(cols)]
        outline(img, tri, c[0])
        tone_fill(img, tri, 0.95 - (XX - x) / 13.0 * 0.8, [c[0], c[1], c[2]], band=0.5)
    line(img, [(x, sag(x)) for x in range(0, W + 1, 2)], '#4a3a6a', 0.5)
    for nx in (1, 160, 318):
        paint(img, rect(nx - 1, 5, nx + 1, 7), '#4a3a6a')
    # the floor: polished boards going away, the shadow along the wall, the window shining on it
    _planks(img, 172, 160, 10, 26, ['#b06a32', '#c47e42', '#d49254', '#e2a868', '#eebe80', '#f8d49c'], '#94582a', 5)
    vfade(img, rect(0, 172, W - 1, 178), '#5a3418', 0.35, 0.0, 172, 178)
    vfade(img, rect(236, 173, 298, 192), '#fff6e0', 0.3, 0.0, 173, 192)
    # the mat: six puffy panels, a thick front edge, the piping
    top_ = poly_mask([(58, 182), (262, 182), (282, 212), (38, 212)])
    front = rect(38, 213, 282, 217)
    s = np.clip((YY - 182) / 30.0, 0, 1)
    xl, xr = 58 - 20 * s, 262 + 20 * s
    f = (XX - xl) / (xr - xl)
    u = (f * 6) % 1.0
    blues = ['#1c4490', '#2858b0', '#346ecc', '#4686e2', '#62a0f0', '#8cbcf8', '#b8d8fc']
    _blend(img, poly_mask([(56, 214), (286, 214), (290, 220), (34, 220)]), '#3a2010', 0.35)   # its shadow
    outline(img, top_ | front, INK)
    tone_fill(img, top_, 0.2 + 0.42 * np.sin(np.pi * u) + 0.22 * (1 - s), blues)
    k6 = np.round(f * 6)
    seam = top_ & (np.abs(k6 - f * 6) * (xr - xl) / 6.0 < 0.55) & (k6 > 0) & (k6 < 6)
    paint(img, seam, blues[0])
    paint(img, top_ & (YY == 212), blues[5])
    ff = (XX - 38) / 244.0
    tone_fill(img, front, 0.1 + 0.3 * np.sin(np.pi * ((ff * 6) % 1.0)) - (YY - 213) / 5.0 * 0.1, blues[:4])
    paint(img, front & (np.abs(np.round(ff * 6) - ff * 6) * 244 / 6.0 < 0.55), INK)
    # the beam on the floor, and the bright window it makes on the mat
    _hull_beam(img, beam_pts, (YY >= 172) & (YY < 212), '#fff6d8', 0.1, 0.12, 172, 212)
    patch = poly_mask([(196, 182), (262, 182), (236, 212), (160, 212)])
    cross = np.zeros((H, W), bool)
    line_pts = [((229, 182), (198, 212)), ((178, 197), (249, 197))]
    for (p0, p1) in line_pts:
        n = 40
        for k in range(n + 1):
            x_, y_ = p0[0] + (p1[0] - p0[0]) * k / n, p0[1] + (p1[1] - p0[1]) * k / n
            cross |= rect(int(x_), int(y_), int(x_), int(y_))
    _blend(img, patch & ~cross & (top_ | (YY >= 172)), '#fff4d0', 0.3)
    # a big pink ball and a little yellow one, in the corner at the right (under the A of the legend)
    _blend(img, ellipse(306, 232, 11, 2.6), '#3a2010', 0.35)
    ball = lobe(img, 306, 221, 11, 11, ['#8a1f55', '#b03070', '#d0457f', '#f06a9e', '#ff9ac0', '#ffd0e2'],
                L=(0.45, -0.6, 0.65))
    outline(img, ball, INK)
    paint(img, ellipse(309, 215, 2.6, 1.8) & ball, '#fff0f6')
    _blend(img, ellipse(287, 236, 4.5, 1.3), '#3a2010', 0.35)
    yb = lobe(img, 287, 232, 4.5, 4.5, ['#c08010', '#e8a820', '#ffc83c', '#ffe27a', '#fff4c0'], L=(0.45, -0.6, 0.65))
    outline(img, yb, INK)
    return img


# ------------------------------------------------------------------ the wizard's laboratory (Forme e colori)
def _potion(img, kind, x, base, liquid, tint='#9a8ad0'):
    """A bottle of the wizard standing on a shelf (base = the shelf's top row): the glass, a
    glowing potion inside, a cork, a glint. liquid = 4 tones, dark to light."""
    if kind == 'round':
        body = ellipse(x + 0.5, base - 7, 7, 7) | rect(x - 1, base - 19, x + 1, base - 12)
        level, cork, gx = base - 9, rect(x - 2, base - 22, x + 2, base - 20), (x - 4, base - 11, base - 6)
    elif kind == 'tall':
        body = _rr(x - 5, base - 17, x + 4, base - 1, 2) | rect(x - 2, base - 22, x + 1, base - 17)
        level, cork, gx = base - 12, rect(x - 2, base - 25, x + 1, base - 23), (x - 3, base - 15, base - 4)
    elif kind == 'cone':
        body = poly_mask([(x - 7, base - 1), (x + 6, base - 1), (x + 1.5, base - 11), (x + 1.5, base - 18),
                          (x - 2.5, base - 18), (x - 2.5, base - 11)])
        level, cork, gx = base - 7, rect(x - 2, base - 21, x + 1, base - 19), (x - 4, base - 6, base - 3)
    else:   # a vial
        body = _rr(x - 3, base - 13, x + 2, base - 1, 1)
        level, cork, gx = base - 10, rect(x - 3, base - 16, x + 2, base - 14), (x - 2, base - 12, base - 3)
    glow(img, x, base - 8, 15, 13, liquid[2], 0.3)
    outline(img, body | cork, INK)
    _blend(img, body, tint, 0.45)   # the empty glass: the wall seen through it
    pot = body & (YY >= level)
    tone_fill(img, pot, 0.95 - (XX - x + 7) / 14.0 * 0.55 - (YY - level) / 14.0 * 0.25, liquid)
    paint(img, body & (YY == level), liquid[3])
    paint(img, rect(gx[0], gx[1], gx[0], gx[2]) & body, '#ffffff')
    tone_fill(img, cork, 0.8 - (XX - x + 2) / 4.0 * 0.6, ['#6a4422', '#9a6a3a', '#c8945a'])
    return body


def laboratorio():
    """The wizard's laboratory: the same room (the round window and the shelves of potions on the
    left, the parchment board where the wizard stands), now lit by the potions, the moon and a
    candle; the board in a real wooden frame with brass corners."""
    from objects import star_mask_px
    img = canvas()
    wall = YY < 190
    light = 0.22 * np.clip(1 - np.sqrt(((XX - 38) / 80.0) ** 2 + ((YY - 22) / 70.0) ** 2), 0, 1)
    for gy in (60, 112):
        light = light + 0.26 * np.clip(1 - np.sqrt(((XX - 38) / 56.0) ** 2 + ((YY - gy) / 34.0) ** 2), 0, 1)
    light = light + 0.14 * np.clip(1 - np.sqrt(((XX - 191) / 170.0) ** 2 + ((YY - 97) / 120.0) ** 2), 0, 1)
    _blocks(img, wall, 0, 190, 14, 36, ['#1a1030', '#22163e', '#2c1d4e', '#372660', '#433072'], '#140c26', 3,
            light=light - 0.06)
    # the round window: stone voussoirs, the night, the crescent moon
    cx, cy = 38.5, 22.5
    gl = ellipse(cx, cy, 17, 17)
    stones = ellipse(cx, cy, 23, 23) & ~gl
    sky = canvas()
    gradient(sky, 5, 40, ['#0a0826', '#130f3a', '#1e1852', '#2a2068'], 20, 58)
    img[gl, :3] = sky[gl, :3]
    rng = np.random.default_rng(12)
    for _ in range(14):
        x, y = int(rng.integers(22, 56)), int(rng.integers(6, 40))
        if gl[y, x]:
            img[y, x, :3] = rgb('#fff3a6' if rng.random() < 0.3 else '#ffffff')
    glow(img, 45, 17, 14, 14, '#fff4c2', 0.32, mask=gl)
    cm = ellipse(45, 17, 7, 7) & ~ellipse(48.5, 13.5, 6, 6)
    paint(img, cm, '#fbeaa0')
    paint(img, cm & (XX < 42), '#fff6cf')
    paint(img, (rect(38, 5, 38, 40) | rect(21, 22, 56, 22)) & gl, '#2e2448')
    paint(img, gl & ~ellipse(cx + 1.2, cy + 1.2, 16, 16), '#0c081c')           # the depth of the wall
    ang = np.arctan2(YY + 0.5 - cy, XX + 0.5 - cx)
    seg = (ang + np.pi) / (2 * np.pi / 12)
    tone_fill(img, stones, 0.5 + ((np.floor(seg) % 2) - 0.5) * 0.25 - (YY - cy) / 23.0 * 0.25 - (XX - cx) / 23.0 * 0.1,
              ['#2c2244', '#3a2e58', '#4a3c6c', '#5c4e80', '#6e6094'])
    r_ = np.sqrt((XX + 0.5 - cx) ** 2 + (YY + 0.5 - cy) ** 2)
    paint(img, stones & (np.abs(seg - np.round(seg)) * r_ * 2 * np.pi / 12 < 0.6), '#160e28')
    outline(img, ellipse(cx, cy, 23, 23), INK)
    paint(img, dilate4(gl) & ~gl, INK)
    # a bunch of lavender drying upside down, between the window and the board
    paint(img, rect(68, 0, 68, 6), '#8a7ab0')
    for (ex, ey) in ((63, 21), (65.5, 23), (68, 24), (70.5, 23), (73, 21)):
        line(img, [(68, 9), (ex, ey - 5)], '#3a7a3a', 0.4)
        for k in range(4):
            fy = ey - 5 + k * 1.6
            paint(img, rect(int(ex + (ex - 68) * k * 0.08), int(fy), int(ex + (ex - 68) * k * 0.08), int(fy)),
                  '#b47aee' if k % 2 else '#7a3ac0')
    paint(img, rect(66, 7, 70, 9), '#c0306a')
    paint(img, rect(66, 7, 70, 7), '#ff86b4')
    # the shelves: planks on brackets, the shadow under them
    for sy in (72, 124):
        _blend(img, rect(2, sy + 4, 74, sy + 9) & wall, '#08040f', 0.3)
        plank = rect(2, sy, 74, sy + 3)
        br = poly_mask([(9, sy + 4), (15, sy + 4), (9, sy + 11)]) | poly_mask([(61, sy + 4), (67, sy + 4), (67, sy + 11)])
        outline(img, plank | br, INK)
        tone_fill(img, plank, 0.95 - (YY - sy) / 3.0 * 0.7, WOOD)
        tone_fill(img, br, 0.35, WOOD)
    _potion(img, 'round', 13, 72, ['#1e6a2a', '#3aa04a', '#7ad86a', '#d4ffb0'])
    _potion(img, 'tall', 30, 72, ['#8a1f55', '#d0457f', '#ff86b4', '#ffd0e2'])
    _potion(img, 'cone', 47, 72, ['#1d6f73', '#2fb3a8', '#7ff0dc', '#d8fff4'])
    _potion(img, 'vial', 63, 72, ['#a06a10', '#e8a820', '#ffd860', '#fff4c0'])
    # the lower shelf: two old books, a round flask, a vial, a candle
    for (x0, y0, x1, y1, c) in ((3, 117, 22, 123, ['#5a1420', '#8a2030', '#b83a46']),
                                (5, 111, 20, 116, ['#1a2a6a', '#2a4aa0', '#4a6ac8'])):
        b = rect(x0, y0, x1, y1)
        outline(img, b, INK)
        tone_fill(img, b, 0.9 - (YY - y0) / float(y1 - y0) * 0.7, c)
        paint(img, rect(x1 - 1, y0 + 1, x1 - 1, y1 - 1), '#f4e4c0')
        paint(img, rect(x0 + 3, y0, x0 + 4, y1), '#ffd23f')
    _potion(img, 'round', 33, 124, ['#4a1a8a', '#7a3ac0', '#b47aee', '#ecd8ff'])
    _potion(img, 'vial', 47, 124, ['#1a5a8a', '#2a8ad0', '#6ac0f4', '#d0f0ff'])
    glow(img, 64, 106, 22, 22, '#ffd88a', 0.35)
    dish = ellipse(64, 123, 6, 1.6)
    candle = rect(62, 110, 66, 122)
    outline(img, dish | candle, INK)
    tone_fill(img, candle, 0.9 - (XX - 62) / 4.0 * 0.6, ['#c8b48a', '#ecdcb4', '#fff6dc'])
    paint(img, rect(63, 110, 63, 113), '#fffbea')
    paint(img, dish, '#c88a10')
    paint(img, rect(64, 108, 64, 109), INK)
    fl = ellipse(64.5, 105, 2.2, 3.6) | poly_mask([(62.5, 104), (66.5, 104), (64.5, 98)])
    tone_fill(img, fl, 0.95 - (YY - 98) / 10.0 * 0.6, ['#e86a1a', '#ffb43c', '#ffe27a', '#fffbe0'])
    # the board: a wooden frame with brass corners, the parchment (lighter in the middle, aged at the edges)
    x0, y0, x1, y1 = 80, 28, 302, 166
    fr = _rr(x0 - 4, y0 - 4, x1 + 4, y1 + 4, 8)
    _blend(img, np.roll(np.roll(fr, 4, 0), 3, 1) & ~fr & wall, '#08040f', 0.4)
    outline(img, fr, INK)
    grain = np.sin(YY * 0.9 + np.sin(XX * 0.05) * 3.0) * 0.08
    tone_fill(img, fr, 0.62 - (XX - x0) / 230.0 * 0.15 - (YY - y0) / 140.0 * 0.2 + grain, WOOD)
    paint(img, fr & ~_rr(x0 - 3, y0 - 3, x1 + 3, y1 + 3, 7) & (YY < 97), WOOD[4])
    pm = _rr(x0, y0, x1, y1, 6)
    paint(img, dilate4(pm) & ~pm, '#4a2a14')
    dd = np.maximum(np.abs(XX - 191) / 111.0, np.abs(YY - 97) / 69.0)
    tone_fill(img, pm, 0.92 - np.clip((dd - 0.72) / 0.28, 0, 1) * 0.55, ['#dcbc84', '#e8cc98', '#f2dcb0', '#fbecc8',
                                                                         '#fdf1d6', '#fef5e0'])
    speck = pm & (((XX * 7 + YY * 13) % 31) == 0)
    paint(img, speck, '#efd8aa')
    for (sx_, sy_, sr) in ((94, 150, 7), (288, 42, 5)):   # old rings of tea on the parchment
        ring_ = ellipse(sx_, sy_, sr, sr * 0.8) & ~ellipse(sx_, sy_, sr - 1, sr * 0.8 - 1)
        _blend(img, ring_ & pm, '#c8a066', 0.3)
    for (bx, by, sx, sy) in ((x0 - 4, y0 - 4, 1, 1), (x1 + 4, y0 - 4, -1, 1), (x0 - 4, y1 + 4, 1, -1),
                             (x1 + 4, y1 + 4, -1, -1)):
        c = poly_mask([(bx, by), (bx + sx * 13, by), (bx + sx * 13, by + sy * 4), (bx + sx * 4, by + sy * 4),
                       (bx + sx * 4, by + sy * 13), (bx, by + sy * 13)])
        outline(img, c, INK)
        tone_fill(img, c, 0.75 - (YY - by) * sy / 13.0 * 0.4, ['#a06a10', '#c88a10', '#ffd23f', '#fff080'])
        paint(img, rect(bx + sx * 2, by + sy * 2, bx + sx * 2, by + sy * 2), '#7a4a10')
    # a star hanging at the right of the board
    paint(img, rect(312, 0, 312, 28), '#8a7ab0')
    glow(img, 312.5, 34, 14, 14, '#ffe27a', 0.3)
    st = star_mask_px(W, H, 312.5, 34.5, 6, 2.6)
    outline(img, st, INK)
    tone_fill(img, st, 0.85 - (YY - 28) / 13.0 * 0.6, ['#c88a10', '#ffd23f', '#fff080', '#fffbd0'])
    # the stone floor, its tiles going away; the glow of the shelves on it
    floor = YY >= 190
    v = np.clip((YY - 190) / 50.0, 0, 1)
    xc = (XX - 160) / (0.5 + v * 0.9)
    tile = ((np.floor(xc / 24.0) + np.floor((YY - 190) / (4 + v * 10))) % 2).astype(int)
    tone_fill(img, floor, 0.2 + tile * 0.3 + v * 0.3, ['#231a38', '#2e2448', '#3a2f58', '#463b68', '#544878'])
    paint(img, floor & (np.abs((XX - 160) % (24 * (0.5 + v * 0.9))) < 0.9), '#1a1228')
    paint(img, rect(0, 190, W - 1, 190), '#120a20')
    vfade(img, rect(0, 191, W - 1, 196), '#08040f', 0.4, 0.0, 191, 196)
    glow(img, 40, 194, 70, 12, '#b47aee', 0.22, mask=floor)
    vfade(img, YY < 24, '#08040f', 0.3, 0.0, 0, 24)
    return img


# ------------------------------------------------------------------ the room of the measures (Misure)
def _giraffe(img, x0, y0, y1):
    """The growth chart: a tall friendly giraffe on the wall, its neck marked like a ruler."""
    neck = rect(x0 + 4, y0 + 18, x0 + 14, y1)
    head = ellipse(x0 + 12, y0 + 12, 10, 8)
    muzzle = ellipse(x0 + 17, y0 + 15, 5, 4)
    ears = ellipse(x0 + 3, y0 + 6, 3.4, 2) | ellipse(x0 + 21, y0 + 6, 3.4, 2)
    horns = rect(x0 + 8, y0 + 0, x0 + 9, y0 + 5) | rect(x0 + 15, y0 + 0, x0 + 16, y0 + 5)
    knobs = ellipse(x0 + 9, y0 + 0, 1.8, 1.8) | ellipse(x0 + 16, y0 + 0, 1.8, 1.8)
    m = neck | head | muzzle | ears | horns | knobs
    _shadow(img, m, -3, 2, '#5a8a7a', 0.25)
    outline(img, m, INK)
    yel = ['#c88a10', '#e0a820', '#f4c430', '#ffd84a', '#ffe88a']
    tone_fill(img, neck, 0.35 + (XX - x0 - 4) / 10.0 * 0.55, yel)
    lobe(img, x0 + 12, y0 + 12, 10, 8, yel, L=(0.5, -0.5, 0.7), clip=head | ears)
    tone_fill(img, ears, 0.5, yel)
    lobe(img, x0 + 17, y0 + 15, 5, 4, ['#e8c890', '#f6dcaa', '#fff0c8'], L=(0.4, -0.6, 0.7))
    tone_fill(img, horns, 0.4, yel)
    paint(img, knobs, '#a8602a')
    paint(img, knobs & (XX % 2 == 0) & (YY <= y0), '#d08a4a')
    for (cx, cy, r) in ((x0 + 7, y0 + 34, 2.4), (x0 + 11, y0 + 52, 2.8), (x0 + 7, y0 + 74, 2.2), (x0 + 11, y0 + 96, 2.6),
                        (x0 + 7, y0 + 118, 2.4), (x0 + 11, y0 + 140, 2.8), (x0 + 7, y0 + 160, 2.2), (x0 + 6, y0 + 10, 2)):
        sp = ellipse(cx, cy, r, r * 1.1) & m
        paint(img, sp, '#d88a2a')
        paint(img, sp & ~ellipse(cx - 0.6, cy - 0.6, r - 0.6, r * 1.1 - 0.6), '#b86a1a')
    for y in range(y0 + 24, y1, 6):   # the marks of the ruler: short, and long every 24
        long_ = (y - y0) % 24 == 0
        paint(img, rect(x0 + (9 if long_ else 12), y, x0 + 14, y), '#8a5a30')
    paint(img, rect(x0 + 13, y0 + 9, x0 + 14, y0 + 10), INK)     # the eye, a glint
    paint(img, rect(x0 + 13, y0 + 9, x0 + 13, y0 + 9), '#ffffff')
    paint(img, rect(x0 + 18, y0 + 18, x0 + 21, y0 + 18), INK)     # the smile
    paint(img, rect(x0 + 21, y0 + 17, x0 + 21, y0 + 17), INK)
    paint(img, rect(x0 + 20, y0 + 13, x0 + 20, y0 + 13), '#8a5a30')
    paint(img, ellipse(x0 + 10, y0 + 16, 1.6, 1), '#ff93c6')


def misure():
    """The room of the measures: the same playroom (the giraffe chart on the left, the window
    and the shelf of toys on the right, the round rug), lit by the sun from the window."""
    import games5
    img = canvas()
    wall = YY < 196
    d = np.sqrt(((XX - 300) / 230.0) ** 2 + ((YY - 48) / 170.0) ** 2)
    lit = np.clip(0.42 - d * 0.42, 0, 0.42)
    tone_fill(img, wall, 0.42 + lit - np.clip((34 - YY) / 34.0, 0, 1) * 0.18,
              ['#b0dccc', '#bee6d6', '#cbede0', '#d7f3ea', '#e2f7f0', '#eefbf7'])
    dots = (((XX // 12) + (YY // 12)) % 2 == 0) & (XX % 12 == 6) & (YY % 12 == 6) & (YY < 160)
    _blend(img, dots | (dots & False), '#ffffff', 0.45)
    _blend(img, np.roll(dots, 1, 1) | np.roll(dots, 1, 0), '#ffffff', 0.25)
    # the wainscot: raised pink panels under a white rail
    rail = rect(0, 162, W - 1, 167)
    tone_fill(img, rail, 0.95 - (YY - 162) / 5.0 * 0.6 + lit * 0.3, ['#c8a8bc', '#e6d0dc', '#f8eef4', '#ffffff'])
    paint(img, rect(0, 168, W - 1, 168), '#d08aa8')
    tone_fill(img, rect(0, 169, W - 1, 191), 0.45 + lit * 0.5, ['#f0a8c6', '#f8bcd4', '#ffcce0', '#ffdcea', '#ffeaf2'])
    for x in range(0, W, 40):
        p = rect(x + 5, 173, x + 35, 187)
        tone_fill(img, p, 0.6 + lit * 0.5, ['#f0a8c6', '#f8bcd4', '#ffcce0', '#ffdcea', '#ffeaf2'])
        paint(img, rect(x + 4, 172, x + 35, 172) | rect(x + 4, 172, x + 4, 187), '#e090b4')    # in shadow
        paint(img, rect(x + 5, 188, x + 36, 188) | rect(x + 36, 173, x + 36, 188), '#fff4f8')  # in the light
    tone_fill(img, rect(0, 191, W - 1, 195), 0.8 - (YY - 191) / 4.0 * 0.6, ['#c8a8bc', '#e6d0dc', '#f8eef4', '#ffffff'])
    paint(img, rect(0, 190, W - 1, 190), '#d08aa8')
    # clouds painted on the wall, very soft
    for lobes in ([(96, 40, 9, 5), (106, 36, 10, 7), (117, 41, 8, 5)], [(212, 122, 8, 4), (221, 119, 9, 6), (231, 123, 7, 4)],
                  [(60, 134, 7, 4), (68, 131, 8, 5), (77, 135, 6, 3.5)]):
        m = np.zeros((H, W), bool)
        for (x, y, rx_, ry_) in lobes:
            m |= ellipse(x, y, rx_, ry_)
        _blend(img, m, '#ffffff', 0.4)
        _blend(img, m & ~np.roll(m, -2, 0), '#ffffff', 0.3)
    # a drawing made with crayons, taped to the wall: a house and a sun
    pic = rect(46, 52, 72, 74)
    _shadow(img, pic, -2, 2, '#5a8a7a', 0.3)
    paint(img, pic, '#fffdf6')
    paint(img, rect(46, 74, 72, 74) | rect(72, 52, 72, 74), '#e8e0d0')
    house = rect(52, 64, 62, 72)
    roof = poly_mask([(50, 64), (64, 64), (57, 57)])
    paint(img, house, '#ffb4d8')
    paint(img, roof, '#e83a4a')
    paint(img, rect(56, 67, 58, 72), '#8a5a30')
    paint(img, ellipse(67, 57, 2.6, 2.6), '#ffd23f')
    for (dx, dy) in ((4, 0), (-4, 0), (0, 4), (0, -4), (3, 3), (-3, 3), (3, -3), (-3, -3)):
        paint(img, rect(67 + dx, 57 + dy, 67 + dx, 57 + dy), '#ffc23c')
    paint(img, rect(47, 73, 71, 73), '#4cc25a')
    for (tx, ty) in ((44, 50), (68, 50)):   # two bits of tape
        paint(img, rect(tx, ty, tx + 5, ty + 2), '#fff0a8')
        _blend(img, rect(tx, ty, tx + 5, ty + 2), '#ffffff', 0.3)
    # the growth chart
    _giraffe(img, 12, 24, 190)
    # the window: a white frame, the sky with a sun and clouds, a hill; a curtain gathered at the left
    frame = rect(280, 18, 319, 78)
    glass = rect(286, 24, 314, 72)
    sill = rect(276, 78, 319, 82)
    tone_fill(img, frame, 0.85 - (YY - 18) / 60.0 * 0.4, ['#a8c4bc', '#cfe2dc', '#eef6f3', '#ffffff'])
    sky = canvas()
    gradient(sky, 24, 73, ['#58b0f4', '#7cc4fa', '#a4d8ff', '#c8eaff'], 286, 315)
    img[glass, :3] = sky[glass, :3]
    glow(img, 310, 30, 14, 14, '#fff6c0', 0.5, mask=glass)
    lobe(img, 310, 30, 4.5, 4.5, ['#ffc23c', '#ffd860', '#ffec9a', '#fffbe0'], L=(-0.3, -0.4, 0.8), clip=glass)
    hill = glass & (YY > 64 + 3 * np.sin((XX - 280) / 7.0))
    tone_fill(img, hill, 0.8 - (YY - 62) / 10.0 * 0.6, ['#3a9a4a', '#52b45a', '#6ccc6a', '#8ee080'])
    cloud(img, [(293, 44, 5, 3.5), (300, 41, 6, 4.5), (307, 45, 4, 3)], ['#c4dcf4', '#e2eefc', '#ffffff'])
    _blend(img, glass & ((((XX + YY) % 30) < 2)), '#ffffff', 0.3)
    paint(img, (rect(300, 24, 301, 72) | rect(286, 48, 314, 49)), '#eef6f3')
    paint(img, (rect(302, 24, 302, 72) | rect(286, 50, 314, 50)) & glass, '#a8c4bc')
    paint(img, rect(284, 22, 316, 74) & ~glass & ~(rect(300, 24, 301, 72) | rect(286, 48, 314, 49)), '#9ab8b0')
    outline(img, frame | sill, INK)
    tone_fill(img, sill, 0.95 - (YY - 78) / 4.0 * 0.7, ['#a8c4bc', '#cfe2dc', '#eef6f3', '#ffffff'])
    rod = rect(264, 12, 319, 13)
    paint(img, rod, '#c88a10')
    paint(img, rect(264, 12, 319, 12), '#ffd23f')
    fin = ellipse(263, 12.5, 2.4, 2.4)
    outline(img, fin | rod, INK)
    paint(img, fin, '#ffd23f')
    cur = poly_mask([(266, 14), (284, 14), (284, 22), (279, 56), (283, 92), (268, 92), (270, 56), (266, 22)])
    _shadow(img, cur, -2, 2, '#5a8a7a', 0.25, under=wall & ~frame)
    outline(img, cur, INK)
    fold = np.cos((XX - 266) / 18.0 * np.pi * 3.0) * 0.25
    tone_fill(img, cur, 0.55 + fold + (XX - 266) / 18.0 * 0.15, ['#d0457f', '#f06a9e', '#ff9ac0', '#ffc4dc', '#ffe4ef'])
    tie = rect(268, 55, 282, 58)
    outline(img, tie, INK)
    paint(img, tie, '#ffd23f')
    paint(img, rect(268, 55, 282, 55), '#fff080')
    # the sunbeam, faint on the wall, brighter where it lands on the floor
    beam_pts = [(286, 24), (314, 24), (314, 72), (270, 214), (230, 226), (204, 226), (286, 72)]
    _hull_beam(img, beam_pts, wall & ~frame & ~cur, '#fffbe8', 0.16, 0.06, 24, 196)
    # the shelf of toys under the window
    sh = rect(276, 120, 319, 123)
    br = poly_mask([(282, 124), (288, 124), (282, 131)])
    _blend(img, rect(276, 124, 319, 128) & wall, '#3a6a5a', 0.25)
    outline(img, sh | br, INK)
    tone_fill(img, sh, 0.95 - (YY - 120) / 3.0 * 0.7, WOOD)
    tone_fill(img, br, 0.4, WOOD)
    games5._shelf_toys(img, 282, 318, 120, np.random.default_rng(31))
    # the floor: warm boards going away, the shadow along the wall
    _planks(img, 196, 170, 12, 28, ['#8a5a34', '#9e6a40', '#b47c4c', '#c69058', '#d6a46a', '#e4b87e'], '#7a4a28', 9)
    paint(img, rect(0, 196, W - 1, 196), '#5a3418')
    vfade(img, rect(0, 197, W - 1, 202), '#4a2a14', 0.35, 0.0, 197, 202)
    # the round rug: rings of colour, lit at the back, a thickness at the front
    rug = ellipse(170, 222, 130, 15)
    _blend(img, ellipse(170, 225, 131, 15) & ~rug, '#3a2010', 0.4)
    outline(img, rug, INK)
    rings = [('#7d3fc4', '#9a5ad8', '#b376ec', '#cba0f6'), ('#d8a020', '#f0bc30', '#ffd23f', '#ffe27a'),
             ('#1e9a8e', '#36bcae', '#4fd6c0', '#8aeadc'), ('#d0457f', '#f06a9e', '#ff93c6', '#ffc4dc'),
             ('#7d3fc4', '#9a5ad8', '#b376ec', '#cba0f6')]
    for k, c in enumerate(rings):
        m = ellipse(170, 222, 130 - k * 12, 15 - k * 1.9)
        tone_fill(img, m, 0.75 - (YY - 207) / 30.0 * 0.6, list(c))
    for k, c in enumerate(rings[:-1]):   # a stitched line along every ring
        m = ellipse(170, 222, 124 - k * 12, 14 - k * 1.9)
        paint(img, (dilate4(m) & ~m) & ((XX // 3) % 2 == 0), c[3])
    paint(img, rug & (YY == 237) | (ellipse(170, 223, 130, 15) & ~rug & (YY > 222)), '#5a2a8a')
    # the sun on the floor and on the rug
    patch = poly_mask([(236, 204), (268, 204), (246, 228), (208, 228)])
    cross = np.zeros((H, W), bool)
    for (p0, p1) in (((252, 204), (227, 228)), ((222, 216), (257, 216))):
        for k in range(41):
            x_, y_ = p0[0] + (p1[0] - p0[0]) * k / 40.0, p0[1] + (p1[1] - p0[1]) * k / 40.0
            cross |= rect(int(x_), int(y_), int(x_), int(y_))
    _blend(img, patch & ~cross, '#fff4d0', 0.3)
    # a little plant in a pot in the corner
    pot = poly_mask([(301, 184), (317, 184), (315, 197), (303, 197)])
    leaves = np.zeros((H, W), bool)
    for (lx, ly, rx_, ry_) in ((304, 176, 3, 7), (309, 172, 3, 9), (314, 176, 3, 7), (300, 180, 4, 3), (318, 180, 4, 3)):
        leaves |= ellipse(lx, ly, rx_, ry_)
    _blend(img, ellipse(309, 198, 9, 2), '#3a2010', 0.35)
    outline(img, pot | leaves, INK)
    for (lx, ly, rx_, ry_) in ((300, 180, 4, 3), (318, 180, 4, 3), (304, 176, 3, 7), (314, 176, 3, 7), (309, 172, 3, 9)):
        lobe(img, lx, ly, rx_, ry_, ['#1e6a3a', '#2e8a4a', '#46a85a', '#6ac870'], L=(0.5, -0.6, 0.6))
    tone_fill(img, pot, 0.8 - (XX - 301) / 16.0 * 0.6, ['#a0482a', '#c8603a', '#e07a4a', '#f4a070'])
    paint(img, rect(300, 184, 318, 186), '#e07a4a')
    paint(img, rect(300, 184, 318, 184), '#f4a070')
    return img


# ------------------------------------------------------------------ the make-up salon (Trucca i mostri)
GOLD = ['#9a5a0a', '#c08010', '#e0a420', '#f8c838', '#ffe070', '#fff4b8']


def _bulb(img, bx, by, avoid):
    glow(img, bx + 0.5, by + 0.5, 10, 10, '#fff4c8', 0.35, mask=~avoid)
    b = ellipse(bx + 0.5, by + 0.5, 3.4, 3.4)
    outline(img, b, '#b87010')
    lobe(img, bx + 0.5, by + 0.5, 3.4, 3.4, ['#ffd860', '#ffec9a', '#fff8d8', '#ffffff'], L=(-0.5, -0.6, 0.6))


def trucco():
    """The make-up salon: the same big mirror with its bulbs (the quiz panel) over the vanity
    table, the striped wallpaper warmed by the bulbs, a gold frame with a bevel."""
    from backgrounds import PANEL
    img = canvas()
    x0, y0, x1, y1 = PANEL
    wall = YY < 178
    stripe = (XX // 12) % 2 == 0
    d = np.sqrt(((XX - 191) / 170.0) ** 2 + ((YY - 97) / 120.0) ** 2)
    lit = np.clip(1.0 - d, 0, 1) * 0.3
    t = 0.42 + lit - np.clip((30 - YY) / 30.0, 0, 1) * 0.2
    tone_fill(img, wall & stripe, t, ['#f0b0cc', '#f6c2d8', '#fbd2e4', '#ffdeee', '#ffe8f3'])
    tone_fill(img, wall & ~stripe, t, ['#f6dce8', '#fae6ef', '#fdf0f6', '#fff6fa', '#fffbfd'])
    for y in range(10, 170, 22):   # little stars on the wallpaper
        for x in range(8, W, 24):
            sx = x + (12 if (y // 22) % 2 else 0)
            _blend(img, (rect(sx, y + 1, sx + 2, y + 1) | rect(sx + 1, y, sx + 1, y + 2)) & wall, '#ff8ab8', 0.35)
    tone_fill(img, rect(0, 0, W - 1, 5), 0.9 - YY / 5.0 * 0.6, ['#e6a8c4', '#f6c8dc', '#ffe4ef', '#ffffff'])
    paint(img, rect(0, 6, W - 1, 6), '#d890b4')
    vfade(img, rect(0, 7, W - 1, 12), '#d890b4', 0.3, 0.0, 7, 12)
    # a garland of little hearts at the top left
    hpts = [(x, 9 + 6 * np.sin(np.pi * x / 74.0)) for x in range(0, 76, 2)]
    line(img, hpts, '#b8608a', 0.4)
    for i, hx in enumerate((12, 30, 48, 64)):
        hy = 9 + 6 * np.sin(np.pi * hx / 74.0) + 4
        hm = ellipse(hx - 2, hy - 1, 2.6, 2.4) | ellipse(hx + 2, hy - 1, 2.6, 2.4) | poly_mask([(hx - 4.5, hy), (hx + 4.5, hy), (hx, hy + 5)])
        outline(img, hm, '#8a2a5a')
        tone_fill(img, hm, 0.85 - (YY - hy + 3) / 8.0 * 0.6, (['#d0457f', '#f06a9e', '#ffa8c8'], ['#c08010', '#f8c838', '#ffe070'],
                                                           ['#7a3ac0', '#a86ee8', '#d0b0ff'], ['#1e9a8e', '#36bcae', '#8aeadc'])[i])
    # the mirror: its warm light and shadow on the wall, the bevelled gold frame, the glass
    frame = _rr(x0 - 5, y0 - 5, x1 + 5, y1 + 5, 12)
    glass = _rr(x0, y0, x1, y1, 8)
    glow(img, 191, 97, 170, 115, '#fff0c0', 0.3, mask=wall & ~frame)
    _blend(img, np.roll(np.roll(frame, 4, 0), 3, 1) & ~frame & wall, '#b05a84', 0.3)
    outline(img, frame, INK)
    outer = frame & ~_rr(x0 - 3, y0 - 3, x1 + 3, y1 + 3, 10)
    inner = frame & ~outer & ~glass
    tone_fill(img, outer, 0.78 - (XX - 191) / 111.0 * 0.2 - (YY - 97) / 69.0 * 0.35, GOLD)
    tone_fill(img, inner, 0.45 + (XX - 191) / 111.0 * 0.15 + (YY - 97) / 69.0 * 0.3, GOLD)
    tone_fill(img, glass, 0.85 - (YY - y0) / float(y1 - y0) * 0.55 + lit * 0.3, ['#bcd4f2', '#cadff8', '#d8e8fc', '#e6f1fe', '#f2f8ff'])
    refl = glass & (YY > y1 - 26) & stripe
    _blend(img, refl, '#ffc8de', 0.22)
    shine = glass & ((((XX - YY) % 56) < 5) | (((XX - YY) % 56) == 8))
    _blend(img, shine, '#ffffff', 0.5)
    paint(img, dilate4(glass) & ~glass & frame, '#8a4a08')
    bulbs = [(x, y0 - 3) for x in range(x0 + 10, x1 - 4, 22)] + \
            [(x0 - 3, y) for y in range(y0 + 14, y1 - 4, 22)] + [(x1 + 3, y) for y in range(y0 + 14, y1 - 4, 22)]
    for (bx, by) in bulbs:
        _bulb(img, bx, by, glass)
    # the vanity table: a lit top, its edge, drawers with gold knobs
    paint(img, rect(0, 177, W - 1, 177), INK)
    tone_fill(img, rect(0, 178, W - 1, 184), 0.95 - (YY - 178) / 6.0 * 0.55, ['#e6a877', '#f6c89f', '#ffe0c4', '#fff0e0'])
    _blend(img, rect(x0, 178, x1, 184), '#ffffff', 0.18)                      # the mirror shines on it
    paint(img, rect(0, 185, W - 1, 185), '#c98a5c')
    tone_fill(img, rect(0, 186, W - 1, H - 1), 0.6 - (YY - 186) / 54.0 * 0.35, ['#c88050', '#d8925e', '#e8a670', '#f4ba86'])
    for (dx0, dx1) in ((6, 150), (170, 314)):
        dr = rect(dx0, 192, dx1, 228)
        paint(img, dilate4(dr) & ~dr, '#a8683c')
        tone_fill(img, dr, 0.75 - (YY - 192) / 36.0 * 0.4, ['#d8925e', '#e8a670', '#f4ba86', '#fccc9c'])
        paint(img, rect(dx0, 192, dx1, 192), '#ffd8b4')
        kx = (dx0 + dx1) // 2
        kb = ellipse(kx + 0.5, 210, 3.2, 3.2)
        outline(img, kb, '#7a4a10')
        lobe(img, kx + 0.5, 210, 3.2, 3.2, GOLD[1:], L=(-0.5, -0.6, 0.6))
    # a pot of brushes and a perfume on the left of the table
    _blend(img, ellipse(38, 178, 26, 2), '#8a4a28', 0.3)
    for i, (bx, col) in enumerate(((47, ['#c0306a', '#ff86b4', '#ffd0e2']), (51, ['#1e9a8e', '#4fd6c0', '#c8fff4']),
                                    (55, ['#c08010', '#ffd23f', '#fff4c0']))):
        stick = rect(bx, 146 + i * 3, bx + 1, 162)
        paint(img, stick, '#8a5a30')
        paint(img, rect(bx, 146 + i * 3, bx, 162), '#c48a50')
        tip = ellipse(bx + 1, 145 + i * 3, 2, 3)
        outline(img, tip, INK)
        tone_fill(img, tip, 0.9 - (YY - 142 - i * 3) / 6.0 * 0.7, col)
    pot = _rr(44, 160, 58, 177, 2)
    outline(img, pot, INK)
    tone_fill(img, pot, 0.85 - (XX - 44) / 14.0 * 0.6, ['#5a2a9a', '#7d3fc4', '#b376ec', '#d8b8ff'])
    paint(img, rect(44, 163, 58, 164), '#ffd23f')
    bottle = _rr(18, 158, 32, 177, 3)
    cap = rect(22, 151, 28, 157)
    outline(img, bottle | cap, INK)
    tone_fill(img, bottle, 0.9 - (XX - 18) / 14.0 * 0.6 - (YY - 158) / 19.0 * 0.2, ['#3a7ac8', '#6aa8ec', '#a8d4ff', '#e4f4ff'])
    paint(img, rect(20, 161, 20, 172), '#ffffff')
    tone_fill(img, cap, 0.85 - (XX - 22) / 6.0 * 0.7, GOLD[1:])
    return img

