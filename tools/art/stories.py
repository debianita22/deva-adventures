"""Wave 3 art: story frames ("La storia in ordine"), emotion faces ("Le
emozioni di Deva") and the furniture of "Sopra, sotto, dentro, fuori".

Story frames are 36x36 little pictures with their own background, like the
panels of a comic; the three frames of a story share the same background.
"""
import numpy as np
from pixel import (blank, blit, ellipse_mask, rect_mask, shaded_part, rgb, ascii_sprite, dilate4, crop)
import objects as O

S = 36


def _poly(pts, w=S, h=S):
    return O.poly_mask(w, h, pts)


def _e(cx, cy, rx, ry, w=S, h=S):
    return ellipse_mask(w, h, cx, cy, rx, ry)


def _r(x0, y0, x1, y1, radius=0, w=S, h=S):
    return rect_mask(w, h, x0, y0, x1, y1, radius)


def frame(bg, ground=None, ground_h=8):
    """Rounded picture with a background colour (and a strip of ground)."""
    img = blank(S, S)
    m = _r(0, 0, S - 1, S - 1, radius=5)
    img[m, :3] = rgb(bg)
    img[m, 3] = 255
    if ground:
        g = m & _r(0, S - ground_h, S - 1, S - 1)
        img[g, :3] = rgb(ground)
    ring = m & ~_r(1, 1, S - 2, S - 2, radius=4)
    img[ring, :3] = rgb('k')
    return img


def paste(img, sprite, x, y):
    blit(img, sprite, int(x), int(y))
    # keep the rounded border on top
    m = _r(0, 0, S - 1, S - 1, radius=5)
    ring = m & ~_r(1, 1, S - 2, S - 2, radius=4)
    img[ring, :3] = rgb('k')
    img[ring, 3] = 255
    return img


def part(mask, fill, light, dark, outline='k'):
    return shaded_part(mask, fill, light, dark, outline)


# ------------------------------------------------------------------ stories
def seme(n):
    img = frame('b', 'q', 10)
    soil = _e(18, 29, 10, 4)
    paste(img, part(soil, 'J', 'j', 'q'), 0, 0)
    if n == 1:
        paste(img, part(_e(18, 26.5, 2.2, 1.6), 'O', 'F', 'q'), 0, 0)
    else:
        top = 18 if n == 2 else 12
        stem = _r(17, top, 18, 27)
        paste(img, part(stem, 'G', 'g', 'd'), 0, 0)
        for sx in (-1, 1):
            leaf = _e(18 + sx * 3.4, top + 5, 3, 1.6)
            paste(img, part(leaf, 'G', 'g', 'd'), 0, 0)
        if n == 3:
            for i in range(6):
                a = np.deg2rad(i * 60)
                petal = _e(18 + 4 * np.cos(a), 9 + 4 * np.sin(a), 2.6, 2.6)
                paste(img, part(petal, 'P', 'p', 'h'), 0, 0)
            paste(img, part(_e(18, 9, 2.4, 2.4), 'Y', 'y', 'o'), 0, 0)
    return img


def uovo(n):
    img = frame('t', 'Y', 9)
    nest = _e(18, 29, 11, 4)
    if n < 3:
        paste(img, part(nest, 'J', 'j', 'q'), 0, 0)
    if n == 1:
        paste(img, part(_e(18, 20, 6.5, 8), 'W', 'w', 's'), 0, 0)
    elif n == 2:
        low = _e(18, 22, 6.5, 6) & _r(0, 20, S - 1, S - 1)
        paste(img, part(low, 'W', 'w', 's'), 0, 0)
        head = _e(18, 17, 4.6, 4.4)
        paste(img, part(head, 'Y', 'y', 'o'), 0, 0)
        img[16, 19, :3] = rgb('k')
        paste(img, part(_poly([(21.5, 17), (25, 18), (21.5, 19.2)]), 'F', 'f', 'O'), 0, 0)
        for x in range(12, 25, 3):   # jagged shell edge
            img[20, x, :3] = rgb('s')
    else:
        chick = _e(15, 22, 6, 5.4) | _e(15, 15.5, 4.2, 4)
        paste(img, part(chick, 'Y', 'y', 'o'), 0, 0)
        img[15, 16, :3] = rgb('k')
        paste(img, part(_poly([(18.8, 15), (22.5, 16), (18.8, 17.2)]), 'F', 'f', 'O'), 0, 0)
        for dx in (22, 27):   # two halves of the shell
            half = _e(dx + 2, 28, 3.4, 2.6) & _r(0, 28, S - 1, S - 1)
            paste(img, part(half, 'W', 'w', 's'), 0, 0)
    return img


def gelato(n):
    img = frame('p')
    cone = _poly([(13, 17), (23, 17), (18, 32)])
    paste(img, part(cone, 'F', 'f', 'O'), 0, 0)
    for i in range(3):
        img[20 + i * 3, 15 + i:21 - i, :3] = rgb('O')
    if n <= 2:
        paste(img, part(_e(18, 14, 6.2, 4.8), 'T', 't', 'e'), 0, 0)
    if n == 1:
        paste(img, part(_e(18, 8.5, 5.4, 4.4), 'P', 'p', 'h'), 0, 0)
        paste(img, part(_e(18, 4.2, 1.6, 1.6), 'r', 'P', 'R'), 0, 0)
    if n == 3:
        for (x, y) in ((12, 10), (24, 9), (21, 6)):   # crumbs of happiness
            img[y, x, :3] = rgb('F')
    return img


def palloncino(n):
    img = frame('b', 'G', 7)
    if n == 1:
        flat = _e(18, 26.5, 5, 2.2)
        paste(img, part(flat, 'P', 'p', 'h'), 0, 0)
        for x in range(23, 27):
            img[27, x, :3] = rgb('k')
    elif n == 2:
        paste(img, part(_e(18, 12, 6.5, 7.6), 'r', 'P', 'R'), 0, 0)
        for y in range(20, 29):
            img[y, 18 + (1 if y % 4 < 2 else 0), :3] = rgb('k')
        hand = _e(18.5, 29.5, 2.4, 1.8)
        paste(img, part(hand, 'a', 'i', 'A'), 0, 0)
    else:
        paste(img, part(_e(24, 7, 4, 4.8), 'r', 'P', 'R'), 0, 0)
        for y in range(12, 20):
            img[y, 24 - (y - 12) // 3, :3] = rgb('k')
        for (x, y) in ((8, 12), (12, 18), (6, 20)):   # little clouds
            paste(img, part(_e(x, y, 3.4, 1.8), 'W', 'w', 's', 'W'), 0, 0)
    return img


def candela(n):
    img = frame('M')
    tall = {1: 14, 2: 20, 3: 26}[n]
    paste(img, part(_e(18, 30, 9, 2.6), 'Y', 'y', 'o'), 0, 0)
    body = _r(14, tall, 21, 29)
    paste(img, part(body, 'p', 'w', 'P'), 0, 0)
    if n < 3:
        img[tall - 2:tall, 17, :3] = rgb('k')
        img[tall - 2:tall, 17, 3] = 255
        # a teardrop flame: round bottom, pointed top, orange heart
        flame = _e(18, tall - 5.5, 2.8, 2.8) | _poly([(15.4, tall - 5.5), (20.6, tall - 5.5), (18, tall - 12.5)])
        paste(img, part(flame, 'Y', 'y', 'o', 'O'), 0, 0)
        heart = _e(18, tall - 4.8, 1.2, 1.6)
        img[heart, :3] = rgb('F')
    else:
        for i, (x, y) in enumerate(((17, 21), (18, 18), (17, 15), (18, 12))):   # smoke
            img[y, x, :3] = rgb('S')
            img[y, x, 3] = 255
    return img


def giorno(n):
    bg = {1: 'N', 2: 'F', 3: 'b'}[n]
    img = frame(bg, 'G', 8)
    if n == 1:
        moon = _e(22, 11, 6, 6) & ~_e(25, 9, 5.2, 5.2)
        paste(img, part(moon, 'Y', 'y', 'o'), 0, 0)
        for (x, y) in ((7, 7), (12, 14), (8, 19), (28, 20)):
            img[y, x, :3] = rgb('w')
    elif n == 2:
        sun = _e(18, 28, 8, 8) & _r(0, 0, S - 1, 27)
        paste(img, part(sun, 'Y', 'y', 'o'), 0, 0)
        for x in (8, 13, 23, 28):
            img[22, x, :3] = rgb('y')
    else:
        for i in range(8):
            a = np.deg2rad(i * 45)
            ray = _e(18 + 9 * np.cos(a), 11 + 9 * np.sin(a), 1.6, 1.6)
            paste(img, part(ray, 'o', 'Y', 'O'), 0, 0)
        paste(img, part(_e(18, 11, 6.4, 6.4), 'Y', 'y', 'o'), 0, 0)
    return img


STORIES = {'seme': seme, 'uovo': uovo, 'gelato': gelato, 'palloncino': palloncino, 'candela': candela,
           'giorno': giorno}


def story_frames():
    out = {}
    for name, fn in STORIES.items():
        for n in (1, 2, 3):
            out['st_%s_%d' % (name, n)] = fn(n)
    return out


# ------------------------------------------------------------------ emotion faces
def emo_face(kind):
    img = blank(S, S)
    face = _e(18, 18, 16, 16)
    fill = {'arrabbiata': ('F', 'f', 'o')}.get(kind, ('Y', 'y', 'o'))
    blit(img, shaded_part(face, *fill), 0, 0)

    def px(x, y, c):
        img[y, x, :3] = rgb(c)
        img[y, x, 3] = 255

    def eye(cx, cy, kind_eye):
        if kind_eye == 'happy':      # upside-down U
            for (dx, dy) in ((-2, 1), (-1, 0), (0, -1), (1, -1), (2, 0), (3, 1)):
                px(cx + dx, cy + dy, 'k')
        elif kind_eye == 'wide':
            m = _e(cx + 0.5, cy + 0.5, 3.2, 3.6)
            ring = dilate4(m) & ~m
            img[m, :3] = rgb('w')
            img[ring, :3] = rgb('k')
            for (dx, dy) in ((0, 0), (1, 0), (0, 1), (1, 1)):
                px(cx + dx, cy + dy, 'k')
        else:                        # dot eyes
            for (dx, dy) in ((0, 0), (1, 0), (0, 1), (1, 1), (0, 2), (1, 2)):
                px(cx + dx, cy + dy, 'k')

    L, R_, EY = 11, 23, 14
    if kind == 'felice':
        eye(L, EY, 'happy')
        eye(R_, EY, 'happy')
        mouth = _e(18, 23, 7, 5) & _r(0, 23, S - 1, S - 1)
        img[mouth, :3] = rgb('k')
        img[_e(18, 25, 4, 2.4) & mouth, :3] = rgb('h')
        for (x, y) in ((7, 20), (8, 20), (27, 20), (28, 20)):
            px(x, y, 'c')
    elif kind == 'triste':
        eye(L, EY, 'dot')
        eye(R_, EY, 'dot')
        for (dx, dy) in ((-2, -3), (-1, -3), (0, -4), (1, -4)):
            px(L + dx, EY + dy, 'k')
            px(R_ + 1 - dx, EY + dy, 'k')
        for x in range(13, 24):   # frown
            px(x, 25 - (1 if 15 <= x <= 21 else 0), 'k')
        tear = _e(11.5, 21, 1.4, 2.2)
        img[tear, :3] = rgb('B')
    elif kind == 'arrabbiata':
        eye(L, EY + 1, 'dot')
        eye(R_, EY + 1, 'dot')
        for i in range(5):        # brows down towards the middle
            px(L - 2 + i, EY - 4 + i // 2, 'k')
            px(R_ + 3 - i, EY - 4 + i // 2, 'k')
        for x in range(13, 24):
            px(x, 25, 'k')
        for x in range(14, 23, 2):
            px(x, 24, 'k')
    elif kind == 'spaventata':
        # pale blue forehead, like a face going white with fear
        top = face & _r(0, 0, S - 1, 10)
        inner = top & ~(dilate4(~face) & face)
        img[inner, :3] = rgb('b')
        eye(L, EY, 'wide')
        eye(R_, EY, 'wide')
        for x in range(12, 25):   # wobbly mouth
            px(x, 25 + (1 if (x // 2) % 2 else 0), 'k')
        drop = _e(29, 9, 1.6, 2.4)
        img[drop, :3] = rgb('b')
        img[dilate4(drop) & ~drop & face, :3] = rgb('B')
    elif kind == 'sorpresa':
        eye(L, EY, 'wide')
        eye(R_, EY, 'wide')
        for (dx, dy) in ((-2, -5), (-1, -6), (0, -6), (1, -5)):
            px(L + dx, EY + dy, 'k')
            px(R_ + dx, EY + dy, 'k')
        o = _e(18, 26, 3.4, 3.8)
        img[o, :3] = rgb('k')
        img[_e(18, 26, 2, 2.4), :3] = rgb('h')
    return img


def emotion_faces():
    return {'emo_' + k: emo_face(k) for k in ('felice', 'triste', 'arrabbiata', 'spaventata', 'sorpresa')}


# ------------------------------------------------------------------ "sopra e sotto"
def tavolo():
    w, h = 64, 40
    img = blank(w, h)
    for x0 in (6, 52):
        leg = rect_mask(w, h, x0, 8, x0 + 5, h - 2)
        blit(img, shaded_part(leg, 'J', 'j', 'q'), 0, 0)
    top = rect_mask(w, h, 1, 2, w - 2, 8, radius=2)
    blit(img, shaded_part(top, 'J', 'j', 'q'), 0, 0)
    return img


def scatola():
    """An open cardboard box: the whole box, and its front face alone (drawn
    again over an object inside, so the object sits in the box)."""
    w, h = 52, 40
    back = blank(w, h)
    inside = rect_mask(w, h, 3, 1, w - 4, 12, radius=1)
    blit(back, shaded_part(inside, 'O', 'o', 'q'), 0, 0)
    front = blank(w, h)
    face = rect_mask(w, h, 1, 10, w - 2, h - 2, radius=2)
    blit(front, shaded_part(face, 'F', 'f', 'o'), 0, 0)
    tape = rect_mask(w, h, w // 2 - 3, 11, w // 2 + 2, h - 3)
    front[tape & face, :3] = rgb('y')
    whole = back.copy()
    blit(whole, front, 0, 0)
    return {'scatola': whole, 'scatola_fronte': front}


def casa_grande():
    O.K = 3.5
    img = O.casa()
    O.K = 1.0
    return img


def gelato_caduto():
    """The ice cream of "Le emozioni": fallen on the floor, cone up."""
    img = blank(S, S)
    splat = _e(18, 30, 13, 3.6) | _e(11, 28.5, 4, 2.6) | _e(25, 28.2, 4.4, 2.8)
    blit(img, shaded_part(splat, 'P', 'p', 'h'), 0, 0)
    cone = _poly([(13, 29), (23, 29), (18, 12)])
    blit(img, shaded_part(cone, 'F', 'f', 'O'), 0, 0)
    for i in range(3):
        img[26 - i * 4, 16 + i // 2:21 - i // 2, :3] = rgb('O')
    for (x, y) in ((5, 25), (31, 24), (8, 32)):   # drops
        img[y, x, :3] = rgb('P')
        img[y, x, 3] = 255
    return img


def dove_props():
    import props   # redrawn in 0.8.0 (same sizes and anchors); the old ones above for reference
    out = {'tavolo': props.tavolo(), 'casa_grande': props.casa_grande(), 'pic_gelato_caduto': gelato_caduto()}
    out.update(props.scatola())
    out.update(props.mini_props())
    return out


# ------------------------------------------------------------------ menu icons
def _mini_face(kind, size=26):
    img = blank(size, size)
    c = size / 2 - 0.5
    face = ellipse_mask(size, size, c, c, size / 2 - 1, size / 2 - 1)
    blit(img, shaded_part(face, 'Y', 'y', 'o'), 0, 0)

    def px(x, y, col='k'):
        img[y, x, :3] = rgb(col)
        img[y, x, 3] = 255
    if kind == 'felice':
        for ex in (7, 16):
            for (dx, dy) in ((0, 1), (1, 0), (2, 0), (3, 1)):
                px(ex + dx, 9 + dy)
        mouth = ellipse_mask(size, size, c, 15.5, 5.5, 4.5) & rect_mask(size, size, 0, 15, size - 1, size - 1)
        img[mouth, :3] = rgb('k')
        img[ellipse_mask(size, size, c, 17.5, 3, 1.6) & mouth, :3] = rgb('h')
    else:
        for ex in (8, 16):
            for (dx, dy) in ((0, 0), (1, 0), (0, 1), (1, 1)):
                px(ex + dx, 10 + dy)
        for x in range(9, 17):
            px(x, 18 - (1 if 11 <= x <= 14 else 0))
        img[ellipse_mask(size, size, 8.5, 15, 1.2, 1.8), :3] = rgb('B')
    return img


def menu_icons():
    import objects as O2
    icons = {}
    # "sopra e sotto": a star on a little table, a ball under it
    img = blank(44, 44)
    top = rect_mask(44, 44, 2, 20, 41, 24, radius=1)
    legs = rect_mask(44, 44, 5, 24, 8, 42) | rect_mask(44, 44, 35, 24, 38, 42)
    blit(img, shaded_part(legs, 'J', 'j', 'q'), 0, 0)
    blit(img, shaded_part(top, 'J', 'j', 'q'), 0, 0)
    O2.K = 0.8
    st = crop(O2.stella())
    O2.K = 0.65
    ball = crop(O2.palla())
    O2.K = 1.0
    blit(img, st, 22 - st.shape[1] // 2, 20 - st.shape[0])
    blit(img, ball, 22 - ball.shape[1] // 2, 43 - ball.shape[0])
    icons['menu_dove'] = img
    # emotions: a sad face behind a happy one
    img = blank(44, 44)
    blit(img, _mini_face('triste'), 18, 18)
    blit(img, _mini_face('felice'), 0, 0)
    icons['menu_emozioni'] = img
    # stories: three little pictures of the flower story, in order
    img = blank(44, 44)
    for i in range(3):
        w, h = 14, 30
        p = blank(w, h)
        m = rect_mask(w, h, 0, 0, w - 1, h - 1, radius=3)
        p[m, :3] = rgb('b')
        p[m, 3] = 255
        p[m & rect_mask(w, h, 0, h - 8, w - 1, h - 1), :3] = rgb('q')
        ring = m & ~rect_mask(w, h, 1, 1, w - 2, h - 2, radius=2)
        p[ring, :3] = rgb('k')
        mound = ellipse_mask(w, h, 6.5, h - 7.5, 4.2, 2)
        p[mound, :3] = rgb('J')
        if i >= 1:
            stem_top = h - 14 if i == 1 else h - 21
            p[stem_top:h - 8, 6, :3] = rgb('d')
            p[stem_top + 2, 4:6, :3] = rgb('G')
            p[stem_top + 3, 7:9, :3] = rgb('G')
        if i == 2:
            fl = ellipse_mask(w, h, 6.5, h - 23, 3, 3)
            p[fl, :3] = rgb('P')
            p[ellipse_mask(w, h, 6.5, h - 23, 1, 1), :3] = rgb('Y')
        blit(img, p, i * 15, 0)
        d = glyph_small(str(i + 1))
        blit(img, d, i * 15 + (14 - d.shape[1]) // 2, 32)
    icons['menu_storie'] = img
    return icons


def glyph_small(ch):
    import ui
    return ui.glyph(ui.SMALL[ch], 'w', 'k', diag=True)


def story_slots():
    import ui
    return {'story_slot': ui.card(44, 44, 'p', 'W'), 'story_slot_q': ui.card(44, 44, 'h', 'w')}


def all_sprites():
    out = {}
    out.update(story_slots())
    out.update(story_frames())
    out.update(emotion_faces())
    out.update(dove_props())
    out.update(menu_icons())
    return out
