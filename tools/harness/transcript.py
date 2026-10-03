#!/usr/bin/env python3
"""Readable script of a harness run: what the child sees and hears, in order.

    python3 tools/harness/transcript.py <out>/log.txt [--all]

Prints the scene changes, the steps of the tales, the map and duel events and
every voice line with its text from data/voce/frasi.csv. By default only the
story (tales, map, duels, the wand) is shown; --all prints the games too.
"""
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
FPS = 60


def load_texts():
    texts = {}
    for line in open(os.path.join(ROOT, 'data', 'voce', 'frasi.csv'), encoding='utf-8'):
        line = line.rstrip('\n')
        if not line or line.startswith('#') or ';' not in line:
            continue
        f = line.split(';')
        who = f[2] if len(f) > 2 and f[2] else ''
        texts[f[0]] = (f[1], who)
    return texts


STORY_SCENES = {'racconto', 'mappa', 'sfida'}


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    show_all = '--all' in sys.argv
    texts = load_texts()
    scene = ''
    for line in open(sys.argv[1], encoding='utf-8', errors='replace'):
        m = re.match(r'^(\d+) \[deva\] BOT (.*)$', line.rstrip('\n'))
        if not m:
            continue
        frame, ev = int(m.group(1)), m.group(2)
        t = '%3d:%02d' % (frame // FPS // 60, frame // FPS % 60)
        if ev.startswith('scene='):
            scene = ev[6:]
            if show_all or scene in STORY_SCENES or scene in ('menu', 'title'):
                print('%s  ----- %s' % (t, scene))
            continue
        story_event = ev.startswith(('racconto', 'mappa ', 'duel ', 'story ', 'menu sfida=1'))
        if ev.startswith('voice '):
            vid = ev[6:]
            if not (show_all or scene in STORY_SCENES or vid.startswith(('bacchetta', 'g_sfida', 'mappa'))):
                continue
            text, who = texts.get(vid, ('?', ''))
            print('%s      %-22s %s%s' % (t, vid, '[%s] ' % who if who else '', text))
        elif story_event or (show_all and ev.startswith(('question', 'answer'))):
            print('%s    # %s' % (t, ev))


if __name__ == '__main__':
    main()
