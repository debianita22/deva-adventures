"""Props redrawn in 0.8.0 with more volume and detail, same sizes and anchors:

  tavolo (64x40, drawn 2x)      the top surface stays at y 2..8, the legs apart (under it: room)
  scatola / scatola_fronte      (52x40, drawn 2x) the inside at y 1..12, the front face from y 10
  casa_grande (84x84)           roof peak at y ~9 (70 px over the floor), walls down to y ~76
  trampolino (64x20)            the bed at y ~6
  stella_<colour> (22x22)       the big stars of the second tale: faceted, with a shine and a smile
"""
import numpy as np

from pixel import blank, rgb, dilate4, ellipse_mask, rect_mask
import objects as O

BAYER4 = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0


class Canvas:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.img = blank(w, h)
        self.yy, self.xx = np.mgrid[0:h, 0:w]

    def rect(self, x0, y0, x1, y1, r=0):
        return rect_mask(self.w, self.h, x0, y0, x1, y1, r)

    def ell(self, cx, cy, rx, ry):
        return ellipse_mask(self.w, self.h, cx, cy, rx, ry)

    def poly(self, pts):
        return O.poly_mask(self.w, self.h, pts)

    def paint(self, m, c):
        self.img[m, :3] = rgb(c)
        self.img[m, 3] = 255

    def ring(self, m, c='k'):
        r = dilate4(m) & ~m
        self.paint(r, c)
        return r

    def ramp(self, m, t, tones, band=0.5):
        n = len(tones)
        v = np.clip(t, 0, 0.9999) * (n - 1)
        i = np.floor(v).astype(int)
        f = np.clip((v - i - 0.5) / band + 0.5, 0, 1)
        i = np.clip(i + (f > BAYER4[self.yy % 4, self.xx % 4]), 0, n - 1)
        for k, c in enumerate(tones):
            self.paint(m & (i == k), c)

    def ball(self, cx, cy, rx, ry, tones, L=(-0.55, -0.65, 0.52), band=0.5, clip=None):
        nx = (self.xx + 0.5 - cx) / rx
        ny = (self.yy + 0.5 - cy) / ry
        r2 = nx * nx + ny * ny
        m = r2 <= 1.0
        if clip is not None:
            m &= clip
        lv = np.array(L) / np.linalg.norm(L)
        d = np.clip(nx * lv[0] + ny * lv[1] + np.sqrt(np.clip(1 - r2, 0, 1)) * lv[2], 0, 1)
        self.ramp(m, d, tones, band)
        return m

    def line(self, pts, c, w=0.5, clip=None):
        for (x0, y0), (x1, y1) in zip(pts[:-1], pts[1:]):
            n = int(max(abs(x1 - x0), abs(y1 - y0)) * 2) + 1
            for t in np.linspace(0, 1, n):
                x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
                m = self.ell(x, y, w, w) if w > 0.6 else self.rect(int(x), int(y), int(x), int(y))
                if clip is not None:
                    m &= clip
                self.paint(m, c)


WOOD = ['#7a4a26', '#a8703e', '#c98f58', '#e0ad76', '#f2cc98']


# ------------------------------------------------------------------ the table
def tavolo():
    c = Canvas(64, 40)
    top = c.rect(1, 2, 62, 6, 2)
    edge = c.rect(1, 7, 62, 9)
    apron = c.rect(5, 10, 58, 13)
    legs = np.zeros((40, 64), bool)
    for x0 in (6, 52):
        leg = c.poly([(x0, 13), (x0 + 6, 13), (x0 + 5, 36), (x0 + 1, 36)])   # tapered
        leg |= c.rect(x0, 35, x0 + 6, 38, 1)                                 # a little foot
        legs |= leg
    whole = top | edge | apron | legs
    c.ring(whole)
    c.ramp(top, 0.85 - (c.yy - 2) / 5.0 * 0.35 - (c.xx - 32) / 200.0, WOOD)
    for (y, x0, x1) in ((3, 6, 26), (4, 30, 58), (5, 12, 40)):   # the grain
        c.paint(c.rect(x0, y, x1, y) & top, '#c98f58')
    c.paint(c.rect(4, 2, 40, 2) & top, '#fbe2bc')
    c.paint(edge, '#a8703e')
    c.paint(c.rect(1, 7, 62, 7) & edge, '#c98f58')
    c.paint(apron, '#8a5630')
    c.paint(c.rect(5, 10, 58, 10), '#a8703e')
    c.ball(32, 11.5, 2.2, 1.6, ['#b86b1c', '#ffd23f', '#fff3a6'])   # a little drawer knob
    for x0 in (6, 52):
        leg = legs & (c.xx >= x0 - 2) & (c.xx <= x0 + 8)
        c.ramp(leg, 0.85 - (c.xx - x0) / 6.0 * 0.8, WOOD[:4], band=0.3)
        c.paint(c.rect(x0, 34, x0 + 6, 34) & leg, '#7a4a26')
    return c.img


# ------------------------------------------------------------------ the cardboard box
CARD = ['#8a5a34', '#b9844e', '#d9a066', '#e8b57a', '#f6d09c']


def scatola():
    """The whole box, and its front face alone (drawn again over an object inside)."""
    c = Canvas(52, 40)
    inside = c.rect(3, 1, 48, 12, 1)
    flap_l = c.poly([(3, 10), (0, 2), (4, 1), (8, 9)])
    flap_r = c.poly([(48, 10), (51, 2), (47, 1), (43, 9)])
    back_flap = c.poly([(8, 3), (12, 0), (40, 0), (44, 3)])
    back = inside | flap_l | flap_r | back_flap
    c.ring(back)
    c.ramp(back_flap, 0.55 + 0 * c.xx, CARD)
    c.ramp(inside, 0.15 + (c.yy - 1) / 11.0 * 0.6, ['#3a2414', '#5a381e', '#7a4a26'])   # the dark inside
    c.ramp(flap_l, 0.75 + 0 * c.xx, CARD)
    c.ramp(flap_r, 0.35 + 0 * c.xx, CARD)
    back_img = c.img.copy()
    f = Canvas(52, 40)
    face = f.rect(1, 10, 50, 38, 2)
    f.ring(face)
    f.ramp(face, 0.75 - (f.xx - 1) / 49.0 * 0.45 - (f.yy - 10) / 28.0 * 0.12, CARD)
    f.paint(f.rect(2, 11, 49, 11) & face, '#f6d09c')               # the lit top edge
    tape = f.rect(23, 11, 28, 37) & face
    f.paint(tape, '#e8d8b8')
    f.paint(f.rect(23, 11, 23, 37) & face, '#f6ecd6')
    f.paint(f.rect(28, 11, 28, 37) & face, '#c9b48c')
    heart = (f.ell(10.5, 22, 2.6, 2.6) | f.ell(15.5, 22, 2.6, 2.6) | f.poly([(8, 23), (18, 23), (13, 29)]))
    f.paint(heart, '#ff93c6')                                       # a sticker on the box
    f.paint(f.ell(10.5, 21.5, 1, 1) & heart, '#ffcbe3')
    for (x, y) in ((36, 30), (40, 18), (44, 33), (6, 34)):          # cardboard specks
        f.paint(f.rect(x, y, x, y), '#b9844e')
    whole = back_img.copy()
    a = f.img[..., 3] > 0
    whole[a] = f.img[a]
    return {'scatola': whole, 'scatola_fronte': f.img}


# ------------------------------------------------------------------ the house
def casa_grande():
    c = Canvas(84, 84)
    walls = c.rect(14, 36, 70, 76)
    base = c.rect(12, 73, 72, 78, 1)
    roof = c.poly([(2, 40), (42, 7), (82, 40), (78, 44), (42, 13), (6, 44)]) | c.poly([(6, 44), (42, 13), (78, 44)])
    chimney = c.rect(58, 14, 65, 30)
    gable = c.poly([(14, 40), (42, 16), (70, 40), (70, 44), (14, 44)])
    whole = walls | base | roof | chimney
    c.ring(whole)
    c.ramp(chimney, 0.7 - (c.xx - 58) / 8.0 * 0.5, ['#8a3a3a', '#b85050', '#d87070'])
    c.paint(c.rect(57, 12, 66, 14), '#5a4a5a')
    c.ring(c.rect(57, 12, 66, 14))
    c.paint(c.rect(58, 18, 65, 18) | c.rect(58, 23, 65, 23), '#8a3a3a')
    # plaster walls, lit from the left
    c.ramp(walls | gable, 0.85 - (c.xx - 14) / 56.0 * 0.5, ['#e2d4ec', '#f0e6f6', '#fff4fa', '#ffffff'])
    # stone base
    c.ramp(base, 0.6 - (c.xx - 12) / 60.0 * 0.3, ['#7a7690', '#a9a5b9', '#cfcbdc'])
    for x in range(16, 72, 8):
        c.paint(c.rect(x, 74, x, 77) & base, '#6a6680')
    # the roof: rows of rounded tiles
    tiles = roof & ~gable
    c.ramp(tiles, 0.55 + 0 * c.xx, ['#9a1f3d', '#c21f45', '#e04860', '#ff6d85'])
    for row in range(5):
        y = 14 + row * 6
        for k in range(-8, 9):
            x = 42 + k * 6 + (3 if row % 2 else 0)
            t = c.ell(x, y + 3, 3.2, 3.0) & tiles & (c.yy >= y)
            c.paint(t & (c.yy >= y + 3), '#9a1f3d')
            c.paint(t & (c.yy < y + 3) & (c.xx < x), '#ff6d85')
    c.paint(c.poly([(40, 8), (42, 6), (44, 8), (42, 10)]), '#ffd23f')   # a little gold tip
    c.paint(roof & (dilate4(gable) & ~gable) & ~chimney, '#6e1230')     # the eaves' shadow line
    # a round window in the gable
    c.ring(c.ell(42, 29, 5, 5), '#b86b1c')
    c.ball(42, 29, 4.6, 4.6, ['#4777d1', '#74b8ff', '#c4e6ff'], band=0.6)
    c.paint(c.rect(41.5, 24, 42, 34) | c.rect(37, 28.5, 47, 29), '#b86b1c')
    # two windows with shutters and flower boxes
    for x0 in (19, 51):
        win = c.rect(x0, 46, x0 + 13, 58, 1)
        c.ring(win)
        c.ramp(win, 0.8 - (c.yy - 46) / 12.0 * 0.5, ['#4777d1', '#74b8ff', '#c4e6ff'])
        c.paint(c.rect(x0 + 6, 46, x0 + 7, 58) | c.rect(x0, 51, x0 + 13, 52), '#fff4fa')
        c.paint(c.rect(x0 + 2, 47, x0 + 4, 47), '#ffffff')
        for sx in (x0 - 4, x0 + 14):
            sh = c.rect(sx, 46, sx + 3, 58)
            c.ring(sh)
            c.paint(sh, '#4fd6c0')
            c.paint(c.rect(sx, 49, sx + 3, 49) | c.rect(sx, 54, sx + 3, 54), '#26a39a')
        box = c.rect(x0 - 1, 59, x0 + 14, 62, 1)
        c.ring(box)
        c.paint(box, '#a8703e')
        for k in range(5):
            fx = x0 + 1 + k * 3
            c.paint(c.ell(fx, 58, 1.6, 1.6), ['#ff93c6', '#ffd23f', '#b376ec'][k % 3])
            c.paint(c.rect(fx, 58, fx, 58), '#fff4fa')
    # the door: arched, wood planks, a gold knob, a doorstep
    door = c.rect(36, 52, 48, 74) | c.ell(42, 53, 6, 5)
    c.ring(door)
    c.ramp(door, 0.75 - (c.xx - 36) / 12.0 * 0.5, WOOD)
    for x in (40, 44):
        c.paint(c.rect(x, 52, x, 74) & door, '#7a4a26')
    c.paint(c.ell(45.5, 64, 1.1, 1.1), '#ffd23f')
    step = c.rect(34, 75, 50, 78, 1)
    c.ring(step)
    c.paint(step, '#cfcbdc')
    return c.img


# ------------------------------------------------------------------ the trampoline
def trampolino():
    c = Canvas(64, 20)
    legs = np.zeros((20, 64), bool)
    for lx in (7, 54):
        legs |= c.rect(lx, 7, lx + 2, 17) | c.rect(lx - 2, 17, lx + 4, 18, 1)
    pad = c.ell(32, 6.5, 30, 5.5)
    bed = c.ell(32, 6.5, 25, 3.4)
    c.ring(legs | pad)
    c.ramp(legs, 0.8 - (c.xx % 9) / 9.0, ['#6b5e80', '#b0a4c8', '#e4def0'])
    c.ramp(pad & ~bed, 0.9 - (c.yy - 1) / 11.0 * 0.8, ['#2b4a9a', '#4777d1', '#74b8ff', '#c4e6ff'])
    c.ramp(bed, 0.5 - (c.yy - 3) / 7.0 * 0.4 - np.abs(c.xx - 26) / 60.0, ['#141020', '#241a36', '#34284a'])
    c.paint(c.rect(18, 4, 30, 4) & bed, '#4a3a6a')                     # a shine on the mat
    for k in range(10):   # the springs, all around
        a = np.pi * (0.1 + 0.8 * k / 9.0)
        x = 32 + 27.5 * np.cos(a) * (1 if k % 2 else -1)
        y = 6.5 + 4.4 * np.sin(a) * 0.6
        c.paint(c.rect(int(x), int(y), int(x), int(y)) & pad, '#e4def0')
    for x in range(12, 54, 6):   # zigzags under the pad
        c.line([(x, 11), (x + 1.5, 12.5), (x + 3, 11)], '#b0a4c8', 0.4)
    return c.img


# ------------------------------------------------------------------ the big stars of the second tale
STAR_RAMPS = {'rossa': ['#6e0f22', '#c21f45', '#e53935', '#ff4d6d', '#ffcbd6'],
              'arancione': ['#7a3a10', '#b86b1c', '#f0a030', '#ffd23f', '#fff3a6'],
              'azzurra': ['#1f3a7a', '#4777d1', '#74b8ff', '#c4e6ff', '#ffffff'],
              'verde': ['#145a2a', '#2a8a45', '#4cc25a', '#a8f08a', '#eaffdd'],
              'oro': ['#8a5a10', '#f0a030', '#ffd23f', '#fff3a6', '#ffffff']}


def stella(col):
    """A big star (22x22): ten facets lit from the upper left, a glint, a smile."""
    w = h = 22
    c = Canvas(w, h)
    cx, cy, ro, ri = 11, 11.5, 10.5, 4.6
    outer = [(cx + ro * np.cos(np.deg2rad(-90 + k * 72)), cy + ro * np.sin(np.deg2rad(-90 + k * 72))) for k in range(5)]
    inner = [(cx + ri * np.cos(np.deg2rad(-54 + k * 72)), cy + ri * np.sin(np.deg2rad(-54 + k * 72))) for k in range(5)]
    whole = c.poly([p for k in range(5) for p in (outer[k], inner[k])])
    c.ring(whole)
    ramp = STAR_RAMPS[col]
    light = np.deg2rad(-125)
    c.paint(whole, ramp[2])
    for k in range(5):   # each arm: the half facing the light brighter, the other darker
        for side in (-1, 1):
            j = k if side > 0 else (k - 1) % 5
            tri = c.poly([(cx, cy), outer[k], inner[j]]) & whole
            mx = (outer[k][0] + inner[j][0]) / 2 - cx
            my = (outer[k][1] + inner[j][1]) / 2 - cy
            b = 0.5 + 0.5 * np.cos(np.arctan2(my, mx) - light)
            c.paint(tri, ramp[3] if b > 0.62 else (ramp[2] if b > 0.3 else ramp[1]))
    core = c.poly(inner)   # the middle, where the face is: one calm tone
    c.paint(core & whole, ramp[2])
    c.paint(c.rect(6, 6, 6, 7) | c.rect(7, 5, 7, 5), ramp[4])      # the glint
    c.paint(c.rect(9, 10, 9, 11) | c.rect(13, 10, 13, 11), '#3b1f4a')   # eyes
    c.paint(c.rect(10, 13, 12, 13), '#3b1f4a')                      # smile
    c.paint(c.rect(9, 12, 9, 12) | c.rect(13, 12, 13, 12), '#3b1f4a')
    return c.img


# ------------------------------------------------------------------ little ones for the cards of the duel
def mini_tavolo():
    """30x20: the table of "sopra e sotto", small (top at y 0..5, legs to y 19)."""
    c = Canvas(30, 20)
    top = c.rect(1, 1, 28, 4, 1)
    legs = c.poly([(3, 5), (7, 5), (6, 18), (4, 18)]) | c.poly([(22, 5), (26, 5), (25, 18), (23, 18)])
    c.ring(top | legs)
    c.ramp(top, 0.85 - (c.yy - 1) / 3.0 * 0.5, WOOD)
    c.paint(c.rect(2, 4, 27, 4) & top, '#a8703e')
    c.ramp(legs, 0.8 - (c.xx % 19 - 3) / 4.0 * 0.6, WOOD[:4], band=0.3)
    return c.img


def mini_scatola(closed=False):
    """24x17: the open box (the dark inside at the top), or closed with its lid."""
    c = Canvas(26, 17)
    face = c.rect(1, 4, 24, 15, 1)
    if closed:
        lid = c.rect(0, 1, 25, 4, 1)
        c.ring(face | lid)
        c.ramp(face, 0.7 - (c.xx - 1) / 23.0 * 0.45, CARD, band=0.3)
        c.ramp(lid, 0.8 - (c.xx) / 25.0 * 0.4, CARD, band=0.3)
        c.paint(c.rect(1, 1, 24, 1) & lid, '#f6d09c')
    else:
        inside = c.rect(2, 1, 23, 5)
        flaps = c.poly([(2, 4), (0, 0), (4, 0), (5, 4)]) | c.poly([(23, 4), (25, 0), (21, 0), (20, 4)])
        c.ring(face | inside | flaps)
        c.paint(inside, '#5a381e')
        c.paint(c.rect(2, 1, 23, 2) & inside, '#3a2414')
        c.ramp(flaps, 0.7 + 0 * c.xx, CARD)
        c.ramp(face, 0.7 - (c.xx - 1) / 23.0 * 0.45, CARD, band=0.3)
    c.paint(c.rect(12, 5, 13, 14) & face, '#e8d8b8')
    return c.img


def mini_scatola_fronte():
    """Only the front face (drawn again over a star inside), same place as in mini_scatola."""
    c = Canvas(26, 17)
    face = c.rect(1, 4, 24, 15, 1)
    c.ring(face)
    c.ramp(face, 0.7 - (c.xx - 1) / 23.0 * 0.45, CARD, band=0.3)
    c.paint(c.rect(12, 5, 13, 14) & face, '#e8d8b8')
    return c.img


def mini_props():
    return {'mini_tavolo': mini_tavolo(), 'mini_scatola': mini_scatola(False),
            'mini_scatola_chiusa': mini_scatola(True), 'mini_scatola_fronte': mini_scatola_fronte()}
