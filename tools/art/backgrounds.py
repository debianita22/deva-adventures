"""Procedural 320x240 backgrounds: stage, counting panel, dressing room, night."""
import numpy as np
from pixel import (blank, blit, rect_mask, ellipse_mask, dilate4, rgb, PALETTE,
                   ascii_sprite, shaded_part)

W, H = 320, 240
BAYER4 = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0


def hexrgb(s):
    return tuple(int(s[i:i + 2], 16) for i in (1, 3, 5))


def canvas(color=(0, 0, 0)):
    img = np.zeros((H, W, 4), dtype=np.uint8)
    img[..., :3] = color
    img[..., 3] = 255
    return img


def gradient(img, y0, y1, stops, x0=0, x1=W):
    """Vertical gradient through colour stops with 4x4 ordered dithering
    between neighbouring stops (keeps a limited, pixel-art palette)."""
    stops = [hexrgb(c) if isinstance(c, str) else c for c in stops]
    n = len(stops) - 1
    for y in range(y0, y1):
        t = (y - y0) / max(1, (y1 - y0 - 1)) * n
        i = min(int(t), n - 1)
        f = t - i
        for x in range(x0, x1):
            c = stops[i + 1] if f > BAYER4[y % 4, x % 4] else stops[i]
            img[y, x, :3] = c


def blend(img, mask, color, a):
    c = np.array(color, dtype=np.float32)
    px = img[mask, :3].astype(np.float32)
    img[mask, :3] = (px * (1 - a) + c * a).astype(np.uint8)


def poly_mask(pts):
    from objects import poly_mask as pm
    return pm(W, H, pts, ss=2)


def stars(img, rng, n, area, avoid=None):
    x0, y0, x1, y1 = area
    for _ in range(n):
        x, y = rng.integers(x0, x1), rng.integers(y0, y1)
        if avoid is not None and avoid[y, x]:
            continue
        c = rgb(['w', 'y', 'b', 'p'][rng.integers(0, 4)])
        img[y, x, :3] = c
        if rng.random() < 0.25 and 1 <= x < W - 1 and 1 <= y < H - 1:
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                img[y + dy, x + dx, :3] = hexrgb('#8a6fd0')


def curtain_strip(h, w, phase=0):
    """Velvet curtain: vertical folds with a cosine shading profile."""
    img = blank(w, h)
    cols = [rgb('H'), hexrgb('#d93f86'), rgb('h'), hexrgb('#ff79b0'), rgb('h'), hexrgb('#d93f86')]
    period = 12
    for x in range(w):
        t = ((x + phase) % period) / period
        k = int(t * len(cols)) % len(cols)
        img[:, x, :3] = cols[k]
        img[:, x, 3] = 255
    # darker line between folds
    for x in range(w):
        if (x + phase) % period == 0:
            img[:, x, :3] = hexrgb('#8f1f59')
    return img


def valance(img):
    """Top scalloped curtain with a gold fringe."""
    yy, xx = np.mgrid[0:H, 0:W]
    hem = 16 + 5 * np.abs(np.cos(np.pi * (xx + 0.5) / 32.0))
    m = yy < hem
    strip = curtain_strip(H, W, phase=3)
    img[m, :3] = strip[m, :3]
    edge = dilate4(m) & ~m
    img[edge, :3] = rgb('Y')
    edge2 = dilate4(dilate4(m)) & ~dilate4(m)
    img[edge2 & ((xx % 2) == 0), :3] = rgb('o')
    # small gold tassels at the scallop joins
    for cx in range(0, W + 1, 32):
        for dy in range(0, 5):
            for dx in (-1, 0):
                x, y = cx + dx, 17 + dy
                if 0 <= x < W:
                    img[y, x, :3] = rgb('Y' if dy < 4 else 'o')


def side_curtains(img, top=0, bottom=200, width=30):
    left = curtain_strip(bottom - top, width, phase=0)
    img[top:bottom, 0:width, :3] = left[..., :3]
    img[top:bottom, W - width:W, :3] = left[:, ::-1, :3]
    for x in (width, W - width - 1):
        img[top:bottom, x, :3] = rgb('K')
    # gold tie-backs
    for x0 in (0, W - width):
        img[118:122, x0:x0 + width, :3] = rgb('Y')
        img[121, x0:x0 + width, :3] = rgb('o')


def floor(img, y0=192):
    planks = [hexrgb('#ffb3d1'), hexrgb('#ffa3c7')]
    for y in range(y0, 226):
        k = ((y - y0) // 6) % 2
        img[y, :, :3] = planks[k]
        if (y - y0) % 6 == 5:
            img[y, :, :3] = hexrgb('#f08ab4')
    # plank joints, staggered
    for row in range((226 - y0) // 6 + 1):
        y = y0 + row * 6
        off = 0 if row % 2 == 0 else 20
        for x in range(off, W, 40):
            img[y:min(y + 5, 226), x, :3] = hexrgb('#f08ab4')
    # front apron with gold trim and footlights
    img[226:240, :, :3] = rgb('H')
    img[226, :, :3] = rgb('Y')
    img[227, :, :3] = rgb('o')
    for cx in range(12, W, 24):
        m = ellipse_mask(W, H, cx + 0.5, 233.5, 3.2, 3.2)
        img[m, :3] = rgb('y')
        ring = dilate4(m) & ~m
        img[ring, :3] = rgb('o')
        img[232, cx - 1, :3] = rgb('w')


def spotlights(img, area_mask=None):
    for sx, tx in ((70, 130), (250, 190)):
        pts = [(sx - 6, 0), (sx + 6, 0), (tx + 46, 214), (tx - 46, 214)]
        m = poly_mask(pts)
        yy, xx = np.mgrid[0:H, 0:W]
        m &= yy < 214
        # two-level dithered light: brighter core
        core = poly_mask([(sx - 2, 0), (sx + 2, 0), (tx + 24, 214), (tx - 24, 214)]) & m
        blend(img, m & ~core, hexrgb('#fff3c4'), 0.16)
        blend(img, core, hexrgb('#fff3c4'), 0.26)
    # light pools on the floor
    for tx in (130, 190):
        m = ellipse_mask(W, H, tx + 0.5, 206, 48, 8)
        blend(img, m, hexrgb('#fff6d8'), 0.35)


def disco_ball(img, cx=160, cy=40, r=11):
    img[22:cy - r, cx, :3] = rgb('S')
    m = ellipse_mask(W, H, cx + 0.5, cy + 0.5, r, r)
    yy, xx = np.mgrid[0:H, 0:W]
    tiles = (((xx // 3) + (yy // 3)) % 2 == 0)
    img[m & tiles, :3] = rgb('s')
    img[m & ~tiles, :3] = rgb('S')
    hl = ellipse_mask(W, H, cx - 3.5, cy - 3.5, 4, 4) & m
    img[hl & tiles, :3] = rgb('w')
    sh = m & ~ellipse_mask(W, H, cx - 1.5, cy - 1.5, r, r)
    img[sh, :3] = rgb('x')
    ring = dilate4(m) & ~m
    img[ring, :3] = rgb('k')


def moon(img, cx=250, cy=58, r=18):
    m = ellipse_mask(W, H, cx, cy, r, r) & ~ellipse_mask(W, H, cx + 8, cy - 5, r - 1, r - 1)
    img[m, :3] = rgb('y')
    ring = dilate4(m) & ~m
    img[ring, :3] = rgb('o')
    # sleepy face on the crescent
    for (x, y) in ((cx - 12, cy - 2), (cx - 11, cy - 1), (cx - 10, cy - 1), (cx - 9, cy - 2)):
        img[y, x, :3] = rgb('O')


def sky(img, night=False):
    stops = ['#1c0f33', '#2c1650', '#46246e', '#63308a', '#8a3f9e'] if not night else \
        ['#0d0820', '#160d33', '#1f1244', '#2a1650', '#35195a']
    gradient(img, 0, 192, stops)


def stage(night=False, seed=7, disco=True):
    img = canvas()
    sky(img, night)
    rng = np.random.default_rng(seed)
    stars(img, rng, 170 if night else 120, (32, 22, W - 32, 186))
    if disco and not night:
        disco_ball(img)
    if night:
        moon(img)
    floor(img)
    if not night:
        spotlights(img)
    else:
        blend(img, np.ones((H, W), dtype=bool) & (np.mgrid[0:H, 0:W][0] >= 192),
              hexrgb('#1a0f33'), 0.45)
    side_curtains(img)
    valance(img)
    return img


PANEL = (80, 28, 302, 166)      # x0, y0, x1, y1 of the counting panel


def conta_bg():
    img = stage(disco=False)
    x0, y0, x1, y1 = PANEL
    m = rect_mask(W, H, x0, y0, x1, y1, radius=8)
    ring = dilate4(m) & ~m
    blend(img, m, hexrgb('#fff4fa'), 0.94)
    img[ring, :3] = rgb('k')
    # inner pink frame, 2 px
    inner = rect_mask(W, H, x0 + 2, y0 + 2, x1 - 2, y1 - 2, radius=6)
    frame = m & ~inner
    img[frame, :3] = rgb('p')
    # subtle polka dots
    yy, xx = np.mgrid[0:H, 0:W]
    dots = inner & ((xx % 16) == 8) & ((yy % 16) == 8)
    img[dots, :3] = hexrgb('#ffe3ef')
    # soft shadow under the panel
    sh = rect_mask(W, H, x0 + 3, y1 + 1, x1 + 2, y1 + 3) & ~m
    blend(img, sh & (((xx + yy) % 2) == 0), rgb('K'), 0.6)
    return img


def camerino_bg():
    img = canvas()
    yy, xx = np.mgrid[0:H, 0:W]
    stripe = (xx // 10) % 2 == 0
    img[stripe, :3] = hexrgb('#ffd6e8')
    img[~stripe, :3] = hexrgb('#fff0f6')
    # tiny hearts on light stripes
    heart = ["k.k", "kkk", ".k."]
    for y in range(8, 180, 20):
        for x in range(13, W, 20):
            off = 10 if (y // 20) % 2 else 0
            hx = x + off
            if hx + 3 < W and not stripe[y, hx]:
                for j, row in enumerate(heart):
                    for i, ch in enumerate(row):
                        if ch == 'k':
                            img[y + j, hx + i, :3] = hexrgb('#ffb3d1')
    # wall moulding
    img[178:182, :, :3] = rgb('W')
    img[182, :, :3] = hexrgb('#f0a8c8')
    # vanity mirror with bulbs (behind the mascot)
    mx0, my0, mx1, my1 = 12, 26, 108, 176
    frame = rect_mask(W, H, mx0, my0, mx1, my1, radius=14)
    glass = rect_mask(W, H, mx0 + 6, my0 + 6, mx1 - 6, my1 - 6, radius=10)
    img[frame, :3] = rgb('Y')
    img[frame & ~rect_mask(W, H, mx0 + 1, my0 + 1, mx1 - 1, my1 - 1, radius=13), :3] = rgb('o')
    img[glass, :3] = hexrgb('#d9ecff')
    shine = glass & ((((xx - yy) % 40) < 6) | (((xx - yy) % 40) == 9))
    img[shine, :3] = hexrgb('#f2f8ff')
    ring = dilate4(frame) & ~frame
    img[ring, :3] = rgb('k')
    ring2 = dilate4(glass) & ~glass & frame
    img[ring2, :3] = rgb('O')
    for bx, by in [(mx0 + 3, y) for y in range(my0 + 14, my1 - 8, 20)] + \
                  [(mx1 - 3, y) for y in range(my0 + 14, my1 - 8, 20)] + \
                  [(x, my0 + 3) for x in range(mx0 + 16, mx1 - 10, 20)]:
        b = ellipse_mask(W, H, bx + 0.5, by + 0.5, 3.4, 3.4)
        img[b, :3] = rgb('y')
        img[dilate4(b) & ~b, :3] = rgb('o')
        img[by - 1, bx - 1, :3] = rgb('w')
    # vanity table
    img[190:240, :, :3] = hexrgb('#f6c89f')
    img[190:193, :, :3] = hexrgb('#ffe0c4')
    img[193, :, :3] = hexrgb('#d99a6c')
    for y in range(200, 240, 12):
        img[y, :, :3] = hexrgb('#ebb58a')
    img[189, :, :3] = rgb('k')
    # shelf behind the reward cards
    img[164:168, 124:314, :3] = hexrgb('#ffe0c4')
    img[168, 124:314, :3] = hexrgb('#d99a6c')
    img[163, 124:314, :3] = rgb('k')
    img[169, 124:314, :3] = rgb('k')
    return img


def curtain_panel(width=176):
    """Closing curtain (left half); the runtime mirrors it for the right."""
    strip = curtain_strip(H, width, phase=5)
    yy, xx = np.mgrid[0:H, 0:width]
    # gathered hem: slightly shorter towards the centre, gold edge
    strip[(yy > 232), :3] = rgb('H')
    strip[:, width - 2:width, :3] = rgb('Y')
    strip[:, width - 1, :3] = rgb('o')
    return strip


def all_backgrounds():
    import scenery   # the dressing room, redrawn in 0.8.0 (same mirror)
    return {
        'bg_palco': stage(),
        'bg_conta': conta_bg(),
        'bg_camerino': scenery.camerino(),
        'bg_notte': stage(night=True),
    }
