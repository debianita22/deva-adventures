#!/usr/bin/env python3
"""Round-trip check of the voice clips: transcribe every clip with Whisper
(sherpa-onnx, offline) and compare with the text in frasi.csv.

  check_voice.py --whisper ~/models/sherpa-onnx-whisper-base
Prints a table and the clips whose transcription differs from the text.
"""
import argparse
import difflib
import os
import subprocess
import sys

import numpy as np

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
sys.path.insert(0, os.path.dirname(__file__))
from gen_voice import load_phrases, canon, skip_hum  # noqa: E402

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--whisper', default=os.path.expanduser('~/models/sherpa-onnx-whisper-base'))
    ap.add_argument('--csv', default=os.path.join(ROOT, 'data', 'voce', 'frasi.csv'))
    ap.add_argument('--dir', default=os.path.join(ROOT, 'data', 'deva_adventures', 'voce'))
    ap.add_argument('--only', default='', help='comma separated ids')
    args = ap.parse_args()
    import sherpa_onnx
    w = args.whisper
    name = os.path.basename(w).replace('sherpa-onnx-whisper-', '')
    rec = sherpa_onnx.OfflineRecognizer.from_whisper(
        encoder=os.path.join(w, name + '-encoder.int8.onnx'),
        decoder=os.path.join(w, name + '-decoder.int8.onnx'),
        tokens=os.path.join(w, name + '-tokens.txt'),
        language='it', task='transcribe', num_threads=2)
    bad = []
    total = 0
    for pid, text, *_ in load_phrases(args.csv):
        if args.only and pid not in args.only.split(','):
            continue
        path = os.path.join(args.dir, pid + '.ogg')
        if not os.path.exists(path):
            continue
        raw = subprocess.run(['ffmpeg', '-v', 'error', '-i', path, '-ac', '1', '-ar', '16000',
                              '-f', 'f32le', '-'], check=True, capture_output=True).stdout
        x = np.frombuffer(raw, dtype='<f4')
        x, words = skip_hum(x, 16000, text)  # "Uhm... riprova!": judged on "riprova!"
        x = np.concatenate([np.zeros(4000, np.float32), x, np.zeros(8000, np.float32)])
        s = rec.create_stream()
        s.accept_waveform(16000, x)
        rec.decode_stream(s)
        heard = s.result.text.strip()
        ratio = difflib.SequenceMatcher(None, canon(words), canon(heard)).ratio()
        total += 1
        flag = '' if ratio >= 0.85 else '  <-- da riascoltare'
        if flag:
            bad.append(pid)
        print('%-18s %.2f  %-48s | %s%s' % (pid, ratio, text[:48], heard, flag))
    print('\n%d clip, %d da riascoltare: %s' % (total, len(bad), ', '.join(bad) or '-'))


if __name__ == '__main__':
    main()
