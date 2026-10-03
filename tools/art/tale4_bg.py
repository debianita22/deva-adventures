"""Backgrounds of the fourth tale, "i giocattoli del Re Capriccio" (0.11.0):
the map of the Regno dei Giocattoli - a play mat with wooden train tracks
from place to place - lively, and grey and still under the king's spell; and
the five places: the toy factory, the bowling alley, the castle of building
blocks, the merry-go-round (the party at the end) and the throne room of the
king's toy castle (the last duel).

The map yields its anchors with the prefix "m4_": where Deva stands at each
place (m4_loc0..5, on the tracks: the toy train runs along them) and the
circle that moves again when the place is freed (m4_reg1..5, m4_rad1..5).

Same layout as the other places: the sky and the far things above y ~150, the
ground from y ~196 (Deva's feet at x ~72, the monster's at x ~238, y ~214),
the middle of the screen calm.
"""
import numpy as np

from backgrounds import W, H, canvas, gradient, poly_mask, stars, hexrgb
from pixel import dilate4, rgb
from scenery import (YY, XX, LIGHT_UL, tone_fill, lobe, ellipse, rect, outline, paint, glow, vfade, speckle,
                     line, cloud, puff_row)

INK = '#3b1f4a'
WOODS = ['#7a4a26', '#a8703e', '#c98f58', '#e0ad76']
BLOCKS = {   # dark, mid, light (top face)
    'r': ('#c21f45', '#e5395a', '#ff7a8a'),
    'y': ('#d99a1c', '#ffc93a', '#ffe68a'),
    'b': ('#3a74c8', '#5a9cf0', '#9ccaff'),
    'g': ('#2a8a45', '#4cc25a', '#94e08a'),
    'p': ('#d0508e', '#ff7ab8', '#ffc0dc'),
    'v': ('#7d3fc4', '#a06ae0', '#cfa8f8'),
    'o': ('#d0601a', '#ff8a3a', '#ffbe80'),
}

# the stops of the tracks (Deva's feet, the monsters' feet) and the regions that move again
LOCS4 = [(28, 214), (150, 214), (262, 160), (146, 140), (52, 96), (226, 86)]
REGS4 = [None, (138, 192, 54), (272, 154, 52), (176, 118, 50), (84, 70, 52), (252, 52, 62)]


# ------------------------------------------------------------------ pieces
def block(img, x, y, s, col, ink=INK, depth=None):
    """A toy building block seen a little from above: the front face (s x s),
    the top face and the right side. (x, y) is the top left of the front face."""
    d = depth if depth is not None else max(2, s // 3)
    dk, md, lt = BLOCKS[col]
    front = rect(x, y, x + s - 1, y + s - 1)
    top = poly_mask([(x, y), (x + s, y), (x + s + d, y - d), (x + d, y - d)])
    side = poly_mask([(x + s, y), (x + s + d, y - d), (x + s + d, y + s - d), (x + s, y + s)])
    outline(img, front | top | side, ink)
    paint(img, front, md)
    paint(img, top, lt)
    paint(img, side, dk)
    paint(img, rect(x + 1, y + 1, x + s - 2, y + 1) & front, lt)   # a soft bevel
    paint(img, rect(x + 1, y + 1, x + 1, y + s - 2) & front, lt)
    paint(img, front & ((XX == x + s - 1) | (YY == y + s - 1)), dk)
    return front | top | side


def toy_tree(img, cx, base, r, cols=('#2a8a45', '#4cc25a', '#94e08a', '#d8ffcc'), ink=INK):
    """A wooden toy tree: a round crown on a peg trunk, on a round stand."""
    stand = ellipse(cx, base, r * 0.75, r * 0.28)
    trunk = rect(cx - max(1, r * 0.16), base - r * 1.1, cx + max(1, r * 0.16), base)
    crown = ellipse(cx, base - r * 1.25, r, r * 0.95)
    outline(img, stand | trunk | crown, ink)
    tone_fill(img, stand, 0.5 + (XX - cx) / (r * 1.5) * -0.4, WOODS[1:])
    tone_fill(img, trunk, 0.6 - (XX - cx) / (r * 0.3) * 0.25, WOODS)
    lobe(img, cx, base - r * 1.25, r, r * 0.95, list(cols), L=LIGHT_UL, gamma=0.9)
    paint(img, ellipse(cx - r * 0.4, base - r * 1.6, r * 0.18, r * 0.14), '#ffffff')


def track(img, pts, ties='#a8703e', tie_dark='#7a4a26', rail='#e0ad76', rail_dark='#8a5a30'):
    """Wooden train tracks along a polyline (centre line at the wheels): ties, then two rails."""
    segs = list(zip(pts[:-1], pts[1:]))
    for (x0, y0), (x1, y1) in segs:   # the ties, every 6 px, across the line
        L = np.hypot(x1 - x0, y1 - y0)
        ux, uy = (x1 - x0) / L, (y1 - y0) / L
        nx, ny = -uy, ux
        for k in range(int(L // 6) + 1):
            cx, cy = x0 + ux * (k * 6 + 3), y0 + uy * (k * 6 + 3)
            if k * 6 + 3 > L:
                break
            tie = poly_mask([(cx - nx * 5 - ux * 1.2, cy - ny * 4 - uy * 1.2), (cx + nx * 5 - ux * 1.2, cy + ny * 4 - uy * 1.2),
                             (cx + nx * 5 + ux * 1.2, cy + ny * 4 + uy * 1.2), (cx - nx * 5 + ux * 1.2, cy - ny * 4 + uy * 1.2)])
            outline(img, tie, tie_dark)
            paint(img, tie, ties)
    for (x0, y0), (x1, y1) in segs:   # the rails
        L = np.hypot(x1 - x0, y1 - y0)
        ux, uy = (x1 - x0) / L, (y1 - y0) / L
        nx, ny = -uy, ux
        for side in (-1, 1):
            off = 2.6 * side
            a = (x0 + nx * off, y0 + ny * off * 0.8)
            b = (x1 + nx * off, y1 + ny * off * 0.8)
            line(img, [a, b], rail_dark, 0.9)
    for (x0, y0), (x1, y1) in segs:
        L = np.hypot(x1 - x0, y1 - y0)
        ux, uy = (x1 - x0) / L, (y1 - y0) / L
        nx, ny = -uy, ux
        for side in (-1, 1):
            off = 2.6 * side
            a = (x0 + nx * off, y0 + ny * off * 0.8 - 0.5)
            b = (x1 + nx * off, y1 + ny * off * 0.8 - 0.5)
            line(img, [a, b], rail, 0.4)


def stitches(img, mask, c='#ffffff', every=4):
    """A dashed seam just inside the edge of a patch (a play mat is sewn)."""
    inner = mask & ~dilate4(~mask)
    edge = inner & ~(~dilate4(~inner))
    seam = edge & (((XX + YY) // 2) % every < 2)
    paint(img, seam, c)


def toy_sun(img, cx, cy, r):
    glow(img, cx, cy, r * 2.6, r * 2.6, '#fff3a6', 0.3)
    for k in range(10):   # wooden rays, like a puzzle piece
        a = k * np.pi / 5
        ray = poly_mask([(cx + np.cos(a - 0.16) * r * 1.1, cy + np.sin(a - 0.16) * r * 1.1),
                         (cx + np.cos(a) * r * 1.6, cy + np.sin(a) * r * 1.6),
                         (cx + np.cos(a + 0.16) * r * 1.1, cy + np.sin(a + 0.16) * r * 1.1)])
        outline(img, ray, '#d99a1c')
        paint(img, ray, '#ffc93a')
    m = lobe(img, cx, cy, r, r, ['#f0a030', '#ffc93a', '#ffe68a', '#fff6cf'], L=(-0.4, -0.5, 0.8), lo=0.2)
    outline(img, m, '#d99a1c')
    for ex in (cx - r * 0.38, cx + r * 0.38):   # a happy face
        paint(img, rect(ex - 1, cy - 2, ex, cy - 1), '#8a5a30')
    paint(img, rect(cx - 2, cy + 2, cx + 1, cy + 2) | rect(cx - 3, cy + 1, cx - 3, cy + 1) |
          rect(cx + 2, cy + 1, cx + 2, cy + 1), '#8a5a30')
    paint(img, rect(cx - r * 0.7, cy + 1, cx - r * 0.5, cy + 1) | rect(cx + r * 0.5, cy + 1, cx + r * 0.7, cy + 1), '#ff93c6')


def kite(img, cx, cy, s, cols=('#ff4d6d', '#ffd23f', '#74b8ff', '#4cc25a')):
    pts = [(cx, cy - s), (cx + s * 0.7, cy), (cx, cy + s * 1.2), (cx - s * 0.7, cy)]
    m = poly_mask(pts)
    outline(img, m, INK)
    for k, (a, b) in enumerate(zip(pts, pts[1:] + pts[:1])):
        tri = poly_mask([(cx, cy), a, b])
        paint(img, tri & m, cols[k])
    tail = [(cx, cy + s * 1.2)] + [(cx - 4 - k * 5, cy + s * 1.2 + 4 + 3 * np.sin(k * 1.4)) for k in range(6)]
    line(img, tail, INK, 0.4)
    for k in range(1, 6, 2):
        x, y = tail[k]
        paint(img, poly_mask([(x - 2, y - 1.5), (x + 2, y + 1.5), (x + 2, y - 1.5), (x - 2, y + 1.5)]), cols[k % 4])


def balloon(img, cx, cy, r, cols=('#e5395a', '#fff4fa')):
    """A hot-air balloon in stripes, with its little basket."""
    env = ellipse(cx, cy, r, r * 1.1) | poly_mask([(cx - r * 0.75, cy + r * 0.6), (cx + r * 0.75, cy + r * 0.6),
                                                   (cx + r * 0.3, cy + r * 1.45), (cx - r * 0.3, cy + r * 1.45)])
    outline(img, env, INK)
    stripes = (np.floor((XX + 0.5 - cx) / max(2.0, r * 0.45)) % 2) == 0
    paint(img, env & stripes, cols[0])
    paint(img, env & ~stripes, cols[1])
    glow(img, cx - r * 0.4, cy - r * 0.5, r * 0.5, r * 0.5, '#ffffff', 0.4, mask=env)
    by = cy + r * 1.45 + 3
    for sx in (-1, 1):
        line(img, [(cx + sx * r * 0.3, cy + r * 1.45), (cx + sx * 2, by)], INK, 0.3)
    bask = rect(cx - 3, by, cx + 3, by + 4)
    outline(img, bask, INK)
    paint(img, bask, '#c98f58')


def pin(img, cx, base, h, ink=INK):
    """A bowling pin: white, two red stripes on the neck."""
    w = h * 0.36
    body = ellipse(cx, base - h * 0.3, w, h * 0.3) | ellipse(cx, base - h * 0.82, w * 0.55, h * 0.18) | \
        rect(cx - w * 0.38, base - h * 0.78, cx + w * 0.38, base - h * 0.45)
    body &= YY <= base
    outline(img, body, ink)
    tone_fill(img, body, 0.75 - (XX - cx) / (w * 2.2), ['#c8bcd8', '#e4dcef', '#fff4fa', '#ffffff'])
    for y in (base - h * 0.66, base - h * 0.58):
        paint(img, body & (YY >= y) & (YY < y + max(1, h * 0.05)), '#e5395a')
    return body


def bowling_ball(img, cx, cy, r, tones=('#3a1e6a', '#5a3aa8', '#7d5fd0', '#b8a0f0')):
    m = lobe(img, cx, cy, r, r, list(tones), L=LIGHT_UL)
    outline(img, m, INK)
    for (dx, dy) in ((-0.25, -0.3), (0.12, -0.38), (-0.05, -0.05)):
        paint(img, ellipse(cx + dx * r * 1.4, cy + dy * r * 1.4, max(0.8, r * 0.13), max(0.8, r * 0.13)), tones[0])
    return m


def factory(img, x0, base, w, h):
    """A toy factory: a saw-tooth roof, round windows, a gear, a chimney with heart puffs."""
    walls = rect(x0, base - h, x0 + w, base)
    teeth = np.zeros((H, W), bool)
    n = 3
    tw = w / n
    for k in range(n):
        teeth |= poly_mask([(x0 + k * tw, base - h), (x0 + (k + 1) * tw, base - h),
                            (x0 + (k + 1) * tw, base - h - tw * 0.55)])
    chim = rect(x0 + w - 10, base - h - 22, x0 + w - 4, base - h)
    outline(img, walls | teeth | chim, INK)
    tone_fill(img, walls, 0.55 - (XX - x0) / w * 0.2, ['#c84a6a', '#e5627e', '#ff8aa0'])
    for y in range(int(base - h) + 4, int(base), 5):   # bricks
        paint(img, walls & (YY == y), '#c84a6a')
    tone_fill(img, teeth, 0.6 + (YY - (base - h)) / 20.0, ['#3a74c8', '#5a9cf0', '#9ccaff'])
    paint(img, chim, '#7a76a0')
    paint(img, chim & (YY < base - h - 18), '#e5395a')
    for k in range(3):   # heart puffs
        hx, hy = x0 + w - 7 + k * 5, base - h - 28 - k * 9
        heart = ellipse(hx - 1.6, hy, 2, 2) | ellipse(hx + 1.6, hy, 2, 2) | poly_mask([(hx - 3.5, hy + 0.5), (hx + 3.5, hy + 0.5), (hx, hy + 4)])
        outline(img, heart, '#c8bcd8')
        paint(img, heart, '#ffffff' if k else '#ffe0ec')
    for k in range(2):   # round windows
        wx = x0 + 9 + k * 15
        win = ellipse(wx, base - h * 0.62, 4, 4)
        outline(img, win, INK)
        paint(img, win, '#fff3a6')
        paint(img, rect(wx - 3, base - h * 0.62, wx + 3, base - h * 0.62) & win, '#f0a030')
    door = rect(x0 + w * 0.36, base - 12, x0 + w * 0.62, base) | ellipse(x0 + w * 0.49, base - 12, w * 0.13, 3)
    outline(img, door, INK)
    paint(img, door, '#7a4a26')
    gx, gy = x0 + w - 9, base - h * 0.35   # the big gear on the wall
    gear = ellipse(gx, gy, 5, 5)
    for k in range(8):
        a = k * np.pi / 4
        gear |= ellipse(gx + np.cos(a) * 6, gy + np.sin(a) * 6, 1.5, 1.5)
    outline(img, gear, INK)
    paint(img, gear, '#ffd23f')
    paint(img, ellipse(gx, gy, 1.8, 1.8), '#d99a1c')


def carousel(img, cx, base, r, h, ink=INK, lit=True):
    """A merry-go-round: a striped cone roof, a rim of bulbs, golden poles with horses, the round deck."""
    deck = ellipse(cx, base - 2, r, r * 0.3)
    deck_side = rect(cx - r, base - 2, cx + r, base + 2) & ~ellipse(cx, base - 6, r, r * 0.3) | ellipse(cx, base + 2, r, r * 0.3)
    roof_y = base - h
    roof = poly_mask([(cx - r - 2, roof_y + h * 0.22), (cx, roof_y - h * 0.12), (cx + r + 2, roof_y + h * 0.22)])
    rim = rect(cx - r - 2, roof_y + h * 0.22, cx + r + 2, roof_y + h * 0.3)
    flag = poly_mask([(cx, roof_y - h * 0.12 - 7), (cx + 6, roof_y - h * 0.12 - 5), (cx, roof_y - h * 0.12 - 3)])
    outline(img, deck | deck_side | roof | rim | flag, ink)
    line(img, [(cx, roof_y - h * 0.12), (cx, roof_y - h * 0.12 - 7)], ink, 0.3)
    paint(img, flag, '#ff4d6d')
    tone_fill(img, deck_side, 0.5, ['#c84a6a', '#e5627e'])
    tone_fill(img, deck, 0.6, ['#e0ad76', '#f2cc98'])
    # poles and horses
    for k, dx in enumerate((-0.62, -0.2, 0.22, 0.64)):
        px_ = cx + dx * r
        line(img, [(px_, roof_y + h * 0.3), (px_, base - 3)], '#d99a1c', 0.5)
        hy = base - h * (0.42 if k % 2 else 0.55)
        horse = ellipse(px_, hy, 5, 3) | ellipse(px_ + (4 if dx < 0.1 else -4), hy - 3, 2.4, 2.2) | \
            rect(px_ - 4, hy + 1, px_ - 3, hy + 5) | rect(px_ + 3, hy + 1, px_ + 4, hy + 5)
        outline(img, horse, ink)
        paint(img, horse, ['#ffffff', '#ffcbe3', '#c4e6ff', '#fff3a6'][k])
        paint(img, ellipse(px_, hy - 1, 2.4, 1.4), ['#ff93c6', '#b376ec', '#4fd6c0', '#ff7b2e'][k])
    stripes = (np.floor(np.arctan2(YY - (roof_y - h * 0.12), XX + 0.5 - cx) * 6) % 2) == 0
    paint(img, roof & stripes, '#ff93c6')
    paint(img, roof & ~stripes, '#fff4fa')
    paint(img, rim, '#ffd23f')
    paint(img, rim & (YY == roof_y + h * 0.3), '#f0a030')
    for x in np.arange(cx - r + 1, cx + r, 5):   # the bulbs
        b = ellipse(x, roof_y + h * 0.26, 1.2, 1.2)
        paint(img, b, '#fff3a6' if lit else '#9a96aa')
        if lit:
            glow(img, x, roof_y + h * 0.26, 4, 4, '#fff3a6', 0.25)


PASTEL = {   # the king's castle: soft blocks, so that the king stands out in front of it
    'p': ('#e8a8c8', '#ffcce2', '#ffe8f2'),
    'b': ('#a8c4ec', '#cce0ff', '#e8f2ff'),
    'y': ('#e8cc88', '#fff0b8', '#fffae0'),
    'v': ('#c4b0e8', '#e0d4ff', '#f2ecff'),
}


def soft_block(img, x, y, s, col, ink='#8a6aa8'):
    dk, md, lt = PASTEL[col]
    d = max(2, s // 4)
    front = rect(x, y, x + s - 1, y + s - 1)
    top = poly_mask([(x, y), (x + s, y), (x + s + d, y - d), (x + d, y - d)])
    side = poly_mask([(x + s, y), (x + s + d, y - d), (x + s + d, y + s - d), (x + s, y + s)])
    outline(img, front | top | side, ink)
    paint(img, front, md)
    paint(img, top, lt)
    paint(img, side, dk)
    paint(img, front & ((XX == x + s - 1) | (YY == y + s - 1)), dk)


def toy_castle(img, x0, x1, base, top, ink=INK, gate=True):
    """The king's castle made of big soft toy blocks: two towers with flags, a wall, a gate, a crown."""
    s = 13
    cols = ['p', 'b', 'y', 'v']
    for t, tx in enumerate((x0, x1 - 2 * s)):   # towers
        y, k = base - s, 0
        while y >= top + 8:
            for i in range(2):
                soft_block(img, tx + i * s, y, s, cols[(k + i + t) % 4])
            y -= s
            k += 1
        for i in range(2):   # crenellations
            soft_block(img, tx + i * s + 2, y + 4, s - 5, cols[(k + i + 1) % 4])
        fx = tx + s
        line(img, [(fx, y + 4), (fx, y - 8)], ink, 0.4)
        pen = poly_mask([(fx, y - 8), (fx + 10, y - 5), (fx, y - 2)])
        outline(img, pen, ink)
        paint(img, pen, '#ff4d6d' if t == 0 else '#74b8ff')
    y, k = base - s, 0   # the wall between
    while y >= top + 30:
        for i in range(int((x1 - x0 - 4 * s) // s)):
            soft_block(img, x0 + 2 * s + i * s, y, s, cols[(k + i) % 4])
        y -= s
        k += 1
    if gate:
        gx = (x0 + x1) / 2
        g = rect(gx - 10, base - 20, gx + 10, base) | ellipse(gx, base - 20, 10, 8)
        outline(img, g, ink)
        tone_fill(img, g, 0.5 + (YY - (base - 26)) / 40.0, ['#8a5a30', '#a8703e', '#c98f58'])
        paint(img, g & (XX == int(gx)), '#7a4a26')
    ccx, ccy = (x0 + x1) / 2, y + 2   # a big golden crown on the wall: the king's sign
    crown = poly_mask([(ccx - 12, ccy + 6), (ccx + 12, ccy + 6), (ccx + 13, ccy - 6), (ccx + 6, ccy - 1), (ccx, ccy - 8),
                       (ccx - 6, ccy - 1), (ccx - 13, ccy - 6)])
    outline(img, crown, ink)
    tone_fill(img, crown, 0.6 - (YY - ccy) / 20.0, ['#d99a1c', '#ffc93a', '#ffe68a'])
    for (x, c) in ((ccx - 6, '#e5395a'), (ccx, '#5a9cf0'), (ccx + 6, '#4cc25a')):
        paint(img, ellipse(x, ccy + 2, 1.6, 1.6), c)


def flowers(img, n, area, seed, cols=('#ffcbe3', '#fff3a6', '#ffffff', '#c4b0ff'), avoid=None):
    rng = np.random.default_rng(seed)
    x0, y0, x1, y1 = area
    for _ in range(n):
        x, y = int(rng.integers(x0, x1)), int(rng.integers(y0, y1))
        if avoid is not None and avoid[min(y, H - 1), min(x, W - 1)]:
            continue
        c = cols[int(rng.integers(0, len(cols)))]
        paint(img, rect(x - 1, y, x + 1, y) | rect(x, y - 1, x, y + 1), c)
        img[y, x, :3] = rgb('#ffd23f')


# ------------------------------------------------------------------ the map
def mappa4():
    img = canvas()
    gradient(img, 0, 44, ['#9ccaff', '#b6d8ff', '#d0e6ff', '#fff0e0'])
    cloud(img, puff_row(4, 70, 14, 6, 3), ['#d8e8ff', '#eef5ff', '#ffffff'])
    cloud(img, puff_row(150, 196, 10, 5, 4), ['#d8e8ff', '#eef5ff', '#ffffff'])
    toy_sun(img, 22, 16, 7)
    balloon(img, 182, 14, 5, ('#4fd6c0', '#fff4fa'))
    kite(img, 124, 14, 6)
    # felt hills at the horizon
    far = poly_mask([(0, 50)] + [(x, 36 + 5 * np.sin(x / 22.0) + 3 * np.sin(x / 7.0)) for x in range(0, W + 8, 8)]
                    + [(W, 50)])
    tone_fill(img, far, 0.55 + (YY - 36) / 16.0 * 0.3, ['#8cc8a0', '#a2d8b0', '#b8e8c0'])
    # the play mat: quilted patches of felt with seams
    gradient(img, 44, H, ['#b4e69c', '#a8de90', '#9cd486'])
    mat = YY >= 44
    pcols = ['#bde9a6', '#a6dc94', '#c6eeb4', '#b0e29e', '#d4f2c2', '#9ed88e']
    for j, y0 in enumerate(range(44, H, 34)):
        for i, x0 in enumerate(range(-((j % 2) * 20), W, 40)):
            patch = rect(x0 + 1, y0 + 1, x0 + 38, y0 + 32) & mat
            paint(img, patch, pcols[(i + j * 2) % len(pcols)])
            stitches(img, patch, '#f2fbe8')
    speckle(img, mat, '#d8f4c8', 0.015, 41)
    # the pond with a paper boat and a rubber duck
    pond = ellipse(208, 166, 18, 7)
    outline(img, pond, '#3a74c8')
    tone_fill(img, pond, 0.35 + (YY - 166) / 7 * 0.35, ['#9ccaff', '#74b8ff', '#5a9cf0'][::-1])
    paint(img, pond & (((XX * 5 + YY * 11) % 23) == 0), '#dff2ff')
    boat = poly_mask([(196, 165), (208, 165), (205, 169), (199, 169)]) | poly_mask([(201, 165), (201, 158), (206, 165)])
    outline(img, boat, INK)
    paint(img, boat, '#fff4fa')
    duck = ellipse(217, 167, 4, 2.6) | ellipse(220, 163, 2.4, 2.4)
    outline(img, duck, INK)
    paint(img, duck, '#ffd23f')
    paint(img, rect(222, 163, 223, 163), '#ff8a3a')
    # the tracks, from stop to stop (the train runs on them)
    track(img, [(x, y - 1) for (x, y) in LOCS4])
    # the stops: a little round platform at each place
    for (x, y) in LOCS4[1:]:
        st = ellipse(x + 10, y, 14, 3.5)
        outline(img, st, INK)
        tone_fill(img, st, 0.5 + (YY - y) / 5.0 * -0.3, ['#e0ad76', '#f2cc98', '#ffe6c0'])
    # trees here and there
    for (x, b, r, c) in ((16, 150, 7, None), (64, 168, 6, ('#3a74c8', '#5a9cf0', '#9ccaff', '#e0f0ff')),
                         (110, 112, 5, None), (304, 112, 7, ('#d0508e', '#ff7ab8', '#ffc0dc', '#fff0f6')),
                         (246, 228, 6, None), (8, 104, 5, ('#d99a1c', '#ffc93a', '#ffe68a', '#fff6cf'))):
        toy_tree(img, x, b, r, c or ('#2a8a45', '#4cc25a', '#94e08a', '#d8ffcc'))
    flowers(img, 60, (0, 60, W, 236), 7, avoid=pond)
    # 0: home - Deva's little play tent
    tent = poly_mask([(6, 206), (42, 206), (24, 176)])
    outline(img, tent, INK)
    stripes = (np.floor((XX - 24) / 4.0) % 2) == 0
    paint(img, tent & stripes, '#ff93c6')
    paint(img, tent & ~stripes, '#fff4fa')
    paint(img, poly_mask([(19, 206), (29, 206), (24, 192)]), '#c84a6a')
    # 1: the toy factory
    factory(img, 84, 206, 40, 30)
    # 2: the bowling green: a lane with the pins and a ball
    lane = poly_mask([(268, 208), (318, 208), (308, 176), (278, 176)])
    wood = poly_mask([(272, 208), (314, 208), (305, 176), (281, 176)])
    outline(img, lane, INK)
    tone_fill(img, lane & ~wood, 0.5, ['#5a4a80', '#6a5a90'])   # the gutters
    tone_fill(img, wood, 0.35 + (YY - 176) / 32.0 * 0.5, ['#c98f58', '#e0ad76', '#f2cc98'])
    for k in (1, 2, 3):
        line(img, [(281 + k * 6, 176), (272 + k * 10.5, 208)], '#c08050', 0.3, mask=wood)
    paint(img, poly_mask([(290, 200), (293, 194), (296, 200)]), '#e5395a')   # the arrow
    for (px_, b) in ((293, 188), (288, 184), (298, 184), (283, 180), (293, 180), (303, 180)):
        pin(img, px_, b, 11)
    bowling_ball(img, 283, 202, 5)
    # 3: the castle of building blocks
    for (bx, by, c) in ((196, 128, 'r'), (206, 128, 'y'), (216, 128, 'b'), (226, 128, 'g'), (196, 118, 'b'),
                        (226, 118, 'p'), (196, 108, 'g'), (226, 108, 'y'), (206, 118, 'v'), (216, 118, 'o')):
        block(img, bx, by, 10, c)
    for (bx, c) in ((197, 'y'), (227, 'r')):
        block(img, bx, 100, 7, c)
    arch = ellipse(216, 132, 4, 5) & (YY <= 137)
    paint(img, arch, '#5a3aa8')
    # 4: the merry-go-round
    carousel(img, 124, 86, 22, 46)
    # 5: the king's castle
    toy_castle(img, 182, 288, 88, 10)
    anchors = {}
    for i, (x, y) in enumerate(LOCS4):
        anchors['m4_loc%d' % i] = (x, y)
    for i in range(1, 6):
        cx, cy, r = REGS4[i]
        anchors['m4_reg%d' % i] = (cx, cy)
        anchors['m4_rad%d' % i] = (r, 0)
    return img, anchors


def ferma(img):
    """Under the king's spell: everything stops, grey and dusty, a little lilac."""
    f = img[..., :3].astype(np.float32)
    lum = f[..., 0] * 0.3 + f[..., 1] * 0.55 + f[..., 2] * 0.15
    g = np.stack([lum] * 3, axis=-1) * 0.62 + f * 0.12
    tint = np.array([0x9a, 0x94, 0xb0], np.float32)
    g = g * 0.74 + tint * 0.28
    out = img.copy()
    out[..., :3] = np.clip(g, 0, 255).astype(np.uint8)
    speckle(out, YY >= 0, '#c8c2d6', 0.02, 77)   # dust
    vfade(out, YY >= 0, '#bdb6cc', 0.12, 0.04, 0, H)
    return out


# ------------------------------------------------------------------ the places
def _floor_tiles(img, y0, c1, c2, line_c, size=20):
    floor = YY >= y0
    rows = (YY - y0) // 11
    k = ((XX + rows * (size // 2)) // size + rows) % 2
    paint(img, floor & (k == 0), c1)
    paint(img, floor & (k == 1), c2)
    paint(img, floor & (((YY - y0) % 11) == 10), line_c)
    paint(img, rect(0, y0, W - 1, y0), INK)


def gear(img, cx, cy, r, teeth, cols, ink=INK):
    m = ellipse(cx, cy, r, r)
    for k in range(teeth):
        a = k * 2 * np.pi / teeth
        m |= ellipse(cx + np.cos(a) * (r + 2), cy + np.sin(a) * (r + 2), max(1.6, r * 0.22), max(1.6, r * 0.22))
    outline(img, m, ink)
    tone_fill(img, m, 0.55 + (-(XX - cx) - (YY - cy)) / (r * 4.0), list(cols))
    hole = ellipse(cx, cy, r * 0.35, r * 0.35)
    outline(img, hole, ink)
    paint(img, hole, cols[0])
    for k in range(4):   # spokes holes
        a = k * np.pi / 2 + np.pi / 4
        paint(img, ellipse(cx + np.cos(a) * r * 0.62, cy + np.sin(a) * r * 0.62, r * 0.14, r * 0.14), cols[0])
    return m


def fabbrica_bg():
    img = canvas()
    # the wall: mint, wooden beams, round windows with the sky
    gradient(img, 0, 196, ['#bfe8dc', '#c8eee2', '#d2f2e8', '#dcf6ee'])
    for x in (0, 108, 214, 318):
        beam = rect(x, 0, x + 6, 196)
        outline(img, beam, INK)
        tone_fill(img, beam, 0.5 + (XX - x) / 6.0 * 0.3, WOODS)
    for (wx, wy) in ((54, 56), (266, 56)):
        win = ellipse(wx, wy, 20, 20)
        glass = ellipse(wx, wy, 16, 16)
        outline(img, win, INK)
        tone_fill(img, win & ~glass, 0.5 + (-(XX - wx) - (YY - wy)) / 40.0, ['#8a5a30', '#a8703e', '#c98f58'])
        tmp = canvas()
        gradient(tmp, wy - 16, wy + 17, ['#8fd0ff', '#afdeff', '#cfeeff'])
        img[glass, :3] = tmp[glass, :3]
        cloud(img, [(wx - 5, wy - 2, 6, 4), (wx + 3, wy - 4, 6, 5), (wx + 9, wy - 1, 4, 3)], ['#e6f2ff', '#ffffff'])
        paint(img, glass & ((XX == wx) | (YY == wy)), '#8a5a30')
    # gears on the wall
    gear(img, 162, 40, 14, 10, ['#d99a1c', '#ffc93a', '#ffe68a'])
    gear(img, 186, 62, 9, 8, ['#3a74c8', '#5a9cf0', '#9ccaff'])
    gear(img, 140, 66, 8, 7, ['#c21f45', '#e5395a', '#ff7a8a'])
    # pipes along the top
    for (y, c) in ((8, ('#c21f45', '#e5395a', '#ff9aaa')), (16, ('#3a74c8', '#5a9cf0', '#bcdcff'))):
        p = rect(0, y, W - 1, y + 4)
        outline(img, p, INK)
        tone_fill(img, p, 0.2 + (YY - y) / 4.0 * -0.6 + 0.8, list(c))
        for x in range(20, W, 60):
            paint(img, rect(x, y - 1, x + 3, y + 5), c[0])
    # the conveyor belt at the back, with toys on it
    belt = rect(0, 148, W - 1, 156)
    outline(img, belt, INK)
    paint(img, belt, '#4a4458')
    paint(img, belt & (YY == 148), '#6b6480')
    for x in range(4, W, 12):
        roller = ellipse(x, 158, 4, 4)
        outline(img, roller, INK)
        tone_fill(img, roller, 0.5 + (-(XX - x) - (YY - 158)) / 10.0, ['#6b6480', '#8e87a3', '#b3adc4'])
    for x in range(0, W, 8):
        paint(img, rect(x, 152, x + 3, 152), '#6b6480')
    legs = np.zeros((H, W), bool)
    for x in range(26, W, 64):
        legs |= rect(x, 162, x + 4, 196)
    outline(img, legs, INK)
    paint(img, legs, '#8e87a3')
    # toys riding the belt: a gift, a ball, a teddy, a block, a little robot head
    for (bx, c) in ((14, 'p'), (230, 'y')):
        block(img, bx, 136, 12, c)
    ball = lobe(img, 98, 141, 7, 7, ['#c21f45', '#e5395a', '#ff7a8a', '#ffffff'], L=LIGHT_UL)
    outline(img, ball, INK)
    paint(img, rect(91, 140, 105, 141) & ball, '#ffffff')
    for (tx, c) in ((150, ('#a8703e', '#c98f58', '#e0ad76')),):   # a teddy
        body = ellipse(tx, 140, 7, 8) | ellipse(tx, 128, 6, 5.5) | ellipse(tx - 5, 123, 2.4, 2.4) | ellipse(tx + 5, 123, 2.4, 2.4)
        outline(img, body, INK)
        tone_fill(img, body, 0.55 + (-(XX - tx) - (YY - 132)) / 30.0, list(c))
        paint(img, ellipse(tx, 130, 2.6, 1.8), '#f2cc98')
        for ex in (tx - 2, tx + 2):
            paint(img, rect(ex, 127, ex, 127), INK)
        paint(img, ellipse(tx, 142, 3.6, 3.6), '#f2cc98')
    rob = rect(282, 132, 296, 147)
    outline(img, rob, INK)
    paint(img, rob, '#9ccaff')
    for ex in (286, 292):
        paint(img, ellipse(ex, 138, 1.8, 1.8), '#ffffff')
        paint(img, rect(ex, 138, ex, 138), INK)
    line(img, [(289, 131), (289, 127)], INK, 0.3)
    paint(img, ellipse(289, 126, 1.5, 1.5), '#e5395a')
    # hanging lamps
    for lx in (80, 240):
        line(img, [(lx, 22), (lx, 30)], INK, 0.3)
        lamp = poly_mask([(lx - 7, 38), (lx + 7, 38), (lx + 4, 30), (lx - 4, 30)])
        outline(img, lamp, INK)
        paint(img, lamp, '#ffd23f')
        glow(img, lx, 44, 22, 16, '#fff3a6', 0.3)
    # the floor: yellow and cream tiles
    _floor_tiles(img, 196, '#ffe8a0', '#fff6d6', '#e6c878')
    vfade(img, YY >= 196, '#d8b060', 0.0, 0.2, 196, H)
    return img


def birilli_bg():
    img = canvas()
    # the back wall: pastel stripes and stars
    stripes = ((XX + 0) // 16) % 2
    tone_fill(img, YY < 112, 0.45 + stripes * 0.3 + (YY / 112.0) * 0.1, ['#e6c6ee', '#f0d6f4', '#f8e6fa'])
    for (x, y) in ((24, 20), (70, 40), (130, 16), (190, 36), (250, 18), (300, 44), (40, 70), (280, 80)):
        st = poly_mask([(x + np.cos(a) * (5 if k % 2 == 0 else 2.2), y + np.sin(a) * (5 if k % 2 == 0 else 2.2))
                        for k, a in enumerate(np.arange(10) * np.pi / 5 - np.pi / 2)])
        outline(img, st, '#d99a1c')
        paint(img, st, '#ffe68a')
    # a strip of lights over the lanes
    bar = rect(0, 92, W - 1, 100)
    outline(img, bar, INK)
    paint(img, bar, '#7d3fc4')
    for x in range(6, W, 12):
        paint(img, ellipse(x, 96, 2, 2), '#fff3a6')
        glow(img, x, 96, 6, 6, '#fff3a6', 0.25)
    # the far end: dark pit behind the pins
    paint(img, rect(0, 101, W - 1, 111), '#3a2466')
    # the lanes, in perspective: from y 112 (far) to the bottom
    vp_x, vp_y = 160, 40
    far_y, near_y = 112, 240
    lanes = [(-1.0, -0.62), (-0.52, -0.14), (-0.04, 0.34), (0.44, 0.82)]
    base_wood = np.zeros((H, W), bool)
    for (a, b) in lanes:
        def xat(t, y):
            return vp_x + t * 260 * (y - vp_y) / (near_y - vp_y)
        lane = poly_mask([(xat(a, far_y), far_y), (xat(b, far_y), far_y), (xat(b, near_y), near_y),
                          (xat(a, near_y), near_y)])
        base_wood |= lane
        tone_fill(img, lane, 0.35 + (YY - far_y) / (near_y - far_y) * 0.55, ['#c98f58', '#d8a26a', '#e6b67e', '#f2cc98'])
        for k in range(1, 6):   # boards
            t = a + (b - a) * k / 6
            pts = [(xat(t, far_y), far_y), (xat(t, near_y), near_y)]
            line(img, pts, '#c08050', 0.3, mask=lane)
        # the arrows on the lane
        y = 176
        cx = xat((a + b) / 2, y)
        arrow = poly_mask([(cx - 3, y + 4), (cx, y - 3), (cx + 3, y + 4)])
        paint(img, arrow, '#e5395a')
        # the pins at the far end of the lane
        cxf = xat((a + b) / 2, far_y)
        for (dx, dy, hgt) in ((0, 10, 12), (-5, 6, 11), (5, 6, 11), (-9, 2, 10), (0, 2, 10), (9, 2, 10)):
            pin(img, cxf + dx, far_y + dy, hgt)
    gutters = (YY >= far_y) & ~base_wood
    tone_fill(img, gutters, 0.3 + (YY - far_y) / (near_y - far_y) * 0.3, ['#4a3a70', '#5a4a80', '#6a5a90'])
    # a rack of balls on the left, a giant pin on the right
    rack = rect(4, 150, 44, 158)
    outline(img, rack, INK)
    paint(img, rack, '#a8703e')
    for (x, tones) in ((12, ('#3a1e6a', '#5a3aa8', '#7d5fd0', '#b8a0f0')), (24, ('#8a1f33', '#c21f45', '#e53935', '#ff7a8a')),
                       (36, ('#1d6f73', '#2fb3a8', '#7ff0dc', '#d8fff4'))):
        bowling_ball(img, x, 144, 6, tones)
    pin(img, 300, 168, 44)
    paint(img, rect(0, 196, W - 1, 196) & ~base_wood, INK)
    return img


def cubi_bg():
    img = canvas()
    gradient(img, 0, 196, ['#9ccaff', '#b6d8ff', '#d0e6ff', '#ffeaf2'])
    cloud(img, puff_row(10, 110, 30, 9, 5), ['#d8e8ff', '#eef5ff', '#ffffff'])
    cloud(img, puff_row(210, 310, 22, 8, 6), ['#d8e8ff', '#eef5ff', '#ffffff'])
    toy_sun(img, 288, 56, 9)
    hills = poly_mask([(0, 196)] + [(x, 170 + 6 * np.sin(x / 30.0) + 3 * np.sin(x / 11.0)) for x in range(0, W + 8, 8)]
                      + [(W, 196)])
    tone_fill(img, hills, 0.5 + (YY - 170) / 26.0 * 0.3, ['#8cc8a0', '#a2d8b0', '#b8e8c0'])
    before = img.copy()
    # the castle of blocks at the back
    cols = ['r', 'y', 'b', 'g', 'p', 'v', 'o']
    s = 14
    rng = np.random.default_rng(3)
    for (x0, x1, top) in ((40, 96, 84), (228, 284, 84)):   # two towers
        for y in range(196 - s, top - 1, -s):
            for x in range(x0, x1, s):
                block(img, x, y, s, cols[int(rng.integers(0, len(cols)))], depth=4)
        for x in range(x0, x1, 2 * s):
            block(img, x + 2, top - s + 4, s - 4, cols[int(rng.integers(0, len(cols)))], depth=3)
    for y in range(196 - s, 125, -s):   # the wall between them
        for x in range(96, 228, s):
            block(img, x, y, s, cols[int(rng.integers(0, len(cols)))], depth=4)
    for x in range(98, 228, 2 * s):
        block(img, x + 2, 126 - s + 6, s - 4, cols[int(rng.integers(0, len(cols)))], depth=3)
    arch = rect(146, 160, 178, 196) | ellipse(162, 160, 16, 12)
    outline(img, arch, INK)
    tone_fill(img, arch, 0.4 + (YY - 150) / 46.0 * 0.3, ['#3a2466', '#4a3a80', '#5a4a90'])
    for (fx, c) in ((68, '#ff4d6d'), (256, '#74b8ff')):   # flags on the towers
        line(img, [(fx, 70), (fx, 52)], INK, 0.4)
        pen = poly_mask([(fx, 52), (fx + 12, 56), (fx, 60)])
        outline(img, pen, INK)
        paint(img, pen, c)
    castle = np.any(img[..., :3] != before[..., :3], axis=-1)
    vfade(img, castle, '#f4ecff', 0.3, 0.3, 0, H)   # a little haze: the castle stays behind the characters
    # the floor: a play mat with seams
    gradient(img, 196, H, ['#a8de90', '#9cd486'])
    for i, x0 in enumerate(range(-10, W, 40)):
        patch = rect(x0 + 1, 197, x0 + 38, H - 1)
        paint(img, patch, ['#bde9a6', '#a6dc94', '#c6eeb4', '#b0e29e'][i % 4])
        stitches(img, patch, '#f2fbe8')
    paint(img, rect(0, 196, W - 1, 196), INK)
    # loose blocks on the floor
    for (x, y, sz, c) in ((8, 204, 12, 'y'), (22, 210, 10, 'b'), (290, 206, 12, 'r'), (304, 214, 9, 'g'),
                          (130, 222, 9, 'p')):
        block(img, x, y, sz, c)
    return img


def giostra_bg():
    img = canvas()
    # evening: a lilac sky going peach, the first stars
    gradient(img, 0, 180, ['#5a4a9a', '#7a5aa8', '#a46ab0', '#d88aa8', '#f4b8a0'])
    rng = np.random.default_rng(9)
    stars(img, rng, 40, (0, 0, W, 80))
    cloud(img, puff_row(0, 70, 96, 9, 7), ['#c08ab8', '#d8a0c4', '#eab8d0'])
    cloud(img, puff_row(250, 320, 104, 9, 8), ['#c08ab8', '#d8a0c4', '#eab8d0'])
    # strings of bunting across the sky
    for (x0, y0, x1, y1) in ((0, 30, 110, 40), (210, 40, 320, 28)):
        line(img, [(x0, y0), ((x0 + x1) / 2, max(y0, y1) + 8), (x1, y1)], INK, 0.3)
        for k in range(8):
            t = (k + 0.5) / 8
            x = x0 + (x1 - x0) * t
            y = y0 + (y1 - y0) * t + 8 * np.sin(np.pi * t)
            f = poly_mask([(x - 4, y), (x + 4, y), (x, y + 7)])
            outline(img, f, INK)
            paint(img, f, ['#ff93c6', '#ffd23f', '#74b8ff', '#4cc25a', '#b376ec'][k % 5])
    # the big merry-go-round in the middle
    carousel(img, 160, 178, 74, 150)
    # lamp posts
    for lx in (26, 294):
        post = rect(lx - 1, 120, lx + 1, 196)
        outline(img, post, INK)
        paint(img, post, '#3a2466')
        lamp = ellipse(lx, 116, 5, 6)
        outline(img, lamp, INK)
        paint(img, lamp, '#fff3a6')
        glow(img, lx, 116, 22, 22, '#fff3a6', 0.35)
    # balloons tied to the posts
    for (bx, by, c, d) in ((14, 92, '#e5395a', '#ff9aaa'), (36, 86, '#5a9cf0', '#bcdcff'), (284, 90, '#ffc93a', '#fff3a6'),
                           (306, 84, '#4cc25a', '#bfffb0')):
        b = ellipse(bx, by, 6, 7.5)
        outline(img, b, INK)
        paint(img, b, c)
        paint(img, ellipse(bx - 2, by - 3, 1.6, 2), d)
        line(img, [(bx, by + 8), (26 if bx < 160 else 294, 118)], INK, 0.3)
    # the square: peach tiles
    _floor_tiles(img, 196, '#f6c6a8', '#fcdcc4', '#e0a888', size=24)
    vfade(img, YY >= 196, '#7a5aa8', 0.0, 0.25, 196, H)
    return img


def castello_giochi_bg():
    img = canvas()
    # the walls: big pastel toy blocks
    wall = YY < 196
    cols = ['#ffd6e8', '#d6e8ff', '#fff0c0', '#e0d6ff', '#d6f6e0']
    for j, y0 in enumerate(range(0, 196, 24)):
        off = (j % 2) * 24
        for i, x0 in enumerate(range(-off, W, 48)):
            b = rect(x0 + 1, y0 + 1, x0 + 46, y0 + 22) & wall
            paint(img, b, cols[(i + j * 3) % len(cols)])
            paint(img, b & (YY == y0 + 1), '#ffffff')
            paint(img, b & (YY == y0 + 22), '#c8bcd8')
        paint(img, wall & (YY == y0), '#b8acd0')
    # the big arched window with the sky
    win = rect(128, 22, 192, 96) | ellipse(160, 22, 32, 18)
    glass = rect(134, 26, 186, 92) | ellipse(160, 26, 26, 14)
    outline(img, win, INK)
    tone_fill(img, win & ~glass, 0.5 + (-(XX - 160) - (YY - 50)) / 80.0, ['#d99a1c', '#ffc93a', '#ffe68a'])
    tmp = canvas()
    gradient(tmp, 8, 93, ['#9ccaff', '#b6d8ff', '#d0e6ff'])
    img[glass, :3] = tmp[glass, :3]
    cloud(img, [(150, 50, 8, 5), (160, 47, 8, 6), (170, 51, 6, 4)], ['#e6f2ff', '#ffffff'])
    paint(img, glass & ((XX == 160) | (YY == 60)), '#d99a1c')
    # banners with a crown
    for bx in (52, 268):
        ban = poly_mask([(bx - 14, 16), (bx + 14, 16), (bx + 14, 84), (bx, 74), (bx - 14, 84)])
        outline(img, ban, INK)
        tone_fill(img, ban, 0.5 + (XX - bx) / 40.0, ['#a01a3a', '#c21f45', '#e5395a'])
        paint(img, rect(bx - 14, 16, bx + 14, 19), '#ffd23f')
        cr = poly_mask([(bx - 8, 50), (bx + 8, 50), (bx + 9, 40), (bx + 4, 44), (bx, 37), (bx - 4, 44), (bx - 9, 40)])
        outline(img, cr, '#d99a1c')
        paint(img, cr, '#ffd23f')
    # the throne, right of the middle (the king stands in front of it)
    seat = rect(212, 150, 268, 196)
    back = rect(220, 92, 260, 152) | ellipse(240, 92, 20, 12)
    arms = rect(206, 140, 218, 196) | rect(262, 140, 274, 196)
    outline(img, seat | back | arms, INK)
    tone_fill(img, back, 0.5 + (-(XX - 240) - (YY - 110)) / 80.0, ['#a01a3a', '#c21f45', '#e5395a', '#ff7a8a'])
    paint(img, back & ~(rect(224, 96, 256, 150) | ellipse(240, 96, 16, 9)), '#ffc93a')
    tone_fill(img, seat, 0.5, ['#c21f45', '#e5395a'])
    tone_fill(img, arms, 0.5 + (XX - 240) / 80.0, ['#d99a1c', '#ffc93a', '#ffe68a'])
    for (x, y) in ((212, 138), (268, 138)):
        b = ellipse(x, y, 5, 4)
        outline(img, b, INK)
        paint(img, b, '#ffe68a')
    # piles of toys on the left (all his!)
    chest = rect(8, 158, 64, 196)
    lid = poly_mask([(6, 158), (66, 158), (60, 146), (12, 146)])
    outline(img, chest | lid, INK)
    tone_fill(img, chest, 0.5 + (XX - 36) / 80.0, WOODS)
    tone_fill(img, lid, 0.7, WOODS)
    paint(img, rect(8, 170, 64, 173), '#ffd23f')
    ball = lobe(img, 24, 140, 9, 9, ['#2a8a45', '#4cc25a', '#94e08a', '#ffffff'], L=LIGHT_UL)
    outline(img, ball, INK)
    paint(img, ball & (YY == 140), '#ffffff')
    drum = rect(40, 130, 58, 146)
    outline(img, drum, INK)
    paint(img, drum, '#5a9cf0')
    paint(img, drum & ((XX - YY) % 6 == 0), '#ffd23f')
    paint(img, rect(40, 130, 58, 132), '#ffffff')
    line(img, [(44, 128), (52, 120)], '#c98f58', 0.5)
    for (bx, by, c) in ((70, 184, 'y'), (82, 186, 'r'), (76, 174, 'b')):
        block(img, bx, by, 10, c)
    teddy = ellipse(98, 184, 8, 9) | ellipse(98, 170, 7, 6.5) | ellipse(92, 164, 2.6, 2.6) | ellipse(104, 164, 2.6, 2.6)
    outline(img, teddy, INK)
    tone_fill(img, teddy, 0.55 + (-(XX - 98) - (YY - 176)) / 30.0, ['#a8703e', '#c98f58', '#e0ad76'])
    paint(img, ellipse(98, 172, 3, 2), '#f2cc98')
    for ex in (95, 101):
        paint(img, rect(ex, 168, ex, 168), INK)
    # the floor: chequered, a red carpet to the throne
    _floor_tiles(img, 196, '#f0e6ff', '#d8ccf0', '#b8acd0', size=20)
    carpet = poly_mask([(206, 196), (274, 196), (300, 240), (180, 240)])
    outline(img, carpet, INK)
    paint(img, carpet, '#c21f45')
    paint(img, carpet & ~poly_mask([(210, 197), (270, 197), (294, 239), (186, 239)]), '#ffd23f')
    return img


def all_backgrounds():
    lit, _ = mappa4()
    return {'bg_mappa4': lit, 'bg_mappa4_ferma': ferma(lit), 'bg_fabbrica': fabbrica_bg(), 'bg_birilli': birilli_bg(),
            'bg_cubi': cubi_bg(), 'bg_giostra': giostra_bg(), 'bg_castello_giochi': castello_giochi_bg()}


def anchors():
    return mappa4()[1]
