"""Fonts, cards, icons and logo."""
import numpy as np
from pixel import (blank, blit, ascii_sprite, add_outline, rect_mask, ellipse_mask,
                   shaded_part, rgb, scale, crop, dilate4)

BIG = {
    '0': ["...######...", "..########..", ".###....###.", "###......###", "###......###",
          "###......###", "###......###", "###......###", "###......###", "###......###",
          "###......###", "###......###", "###......###", "###......###", "###......###",
          ".###....###.", "..########..", "...######..."],
    '1': ["....####....", "...#####....", "..######....", ".###.###....", ".....###....",
          ".....###....", ".....###....", ".....###....", ".....###....", ".....###....",
          ".....###....", ".....###....", ".....###....", ".....###....", ".....###....",
          ".....###....", "..#########.", "..#########."],
    '2': ["..#######...", ".#########..", "###.....###.", "##.......###", ".........###",
          ".........###", "........###.", ".......###..", "......###...", ".....###....",
          "....###.....", "...###......", "..###.......", ".###........", "###.........",
          "###.........", "############", "############"],
    '3': [".#########..", "###########.", ".........###", ".........###", ".........###",
          "........###.", "...#######..", "...#######..", "........###.", ".........###",
          ".........###", ".........###", ".........###", ".........###", "##.......###",
          "###.....###.", ".#########..", "..#######..."],
    '4': ["......####..", ".....#####..", "....######..", "...###.###..", "..###..###..",
          ".###...###..", "###....###..", "###....###..", "###....###..", "############",
          "############", ".......###..", ".......###..", ".......###..", ".......###..",
          ".......###..", ".......###..", ".......###.."],
    '5': ["###########.", "###########.", "###.........", "###.........", "###.........",
          "###.........", "#########...", "##########..", "........###.", ".........###",
          ".........###", ".........###", ".........###", ".........###", "##.......###",
          "###.....###.", ".#########..", "..#######..."],
    '6': ["....######..", "..########..", ".###........", "###.........", "###.........",
          "###.........", "###.######..", "##########..", "####....###.", "###......###",
          "###......###", "###......###", "###......###", "###......###", "###......###",
          ".###....###.", "..########..", "...######..."],
    '7': ["############", "############", ".........###", "........###.", "........###.",
          ".......###..", ".......###..", "......###...", "......###...", ".....###....",
          ".....###....", "....###.....", "....###.....", "....###.....", "...###......",
          "...###......", "...###......", "...###......"],
    '8': ["...######...", "..########..", ".###....###.", ".###....###.", ".###....###.",
          ".###....###.", "..########..", "..########..", ".###....###.", "###......###",
          "###......###", "###......###", "###......###", "###......###", "###......###",
          ".###....###.", "..########..", "...######..."],
    'P': ["#########...", "##########..", "###.....###.", "###......###", "###......###",
          "###......###", "###.....###.", "##########..", "#########...", "###.........",
          "###.........", "###.........", "###.........", "###.........", "###.........",
          "###.........", "###.........", "###........."],
    'I': ["#########", "#########"] + ["...###..."] * 14 + ["#########", "#########"],
    'R': ["#########...", "##########..", "###.....###.", "###......###", "###......###",
          "###......###", "###.....###.", "##########..", "#########...", "###..###....",
          "###...###...", "###...###...", "###....###..", "###....###..", "###.....###.",
          "###.....###.", "###......###", "###......###"],
    'E': ["###########", "###########", "###........", "###........", "###........",
          "###........", "###........", "#########..", "#########..", "###........",
          "###........", "###........", "###........", "###........", "###........",
          "###........", "###########", "###########"],
    'T': ["###########", "###########"] + ["....###...."] * 16,
    'A': ["....####....", "...######...", "...######...", "..###..###..", "..###..###..",
          "..###..###..", ".###....###.", ".###....###.", ".###....###.", ".##########.",
          "############", "###......###", "###......###", "###......###", "###......###",
          "###......###", "###......###", "###......###"],
}
BIG['9'] = [r[::-1] for r in BIG['6'][::-1]]
BIG['O'] = BIG['0']
BIG['B'] = ["#########...", "##########..", "###.....###.", "###......###", "###......###", "###......###",
            "###.....###.", "#########...", "##########..", "###.....###.", "###......###", "###......###",
            "###......###", "###......###", "###......###", "###.....###.", "##########..", "#########..."]
BIG['C'] = ["...#######..", "..#########.", ".###.....###", "###.......##"] + ["###........."] * 10 + \
           ["###.......##", ".###.....###", "..#########.", "...#######.."]
BIG['D'] = ["########....", "#########...", "###....###..", "###.....###."] + ["###......###"] * 10 + \
           ["###.....###.", "###....###..", "#########...", "########...."]
BIG['F'] = ["###########", "###########"] + ["###........"] * 5 + ["#########..", "#########.."] + \
           ["###........"] * 9
BIG['G'] = ["...#######..", "..#########.", ".###.....###", "###.......##", "###.........", "###.........",
            "###.........", "###.........", "###...######", "###...######", "###......###", "###......###",
            "###......###", "###......###", "###......###", ".###....####", "..##########", "...######.##"]
BIG['H'] = ["###......###"] * 8 + ["############"] * 2 + ["###......###"] * 8
BIG['J'] = ["....########", "....########"] + [".........###"] * 12 + \
           ["##.......###", "###.....###.", ".#########..", "..#######..."]
BIG['K'] = ["###......###", "###.....###.", "###....###..", "###...###...", "###..###....", "###.###.....",
            "######......", "#####.......", "#####.......", "######......", "###.###.....", "###..###....",
            "###...###...", "###....###..", "###.....###.", "###......###", "###......###", "###......###"]
BIG['L'] = ["###........"] * 16 + ["###########"] * 2
BIG['M'] = ["###......###", "####....####", "#####..#####", "############", "###.####.###", "###..##..###"] + \
           ["###......###"] * 12
BIG['N'] = ["###......###", "####.....###", "#####....###", "######...###", "###.###..###", "###..###.###",
            "###...######", "###....#####", "###.....####"] + ["###......###"] * 9
BIG['Q'] = ["...######...", "..########..", ".###....###."] + ["###......###"] * 11 + \
           ["###...##.###", ".###...####.", "..#########.", "...######.##"]
BIG['S'] = ["...#######..", "..#########.", ".###.....###", "###.......##", "###.........", "###.........",
            ".###........", "..#######...", "...########.", "........###.", ".........###", ".........###",
            ".........###", ".........###", "##.......###", "###.....###.", ".#########..", "..#######..."]
BIG['U'] = ["###......###"] * 15 + [".###....###.", "..########..", "...######..."]
BIG['V'] = ["###......###"] * 6 + [".###....###."] * 3 + ["..###..###.."] * 3 + ["...######..."] * 3 + \
           ["....####...."] * 2 + [".....##....."]
BIG['W'] = ["###......###"] * 10 + ["###..##..###", "###.####.###", "###.####.###", "############",
                                     "#####..#####", "####....####", "###......###", "##........##"]
_X_TOP = ["###......###", ".###....###.", ".###....###.", "..###..###..", "..###..###..", "...######...",
          "...######...", "....####....", "....####...."]
BIG['X'] = _X_TOP + _X_TOP[::-1]
BIG['Y'] = _X_TOP[:7] + ["....####...."] * 11
BIG['Z'] = ["############"] * 2 + \
           ["".join("#" if round(9 - 9 * (r - 2) / 13) <= c < round(9 - 9 * (r - 2) / 13) + 3 else "."
                    for c in range(12)) for r in range(2, 16)] + ["############"] * 2
BIG["'"] = ["###", "###", ".##", ".#."] + ["..."] * 14
BIG['?'] = ["..#######...", ".#########..", "###.....###.", "##.......###", ".........###", "........###.",
            ".......###..", "......###...", ".....###....", ".....###....", ".....###....", "............",
            "............", ".....###....", ".....###....", ".....###....", "............", "............"]
for _k, _v in BIG.items():
    assert len(_v) == 18, (_k, len(_v))

SMALL = {
    '0': [".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."],
    '1': ["..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."],
    '2': [".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"],
    '3': ["####.", "....#", "....#", ".###.", "....#", "....#", "####."],
    '4': ["...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."],
    '5': ["#####", "#....", "####.", "....#", "....#", "#...#", ".###."],
    '6': [".###.", "#....", "#....", "####.", "#...#", "#...#", ".###."],
    '7': ["#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."],
    '8': [".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."],
    '9': [".###.", "#...#", "#...#", ".####", "....#", "....#", ".###."],
    'A': [".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"],
    'B': ["####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."],
    'Z': ["#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"],
    'X': ["#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"],
    'Y': ["#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."],
}


def glyph(rows, fill, outline='k', hi=None, diag=False):
    img = ascii_sprite([r.replace('#', fill) for r in rows])
    if hi:
        # 1 px highlight on the top-left of every stroke
        m = img[..., 3] > 0
        up = np.zeros_like(m)
        up[1:, :] = m[:-1, :]
        left = np.zeros_like(m)
        left[:, 1:] = m[:, :-1]
        sel = m & (~up | ~left)
        img[sel, :3] = rgb(hi)
    return add_outline(img, outline, diag=diag)


def big_digits():
    out = {}
    for d in '0123456789':
        out['dig_' + d] = glyph(BIG[d], 'V', 'k', hi='v')
        out['digsel_' + d] = glyph(BIG[d], 'h', 'k', hi='P')
        out['digoff_' + d] = glyph(BIG[d], 'S', 'x')
    return out


def small_digits():
    out = {}
    for d in '0123456789':
        out['sdig_' + d] = glyph(SMALL[d], 'w', 'k', diag=True)
    return out


def card(w, h, frame, fill, outline='k', frame_w=2, glow=None):
    img = blank(w, h)
    outer = rect_mask(w, h, 1, 1, w - 2, h - 2, radius=6)
    ring = dilate4(outer) & ~outer
    img[ring, :3] = rgb(outline)
    img[ring, 3] = 255
    img[outer, :3] = rgb(frame)
    img[outer, 3] = 255
    inner = rect_mask(w, h, 1 + frame_w, 1 + frame_w, w - 2 - frame_w, h - 2 - frame_w, radius=4)
    img[inner, :3] = rgb(fill)
    # soft highlight on the top edge of the fill, shade on the bottom
    ys, xs = np.where(inner)
    top, bot = ys.min(), ys.max()
    for x in range(w):
        if inner[top, x]:
            img[top, x, :3] = rgb('w')
        if inner[bot, x]:
            img[bot, x, :3] = rgb({'W': 'p', 'w': 'y', 's': 'S'}.get(fill, fill))
    if glow:
        g = blank(w + 4, h + 4)
        m = np.zeros((h + 4, w + 4), dtype=bool)
        m[2:-2, 2:-2] = img[..., 3] > 0
        ring2 = dilate4(dilate4(m)) & ~m
        g[ring2, :3] = rgb(glow)
        g[ring2, 3] = 255
        blit(g, img, 2, 2)
        img = g
    return img


def cards():
    return {
        'card': card(56, 48, 'p', 'W'),
        'card_sel': card(56, 48, 'h', 'w', frame_w=3, glow='Y'),
        'card_off': card(56, 48, 's', 's', outline='S'),
        'rcard': card(52, 52, 'p', 'W'),
        'rcard_sel': card(52, 52, 'h', 'w', frame_w=3, glow='Y'),
        'rcard_off': card(52, 52, 's', 's', outline='S'),
    }


def star_icon(fill, light, dark, outline='k', face=False):
    from objects import star_mask
    m = star_mask(16, 16, cx=8, cy=8.6, R=7.6, r=3.5)
    img = shaded_part(m, fill, light, dark, outline)
    if face:   # a happy little face: the stars she wins smile at her
        for (x, y) in ((6, 8), (6, 9), (10, 8), (10, 9)):
            img[y, x, :3] = rgb('k')
        for (x, y) in ((7, 11), (8, 11), (9, 11)):
            img[y, x, :3] = rgb('k')
        for (x, y) in ((5, 10), (11, 10)):
            img[y, x, :3] = rgb('c')
    return img


def button(letter, fill, light, dark, ink='w'):
    m = ellipse_mask(15, 15, 7.5, 7.5, 7.0, 7.0)
    img = shaded_part(m, fill, light, dark)
    blit(img, ascii_sprite([r.replace('#', ink) for r in SMALL[letter]]), 5, 4)
    return img


# Face buttons of the XiFan RF35H, SNES style: A red (right), B yellow (bottom),
# X blue (top), Y green (left). On-screen prompts use the same colours, so a
# child who does not read yet can match the colour.
BUTTONS = {'A': ('X', 'r', 'R', 'w'), 'B': ('Y', 'y', 'o', 'k'), 'X': ('n', 'B', 'N', 'w'), 'Y': ('e', 'T', 'E', 'w')}


def console_sprites():
    """The console seen from the front, 80x42, plus three highlighted
    versions (D-pad, A, B) for the spoken tutorial."""
    W, H = 80, 42
    dpad_c, face_c, lstick, rstick = (9, 15), (70, 15), (9, 31), (70, 31)
    face = {'X': (0, -5), 'Y': (-5, 0), 'A': (5, 0), 'B': (0, 5)}

    def cross_mask(cx, cy, arm=11, thick=3):
        yy, xx = np.mgrid[0:H, 0:W]
        h = arm // 2
        return (((abs(xx - cx) <= thick // 2) & (abs(yy - cy) <= h)) |
                ((abs(yy - cy) <= thick // 2) & (abs(xx - cx) <= h)))

    def disc(cx, cy, r):
        return ellipse_mask(W, H, cx + 0.5, cy + 0.5, r, r)

    def glow(img, mask):
        ring1 = dilate4(mask) & ~mask
        ring2 = dilate4(ring1 | mask) & ~(ring1 | mask)
        img[ring1, :3] = rgb('w')
        img[ring1, 3] = 255
        img[ring2, :3] = rgb('Y')
        img[ring2, 3] = 255

    def draw(highlight=None):
        img = blank(W, H)
        # shoulder buttons peeking over the top edge
        for x0 in (8, 60):
            blit(img, shaded_part(rect_mask(W, 4, x0, 1, x0 + 11, 3, radius=1), 'U', 'x', 'K'), 0, 0)
        body = rect_mask(W, H, 1, 3, W - 2, H - 2, radius=7)
        blit(img, shaded_part(body, 'z', 'I', 'Z'), 0, 0)
        # screen: bezel, then a tiny stage with the question panel and three cards
        bez = rect_mask(W, H, 18, 5, 61, 38, radius=1)
        img[bez, :3] = rgb('K')
        img[bez, 3] = 255
        scr = rect_mask(W, H, 20, 7, 59, 36)
        img[scr, :3] = rgb('V')
        floor = rect_mask(W, H, 20, 31, 59, 36)
        img[floor, :3] = rgb('P')
        panel = rect_mask(W, H, 25, 10, 54, 22, radius=1)
        img[panel, :3] = rgb('W')
        for sx, sy in ((31, 13), (39, 17), (47, 13)):   # little stars in the panel
            star = cross_mask(sx, sy, arm=3, thick=1)
            img[star, :3] = rgb('Y')
        for cx in (27, 36, 45):
            card = rect_mask(W, H, cx, 25, cx + 6, 30)
            img[card, :3] = rgb('W')
        img[rect_mask(W, H, 36, 25, 42, 30) & ~rect_mask(W, H, 37, 26, 41, 29), :3] = rgb('h')
        # left: D-pad, "-" and SELECT pills, stick with its RGB ring
        dp = cross_mask(*dpad_c)
        blit(img, shaded_part(dp, 'U', 'x', 'K', outline='Z'), 0, 0)
        for x0, y0 in ((13, 6), (13, 23)):
            img[rect_mask(W, H, x0, y0, x0 + 3, y0 + 1), :3] = rgb('U')
            img[rect_mask(W, H, x0, y0, x0 + 3, y0 + 1), 3] = 255
        rainbow = ['h', 'Y', 'G', 'T', 'B', 'v']
        for (cx, cy) in (lstick, rstick):
            ring = disc(cx, cy, 5.2) & ~disc(cx, cy, 4.0)
            ys, xs = np.where(ring)
            ang = (np.degrees(np.arctan2(ys - cy, xs - cx)) + 360) % 360
            for yv, xv, a in zip(ys, xs, ang):
                img[yv, xv, :3] = rgb(rainbow[int(a // 60) % 6])
                img[yv, xv, 3] = 255
            cap = disc(cx, cy, 3.4)
            blit(img, shaded_part(cap, 'U', 'x', 'K', outline='U'), 0, 0)
        # right: "+", START pill, the four coloured buttons
        plus = cross_mask(63, 7, arm=3, thick=1)
        img[plus, :3] = rgb('U')
        img[rect_mask(W, H, 63, 23, 66, 24), :3] = rgb('U')
        for name, (dx, dy) in face.items():
            fill, light, dark, _ = BUTTONS[name]
            bx, by = face_c[0] + dx, face_c[1] + dy
            if name == highlight:   # round glow: white, then gold
                for rr, col in ((4.9, 'Y'), (4.0, 'w')):
                    g = disc(bx, by, rr)
                    img[g, :3] = rgb(col)
                    img[g, 3] = 255
            r = 2.9 if name == highlight else 2.3
            m = disc(bx, by, r)
            blit(img, shaded_part(m, fill, light, dark, outline='k'), 0, 0)
        if highlight == 'dpad':
            glow(img, dilate4(dp))
        return img

    return {'console': draw(), 'console_dpad': draw('dpad'), 'console_a': draw('A'), 'console_b': draw('B')}


def dpad_sprites():
    """A close-up of the console's black cross for "Balla con me", 40x40 on a
    patch of grey plastic; the lit versions colour one arm like the arrow on
    screen."""
    P, S, off = 40, 32, 4
    arm_colors = {'up': ('Y', 'y', 'o'), 'down': ('T', 't', 'e'), 'left': ('h', 'P', 'H'), 'right': ('v', 'L', 'V')}
    yy, xx = np.mgrid[0:S, 0:S]
    c, t = 15.5, 5
    vert = (abs(xx - c) <= t) & (yy >= 1) & (yy <= S - 2)
    horz = (abs(yy - c) <= t) & (xx >= 1) & (xx <= S - 2)
    cross = vert | horz
    arms = {'up': vert & (yy < c - t), 'down': vert & (yy > c + t), 'left': horz & (xx < c - t),
            'right': horz & (xx > c + t)}
    # engraved triangles pointing outwards
    tri = {'up': (xx >= 13) & (xx <= 18) & (yy >= 4) & (yy <= 7) & (abs(xx - c) <= (yy - 3.5)),
           'down': (xx >= 13) & (xx <= 18) & (yy >= 24) & (yy <= 27) & (abs(xx - c) <= (27.5 - yy)),
           'left': (yy >= 13) & (yy <= 18) & (xx >= 4) & (xx <= 7) & (abs(yy - c) <= (xx - 3.5)),
           'right': (yy >= 13) & (yy <= 18) & (xx >= 24) & (xx <= 27) & (abs(yy - c) <= (27.5 - xx))}

    def draw(lit=None):
        img = shaded_part(cross, 'U', 'x', 'K', outline='Z')
        dimple = ellipse_mask(S, S, c + 0.5, c + 0.5, 3.2, 3.2)
        img[dimple, :3] = rgb('K')
        for d, m in tri.items():
            img[m, :3] = rgb('x')
        if lit:
            fill, light, dark = arm_colors[lit]
            part = shaded_part(arms[lit], fill, light, dark)
            m = arms[lit]
            img[m, :3] = part[m, :3]
            img[tri[lit], :3] = rgb('w')
        plate = shaded_part(rect_mask(P, P, 1, 1, P - 2, P - 2, radius=9), 'z', 'I', 'Z')
        blit(plate, img, off, off)
        return plate

    out = {'dpad': draw()}
    for d in arm_colors:
        out['dpad_' + d] = draw(d)
    return out


def shadow_ellipse(w=30, h=6):
    m = ellipse_mask(w, h, w / 2, h / 2, w / 2, h / 2)
    img = blank(w, h)
    yy, xx = np.mgrid[0:h, 0:w]
    sel = m & (((xx + yy) % 2) == 0)
    img[sel, :3] = rgb('K')
    img[sel, 3] = 255
    return img


def zeds():
    return {
        'zed_s': ascii_sprite(["bbb", "..b", ".b.", "b..", "bbb"]),
        'zed_l': add_outline(ascii_sprite([r.replace('#', 'b') for r in SMALL['Z']]), 'N'),
    }


LOGO_COLORS = ['h', 'Y', 'T', 'v', 'Y', 'B']


def text_row(text, fills, gap=1, space=6, outline='k', lights=None):
    """Render a row of BIG glyphs with per-letter fill colours."""
    glyphs = []
    for i, ch in enumerate(text):
        if ch == ' ':
            glyphs.append(None)
            continue
        col = fills[i % len(fills)]
        light = (lights or {}).get(col, {'h': 'P', 'Y': 'y', 'T': 't', 'v': 'L', 'B': 'b', 'W': 'w'}.get(col))
        glyphs.append(glyph(BIG[ch], col, outline, hi=light))
    w = sum(g.shape[1] if g is not None else space for g in glyphs) + gap * (len(glyphs) - 1)
    h = max(g.shape[0] for g in glyphs if g is not None)
    img = blank(w + 2, h + 3)
    for pass_ in ('shadow', 'fill'):
        x = 0
        for i, g in enumerate(glyphs):
            if g is None:
                x += space + gap
                continue
            if pass_ == 'shadow':
                sh = g.copy()
                sh[sh[..., 3] > 0, :3] = rgb('K')
                blit(img, sh, x + 1, 2)
            else:
                blit(img, g, x, 1 if (i % 2 and text[i] != "'") else 0)
            x += g.shape[1] + gap
    return img


def logo():
    top = scale(text_row("DEVA'S", LOGO_COLORS), 2)
    sub = text_row("AWESOME ADVENTURES", ['W'], space=7)
    w = max(top.shape[1], sub.shape[1])
    img = blank(w, top.shape[0] + 3 + sub.shape[0])
    blit(img, top, (w - top.shape[1]) // 2, 0)
    blit(img, sub, (w - sub.shape[1]) // 2, top.shape[0] + 3)
    return img



HAND = [
    "..k.k.k...",
    ".kakakak..",
    ".kakakak.k",
    ".kakakakka",
    ".kaaaaaaka",
    "kkaaaaaaka",
    "kaaaaaaaak",
    ".kaaaaaak.",
    "..kaAAak..",
    "...kkkk...",
]


def arrow(color, light, dark):
    """Big arrow pointing up, 24x24; rotate for the other directions."""
    from objects import poly_mask
    m = poly_mask(24, 24, [(12, 1.5), (22.5, 12), (16, 12), (16, 22.5), (8, 22.5), (8, 12), (1.5, 12)])
    return shaded_part(m, color, light, dark)


def syllable_block(state):
    """One beat of a word: 'off' (not yet said), 'cur' (being said), 'on' (said)."""
    m = rect_mask(20, 16, 1, 1, 18, 14, radius=4)
    fill, light, dark, outline = {'off': ('s', 'w', 'S', 'S'), 'cur': ('Y', 'y', 'o', 'k'),
                                  'on': ('h', 'P', 'H', 'k')}[state]
    return shaded_part(m, fill, light, dark, outline=outline)


def small_cards():
    """30x30 slots for the Sequenze row and the Balla con me strip."""
    return {
        'scard': card(30, 30, 'p', 'W'),
        'scard_sel': card(30, 30, 'h', 'w', glow='Y'),
        'scard_off': card(30, 30, 's', 's', outline='S'),
    }


def level_stars():
    from objects import star_mask
    m = star_mask(11, 11, cx=5.5, cy=5.9, R=5.4, r=2.5)
    return {'lstar_on': shaded_part(m, 'Y', 'y', 'o'), 'lstar_off': shaded_part(m, 's', 'w', 'S', outline='x')}


def letters():
    out = {}
    for ch in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
        out['let_' + ch] = glyph(BIG[ch], 'V', 'k', hi='v')
        out['letsel_' + ch] = glyph(BIG[ch], 'h', 'k', hi='P')
    out['qmark'] = glyph(BIG['?'], 'h', 'k', hi='P')
    return out


def menu_icons():
    """Game icons for the menu, 44x44, built from existing art."""
    import objects as O
    o_small, _, w_small = O.render_all(1.0)
    icons = {}
    # numbers: a star over 1 2 3
    img = blank(44, 44)
    blit(img, crop(o_small['stelle']), 10, 0)
    x = 2
    for d, c in zip('123', ('h', 'Y', 'T')):
        g = glyph(BIG[d], c, 'k', hi={'h': 'P', 'Y': 'y', 'T': 't'}[c])
        blit(img, g, x, 24)
        x += 13
    icons['menu_conta'] = img
    # words: a big A next to an apple
    img = blank(44, 44)
    blit(img, glyph(BIG['A'], 'v', 'k', hi='L'), 2, 12)
    blit(img, crop(w_small['mela']), 18, 16)
    icons['menu_parole'] = img
    # sequences: star heart star ?
    img = blank(44, 44)
    small = [crop(o_small['stelle']), crop(o_small['cuori'])]
    for i, (dx, dy) in enumerate(((0, 2), (14, 2), (0, 22))):
        blit(img, small[i % 2], dx, dy)
    q = glyph(BIG['?'], 'h', 'k', hi='P')
    blit(img, q, 24, 20)
    icons['menu_sequenze'] = img
    # dance: big music note and a ballet slipper
    o_big, _, _ = O.render_all(1.5)
    img = blank(44, 44)
    note = ascii_sprite([
        "......kkkkk",
        "......kVVVk",
        "......kVkkk",
        "......kVk..",
        "......kVk..",
        "..kkk.kVk..",
        ".kVVVkkVk..",
        "kVVVVVVVk..",
        "kVVvVVVk...",
        ".kVVVVk....",
        "..kkkk.....",
    ])
    blit(img, scale(note, 2), 20, 0)
    shoe = crop(o_big['scarpette'])
    blit(img, shoe, 0, 44 - shoe.shape[0])
    icons['menu_balla'] = img
    # my name: three letter tiles A B C and a pencil
    img = blank(44, 44)
    for i, (ch, col, hi) in enumerate((('A', 'v', 'L'), ('B', 'h', 'P'), ('C', 'T', 't'))):
        tile = card(20, 24, 'p', 'W')
        g = glyph(BIG[ch], col, 'k', hi=hi)
        tx, ty = (0, 0) if i == 0 else ((22, 0) if i == 1 else (11, 20))
        blit(img, tile, tx, ty)
        blit(img, g, tx + (20 - g.shape[1]) // 2, ty + (24 - g.shape[0]) // 2)
    icons['menu_nome'] = img
    # memory: two cards, one face down, one showing a lipstick
    import objects as O2
    img = blank(44, 44)
    back = card(22, 28, 'H', 'h')
    front = card(22, 28, 'p', 'W')
    blit(img, back, 2, 4)
    blit(img, front, 20, 12)
    O2.K = 0.8
    lip = crop(O2.rossetto())
    O2.K = 1.0
    blit(img, lip, 20 + (22 - lip.shape[1]) // 2, 12 + (28 - lip.shape[0]) // 2)
    icons['menu_memory'] = img
    # rhythm: two clapping hands and a row of dots
    img = blank(44, 44)
    hand = ascii_sprite(HAND)
    blit(img, scale(hand, 2), 2, 2)
    blit(img, scale(hand, 2)[:, ::-1].copy(), 22, 2)
    for i, c in enumerate(('Y', 'Y', 's', 'Y')):
        dot = shaded_part(ellipse_mask(8, 8, 4, 4, 3.4, 3.4), c, 'y' if c == 'Y' else 'w', 'o' if c == 'Y' else 'S')
        blit(img, dot, 2 + i * 10 + (4 if i == 3 else 0), 30)
    icons['menu_ritmo'] = img
    return icons


def memory_cards():
    """Cards of the dressing-room memory (44x44) and the cursor ring."""
    front = card(44, 44, 'p', 'W')
    back = card(44, 44, 'H', 'h')
    m = back[..., 3] > 0
    yy, xx = np.mgrid[0:44, 0:44]
    inner = rect_mask(44, 44, 5, 5, 38, 38, radius=3)
    # little hearts in a diagonal pattern on the back
    heart = [".P.P.", "PPPPP", "PPPPP", ".PPP.", "..P.."]
    hs = ascii_sprite(heart)
    for gy in range(3):
        for gx in range(3):
            if (gx + gy) % 2 == 0:
                blit(back, hs, 8 + gx * 11, 8 + gy * 11)
    star = ["..Y..", ".YyY.", "YyyyY", ".YyY.", "..Y.."]
    for gy in range(2):
        for gx in range(2):
            blit(back, ascii_sprite(star), 13 + gx * 11, 13 + gy * 11)
    ring = blank(50, 50)
    outer = rect_mask(50, 50, 0, 0, 49, 49, radius=8)
    inner2 = rect_mask(50, 50, 3, 3, 46, 46, radius=6)
    sel = outer & ~inner2
    ring[sel, :3] = rgb('Y')
    ring[sel, 3] = 255
    edge = (outer & ~rect_mask(50, 50, 1, 1, 48, 48, radius=7)) | (inner2 & ~rect_mask(50, 50, 2, 2, 47, 47, radius=7) & sel)
    ring[outer & ~rect_mask(50, 50, 1, 1, 48, 48, radius=7), :3] = rgb('o')
    return {'memo_front': front, 'memo_back': back, 'memo_cursor': ring}


def beat_dots():
    """Dots of the rhythm lines: Deva's (off/on) and hers."""
    m = ellipse_mask(12, 12, 6, 6, 5.2, 5.2)
    return {'beat_off': shaded_part(m, 's', 'w', 'S', outline='x'),
            'beat_on': shaded_part(m, 'Y', 'y', 'o'),
            'beat_her': shaded_part(m, 'h', 'P', 'H')}


def conta_extra():
    """Number line boxes and the plus sign of "Conta"."""
    out = {'nbox': card(44, 36, 'p', 'W'), 'nbox_q': card(44, 36, 'h', 'w', frame_w=2)}
    m = rect_mask(16, 16, 6, 1, 9, 14) | rect_mask(16, 16, 1, 6, 14, 9)
    out['plus'] = shaded_part(m, 'h', 'P', 'H')
    return out


def ui_extra():
    out = {}
    out['hand'] = ascii_sprite(HAND)
    up = arrow('Y', 'y', 'o')
    out['arrow_up'] = up
    out['arrow_down'] = np.rot90(arrow('T', 't', 'e'), 2).copy()
    out['arrow_left'] = np.rot90(arrow('h', 'P', 'H'), 1).copy()
    out['arrow_right'] = np.rot90(arrow('v', 'L', 'V'), -1).copy()
    for st in ('off', 'cur', 'on'):
        out['sblock_' + st] = syllable_block(st)
    out.update(small_cards())
    out.update(level_stars())
    out['mcard'] = card(60, 60, 'p', 'W')
    out['mcard_sel'] = card(60, 60, 'h', 'w', frame_w=3, glow='Y')
    out.update(letters())
    out.update(menu_icons())
    out.update(conta_extra())
    out.update(memory_cards())
    out.update(beat_dots())
    return out


def ui_sprites():
    out = {}
    out.update(big_digits())
    out.update(small_digits())
    out.update(cards())
    out['star_on'] = star_icon('Y', 'y', 'o', face=True)
    out['star_off'] = star_icon('x', 'S', 'k')
    for name, (fill, light, dark, ink) in BUTTONS.items():
        out['btn_' + name.lower()] = button(name, fill, light, dark, ink)
    out.update(console_sprites())
    out.update(dpad_sprites())
    out['shadow'] = shadow_ellipse()
    out.update(zeds())
    out['logo'] = logo()
    out.update(ui_extra())
    return out
