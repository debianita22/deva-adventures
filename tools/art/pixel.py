"""Tiny pixel-art toolkit: palette, ASCII sprites, masks, outlines.

Sprites are numpy RGBA arrays (h, w, 4) with alpha 0 or 255 only, because
the runtime uses 1-bit transparency.
"""
import numpy as np
from PIL import Image

# Pastel-pop palette. One character per colour; '.' is transparent.
PALETTE = {
    'k': (0x3b, 0x1f, 0x4a),  # outline, dark plum
    'K': (0x24, 0x12, 0x2f),  # deepest plum
    'w': (0xff, 0xff, 0xff),
    'W': (0xff, 0xf4, 0xfa),  # cream
    'l': (0xea, 0xdc, 0xfc),  # lilac light
    'L': (0xcb, 0xab, 0xf2),  # lilac
    'm': (0xa5, 0x86, 0xdb),  # lilac shadow
    'M': (0x7c, 0x5f, 0xbb),  # lilac deep shadow
    'p': (0xff, 0xcb, 0xe3),  # pink light
    'P': (0xff, 0x93, 0xc6),  # pink
    'h': (0xf0, 0x55, 0x9e),  # hot pink
    'H': (0xb8, 0x2a, 0x72),  # deep pink
    'c': (0xff, 0xa8, 0xc0),  # blush
    'r': (0xff, 0x4d, 0x6d),  # red
    'R': (0xc2, 0x1f, 0x45),  # dark red
    'y': (0xff, 0xf3, 0xa6),  # yellow light
    'Y': (0xff, 0xd2, 0x3f),  # gold
    'o': (0xf0, 0xa0, 0x30),  # gold shadow
    'O': (0xb8, 0x6b, 0x1c),  # dark gold
    't': (0xbd, 0xf5, 0xea),  # turquoise light
    'T': (0x4f, 0xd6, 0xc0),  # turquoise
    'e': (0x26, 0xa3, 0x9a),  # teal shadow
    'E': (0x17, 0x75, 0x6f),  # dark teal
    'b': (0xc4, 0xe6, 0xff),  # blue light
    'B': (0x74, 0xb8, 0xff),  # sky blue
    'n': (0x47, 0x77, 0xd1),  # blue shadow
    'N': (0x2b, 0x4a, 0x9a),  # navy
    'v': (0xb3, 0x76, 0xec),  # violet
    'V': (0x7d, 0x3f, 0xc4),  # deep violet
    'g': (0xa8, 0xf0, 0x8a),  # green light
    'G': (0x4c, 0xc2, 0x5a),  # green
    'd': (0x2a, 0x8a, 0x45),  # dark green
    's': (0xe4, 0xde, 0xf0),  # silver light
    'S': (0xb0, 0xa4, 0xc8),  # silver
    'x': (0x6b, 0x5e, 0x80),  # dark grey-violet
    'f': (0xff, 0xe0, 0xb8),  # peach light
    'F': (0xff, 0xb8, 0x7a),  # peach
    # Deva: skin, honey-brown hair with sun-kissed tips, brown eyes
    'i': (0xff, 0xef, 0xe2),  # skin light
    'a': (0xff, 0xd8, 0xbe),  # skin
    'A': (0xf1, 0xb0, 0x92),  # skin shadow
    'u': (0xf6, 0xd9, 0x92),  # hair tips / highlight
    'j': (0xde, 0xae, 0x67),  # hair light
    'J': (0xb9, 0x85, 0x48),  # hair
    'q': (0x8b, 0x5b, 0x2e),  # hair shadow
    'Q': (0x5d, 0x39, 0x1c),  # hair deep shadow
    'C': (0x8a, 0x55, 0x2c),  # iris brown
    'D': (0x45, 0x27, 0x14),  # iris dark
    # the console (XiFan RF35H, grey) and its SNES-style buttons
    'I': (0xd2, 0xd0, 0xd6),  # console light
    'z': (0xa9, 0xa7, 0xae),  # console grey
    'Z': (0x82, 0x80, 0x88),  # console shadow
    'U': (0x2b, 0x29, 0x31),  # D-pad, sticks, pills
    'X': (0xe5, 0x39, 0x35),  # A button red
    # the tale: the grey spell, monsters and the witch (digits and signs: the letters are taken)
    '0': (0x1e, 0x17, 0x33),  # night, deepest
    '1': (0x4a, 0x47, 0x5c),  # stone dark
    '2': (0x7a, 0x76, 0x8c),  # stone
    '3': (0xa9, 0xa5, 0xb9),  # stone light
    '4': (0x2f, 0x4f, 0x33),  # swamp dark
    '5': (0x55, 0x80, 0x45),  # swamp
    '6': (0x8f, 0xb8, 0x5a),  # swamp light
    '7': (0xe6, 0xff, 0x6e),  # glowing lime (monster eyes)
    '8': (0xc3, 0xcf, 0xb4),  # the witch's grey-green skin
    '9': (0x5c, 0x17, 0x33),  # inside of a mouth
    '@': (0xff, 0x7b, 0x2e),  # orange glow
    '$': (0x3a, 0x26, 0x66),  # deep violet night
    '%': (0x95, 0x8e, 0xab),  # fog
}


def rgb(c):
    if c[0] == '#' and len(c) == 7:   # a colour outside the palette, "#rrggbb"
        return tuple(int(c[i:i + 2], 16) for i in (1, 3, 5))
    return PALETTE[c]


def blank(w, h):
    return np.zeros((h, w, 4), dtype=np.uint8)


def ascii_sprite(rows, pal=None):
    """Build a sprite from equal-length strings; '.' or ' ' = transparent."""
    pal = pal or PALETTE
    h, w = len(rows), max(len(r) for r in rows)
    img = blank(w, h)
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            if ch in '. ':
                continue
            img[y, x, :3] = pal[ch]
            img[y, x, 3] = 255
    return img


def mirror_rows(half_rows, odd_center=False):
    """Left half -> full symmetric rows (optionally sharing the centre column)."""
    out = []
    for r in half_rows:
        right = r[::-1][1:] if odd_center else r[::-1]
        out.append(r + right)
    return out


def flip_h(img):
    return img[:, ::-1].copy()


def blit(dst, src, x, y):
    """Alpha-keyed paste of src onto dst at (x, y), clipped."""
    h, w = src.shape[:2]
    H, W = dst.shape[:2]
    x0, y0 = max(0, x), max(0, y)
    x1, y1 = min(W, x + w), min(H, y + h)
    if x0 >= x1 or y0 >= y1:
        return dst
    s = src[y0 - y:y1 - y, x0 - x:x1 - x]
    m = s[..., 3] > 0
    d = dst[y0:y1, x0:x1]
    d[m] = s[m]
    return dst


def recolor(img, mapping):
    """Replace palette colours: mapping {'P': 'B', ...}."""
    out = img.copy()
    for a, b in mapping.items():
        ca, cb = np.array(rgb(a)), np.array(rgb(b))
        m = (out[..., 3] > 0) & np.all(out[..., :3] == ca, axis=-1)
        out[m, :3] = cb
    return out


def shear_x(img, k, anchor_bottom=True):
    """Horizontal shear: rows far from the anchor shift by k pixels per row."""
    h, w = img.shape[:2]
    pad = int(abs(k) * h) + 1
    out = blank(w + 2 * pad, h)
    for y in range(h):
        d = (h - 1 - y) if anchor_bottom else y
        s = int(round(d * k))
        out[y, pad + s:pad + s + w] = img[y]
    # trim empty columns
    cols = np.where(out[..., 3].any(axis=0))[0]
    return out[:, cols.min():cols.max() + 1]


# ---------------------------------------------------------------- shapes

def ellipse_mask(w, h, cx, cy, rx, ry):
    yy, xx = np.mgrid[0:h, 0:w]
    return ((xx + 0.5 - cx) / rx) ** 2 + ((yy + 0.5 - cy) / ry) ** 2 <= 1.0


def capsule_mask(w, h, p0, p1, r):
    yy, xx = np.mgrid[0:h, 0:w]
    px, py = xx + 0.5, yy + 0.5
    (x0, y0), (x1, y1) = p0, p1
    dx, dy = x1 - x0, y1 - y0
    L2 = dx * dx + dy * dy or 1e-9
    t = np.clip(((px - x0) * dx + (py - y0) * dy) / L2, 0, 1)
    qx, qy = x0 + t * dx, y0 + t * dy
    return (px - qx) ** 2 + (py - qy) ** 2 <= r * r


def rect_mask(w, h, x0, y0, x1, y1, radius=0):
    yy, xx = np.mgrid[0:h, 0:w]
    m = (xx >= x0) & (xx <= x1) & (yy >= y0) & (yy <= y1)
    if radius:
        for (cx, cy) in ((x0 + radius, y0 + radius), (x1 - radius, y0 + radius),
                         (x0 + radius, y1 - radius), (x1 - radius, y1 - radius)):
            corner = ((xx < x0 + radius) if cx == x0 + radius else (xx > x1 - radius)) & \
                     ((yy < y0 + radius) if cy == y0 + radius else (yy > y1 - radius))
            inside = (xx - cx) ** 2 + (yy - cy) ** 2 <= radius * radius + 0.5
            m &= ~corner | inside
    return m


def dilate4(mask):
    m = mask.copy()
    m[1:, :] |= mask[:-1, :]
    m[:-1, :] |= mask[1:, :]
    m[:, 1:] |= mask[:, :-1]
    m[:, :-1] |= mask[:, 1:]
    return m


def dilate8(mask):
    m = dilate4(mask)
    m[1:, 1:] |= mask[:-1, :-1]
    m[1:, :-1] |= mask[:-1, 1:]
    m[:-1, 1:] |= mask[1:, :-1]
    m[:-1, :-1] |= mask[1:, 1:]
    return m


def shaded_part(mask, base, light, shadow, outline='k', rim=True):
    """Fill a mask with 3-tone rim shading (light top-left, shadow bottom-right)
    and a 1 px outline around it. Returns an RGBA sprite of the mask size."""
    h, w = mask.shape
    img = blank(w, h)
    img[mask, :3] = rgb(base)
    img[mask, 3] = 255
    if rim:
        up_left = np.zeros_like(mask)
        up_left[1:, 1:] = mask[:-1, :-1]
        up = np.zeros_like(mask)
        up[1:, :] = mask[:-1, :]
        left = np.zeros_like(mask)
        left[:, 1:] = mask[:, :-1]
        dn_right = np.zeros_like(mask)
        dn_right[:-1, :-1] = mask[1:, 1:]
        dn = np.zeros_like(mask)
        dn[:-1, :] = mask[1:, :]
        rt = np.zeros_like(mask)
        rt[:, :-1] = mask[:, 1:]
        hi = mask & (~up | ~left) & ~(~dn | ~rt)
        sh = mask & (~dn | ~rt)
        if light:
            img[hi, :3] = rgb(light)
        if shadow:
            img[sh, :3] = rgb(shadow)
    ring = dilate4(mask) & ~mask
    img[ring, :3] = rgb(outline)
    img[ring, 3] = 255
    return img


def add_outline(img, color='k', diag=False):
    """Outline around the opaque area of an existing sprite (grows by 1 px)."""
    h, w = img.shape[:2]
    out = blank(w + 2, h + 2)
    out[1:-1, 1:-1] = img
    m = out[..., 3] > 0
    ring = (dilate8(m) if diag else dilate4(m)) & ~m
    out[ring, :3] = rgb(color)
    out[ring, 3] = 255
    return out


def dither_fill(img, mask, c1, c2, pattern='checker'):
    yy, xx = np.mgrid[0:img.shape[0], 0:img.shape[1]]
    if pattern == 'checker':
        sel = ((xx + yy) % 2) == 0
    else:
        sel = (xx % 2 == 0) & (yy % 2 == 0)
    a = mask & sel
    b = mask & ~sel
    img[a, :3] = rgb(c1)
    img[b, :3] = rgb(c2)
    img[mask, 3] = 255


def crop(img):
    a = img[..., 3] > 0
    if not a.any():
        return img
    ys, xs = np.where(a)
    return img[ys.min():ys.max() + 1, xs.min():xs.max() + 1]


def save(img, path):
    from pngpal import save_png  # lossless indexed when <= 256 colours (1.0)
    save_png(Image.fromarray(img, 'RGBA'), path)


def scale(img, s):
    return np.repeat(np.repeat(img, s, axis=0), s, axis=1)


def sheet(items, bg=(0x2a, 0x1a, 0x4a), s=4, pad=4, cols=8):
    """Preview sheet of (name, img) scaled x s on a dark background."""
    cells = [scale(i, s) for _, i in items]
    cw = max(c.shape[1] for c in cells) + pad * 2
    ch = max(c.shape[0] for c in cells) + pad * 2
    rows = (len(cells) + cols - 1) // cols
    out = np.zeros((rows * ch, cols * cw, 4), dtype=np.uint8)
    out[..., :3] = bg
    out[..., 3] = 255
    for i, c in enumerate(cells):
        r, q = divmod(i, cols)
        blit(out, c, q * cw + pad, r * ch + pad)
    return out
