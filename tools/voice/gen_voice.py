#!/usr/bin/env python3
"""Generate the voice clips of Deva's Awesome Adventures from data/voce/frasi.csv.

Offline TTS: Piper voice "it_IT-paola-medium" run through sherpa-onnx.
Each clip is trimmed, loudness-matched and encoded as OGG Vorbis,
mono 22050 Hz, into data/deva_adventures/voce/<id>.ogg.

  pip install sherpa-onnx         (plus ffmpeg with libvorbis)
  model: https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/
         vits-piper-it_IT-paola-medium.tar.bz2

Examples:
  gen_voice.py --model ~/models/vits-piper-it_IT-paola-medium
  gen_voice.py --nome Sofia                 # adds the personalised clips
  gen_voice.py --only q_stelle,n07          # regenerate some clips
  gen_voice.py --tono 2                     # +2 semitones (brighter voice)
  gen_voice.py --asr ~/models/sherpa-onnx-whisper-small
        # best-of-N: synthesise several takes per phrase and keep the one an
        # offline Whisper model understands best (helps short words)

Recorded clips win: put your own <id>.ogg/.wav in data/voce/registrate/ and
it is copied instead of synthesising that phrase.

A third field in frasi.csv picks a character voice for the tale, made from
the same TTS voice: "mago" (lower, slower), "strega" (higher, with a
witchy vibrato), "strega_buona", "mostro" (much lower: a big creature),
"mostro_buono", "tuono" (a monster with a thundery echo), "stregone",
"stregone_buono", "orco" (the ogre: very low and slow), "orco_buono".

A "..." inside a phrase ("Uhm... riprova!") is a real pause: the parts are
synthesised one by one and joined with PAUSE seconds of silence (the TTS
itself would run over it). A part that is only a thinking sound ("Uhm",
"Ehm"; "Mmm" and "Hmm" are read as "Uhm") becomes a hum: the TTS cannot
say "mmm" (espeak spells it "emme emme emme"), so it says "uhm" and its
final nasal is stretched, pitch kept, to a real "uhmmm".

Syllabified phrases ("Pal... la.") are built one syllable at a time and joined
with fixed pauses, so the syllable onsets are exact; they are written to
voce/sillabe.txt ("<id> <n> <onset ms> ...") for the game's syllable blocks.
"""
SYLLABLE_GAP = 0.30        # seconds of silence between syllables
PAUSE = 0.28               # seconds of silence at a "..." inside a phrase
HUM_NASAL = 0.45           # seconds of "mmm" at the end of a thinking sound
import argparse
import csv
import os
import re
import shutil
import subprocess
import sys
import tempfile
import wave

import numpy as np

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SR = 22050


NUM_WORDS = {1: 'uno', 2: 'due', 3: 'tre', 4: 'quattro', 5: 'cinque', 6: 'sei', 7: 'sette',
             8: 'otto', 9: 'nove', 10: 'dieci', 11: 'undici', 12: 'dodici', 13: 'tredici',
             14: 'quattordici', 15: 'quindici', 16: 'sedici', 17: 'diciassette',
             18: 'diciotto', 19: 'diciannove', 20: 'venti'}


def canon(s):
    """Lower-case, no accents/punctuation, digits spelled out."""
    import re
    import unicodedata
    s = unicodedata.normalize('NFKD', s.lower())
    s = ''.join(c for c in s if not unicodedata.combining(c))
    s = re.sub(r'\b(\d+)\b', lambda m: NUM_WORDS.get(int(m.group(1)), m.group(1)), s)
    s = re.sub(r'[^a-z ]+', ' ', s)
    return ' '.join(s.split())


class Judge:
    """Scores a take by how well offline Whisper transcribes it (0..1)."""

    def __init__(self, model_dir, threads=2):
        import sherpa_onnx
        n = os.path.basename(model_dir.rstrip('/')).replace('sherpa-onnx-whisper-', '')
        self.rec = sherpa_onnx.OfflineRecognizer.from_whisper(
            encoder=os.path.join(model_dir, n + '-encoder.int8.onnx'),
            decoder=os.path.join(model_dir, n + '-decoder.int8.onnx'),
            tokens=os.path.join(model_dir, n + '-tokens.txt'),
            language='it', task='transcribe', num_threads=threads)

    def score(self, x, sr, text):
        import difflib
        from scipy.signal import resample_poly
        from math import gcd
        g = gcd(16000, sr)
        y = resample_poly(np.asarray(x, np.float32), 16000 // g, sr // g).astype(np.float32)
        y = np.concatenate([np.zeros(4000, np.float32), y, np.zeros(8000, np.float32)])
        s = self.rec.create_stream()
        s.accept_waveform(16000, y)
        self.rec.decode_stream(s)
        heard = s.result.text.strip()
        return difflib.SequenceMatcher(None, canon(text), canon(heard)).ratio(), heard


def heard_version(x, sr, semi, extra, tmp):
    """A take as the game will play it: encoded like the final file (character
    voice included), then decoded at 16 kHz, exactly as check_voice.py hears it."""
    wav, ogg = os.path.join(tmp, '_take.wav'), os.path.join(tmp, '_take.ogg')
    write_wav(wav, x, sr)
    encode(wav, ogg, semi, extra)
    raw = subprocess.run(['ffmpeg', '-v', 'error', '-i', ogg, '-ac', '1', '-ar', '16000', '-f', 'f32le', '-'],
                         check=True, capture_output=True).stdout
    return np.frombuffer(raw, dtype='<f4')


def best_take(tts, judge, text, speed, tries, hear=None):
    """Synthesise up to `tries` takes (varying speed and final punctuation)
    and return the one the judge understands best. `hear(x, sr)` gives the
    take as it will be played (16 kHz); without it the raw take is judged."""
    alt = text[:-1] + '!' if text.endswith('.') else text
    plan = [(text, speed), (alt, speed), (text, speed * 1.07), (alt, speed * 0.93),
            (text, speed * 0.93), (alt, speed * 1.07)]
    best = None
    for i in range(tries):
        t, sp = plan[i % len(plan)]
        a = tts.generate(t, sid=0, speed=sp)
        x = tidy(a.samples, a.sample_rate)
        sc, heard = judge.score(hear(x, a.sample_rate), 16000, text) if hear else judge.score(x, a.sample_rate, text)
        if best is None or sc > best[0]:
            best = (sc, x, a.sample_rate, heard)
        if sc >= 0.95:
            break
    return best


def syllable_clip(tts, text, speed, sr_out=SR):
    """'Pal... la.' -> one take per syllable joined by pauses; returns
    (samples, sample_rate, onsets_ms)."""
    parts = [p.strip(' .') for p in text.split('...') if p.strip(' .')]
    chunks, onsets, pos = [], [], 0
    rate = sr_out
    for i, syl in enumerate(parts):
        a = tts.generate(syl + '.', sid=0, speed=speed * 0.95)
        rate = a.sample_rate
        x = np.asarray(a.samples, dtype=np.float32)
        x = tidy(x, rate)[:-int(rate * 0.03)]        # drop tidy's tail, we add our own gap
        if i:
            gap = np.zeros(int(rate * SYLLABLE_GAP), dtype=np.float32)
            chunks.append(gap)
            pos += len(gap)
        onsets.append(int(round(pos * 1000 / rate)) + 60)   # tidy keeps 60 ms before speech
        chunks.append(x)
        pos += len(x)
    y = np.concatenate(chunks + [np.zeros(int(rate * 0.04), dtype=np.float32)])
    return y, rate, onsets


# thinking sounds: the word the TTS can say for each written form
HUMS = {'uhm': 'Uhm', 'ehm': 'Ehm', 'mmm': 'Uhm', 'hmm': 'Uhm', 'mmh': 'Uhm'}


def parts_of(text):
    """'Uhm... riprova!' -> ['Uhm', 'riprova!']: a '...' followed by more words is a pause."""
    return [p.strip() for p in re.split(r'\.\.\.\s+(?=\S)', text) if p.strip()]


def hum_word(part):
    """The TTS word of a part that is only a thinking sound, else None."""
    return HUMS.get(re.sub(r'[^a-z]', '', part.lower()))


def stretch(x, sr, tempo):
    """Time-stretch (tempo < 1 = longer) keeping the pitch, with ffmpeg's rubberband."""
    with tempfile.TemporaryDirectory() as d:
        src = os.path.join(d, 'in.wav')
        write_wav(src, x, sr)
        raw = subprocess.run(['ffmpeg', '-v', 'error', '-i', src, '-af', 'rubberband=tempo=%.4f' % tempo,
                              '-ac', '1', '-ar', str(sr), '-f', 'f32le', '-'], check=True, capture_output=True).stdout
    return np.frombuffer(raw, dtype='<f4').copy()


def hum_clip(tts, word, speed, tries=6):
    """A thinking sound: the TTS says the short word ("uhm"), then its final nasal
    (where the level drops after the vowel) is stretched to HUM_NASAL seconds.
    Of a few takes, the one with the longest natural nasal is kept (less stretching)."""
    best = None
    for _ in range(tries):
        a = tts.generate(word + '...', sid=0, speed=speed * 0.68)
        sr = a.sample_rate
        x = np.asarray(a.samples, dtype=np.float32)
        f = int(sr * 0.01)
        env = np.array([np.sqrt(np.mean(x[i:i + f] ** 2)) for i in range(0, len(x) - f, f)] or [0])
        k = int(np.argmax(env))
        top = env[k] or 1e-9
        while k < len(env) and env[k] > 0.45 * top:
            k += 1
        e = len(env) - 1
        while e > k and env[e] < 0.05 * top:
            e -= 1
        a0, a1 = max(0, (k - 1) * f), (e + 1) * f
        if best is None or a1 - a0 > best[0]:
            best = (a1 - a0, x, sr, a0, a1)
    n, x, sr, a0, a1 = best
    target = HUM_NASAL
    if n < int(sr * 0.08):  # no clear nasal: stretch the whole take
        a0, a1, target = 0, len(x), HUM_NASAL + 0.15
    head, nasal = x[:a0], x[a0:a1]
    ns = stretch(nasal, sr, max(0.2, min(1.0, (len(nasal) / sr) / target)))
    cf = int(sr * 0.012)
    if len(head) > cf and len(ns) > cf:
        r = np.linspace(0, 1, cf, dtype=np.float32)
        y = np.concatenate([head[:-cf], head[-cf:] * (1 - r) + ns[:cf] * r, ns[cf:]])
    else:
        y = np.concatenate([head, ns])
    return tidy(y, sr), sr


def skip_hum(x, sr, text):
    """What the judge should compare: a phrase that starts with a thinking sound is
    judged on the words after the pause (Whisper turns a hum into random words)."""
    parts = parts_of(text)
    if len(parts) < 2 or not hum_word(parts[0]):
        return x, text
    rest = ' '.join(parts[1:])
    f = int(sr * 0.01)
    env = np.array([np.abs(x[i:i + f]).max() for i in range(0, len(x) - f, f)] or [0])
    quiet = env < (env.max() or 1e-9) * 10 ** (-40 / 20)
    run = 0
    for i, q in enumerate(quiet):
        run = run + 1 if q else 0
        if run >= int(PAUSE * 100 * 0.6) and i * 0.01 > 0.3:
            return x[(i + 1) * f:], rest
    return x, rest


# character voices of the tale: pitch in semitones, speech speed factor, extra ffmpeg filters
PROFILES = {
    'mago': (-4.0, 0.95, ''),
    'strega': (3.0, 1.06, 'vibrato=f=6.5:d=0.22'),
    'strega_buona': (2.0, 1.0, ''),
    'mostro': (-6.0, 0.97, ''),
    'mostro_buono': (-4.0, 1.0, ''),
    'tuono': (-7.0, 0.97, 'aecho=0.8:0.4:45:0.2'),
    'stregone': (-5.0, 0.94, 'aecho=0.8:0.45:60:0.2'),
    'stregone_buono': (-3.5, 1.0, ''),
    'orco': (-7.0, 0.92, ''),          # l'Orco Brontolone: deep and slow, grumpy but never scary
    'orco_buono': (-5.0, 0.97, ''),
    're': (3.5, 1.05, ''),             # il Re Capriccio: a little boy, quick and bossy (0.11)
}


def load_phrases(path):
    rows = []
    with open(path, encoding='utf-8') as f:
        for line in f:
            line = line.rstrip('\n')
            if not line or line.startswith('#') or line.startswith('id;'):
                continue
            parts = line.split(';')
            pid, text = parts[0], parts[1]
            prof = parts[2].strip() if len(parts) > 2 else ''
            if prof and prof not in PROFILES:
                raise SystemExit('%s: unknown voice "%s"' % (pid, prof))
            rows.append((pid.strip(), text.strip(), prof))
    return rows


def make_tts(model_dir, threads):
    import sherpa_onnx
    onnx = [f for f in os.listdir(model_dir) if f.endswith('.onnx')][0]
    cfg = sherpa_onnx.OfflineTtsConfig(
        model=sherpa_onnx.OfflineTtsModelConfig(
            vits=sherpa_onnx.OfflineTtsVitsModelConfig(
                model=os.path.join(model_dir, onnx),
                tokens=os.path.join(model_dir, 'tokens.txt'),
                data_dir=os.path.join(model_dir, 'espeak-ng-data'),
                noise_scale=0.6, noise_scale_w=0.7, length_scale=1.0),
            num_threads=threads),
        max_num_sentences=1)
    return sherpa_onnx.OfflineTts(cfg)


def tidy(x, sr):
    """Trim silence (gently, so soft initial consonants survive), match
    loudness, fade the edges and add a short tail."""
    x = np.asarray(x, dtype=np.float32)
    peak0 = np.abs(x).max() or 1e-9
    thr = peak0 * 10 ** (-48 / 20)             # relative to the clip's own peak
    frame = int(sr * 0.005)
    env = np.array([np.abs(x[i:i + frame]).max() for i in range(0, len(x), frame)] or [0])
    on = np.where(env > thr)[0]
    if len(on):
        a = max(0, on[0] * frame - int(sr * 0.06))
        b = min(len(x), (on[-1] + 1) * frame + int(sr * 0.09))
        x = x[a:b]
    rms = np.sqrt(np.mean(x ** 2)) or 1e-9
    x = x * (10 ** (-19 / 20) / rms)
    peak = np.abs(x).max() or 1e-9
    if peak > 10 ** (-1 / 20):
        x = x * (10 ** (-1 / 20) / peak)
    fi, fo = int(sr * 0.003), int(sr * 0.012)
    if len(x) > fi + fo:
        x[:fi] *= np.linspace(0, 1, fi)
        x[-fo:] *= np.linspace(1, 0, fo)
    return np.concatenate([x, np.zeros(int(sr * 0.03), dtype=np.float32)])


def write_wav(path, x, sr):
    pcm = (np.clip(x, -1, 1) * 32767).astype('<i2')
    with wave.open(path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(sr)
        w.writeframes(pcm.tobytes())


def encode(src, dst, semitones=0.0, extra=''):
    af = ['aresample=%d' % SR]
    if extra:
        af.insert(0, extra)
    if semitones:
        af.insert(0, 'rubberband=pitch=%.5f' % (2 ** (semitones / 12.0)))
    cmd = ['ffmpeg', '-v', 'error', '-y', '-i', src, '-af', ','.join(af),
           '-ac', '1', '-ar', str(SR), '-c:a', 'libvorbis', '-q:a', '4', dst]
    subprocess.run(cmd, check=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--model', default=os.environ.get('DEVA_VOICE_MODEL',
                    os.path.expanduser('~/models/vits-piper-it_IT-paola-medium')))
    ap.add_argument('--csv', default=os.path.join(ROOT, 'data', 'voce', 'frasi.csv'))
    ap.add_argument('--out', default=os.path.join(ROOT, 'data', 'deva_adventures', 'voce'))
    ap.add_argument('--recorded', default=os.path.join(ROOT, 'data', 'voce', 'registrate'))
    ap.add_argument('--nome', default='', help='name of the child for the personalised clips')
    ap.add_argument('--speed', type=float, default=0.88, help='speech speed, < 1 is slower')
    ap.add_argument('--tono', type=float, default=0.0, help='pitch shift in semitones')
    ap.add_argument('--only', default='', help='comma separated ids')
    ap.add_argument('--threads', type=int, default=2)
    ap.add_argument('--asr', default='', help='Whisper model dir for best-of-N selection')
    ap.add_argument('--tentativi', type=int, default=8, help='max takes per phrase with --asr')
    args = ap.parse_args()
    judge = Judge(args.asr, args.threads) if args.asr else None

    os.makedirs(args.out, exist_ok=True)
    only = set(filter(None, args.only.split(',')))
    tts = None
    done = 0
    meta_path = os.path.join(args.out, 'sillabe.txt')
    meta = {}
    if os.path.exists(meta_path):
        for line in open(meta_path, encoding='utf-8'):
            t = line.split()
            if t and not t[0].startswith('#'):
                meta[t[0]] = ' '.join(t[1:])
    with tempfile.TemporaryDirectory() as tmp:
        for pid, text, prof in load_phrases(args.csv):
            if only and pid not in only:
                continue
            p_semi, p_speed, p_extra = PROFILES.get(prof, (0.0, 1.0, ''))
            semi, speed = args.tono + p_semi, args.speed * p_speed
            dst = os.path.join(args.out, pid + '.ogg')
            if '{nome}' in text:
                if not args.nome:
                    if os.path.exists(dst):
                        os.remove(dst)      # never keep a stale name clip
                    continue
                text = text.replace('{nome}', args.nome)
            rec = [os.path.join(args.recorded, pid + e) for e in ('.ogg', '.wav', '.mp3', '.m4a')]
            rec = [r for r in rec if os.path.exists(r)]
            wav = os.path.join(tmp, pid + '.wav')
            if rec:
                subprocess.run(['ffmpeg', '-v', 'error', '-y', '-i', rec[0], '-ac', '1', '-ar', str(SR),
                                '-f', 'f32le', os.path.join(tmp, 'raw')], check=True)
                x = np.fromfile(os.path.join(tmp, 'raw'), dtype='<f4')
                write_wav(wav, tidy(x, SR), SR)
                encode(wav, dst)
                src = 'registrata'
            elif pid.startswith('s_') and '...' in text:
                if tts is None:
                    tts = make_tts(args.model, args.threads)
                x, sr, onsets = syllable_clip(tts, text, args.speed)
                meta[pid] = '%d %s' % (len(onsets), ' '.join(map(str, onsets)))
                write_wav(wav, x, sr)
                encode(wav, dst, args.tono)
                src, heard = 'sillabe', ''
            elif len(parts_of(text)) > 1 or hum_word(text):
                if tts is None:
                    tts = make_tts(args.model, args.threads)
                chunks = []
                for part in parts_of(text):
                    h = hum_word(part)
                    if h:
                        y, sr = hum_clip(tts, h, speed)
                    elif judge:
                        _, y, sr, _ = best_take(tts, judge, part, speed, args.tentativi,
                                                lambda z, r: heard_version(z, r, semi, p_extra, tmp))
                    else:
                        a = tts.generate(part, sid=0, speed=speed)
                        y, sr = tidy(a.samples, a.sample_rate), a.sample_rate
                    if chunks:
                        chunks.append(np.zeros(int(sr * PAUSE), dtype=np.float32))
                    chunks.append(y)
                x = tidy(np.concatenate(chunks), sr)
                src, heard = 'tts %d parti' % len(parts_of(text)), ''
                if judge:
                    z, words = skip_hum(heard_version(x, sr, semi, p_extra, tmp), 16000, text)
                    sc, heard = judge.score(z, 16000, words)
                    src = 'tts %.2f %dp' % (sc, len(parts_of(text)))
                if prof:
                    src += ' ' + prof
                write_wav(wav, x, sr)
                encode(wav, dst, semi, p_extra)
            else:
                if tts is None:
                    tts = make_tts(args.model, args.threads)
                if judge and len(text) > 3 and not pid.startswith(('sy_', 'l_')):
                    sc, x, sr, heard = best_take(tts, judge, text, speed, args.tentativi,
                                                 lambda y, r: heard_version(y, r, semi, p_extra, tmp))
                    src = 'tts %.2f' % sc
                else:
                    audio = tts.generate(text, sid=0, speed=speed)
                    x, sr, heard = tidy(audio.samples, audio.sample_rate), audio.sample_rate, ''
                    src = 'tts'
                if prof:
                    src += ' ' + prof
                write_wav(wav, x, sr)
                encode(wav, dst, semi, p_extra)
            done += 1
            print('%-20s %-10s %-50s %s' % (pid, src, text, ('-> ' + heard) if judge and not rec and heard else ''))
    with open(meta_path, 'w', encoding='utf-8') as f:
        f.write('# id n onset_ms... (generated by tools/voice/gen_voice.py)\n')
        for k in sorted(meta):
            f.write('%s %s\n' % (k, meta[k]))
    print('%d clip in %s' % (done, args.out))


if __name__ == '__main__':
    sys.exit(main())
