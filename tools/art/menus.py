"""Icons of the main menu, the pause, the saves and the dressing room (v0.5).

44x44 icons sit in the 60x60 menu cards like the game icons; the small
marks (check, crown) decorate cards and keys. All drawn from scratch.
"""
import math

import numpy as np

from pixel import blank, blit, ellipse_mask, rect_mask, shaded_part, rgb, dilate4
from objects import poly_mask, star_mask

S = 44


def _star(img, cx, cy, R, r, fill='Y', light='y', dark='o'):
    w = int(R * 2 + 3)
    m = star_mask(w, w, cx=w / 2, cy=w / 2 + 0.4, R=R, r=r)
    blit(img, shaded_part(m, fill, light, dark), int(round(cx - w / 2)), int(round(cy - w / 2)))


def gioca():
    """Play: a round pink triangle and a star."""
    img = blank(S, S)
    tri = poly_mask(S, S, [(12, 6), (38, 22), (12, 38)])
    tri = tri & ~(ellipse_mask(S, S, 12, 6, 2.5, 2.5) & ~ellipse_mask(S, S, 14.4, 9, 3, 3))
    blit(img, shaded_part(tri, 'h', 'P', 'H'), 0, 0)
    # a shine on the triangle
    for i in range(6):
        img[12 + i, 16, :3] = rgb('p')
    _star(img, 35, 8, 6.2, 2.8)
    return img


def camerino():
    """The dressing-room mirror with its bulbs, and a lipstick in front."""
    img = blank(S, S)
    frame = rect_mask(S, S, 7, 3, 33, 35, radius=9)
    blit(img, shaded_part(frame, 'Y', 'y', 'o'), 0, 0)
    glass = rect_mask(S, S, 11, 7, 29, 31, radius=6)
    img[glass, :3] = rgb('b')
    for i in range(14):          # two shine stripes on the glass
        for d in (0, 1):
            x, y = 14 + i // 2 + d, 26 - i
            if 0 <= y < S and glass[y, x]:
                img[y, x, :3] = rgb('w')
        x, y = 20 + i // 2, 28 - i
        if glass[y, x]:
            img[y, x, :3] = rgb('w')
    # bulbs around the frame
    for (bx, by) in [(8, 10), (8, 19), (8, 28), (32, 10), (32, 19), (32, 28), (14, 4), (20, 4), (26, 4)]:
        m = ellipse_mask(S, S, bx + 0.5, by + 0.5, 1.8, 1.8)
        img[m, :3] = rgb('W')
        img[m, 3] = 255
    stand = rect_mask(S, S, 15, 36, 25, 40, radius=1)
    blit(img, shaded_part(stand, 'O', 'o', 'k'), 0, 0)
    # lipstick in front, bottom right
    tube = rect_mask(S, S, 32, 30, 39, 41, radius=1)
    blit(img, shaded_part(tube, 'Y', 'y', 'o'), 0, 0)
    bullet = poly_mask(S, S, [(33, 30.5), (33, 23), (38.6, 19.5), (38.6, 30.5)])
    blit(img, shaded_part(bullet, 'r', 'P', 'R'), 0, 0)
    return img


def album():
    """A lilac book with a gold star on the cover."""
    img = blank(S, S)
    pages = rect_mask(S, S, 11, 6, 38, 38, radius=3)
    blit(img, shaded_part(pages, 'W', 'w', 's'), 0, 0)
    cover = rect_mask(S, S, 5, 4, 34, 39, radius=3)
    blit(img, shaded_part(cover, 'L', 'l', 'm'), 0, 0)
    spine = rect_mask(S, S, 5, 4, 9, 39, radius=2)
    img[spine & (img[..., 3] > 0) & ~(dilate4(~cover)), :3] = rgb('m')
    _star(img, 21, 19, 8.5, 3.9)
    # a ribbon bookmark
    rib = rect_mask(S, S, 27, 36, 29, 42)
    blit(img, shaded_part(rib, 'h', 'P', 'H'), 0, 0)
    return img


def _face(img, cx, cy, hair, frame, frame_light, frame_dark):
    ring = ellipse_mask(S, S, cx, cy, 9.5, 9.5)
    blit(img, shaded_part(ring, frame, frame_light, frame_dark), 0, 0)
    head = ellipse_mask(S, S, cx, cy + 1, 6.2, 6.2)
    img[head, :3] = rgb('a')
    cap = ellipse_mask(S, S, cx, cy - 1.5, 7.2, 5.6) & ~ellipse_mask(S, S, cx, cy + 2.2, 6.0, 5.0)
    cap &= ring
    img[cap, :3] = rgb(hair)
    ix, iy = int(cx), int(cy)
    for dx in (-3, 2):
        img[iy + 1, ix + dx, :3] = rgb('k')
        img[iy + 2, ix + dx, :3] = rgb('k')
    img[iy + 4, ix - 1:ix + 2, :3] = rgb('H')
    img[iy + 3, ix - 4, :3] = rgb('c')
    img[iy + 3, ix + 3, :3] = rgb('c')


def salvataggi():
    """Who is playing? Three little faces in round frames."""
    img = blank(S, S)
    _face(img, 11.5, 13.5, 'J', 'P', 'p', 'h')
    _face(img, 32.5, 13.5, 'Q', 'T', 't', 'e')
    _face(img, 22, 30.5, 'u', 'Y', 'y', 'o')
    return img


def opzioni():
    """A gear, for the grown-ups."""
    img = blank(S, S)
    yy, xx = np.mgrid[0:S, 0:S]
    dx, dy = xx + 0.5 - 22, yy + 0.5 - 22
    rho = np.hypot(dx, dy)
    th = np.arctan2(dy, dx)
    teeth = (np.floor(th * 16 / (2 * math.pi) + 0.5) % 2) == 0
    m = (rho <= np.where(teeth, 18.5, 15.0)) & (rho >= 6.5)
    blit(img, shaded_part(m, 'S', 's', 'x'), 0, 0)
    hub = (rho < 6.5) & (rho >= 3.5)
    img[hub, :3] = rgb('L')
    img[hub, 3] = 255
    ring = dilate4(rho < 3.5) & ~(rho < 3.5)
    img[ring & hub, :3] = rgb('k')
    return img


def esci():
    """Good night: a golden crescent moon, a "z" and two stars."""
    img = blank(S, S)
    moon = ellipse_mask(S, S, 20, 24, 15, 15) & ~ellipse_mask(S, S, 27.5, 18.5, 12.5, 12.5)
    blit(img, shaded_part(moon, 'Y', 'y', 'o'), 0, 0)
    z = ["#####", "...#.", "..#..", ".#...", "#####"]
    for y, r in enumerate(z):
        for x, c in enumerate(r):
            if c == '#':
                img[5 + y, 30 + x, :3] = rgb('V')
                img[5 + y, 30 + x, 3] = 255
    _star(img, 36, 32, 4.6, 2.0)
    _star(img, 28, 38, 3.4, 1.5, 'p', 'w', 'P')
    return img


def giochi():
    """Back to the games: three cards fanned out."""
    img = blank(S, S)
    for (x0, y0, fill, light, dark) in [(3, 11, 'T', 't', 'e'), (25, 11, 'Y', 'y', 'o'), (13, 5, 'P', 'p', 'h')]:
        c = rect_mask(S, S, x0, y0, x0 + 16, y0 + 24, radius=3)
        blit(img, shaded_part(c, fill, light, dark), 0, 0)
        inner = rect_mask(S, S, x0 + 3, y0 + 3, x0 + 13, y0 + 21, radius=2)
        img[inner & (img[..., 3] > 0), :3] = rgb('W')
    _star(img, 21.5, 17, 5.2, 2.4, 'h', 'P', 'H')
    heart = ellipse_mask(S, S, 9.5, 27, 2.6, 2.6) | ellipse_mask(S, S, 13.5, 27, 2.6, 2.6) | \
        poly_mask(S, S, [(7, 28), (16, 28), (11.5, 33)])
    img[heart & (img[..., 3] > 0), :3] = rgb('r')
    for (x, y) in [(31, 22), (34, 25), (31, 28)]:
        img[y:y + 2, x:x + 2, :3] = rgb('n')
    return img


def mappa():
    """Back to the map: a folded parchment with a dotted path and a star."""
    img = blank(S, S)
    panels = [[(3, 10), (15, 6), (15, 36), (3, 40)], [(15, 6), (29, 10), (29, 40), (15, 36)],
              [(29, 10), (41, 6), (41, 36), (29, 40)]]
    for i, p in enumerate(panels):
        m = poly_mask(S, S, p)
        blit(img, shaded_part(m, 'f' if i != 1 else 'F', 'W', 'F' if i != 1 else 'O'), 0, 0)
    for (x, y) in [(7, 33), (10, 29), (14, 26), (18, 25), (22, 22), (25, 18)]:
        img[y:y + 2, x:x + 2, :3] = rgb('h')
    _star(img, 32, 15, 5.4, 2.4)
    return img


def inizio():
    """Back to the start: a little house (drawn at 44 px)."""
    import objects as O
    k = O.K
    O.K = S / 24.0
    try:
        img = O.casa()
    finally:
        O.K = k
    return img


def check():
    """A green tick, 16x16 (worn items, the OK key)."""
    m = poly_mask(16, 16, [(1.5, 8.5), (4.5, 5.5), (7, 8), (12.5, 2), (15, 4.5), (7, 13.5)])
    return shaded_part(m, 'G', 'g', 'd')


def corona():
    """A small crown, 18x13 (the profile in use)."""
    m = poly_mask(18, 13, [(1.5, 3), (5.5, 7), (9, 1), (12.5, 7), (16.5, 3), (15.5, 11.5), (2.5, 11.5)])
    img = shaded_part(m, 'Y', 'y', 'o')
    img[9, 8:10, :3] = rgb('h')
    return img


def lucchetto():
    """A padlock, 14x16 (grown-up items of the menu)."""
    img = blank(14, 16)
    shackle = ellipse_mask(14, 16, 7, 7, 5, 5.5) & ~ellipse_mask(14, 16, 7, 7, 2.6, 3.2)
    shackle[7:, :] = False
    blit(img, shaded_part(shackle, 'S', 's', 'x'), 0, 0)
    body = rect_mask(14, 16, 1, 7, 12, 14, radius=1)
    blit(img, shaded_part(body, 'Y', 'y', 'o'), 0, 0)
    img[10:12, 6:8, :3] = rgb('O')
    return img


def all_sprites():
    return {
        'mm_gioca': gioca(),
        'mm_camerino': camerino(),
        'mm_album': album(),
        'mm_salvataggi': salvataggi(),
        'mm_opzioni': opzioni(),
        'mm_esci': esci(),
        'mm_giochi': giochi(),
        'mm_mappa': mappa(),
        'mm_inizio': inizio(),
        'mark_ok': check(),
        'mark_corona': corona(),
        'mark_lucchetto': lucchetto(),
    }
