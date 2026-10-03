"""Painted scenery for the places of the tale (0.8.0): volume shading for the
backgrounds that were flat shapes with a one-pixel rim.

Helpers:
  tone_fill  a 0..1 field -> a ramp of colours, ordered dithering between steps
  lobe       an ellipse lit like a ball (lambert), the brick of crowns, clouds, rocks
  glow       a soft halo of light (dithered alpha steps)
  facet      a polygon filled with one tone (angular rocks, crystals)

Scenes: bosco, palude, grotta, nuvole (first adventure), vulcano, ghiaccio (the
second). The layout of each place is the one the tales and the duels expect:
the sky and the far things above y ~ 150, the ground from y ~ 196 (Deva's feet
at x ~ 72, the monster's at x ~ 238, y ~ 214), the middle of the screen calm.
"""
import numpy as np

from backgrounds import W, H, BAYER4, canvas, gradient, blend, poly_mask, stars, hexrgb
from pixel import dilate4, rgb

YY, XX = np.mgrid[0:H, 0:W]
BAYER = BAYER4[YY % 4, XX % 4]
LIGHT_MOON_R = (0.55, -0.62, 0.56)   # moonlight from the upper right
LIGHT_UL = (-0.55, -0.62, 0.56)      # light from the upper left


def c3(c):
    return np.array(rgb(c) if isinstance(c, str) else c, dtype=np.float32)


def tone_fill(img, mask, t, tones, dith=True, band=1.0):
    """Colour the mask with the ramp `tones` (dark -> light) along t in 0..1;
    band < 1 keeps the dithering to a narrow strip between two tones."""
    n = len(tones)
    v = np.clip(t, 0.0, 0.9999) * (n - 1)
    i = np.floor(v).astype(int)
    if dith:
        f = v - i
        if band < 1.0:
            f = np.clip((f - 0.5) / max(band, 1e-3) + 0.5, 0.0, 1.0)
        i = i + (f > BAYER)
    else:
        i = np.rint(v).astype(int)
    i = np.clip(i, 0, n - 1)
    for k, c in enumerate(tones):
        m = mask & (i == k)
        img[m, :3] = rgb(c)


def ellipse_field(cx, cy, rx, ry, L):
    nx = (XX + 0.5 - cx) / rx
    ny = (YY + 0.5 - cy) / ry
    r2 = nx * nx + ny * ny
    nz = np.sqrt(np.clip(1.0 - r2, 0.0, 1.0))
    lv = np.array(L, dtype=np.float32)
    lv /= np.linalg.norm(lv)
    d = nx * lv[0] + ny * lv[1] + nz * lv[2]
    return r2 <= 1.0, np.clip(d, 0.0, 1.0)


def lobe(img, cx, cy, rx, ry, tones, L=LIGHT_UL, lo=0.0, hi=1.0, gamma=1.0, clip=None, band=1.0):
    """A lit ellipse; returns its mask."""
    m, d = ellipse_field(cx, cy, rx, ry, L)
    if clip is not None:
        m &= clip
    tone_fill(img, m, lo + (hi - lo) * d ** gamma, tones, band=band)
    return m


def ellipse(cx, cy, rx, ry):
    return ((XX + 0.5 - cx) / rx) ** 2 + ((YY + 0.5 - cy) / ry) ** 2 <= 1.0


def rect(x0, y0, x1, y1):
    return (XX >= x0) & (XX <= x1) & (YY >= y0) & (YY <= y1)


def outline(img, mask, c):
    img[dilate4(mask) & ~mask, :3] = rgb(c)


def paint(img, mask, c):
    img[mask, :3] = rgb(c)


def glow(img, cx, cy, rx, ry, color, a=0.35, mask=None, steps=6):
    """A halo: strongest in the middle, fading out, in dithered steps."""
    d = np.sqrt(((XX + 0.5 - cx) / rx) ** 2 + ((YY + 0.5 - cy) / ry) ** 2)
    k = np.clip(1.0 - d, 0.0, 1.0) ** 1.6 * a
    q = np.floor(k * steps + BAYER) / steps
    m = q > 0
    if mask is not None:
        m &= mask
    col = c3(color)
    px = img[m, :3].astype(np.float32)
    img[m, :3] = np.clip(px * (1.0 - q[m, None]) + col * q[m, None], 0, 255).astype(np.uint8)


def vfade(img, mask, color, a0, a1, y0, y1, steps=6):
    """Blend towards a colour, alpha going from a0 (at y0) to a1 (at y1), dithered."""
    t = np.clip((YY - y0) / max(1, (y1 - y0)), 0.0, 1.0)
    k = a0 + (a1 - a0) * t
    q = np.floor(k * steps + BAYER) / steps
    m = mask & (q > 0)
    col = c3(color)
    px = img[m, :3].astype(np.float32)
    img[m, :3] = np.clip(px * (1.0 - q[m, None]) + col * q[m, None], 0, 255).astype(np.uint8)


def speckle(img, mask, color, density, seed):
    rng = np.random.default_rng(seed)
    m = mask & (rng.random((H, W)) < density)
    img[m, :3] = rgb(color)


def line(img, pts, color, width=1.0, mask=None):
    """A polyline of round dots."""
    for (x0, y0), (x1, y1) in zip(pts[:-1], pts[1:]):
        n = int(max(abs(x1 - x0), abs(y1 - y0)) * 2) + 1
        for t in np.linspace(0, 1, n):
            x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
            m = ellipse(x, y, width, width) if width > 0.6 else rect(int(x), int(y), int(x), int(y))
            if mask is not None:
                m &= mask
            img[m, :3] = rgb(color)


def cells(img, mask, n, seed, tones, crack, L=LIGHT_UL, stretch=(1.0, 1.0), bias=None, gap=1.1):
    """Stones: the mask cut into irregular cells (nearest seed), each one bulging
    towards the light, with dark cracks between them. bias(t-field) darkens/lightens."""
    rng = np.random.default_rng(seed)
    ys, xs = np.where(mask)
    if len(xs) == 0:
        return
    pick = rng.integers(0, len(xs), n)
    sx = xs[pick].astype(np.float32) + rng.uniform(-0.5, 0.5, n)
    sy = ys[pick].astype(np.float32) + rng.uniform(-0.5, 0.5, n)
    px = (XX[mask] + 0.5)[:, None]
    py = (YY[mask] + 0.5)[:, None]
    d = np.sqrt(((px - sx[None]) / stretch[0]) ** 2 + ((py - sy[None]) / stretch[1]) ** 2)
    order = np.argsort(d, axis=1)
    i1, i2 = order[:, 0], order[:, 1]
    d1 = d[np.arange(len(px)), i1]
    d2 = d[np.arange(len(px)), i2]
    lv = np.array(L[:2], dtype=np.float32)
    lv /= np.linalg.norm(lv)
    vx = (px[:, 0] - sx[i1]) / stretch[0]
    vy = (py[:, 0] - sy[i1]) / stretch[1]
    size = np.maximum(d2, 4.0)
    t = 0.55 - (vx * lv[0] + vy * lv[1]) / size * 0.9 - (d1 / size) * 0.15
    if bias is not None:
        t = t + bias[mask]
    field = np.zeros((H, W), np.float32)
    field[mask] = t
    tone_fill(img, mask, field, tones)
    edge = np.zeros((H, W), bool)
    edge[mask] = (d2 - d1) < gap
    paint(img, edge, crack)


def facet(img, pts, color):
    m = poly_mask(pts)
    img[m, :3] = rgb(color)
    return m


# ------------------------------------------------------------------ pieces
def round_tree(img, cx, base, r, greens, trunks, L=LIGHT_MOON_R, ink=None, seed=0, trunk_h=None):
    """A leafy tree: a trunk with bark and a crown of clumps, each lit like a ball."""
    rng = np.random.default_rng(seed)
    th = trunk_h if trunk_h is not None else int(r * 1.1)
    top = base - th - int(r * 0.8)
    trunk = poly_mask([(cx - r * 0.18, base), (cx - r * 0.12, base - th - 4), (cx + r * 0.12, base - th - 4),
                       (cx + r * 0.2, base)]) | poly_mask([(cx - r * 0.34, base), (cx - r * 0.1, base - 6),
                                                          (cx + r * 0.1, base - 6), (cx + r * 0.36, base)])
    clumps = [(cx, top - r * 0.15, r * 0.72, r * 0.62)]
    for k in range(7):
        a = -np.pi / 2 + (k - 3) * 0.62 + rng.uniform(-0.15, 0.15)
        d = r * rng.uniform(0.48, 0.62)
        clumps.append((cx + d * np.cos(a) * 1.15, top + r * 0.2 + d * np.sin(a) * 0.8 + r * 0.1,
                       r * rng.uniform(0.38, 0.5), r * rng.uniform(0.34, 0.44)))
    clumps.append((cx - r * 0.55, top + r * 0.45, r * 0.42, r * 0.34))
    clumps.append((cx + r * 0.55, top + r * 0.45, r * 0.42, r * 0.34))
    crown = np.zeros((H, W), bool)
    for (x, y, rx, ry) in clumps:
        crown |= ellipse(x, y, rx, ry)
    if ink:
        outline(img, trunk | crown, ink)
    # trunk: lit side, bark lines
    tx = (XX + 0.5 - cx) / max(1.0, r * 0.3)
    tone_fill(img, trunk, np.clip(0.5 + tx * (0.9 if L[0] > 0 else -0.9) * 0.6, 0, 1), trunks)
    for k in range(3):
        bx = cx - r * 0.08 + k * r * 0.08
        line(img, [(bx, base - 3), (bx + rng.uniform(-1, 1), base - th + 2)], trunks[0], 0.5, trunk)
    # the crown: back clumps darker, front clumps brighter
    order = sorted(clumps, key=lambda c: c[1])
    for (x, y, rx, ry) in order:
        lobe(img, x, y, rx, ry, greens, L=L, lo=0.0, hi=1.0, gamma=0.85)
    # a darker belly under the crown and a few leaf pixels
    belly = crown & (YY > top + r * 0.55)
    vfade(img, belly, greens[0], 0.0, 0.45, top + r * 0.55, top + r * 1.0)
    leaves = crown & (rng.random((H, W)) < 0.05)
    img[leaves, :3] = rgb(greens[-1] if L[0] > 0 else greens[1])
    return crown | trunk


def pine(img, cx, base, h, w, tones, L=LIGHT_MOON_R, ink=None):
    """A fir: stacked lit triangles."""
    m_all = np.zeros((H, W), bool)
    tiers = 4
    for k in range(tiers):
        y1 = base - 6 - k * h * 0.2
        y0 = y1 - h * 0.42
        ww = w * (1.0 - k * 0.2)
        m = poly_mask([(cx - ww / 2, y1), (cx, y0), (cx + ww / 2, y1)])
        m_all |= m
    trunk = rect(cx - 1, base - 8, cx + 1, base)
    if ink:
        outline(img, m_all | trunk, ink)
    paint(img, trunk, tones[0])
    for k in range(tiers):
        y1 = base - 6 - k * h * 0.2
        y0 = y1 - h * 0.42
        ww = w * (1.0 - k * 0.2)
        m = poly_mask([(cx - ww / 2, y1), (cx, y0), (cx + ww / 2, y1)])
        side = (XX + 0.5 - cx) / max(1.0, ww / 2)
        t = 0.45 + side * (0.45 if L[0] > 0 else -0.45) - (YY - y0) / max(1.0, (y1 - y0)) * 0.25
        tone_fill(img, m, t, tones)
    return m_all


def cloud(img, lobes, tones, L=(-0.35, -0.8, 0.5), ink=None, lo=0.0, band=0.35):
    """A cloud of lobes (x, y, rx, ry), lit from above: crisp bands of tone."""
    m_all = np.zeros((H, W), bool)
    for (x, y, rx, ry) in lobes:
        m_all |= ellipse(x, y, rx, ry)
    if ink:
        outline(img, m_all, ink)
    for (x, y, rx, ry) in sorted(lobes, key=lambda c: -c[1]):
        lobe(img, x, y, rx, ry, tones, L=L, lo=lo, gamma=0.8, band=band)
    return m_all


def puff_row(x0, x1, y, r, seed, jitter=4):
    """Lobes along a row, for cloud banks."""
    rng = np.random.default_rng(seed)
    out, x = [], x0
    while x < x1:
        rr = r * rng.uniform(0.7, 1.2)
        out.append((x, y + rng.uniform(-jitter, jitter), rr, rr * 0.75))
        x += rr * 1.3
    return out


def mushroom_glow(img, cx, base, s=1.0, cap=('#1d6f73', '#2fb3a8', '#7ff0dc', '#d8fff4'), halo='#7ff0dc'):
    glow(img, cx, base - 6 * s, 14 * s, 10 * s, halo, 0.28)
    stem = rect(cx - 1, base - 5 * s, cx + 1, base)
    paint(img, stem, '#cfd8e8')
    paint(img, stem & (XX == cx + 1), '#8e9ab4')
    lobe(img, cx, base - 5 * s, 4.6 * s, 3.0 * s, list(cap), L=LIGHT_UL, clip=YY <= base - 4 * s)
    img[int(base - 7 * s), cx - 1, :3] = rgb('w')


def firefly(img, x, y, color='#e6ff6e'):
    glow(img, x, y, 6, 6, color, 0.35)
    img[y, x, :3] = rgb('#fbffd0')


def grass_tufts(img, y0, y1, colors, seed, n=60, hmin=3, hmax=7, avoid=None):
    rng = np.random.default_rng(seed)
    for _ in range(n):
        x = int(rng.integers(0, W))
        y = int(rng.integers(y0, y1))
        if avoid is not None and avoid[y, x]:
            continue
        h = int(rng.integers(hmin, hmax + 1))
        c = colors[int(rng.integers(0, len(colors)))]
        for dx, lean in ((-2, -1), (0, 0), (2, 1)):
            for k in range(h - abs(dx) // 2):
                xx = x + dx + (lean * k) // 3
                yy = y - k
                if 0 <= xx < W and 0 <= yy < H:
                    img[yy, xx, :3] = rgb(c)


def moon_face(img, cx, cy, r, ink, blush='#ffa8c0'):
    """A sleepy, happy face on a full moon: closed eyes like little arcs, pink
    cheeks, a small smile (kawaii, 0.9.0)."""
    cx, cy = int(round(cx)), int(round(cy))
    e = max(3, int(r * 0.36))
    for ex in (cx - e, cx + e):
        for (dx, dy) in ((-2, 1), (-1, 0), (0, 0), (1, 0), (2, 1)):
            img[cy - 1 + dy, ex + dx, :3] = rgb(ink)
    for bx in (cx - e - 2, cx + e + 1):
        paint(img, rect(bx, cy + 2, bx + 1, cy + 2), blush)
    for (dx, dy) in ((-1, 3), (0, 4), (1, 3)):
        img[cy + dy, cx + dx, :3] = rgb(ink)


def crescent_face(img, cx, cy, r, ink, blush='#ffa8c0'):
    """The face of a sleeping crescent (cut towards the upper right): one closed
    eye and a cheek on the thick part."""
    ex, ey = int(round(cx - r * 0.5)), int(round(cy + r * 0.12))
    for (dx, dy) in ((-1, 0), (0, 1), (1, 1), (2, 0)):
        img[ey + dy, ex + dx, :3] = rgb(ink)
    paint(img, rect(ex - 2, ey + 3, ex - 1, ey + 3), blush)
    for (dx, dy) in ((1, 5), (2, 6), (3, 5)):
        img[ey + dy, ex + dx, :3] = rgb(ink)


def moon(img, cx, cy, r, halo=True, face=True):
    if halo:
        glow(img, cx, cy, r * 2.4, r * 2.4, '#fff4c2', 0.2)
    m = lobe(img, cx, cy, r, r, ['#e8c65a', '#f5dc7a', '#fbeaa0', '#fff6cf'], L=(-0.4, -0.5, 0.8), lo=0.15)
    for (x, y, rr) in ((cx + r * 0.55, cy - r * 0.45, r * 0.16), (cx - r * 0.62, cy - r * 0.5, r * 0.12)):
        paint(img, ellipse(x, y, rr, rr) & m, '#e8c65a')
    if face:
        moon_face(img, cx, cy, r, '#b8862a')
    return m


# ------------------------------------------------------------------ the forest (Ciuffone)
def bosco():
    img = canvas()
    gradient(img, 0, 200, ['#0c0e2c', '#141a44', '#1f2a5c', '#2d3a6e', '#3a4a78'])
    rng = np.random.default_rng(3)
    stars(img, rng, 110, (0, 0, W, 110))
    moon(img, 252, 42, 17)
    # far hills and a line of misty firs
    hills = poly_mask([(0, 150)] + [(x, 138 + 8 * np.sin(x / 37.0) + 5 * np.sin(x / 13.0)) for x in range(0, W + 8, 8)]
                      + [(W, 200), (0, 200)])
    paint(img, hills, '#1f2f52')
    vfade(img, (YY > 110) & (YY < 200), '#3a4a78', 0.0, 0.35, 118, 176)
    for k in range(22):
        x = k * 15 + (k * 7) % 9
        pine(img, x, 172 + (k * 5) % 7, 34 + (k * 11) % 14, 16 + (k * 3) % 6, ['#223459', '#2b416b', '#37507c'])
    vfade(img, (YY > 150) & (YY < 200), '#46578a', 0.0, 0.3, 150, 196)
    # the middle row: round trees in the moonlight (the middle of the stage stays open)
    greens = ['#0e2420', '#163527', '#1f4a32', '#2d6440', '#46844f', '#6aa962']
    trunks = ['#1a1220', '#2b1f2e', '#3f2e3e', '#574353']
    for (cx, base, r, s) in ((26, 190, 24, 11), (98, 186, 19, 12), (214, 186, 18, 13), (290, 190, 25, 14)):
        round_tree(img, cx, base, r, greens[:5], trunks, seed=s, ink='#0a1418')
    for (cx, base, r, s) in ((156, 182, 14, 15),):
        round_tree(img, cx, base, r, greens[:4], trunks, seed=s)
    # the ground: grass, a lighter path to the back, ferns
    ground = YY >= 196
    gradient(img, 196, H, ['#1d3d2c', '#163326', '#10281e'])
    path = poly_mask([(140, 240), (178, 240), (172, 214), (164, 200), (156, 196), (150, 200), (146, 214)])
    tone_fill(img, path, 0.3 + (YY - 196) / 60.0 * 0.5, ['#2a3a34', '#34473c', '#40574a'])
    speckle(img, ground & ~path, '#285a3a', 0.06, 5)
    speckle(img, ground & ~path, '#0c2016', 0.05, 6)
    vfade(img, ground & (XX > 200), '#5a8a7a', 0.12, 0.0, 196, 226)   # moonlight on the grass
    for (x, y) in ((22, 205), (84, 199), (230, 200), (300, 204)):       # shade at the foot of the trees
        glow(img, x, y, 26, 5, '#07130e', 0.5, mask=ground)
    fr = np.random.default_rng(12)
    for _ in range(26):   # little bell flowers
        x, y = int(fr.integers(4, W - 4)), int(fr.integers(202, 236))
        if path[y, x]:
            continue
        c = ['#8f9cf0', '#c9a0f0', '#f0a0c8'][int(fr.integers(0, 3))]
        paint(img, rect(x, y - 3, x, y), '#2f6a44')
        paint(img, rect(x - 1, y - 4, x + 1, y - 3), c)
    grass_tufts(img, 200, 238, ['#2f6a44', '#3f8454', '#24553a'], 7, n=70, avoid=path)
    # frame: two big trees at the edges, close to us
    greens_near = ['#081a17', '#0f2a20', '#173d2a', '#245636', '#3a7447']
    round_tree(img, -8, 226, 38, greens_near, trunks, seed=21, ink='#050c0e', trunk_h=60)
    round_tree(img, 330, 228, 40, greens_near, trunks, seed=22, ink='#050c0e', trunk_h=64)
    for (cx, by, s) in ((34, 216, 1.0), (58, 206, 0.8), (292, 212, 1.0), (266, 204, 0.8), (124, 222, 0.9),
                        (196, 230, 1.1)):
        mushroom_glow(img, cx, by, s)
    fl = np.random.default_rng(9)
    for _ in range(14):   # fireflies
        firefly(img, int(fl.integers(10, W - 10)), int(fl.integers(96, 190)))
    # a low mist over the grass
    vfade(img, (YY > 184) & (YY < 204), '#8f9ac0', 0.0, 0.18, 184, 204)
    return img


# ------------------------------------------------------------------ the swamp (Melmoso)
def branch(img, pts, w0, w1, tones, L=LIGHT_UL, mask_out=None):
    """A tapering limb along pts, lit on one side."""
    n = len(pts) - 1
    m_all = np.zeros((H, W), bool)
    for i in range(n):
        (x0, y0), (x1, y1) = pts[i], pts[i + 1]
        steps = int(max(abs(x1 - x0), abs(y1 - y0)) * 2) + 1
        for t in np.linspace(0, 1, steps):
            f = (i + t) / n
            r = w0 + (w1 - w0) * f
            m_all |= ellipse(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, r, r)
    return m_all


def gnarled_tree(img, base_x, base_y, limbs, tones, ink, moss=None, seed=0):
    """limbs: [(points, w0, w1)]; the first is the trunk. Lit from the upper left."""
    rng = np.random.default_rng(seed)
    m_all = np.zeros((H, W), bool)
    masks = []
    for (pts, w0, w1) in limbs:
        m = branch(img, pts, w0, w1, tones)
        masks.append(m)
        m_all |= m
    outline(img, m_all, ink)
    # shading: distance to the lit edge approximated by a shifted mask
    lit = m_all & ~np.roll(np.roll(m_all, 2, axis=1), 2, axis=0)
    dark = m_all & ~np.roll(np.roll(m_all, -2, axis=1), -1, axis=0)
    paint(img, m_all, tones[1])
    paint(img, dark, tones[0])
    paint(img, lit, tones[2])
    # bark: a few vertical cracks
    cracks = m_all & (rng.random((H, W)) < 0.08) & ~lit
    paint(img, cracks, tones[0])
    if moss:
        for (pts, w0, w1) in limbs[1:]:
            for (x, y) in pts[1:]:
                if rng.random() < 0.7:
                    ln = int(rng.integers(8, 20))
                    xs = [(x + np.sin(k * 0.7 + x) * 1.2, y + k) for k in range(0, ln, 2)]
                    line(img, xs, moss[0], 0.5)
                    line(img, [(px + 1, py) for (px, py) in xs[::2]], moss[1], 0.5)
    return m_all


def lily_pad(img, cx, cy, rx, ry, flower=None):
    pad = ellipse(cx, cy, rx, ry) & ~poly_mask([(cx, cy), (cx + rx * 1.2, cy - ry * 0.6), (cx + rx * 1.2, cy + ry * 0.1)])
    outline(img, pad, '#12301f')
    tone_fill(img, pad, 0.35 + (-(XX - cx) / rx - (YY - cy) / ry) * 0.25, ['#2f6b3a', '#3f8a45', '#5aa653', '#7cc46a'])
    line(img, [(cx, cy), (cx - rx * 0.7, cy - ry * 0.2)], '#2f6b3a', 0.4, pad)
    if flower:
        fx, fy = cx - rx * 0.2, cy - ry * 0.9
        for (dx, dy) in ((-3, 0), (3, 0), (-2, -2), (2, -2), (0, -3)):
            m = ellipse(fx + dx, fy + dy, 2.2, 1.6)
            paint(img, m, flower[1])
            paint(img, m & (YY < fy + dy - 0.5), flower[0])
        paint(img, ellipse(fx, fy - 1, 1.3, 1.1), '#ffd23f')


def cattail(img, x, base, h, lean=0):
    top = base - h
    line(img, [(x, base), (x + lean, top + 8)], '#3e6a3a', 0.5)
    head = ellipse(x + lean, top + 4, 1.6, 4.5)
    paint(img, head, '#6b4226')
    paint(img, head & (XX <= x + lean - 0.5), '#8b5b2e')
    line(img, [(x + lean, top - 2), (x + lean, top)], '#3e6a3a', 0.3)
    # a leaf
    line(img, [(x, base - 2), (x - 3, base - h * 0.45), (x - 5, base - h * 0.6)], '#4c7a40', 0.4)


def palude():
    img = canvas()
    gradient(img, 0, 196, ['#0e1a1a', '#15272a', '#1d3634', '#2a4a40', '#3a5e4a'])
    rng = np.random.default_rng(4)
    stars(img, rng, 40, (0, 0, W, 70))
    # a pale moon behind thin clouds
    glow(img, 70, 44, 40, 40, '#e8f2c0', 0.18)
    lobe(img, 70, 44, 15, 15, ['#b9c98f', '#d2e0a6', '#e6f0c2', '#f6fbe2'], L=(-0.4, -0.5, 0.8), lo=0.2)
    moon_face(img, 70, 44, 15, '#7a8a50', '#f0b8c8')
    for (cx, cy, rx, a) in ((64, 40, 44, 0.55), (96, 52, 40, 0.45), (130, 34, 30, 0.35)):   # thin clouds
        glow(img, cx, cy, rx, 3.2, '#2f4a44', a, steps=4)
    # far willows, in the mist: a lumpy canopy, strands hanging down
    wl = np.random.default_rng(17)
    for (cx, cy, rx, ry) in ((30, 150, 34, 20), (100, 156, 28, 16), (172, 148, 40, 22), (250, 152, 36, 20),
                             (310, 146, 30, 22)):
        canopy = np.zeros((H, W), bool)
        for k in range(6):
            x = cx - rx * 0.8 + k * rx * 0.32
            canopy |= ellipse(x, cy - ry * 0.2 + wl.uniform(-3, 3), rx * 0.34, ry * 0.7)
        canopy |= ellipse(cx, cy, rx, ry * 0.75)
        for k in range(int(rx * 0.9)):
            x = int(cx - rx * 0.9 + 2 * k)
            y0 = int(cy + ry * 0.3)
            canopy |= rect(x, y0, x, y0 + 8 + int(wl.integers(0, 14)))
        tone_fill(img, canopy & (YY < 190), np.clip((cy + ry - YY) / (2.2 * ry), 0, 1),
                  ['#1a3230', '#1f3a34', '#264540', '#2f5249'])
    paint(img, rect(0, 178, W - 1, 196), '#1f3833')
    vfade(img, (YY > 120) & (YY < 196), '#5f7f6a', 0.0, 0.35, 124, 190)
    # the pond: dark water, the moon's reflection, ripples
    gradient(img, 190, H, ['#26402f', '#1f3528', '#182b21'])
    pond = ellipse(170, 216, 128, 22)
    outline(img, pond, '#101f19')
    tone_fill(img, pond, 0.2 + (YY - 196) / 44.0 * 0.4, ['#12302e', '#173c38', '#1c4a43', '#245a50'])
    for y in range(198, 236, 2):   # the moon, shaking on the water
        w = int(7 - abs(y - 210) * 0.35)
        if w > 0:
            x0 = 70 + int(2 * np.sin(y * 1.7)) - w
            paint(img, rect(x0, y, x0 + 2 * w, y) & pond, '#a9c090' if (y // 2) % 2 else '#d6e4b0')
    for (cx, cy, rx) in ((120, 208, 14), (214, 222, 18), (160, 230, 10), (250, 210, 12)):
        rip = ellipse(cx, cy, rx, rx * 0.22) & ~ellipse(cx, cy, rx - 1.5, rx * 0.22 - 1)
        paint(img, rip & pond, '#3f7a6a')
    for (cx, cy, rx, ry, fl) in ((104, 214, 9, 3.4, None), (138, 226, 8, 3.0, ('#ffcbe3', '#ff93c6')),
                                 (196, 208, 7, 2.8, None), (236, 228, 10, 3.6, ('#fff4fa', '#f0c0e0')),
                                 (268, 216, 8, 3.0, None)):
        lily_pad(img, cx, cy, rx, ry, fl)
    # muddy banks with reeds
    speckle(img, (YY >= 190) & ~pond, '#35553d', 0.08, 14)
    grass_tufts(img, 196, 238, ['#3e6a3a', '#4c7a40', '#2e5230'], 15, n=40, avoid=pond)
    for (x, h, lean) in ((22, 34, -1), (30, 28, 1), (38, 38, 0), (300, 36, 1), (292, 30, -1), (308, 26, 0),
                         (46, 24, 1), (284, 22, -1)):
        cattail(img, x, 204 + (x % 5), h, lean)
    # two old gnarled trees at the edges, with hanging moss
    tones = ['#1a1a24', '#2c2b38', '#46445a']
    gnarled_tree(img, 0, 0, [
        ([(-6, 200), (4, 150), (0, 100), (8, 60), (4, 20)], 9, 4),
        ([(4, 128), (22, 112), (38, 108), (50, 96)], 3.2, 1.2),
        ([(4, 88), (20, 70), (30, 50)], 2.6, 1.0),
        ([(22, 112), (28, 124)], 1.6, 0.8),
        ([(8, 60), (26, 46), (44, 44)], 2.4, 0.9),
    ], tones, '#0c0c14', moss=('#5f7f5a', '#7e9f6e'), seed=41)
    gnarled_tree(img, 0, 0, [
        ([(326, 204), (314, 150), (320, 110), (310, 70), (316, 30)], 10, 4),
        ([(314, 136), (294, 122), (278, 120), (264, 108)], 3.4, 1.2),
        ([(318, 96), (298, 80), (290, 60)], 2.8, 1.0),
        ([(294, 122), (290, 136)], 1.6, 0.8),
        ([(310, 70), (292, 58), (276, 60)], 2.4, 0.9),
    ], tones, '#0c0c14', moss=('#5f7f5a', '#7e9f6e'), seed=42)
    fl = np.random.default_rng(16)
    for _ in range(10):   # will-o'-the-wisps
        firefly(img, int(fl.integers(20, W - 20)), int(fl.integers(100, 196)), '#b8ff9e')
    vfade(img, (YY > 180) & (YY < 206), '#8fb0a0', 0.0, 0.2, 180, 204)
    return img


# ------------------------------------------------------------------ the crystal cave (Rocciolo)
ROCK = ['#120c20', '#1c1430', '#281d42', '#382a58', '#4a3a70']


def rock_mass(img, lobes, tones=ROCK, L=LIGHT_UL, ink='#0a0614', lo=0.0):
    m_all = np.zeros((H, W), bool)
    for (x, y, rx, ry) in lobes:
        m_all |= ellipse(x, y, rx, ry)
    if ink:
        outline(img, m_all, ink)
    for (x, y, rx, ry) in lobes:
        lobe(img, x, y, rx, ry, tones, L=L, lo=lo, gamma=1.1)
    return m_all


def spike(img, x, y, w, h, tones, down=True, ink='#0a0614'):
    """A stalactite (down) or a stalagmite (up), lit on its left side."""
    tip = y + h if down else y - h
    pts = [(x - w / 2, y), (x + w / 2, y), (x + w * 0.08, tip), (x - w * 0.08, tip)]
    m = poly_mask(pts)
    outline(img, m, ink)
    side = (XX + 0.5 - x) / max(1.0, w / 2)
    along = np.abs(YY + 0.5 - y) / max(1.0, h)
    tone_fill(img, m, 0.55 - side * 0.45 - along * 0.1, tones)
    return m


def crystal_cluster(img, cx, by, s, cols, halo):
    """cols: (dark, mid, light, white). Crystals of two facets each."""
    glow(img, cx, by - 12 * s, 34 * s, 22 * s, halo, 0.22)
    shapes = [(-9, 20, 4.5, -0.25), (-3, 34, 6.0, -0.08), (5, 26, 5.0, 0.12), (11, 16, 4.0, 0.3), (-14, 12, 3.4, -0.4)]
    for (dx, hh, w, lean) in shapes:
        x0 = cx + dx * s
        top = (x0 + lean * hh * s, by - hh * s)
        pts = [(x0 - w * s, by), (x0 - w * s * 0.9 + lean * hh * s * 0.8, by - hh * s * 0.78), top,
               (x0 + w * s * 0.9 + lean * hh * s * 0.8, by - hh * s * 0.78), (x0 + w * s, by)]
        m = poly_mask(pts)
        outline(img, m, '#0a0614')
        left = poly_mask([pts[0], pts[1], top, (x0 + lean * hh * s * 0.5, by)])
        paint(img, m, cols[1])
        paint(img, m & left, cols[2])
        paint(img, m & ~left & (YY > by - hh * s * 0.35), cols[0])
        line(img, [(x0 - w * s * 0.45 + lean * hh * s * 0.3, by - 3), (top[0] - 1, top[1] + 4)], cols[3], 0.4, m)


def grotta():
    img = canvas()
    gradient(img, 0, H, ['#0e0a1c', '#160f2a', '#1d1438', '#241a44', '#1a1232'])
    # the back wall: big dim stones, darker towards the top and the sides
    back = YY < 204
    vign = -0.35 * np.abs(XX - 170) / 170.0 - 0.3 * (1.0 - YY / 204.0)
    cells(img, back, 46, 21, ['#0f0a1e', '#130d25', '#18112d', '#1e1638', '#251b43'], '#0b0718',
          stretch=(1.4, 1.0), bias=vign)
    # a beam of light from a hole in the ceiling, with floating dust
    beam = poly_mask([(150, 0), (176, 0), (206, 200), (136, 200)])
    vfade(img, beam, '#b9a6ff', 0.22, 0.04, 0, 200)
    dust = np.random.default_rng(22)
    for _ in range(40):
        x, y = int(dust.uniform(140, 200)), int(dust.uniform(10, 190))
        if beam[y, x]:
            img[y, x, :3] = rgb('#d9ccff')
    # craggy side walls framing the stage, lit from the middle
    for (pts, lit_right) in (
            ([(0, 0), (52, 0), (44, 18), (50, 40), (36, 62), (42, 88), (28, 112), (38, 140), (24, 168),
              (32, 196), (0, 204)], True),
            ([(W, 0), (266, 0), (276, 22), (268, 46), (284, 70), (276, 98), (292, 126), (282, 154), (296, 180),
              (288, 200), (W, 206)], False)):
        m = poly_mask(pts)
        outline(img, m, '#0a0614')
        xs = np.array([p[0] for p in pts], dtype=np.float32)
        x_in = xs.max() if lit_right else xs.min()
        t = np.clip(1.0 - np.abs(XX + 0.5 - x_in) / 60.0, 0, 1) - 0.45
        cells(img, m, 16, 27 if lit_right else 28, ROCK, '#0a0614', L=(-0.8, -0.5) if lit_right else (0.8, -0.5),
              stretch=(1.0, 1.5), bias=t)
    # the ceiling and its stalactites
    ceil = poly_mask([(0, 0), (W, 0), (W, 10)] + [(x, 12 + 6 * np.sin(x / 17.0)) for x in range(W, -1, -8)])
    paint(img, ceil, '#120c20')
    for (x, w, h) in ((30, 14, 30), (52, 9, 18), (74, 12, 38), (98, 8, 16), (120, 11, 26), (196, 12, 30),
                      (216, 8, 18), (238, 13, 40), (262, 9, 22), (284, 12, 30), (304, 8, 18), (140, 7, 12),
                      (180, 6, 10)):
        spike(img, x, 8, w, h, ['#1c1430', '#2a1f46', '#3c2e60', '#54447e'])
        img[int(8 + h + 3), x, :3] = rgb('#9ad8ff')   # a drop
    worms = np.random.default_rng(23)
    for _ in range(30):   # glow-worms on the ceiling
        x, y = int(worms.uniform(0, W)), int(worms.uniform(4, 30))
        img[y, x, :3] = rgb('#7ff0dc')
    # the floor: rocky, with pebbles and small stalagmites
    floor = YY >= 200
    gradient(img, 200, H, ['#2a1f44', '#221a38', '#1a142c'])
    edge = poly_mask([(0, 206)] + [(x, 198 + 3 * np.sin(x / 11.0) + 2 * np.sin(x / 5.0)) for x in range(0, W + 4, 4)]
                     + [(W, 206)])
    paint(img, edge & (YY < 206), '#2a1f44')
    speckle(img, floor, '#3a2d5a', 0.07, 24)
    speckle(img, floor, '#140f22', 0.05, 25)
    pb = np.random.default_rng(26)
    for _ in range(22):
        x, y = pb.uniform(0, W), pb.uniform(206, 238)
        lobe(img, x, y, pb.uniform(2, 4.5), pb.uniform(1.4, 2.6), ['#1c1430', '#2e2350', '#4a3a70'], L=LIGHT_UL)
    for (x, w, h) in ((112, 10, 16), (124, 7, 10), (206, 9, 14), (196, 6, 9)):
        spike(img, x, 206, w, h, ['#1c1430', '#2a1f46', '#3c2e60', '#54447e'], down=False)
    # glowing crystals
    crystal_cluster(img, 26, 206, 1.25, ('#b0306e', '#ff5fa2', '#ff9ccb', '#ffe6f2'), '#ff5fa2')
    crystal_cluster(img, 60, 212, 0.75, ('#5a2ea0', '#9a5cf0', '#c9a0ff', '#f0e6ff'), '#9a5cf0')
    crystal_cluster(img, 292, 204, 1.35, ('#137a70', '#2fd3bd', '#8ff5e2', '#e6fffa'), '#2fd3bd')
    crystal_cluster(img, 262, 212, 0.8, ('#b0306e', '#ff5fa2', '#ff9ccb', '#ffe6f2'), '#ff5fa2')
    crystal_cluster(img, 160, 214, 0.6, ('#5a2ea0', '#9a5cf0', '#c9a0ff', '#f0e6ff'), '#9a5cf0')
    return img


# ------------------------------------------------------------------ the stormy summit (Tuonello)
STORM = ['#2a2e48', '#3a4060', '#4e5678', '#667092', '#8790b0', '#aab2cc']


def bolt(img, pts, core='#fffbe0', edge='#ffe066', halo='#fff4a0'):
    for (x, y) in pts[::2]:
        glow(img, x, y, 14, 14, halo, 0.18)
    line(img, pts, edge, 1.0)
    line(img, pts, core, 0.4)


def nuvole():
    img = canvas()
    gradient(img, 0, 200, ['#1c1f36', '#252a46', '#323858', '#434a6c', '#5a6184'])
    # far clouds, soft
    cloud(img, puff_row(-10, W + 20, 112, 14, 61, 5) + puff_row(10, W + 20, 120, 18, 64, 3),
          ['#3a4262', '#465072', '#545e82', '#646f94'], lo=0.1)
    # a lightning bolt far away on the right
    bolt(img, [(262, 108), (256, 124), (266, 128), (252, 150), (262, 152), (246, 176)])
    # the big storm clouds overhead
    cloud(img, [(20, 20, 40, 26), (64, 10, 36, 22), (40, 44, 30, 16), (0, 52, 26, 18)], STORM, ink='#171a2c')
    cloud(img, [(132, 6, 44, 24), (176, 16, 36, 22), (150, 32, 30, 14), (106, 26, 26, 14)], STORM, ink='#171a2c')
    cloud(img, [(250, 14, 42, 26), (296, 22, 38, 24), (272, 40, 30, 15), (318, 50, 24, 16), (222, 34, 22, 12)],
          STORM, ink='#171a2c')
    for (cx, cy) in ((60, 40), (170, 30), (270, 44)):   # dark bellies
        glow(img, cx, cy + 10, 50, 12, '#1d2034', 0.3)
    # rain, two layers
    rn = np.random.default_rng(62)
    for _ in range(170):
        x, y = rn.uniform(0, W), rn.uniform(50, 196)
        ln = rn.uniform(3, 6)
        line(img, [(x, y), (x - ln * 0.4, y + ln)], '#6c78a0', 0.3)
    for _ in range(60):
        x, y = rn.uniform(0, W), rn.uniform(60, 200)
        ln = rn.uniform(7, 11)
        line(img, [(x, y), (x - ln * 0.4, y + ln)], '#9aa6cc', 0.3)
    # we are above the clouds: a sea of cloud tops far below
    paint(img, rect(0, 184, W - 1, 200), '#5a6184')
    cloud(img, puff_row(-6, W + 20, 184, 13, 66, 3), ['#5a6184', '#6e779a', '#8a93b3', '#aab2cc', '#c8cee0'], lo=0.15)
    cloud(img, puff_row(-20, W + 20, 194, 16, 67, 2), ['#646d90', '#7a84a6', '#98a0be', '#b8bfd6', '#d4d9e8'], lo=0.2)
    # the rocky summit
    top = poly_mask([(0, 206), (26, 196), (58, 200), (96, 192), (132, 198), (170, 194), (204, 199), (246, 192),
                     (284, 198), (320, 194), (320, 240), (0, 240)])
    outline(img, top, '#141626')
    cells(img, top, 18, 63, ['#2c2c3e', '#3a3a50', '#4a4a62', '#5e5e78', '#76768e'], '#1a1a2a', stretch=(2.2, 1.0),
          bias=-0.25 * (YY - 196) / 44.0)
    paint(img, dilate4(top) & ~top & (YY < 204), '#8a8aa4')   # wet rock catching the light
    for lobes in ([(-4, 196, 22, 16), (16, 204, 16, 10), (4, 186, 12, 10)],
                  [(326, 194, 24, 18), (304, 204, 16, 10), (318, 182, 12, 10)]):   # boulders at the edges
        cloud(img, lobes, ['#2c2c3e', '#3e3e54', '#54546c', '#6c6c86', '#8a8aa4'], ink='#141626', band=0.5)
    for (cx, cy, rx) in ((120, 222, 16), (214, 230, 20)):   # puddles with rain rings
        pud = ellipse(cx, cy, rx, rx * 0.25)
        paint(img, pud, '#4a5578')
        paint(img, pud & (YY < cy - 0.5), '#6c78a0')
        rng_ring = ellipse(cx - rx * 0.3, cy, 3, 1) & ~ellipse(cx - rx * 0.3, cy, 2, 0.5)
        paint(img, rng_ring, '#aab2cc')
    return img


# ------------------------------------------------------------------ the volcano (Fumino), a night without stars
def lava_flow(img, pts, w=2.4):
    for (x, y) in pts[::2]:
        glow(img, x, y, 12, 12, '#ff7b2e', 0.16)
    line(img, pts, '#8a1c10', w + 1.0)
    line(img, pts, '#ff5a1e', w)
    line(img, pts, '#ffb040', w * 0.55)
    line(img, [(x + 0.4, y) for (x, y) in pts[::3]], '#fff0a0', 0.4)


def vulcano():
    img = canvas()
    gradient(img, 0, 200, ['#120a18', '#1c0f20', '#2a1424', '#3e1a26', '#5a2228'])
    glow(img, 160, 200, 220, 70, '#ff6a2a', 0.22)   # the red glow of the lava on the haze
    # a far volcano on the right, asleep
    far = poly_mask([(210, 200), (266, 118), (280, 116), (330, 200)])
    tone_fill(img, far, 0.3 - (XX - 270) / 120.0, ['#1e1220', '#2a1826', '#36202c'])
    # the big volcano
    cone = poly_mask([(20, 200), (122, 72), (134, 66), (186, 66), (198, 72), (300, 200)])
    outline(img, cone, '#0e0810')
    base_t = 0.55 - (XX - 160) / 150.0 * 0.5 - (YY - 66) / 134.0 * 0.15
    cells(img, cone, 34, 71, ['#1a1016', '#261820', '#34222a', '#463036', '#5c3e40'], '#120a0e',
          L=(-0.6, -0.5), stretch=(0.8, 1.6), bias=base_t - 0.55)
    for (x0, x1) in ((140, 96), (152, 132), (170, 196), (182, 236)):   # gullies down the slopes
        line(img, [(x0, 70), ((x0 + x1) / 2 + 3, 130), (x1, 196)], '#120a0e', 0.5, cone)
    # the crater, boiling
    glow(img, 160, 66, 60, 26, '#ff7b2e', 0.35)
    crater = ellipse(160, 67, 28, 5.5)
    paint(img, crater, '#8a1c10')
    paint(img, ellipse(160, 66, 24, 4), '#ff5a1e')
    paint(img, ellipse(158, 65.5, 16, 2.4), '#ffb040')
    paint(img, ellipse(154, 65, 7, 1.2), '#fff0a0')
    # lava running down
    lava_flow(img, [(146, 70), (140, 92), (146, 112), (136, 138), (142, 160), (130, 196)])
    lava_flow(img, [(176, 70), (184, 96), (178, 122), (190, 150), (186, 176)], 1.8)
    lava_flow(img, [(158, 71), (162, 88), (156, 100)], 1.4)
    # the smoke plume, lit from below by the fire
    sm = np.random.default_rng(74)
    smoke = []
    for k in range(16):   # a billowing column, leaning with the wind
        f = k / 15.0
        cx = 156 + f * 48 + sm.uniform(-6, 6)
        cy = 58 - f * 58 + sm.uniform(-3, 3)
        r = 7 + f * 12 + sm.uniform(-2, 2)
        smoke.append((cx + sm.uniform(-r * 0.6, r * 0.6), cy, r, r * 0.8))
    cloud(img, smoke, ['#1e161e', '#2a2028', '#3a2e34', '#4e3e42', '#665254'], L=(-0.3, -0.6, 0.7), ink='#140c12',
          band=0.4)
    for (x, y, r, ry) in smoke[:6]:   # the fire lights the smoke from below
        glow(img, x, y + ry * 0.6, r * 1.2, ry * 0.8, '#ff7b2e', 0.3)
    # the ground: black rock with glowing cracks
    gradient(img, 200, H, ['#241a26', '#1c1420', '#15101a'])
    rocks = poly_mask([(0, 204), (40, 198), (90, 202), (140, 197), (190, 201), (240, 196), (290, 202), (320, 198),
                       (320, 240), (0, 240)])
    cells(img, rocks, 20, 72, ['#140e16', '#1e1620', '#2a1e28', '#3a2830'], '#0c080c', stretch=(2.0, 1.0),
          bias=-0.2 * (YY - 200) / 40.0)
    for (pts) in ([(20, 222), (40, 218), (58, 226), (74, 224)], [(116, 234), (136, 228), (158, 232), (170, 226)],
                  [(208, 220), (224, 226), (248, 222), (262, 230)], [(284, 228), (300, 222), (318, 226)]):
        for (x, y) in pts:
            glow(img, x, y, 10, 5, '#ff7b2e', 0.25)
        line(img, pts, '#ff5a1e', 0.6)
        line(img, pts[1:-1], '#ffb040', 0.3)
    paint(img, dilate4(rocks) & ~rocks & (YY < 206), '#6a3a30')   # the edge lit by the fire
    for lobes in ([(-6, 196, 26, 18), (18, 206, 14, 9)], [(328, 192, 28, 20), (300, 206, 16, 9)]):
        cloud(img, lobes, ['#140e16', '#221822', '#34242c', '#5a3634', '#9a4a34'], L=(0.0, 0.9, 0.4), ink='#0c080c',
              band=0.5)
    em = np.random.default_rng(73)
    for _ in range(40):   # embers in the air
        x, y = int(em.uniform(0, W)), int(em.uniform(20, 196))
        img[y, x, :3] = rgb('#ffb040' if em.random() < 0.6 else '#ff5a1e')
    return img


# ------------------------------------------------------------------ the ice mountains (Nevone), a night without stars
def ice_mountain(img, x0, x1, top_x, top_y, base, tones_lit, tones_dark, snow, seed, ink=None):
    """Two faces: the left one lit, the right one in shadow; snow down the ridges."""
    rng = np.random.default_rng(seed)
    m = poly_mask([(x0, base), (top_x, top_y), (x1, base)])
    if ink:
        outline(img, m, ink)
    # the ridge wanders down from the top
    ridge = [(top_x, top_y)]
    y = top_y
    x = top_x
    while y < base:
        y += 8
        x += rng.uniform(-3, 5)
        ridge.append((x, y))
    rx = np.interp(YY, [p[1] for p in ridge], [p[0] for p in ridge])
    lit = m & (XX < rx)
    tone_fill(img, lit, 0.9 - (YY - top_y) / max(1, base - top_y) * 0.6, tones_lit, band=0.5)
    tone_fill(img, m & ~lit, 0.7 - (YY - top_y) / max(1, base - top_y) * 0.5, tones_dark, band=0.5)
    # snow: a cap and streaks along the gullies
    cap_y = top_y + (base - top_y) * rng.uniform(0.28, 0.36)
    cap = m & (YY < cap_y + 3 * np.sin(XX / 3.0))
    paint(img, cap & lit, snow[1])
    paint(img, cap & ~lit, snow[0])
    for k in range(5):
        gx = top_x + rng.uniform(-0.6, 0.6) * (x1 - x0) * 0.25
        gy = cap_y - 2
        pts = [(gx, gy)]
        for j in range(4):
            pts.append((pts[-1][0] + (pts[-1][0] - top_x) * 0.25 + rng.uniform(-2, 2), pts[-1][1] + rng.uniform(5, 9)))
        line(img, pts, snow[1] if pts[0][0] < top_x else snow[0], 0.5, m)
    return m


def ghiaccio():
    img = canvas()
    gradient(img, 0, 196, ['#070a22', '#0c1234', '#141c48', '#1f2a5c', '#2b3a70'])
    crescent_m = ellipse(252, 40, 13, 13) & ~ellipse(258, 36, 11, 11)
    glow(img, 250, 40, 34, 34, '#dfe8ff', 0.16)
    paint(img, dilate4(crescent_m) & ~crescent_m & ~ellipse(258, 36, 11, 11), '#b8c4e8')
    paint(img, crescent_m, '#f0f4ff')
    crescent_face(img, 252, 40, 13, '#6a78a8')
    # far range, pale and cold
    for (x0, x1, tx, ty, s) in ((-40, 110, 34, 64, 81), (70, 230, 150, 40, 82), (180, 330, 262, 58, 83),
                                (250, 380, 330, 74, 84)):
        ice_mountain(img, x0, x1, tx, ty, 178, ['#3a4a80', '#4a5c94', '#5c70a8'], ['#26315e', '#2e3a6c', '#36447a'],
                     ('#6a7cb4', '#8fa2d4'), s)
    vfade(img, (YY > 100) & (YY < 190), '#2b3a70', 0.0, 0.45, 110, 180)
    # near peaks at the edges, bright ice
    for (x0, x1, tx, ty, s) in ((-50, 84, 14, 104, 85), (238, 370, 306, 98, 86)):
        ice_mountain(img, x0, x1, tx, ty, 204, ['#8ab4e8', '#aacdf4', '#cfe6ff', '#eef7ff'],
                     ['#4a6ab0', '#5a7cc0', '#6e90d0'], ('#c0d4f0', '#ffffff'), s, ink='#1c2350')
    # a frozen lake, the moon on the ice
    lake = ellipse(160, 206, 110, 12) & (YY > 196)
    paint(img, rect(0, 196, W - 1, 206), '#b8c8e8')
    gradient(img, 200, H, ['#d4e0f4', '#c4d2ee', '#b0c2e4'])
    tone_fill(img, lake, 0.3 + (XX - 60) / 400.0, ['#5a78b8', '#6e8cc8', '#86a4d8'], band=0.5)
    for (x0, y0, x1, y1) in ((80, 204, 120, 208), (140, 200, 160, 212), (200, 203, 236, 207)):   # cracks in the ice
        line(img, [(x0, y0), ((x0 + x1) / 2, (y0 + y1) / 2 + 1), (x1, y1)], '#b8d0f4', 0.4, lake)
    for y in range(198, 214, 2):   # the moon's reflection
        w = 3 - abs(y - 205) // 3
        if w > 0:
            paint(img, rect(252 - w, y, 252 + w, y) & lake, '#dfe8ff')
    # snow drifts and ice crystals in front
    for lobes in ([(-8, 214, 40, 14), (30, 222, 30, 10)], [(330, 212, 44, 16), (290, 224, 28, 9)],
                  [(150, 236, 50, 8)]):
        cloud(img, lobes, ['#9fb2dc', '#b8c8e8', '#d4e0f4', '#eef4ff', '#ffffff'], L=(-0.4, -0.8, 0.5), band=0.5)
    for (cx, by, s) in ((34, 208, 1.0), (62, 212, 0.7), (262, 210, 0.9), (292, 206, 1.1)):
        for (dx, hh, w) in ((-6, 20, 4), (0, 30, 5), (6, 16, 4)):
            pts = [(cx + dx - w * s, by), (cx + dx, by - hh * s), (cx + dx + w * s, by)]
            m = poly_mask(pts)
            outline(img, m, '#1c2350')
            paint(img, m, '#8ff5e2')
            paint(img, m & (XX < cx + dx), '#e6fffa')
            paint(img, m & (XX > cx + dx + w * s * 0.4), '#2fd3bd')
    fl = np.random.default_rng(87)
    for _ in range(110):   # falling snow
        x, y = int(fl.uniform(0, W)), int(fl.uniform(0, 236))
        img[y, x, :3] = rgb('w')
        if fl.random() < 0.2 and x + 1 < W:
            img[y, x + 1, :3] = rgb('#c8d4f0')
    return img


# ------------------------------------------------------------------ the dressing room
def camerino():
    """The vanity room: same mirror (glass 18..102 x 32..170), warmer and richer."""
    from pixel import rect_mask, ellipse_mask
    img = canvas()
    stripe = (XX // 10) % 2 == 0
    paint(img, stripe, '#ffd6e8')
    paint(img, ~stripe, '#fff0f6')
    paint(img, stripe & ((XX % 10) == 0), '#f7c4dc')
    for y in range(8, 180, 20):   # little hearts on the light stripes
        for x in range(13, W, 20):
            hx = x + (10 if (y // 20) % 2 else 0)
            if hx + 3 < W and not stripe[y, hx]:
                for j, row in enumerate(["k.k", "kkk", ".k."]):
                    for i, ch in enumerate(row):
                        if ch == 'k':
                            img[y + j, hx + i, :3] = rgb('#ffb3d1')
    vfade(img, YY < 60, '#e9a6c6', 0.3, 0.0, 0, 60)          # a softer ceiling
    paint(img, rect(0, 0, W - 1, 3), '#f7c4dc')              # cornice
    paint(img, rect(0, 4, W - 1, 4), '#e39cbd')
    # wall moulding (dado rail) with a little shading
    paint(img, rect(0, 176, W - 1, 181), '#fff4fa')
    paint(img, rect(0, 176, W - 1, 176), '#ffffff')
    paint(img, rect(0, 182, W - 1, 182), '#f0a8c8')
    paint(img, rect(0, 183, W - 1, 188), '#ffe4ef')
    # the mirror: warm light around it first
    glow(img, 60, 100, 76, 96, '#ffe6a0', 0.35)
    mx0, my0, mx1, my1 = 12, 26, 108, 176
    frame = rect_mask(W, H, mx0, my0, mx1, my1, radius=14)
    glass = rect_mask(W, H, mx0 + 6, my0 + 6, mx1 - 6, my1 - 6, radius=10)
    outline(img, frame, 'k')
    inner = rect_mask(W, H, mx0 + 1, my0 + 1, mx1 - 1, my1 - 1, radius=13)
    paint(img, frame, '#ffd23f')
    paint(img, frame & ~inner, '#f0a030')
    mid = rect_mask(W, H, mx0 + 3, my0 + 3, mx1 - 3, my1 - 3, radius=12)
    paint(img, frame & inner & ~mid & ((XX < 60) & (YY < 100)), '#fff3a6')   # the bevel catches the light
    # the glass: a pale sky-blue with the room's pink glow at the bottom and two shines
    tone_fill(img, glass, 0.8 - (YY - 32) / 139.0 * 0.5, ['#f3d6ec', '#e2e4fa', '#d9ecff', '#e8f4ff'], band=0.6)
    shine = glass & ((((XX - YY) % 44) < 5) | (((XX - YY) % 44) == 8))
    paint(img, shine, '#f6fbff')
    paint(img, dilate4(glass) & ~glass & frame, '#b86b1c')
    for bx, by in [(mx0 + 3, y) for y in range(my0 + 14, my1 - 8, 20)] + \
                  [(mx1 - 3, y) for y in range(my0 + 14, my1 - 8, 20)] + \
                  [(x, my0 + 3) for x in range(mx0 + 16, mx1 - 10, 20)]:
        b = ellipse_mask(W, H, bx + 0.5, by + 0.5, 3.4, 3.4)
        glow(img, bx + 0.5, by + 0.5, 9, 9, '#fff8d0', 0.3, mask=~glass)
        outline(img, b, '#f0a030')
        lobe(img, bx + 0.5, by + 0.5, 3.4, 3.4, ['#ffe27a', '#fff3a6', '#ffffff'], L=(-0.5, -0.6, 0.6))
    # the vanity table: wood with a lighter top and a front edge
    top = rect(0, 189, W - 1, 196)
    paint(img, rect(0, 188, W - 1, 188), 'k')
    tone_fill(img, top, 0.75 - (YY - 189) / 8.0 * 0.4, ['#e6a877', '#f6c89f', '#ffe0c4'], band=0.5)
    paint(img, rect(0, 197, W - 1, 198), '#c98a5c')
    gradient(img, 199, H, ['#f2bd90', '#e8ae80', '#dc9e70'])
    for y in (206, 220, 234):
        line(img, [(x, y + 1.5 * np.sin(x / 23.0)) for x in range(0, W + 8, 8)], '#d99a6c', 0.4)
    # little things on the table, under the mirror
    bottle = rect_mask(W, H, 16, 176, 26, 188, radius=3) | rect(19, 172, 23, 176)
    outline(img, bottle, 'k')
    tone_fill(img, bottle, 0.6 - (XX - 16) / 12.0, ['#7d3fc4', '#b376ec', '#cbabf2', '#f0e6ff'], band=0.5)
    paint(img, rect(18, 169, 24, 171), '#ffd23f')
    outline(img, rect(18, 169, 24, 171), 'k')
    puff = ellipse(96, 185, 9, 4)
    outline(img, puff, 'k')
    lobe(img, 96, 185, 9, 4, ['#ff93c6', '#ffcbe3', '#fff4fa'])
    box = rect(34, 180, 50, 188)
    outline(img, box, 'k')
    tone_fill(img, box, 0.5 - (XX - 34) / 32.0, ['#26a39a', '#4fd6c0', '#bdf5ea'], band=0.5)
    paint(img, rect(34, 183, 50, 183), '#ffd23f')
    # the glass shelf behind the reward cards, on two gold brackets
    shelf = rect(124, 164, 313, 168)
    outline(img, shelf, 'k')
    paint(img, shelf, '#e8f4ff')
    paint(img, rect(124, 164, 313, 164), '#ffffff')
    paint(img, rect(124, 168, 313, 168), '#b8cfe8')
    for x in (136, 300):
        br = poly_mask([(x - 3, 169), (x + 3, 169), (x, 176)])
        paint(img, br, '#f0a030')
    return img
