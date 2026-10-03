#!/usr/bin/env python3
"""Synthesise the sound effects and the original music loops of
Deva's Awesome Adventures.

Everything is generated from code (no samples), mono 22050 Hz, mastered for
the small speaker on the back of the XiFan RF35H: nothing under ~150 Hz (it
cannot play it and it only eats headroom), bass lines an octave up and rich in
harmonics, so the ear still hears them.
  data/deva_adventures/sfx/*.wav     short effects, 16-bit PCM (no decoding cost)
  data/deva_adventures/musica/*.ogg  loops: 'palco' (stage, 112 BPM), 'nanna'
                              (music-box lullaby, 3/4), 'mappa', 'sfida', 'notte' and
                              'valle' (the map of the third adventure, 0.10),
                              'giocattoli' (the map of the fourth, 0.11)
  make_audio.py [names]              only those effects/loops (the others untouched)
"""
import os
import subprocess
import tempfile
import wave

import numpy as np

SR = 22050
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SFX_DIR = os.path.join(ROOT, 'data', 'deva_adventures', 'sfx')
MUS_DIR = os.path.join(ROOT, 'data', 'deva_adventures', 'musica')

NOTE = {n: i for i, n in enumerate(['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'])}


def hz(name):
    """'C5' -> frequency (A4 = 440)."""
    n, o = name[:-1], int(name[-1])
    midi = 12 * (o + 1) + NOTE[n]
    return 440.0 * 2 ** ((midi - 69) / 12)


def t_axis(dur):
    return np.arange(int(SR * dur)) / SR


def env_adsr(n, a=0.005, d=0.05, s=0.6, r=0.05):
    a, d, r = int(SR * a), int(SR * d), int(SR * r)
    s_len = max(0, n - a - d - r)
    e = np.concatenate([np.linspace(0, 1, a, endpoint=False), np.linspace(1, s, d, endpoint=False),
                        np.full(s_len, s), np.linspace(s, 0, r)])
    return e[:n] if len(e) >= n else np.pad(e, (0, n - len(e)))


def osc(kind, f, dur, phase=0.0):
    t = t_axis(dur)
    if np.ndim(f):
        ph = 2 * np.pi * np.cumsum(f) / SR + phase
    else:
        ph = 2 * np.pi * f * t + phase
    if kind == 'sine':
        return np.sin(ph)
    if kind == 'tri':
        return 2 / np.pi * np.arcsin(np.sin(ph))
    if kind == 'square':
        return np.tanh(3 * np.sin(ph)) * 0.7          # soft-edged square
    if kind == 'pulse':
        return np.where((ph / (2 * np.pi)) % 1.0 < 0.25, 1.0, -1.0) * 0.6
    raise ValueError(kind)


def lowpass(x, cutoff):
    a = np.exp(-2 * np.pi * cutoff / SR)
    y = np.empty_like(x)
    acc = 0.0
    for i, v in enumerate(x):
        acc = (1 - a) * v + a * acc
        y[i] = acc
    return y


def highpass(x, cutoff, q=0.7071):
    """2nd-order high-pass (RBJ biquad)."""
    w0 = 2 * np.pi * cutoff / SR
    alpha = np.sin(w0) / (2 * q)
    cw = np.cos(w0)
    b0, b1, b2 = (1 + cw) / 2, -(1 + cw), (1 + cw) / 2
    a0, a1, a2 = 1 + alpha, -2 * cw, 1 - alpha
    b0, b1, b2, a1, a2 = b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0
    y = np.empty_like(x)
    x1 = x2 = y1 = y2 = 0.0
    for i, v in enumerate(x):
        out = b0 * v + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        x2, x1, y2, y1 = x1, v, y1, out
        y[i] = out
    return y


def speaker_master(x, peak):
    """Cut what the handheld speaker cannot reproduce, then normalise."""
    return norm(highpass(highpass(x, 150), 150), peak)


def norm(x, peak=0.85):
    m = np.abs(x).max() or 1
    return x * (peak / m)


def write_wav(path, x):
    pcm = (np.clip(x, -1, 1) * 32767).astype('<i2')
    with wave.open(path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())


# ------------------------------------------------------------------ effects
def sfx_pop():
    n = int(SR * 0.07)
    f = np.linspace(700, 260, n)
    return norm(osc('sine', f, 0.07) * np.exp(-np.linspace(0, 6, n)), 0.6)


def sfx_blip():
    n = int(SR * 0.045)
    return norm(osc('tri', 988, 0.045) * env_adsr(n, 0.002, 0.01, 0.7, 0.02), 0.45)


def sfx_ding():
    out = np.zeros(int(SR * 0.6))
    for i, name in enumerate(['C6', 'E6', 'G6', 'C7']):
        start = int(SR * 0.07 * i)
        d = 0.6 - 0.07 * i
        n = int(SR * d)
        tone = osc('sine', hz(name), d) + 0.3 * osc('sine', 2 * hz(name), d)
        tone *= np.exp(-np.linspace(0, 5, n))
        out[start:start + n] += tone[:len(out) - start]
    return norm(out, 0.7)


def sfx_boop():
    n = int(SR * 0.22)
    f = np.linspace(392, 262, n)
    return norm(osc('tri', f, 0.22) * env_adsr(n, 0.01, 0.05, 0.6, 0.08), 0.45)


def sfx_sparkle():
    rng = np.random.default_rng(3)
    out = np.zeros(int(SR * 0.5))
    penta = ['C7', 'D7', 'E7', 'G7', 'A7', 'C8']
    for i in range(9):
        start = int(SR * 0.04 * i)
        d = 0.18
        n = int(SR * d)
        tone = osc('sine', hz(penta[rng.integers(0, len(penta))]), d) * np.exp(-np.linspace(0, 7, n))
        out[start:start + n] += tone[:len(out) - start] * (1 - i / 12)
    return norm(out, 0.5)


def sfx_star():
    n = int(SR * 0.3)
    f = np.geomspace(660, 1320, n)
    x = osc('sine', f, 0.3) + 0.25 * osc('sine', 2 * f, 0.3)
    return norm(x * env_adsr(n, 0.005, 0.05, 0.7, 0.15), 0.55)


def sfx_fanfare():
    notes = [('G5', 0.12), ('C6', 0.12), ('E6', 0.12), ('G6', 0.5)]
    out = np.zeros(int(SR * 0.95))
    pos = 0
    for name, d in notes:
        n = int(SR * d)
        tone = osc('square', hz(name), d) * 0.6 + osc('tri', hz(name) / 2, d) * 0.4
        tone = lowpass(tone, 3500) * env_adsr(n, 0.005, 0.04, 0.75, 0.06)
        out[pos:pos + n] += tone
        pos += n
    return norm(out, 0.6)


def sfx_whoosh():
    rng = np.random.default_rng(5)
    n = int(SR * 0.6)
    noise = rng.standard_normal(n)
    y = np.zeros(n)
    acc = 0.0
    for i in range(n):
        c = 300 + 2500 * np.sin(np.pi * i / n)
        a = np.exp(-2 * np.pi * c / SR)
        acc = (1 - a) * noise[i] + a * acc
        y[i] = acc
    return norm(y * np.sin(np.linspace(0, np.pi, n)), 0.45)


def sfx_yawn():
    n = int(SR * 0.9)
    f = np.concatenate([np.linspace(330, 440, n // 3), np.linspace(440, 247, n - n // 3)])
    x = osc('sine', f, 0.9) + 0.2 * osc('sine', 2 * f, 0.9)
    return norm(lowpass(x, 1200) * env_adsr(n, 0.08, 0.1, 0.8, 0.3), 0.4)


def tone(name):
    """Bell-like note for the dance moves (Simon-style: one note per arrow)."""
    def fn():
        n = int(SR * 0.32)
        f = hz(name)
        x = osc('tri', f, 0.32) * 0.7 + osc('sine', 2 * f, 0.32) * 0.25
        return norm(x * np.exp(-np.linspace(0, 4.5, n)) * np.clip(np.arange(n) / (SR * 0.004), 0, 1), 0.6)
    return fn


def sfx_clap():
    rng = np.random.default_rng(9)
    n = int(SR * 0.12)
    x = rng.standard_normal(n)
    x = x - lowpass(x, 900)
    env = np.exp(-np.linspace(0, 12, n))
    env[:int(SR * 0.012)] *= np.linspace(0.3, 1, int(SR * 0.012))
    return norm(lowpass(x, 5000) * env, 0.55)


# ---- the tale: spells, monsters, a little thunder
def sfx_tuono():
    """Thunder: a crack, then a rumble that rolls away (kept above the speaker's cut)."""
    rng = np.random.default_rng(21)
    n = int(SR * 1.6)
    noise = rng.standard_normal(n)
    y = np.zeros(n)
    acc = 0.0
    for i in range(n):
        c = 2400 * np.exp(-i / (SR * 0.08)) + 260
        a = np.exp(-2 * np.pi * c / SR)
        acc = (1 - a) * noise[i] + a * acc
        y[i] = acc
    t = np.arange(n) / SR
    rumble = 0.6 + 0.4 * np.sin(2 * np.pi * 7 * t) * np.sin(2 * np.pi * 1.3 * t)
    env = np.exp(-t * 2.2) * np.clip(t / 0.004, 0, 1)
    return speaker_master(y * env * rumble, 0.55)


def sfx_ruggito():
    """A monster's growl: a buzzy low voice that rises and falls, with flutter."""
    rng = np.random.default_rng(22)
    d = 0.9
    n = int(SR * d)
    t = np.arange(n) / SR
    f = 120 + 50 * np.sin(np.pi * t / d)
    x = osc('square', f, d) + 0.5 * osc('square', f * 1.5, d)
    x *= 0.55 + 0.45 * np.sin(2 * np.pi * 27 * t)
    x += 0.25 * lowpass(rng.standard_normal(n), 1200)
    x = lowpass(x, 1300) * env_adsr(n, 0.05, 0.1, 0.8, 0.25)
    return speaker_master(x, 0.55)


def sfx_incantesimo():
    """Spell: a rising sparkle glissando with a soft whoosh."""
    rng = np.random.default_rng(23)
    d = 0.7
    n = int(SR * d)
    f = np.geomspace(500, 2000, n)
    x = osc('sine', f, d) * 0.6 + osc('tri', f * 2, d) * 0.2
    x *= env_adsr(n, 0.01, 0.1, 0.7, 0.3)
    out = x.copy()
    penta = ['C7', 'E7', 'G7', 'A7', 'C8', 'D8']
    for i in range(7):
        st = int(SR * (0.2 + 0.06 * i))
        dd = 0.16
        m = int(SR * dd)
        tone = osc('sine', hz(penta[i % len(penta)]), dd) * np.exp(-np.linspace(0, 7, m))
        out[st:st + m] += tone[:max(0, n - st)] * 0.5
    w = lowpass(rng.standard_normal(n), 3000) * np.sin(np.linspace(0, np.pi, n)) * 0.15
    return speaker_master(out + w, 0.55)


def sfx_rottura():
    """An orb breaking: glassy pings and a small crash."""
    rng = np.random.default_rng(24)
    n = int(SR * 0.5)
    out = np.zeros(n)
    for i in range(9):
        st = int(SR * rng.uniform(0, 0.12))
        dd = rng.uniform(0.08, 0.25)
        m = int(SR * dd)
        f = rng.uniform(2200, 5200)
        tone = osc('sine', f, dd) * np.exp(-np.linspace(0, 9, m))
        out[st:st + m] += tone[:n - st] * rng.uniform(0.3, 0.7)
    crash = rng.standard_normal(int(SR * 0.12))
    crash = (crash - lowpass(crash, 2500)) * np.exp(-np.linspace(0, 8, len(crash)))
    out[:len(crash)] += crash * 0.5
    return speaker_master(out, 0.5)


def sfx_trasforma():
    """The spell breaks: a shimmer up and down, and a warm major chord."""
    d = 1.4
    n = int(SR * d)
    out = np.zeros(n)
    seq = ['C6', 'E6', 'G6', 'C7', 'E7', 'G7', 'C8', 'G7', 'E7', 'C7', 'G6', 'E6']
    for i, name in enumerate(seq):
        st = int(SR * 0.07 * i)
        dd = 0.25
        m = int(SR * dd)
        tone = osc('sine', hz(name), dd) * np.exp(-np.linspace(0, 6, m))
        out[st:st + m] += tone[:n - st] * 0.5
    t = np.arange(n) / SR
    chord = sum(osc('tri', hz(c), d) for c in ('C5', 'E5', 'G5')) / 3
    chord *= np.clip(t / 0.4, 0, 1) * np.exp(-np.clip(t - 0.6, 0, None) * 3) * (0.8 + 0.2 * np.sin(2 * np.pi * 6 * t))
    return speaker_master(out + chord * 0.6, 0.55)


def sfx_schivata():
    """The monster dodges: a springy boing."""
    d = 0.35
    n = int(SR * d)
    t = np.arange(n) / SR
    f = 330 + 160 * np.sin(2 * np.pi * 9 * t) * np.exp(-t * 6)
    x = osc('tri', f, d) * np.exp(-t * 7)
    return speaker_master(x, 0.5)


def sfx_poof():
    """A puff of magic smoke: characters appear or vanish."""
    rng = np.random.default_rng(25)
    n = int(SR * 0.4)
    noise = rng.standard_normal(n)
    y = np.zeros(n)
    acc = 0.0
    for i in range(n):
        c = 3000 * np.exp(-i / (SR * 0.12)) + 300
        a = np.exp(-2 * np.pi * c / SR)
        acc = (1 - a) * noise[i] + a * acc
        y[i] = acc
    return speaker_master(y * np.exp(-np.linspace(0, 5, n)) * np.clip(np.arange(n) / (SR * 0.01), 0, 1), 0.5)


def sfx_boing():
    """A jump on the trampoline: a spring that goes up."""
    d = 0.32
    n = int(SR * d)
    t = np.arange(n) / SR
    f = 220 + 520 * (1 - np.exp(-t * 9)) + 40 * np.sin(2 * np.pi * 16 * t) * np.exp(-t * 5)
    x = osc('tri', f, d) * np.exp(-t * 5.5) * np.clip(t / 0.004, 0, 1)
    return speaker_master(x, 0.5)


def sfx_bloop():
    """A shape drops into the cauldron: bloop, and a couple of bubbles."""
    d = 0.45
    n = int(SR * d)
    t = np.arange(n) / SR
    f = 700 * np.exp(-t * 5) + 180
    x = osc('sine', f, d) * np.exp(-t * 7)
    for (t0, f0) in ((0.18, 900), (0.28, 1200)):
        k = (t >= t0) & (t < t0 + 0.06)
        tt = t[k] - t0
        x[k] += 0.5 * np.sin(2 * np.pi * (f0 + 3000 * tt) * tt) * np.exp(-tt * 40)
    return speaker_master(x, 0.55)


def sfx_pennello():
    """A soft brush stroke: make-up goes on."""
    rng = np.random.default_rng(31)
    d = 0.28
    n = int(SR * d)
    t = np.arange(n) / SR
    noise = highpass(rng.standard_normal(n), 2500)
    env = np.sin(np.pi * np.clip(t / d, 0, 1)) ** 1.5
    return norm(noise * env, 0.3)


# ---- the third tale (0.10): the five magic notes, the ogre stamping and snoring
def nota(name):
    """A magic note: a celesta-like bell (sine, a soft octave, a glassy 3rd harmonic that
    fades first) with a twinkle on top; long enough to ring under the next one."""
    def fn():
        d = 1.1
        n = int(SR * d)
        t = np.arange(n) / SR
        f = hz(name)
        x = np.sin(2 * np.pi * f * t) + 0.3 * np.sin(2 * np.pi * 2 * f * t) * np.exp(-t * 2.5) \
            + 0.12 * np.sin(2 * np.pi * 3 * f * t) * np.exp(-t * 7)
        x *= np.exp(-t * 2.6) * np.clip(t * 500, 0, 1)
        tw = np.sin(2 * np.pi * 4 * f * t) * np.exp(-t * 18) * 0.12   # the sparkle of the magic
        return norm(x + tw, 0.6)
    return fn


def sfx_pestone():
    """The ogre stamps his feet, twice: a round wooden thump (no deep bass: the speaker)."""
    rng = np.random.default_rng(21)
    out = np.zeros(int(SR * 0.85))
    for t0, g in ((0.0, 1.0), (0.42, 0.85)):
        n = int(SR * 0.3)
        f = np.linspace(190, 85, n)
        body = np.sin(2 * np.pi * np.cumsum(f) / SR) + 0.5 * np.sin(4 * np.pi * np.cumsum(f) / SR)
        body *= np.exp(-np.linspace(0, 8, n))
        dust = lowpass(rng.standard_normal(n), 1200) * np.exp(-np.linspace(0, 14, n)) * 0.6
        i = int(SR * t0)
        out[i:i + n] += (body + dust) * g
    return norm(highpass(out, 70), 0.62)


def sfx_ronfo():
    """A cartoon snore: a soft purring "rrr" breathing in, a little whistle breathing out."""
    rng = np.random.default_rng(22)
    n1, n2 = int(SR * 0.7), int(SR * 0.55)
    t1 = np.arange(n1) / SR
    purr = lowpass(rng.standard_normal(n1), 700) * (0.6 + 0.4 * np.sin(2 * np.pi * 28 * t1))
    purr += 0.4 * osc('pulse', np.linspace(95, 120, n1), 0.7) * (0.5 + 0.5 * np.sin(2 * np.pi * 28 * t1))
    purr = lowpass(purr, 900) * np.sin(np.linspace(0, np.pi, n1)) ** 0.7
    f = np.linspace(1250, 780, n2)
    whistle = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.sin(np.linspace(0, np.pi, n2)) ** 1.5
    x = np.concatenate([purr * 0.8, np.zeros(int(SR * 0.08)), whistle * 0.35])
    return norm(highpass(x, 120), 0.45)


# ---- the fourth tale (0.11): the king's toy trumpet, the wind-up keys, the toy train; the shop, the scales
def sfx_trombetta():
    """A toy trumpet: ta-ta-taa, nasal and a little cheeky (the little king arrives)."""
    out = np.zeros(int(SR * 0.9))
    for t0, d, name in ((0.0, 0.11, 'G5'), (0.14, 0.11, 'G5'), (0.28, 0.5, 'C6')):
        n = int(SR * d)
        t = np.arange(n) / SR
        f = hz(name) * (1 + 0.012 * np.sin(2 * np.pi * 6.5 * t) * np.clip(t * 6, 0, 1))
        x = osc('pulse', f, d) * 0.6 + osc('square', f, d) * 0.4
        x = lowpass(x, 3200) * env_adsr(n, 0.008, 0.04, 0.75, min(0.06, d / 3))
        i = int(SR * t0)
        out[i:i + n] += x
    return speaker_master(out, 0.5)


def sfx_carica():
    """A wind-up key turning: cric-cric-cric, faster and higher, then a little bell: ready!"""
    rng = np.random.default_rng(41)
    out = np.zeros(int(SR * 1.1))
    t0, k = 0.0, 0
    while t0 < 0.62:
        n = int(SR * 0.02)
        click = highpass(rng.standard_normal(n), 2000) * np.exp(-np.linspace(0, 9, n))
        f = 1800 + k * 90
        click += 0.6 * osc('sine', f, 0.02) * np.exp(-np.linspace(0, 12, n))
        i = int(SR * t0)
        out[i:i + n] += click * (1.0 + 0.5 * (k % 2))
        t0 += 0.085 - k * 0.003
        k += 1
    d = 0.45
    n = int(SR * d)
    t = np.arange(n) / SR
    bell = np.sin(2 * np.pi * hz('E6') * t) + 0.4 * np.sin(2 * np.pi * hz('B6') * t) * np.exp(-t * 8)
    i = int(SR * 0.64)
    out[i:i + n] += 0.7 * bell * np.exp(-t * 6)
    return speaker_master(out, 0.55)


def sfx_treno():
    """The toy train whistles: tu-tuu! Two soft reeds a sixth apart, a breath of steam."""
    rng = np.random.default_rng(43)
    out = np.zeros(int(SR * 1.0))
    for t0, d in ((0.0, 0.18), (0.26, 0.55)):
        n = int(SR * d)
        t = np.arange(n) / SR
        x = osc('tri', hz('E5') * (1 + 0.004 * np.sin(2 * np.pi * 5 * t)), d) \
            + 0.8 * osc('tri', hz('C#6') * (1 + 0.004 * np.sin(2 * np.pi * 5.3 * t)), d)
        steam = lowpass(highpass(rng.standard_normal(n), 1500), 5000) * 0.25
        x = (x + steam) * env_adsr(n, 0.03, 0.05, 0.85, min(0.08, d / 3))
        i = int(SR * t0)
        out[i:i + n] += x
    return speaker_master(out, 0.45)


def sfx_coin():
    """A coin: a bright metal clink (inharmonic partials that die at once)."""
    d = 0.3
    n = int(SR * d)
    t = np.arange(n) / SR
    x = np.zeros(n)
    for f, a, k in ((2093, 1.0, 14), (3140, 0.6, 18), (4420, 0.4, 24), (5600, 0.25, 30)):
        x += a * np.sin(2 * np.pi * f * t) * np.exp(-t * k)
    j = int(SR * 0.06)   # the little bounce
    x[j:] += 0.45 * x[:n - j]
    return norm(x * np.clip(t * 2000, 0, 1), 0.55)


def sfx_cassa():
    """The toy till: the drawer slides out with a knock, and the bell rings: ka-ching!"""
    rng = np.random.default_rng(45)
    out = np.zeros(int(SR * 0.9))
    n = int(SR * 0.08)
    knock = lowpass(rng.standard_normal(n), 1800) * np.exp(-np.linspace(0, 10, n))
    knock += 0.6 * osc('sine', np.linspace(420, 260, n), 0.08) * np.exp(-np.linspace(0, 8, n))
    out[:n] += knock
    d = 0.7
    n = int(SR * d)
    t = np.arange(n) / SR
    bell = np.zeros(n)
    for f, a in ((hz('A6'), 1.0), (hz('A6') * 2.76, 0.35), (hz('E7'), 0.5)):
        bell += a * np.sin(2 * np.pi * f * t)
    bell *= np.exp(-t * 5) * np.clip(t * 900, 0, 1)
    i = int(SR * 0.09)
    out[i:i + n] += 0.6 * bell
    return speaker_master(out, 0.5)


def sfx_molla():
    """Something drops on the spring scale: boiing, the spring wobbles down and settles."""
    d = 0.5
    n = int(SR * d)
    t = np.arange(n) / SR
    f = 520 - 230 * (1 - np.exp(-t * 7)) + 70 * np.sin(2 * np.pi * 13 * t) * np.exp(-t * 5)
    x = osc('tri', f, d) * np.exp(-t * 6) * np.clip(t / 0.004, 0, 1)
    return speaker_master(x, 0.45)


SFX = {
    'trombetta': sfx_trombetta, 'carica': sfx_carica, 'treno': sfx_treno, 'coin': sfx_coin,
    'cassa': sfx_cassa, 'molla': sfx_molla,
    'nota_do': nota('C5'), 'nota_re': nota('D5'), 'nota_mi': nota('E5'), 'nota_fa': nota('F5'),
    'nota_sol': nota('G5'), 'pestone': sfx_pestone, 'ronfo': sfx_ronfo,
    'boing': sfx_boing, 'bloop': sfx_bloop, 'pennello': sfx_pennello,
    'tuono': sfx_tuono, 'ruggito': sfx_ruggito, 'incantesimo': sfx_incantesimo, 'rottura': sfx_rottura,
    'trasforma': sfx_trasforma, 'schivata': sfx_schivata, 'poof': sfx_poof,
    'pop': sfx_pop, 'blip': sfx_blip, 'ding': sfx_ding, 'boop': sfx_boop,
    'sparkle': sfx_sparkle, 'star': sfx_star, 'fanfare': sfx_fanfare,
    'whoosh': sfx_whoosh, 'yawn': sfx_yawn, 'clap': sfx_clap,
    'tone_su': tone('C6'), 'tone_giu': tone('C5'), 'tone_sx': tone('E5'), 'tone_dx': tone('G5'),
}


# ------------------------------------------------------------------ music
def render_track(events, total, voice):
    """events: (start_s, dur_s, note, vel)."""
    out = np.zeros(int(SR * total) + SR)
    for start, dur, name, vel in events:
        x = voice(hz(name), dur) * vel
        i = int(SR * start)
        out[i:i + len(x)] += x
    return out[:int(SR * total)]


def lead_voice(f, dur):
    n = int(SR * dur)
    t = t_axis(dur)
    vib = 1 + 0.004 * np.sin(2 * np.pi * 5.5 * t) * np.clip(t * 4, 0, 1)
    x = osc('pulse', f * vib, dur) * 0.5 + osc('tri', f * vib, dur) * 0.5
    return lowpass(x, 2600) * env_adsr(n, 0.01, 0.08, 0.55, min(0.08, dur / 3))


def bass_voice(f, dur):
    # an octave up with a bright pulse: its harmonics carry the bass line on
    # a speaker that cannot play the fundamental
    n = int(SR * dur)
    x = osc('pulse', 2 * f, dur) * 0.5 + osc('tri', 2 * f, dur) * 0.5
    return lowpass(x, 1400) * env_adsr(n, 0.005, 0.1, 0.7, min(0.05, dur / 3))


def box_voice(f, dur):
    n = int(SR * max(dur, 1.2))
    t = np.arange(n) / SR
    x = np.sin(2 * np.pi * f * t) + 0.35 * np.sin(2 * np.pi * 2 * f * t) + 0.12 * np.sin(2 * np.pi * 3 * f * t)
    return x * np.exp(-t * 3.2) * np.clip(t * 400, 0, 1)


def drums(bars, beat, beats_per_bar=4):
    rng = np.random.default_rng(11)
    total = bars * beats_per_bar * beat
    out = np.zeros(int(SR * total) + SR)
    for b in range(bars * beats_per_bar):
        t0 = b * beat
        # kick on 1 and 3: a higher, clicky "tom" that a small speaker can play
        if b % 2 == 0:
            n = int(SR * 0.10)
            f = np.linspace(260, 110, n)
            k = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-np.linspace(0, 7, n))
            click = rng.standard_normal(n) * np.exp(-np.linspace(0, 60, n)) * 0.3
            i = int(SR * t0)
            out[i:i + n] += (k + click) * 0.55
        # soft clap on 2 and 4
        if b % 2 == 1:
            n = int(SR * 0.09)
            c = lowpass(rng.standard_normal(n), 3000) * np.exp(-np.linspace(0, 9, n))
            i = int(SR * t0)
            out[i:i + n] += c * 0.25
        # hats on off-beats
        n = int(SR * 0.03)
        h = rng.standard_normal(n)
        h = h - lowpass(h, 6000)
        i = int(SR * (t0 + beat / 2))
        out[i:i + n] += h * np.exp(-np.linspace(0, 6, n)) * 0.12
    return out[:int(SR * total)]


def music_palco():
    bpm = 112
    beat = 60 / bpm
    e = beat / 2                                   # eighth note
    melody = [
        'C5 E5 G5 E5 C5 E5 G5 A5', 'G5 - E5 - C5 D5 E5 -',
        'F5 A5 C6 A5 F5 A5 C6 D6', 'C6 - A5 - G5 - - -',
        'E5 G5 C6 G5 E5 G5 C6 D6', 'E6 - D6 C6 A5 - G5 -',
        'F5 - A5 - G5 - B5 -', 'C6 - - - - - - -',
    ]
    ev = []
    for bar, line in enumerate(melody):
        toks = line.split()
        i = 0
        while i < len(toks):
            if toks[i] == '-':
                i += 1
                continue
            j = i + 1
            while j < len(toks) and toks[j] == '-':
                j += 1
            ev.append(((bar * 8 + i) * e, (j - i) * e * 0.95, toks[i], 0.55))
            i = j
    bass_roots = ['C3', 'C3', 'F2', 'F2', 'C3', 'A2', 'F2', 'C3']
    bass_fifth = ['G3', 'G3', 'C3', 'C3', 'G3', 'E3', 'G2', 'G2']
    bev = []
    for bar in range(8):
        for q in range(4):
            name = bass_roots[bar] if q % 2 == 0 else bass_fifth[bar]
            if bar == 6 and q >= 2:
                name = 'G2' if q == 2 else 'D3'
            bev.append(((bar * 4 + q) * beat, beat * 0.8, name, 0.5))
    total = 8 * 4 * beat
    mix = render_track(ev, total, lead_voice) * 0.8 + render_track(bev, total, bass_voice) * 0.45 \
        + drums(8, beat) * 0.8
    return speaker_master(mix, 0.7)


def music_nanna():
    bpm = 72
    beat = 60 / bpm
    lines = [
        'E5 - G5', 'C6 - -', 'B5 - A5', 'G5 - -',
        'A5 - F5', 'E5 - D5', 'C5 - -', '- - -',
        'E5 - G5', 'C6 - E6', 'D6 - C6', 'B5 - -',
        'A5 - C6', 'G5 - D5', 'C5 - -', '- - -',
    ]
    ev = []
    for bar, line in enumerate(lines):
        for i, tok in enumerate(line.split()):
            if tok != '-':
                ev.append(((bar * 3 + i) * beat, beat * 2, tok, 0.5))
    roots = ['C4', 'C4', 'G3', 'C4', 'F3', 'C4', 'G3', 'C4'] * 2   # kept above 170 Hz
    for bar, r in enumerate(roots):
        ev.append((bar * 3 * beat, beat * 3, r, 0.3))
    total = 16 * 3 * beat
    mix = render_track(ev, total, box_voice)
    return speaker_master(mix, 0.6)


def melody_events(lines, step, vel=0.55, per_bar=8):
    """Bars of tokens ('-' holds the previous note) -> (start, dur, note, vel)."""
    ev = []
    for bar, line in enumerate(lines):
        toks = line.split()
        i = 0
        while i < len(toks):
            if toks[i] == '-':
                i += 1
                continue
            j = i + 1
            while j < len(toks) and toks[j] == '-':
                j += 1
            ev.append(((bar * per_bar + i) * step, (j - i) * step * 0.95, toks[i], vel))
            i = j
    return ev


def flute_voice(f, dur):
    n = int(SR * dur)
    t = t_axis(dur)
    vib = 1 + 0.006 * np.sin(2 * np.pi * 5 * t) * np.clip(t * 3, 0, 1)
    x = osc('sine', f * vib, dur) * 0.75 + osc('tri', f * vib, dur) * 0.25
    return x * env_adsr(n, 0.03, 0.08, 0.7, min(0.08, dur / 3))


def pizz_voice(f, dur):
    d = 0.25
    n = int(SR * d)
    x = osc('pulse', 2 * f, d) * 0.5 + osc('tri', 2 * f, d) * 0.5
    return lowpass(x, 1600) * np.exp(-np.linspace(0, 9, n))


def woodblock(bars, step, per_bar=8):
    """Tick-tock on the eighths: a hollow high knock."""
    total = bars * per_bar * step
    out = np.zeros(int(SR * total) + SR)
    for k in range(bars * per_bar):
        d = 0.05
        n = int(SR * d)
        f = 1100 if k % 2 == 0 else 820
        x = osc('sine', f, d) * np.exp(-np.linspace(0, 10, n))
        i = int(SR * k * step)
        out[i:i + n] += x * (0.22 if k % 2 == 0 else 0.15)
    return out[:int(SR * total)]


def music_mappa():
    """Adventure on the map: a bouncy tune in G for flute, pizzicato and drums."""
    bpm = 104
    beat = 60 / bpm
    e = beat / 2
    melody = [
        'G4 - B4 - D5 - B4 -', 'C5 - E5 - D5 - - -',
        'B4 - D5 - G5 - F#5 E5', 'D5 - - - - - - -',
        'E5 - C5 - A4 - C5 -', 'D5 - B4 - G4 - B4 -',
        'A4 - C5 - B4 - A4 F#4', 'G4 - - - - - - -',
    ]
    ev = melody_events(melody, e)
    roots = ['G2', 'C3', 'G2', 'D3', 'C3', 'G2', 'D3', 'G2']
    fifths = ['D3', 'G3', 'D3', 'A3', 'G3', 'D3', 'A3', 'D3']
    bev = [((bar * 4 + q) * beat, beat * 0.6, roots[bar] if q % 2 == 0 else fifths[bar], 0.5)
           for bar in range(8) for q in range(4)]
    total = 8 * 4 * beat
    mix = render_track(ev, total, flute_voice) * 0.6 + render_track(bev, total, pizz_voice) * 0.5 \
        + drums(8, beat) * 0.55
    return speaker_master(mix, 0.42)   # the flute is dense: same loudness as "palco"


def music_sfida():
    """The duel: a creeping little villain march in A minor, tick-tock and pizzicato."""
    bpm = 126
    beat = 60 / bpm
    e = beat / 2
    melody = [
        'A4 - C5 - E5 - D#5 E5', '- - C5 - A4 - - -',
        'A4 - C5 - E5 - F5 E5', 'D5 - B4 - G#4 - - -',
        'F4 - A4 - C5 - B4 C5', '- - A4 - F4 - - -',
        'E4 - G#4 - B4 - D5 C5', 'B4 - G#4 - E4 - - -',
    ]
    ev = melody_events(melody, e, 0.5)
    roots = ['A2', 'A2', 'A2', 'E2', 'F2', 'F2', 'E2', 'E2']
    fifths = ['E3', 'E3', 'E3', 'B2', 'C3', 'C3', 'B2', 'B2']
    bev = [((bar * 4 + q) * beat, beat * 0.5, roots[bar] if q % 2 == 0 else fifths[bar], 0.55)
           for bar in range(8) for q in range(4)]
    total = 8 * 4 * beat
    mix = render_track(ev, total, lead_voice) * 0.7 + render_track(bev, total, pizz_voice) * 0.6 \
        + woodblock(8, e) + drums(8, beat) * 0.35
    return speaker_master(mix, 0.66)


def music_notte():
    """The night map of the second adventure: a calm, starry tune in A minor
    for flute and music box, pizzicato roots and no drums."""
    bpm = 84
    beat = 60 / bpm
    e = beat / 2
    melody = [
        'A4 - C5 - E5 - D5 C5', 'D5 - - - B4 - - -',
        'C5 - E5 - A5 - G5 E5', 'G5 - - - E5 - - -',
        'F5 - E5 - D5 - C5 -', 'D5 - E5 - C5 - A4 -',
        'B4 - C5 - D5 - E5 -', 'A4 - - - - - - -',
        'E5 - A5 - C6 - B5 A5', 'B5 - - - G5 - - -',
        'A5 - C6 - E6 - D6 C6', 'D6 - - - B5 - - -',
        'C6 - B5 - A5 - G5 -', 'F5 - G5 - A5 - E5 -',
        'F5 - E5 - D5 - B4 -', 'A4 - - - - - - -',
    ]
    ev = melody_events(melody, e, 0.5)
    chords = [('A3', 'C4', 'E4'), ('G3', 'B3', 'D4'), ('A3', 'C4', 'E4'), ('C4', 'E4', 'G4'),
              ('F3', 'A3', 'C4'), ('D4', 'F4', 'A4'), ('E3', 'G#3', 'B3'), ('A3', 'C4', 'E4')] * 2
    arp = []
    for bar, ch in enumerate(chords):   # the music box breaks the chord in quarters, an octave up
        for q, nt in enumerate((ch[0], ch[1], ch[2], ch[1])):
            arp.append(((bar * 4 + q) * beat, beat, nt[:-1] + str(int(nt[-1]) + 1), 0.22))
    roots = [c[0][:-1] + '2' for c in chords]   # pizzicato, played an octave up
    bev = [((bar * 4 + q) * beat, beat * 0.6, roots[bar], 0.4) for bar in range(16) for q in (0, 2)]
    total = 16 * 4 * beat
    mix = render_track(ev, total, flute_voice) * 0.55 + render_track(arp, total, box_voice) * 0.5 \
        + render_track(bev, total, pizz_voice) * 0.4
    return speaker_master(mix, 0.38)   # calm: a little under the day map


def marimba_voice(f, dur):
    """A little xylophone: wood, a bright 4th harmonic that dies at once."""
    d = max(dur, 0.45)
    n = int(SR * d)
    t = np.arange(n) / SR
    x = np.sin(2 * np.pi * f * t) + 0.35 * np.sin(2 * np.pi * 4 * f * t) * np.exp(-t * 30)
    return x * np.exp(-t * 5.5) * np.clip(t * 700, 0, 1)


def shaker(bars, step, per_bar=8):
    rng = np.random.default_rng(31)
    total = bars * per_bar * step
    out = np.zeros(int(SR * total) + SR)
    for k in range(bars * per_bar):
        n = int(SR * 0.045)
        h = rng.standard_normal(n)
        h = h - lowpass(h, 5000)
        i = int(SR * k * step)
        out[i:i + n] += h * np.exp(-np.linspace(0, 5, n)) * (0.1 if k % 2 == 0 else 0.06)
    return out[:int(SR * total)]


def music_valle():
    """The Valle della Musica (third adventure): a happy little tune in F for xylophone,
    with a music box breaking the chords, pizzicato roots, a shaker and a soft beat."""
    bpm = 100
    beat = 60 / bpm
    e = beat / 2
    melody = [
        'C5 - F5 - A5 - F5 -', 'G5 - A5 - A#5 - G5 -', 'A5 - C6 - A5 - F5 -', 'G5 - - - C5 - - -',
        'C5 - F5 - A5 - F5 -', 'G5 - A5 - A#5 - D6 -', 'C6 - A5 - G5 - E5 -', 'F5 - - - - - - -',
        'A#5 - D6 - A#5 - G5 -', 'A5 - C6 - A5 - F5 -', 'G5 - A#5 - G5 - E5 -', 'F5 - G5 - A5 - C6 -',
        'D6 - C6 - A#5 - A5 -', 'G5 - A5 - A#5 - G5 -', 'A5 - F5 - G5 - E5 -', 'F5 - - - - - - -',
    ]
    ev = melody_events(melody, e, 0.55)
    chords = [('F3', 'A3', 'C4'), ('C3', 'E3', 'G3'), ('F3', 'A3', 'C4'), ('C3', 'E3', 'G3'),
              ('F3', 'A3', 'C4'), ('G3', 'A#3', 'D4'), ('C3', 'E3', 'G3'), ('F3', 'A3', 'C4'),
              ('A#2', 'D3', 'F3'), ('F3', 'A3', 'C4'), ('C3', 'E3', 'G3'), ('F3', 'A3', 'C4'),
              ('A#2', 'D3', 'F3'), ('C3', 'E3', 'G3'), ('C3', 'E3', 'G3'), ('F3', 'A3', 'C4')]
    arp = []
    for bar, ch in enumerate(chords):   # the music box, two octaves up, in eighths: 1 3 5 3 ...
        seq = (ch[0], ch[1], ch[2], ch[1]) * 2
        for k, nt in enumerate(seq):
            arp.append(((bar * 8 + k) * e, e, nt[:-1] + str(int(nt[-1]) + 2), 0.13))
    bev = [((bar * 4 + q) * beat, beat * 0.6, (chords[bar][0] if q % 2 == 0 else chords[bar][2]), 0.45)
           for bar in range(16) for q in range(4)]
    total = 16 * 4 * beat
    mix = render_track(ev, total, marimba_voice) * 0.7 + render_track(arp, total, box_voice) * 0.5 \
        + render_track(bev, total, pizz_voice) * 0.45 + shaker(16, e) + drums(16, beat) * 0.3
    # the xylophone attacks are sharp: a soft limiter brings it to the loudness of the other maps
    x = speaker_master(mix, 1.0)
    return np.tanh(1.5 * x) / np.tanh(1.5) * 0.52


def toy_piano_voice(f, dur):
    """A toy piano: little metal rods hit by a hammer - a bright, slightly glassy tone."""
    d = max(dur, 0.6)
    n = int(SR * d)
    t = np.arange(n) / SR
    x = np.sin(2 * np.pi * f * t) + 0.5 * np.sin(2 * np.pi * 2 * f * t) * np.exp(-t * 6) \
        + 0.25 * np.sin(2 * np.pi * 5.4 * f * t) * np.exp(-t * 22)
    return x * np.exp(-t * 3.8) * np.clip(t * 900, 0, 1)


def music_giocattoli():
    """The Regno dei Giocattoli (fourth adventure): a cheerful wind-up tune in D for toy
    piano, with a glockenspiel breaking the chords, pizzicato roots, the tick-tock of a
    clockwork toy and a soft beat."""
    bpm = 116
    beat = 60 / bpm
    e = beat / 2
    melody = [
        'D5 - F#5 - A5 - F#5 -', 'G5 - B5 - A5 - - -', 'F#5 - A5 - D6 - C#6 B5', 'A5 - - - - - - -',
        'B5 - G5 - E5 - G5 -', 'A5 - F#5 - D5 - F#5 -', 'E5 - G5 - F#5 - E5 C#5', 'D5 - - - - - - -',
        'A5 - A5 B5 A5 - F#5 -', 'G5 - G5 A5 G5 - E5 -', 'F#5 - F#5 G5 F#5 - D5 -', 'E5 - F#5 - G5 - A5 -',
        'B5 - A5 - G5 - F#5 -', 'G5 - F#5 - E5 - D5 -', 'C#5 - E5 - A5 - G5 E5', 'D5 - - - - - - -',
    ]
    ev = melody_events(melody, e, 0.5)
    D, G, A, Em = ('D3', 'F#3', 'A3'), ('G3', 'B3', 'D4'), ('A3', 'C#4', 'E4'), ('E3', 'G3', 'B3')
    chords = [D, G, D, A, G, D, A, D, D, Em, D, A, G, Em, A, D]
    arp = []
    for bar, ch in enumerate(chords):   # the glockenspiel, two octaves up, in quarters: 1 3 5 3
        for q, nt in enumerate((ch[0], ch[1], ch[2], ch[1])):
            arp.append(((bar * 4 + q) * beat, beat, nt[:-1] + str(int(nt[-1]) + 2), 0.12))
    bev = [((bar * 4 + q) * beat, beat * 0.55, (chords[bar][0] if q % 2 == 0 else chords[bar][2]), 0.45)
           for bar in range(16) for q in range(4)]
    total = 16 * 4 * beat
    mix = render_track(ev, total, toy_piano_voice) * 0.75 + render_track(arp, total, box_voice) * 0.5 \
        + render_track(bev, total, pizz_voice) * 0.42 + woodblock(16, e) * 0.8 + drums(16, beat) * 0.3
    x = speaker_master(mix, 1.0)
    return np.tanh(1.5 * x) / np.tanh(1.5) * 0.52


MUSIC = {'palco': music_palco, 'nanna': music_nanna, 'mappa': music_mappa, 'sfida': music_sfida,
         'notte': music_notte, 'valle': music_valle, 'giocattoli': music_giocattoli}


def encode_ogg(x, dst):
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, 'm.wav')
        write_wav(src, x)
        subprocess.run(['ffmpeg', '-v', 'error', '-y', '-i', src, '-c:a', 'libvorbis', '-q:a', '3', dst], check=True)


def main():
    """make_audio.py            everything
       make_audio.py valle ronfo  only those (effects or music), the rest untouched"""
    import sys
    only = set(sys.argv[1:])
    unknown = only - set(SFX) - set(MUSIC)
    if unknown:
        raise SystemExit('unknown sound(s): %s' % ', '.join(sorted(unknown)))
    os.makedirs(SFX_DIR, exist_ok=True)
    os.makedirs(MUS_DIR, exist_ok=True)
    for name, fn in SFX.items():
        if only and name not in only:
            continue
        write_wav(os.path.join(SFX_DIR, name + '.wav'), fn())
        print('sfx', name)
    for name, fn in MUSIC.items():
        if only and name not in only:
            continue
        x = fn()
        encode_ogg(x, os.path.join(MUS_DIR, name + '.ogg'))
        print('musica %s %.1f s' % (name, len(x) / SR))


if __name__ == '__main__':
    main()
