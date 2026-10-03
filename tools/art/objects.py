"""Countable objects, reward icons and small props, at any scale.

Shapes are drawn on a virtual 24x24 grid and rasterised at 24*K pixels, so the
same generator yields the 24 px sprites (dense grids of up to 20 objects) and
the 36 px ones (few objects, scattered). Outlines and rim shading stay 1 px
wide at every size, like the rest of the pixel art.
"""
import numpy as np
from pixel import (blank, blit, ellipse_mask as _ellipse, capsule_mask as _capsule,
                   rect_mask as _rect, shaded_part, rgb, ascii_sprite, flip_h)

K = 1.0


def size():
    return int(round(24 * K))


def poly_mask(w, h, pts, ss=4):
    """Supersampled polygon fill -> boolean mask (pixel filled when >= 50%)."""
    yy, xx = np.mgrid[0:h * ss, 0:w * ss]
    px, py = (xx + 0.5) / ss, (yy + 0.5) / ss
    inside = np.zeros(px.shape, dtype=bool)
    n = len(pts)
    for i in range(n):
        x0, y0 = pts[i]
        x1, y1 = pts[(i + 1) % n]
        cond = ((y0 > py) != (y1 > py))
        xint = x0 + (py - y0) * (x1 - x0) / ((y1 - y0) if y1 != y0 else 1e-9)
        inside ^= cond & (px < xint)
    cov = inside.reshape(h, ss, w, ss).mean(axis=(1, 3))
    return cov >= 0.5


# ---- scale-aware primitives (coordinates on the virtual 24 grid)
def E(cx, cy, rx, ry):
    s = size()
    return _ellipse(s, s, cx * K, cy * K, rx * K, ry * K)


def C(p0, p1, r):
    s = size()
    return _capsule(s, s, (p0[0] * K, p0[1] * K), (p1[0] * K, p1[1] * K), r * K)


def R(x0, y0, x1, y1, radius=0):
    s = size()
    return _rect(s, s, int(round(x0 * K)), int(round(y0 * K)), int(round((x1 + 1) * K)) - 1,
                 int(round((y1 + 1) * K)) - 1, int(round(radius * K)))


def POLY(pts):
    s = size()
    return poly_mask(s, s, [(x * K, y * K) for x, y in pts])


def cell(x, y):
    """Pixel block covering virtual pixel (x, y)."""
    return (int(round(x * K)), int(round((x + 1) * K)), int(round(y * K)), int(round((y + 1) * K)))


def put(img, x, y, c, mask=None):
    x0, x1, y0, y1 = cell(x, y)
    for yy in range(y0, max(y1, y0 + 1)):
        for xx in range(x0, max(x1, x0 + 1)):
            if 0 <= yy < img.shape[0] and 0 <= xx < img.shape[1]:
                if mask is not None and not mask[yy, xx]:
                    continue
                if mask is not None and img[yy, xx, :3].tolist() == list(rgb('k')):
                    continue
                img[yy, xx, :3] = rgb(c)
                img[yy, xx, 3] = 255


def canvas():
    s = size()
    return blank(s, s)


def face(img, cx, cy, gap=4, blush=True):
    """Kawaii face centred on virtual (cx, cy): eyes, smile, blush."""
    if K <= 1.01:
        for ex in (cx - gap // 2 - 1, cx + gap // 2):
            put(img, ex, cy, 'k')
            put(img, ex, cy + 1, 'k')
        for (x, y) in ((cx - 1, cy + 3), (cx, cy + 3), (cx - 2, cy + 2), (cx + 1, cy + 2)):
            put(img, x, y, 'k')
        if blush:
            put(img, cx - gap // 2 - 3, cy + 2, 'c')
            put(img, cx + gap // 2 + 2, cy + 2, 'c')
        return
    # larger sprites: real-pixel face (2x3 eyes with a shine, wider smile)
    X, Y = int(round(cx * K)), int(round(cy * K))
    g = int(round(gap * K / 2)) + 1
    def px(x, y, c):
        if 0 <= y < img.shape[0] and 0 <= x < img.shape[1]:
            img[y, x, :3] = rgb(c)
            img[y, x, 3] = 255
    for ex in (X - g - 1, X + g - 1):
        for dy in range(3):
            px(ex, Y + dy, 'k')
            px(ex + 1, Y + dy, 'k')
        px(ex + 1, Y, 'w')
    for x in range(X - 2, X + 2):
        px(x, Y + 5, 'k')
    px(X - 3, Y + 4, 'k')
    px(X + 2, Y + 4, 'k')
    if blush:
        for dx in (0, 1):
            px(X - g - 4 + dx, Y + 4, 'c')
            px(X + g + 2 + dx, Y + 4, 'c')


def star_mask_px(w, h, cx, cy, R_, r_, rot=-90):
    pts = []
    for i in range(10):
        a = np.deg2rad(rot + i * 36)
        rr = R_ if i % 2 == 0 else r_
        pts.append((cx + rr * np.cos(a), cy + rr * np.sin(a)))
    return poly_mask(w, h, pts)


def star_mask(w=None, h=None, cx=12, cy=12.8, R=11.2, r=5.2, rot=-90):
    """Star on explicit pixel geometry (used by the UI) or on the virtual grid."""
    if w is not None:
        return star_mask_px(w, h, cx, cy, R, r, rot)
    s = size()
    return star_mask_px(s, s, cx * K, cy * K, R * K, r * K, rot)


# ------------------------------------------------------------------ objects
def stella():
    img = shaded_part(star_mask(), 'Y', 'y', 'o')
    face(img, 12, 12)
    return img


def cuore():
    m = E(7.6, 8.6, 5.6, 5.4) | E(16.4, 8.6, 5.6, 5.4) | POLY([(2.4, 10.0), (21.6, 10.0), (12.0, 21.6)])
    img = shaded_part(m, 'h', 'P', 'H')
    for (x, y) in ((6, 6), (6, 7), (7, 5), (5, 7)):
        put(img, x, y, 'p', m)
    face(img, 12, 10)
    return img


def rossetto(bullet='r', light='P', dark='R'):
    img = canvas()
    blit(img, shaded_part(POLY([(9, 10.5), (9, 4.5), (13.5, 1.5), (15, 3), (15, 10.5)]), bullet, light, dark), 0, 0)
    blit(img, shaded_part(R(8, 10, 15, 12), 'S', 's', 'x'), 0, 0)
    tube = R(7, 13, 16, 21, radius=1)
    blit(img, shaded_part(tube, 'Y', 'y', 'o'), 0, 0)
    for y in range(15, 20):
        put(img, 9, y, 'y', tube)
    return img


def smalto(color='v', light='L', dark='V'):
    img = canvas()
    bottle = R(5, 10, 18, 21, radius=3)
    blit(img, shaded_part(bottle, color, light, dark), 0, 0)
    blit(img, shaded_part(R(8, 9, 15, 9), 'S', 's', 'x'), 0, 0)
    blit(img, shaded_part(R(9, 1, 14, 8, radius=1), 'x', 'S', 'k'), 0, 0)
    for y in range(12, 18):
        put(img, 7, y, 'w', bottle)
    put(img, 8, 12, 'w', bottle)
    for (x, y) in ((11, 15), (13, 15), (10, 16), (11, 16), (12, 16), (13, 16), (14, 16),
                   (11, 17), (12, 17), (13, 17), (12, 18)):
        put(img, x, y, 'p', bottle)
    return img


def fiocco(c='B', light='b', dark='n'):
    img = canvas()
    half = canvas()
    blit(half, shaded_part(POLY([(10.5, 12), (6, 22), (9, 21), (10, 22.5), (12.5, 13)]), c, light, dark), 0, 0)
    loop = POLY([(11, 9.5), (2.5, 3.5), (1.5, 6), (1.5, 14), (2.5, 16.5), (11, 12.5)])
    blit(half, shaded_part(loop, c, light, dark), 0, 0)
    for (x, y) in ((4, 8), (5, 9), (4, 12), (5, 11)):
        put(half, x, y, dark, loop)
    blit(img, half, 0, 0)
    blit(img, flip_h(half), 0, 0)
    knot = E(12, 11, 3.2, 3.4)
    blit(img, shaded_part(knot, c, light, dark), 0, 0)
    put(img, 11, 10, light, knot)
    return img


def microfono():
    img = canvas()
    handle = C((12, 12), (12, 21.5), 2.6)
    blit(img, shaded_part(handle, 'h', 'P', 'H'), 0, 0)
    blit(img, shaded_part(R(8, 10, 15, 11, radius=1), 'Y', 'y', 'o'), 0, 0)
    head = E(12, 6.5, 5.6, 5.6)
    part = shaded_part(head, 'S', 's', 'x')
    ys, xs = np.where(head)
    for y, x in zip(ys, xs):
        if (x + y) % 2 == 0 and part[y, x, :3].tolist() == list(rgb('S')):
            part[y, x, :3] = rgb('x')
    blit(img, part, 0, 0)
    for (x, y) in ((9, 3), (9, 4), (10, 3)):
        put(img, x, y, 'w', head)
    for y in range(14, 20):
        put(img, 11, y, 'p', handle)
    return img


def scarpetta():
    """Ballet slipper, side view, with crossed satin ribbons."""
    img = canvas()
    blit(img, shaded_part(C((13.0, 13.5), (6.0, 3.0), 0.95), 'h', 'P', 'H', rim=False), 0, 0)
    blit(img, shaded_part(C((8.0, 13.5), (15.0, 3.0), 0.95), 'h', 'P', 'H', rim=False), 0, 0)
    s = size()
    yy, xx = np.mgrid[0:s, 0:s]
    body = E(11.0, 16.5, 9.4, 4.6) | E(16.5, 16.0, 5.6, 4.4)
    opening = (yy + 0.5 < 13.6 * K) & (xx >= 4 * K) & (xx <= 14.9 * K)
    m = body & ~opening
    part = shaded_part(m, 'P', 'p', 'h')
    # lining at the opening, glossy toe, darker sole
    top = {}
    for y, x in zip(*np.where(m)):
        top[x] = min(top.get(x, 999), y)
    for x, y in top.items():
        if 4 * K <= x <= 15 * K and part[y, x, :3].tolist() != list(rgb('k')):
            part[y, x, :3] = rgb('W')
    for (x, y) in ((17, 13), (18, 13), (19, 14), (16, 13)):
        put(part, x, y, 'w', m)
    bottom = {}
    for y, x in zip(*np.where(m)):
        bottom[x] = max(bottom.get(x, -1), y)
    for x, y in bottom.items():
        if part[y, x, :3].tolist() != list(rgb('k')):
            part[y, x, :3] = rgb('H')
    blit(img, part, 0, 0)
    return img


def farfalla():
    img = canvas()
    half = canvas()
    lo = E(7.5, 16.5, 4.2, 4.0)
    up = E(6.2, 7.5, 5.4, 5.8)
    blit(half, shaded_part(lo, 'T', 't', 'e'), 0, 0)
    blit(half, shaded_part(up, 'T', 't', 'e'), 0, 0)
    for (x, y) in ((5, 6), (6, 6), (5, 7), (6, 7)):
        put(half, x, y, 'Y', up)
    for (x, y) in ((7, 16), (7, 17)):
        put(half, x, y, 'Y', lo)
    put(half, 4, 5, 'w', up)
    blit(img, half, 0, 0)
    blit(img, flip_h(half), 0, 0)
    blit(img, shaded_part(C((12, 6.5), (12, 19.5), 1.8), 'V', 'v', 'k'), 0, 0)
    for (x, y) in ((10, 4), (9, 3), (8, 2), (13, 4), (14, 3), (15, 2)):
        put(img, x, y, 'k')
    put(img, 8, 1, 'h')
    put(img, 15, 1, 'h')
    return img


def fiore(petal='w', light='w', dark='S'):
    img = canvas()
    for i in range(5):
        a = np.deg2rad(-90 + i * 72)
        blit(img, shaded_part(E(12 + 6.2 * np.cos(a), 12 + 6.2 * np.sin(a), 4.4, 4.4), petal, light, dark), 0, 0)
    blit(img, shaded_part(E(12, 12, 3.6, 3.6), 'Y', 'y', 'o'), 0, 0)
    face(img, 12, 11, gap=2, blush=False)
    return img


def diamante():
    top = POLY([(6, 4), (18, 4), (22.5, 9.5), (1.5, 9.5)])
    bot = POLY([(1.5, 9.5), (22.5, 9.5), (12, 22)])
    m = top | bot
    part = shaded_part(m, 'B', 'b', 'n')

    def line(x0, y0, x1, y1):
        n = int(max(abs(x1 - x0), abs(y1 - y0)) * K) + 1
        for i in range(n + 1):
            x = int(round((x0 + (x1 - x0) * i / n) * K))
            y = int(round((y0 + (y1 - y0) * i / n) * K))
            if 0 <= y < m.shape[0] and 0 <= x < m.shape[1] and m[y, x] \
                    and part[y, x, :3].tolist() != list(rgb('k')):
                part[y, x, :3] = rgb('n')
    line(2.5, 9.5, 21.5, 9.5)
    for seg in ((8, 5, 6, 9), (16, 5, 18, 9), (6, 10, 12, 20), (18, 10, 12, 20), (12, 10, 12, 20)):
        line(*seg)
    for (x, y) in ((8, 6), (9, 6), (7, 7), (5, 8), (6, 11), (7, 12), (8, 13)):
        put(part, x, y, 'w', m)
    return part



# ------------------------------------------------------------------ word pictures (Parole)
def palla():
    s = size()
    m = E(12, 12, 10.2, 10.2)
    part = shaded_part(m, 'w', None, None)
    cols = ['r', 'W', 'B', 'W', 'Y', 'W']
    ys, xs = np.where(m)
    for y, x in zip(ys, xs):
        a = (np.degrees(np.arctan2(y + 0.5 - 12 * K, x + 0.5 - 12 * K)) + 360 + 15) % 360
        part[y, x, :3] = rgb(cols[int(a // 60)])
    ring = E(12, 12, 9.2, 9.2)
    edge = m & ~ring
    for y, x in zip(*np.where(edge)):
        if y > 12 * K or x > 14 * K:
            part[y, x, :3] = (np.array(part[y, x, :3]) * 0.8).astype(np.uint8)
    blit(part, shaded_part(E(12, 12, 2.2, 2.2), 'W', 'w', 'S'), 0, 0)
    for (x, y) in ((6, 6), (7, 5), (6, 7)):
        put(part, x, y, 'w', m)
    return part


def mela():
    img = canvas()
    body = E(9, 14, 7.2, 7.6) | E(15, 14, 7.2, 7.6) | E(12, 17, 7, 5.4)
    blit(img, shaded_part(body, 'r', 'P', 'R'), 0, 0)
    blit(img, shaded_part(C((12, 7.5), (13.5, 3.5), 0.9), 'O', 'F', 'k', rim=False), 0, 0)
    blit(img, shaded_part(E(16.5, 5.0, 3.2, 1.8), 'G', 'g', 'd'), 0, 0)
    for (x, y) in ((6, 11), (6, 12), (7, 10), (5, 13)):
        put(img, x, y, 'w', body)
    return img


def luna():
    m = E(12, 12, 10, 10) & ~E(16.5, 9, 8.2, 8.2)
    img = shaded_part(m, 'Y', 'y', 'o')
    for (x, y) in ((6, 12), (7, 13), (8, 13), (9, 12)):
        put(img, x, y, 'O', m)
    put(img, 5, 15, 'c', m)
    return img


def sole():
    img = canvas()
    for i in range(8):
        a = np.deg2rad(i * 45 + 22.5)
        blit(img, shaded_part(C((12 + 7.5 * np.cos(a), 12 + 7.5 * np.sin(a)),
                                (12 + 10.8 * np.cos(a), 12 + 10.8 * np.sin(a)), 1.5), 'o', 'Y', 'O'), 0, 0)
    blit(img, shaded_part(E(12, 12, 7.2, 7.2), 'Y', 'y', 'o'), 0, 0)
    face(img, 12, 11)
    return img


def pesce():
    img = canvas()
    tail = POLY([(5.5, 12), (1, 6.5), (1.5, 17.5)])
    blit(img, shaded_part(tail, 'o', 'F', 'O'), 0, 0)
    body = E(13.5, 12, 9, 6.4)
    blit(img, shaded_part(body, 'F', 'f', 'o'), 0, 0)
    fin = POLY([(11, 7), (15, 3.5), (17, 6.5)])
    blit(img, shaded_part(fin, 'o', 'F', 'O'), 0, 0)
    for (x, y) in ((10, 10), (11, 13), (9, 14), (12, 11)):
        put(img, x, y, 'o', body)
    blit(img, shaded_part(E(17.5, 10, 1.25, 1.25), 'k', None, None, outline='k'), 0, 0)
    put(img, 17, 9, 'w')
    put(img, 21, 13, 'k')
    put(img, 20, 14, 'k')
    return img


def casa():
    img = canvas()
    walls = R(4, 11, 19, 21, radius=1)
    blit(img, shaded_part(walls, 'W', 'w', 's'), 0, 0)
    roof = POLY([(1.5, 11.5), (12, 2.5), (22.5, 11.5)])
    blit(img, shaded_part(roof, 'r', 'P', 'R'), 0, 0)
    blit(img, shaded_part(R(10, 14, 13, 21), 'O', 'F', 'k'), 0, 0)
    blit(img, shaded_part(R(5, 13, 8, 16), 'B', 'b', 'n'), 0, 0)
    blit(img, shaded_part(R(15, 13, 18, 16), 'B', 'b', 'n'), 0, 0)
    put(img, 12, 17, 'Y')
    return img


def torta():
    img = canvas()
    blit(img, shaded_part(R(3, 13, 20, 21, radius=2), 'F', 'f', 'O'), 0, 0)
    top = R(4, 8, 19, 13, radius=2)
    blit(img, shaded_part(top, 'P', 'p', 'h'), 0, 0)
    for x in (5, 8, 11, 14, 17):
        put(img, x, 14, 'P')
        put(img, x, 15, 'P')
    blit(img, shaded_part(E(12, 6.5, 2.2, 2.2), 'r', 'P', 'R'), 0, 0)
    blit(img, shaded_part(R(11, 1, 12, 4), 'b', 'w', 'B'), 0, 0)
    put(img, 11, 0, 'Y')
    for (x, y) in ((6, 18), (10, 18), (14, 18), (18, 18)):
        put(img, x, y, 'y')
    return img


def banana():
    outer = E(12, 7, 11, 13.5)
    inner = E(13.5, 4.5, 10.5, 12.5)
    m = outer & ~inner
    s = size()
    yy, xx = np.mgrid[0:s, 0:s]
    m &= yy > 7 * K
    img = shaded_part(m, 'Y', 'y', 'o')
    ys, xs = np.where(m)
    for y, x in zip(ys, xs):
        if x < 3.3 * K or x > 21 * K:
            img[y, x, :3] = rgb('O')
    return img


def gelato():
    img = canvas()
    cone = POLY([(6, 11), (18, 11), (12, 23)])
    part = shaded_part(cone, 'F', 'f', 'o')
    ys, xs = np.where(cone)
    for y, x in zip(ys, xs):
        if ((x + y) % int(4 * K) == 0 or (x - y) % int(4 * K) == 0) and part[y, x, :3].tolist() != list(rgb('k')):
            part[y, x, :3] = rgb('o')
    blit(img, part, 0, 0)
    blit(img, shaded_part(E(12, 9.5, 6.4, 4), 't', 'w', 'T'), 0, 0)
    blit(img, shaded_part(E(12, 5, 5, 4), 'P', 'p', 'h'), 0, 0)
    blit(img, shaded_part(E(12, 1.8, 1.6, 1.6), 'r', 'P', 'R'), 0, 0)
    return img


def palloncino():
    img = canvas()
    for y in range(15, 24):
        x = 12 + int(round(1.2 * np.sin(y * 0.9)))
        put(img, x, y, 'x')
    body = E(12, 8.5, 7, 8.2)
    blit(img, shaded_part(body, 'h', 'P', 'H'), 0, 0)
    blit(img, shaded_part(POLY([(10.5, 17.5), (13.5, 17.5), (12, 15.5)]), 'H', 'h', 'k'), 0, 0)
    for (x, y) in ((8, 4), (8, 5), (9, 3), (7, 6)):
        put(img, x, y, 'w', body)
    return img


def ape():
    img = canvas()
    for sgn in (-1, 1):
        blit(img, shaded_part(E(12 + sgn * 4.5, 6, 4, 4.4), 'b', 'w', 'B'), 0, 0)
    body = E(12, 14, 8.2, 6.4)
    part = shaded_part(body, 'Y', 'y', 'o')
    ys, xs = np.where(body)
    for y, x in zip(ys, xs):
        if int((x / K - 4) // 3) % 2 == 1 and part[y, x, :3].tolist() != list(rgb('k')):
            part[y, x, :3] = rgb('K')
    blit(img, part, 0, 0)
    blit(img, shaded_part(E(19.5, 13, 3.6, 3.6), 'Y', 'y', 'o'), 0, 0)
    put(img, 20, 12, 'k')
    put(img, 21, 14, 'c')
    blit(img, shaded_part(POLY([(3.5, 13), (0.5, 14), (3.5, 15.5)]), 'K', 'x', 'k', rim=False), 0, 0)
    return img


def uva():
    img = canvas()
    blit(img, shaded_part(C((12, 5), (13.5, 1.5), 0.8), 'O', 'F', 'k', rim=False), 0, 0)
    blit(img, shaded_part(E(15.5, 3.5, 3.2, 1.8), 'G', 'g', 'd'), 0, 0)
    for (x, y) in ((8, 8), (12, 8), (16, 8), (6, 12), (10, 12), (14, 12), (18, 12), (8, 16), (12, 16),
                   (16, 16), (10, 20), (14, 20)):
        blit(img, shaded_part(E(x, y, 2.6, 2.6), 'v', 'L', 'V'), 0, 0)
    return img


def uovo():
    s = size()
    m = (((PX_(s) - 12 * K) / (7.6 * K)) ** 2 + ((PY_(s) - 13 * K) / (9.8 * K * np.where(PY_(s) < 13 * K, 1.0, 0.82))) ** 2) <= 1
    img = shaded_part(m, 'f', 'W', 'F')
    for (x, y) in ((8, 8), (8, 9), (9, 7)):
        put(img, x, y, 'w', m)
    return img


def PX_(s):
    return np.mgrid[0:s, 0:s][1] + 0.5


def PY_(s):
    return np.mgrid[0:s, 0:s][0] + 0.5


def ombrello():
    img = canvas()
    for y in range(12, 21):
        put(img, 12, y, 'x')
    for (x, y) in ((12, 21), (11, 22), (10, 22), (9, 21)):
        put(img, x, y, 'x')
    s = size()
    dome = E(12, 12, 10.8, 9) & (PY_(s) < 12 * K)
    scallop = np.zeros_like(dome)
    for cx in (3.6, 9.2, 14.8, 20.4):
        scallop |= E(cx, 12, 2.8, 1.8)
    m = dome | (scallop & (PY_(s) >= 11 * K) & E(12, 12, 10.9, 12))
    part = shaded_part(m, 'h', 'P', 'H')
    ys, xs = np.where(m)
    for y, x in zip(ys, xs):
        if int(((x / K) - 1.2) // 5.6) % 2 == 1 and part[y, x, :3].tolist() not in (list(rgb('k')),):
            part[y, x, :3] = rgb('W') if part[y, x, :3].tolist() == list(rgb('h')) else part[y, x, :3]
    blit(img, part, 0, 0)
    put(img, 12, 2, 'x')
    return img


def isola():
    img = canvas()
    s = size()
    sea = R(0, 18, 23, 23)
    part = shaded_part(sea, 'B', 'b', 'n', rim=False)
    blit(img, part, 0, 0)
    blit(img, shaded_part(E(11, 19, 9, 3.6), 'y', 'w', 'o'), 0, 0)
    trunk = C((12, 18), (14.5, 7), 1.1)
    blit(img, shaded_part(trunk, 'O', 'F', 'k'), 0, 0)
    for a in (-160, -120, -60, -20, 40):
        r = np.deg2rad(a)
        blit(img, shaded_part(C((14.5, 7), (14.5 + 7 * np.cos(r), 7 + 5 * np.sin(r) + 1.5), 1.4), 'G', 'g', 'd'), 0, 0)
    blit(img, shaded_part(E(13.5, 8, 1.4, 1.4), 'O', 'F', 'k'), 0, 0)
    return img


def elefante():
    img = canvas()
    for sgn in (-1, 1):
        blit(img, shaded_part(E(12 + sgn * 7.6, 10, 4.6, 6.2), 'S', 's', 'x'), 0, 0)
        blit(img, shaded_part(E(12 + sgn * 7.6, 10.5, 2.6, 3.8), 'p', 'w', 'c', outline='S'), 0, 0)
    head = E(12, 10, 6.6, 6.8)
    blit(img, shaded_part(head, 'S', 's', 'x'), 0, 0)
    trunk = C((12, 14), (12, 20), 1.9) | C((12, 20), (14.5, 21.5), 1.5)
    blit(img, shaded_part(trunk, 'S', 's', 'x'), 0, 0)
    for x in (9, 14):
        put(img, x, 9, 'k')
        put(img, x, 10, 'k')
    put(img, 7, 12, 'c')
    put(img, 17, 12, 'c')
    return img



# ---- rhyme partners (cane/pane, gatto/piatto, nave/chiave, porta, vela,
#      candela, padella, caramella, letto, rana, campana, pulcino)
def cane():
    img = canvas()
    for sx in (-1, 1):   # floppy ears
        blit(img, shaded_part(E(12 + sx * 7.5, 12, 3.2, 6.2), 'O', 'F', 'q'), 0, 0)
    head = E(12, 12.5, 7.6, 7.2)
    blit(img, shaded_part(head, 'F', 'f', 'o'), 0, 0)
    blit(img, shaded_part(E(12, 16.5, 4.4, 3.2), 'W', 'w', 's', rim=True), 0, 0)
    blit(img, shaded_part(E(12, 14.6, 1.8, 1.3), 'k', 'x', 'K'), 0, 0)
    blit(img, shaded_part(E(12, 20, 1.4, 1.3), 'P', 'p', 'h'), 0, 0)
    for ex in (9, 14):
        put(img, ex, 10, 'k')
        put(img, ex, 11, 'k')
    return img


def pane():
    img = canvas()
    loaf = E(12, 14, 10.4, 6.4)
    blit(img, shaded_part(loaf, 'F', 'f', 'o'), 0, 0)
    for x0 in (6, 11, 16):   # the cuts on top
        for i in range(4):
            put(img, x0 + i, 11 + (i // 2) - (1 if i == 0 else 0), 'O', loaf)
    for (x, y) in ((5, 12), (6, 11), (9, 10), (14, 10)):
        put(img, x, y, 'y', loaf)
    return img


def gatto():
    img = canvas()
    for sx in (-1, 1):
        ear = POLY([(12 + sx * 3, 7), (12 + sx * 8.5, 2), (12 + sx * 8.5, 10)])
        blit(img, shaded_part(ear, 'S', 's', 'x'), 0, 0)
        put(img, 12 + sx * 7 - (1 if sx < 0 else 0), 6, 'P')
    head = E(12, 13.5, 8.5, 7.2)
    blit(img, shaded_part(head, 'S', 's', 'x'), 0, 0)
    for ex in (8, 15):
        put(img, ex, 12, 'k')
        put(img, ex, 13, 'k')
    put(img, 11, 15, 'h')
    put(img, 12, 15, 'h')
    for (x, y) in ((4, 15), (5, 15), (6, 15), (17, 15), (18, 15), (19, 15), (5, 17), (18, 17)):
        put(img, x, y, 'k')
    put(img, 6, 16, 'c')
    put(img, 17, 16, 'c')
    return img


def piatto():
    img = canvas()
    blit(img, shaded_part(E(12, 12, 10.6, 10.6), 'W', 'w', 's'), 0, 0)
    rim = E(12, 12, 9.2, 9.2) & ~E(12, 12, 8.2, 8.2)
    m = E(12, 12, 9.2, 9.2)
    img[rim, :3] = rgb('B')
    well = E(12, 12, 6.2, 6.2)
    img[well & m, :3] = rgb('s')
    img[E(11, 11, 5.4, 5.4) & well, :3] = rgb('w')
    return img


def nave():
    img = canvas()
    blit(img, shaded_part(R(4, 21, 20, 22), 'B', 'b', 'n', rim=False), 0, 0)   # sea
    blit(img, shaded_part(R(14, 5, 17, 11), 'r', 'P', 'R'), 0, 0)             # funnel
    img[R(14, 5, 17, 6), :3] = rgb('k')
    blit(img, shaded_part(R(7, 10, 16, 15), 'W', 'w', 's'), 0, 0)             # cabin
    for x in (9, 12):
        put(img, x, 12, 'b')
        put(img, x + 1, 12, 'b')
    hull = POLY([(1.5, 14.5), (22.5, 14.5), (19, 21), (5, 21)])
    blit(img, shaded_part(hull, 'n', 'B', 'N'), 0, 0)
    for x in (6, 10, 14, 18):
        put(img, x, 17, 'w', hull)
    return img


def chiave():
    img = canvas()
    ring = E(7, 12, 5.4, 5.4) & ~E(7, 12, 2.4, 2.4)
    shaft = C((11, 12), (21.5, 12), 1.4)
    teeth = R(17, 13, 18, 16) | R(20, 13, 21, 15)
    blit(img, shaded_part(ring | shaft | teeth, 'Y', 'y', 'o'), 0, 0)
    return img


def porta():
    img = canvas()
    door = R(6, 3, 18, 22, 2) | E(12, 5, 6, 3)
    blit(img, shaded_part(door, 'J', 'j', 'q'), 0, 0)
    for (y0, y1) in ((6, 11), (14, 20)):
        panel = R(8, y0, 16, y1)
        img[panel & ~R(9, y0 + 1, 15, y1 - 1), :3] = rgb('q')
    blit(img, shaded_part(E(15.5, 13, 1.3, 1.3), 'Y', 'y', 'o'), 0, 0)
    return img


def vela():
    img = canvas()
    blit(img, shaded_part(R(3, 21, 21, 22), 'B', 'b', 'n', rim=False), 0, 0)   # sea
    blit(img, shaded_part(R(11, 2, 12, 17), 'q', 'j', 'Q', rim=False), 0, 0)   # mast
    sail = POLY([(13, 3), (13, 16), (21, 16)])
    blit(img, shaded_part(sail, 'W', 'w', 's'), 0, 0)
    jib = POLY([(10, 5), (10, 16), (4, 16)])
    blit(img, shaded_part(jib, 'p', 'w', 'P'), 0, 0)
    blit(img, shaded_part(POLY([(12, 1.5), (16, 3), (12, 4.5)]), 'r', 'P', 'R'), 0, 0)
    hull = POLY([(3, 17), (21, 17), (18, 21.5), (6, 21.5)])
    blit(img, shaded_part(hull, 'r', 'P', 'R'), 0, 0)
    return img


def candela():
    img = canvas()
    blit(img, shaded_part(E(12, 21, 8, 2.2), 'Y', 'y', 'o'), 0, 0)             # dish
    body = R(8.5, 9, 15.5, 20)
    blit(img, shaded_part(body, 'p', 'w', 'P'), 0, 0)
    blit(img, shaded_part(C((14, 9.5), (14, 13), 1.1), 'p', 'w', 'P'), 0, 0)   # drip
    put(img, 12, 8, 'k')
    put(img, 12, 7, 'k')
    flame = E(12, 4.5, 2.6, 3.8)
    blit(img, shaded_part(flame, 'Y', 'y', 'o', outline='O'), 0, 0)
    img[E(12, 5.2, 1.2, 2.0), :3] = rgb('w')
    return img


def padella():
    img = canvas()
    blit(img, shaded_part(C((16, 13), (23, 11), 1.6), 'q', 'j', 'Q'), 0, 0)    # handle
    pan = E(10, 13, 9, 8)
    blit(img, shaded_part(pan, 'x', 'S', 'K'), 0, 0)
    inner = E(10, 13, 7.2, 6.4)
    img[inner, :3] = rgb('K')
    blit(img, shaded_part(E(9.5, 13, 4.8, 4), 'W', 'w', 's', outline='K'), 0, 0)   # fried egg
    blit(img, shaded_part(E(10, 13, 2, 2), 'Y', 'y', 'o', outline='o'), 0, 0)
    return img


def caramella():
    img = canvas()
    for sx in (-1, 1):
        end = POLY([(12 + sx * 5, 12), (12 + sx * 11.5, 6.5), (12 + sx * 11.5, 17.5)])
        blit(img, shaded_part(end, 'P', 'p', 'h'), 0, 0)
    body = E(12, 12, 6.4, 5)
    blit(img, shaded_part(body, 'h', 'P', 'H'), 0, 0)
    for x in (9, 12, 15):
        for dy in range(-3, 4):
            put(img, x + (dy + 3) // 3 - 1, 12 + dy, 'w', body)
    return img


def letto():
    img = canvas()
    blit(img, shaded_part(R(2, 6, 5, 21), 'J', 'j', 'q'), 0, 0)                 # headboard
    blit(img, shaded_part(R(20, 12, 22, 21), 'J', 'j', 'q'), 0, 0)              # footboard
    blit(img, shaded_part(R(4, 13, 21, 17), 'W', 'w', 's'), 0, 0)               # mattress
    blit(img, shaded_part(E(8, 12, 3.2, 1.8), 'w', 'w', 's'), 0, 0)             # pillow
    blit(img, shaded_part(R(10, 11, 21, 17, 1), 'L', 'l', 'm'), 0, 0)           # blanket
    for (x, y) in ((13, 13), (16, 14), (19, 13)):
        put(img, x, y, 'w')
    return img


def rana():
    img = canvas()
    for sx in (-1, 1):
        blit(img, shaded_part(E(12 + sx * 5, 8, 3.4, 3.4), 'G', 'g', 'd'), 0, 0)
    head = E(12, 14, 9.6, 6.6)
    blit(img, shaded_part(head, 'G', 'g', 'd'), 0, 0)
    for sx in (-1, 1):
        blit(img, shaded_part(E(12 + sx * 5, 7.6, 1.9, 1.9), 'W', 'w', 's', outline='d'), 0, 0)
        put(img, 12 + sx * 5 - (1 if sx < 0 else 0), 8, 'k')
    for x in range(8, 16):
        put(img, x, 16 + (1 if 10 <= x <= 13 else 0), 'd')
    put(img, 6, 15, 'c')
    put(img, 17, 15, 'c')
    return img


def campana():
    img = canvas()
    bell = E(12, 11, 6.6, 7.2) | R(5.5, 11, 18.5, 17) | E(12, 17.5, 9.2, 2.6)
    blit(img, shaded_part(bell, 'Y', 'y', 'o'), 0, 0)
    blit(img, shaded_part(E(12, 20.5, 2, 2), 'O', 'o', 'q'), 0, 0)             # clapper
    blit(img, shaded_part(E(12, 3.2, 2.2, 1.8), 'o', 'Y', 'O'), 0, 0)           # loop
    blit(img, shaded_part(R(7, 14, 16.5, 15), 'r', 'P', 'R', rim=False), 0, 0)  # band
    return img


def pulcino():
    img = canvas()
    body = E(12, 15, 8, 6.8) | E(11, 9, 5.6, 5.2)
    blit(img, shaded_part(body, 'Y', 'y', 'o'), 0, 0)
    blit(img, shaded_part(E(16, 15, 3.4, 2.4), 'o', 'Y', 'O'), 0, 0)           # wing
    blit(img, shaded_part(POLY([(15.5, 8.5), (20, 9.5), (15.5, 10.8)]), 'F', 'f', 'O'), 0, 0)
    put(img, 12, 7, 'k')
    put(img, 12, 8, 'k')
    for x in (9, 14):
        put(img, x, 22, 'F')
        put(img, x + 1, 22, 'F')
    put(img, 9, 10, 'c')
    return img



# ---- pictures of the situations in "Le emozioni di Deva"
def disegno():
    """A drawing torn in two."""
    img = canvas()
    left = POLY([(3, 4), (12, 4), (10, 9), (13, 13), (10, 17), (12, 21), (3, 21)])
    right = POLY([(14, 5), (22, 5), (22, 22), (13.5, 22), (15.5, 18), (12.5, 14), (15.5, 10)])
    blit(img, shaded_part(left, 'W', 'w', 's'), 0, 0)
    blit(img, shaded_part(right, 'W', 'w', 's'), 1, 1)
    for (x, y) in ((6, 8), (7, 8), (5, 9), (8, 9), (6, 10), (7, 10)):   # a little sun
        put(img, x, y, 'Y')
    for x in range(4, 11):
        put(img, x, 17, 'G')
    for (x, y) in ((17, 10), (18, 9), (19, 10), (18, 11), (18, 12), (18, 13)):   # a flower
        put(img, x + 1, y + 1, 'h' if y < 11 else 'G')
    for x in range(15, 22):
        put(img, x + 1, 18, 'G')
    return img


def tuono():
    """A dark cloud and a lightning bolt."""
    img = canvas()
    bolt = POLY([(12, 11), (16, 11), (13.5, 15.5), (16.5, 15.5), (9.5, 23), (11.5, 17), (8.5, 17)])
    blit(img, shaded_part(bolt, 'Y', 'y', 'o'), 0, 0)
    cloud = E(7, 10, 5, 4.2) | E(13, 7.5, 6, 5.2) | E(18, 10, 4.8, 4.2) | R(4, 9, 21, 13, 2)
    blit(img, shaded_part(cloud, 'x', 'S', 'K'), 0, 0)
    for (x, y) in ((10, 5), (11, 5), (9, 6)):
        put(img, x, y, 'S', cloud)
    return img


def regalo():
    """A gift box with a ribbon and a bow."""
    img = canvas()
    box = R(4, 11, 19, 22, 1)
    blit(img, shaded_part(box, 'h', 'P', 'H'), 0, 0)
    lid = R(3, 8, 20, 11, 1)
    blit(img, shaded_part(lid, 'P', 'p', 'h'), 0, 0)
    ribbon = R(10.5, 8, 12.5, 22)
    img[ribbon & (box | lid), :3] = rgb('Y')
    for sx in (-1, 1):
        loop = E(12 + sx * 3.4, 5.6, 3, 2.4)
        blit(img, shaded_part(loop, 'Y', 'y', 'o'), 0, 0)
    blit(img, shaded_part(E(12, 6.5, 1.6, 1.6), 'o', 'Y', 'O'), 0, 0)
    return img


WORD_PICTURES = {
    'palla': palla, 'mela': mela, 'luna': luna, 'sole': sole, 'pesce': pesce, 'casa': casa,
    'torta': torta, 'banana': banana, 'gelato': gelato, 'palloncino': palloncino, 'ape': ape,
    'uva': uva, 'uovo': uovo, 'ombrello': ombrello, 'isola': isola, 'elefante': elefante,
    'cane': cane, 'pane': pane, 'gatto': gatto, 'piatto': piatto, 'nave': nave, 'chiave': chiave,
    'porta': porta, 'vela': vela, 'candela': candela, 'padella': padella, 'caramella': caramella,
    'letto': letto, 'rana': rana, 'campana': campana, 'pulcino': pulcino,
    'disegno': disegno, 'tuono': tuono, 'regalo': regalo,
}


OBJECTS = {
    'stelle': stella,
    'cuori': cuore,
    'rossetti': rossetto,
    'smalti': smalto,
    'fiocchi': fiocco,
    'microfoni': microfono,
    'scarpette': scarpetta,
    'farfalle': farfalla,
    'fiori': fiore,
    'diamanti': diamante,
}


# ------------------------------------------------------------------ reward icons
def ombretto(pan):
    img = canvas()
    blit(img, shaded_part(E(12, 13, 10.5, 9.0), 'x', 'S', 'k'), 0, 0)
    inner = E(12, 13, 8.0, 6.6)
    light = {'P': 'p', 'B': 'b', 'v': 'L', 'Y': 'y'}[pan]
    dark = {'P': 'h', 'B': 'n', 'v': 'V', 'Y': 'o'}[pan]
    blit(img, shaded_part(inner, pan, light, dark, outline='S'), 0, 0)
    for (x, y) in ((8, 11), (9, 10), (10, 10), (8, 12), (15, 15)):
        put(img, x, y, 'w', inner)
    return img


def pennello():
    img = canvas()
    blit(img, shaded_part(C((6, 20), (13, 11), 1.8), 'F', 'f', 'O'), 0, 0)
    blit(img, shaded_part(C((12.5, 11.5), (14.5, 9), 2.2), 'S', 's', 'x'), 0, 0)
    blit(img, shaded_part(E(17, 6.5, 4.8, 4.2), 'c', 'p', 'h'), 0, 0)
    for (x, y) in ((4, 12), (3, 13), (5, 13), (4, 14)):
        put(img, x, y, 'c')
    return img


def brillantini():
    img = canvas()
    big = ["....k....", "...kyk...", "...kyk...", "..kyYyk..", "kkyYwYykk",
           "..kyYyk..", "...kyk...", "...kyk...", "....k...."]
    small = ["..k..", ".kyk.", "kywyk", ".kyk.", "..k.."]
    tiny = [".k.", "kwk", ".k."]
    s = size()
    bi = ascii_sprite(big)
    blit(img, bi, int(2 * K), int(2 * K))
    blit(img, ascii_sprite(small), int(15 * K), int(3 * K))
    blit(img, ascii_sprite(small), int(13 * K), int(14 * K))
    blit(img, ascii_sprite(tiny), int(4 * K), int(16 * K))
    if K > 1.2:
        blit(img, ascii_sprite(small), s - 7, s // 2 - 2)
    return img


def coroncina():
    rows = [
        "..........k..........",
        ".........kYk.........",
        "...k.....kYk.....k...",
        "..kYk...kYyYk...kYk..",
        "..kYYk.kYYYYYk.kYYk..",
        "..kYyYkYYYhYYYkYyYk..",
        "..kYYYYYYhHhYYYYYYk..",
        "..kYYBYYYYhYYYYBYYk..",
        "..kYYYYYYYYYYYYYYYk..",
        "..koooooooooooooook..",
        "...kkkkkkkkkkkkkkk...",
    ]
    img = canvas()
    spr = ascii_sprite(rows)
    if K > 1.2:
        spr = np.repeat(np.repeat(spr, 2, axis=0), 2, axis=1)[:, 2:-2]
    s = size()
    blit(img, spr, (s - spr.shape[1]) // 2, (s - spr.shape[0]) // 2)
    return img


REWARD_ICONS = {
    'ombretto_rosa': lambda: ombretto('P'),
    'ombretto_azzurro': lambda: ombretto('B'),
    'ombretto_viola': lambda: ombretto('v'),
    'ombretto_oro': lambda: ombretto('Y'),
    'rossetto_rosso': lambda: rossetto('X', 'r', 'R'),  # a true red: 'r' is too close to the pink
    'rossetto_rosa': lambda: rossetto('h', 'p', 'H'),
    'rossetto_viola': lambda: rossetto('V', 'v', 'k'),
    'guance': pennello,
    'adesivo_stella': stella,
    'adesivo_cuore': cuore,
    'brillantini': brillantini,
    'coroncina': coroncina,
    'fiore': lambda: fiore('P', 'p', 'h'),
}


def render_all(k):
    """Render objects, reward icons and word pictures at scale k."""
    global K
    old, K = K, k
    try:
        objs = {name: fn() for name, fn in OBJECTS.items()}
        icons = {name: fn() for name, fn in REWARD_ICONS.items()}
        words = {name: fn() for name, fn in WORD_PICTURES.items()}
    finally:
        K = old
    return objs, icons, words
