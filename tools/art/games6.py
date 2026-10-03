"""0.12.0: a setting of its own for each of the games that shared the pink stage.

  bg_bacheca     Conta: a sunny meadow, the question on a wooden noticeboard
  bg_pappagallo  Parole: under the parrot's tree, the question in its speech bubble
  bg_trenino     Sequenze: a toy railway across the room, the pictures ride in the wagons
  bg_insegna     Il mio nome: the stage, with the name in a sign of light bulbs
  bg_cameretta   Sopra e sotto: Deva's bedroom, the furniture on the rug (no panel)
  bg_salotto     Emozioni: a sitting room with the friends' portraits, the question in a golden frame
  bg_libro       La storia in ordine: a bedtime picture book that unfolds in three pages

The fixed places of the card games are kept clear: the panel (80, 28)-(302, 166), the
three cards at y 176-228, Deva at the lower left (x 10-54, feet at y 214), the stars of
the round at the top middle.

Sprites: the parrot (pappagallo, pappagallo_p beak open), the toy train (loco, vagone_0..3,
vagone_sel), the big letter tiles of the sign (lettera_box, lettera_box_sel: 40x48, the letters
at 2x).
"""
import numpy as np

import backgrounds as B
import ui as U
from objects import poly_mask
from pixel import blank, blit, ellipse_mask, rect_mask, shaded_part, dilate4, rgb
from scenery import (YY, XX, LIGHT_UL, tone_fill, lobe, ellipse, rect, outline, paint, glow, vfade, speckle,
                     cloud, puff_row)

W, H = 320, 240
INK = '#3b1f4a'
PANEL = (80, 28, 302, 166)


def _canvas():
    return B.canvas()

# ------------------------------------------------------------------ sprite helpers (as in games5)
def put(img, mask, base, light, shadow, ol='k'):
    blit(img, shaded_part(mask, base, light, shadow, ol), 0, 0)


def spaint(img, mask, c):
    img[mask, :3] = rgb(c) if isinstance(c, str) else c
    img[mask, 3] = 255


def spx(img, x, y, c):
    if 0 <= y < img.shape[0] and 0 <= x < img.shape[1]:
        img[y, x, :3] = rgb(c)
        img[y, x, 3] = 255


def E(w, h, cx, cy, rx, ry):
    return ellipse_mask(w, h, cx, cy, rx, ry)


def R(w, h, x0, y0, x1, y1, r=0):
    return rect_mask(w, h, int(round(x0)), int(round(y0)), int(round(x1)), int(round(y1)), int(r))


def P(w, h, pts):
    return poly_mask(w, h, pts)



def _rrect(x0, y0, x1, y1, r):
    return rect_mask(W, H, x0, y0, x1, y1, radius=r)


def _hline(img, x0, x1, y, c):
    paint(img, rect(x0, y, x1, y), c)


def flower(img, cx, cy, petal, centre='#ffd23f', r=2.2, n=5):
    """A little flower: n round petals around a yellow middle."""
    for k in range(n):
        a = k * 2 * np.pi / n - np.pi / 2
        m = ellipse(cx + np.cos(a) * r * 1.2, cy + np.sin(a) * r * 1.2, r, r)
        paint(img, m, petal)
    paint(img, ellipse(cx, cy, r * 0.75, r * 0.75), centre)


def tulip(img, x, base, h, col):
    paint(img, rect(x, base - h, x, base), '#3e9a5a')
    m = ellipse(x + 0.5, base - h - 2, 3, 3.6)
    outline(img, m, INK)
    tone_fill(img, m, 0.5 + (YY - (base - h - 5)) / 8.0 * 0.4, [col[2], col[1], col[0]])
    paint(img, rect(x, base - h - 5, x, base - h - 3), col[2])


def butterfly(img, cx, cy, wing, spot='#ffffff'):
    for sx in (-1, 1):
        up = ellipse(cx + sx * 3.2, cy - 2, 3.2, 3.0)
        lo = ellipse(cx + sx * 2.6, cy + 2.5, 2.4, 2.2)
        outline(img, up | lo, INK)
        paint(img, up | lo, wing)
        paint(img, ellipse(cx + sx * 3.4, cy - 2.4, 1.0, 1.0), spot)
    paint(img, rect(cx, cy - 4, cx, cy + 4), INK)


def fence(img, y0, y1, x0=0, x1=W - 1, step=14):
    """A white picket fence."""
    for x in range(x0, x1 + 1, step):
        post = (XX >= x) & (XX <= x + 7) & (YY >= y0) & (YY <= y1) & ~((YY < y0 + 4) & (np.abs(XX - x - 3.5) > (YY - y0)))
        outline(img, post, '#8a7aa0')
        tone_fill(img, post, 0.55 + (XX - x) / 8.0 * 0.4, ['#d8d0e8', '#eee8f6', '#ffffff'])
    for y in (y0 + 8, y1 - 8):
        rail = rect(x0, y, x1, y + 3) & ~((XX % step) < 8)
        outline(img, rail, '#8a7aa0')
        tone_fill(img, rail, 0.6, ['#e6dff0', '#f6f2fa'])


def wood_board(img, x0, y0, x1, y1, r=8, frame=5, paper=('#fff6e4', '#fffaf0'), lines=True):
    """A noticeboard: a wooden frame round a cream paper (the panel of the questions)."""
    outer = _rrect(x0, y0, x1, y1, r)
    inner = _rrect(x0 + frame, y0 + frame, x1 - frame, y1 - frame, max(2, r - frame + 2))
    outline(img, outer, INK)
    wood = outer & ~inner
    tone_fill(img, wood, 0.35 + ((XX - x0) % 23) / 23.0 * 0.25 + (YY < y0 + frame) * 0.25,
              ['#8a5a30', '#a8703e', '#c98f58', '#e0ad76'])
    paint(img, dilate4(inner) & ~inner & wood, '#6e4526')
    tone_fill(img, inner, 0.2 + (YY - y0) / float(y1 - y0) * 0.6, list(paper))
    if lines:   # the faint lines of a sheet of drawing paper
        for y in range(y0 + frame + 14, y1 - frame - 4, 14):
            paint(img, inner & (YY == y) & ((XX % 4) != 0), '#f8ead2')
    for (cx, cy) in ((x0 + 3, y0 + 3), (x1 - 3, y0 + 3), (x0 + 3, y1 - 3), (x1 - 3, y1 - 3)):   # brass nails
        paint(img, ellipse(cx + 0.5, cy + 0.5, 1.6, 1.6), '#ffd23f')
        paint(img, rect(cx, cy, cx, cy), '#fff4b0')
    return inner


# ------------------------------------------------------------------ Conta: the meadow
def prato_bg():
    img = _canvas()
    B.gradient(img, 0, 150, ['#8fd8ff', '#a8e2ff', '#c4ecff', '#dcf4ff', '#eefaff'])
    # the sun, high on the left, and soft clouds
    glow(img, 34, 30, 40, 40, '#fff6b0', a=0.55)
    sun = lobe(img, 34, 30, 13, 13, ['#ffb43c', '#ffd23f', '#ffe680', '#fff4c0'])
    outline(img, sun, '#e8902a')
    for (cx, cy) in ((28, 27), (40, 27)):   # closed happy eyes, a smile, rosy cheeks
        for dx, dy in ((-1, 1), (0, 0), (1, 0), (2, 1)):
            paint(img, rect(cx + dx, cy + dy, cx + dx, cy + dy), INK)
    for dx, dy in ((-3, 0), (-2, 1), (-1, 1), (0, 2), (1, 2), (2, 1), (3, 1), (4, 0)):
        paint(img, rect(34 + dx, 33 + dy, 34 + dx, 33 + dy), INK)
    paint(img, ellipse(25.5, 32, 2, 1.2) | ellipse(42.5, 32, 2, 1.2), '#ff9a8a')
    for k in range(8):   # little rays
        a = k * np.pi / 4 + 0.3
        x, y = 34 + np.cos(a) * 18, 30 + np.sin(a) * 18
        paint(img, ellipse(x, y, 1.6, 1.6), '#ffd23f')
    for (x0, x1, y, r, seed) in ((90, 150, 14, 7, 3), (230, 300, 20, 8, 5), (300, 330, 8, 6, 9)):
        cloud(img, puff_row(x0, x1, y, r, seed), ['#d8ecff', '#eef7ff', '#ffffff'])
    # far hills, near hills
    far = (YY > 118 + 10 * np.sin(XX / 40.0) + 6 * np.sin(XX / 17.0 + 1.0))
    tone_fill(img, far & (YY < 200), 0.4 + (YY - 118) / 60.0 * 0.4, ['#7cc87a', '#94d68c', '#acdfa0'])
    near = (YY > 146 + 8 * np.sin(XX / 33.0 + 2.0))
    tone_fill(img, near, 0.3 + (YY - 146) / 90.0 * 0.6, ['#5cb85c', '#6cc46a', '#7ed07a', '#92d888'])
    # trees far away
    for (cx, base, r) in ((14, 132, 13), (306, 128, 15), (60, 128, 10)):
        paint(img, rect(cx - 1, base - 6, cx + 2, base + 6), '#8a5a30')
        m = lobe(img, cx, base - r, r, r * 0.95, ['#3e8a4a', '#4fa65a', '#6cc46a', '#8ad888'])
        outline(img, m, '#2e6a3a')
    # the white fence behind the meadow
    fence(img, 150, 176)
    # the noticeboard on its two posts (the panel of the questions)
    x0, y0, x1, y1 = PANEL
    for px_ in (x0 + 24, x1 - 30):
        post = rect(px_, y1, px_ + 6, 200)
        outline(img, post, INK)
        tone_fill(img, post, 0.5 + (XX - px_) / 7.0 * 0.4, ['#8a5a30', '#a8703e', '#c98f58'])
    wood_board(img, x0, y0, x1, y1)
    # a garland of flowers on top of the board
    for k, x in enumerate(range(x0 + 14, x1 - 8, 22)):
        flower(img, x, y0 + 1 + (k % 2), ['#ff93c6', '#ffffff', '#b4a0ff', '#ffd23f'][k % 4], r=2.2)
        paint(img, ellipse(x + 7, y0 + 2, 3, 1.3), '#4fa65a')
    # the grass in front: flowers, tulips, a butterfly or two
    rng = np.random.default_rng(12)
    for _ in range(70):
        x, y = int(rng.integers(0, W)), int(rng.integers(188, 240))
        if 60 < x < 300 and 172 < y < 232:
            continue   # not behind the cards
        flower(img, x, y, ['#ff93c6', '#ffffff', '#ffd23f', '#b4a0ff', '#ff7a7a'][int(rng.integers(0, 5))], r=1.4)
    for (x, base, col) in ((300, 236, ('#c4305a', '#ff5a8a', '#ff93b8')), (308, 232, ('#d8a020', '#ffd23f', '#fff080')),
                           (64, 238, ('#8a50d0', '#b47aff', '#d8b4ff'))):
        tulip(img, x, base, 10, col)
    butterfly(img, 20, 70, '#ff93c6')
    butterfly(img, 312, 96, '#8fd8ff')
    butterfly(img, 62, 104, '#ffd23f')
    return img


# ------------------------------------------------------------------ Parole: the parrot's tree
def pappagallo(talk=False):
    """Coco the parrot (36x44, drawn at 2x on its branch), facing right towards the speech bubble;
    talk: beak open."""
    w, h = 36, 44
    img = blank(w, h)
    yy, xx = np.mgrid[0:h, 0:w]
    # the long tail down behind the branch: blue with a yellow edge
    tail = P(w, h, [(9, 30), (16, 31), (14, 43), (8, 43)])
    put(img, tail, '#3a5ad8', '#6a8aff', '#2a3aa8')
    spaint(img, tail & (xx >= 13), '#ffd23f')
    # the body, a round red pear, a lighter belly
    body = E(w, h, 16, 25, 9, 11)
    put(img, body, '#e83a4a', '#ff6a6a', '#b0243a')
    spaint(img, E(w, h, 19, 29, 4.5, 6.5) & body, '#ff7a6a')
    # the wing folded on the side: yellow on the shoulder, then green, the long blue feathers
    wing = P(w, h, [(8, 20), (15, 18), (18, 24), (15, 35), (9, 38), (6, 30)])
    put(img, wing, '#3a8ae8', '#6ab4ff', '#2a5ab0')
    spaint(img, wing & (yy < 23), '#ffd23f')
    spaint(img, wing & (yy >= 23) & (yy < 27), '#4cc06a')
    for k in range(3):   # the feather lines
        for t in range(6):
            spx(img, 9 + k * 2 + t // 3, 28 + t, '#2a5ab0')
    # the head, with the white patch round the eye
    head = E(w, h, 20, 11, 8, 7.5)
    put(img, head, '#e83a4a', '#ff6a6a', '#b0243a')
    spaint(img, E(w, h, 23, 10.5, 3.6, 3.4) & head, '#fff4fa')
    spaint(img, R(w, h, 23, 9, 24, 11), 'k')
    spx(img, 23, 9, 'w')
    spaint(img, E(w, h, 21, 14.5, 1.6, 1.0), '#ffb4c8')   # a pink cheek
    for (x0, y0, x1, y1) in ((16, 5, 13, 0), (18, 4, 17, -1), (20, 4, 21, 0)):   # a little crest
        for t in range(7):
            spx(img, round(x0 + (x1 - x0) * t / 6), round(y0 + (y1 - y0) * t / 6), '#ffd23f')
    # the curved beak
    if talk:
        upper = P(w, h, [(26, 8), (34, 10), (32, 13), (27, 12)])
        lower = P(w, h, [(27, 14), (31, 15), (28, 18)])
        put(img, upper, '#f0c040', '#fff080', '#c09020')
        put(img, lower, '#d8a020', '#f0c040', '#a07010')
        spaint(img, P(w, h, [(27, 12.5), (31, 13.5), (28, 14.5)]), '#c42a3a')
    else:
        upper = P(w, h, [(26, 8), (34, 10), (33, 14), (27, 14)])
        put(img, upper, '#f0c040', '#fff080', '#c09020')
        spaint(img, R(w, h, 27, 13, 31, 13), '#a07010')
    for fx_ in (14, 19):   # the feet on the branch
        spaint(img, R(w, h, fx_, 35, fx_ + 2, 37), '#8a6a5a')
    return img


def speech_bubble(img, x0, y0, x1, y1, tip, base_y0, base_y1, fill='#ffffff', edge='#8a6ad0'):
    """A big speech bubble (the panel), with its tail towards `tip`, joined on the left side."""
    body = _rrect(x0, y0, x1, y1, 14)
    tx, ty = tip
    tail = poly_mask(W, H, [(x0 + 6, base_y0), (tx, ty), (x0 + 6, base_y1)])
    m = body | tail
    outline(img, m, edge)
    paint(img, m, fill)
    paint(img, dilate4(m) & ~m & dilate4(dilate4(m)), edge)
    tone_fill(img, body, 0.75 + (YY - y0) / float(y1 - y0) * 0.25, ['#f2ecfa', '#faf6ff', '#ffffff'])
    return body


def pappagallo_bg():
    img = _canvas()
    B.gradient(img, 0, 170, ['#a8e2ff', '#bfeaff', '#d4f2ff', '#e8faff'])
    # a hedge with big pink flowers far behind
    hedge = YY > 122 + 6 * np.sin(XX / 9.0) + 4 * np.sin(XX / 23.0)
    tone_fill(img, hedge & (YY < 200), 0.3 + (YY - 118) / 60.0 * 0.5, ['#3e8a4a', '#4fa65a', '#5fb868'])
    rng = np.random.default_rng(4)
    for _ in range(26):
        x, y = int(rng.integers(0, W)), int(rng.integers(128, 168))
        flower(img, x, y, ['#ff7ab8', '#ff93c6', '#ffffff'][int(rng.integers(0, 3))], r=2.4)
    # the lawn
    lawn = YY >= 168
    tone_fill(img, lawn, 0.3 + (YY - 168) / 72.0 * 0.6, ['#5cb85c', '#6cc46a', '#7ed07a', '#92d888'])
    # the big tree on the left: trunk, the branch where Coco sits, the leaves above
    trunk = (XX < 26 + 6 * np.sin(YY / 30.0)) & (YY > 30)
    outline(img, trunk, '#4a2e1a')
    tone_fill(img, trunk, 0.3 + XX / 30.0 * 0.6, ['#6e4526', '#8a5a30', '#a8703e', '#c98f58'])
    for y in range(40, 240, 9):   # the bark
        paint(img, trunk & (YY == y) & ((XX + y) % 7 < 3), '#6e4526')
    branch = poly_mask(W, H, [(18, 94), (80, 98), (80, 103), (18, 104)])
    outline(img, branch, '#4a2e1a')
    tone_fill(img, branch, 0.35 + (YY - 94) / 10.0 * 0.5, ['#8a5a30', '#a8703e', '#c98f58'][::-1])
    for (cx, cy, rx, ry) in ((10, 6, 46, 30), (64, 0, 44, 22), (120, -6, 40, 16), (-6, 40, 26, 26)):
        m = lobe(img, cx, cy, rx, ry, ['#2e7a3a', '#3e8a4a', '#4fa65a', '#6cc46a', '#8ad888'])
        outline(img, m, '#256a32')
    for (x, y) in ((24, 18), (52, 10), (90, 6), (12, 34), (70, 16)):   # fruits in the leaves
        m = lobe(img, x, y, 3.2, 3.2, ['#d83a3a', '#ff6a5a', '#ffa08a'])
        outline(img, m, '#8a2030')
    # leaves hanging from the branch, a little leafy twig on its end
    for (x, y) in ((70, 104), (60, 106), (78, 100)):
        m = ellipse(x, y + 3, 2.2, 4)
        paint(img, m, '#4fa65a')
    # the speech bubble (the panel of the questions), its tail towards Coco's beak (2x, at x 70, y 44)
    x0, y0, x1, y1 = PANEL
    speech_bubble(img, x0, y0, x1, y1, (74, 44), 38, 56)
    # flowers on the lawn, clear of the cards
    for _ in range(60):
        x, y = int(rng.integers(0, W)), int(rng.integers(186, 240))
        if 60 < x < 300 and 170 < y < 232:
            continue
        flower(img, x, y, ['#ff93c6', '#ffffff', '#ffd23f', '#b4a0ff'][int(rng.integers(0, 4))], r=1.4)
    butterfly(img, 308, 150, '#ffd23f')
    return img


# ------------------------------------------------------------------ Sequenze: the toy railway
RAIL_Y = 132   # the top of the rails: the wheels of the train stand on it (scene_sequenze.c)
WAGON_COLS = [('#e83a4a', '#ff6a6a', '#b0243a'), ('#3a8ae8', '#6ab4ff', '#2a5ab0'),
              ('#4cc06a', '#7ee08a', '#2e8a4a'), ('#ff9a2e', '#ffc060', '#c86a10')]


def vagone(col, sel=False):
    """An open toy wagon (36x26): a box on two red wheels, hooks at both ends; sel = golden, glowing."""
    w, h = 36, 26
    img = blank(w, h)
    if sel:
        col = ('#ffd23f', '#fff080', '#e8a020')
    box = R(w, h, 3, 4, 32, 19, 2)
    put(img, box, *col)
    spaint(img, R(w, h, 4, 5, 31, 6), col[1])                 # the rim, lit
    for x in (12, 23):                                       # the planks
        spaint(img, R(w, h, x, 8, x, 18), col[2])
    spaint(img, R(w, h, 6, 10, 9, 15) | R(w, h, 26, 10, 29, 15), col[1])
    for x in (0, 33):                                        # the hooks
        spaint(img, R(w, h, x, 15, x + 2, 16), '#5a5a6a')
    for cx in (10, 25):                                      # the wheels
        wheel = E(w, h, cx, 21, 4.4, 4.4)
        put(img, wheel, '#3b3b4a', '#6a6a7a', '#1e1e28')
        spaint(img, E(w, h, cx, 21, 1.6, 1.6), '#ffd23f')
    if sel:   # a glint
        spaint(img, R(w, h, 6, 7, 7, 8), 'w')
    return img


def loco():
    """The toy locomotive (46x42), facing left: it pulls the wagons. A toy, with no face."""
    w, h = 46, 42
    img = blank(w, h)
    yy, xx = np.mgrid[0:h, 0:w]
    # the cab at the back (right), red with a window and a blue roof
    cab = R(w, h, 26, 8, 41, 33, 1)
    put(img, cab, '#e83a4a', '#ff6a6a', '#b0243a')
    roof = R(w, h, 24, 4, 43, 8, 2)
    put(img, roof, '#3a5ad8', '#6a8aff', '#2a3aa8')
    win = R(w, h, 29, 12, 38, 20, 1)
    spaint(img, win, '#bfeaff')
    spaint(img, R(w, h, 29, 12, 33, 13), 'w')
    # the boiler, a blue drum with gold bands, the chimney, the lamp
    boiler = R(w, h, 7, 16, 27, 33, 6)
    put(img, boiler, '#3a8ae8', '#6ab4ff', '#2a5ab0')
    for x in (13, 21):
        spaint(img, boiler & (xx >= x) & (xx <= x + 1), '#ffd23f')
    chim = P(w, h, [(11, 4), (19, 4), (17, 10), (17, 16), (13, 16), (13, 10)])
    put(img, chim, '#ffd23f', '#fff080', '#e8a020')
    lamp = E(w, h, 5, 22, 3, 3)
    put(img, lamp, '#fff4b0', '#ffffff', '#ffd23f')
    # the base and the cow-catcher at the front
    base = R(w, h, 4, 32, 43, 35)
    spaint(img, base, '#5a3a8a')
    catcher = P(w, h, [(0, 37), (6, 31), (8, 37)])
    put(img, catcher, '#ffd23f', '#fff080', '#e8a020')
    for cx, r in ((13, 5.4), (25, 5.4), (37, 4.4)):            # the wheels
        wheel = E(w, h, cx, 37 - (5.4 - r), r, r)
        put(img, wheel, '#e83a4a', '#ff6a6a', '#b0243a')
        spaint(img, E(w, h, cx, 37 - (5.4 - r), 1.7, 1.7), '#ffd23f')
    spaint(img, R(w, h, 12, 36, 26, 37), '#ffd23f')            # the rod between the wheels
    return img


def trenino_bg():
    img = _canvas()
    B.gradient(img, 0, 150, ['#9ad8ff', '#b4e4ff', '#cdeeff', '#e2f6ff', '#f2fbff'])
    for (x0, x1, y, r, seed) in ((20, 90, 18, 7, 2), (150, 230, 10, 6, 6), (250, 320, 26, 8, 8)):
        cloud(img, puff_row(x0, x1, y, r, seed), ['#d8ecff', '#eef7ff', '#ffffff'])
    # hills far away, a little town of toy houses
    far = YY > 92 + 9 * np.sin(XX / 37.0 + 0.5) + 5 * np.sin(XX / 13.0)
    tone_fill(img, far & (YY < 150), 0.4 + (YY - 88) / 50.0 * 0.4, ['#8ad08a', '#9cd89a', '#b0e2a8'])
    for k, (x, wdt, hgt, roof_c) in enumerate(((150, 18, 14, '#e83a4a'), (172, 14, 18, '#3a8ae8'),
                                                  (192, 20, 12, '#ff9a2e'), (240, 16, 16, '#b47aff'))):
        base = 96 + 4 * np.sin(x / 37.0 + 0.5)
        house = rect(x, int(base - hgt), x + wdt, int(base) + 4)
        outline(img, house, '#8a7aa0')
        paint(img, house, ['#fff6e4', '#fff0f6', '#f0f8ff', '#fffae0'][k])
        roof = poly_mask(W, H, [(x - 3, base - hgt), (x + wdt / 2.0, base - hgt - 9), (x + wdt + 3, base - hgt)])
        outline(img, roof, INK)
        paint(img, roof, roof_c)
        paint(img, rect(x + wdt // 2 - 2, int(base) - 6, x + wdt // 2 + 1, int(base) + 4), '#a8703e')
    # the meadow and the embankment of the railway
    tone_fill(img, YY >= 112 + 4 * np.sin(XX / 21.0), 0.3 + (YY - 112) / 128.0 * 0.6,
              ['#5cb85c', '#6cc46a', '#7ed07a', '#92d888'])
    bank = rect(0, RAIL_Y - 2, W - 1, RAIL_Y + 12)
    tone_fill(img, bank, 0.35 + (YY - RAIL_Y) / 14.0 * 0.4, ['#a89a8a', '#c4b8a8', '#d8cec0'])
    speckle(img, bank, '#8a7a6a', 0.08, 3)
    for x in range(-4, W, 10):                                # the sleepers
        sl = rect(x, RAIL_Y + 2, x + 6, RAIL_Y + 6)
        outline(img, sl, '#4a2e1a')
        paint(img, sl, '#8a5a30')
    for y in (RAIL_Y, RAIL_Y + 4):                            # the two rails
        paint(img, rect(0, y, W - 1, y + 1), '#6a6a7a')
        paint(img, rect(0, y, W - 1, y), '#b4b4c4')
    # a tunnel on the right where the train comes from
    hill = ellipse(318, RAIL_Y + 4, 34, 46) & (YY <= RAIL_Y + 12)
    outline(img, hill, '#2e6a3a')
    tone_fill(img, hill, 0.35 + (318 - XX) / 40.0 * 0.4, ['#3e8a4a', '#4fa65a', '#6cc46a'])
    mouth = (ellipse(312, RAIL_Y + 6, 16, 24) & (YY <= RAIL_Y + 6)) & (XX >= 296)
    outline(img, mouth, '#8a7a6a')
    paint(img, mouth, '#2a2238')
    # the station sign with the little train on it
    paint(img, rect(26, 70, 28, RAIL_Y - 2), '#6e4526')
    sign = rect(10, 54, 44, 70)
    outline(img, sign, INK)
    tone_fill(img, sign, 0.6, ['#fff6e4', '#fffaf0'])
    for (x, c) in ((15, '#e83a4a'), (23, '#3a8ae8'), (31, '#4cc06a')):
        paint(img, rect(x, 59, x + 6, 64), c)
        paint(img, rect(x + 1, 65, x + 2, 66) | rect(x + 4, 65, x + 5, 66), INK)
    # flowers by the railway, clear of the cards
    rng = np.random.default_rng(9)
    for _ in range(70):
        x, y = int(rng.integers(0, W)), int(rng.integers(150, 240))
        if 60 < x < 300 and 170 < y < 232:
            continue
        flower(img, x, y, ['#ff93c6', '#ffffff', '#ffd23f', '#b4a0ff'][int(rng.integers(0, 4))], r=1.4)
    return img


# ------------------------------------------------------------------ Il mio nome: the sign of light bulbs
BULB_STEP = 12   # the bulbs of the sign, every 12 pixels along its frame (amb.c makes them blink)


def marquee_bulbs():
    """Where the bulbs of the sign are: (x, y) along the frame of the panel."""
    x0, y0, x1, y1 = PANEL
    pts = []
    for x in range(x0 + 6, x1 - 2, BULB_STEP):
        pts += [(x, y0 + 3), (x, y1 - 3)]
    for y in range(y0 + 6 + BULB_STEP, y1 - 8, BULB_STEP):
        pts += [(x0 + 3, y), (x1 - 3, y)]
    return pts


def insegna_bg():
    img = B.stage(disco=False)
    x0, y0, x1, y1 = PANEL
    outer = _rrect(x0 - 2, y0 - 2, x1 + 2, y1 + 2, 6)
    inner = _rrect(x0 + 7, y0 + 7, x1 - 7, y1 - 7, 4)
    outline(img, outer, INK)
    frame = outer & ~inner
    tone_fill(img, frame, 0.3 + (YY < y0 + 6) * 0.4 + (XX < x0 + 6) * 0.2, ['#8a1840', '#b02458', '#d83a72', '#ff6a9a'])
    paint(img, dilate4(inner) & ~inner, '#ffd23f')
    # the inside: deep blue velvet with a few twinkles, light enough round the letters
    tone_fill(img, inner, 0.15 + (YY - y0) / float(y1 - y0) * 0.5, ['#2a1650', '#331c60', '#3e2470', '#4a2c80'])
    rng = np.random.default_rng(5)
    for _ in range(40):
        x, y = int(rng.integers(x0 + 10, x1 - 10)), int(rng.integers(y0 + 10, y1 - 10))
        paint(img, rect(x, y, x, y), ['#ffd23f', '#ffffff', '#ff93c6'][int(rng.integers(0, 3))])
    glow(img, (x0 + x1) / 2, y0 + 44, 70, 40, '#8a5ad0', a=0.35, mask=inner)   # a light on the portrait
    for (bx, by) in marquee_bulbs():                                           # the bulbs, lit
        m = ellipse(bx + 0.5, by + 0.5, 2.6, 2.6)
        outline(img, m, '#8a5a10')
        paint(img, m, '#fff4b0')
        paint(img, rect(bx - 1, by - 1, bx - 1, by - 1), '#ffffff')
    # the sign hangs from two gold chains
    for cx in (x0 + 30, x1 - 30):
        for y in range(18, y0 - 2, 3):
            paint(img, rect(cx, y, cx + 1, y + 1), '#ffd23f')
    return img


# ------------------------------------------------------------------ Sopra e sotto: Deva's bedroom
FLOOR_Y = 158   # where the table, the box and the house stand (scene_dove.c): on the rug, well inside the floor
WALL_Y = 132    # where the wall meets the floor


def cameretta_bg():
    img = _canvas()
    # lilac wallpaper with little hearts and stars
    tone_fill(img, YY < WALL_Y, 0.45 + (YY / float(WALL_Y)) * 0.3, ['#e2d2f6', '#ecdefa', '#f4ecfd'])
    for y in range(12, WALL_Y - 10, 22):
        for x in range((y // 22 % 2) * 11 + 4, W, 22):
            if (y // 22 + x // 22) % 2:
                for (dx, dy) in ((0, 0), (2, 0), (-1, 1), (3, 1), (0, 2), (2, 2), (1, 3)):
                    paint(img, rect(x + dx, y + dy, x + dx, y + dy), '#ffb4d8')
            else:
                paint(img, rect(x + 1, y, x + 1, y + 2) | rect(x, y + 1, x + 2, y + 1), '#fff4b0')
    # the skirting board and the wooden floor, its boards running into the room
    paint(img, rect(0, WALL_Y - 7, W - 1, WALL_Y - 1), '#ffffff')
    paint(img, rect(0, WALL_Y - 8, W - 1, WALL_Y - 8), '#b4a0d0')
    paint(img, rect(0, WALL_Y - 2, W - 1, WALL_Y - 1), '#d8c8ec')
    floor = YY >= WALL_Y
    depth = (YY - WALL_Y) / float(H - WALL_Y)
    board = np.floor((XX - 160) / (24 + 40 * depth)).astype(int)        # boards widen towards her
    tone_fill(img, floor, 0.35 + (board % 2) * 0.2 + depth * 0.3, ['#c98f58', '#d8a26a', '#e6b67e', '#f2cc98'])
    edge = np.floor((XX - 1 - 160) / (24 + 40 * depth)).astype(int) != board
    paint(img, floor & edge, '#b07a46')
    paint(img, rect(0, WALL_Y, W - 1, WALL_Y), '#a8703e')
    # a round rug where the things stand
    rug = ellipse(191, FLOOR_Y - 2, 120, 15)
    outline(img, rug, '#c45a9a')
    tone_fill(img, rug, 0.5, ['#ff93c6', '#ffb4d8'])
    paint(img, rug & ~ellipse(191, FLOOR_Y - 2, 112, 12) & ((XX % 4) < 2), '#ffffff')
    paint(img, ellipse(191, FLOOR_Y - 2, 64, 7) & ~ellipse(191, FLOOR_Y - 2, 60, 5), '#ffd6ec')
    # the window with curtains, top left
    win = rect(8, 18, 66, 82)
    outline(img, win, INK)
    B.gradient(img, 19, 82, ['#9ad8ff', '#bfeaff', '#e2f6ff'], x0=9, x1=66)
    cloud(img, puff_row(16, 50, 40, 5, 4), ['#e8f4ff', '#ffffff'])
    paint(img, rect(36, 18, 37, 82) | rect(8, 49, 66, 50), '#ffffff')
    paint(img, rect(4, 82, 70, 86), '#ffffff')
    paint(img, rect(4, 87, 70, 87), '#b4a0d0')
    for (cx, flip) in ((10, 1), (64, -1)):   # the curtains, tied
        cur = poly_mask(W, H, [(cx - 8 * flip, 12), (cx + 10 * flip, 12), (cx + 3 * flip, 50), (cx + 9 * flip, 88), (cx - 8 * flip, 88)])
        outline(img, cur, '#c45a9a')
        tone_fill(img, cur, 0.4 + ((XX + 2) % 6 < 3) * 0.3, ['#ff7ab8', '#ff93c6', '#ffb4d8'])
    paint(img, rect(0, 10, 76, 13), '#a8703e')
    # her drawing pinned on the wall, right: a smiling sun over a little house
    pap = rect(282, 22, 314, 50)
    outline(img, pap, '#b4a0d0')
    paint(img, pap, '#ffffff')
    sun = ellipse(292, 31, 5, 5)
    paint(img, sun, '#ffd23f')
    for (dx, dy) in ((-8, 0), (8, 0), (0, -8), (0, 7), (-6, -6), (6, -6), (-6, 5), (6, 5)):
        paint(img, rect(292 + dx, 31 + dy, 292 + dx, 31 + dy), '#ffb43c')
    paint(img, rect(290, 31, 290, 31) | rect(294, 31, 294, 31), INK)
    paint(img, poly_mask(W, H, [(300, 40), (306, 34), (312, 40)]), '#e83a4a')
    paint(img, rect(301, 41, 311, 47), '#ff93c6')
    paint(img, rect(305, 43, 307, 47), '#a8703e')
    paint(img, rect(284, 47, 312, 48), '#7ad66a')
    paint(img, rect(297, 20, 299, 22), '#e83a4a')   # the pin
    # a little chest of drawers on the right, the teddy sitting on it
    ch = rect(292, 92, 319, WALL_Y + 4)
    outline(img, ch, INK)
    tone_fill(img, ch, 0.45 + (XX < 296) * 0.3, ['#e8a0c8', '#ffb4d8', '#ffd6ec'])
    for y in (100, 112, 124):
        paint(img, rect(295, y, 319, y), '#c45a9a')
        paint(img, rect(305, y + 4, 308, y + 5), '#ffd23f')
    paint(img, rect(292, 92, 319, 93), '#ffffff')
    body = lobe(img, 306, 84, 8, 7, ['#a8703e', '#c98f58', '#e0ad76'])
    head = lobe(img, 306, 72, 6.5, 6, ['#a8703e', '#c98f58', '#e0ad76'])
    ears = ellipse(300, 66, 2.5, 2.5) | ellipse(312, 66, 2.5, 2.5)
    paint(img, ears, '#a8703e')
    outline(img, body | head | ears, INK)
    paint(img, rect(303, 71, 303, 72) | rect(309, 71, 309, 72), INK)
    paint(img, ellipse(306, 75.5, 2.5, 1.6), '#f2cc98')
    paint(img, rect(306, 75, 306, 75), INK)
    paint(img, rect(304, 79, 308, 80), '#ff7ab8')   # a bow
    return img


# ------------------------------------------------------------------ Emozioni: the sitting room
def _portrait(img, x0, y0, icon, frame='#ffd23f', mat='#fff6e4'):
    """A little framed portrait of a friend on the wall (a 44x44 icon, made smaller: every other pixel)."""
    from PIL import Image
    import os
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'art', 'png', icon + '.png')
    try:
        pic = np.array(Image.open(path).convert('RGBA'))
    except OSError:
        return
    pic = pic[::2, ::2]   # 22x22
    h, w = pic.shape[:2]
    fr = rect(x0, y0, x0 + w + 7, y0 + h + 7)
    outline(img, fr, INK)
    tone_fill(img, fr, 0.5 + (YY < y0 + 3) * 0.3, ['#c88a10', '#e8a020', frame])
    paint(img, rect(x0 + 3, y0 + 3, x0 + w + 4, y0 + h + 4), mat)
    sub = img[y0 + 4:y0 + 4 + h, x0 + 4:x0 + 4 + w]
    a = pic[..., 3] > 127
    sub[a, :3] = pic[a, :3]


def salotto_bg():
    img = _canvas()
    # striped wallpaper, warm cream and soft pink
    stripes = ((XX // 9) % 2)
    tone_fill(img, YY < 170, 0.35 + stripes * 0.3 + (YY / 170.0) * 0.2, ['#f6dce6', '#fbe8ee', '#fff2f4'])
    paint(img, (YY < 170) & ((XX % 18) == 0), '#f0c8d8')
    # a dado rail and the lower wall
    paint(img, rect(0, 168, W - 1, 170), '#c98f58')
    tone_fill(img, (YY > 170) & (YY < 196), 0.5, ['#d8a26a', '#e6b67e'])
    # the floor
    tone_fill(img, YY >= 196, 0.3 + (YY - 196) / 44.0 * 0.5, ['#a8703e', '#b88050', '#c98f58'])
    paint(img, (YY >= 196) & (((XX + (YY - 196) // 8 * 17) % 34) == 0), '#8a5a30')
    paint(img, rect(0, 195, W - 1, 196), '#6e4526')
    # the big golden frame (the panel of the questions): a soft light inside it
    x0, y0, x1, y1 = PANEL
    outer = _rrect(x0 - 3, y0 - 3, x1 + 3, y1 + 3, 3)
    inner = rect(x0 + 6, y0 + 6, x1 - 6, y1 - 6)
    outline(img, outer, INK)
    gold = outer & ~inner
    tone_fill(img, gold, 0.25 + (YY < y0 + 3) * 0.3 + (XX < x0 + 3) * 0.2 + ((XX + YY) % 7 == 0) * 0.2,
              ['#a87010', '#c88a10', '#e8a020', '#ffd23f', '#fff080'])
    paint(img, dilate4(inner) & ~inner, '#8a5a10')
    cx, cy = (x0 + x1) / 2.0, (y0 + y1) / 2.0
    d = np.sqrt(((XX - cx) / 110.0) ** 2 + ((YY - cy) / 70.0) ** 2)
    tone_fill(img, inner, 0.95 - d * 0.6, ['#f4e4d4', '#faeee2', '#fff6ec', '#fffaf4'])
    for (gx, gy) in ((x0 - 1, y0 - 1), (x1 - 5, y0 - 1), (x0 - 1, y1 - 5), (x1 - 5, y1 - 5)):   # gold rosettes
        m = ellipse(gx + 3, gy + 3, 4, 4)
        outline(img, m, '#8a5a10')
        paint(img, m, '#ffd23f')
        paint(img, rect(gx + 2, gy + 2, gx + 3, gy + 3), '#fff4b0')
    # little portraits of the friends on the left wall, and one on the right
    _portrait(img, 10, 22, 'amico_ciuffone')
    _portrait(img, 40, 48, 'amico_polpone', frame='#ff93c6')
    _portrait(img, 8, 86, 'amico_caramellone', frame='#8fd8ff')
    # a sideboard under the frame, a vase of flowers on it; a lamp on the right
    sb = rect(84, 172, 300, 194)
    outline(img, sb, INK)
    tone_fill(img, sb, 0.5, ['#8a5a30', '#a8703e', '#c98f58'])
    for x in range(92, 300, 52):
        door = rect(x, 176, x + 44, 192)
        outline(img, door, '#6e4526')
        paint(img, rect(x + 20, 183, x + 23, 184), '#ffd23f')
    lamp_base = rect(304, 120, 308, 196)
    paint(img, lamp_base, '#8a5a30')
    shade = poly_mask(W, H, [(294, 104), (318, 104), (314, 88), (298, 88)])
    outline(img, shade, INK)
    tone_fill(img, shade, 0.6, ['#ffb4d8', '#ffd6ec'])
    glow(img, 306, 110, 30, 26, '#fff6c8', a=0.4)
    return img


# ------------------------------------------------------------------ La storia in ordine: the picture book
BOOK = [(111, 86), (191, 86), (271, 86)]   # the middle of the three pages, where the pictures go (2x)


def libro_bg():
    img = _canvas()
    # a bedroom at night: deep blue wallpaper with stars, a window with the moon
    tone_fill(img, YY < 158, 0.2 + (YY / 158.0) * 0.5, ['#22184a', '#2a1e5a', '#34266a', '#3e2e78'])
    rng = np.random.default_rng(3)
    for _ in range(60):
        x, y = int(rng.integers(0, W)), int(rng.integers(0, 150))
        paint(img, rect(x, y, x, y), ['#ffd23f', '#fff4b0', '#b4a0ff'][int(rng.integers(0, 3))])
    win = rect(6, 14, 62, 76)
    outline(img, win, '#1a1238')
    B.gradient(img, 15, 76, ['#0d0a2e', '#1a1450', '#2a1e6a'], x0=7, x1=62)
    m = ellipse(44, 34, 10, 10) & ~ellipse(48, 30, 9, 9)
    paint(img, m, '#fff4b0')
    for (x, y) in ((16, 26), (24, 50), (52, 60), (14, 64)):
        paint(img, rect(x, y, x, y), '#ffffff')
    paint(img, rect(33, 14, 34, 76) | rect(6, 44, 62, 45), '#8a7ab0')
    paint(img, rect(2, 76, 66, 80), '#8a7ab0')
    # the warm light of the bedside lamp falls on the book
    glow(img, 191, 84, 150, 80, '#ffd88a', a=0.3)
    # the quilt of the bed, at the bottom
    quilt = YY >= 150 + 3 * np.sin(XX / 18.0)
    k = ((XX // 20) + (YY // 20)) % 2
    tone_fill(img, quilt, 0.4 + k * 0.3 + (YY - 150) / 90.0 * 0.2, ['#c45a9a', '#ff7ab8', '#ff93c6', '#ffb4d8'])
    paint(img, quilt & (((XX % 20) == 0) | ((YY % 20) == 0)), '#ffd6ec')
    # the picture book: three pages that unfold like an accordion, on the quilt
    pages = []
    for i, (cx, cy) in enumerate(BOOK):
        x0, x1 = cx - 40, cx + 39
        pg = rect(x0, 30 + (i % 2) * 2, x1, 142 - (i % 2) * 2)
        pages.append(pg)
        outline(img, pg, INK)
        lit = i != 1   # the middle page turns away a little: a shade darker
        tone_fill(img, pg, (0.75 if lit else 0.45) + (XX - x0) / 80.0 * 0.15, ['#e8d8c0', '#f4e8d4', '#fff6e4', '#fffaf0'])
        paint(img, rect(x0 + 3, cy + 41, x1 - 3, cy + 41), '#e8d0b0')   # a line under the picture
        paint(img, ellipse(cx + 0.5, 138 - (i % 2) * 2, 30, 2) & pg, '#ecdcc4')
    for x in (151, 231):   # the folds
        paint(img, rect(x - 1, 32, x, 140), '#c8b090')
    paint(img, rect(71, 143, 311, 145) & ~quilt, '#3e2e78')   # the shadow of the book
    # a soft toy on the quilt, by the book
    bunny = lobe(img, 302, 168, 10, 9, ['#d8d0e8', '#eee8f6', '#ffffff'])
    head = lobe(img, 302, 154, 7, 6.5, ['#d8d0e8', '#eee8f6', '#ffffff'])
    ears = ellipse(298, 142, 2.2, 7) | ellipse(306, 142, 2.2, 7)
    paint(img, ears, '#eee8f6')
    paint(img, ellipse(298, 143, 1, 4) | ellipse(306, 143, 1, 4), '#ffb4d8')
    outline(img, bunny | head | ears, INK)
    for ex in (299, 305):
        paint(img, rect(ex, 153, ex, 154), INK)
    return img


# ------------------------------------------------------------------ what this module gives
def all_backgrounds():
    return {'bg_bacheca': prato_bg(), 'bg_pappagallo': pappagallo_bg(), 'bg_trenino': trenino_bg(),
            'bg_insegna': insegna_bg(), 'bg_cameretta': cameretta_bg(), 'bg_salotto': salotto_bg(),
            'bg_libro': libro_bg()}


def all_sprites():
    out = {'pappagallo': pappagallo(), 'pappagallo_p': pappagallo(True), 'loco': loco(), 'vagone_sel': vagone(None, True)}
    for i, c in enumerate(WAGON_COLS):
        out['vagone_%d' % i] = vagone(c)
    out['lettera_box'] = U.card(40, 48, 'p', 'W')                         # a letter at 2x: 28x40
    out['lettera_box_sel'] = U.card(40, 48, 'h', 'w', frame_w=3, glow='Y')
    return out
