#!/usr/bin/env python3
"""When a word is said in a voice clip, in video frames (60 fps): the cues of the pictures of a tale.

    cues.py <id> <word> [<word> ...]

The clip is cut into its spoken stretches at the pauses (the voice pauses at commas and full
stops), the text of data/voce/frasi.csv into the same stretches at its punctuation; a word is
placed inside its stretch by its letters. Good to a few frames: enough for a gem that pops out
"...cinque gemme magiche" or a spell that falls on "...una magia".
"""
import itertools
import os
import re
import subprocess
import sys

import numpy as np

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
FPS = 60
SR = 16000


def clip_text(pid):
    for line in open(os.path.join(ROOT, 'data', 'voce', 'frasi.csv'), encoding='utf-8'):
        f = line.rstrip('\n').split(';')
        if f[0] == pid:
            return f[1]
    sys.exit('no such id: ' + pid)


def stretches(path, n, weights=None, level=0.06):
    """The n spoken stretches of a clip: it is cut at its n - 1 longest pauses (the voice pauses
    longest at commas and full stops)."""
    raw = subprocess.run(['ffmpeg', '-v', 'error', '-i', path, '-f', 'f32le', '-ac', '1', '-ar', str(SR), '-'],
                         check=True, capture_output=True).stdout
    x = np.frombuffer(raw, dtype='<f4')
    hop = SR // 100
    rms = np.array([np.sqrt(np.mean(x[i:i + hop] ** 2)) for i in range(0, len(x) - hop, hop)])
    loud = np.nonzero(rms > rms.max() * level)[0]
    first, last = loud[0], loud[-1] + 1
    gaps = []   # (length, start, end) of the quiet runs inside the speech
    i = first
    while i < last:
        if rms[i] <= rms.max() * level:
            j = i
            while j < last and rms[j] <= rms.max() * level:
                j += 1
            gaps.append((j - i, i, j))
            i = j
        else:
            i += 1
    def cut(chosen):
        spans, start = [], first
        for _, g0, g1 in sorted(chosen, key=lambda g: g[1]):
            spans.append((start * 0.01, g0 * 0.01))
            start = g1
        spans.append((start * 0.01, last * 0.01))
        return spans
    best, best_cost = cut([]), None
    if weights is not None and n > 1:   # among the longest pauses, the ones that cut the clip like the text is cut
        cand = sorted(gaps, reverse=True)[:8]
        for chosen in itertools.combinations(cand, n - 1):
            spans = cut(chosen)
            d = np.array([b - a for a, b in spans])
            cost = float(np.sum((d / d.sum() - weights) ** 2)) - 0.002 * sum(g[0] for g in chosen)
            if best_cost is None or cost < best_cost:
                best, best_cost = spans, cost
    return best, len(x) / SR


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    pid, words = sys.argv[1], sys.argv[2:]
    text = clip_text(pid)
    path = os.path.join(ROOT, 'data', 'deva_adventures', 'voce', pid + '.ogg')
    parts = [p.strip() for p in re.split(r'[,.!?:;]+', text) if p.strip()]
    w = np.array([len(p) for p in parts], dtype=float)
    spans, total = stretches(path, len(parts), w / w.sum())
    print('%s: %.2f s, %d stretches heard, %d in the text' % (pid, total, len(spans), len(parts)))
    for (a, b), p in zip(spans, parts):
        print('  %5.2f-%5.2f s  frames %3d-%3d  %s' % (a, b, a * FPS, b * FPS, p))
    if len(spans) != len(parts):
        print('  (the pauses do not match the punctuation: the cues below are estimates over the whole clip)')
        spans, parts = [(spans[0][0], spans[-1][1])], [' '.join(parts)]
    for w in words:
        for (a, b), p in zip(spans, parts):
            k = p.lower().find(w.lower())
            if k >= 0:
                t = a + (b - a) * k / max(1, len(p))
                print('%-12s %5.2f s = frame %d' % (w, t, round(t * FPS)))
                break
        else:
            print('%-12s not found' % w)


if __name__ == '__main__':
    main()
