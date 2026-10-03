"""0.13.0: the blink of the characters: for every character that stays on the screen a while (the
monsters, under the spell and set free, the witch, the wizard, the ogre, the little king, the Mago
Pistacchio) a copy with the eyes closed, <name>_bl; the game shows it for a few frames now and then
(src/anim.c, actor_draw).

The eyes are found where the drawings put them: the helpers that paint eyes (tale.eye_round,
tale2.angry_eyes, tale3._eye) are wrapped to note every eye while each character is drawn, and the
few characters with eyes of their own are listed by hand (EYES). A closed eye: the eye painted over
with the colour of the skin around it, and a little lid, a curve, in the ink of the drawing.
"""
import numpy as np

import tale as T1
import tale2 as T2
import tale3 as T3
import tale4 as T4
from pixel import rgb

LOG = []


def _wrap(mod, name, eyes_of):
    orig = getattr(mod, name)

    def f(*a, **k):
        LOG.extend(eyes_of(*a, **k))
        return orig(*a, **k)
    f.__wrapped__ = orig
    setattr(mod, name, f)


def _round(img, cx, cy, r=4, look=(0, 0)):
    return [(cx, cy, r, r + 0.5)]


def _angry(img, y, dx=8, cx=32, iris=None, slit=False, rx=4.5, ry=3.6):
    return [(cx - dx, y, rx, ry), (cx + dx, y, rx, ry)]


def _ogre(img, ex, ey, lid=0, look=0):
    return [(ex, ey, 2.6, 3.2)]


_installed = False


def _install():
    global _installed
    if _installed:
        return
    for m in (T1, T2, T3, T4):
        if hasattr(m, 'eye_round'):
            _wrap(m, 'eye_round', _round)
    for m in (T2, T3, T4):
        _wrap(m, 'angry_eyes', _angry)
    _wrap(T3, '_eye', _ogre)
    _installed = True


# eyes drawn by hand in the generators: (cx, cy, rx, ry), read in their code
EYES = {
    'mo_ciuffone_c': [(24, 30, 4.5, 3.6), (40, 30, 4.5, 3.6)],
    'mo_melmoso_c': [(32, 29, 9, 8)],
    'mo_melmoso_b': [(32, 29, 9, 8.5)],
    'mo_tuonello_c': [(24, 29, 4, 3.5), (40, 29, 4, 3.5)],
    'mo_rocciolo_c': [(24.5, 32.5, 3.5, 1.5), (40.5, 32.5, 3.5, 1.5)],     # glowing slits under the brow
    'mo_lumacone_c': [(7, 12, 4.6, 4.2), (22, 12, 4.6, 4.2)],             # on the stalks
    'mo_ranocchione_c': [(20, 21, 5.5, 4.5), (44, 21, 5.5, 4.5)],
    'mo_tuonello_b': [],                                                 # its eyes are closed already (happy)
}
for f in ('b', 'bp'):
    EYES['strega_' + f] = [(21.5, 31, 1.5, 2), (31.5, 31, 1.5, 2)]
    EYES['stregone_' + f] = [(22.5, 29, 1.5, 2), (30.5, 29, 1.5, 2)]
for f in ('c', 'cp'):
    EYES['strega_' + f] = [(21.5, 31, 2.5, 1.0), (31.5, 31, 2.5, 1.0)]
    EYES['stregone_' + f] = [(22, 28, 2.2, 1.6), (30, 28, 2.2, 1.6)]
for f in ('c', 'cp', 'b', 'bp'):
    EYES['re_' + f] = [(19, 25, 2.6, 3), (29, 25, 2.6, 3)]
for n in ('mago', 'mago_p'):   # behind the round glasses: the lens stays blue
    EYES[n] = [(21, 29.4, 1.6, 1.8, 'b'), (31, 29.4, 1.6, 1.8, 'b')]

# what blinks: the forms that stand on the screen a while ('r' squirms, its eyes are squeezed)
MONSTERS = {}
for mod, table in ((T1, T1.MONSTERS), (T2, T2.MONSTERS2), (T3, T3.MONSTERS3), (T4, T4.MONSTERS4)):
    for name, fn in table.items():
        MONSTERS[name] = fn
# (the talking forms, _p, blink with the face of the silent one: anim.c drops the _p)
CHARACTERS = [('strega_' + f, T1.strega, f) for f in ('c', 'b')] + \
             [('stregone_' + f, T2.stregone, f) for f in ('c', 'b')] + \
             [('orco_' + f, T3.orco, f) for f in ('c', 'b', 'stanco')] + \
             [('re_' + f, T4.re, f) for f in ('c', 'b')] + \
             [('mago', T1.mago, '')]


def eyes_of(fn, form):
    """Draw it once, noting the eyes the helpers paint."""
    _install()
    del LOG[:]
    img = fn(form)
    return img, list(LOG)


INKS = ('k', 'K', '0')   # the outlines: never the colour of a skin


def close_eyes(img, eyes, ink='k'):
    """The eyes painted over with the skin around them, and a lid: a little curve, lower in the
    middle (a happy, sleepy eye)."""
    out = img.copy()
    h, w = img.shape[:2]
    yy, xx = np.mgrid[0:h, 0:w]
    opaque = img[..., 3] > 0
    k = np.array(rgb(ink), np.int32)
    for e in eyes:
        (cx, cy, rx, ry), fill = e[:4], (e[4] if len(e) > 4 else None)
        d = ((xx + 0.5 - cx) / (rx + 1.3)) ** 2 + ((yy + 0.5 - cy) / (ry + 1.3)) ** 2
        eye = (d <= 1.0) & opaque
        ring = (((xx + 0.5 - cx) / (rx + 3.2)) ** 2 + ((yy + 0.5 - cy) / (ry + 3.2)) ** 2 <= 1.0) & ~eye & opaque
        cols = img[ring][:, :3].astype(np.int32)
        inks = np.array([rgb(c) for c in INKS], np.int32)
        not_ink = ~(cols[:, None, :] == inks[None, :, :]).all(axis=2).any(axis=1)
        cols = cols[not_ink & (cols.sum(axis=1) > 200)]   # not the ink of the outlines
        if len(cols) == 0 and fill is None:
            continue
        keys, counts = np.unique(cols, axis=0, return_counts=True)
        skin = np.array(rgb(fill), np.int32) if fill else keys[counts.argmax()]
        out[eye, :3] = skin
        # the lid: from one corner to the other, a pixel lower in the middle
        x0, x1 = int(round(cx - rx + 0.5)), int(round(cx + rx - 0.5))
        for x in range(x0, x1 + 1):
            u = (x + 0.5 - cx) / max(rx, 1.0)
            y = int(round(cy + 0.2 * ry + (1.0 - u * u) * max(1.0, ry * 0.35)))
            if 0 <= y < h and 0 <= x < w and out[y, x, 3] > 0:
                out[y, x, :3] = k
                if rx >= 6 and 0 <= y - 1 < h and abs(u) < 0.85:   # a big eye: a thicker lid
                    out[y - 1, x, :3] = k
    return out


def all_sprites():
    out = {}
    missing = []
    for name, fn in MONSTERS.items():
        for form in ('c', 'b'):
            sname = 'mo_%s_%s' % (name, form)
            img, eyes = eyes_of(fn, form)
            eyes = EYES.get(sname, eyes)
            if eyes:
                out[sname + '_bl'] = close_eyes(img, eyes)
            elif sname not in EYES:
                missing.append(sname)
    for sname, fn, form in CHARACTERS:
        img, eyes = eyes_of(fn, form)
        eyes = EYES.get(sname, eyes)
        if eyes:
            out[sname + '_bl'] = close_eyes(img, eyes)
        else:
            missing.append(sname)
    if missing:
        print('blink: no eyes for', ' '.join(missing))
    return out


if __name__ == '__main__':
    s = all_sprites()
    print(len(s), 'blink sprites')
