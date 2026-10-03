/* Deva's Awesome Adventures - audio mixer.
 *
 * Voices and music stay compressed in memory (the OGG files, ~5 MB in all)
 * and are decoded while they play, a few hundred samples per video frame,
 * by one decoder per streaming channel with its own fixed heap: no big PCM
 * buffers, no long background decoding at startup, no malloc while playing.
 * The files are read from the card in the background, a little per frame, in
 * the order the game needs them; a clip needed earlier is read on the spot.
 * The short effects (WAV) are kept decoded.
 * SPDX-License-Identifier: MIT
 */
#include "audio.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* stb_vorbis is compiled in third_party_impl.c; this pulls in its declarations. */
#define STB_VORBIS_HEADER_ONLY
#define STB_VORBIS_NO_PUSHDATA_API
#include "stb_vorbis.c"

#define MAX_CLIPS 2048           /* ~860 today (0.10): voices, effects, music */
#define NUM_CH 8
#define CH_VOICE 0
#define CH_MUSIC 1
#define CH_SFX0 2
#define QUEUE_LEN 6
#define STREAM_BUF 4096          /* decoded samples kept ahead of a streaming channel */
#define DEC_HEAP (256 * 1024)    /* memory of one decoder (voice, music, length probe) */
#define LOAD_BUDGET (64 * 1024)  /* bytes read in the background per video frame */

enum { DIR_VOCE, DIR_SFX, DIR_MUSICA, DIR_COUNT };

typedef struct {
    char name[40];
    uint8_t kind, dir;
    bool ogg, failed;
    uint8_t *data; /* OGG: the whole compressed file */
    int size;
    int16_t *pcm;  /* WAV: decoded samples (mono) */
    int len;       /* samples (mono), -1 = not known yet */
    int rate;
} clip_t;

typedef struct {
    sound_t clip;
    uint64_t pos;  /* 32.32 fixed point sample position since the start */
    uint64_t step; /* per output sample */
    int vol;       /* 0..256 */
    bool loop;
    bool active;
    /* streaming (OGG) */
    stb_vorbis *dec;
    int channels;
    bool ended;              /* the decoder has nothing more (not looping) */
    uint32_t buf_start;      /* stream sample index of buf[0] */
    int buf_len;
    int16_t buf[STREAM_BUF];
} channel_t;

static clip_t s_clips[MAX_CLIPS];
static int s_nclips;
#define CLIP_SLOTS (2 * MAX_CLIPS)  /* 1.0: clips found by name through a hash, not a walk of ~970 names */
static int16_t s_clip_hash[CLIP_SLOTS]; /* index + 1 into s_clips, 0 = empty */

static uint32_t clip_slot(int kind, const char *name)
{
    uint32_t h = 2166136261u ^ (uint32_t)kind;
    while (*name)
        h = (h ^ (uint8_t)*name++) * 16777619u;
    return h & (CLIP_SLOTS - 1);
}
static char s_dirs[DIR_COUNT][512];
static int s_load_next;
static sound_t s_order[MAX_CLIPS]; /* background reading order */
static channel_t s_ch[NUM_CH];
static char *s_heap[2];            /* decoder heaps of the voice and music channels */
static char *s_probe_heap;         /* decoder heap to measure a clip's length */
static sound_t s_queue[QUEUE_LEN];
static int s_qlen;
static int s_vol_music = 150, s_vol_voice = 256, s_vol_sfx = 200;
static int s_duck = 256; /* current music gain (0..256) */
static bool s_muffle;     /* pause: the music plays softly */
static struct {
    bool saved;
    sound_t clip;
    uint32_t sample;
    sound_t queue[QUEUE_LEN];
    int qlen;
} s_hold; /* what the voice was saying when the game was paused */
static long s_bytes_loaded;

static sound_t lookup(int kind, const char *name);

/* ------------------------------------------------------------------ loading */

static void clip_path(const clip_t *c, char *out, size_t n)
{
    snprintf(out, n, "%.500s/%.40s.%s", s_dirs[c->dir], c->name, c->ogg ? "ogg" : "wav");
}

static bool read_wav(const char *path, int16_t **pcm, int *len, int *rate)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    unsigned char hdr[12];
    bool ok = false;
    int channels = 0, bits = 0;
    if (fread(hdr, 1, 12, f) != 12 || memcmp(hdr, "RIFF", 4) || memcmp(hdr + 8, "WAVE", 4))
        goto done;
    for (;;) {
        unsigned char ck[8];
        if (fread(ck, 1, 8, f) != 8)
            goto done;
        uint32_t sz = ck[4] | ck[5] << 8 | ck[6] << 16 | (uint32_t)ck[7] << 24;
        if (!memcmp(ck, "fmt ", 4)) {
            unsigned char fmt[16];
            if (sz < 16 || fread(fmt, 1, 16, f) != 16)
                goto done;
            channels = fmt[2] | fmt[3] << 8;
            *rate = (int)(fmt[4] | fmt[5] << 8 | fmt[6] << 16 | (uint32_t)fmt[7] << 24);
            bits = fmt[14] | fmt[15] << 8;
            if ((fmt[0] | fmt[1] << 8) != 1 || bits != 16 || channels < 1 || channels > 2)
                goto done;
            fseek(f, (long)(sz - 16 + (sz & 1)), SEEK_CUR);
        } else if (!memcmp(ck, "data", 4)) {
            if (!channels)
                goto done;
            int frames = (int)(sz / (2u * channels));
            int16_t *buf = malloc((size_t)frames * channels * 2);
            if (!buf || fread(buf, 2, (size_t)frames * channels, f) != (size_t)frames * channels) {
                free(buf);
                goto done;
            }
            if (channels == 2) {
                for (int i = 0; i < frames; i++)
                    buf[i] = (int16_t)((buf[2 * i] + buf[2 * i + 1]) / 2);
            }
            *pcm = buf;
            *len = frames;
            ok = true;
            goto done;
        } else {
            fseek(f, (long)(sz + (sz & 1)), SEEK_CUR);
        }
    }
done:
    fclose(f);
    return ok;
}

/* Read a clip into memory: the compressed bytes of an OGG, the samples of a WAV. */
static bool load_clip(clip_t *c)
{
    if (c->failed)
        return false;
    if (c->ogg ? c->data != NULL : c->pcm != NULL)
        return true;
    char path[600];
    clip_path(c, path, sizeof(path));
    if (!c->ogg) {
        if (!read_wav(path, &c->pcm, &c->len, &c->rate) || c->rate <= 0) {
            free(c->pcm);
            c->pcm = NULL;
            c->failed = true;
            log_msg(LOG_WARN, "cannot decode %s\n", path);
            return false;
        }
        s_bytes_loaded += (long)c->len * 2;
        return true;
    }
    FILE *f = fopen(path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long n = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (n > 0 && n < (64L << 20)) {
            c->data = malloc((size_t)n);
            if (c->data && fread(c->data, 1, (size_t)n, f) == (size_t)n) {
                c->size = (int)n;
                s_bytes_loaded += n;
            } else {
                free(c->data);
                c->data = NULL;
            }
        }
        fclose(f);
    }
    if (!c->data) {
        c->failed = true;
        log_msg(LOG_WARN, "cannot read %s\n", path);
    }
    return c->data != NULL;
}

static void scan_dir(const char *data_dir, const char *sub, int kind, int dir_idx)
{
    snprintf(s_dirs[dir_idx], sizeof(s_dirs[0]), "%.480s/%s", data_dir, sub);
    DIR *d = opendir(s_dirs[dir_idx]);
    if (!d) {
        log_msg(LOG_WARN, "no audio folder %s\n", s_dirs[dir_idx]);
        return;
    }
    struct dirent *e;
    while ((e = readdir(d))) {
        if (s_nclips >= MAX_CLIPS) {
            log_msg(LOG_WARN, "too many sound files in %s: raise MAX_CLIPS\n", s_dirs[dir_idx]);
            break;
        }
        size_t n = strlen(e->d_name);
        if (n < 5 || n >= sizeof(s_clips[0].name) + 4)
            continue;
        const char *ext = e->d_name + n - 4;
        if (strcmp(ext, ".ogg") && strcmp(ext, ".wav"))
            continue;
        clip_t *c = &s_clips[s_nclips];
        memset(c, 0, sizeof(*c));
        snprintf(c->name, sizeof(c->name), "%.*s", (int)(n - 4), e->d_name);
        c->kind = (uint8_t)kind;
        c->dir = (uint8_t)dir_idx;
        c->ogg = !strcmp(ext, ".ogg");
        c->len = -1;
        /* an .ogg and a .wav with the same name: keep the first one found */
        if (lookup(kind, c->name) >= 0)
            continue;
        uint32_t slot = clip_slot(kind, c->name);
        while (s_clip_hash[slot])
            slot = (slot + 1) & (CLIP_SLOTS - 1);
        s_clip_hash[slot] = (int16_t)(s_nclips + 1);
        s_nclips++;
    }
    closedir(d);
}

bool audio_init(const char *data_dir)
{
    audio_free();
    for (int i = 0; i < 2; i++)
        if (!s_heap[i])
            s_heap[i] = malloc(DEC_HEAP);
    if (!s_probe_heap)
        s_probe_heap = malloc(DEC_HEAP);
    scan_dir(data_dir, "voce", SND_VOICE, DIR_VOCE);
    scan_dir(data_dir, "sfx", SND_SFX, DIR_SFX);
    scan_dir(data_dir, "musica", SND_MUSIC, DIR_MUSICA);
    for (int i = 0; i < NUM_CH; i++)
        s_ch[i].clip = -1;
    /* read first what the title, the menus and the first answers need */
    static const char *FIRST[] = {"saluto_nome", "saluto", "saluto_continua_nome", "saluto_continua",
                                  "saluto_gioca_nome", "saluto_gioca", "premi_a", "menu_scegli", "g_conta", "g_parole",
                                  "g_sequenze", "g_balla", "conta_intro", "par_intro", "seq_intro",
                                  "balla_intro", "istruzioni", "aiuto_b", "par_ascolta", "par_cosa",
                                  "balla_guarda", "balla_tocca", "ok_1", "ok_2", "ok_3", "ok_4", "ok_5",
                                  "ok_6", "ok_nome", "ko_1", "ko_2",
                                  /* the tale starts right after the title */
                                  "st_prologo_1", "st_prologo_2", "st_prologo_3", "st_prologo_4",
                                  "st_prologo_5", "st_prologo_6", "st_prologo_7", "st_cap1", "mappa_gioca",
                                  "mappa_sfida"};
    bool queued[MAX_CLIPS] = {false};
    int n = 0;
    for (int i = 0; i < s_nclips; i++) /* effects and music first: small, and needed at once */
        if (s_clips[i].kind != SND_VOICE)
            s_order[n++] = i, queued[i] = true;
    for (int k = 0; k < ARRAY_LEN(FIRST); k++) {
        sound_t v = lookup(SND_VOICE, FIRST[k]);
        if (v >= 0 && !queued[v])
            s_order[n++] = v, queued[v] = true;
    }
    for (int i = 0; i < s_nclips; i++)
        if (!queued[i])
            s_order[n++] = i, queued[i] = true;
    log_msg(LOG_INFO, "audio: %d clips registered\n", s_nclips);
    return s_nclips > 0;
}

static void stream_close(channel_t *ch)
{
    if (ch->dec)
        stb_vorbis_close(ch->dec);
    ch->dec = NULL;
}

void audio_free(void)
{
    for (int i = 0; i < NUM_CH; i++)
        stream_close(&s_ch[i]);
    for (int i = 0; i < s_nclips; i++) {
        free(s_clips[i].data);
        free(s_clips[i].pcm);
    }
    memset(s_ch, 0, sizeof(s_ch));
    memset(s_clip_hash, 0, sizeof(s_clip_hash));
    s_nclips = s_load_next = s_qlen = 0;
    s_bytes_loaded = 0;
}

/* (every kind and name is there once: scan_dir keeps the first file found) */
static sound_t lookup(int kind, const char *name)
{
    for (uint32_t slot = clip_slot(kind, name); s_clip_hash[slot]; slot = (slot + 1) & (CLIP_SLOTS - 1)) {
        int i = s_clip_hash[slot] - 1;
        if (s_clips[i].kind == kind && !strcmp(s_clips[i].name, name))
            return i;
    }
    return -1;
}

sound_t snd_find(int kind, const char *name)
{
    sound_t s = lookup(kind, name);
    if (s < 0 && s_nclips > 0) {
        /* a missing clip is silent: say so once, the tests count these warnings */
        static char warned[32][40];
        static int nwarned;
        for (int i = 0; i < nwarned; i++)
            if (!strcmp(warned[i], name))
                return -1;
        if (nwarned < 32)
            snprintf(warned[nwarned++], sizeof(warned[0]), "%s", name);
        log_msg(LOG_WARN, "missing sound %s\n", name);
    }
    return s;
}

bool snd_exists(int kind, const char *name) { return lookup(kind, name) >= 0; }

static stb_vorbis *open_decoder(clip_t *c, char *heap)
{
    if (!load_clip(c) || !c->ogg)
        return NULL;
    stb_vorbis_alloc a = {heap, DEC_HEAP};
    int err = 0;
    stb_vorbis *v = stb_vorbis_open_memory(c->data, c->size, &err, heap ? &a : NULL);
    if (!v && heap) /* a file needing more memory than the heap: fall back to malloc */
        v = stb_vorbis_open_memory(c->data, c->size, &err, NULL);
    if (!v) {
        c->failed = true;
        log_msg(LOG_WARN, "cannot open %s (%d)\n", c->name, err);
        return NULL;
    }
    c->rate = (int)stb_vorbis_get_info(v).sample_rate;
    return v;
}

int snd_length_frames(sound_t s)
{
    if (s < 0 || s >= s_nclips)
        return 0;
    clip_t *c = &s_clips[s];
    if (c->len < 0 && c->ogg) {
        stb_vorbis *v = open_decoder(c, s_probe_heap);
        if (!v)
            return 0;
        c->len = (int)stb_vorbis_stream_length_in_samples(v);
        stb_vorbis_close(v);
    } else if (!c->ogg && !load_clip(c)) {
        return 0;
    }
    if (c->len <= 0 || c->rate <= 0)
        return 0;
    return (int)((int64_t)c->len * FPS / c->rate);
}

void audio_prefetch_step(void)
{
    /* background reading from the card, a little per video frame, so that no
       clip has to be read the moment it is needed */
    long budget = LOAD_BUDGET;
    while (s_load_next < s_nclips && budget > 0) {
        clip_t *c = &s_clips[s_order[s_load_next++]];
        long before = s_bytes_loaded;
        load_clip(c);
        budget -= (s_bytes_loaded - before) + 4096; /* the open counts too */
    }
}

long audio_memory_bytes(void) { return s_bytes_loaded; }

/* ------------------------------------------------------------------ playback */

static void start(int ch, sound_t s, int vol, bool loop)
{
    channel_t *c = &s_ch[ch];
    stream_close(c);
    c->active = false;
    if (s < 0 || s >= s_nclips)
        return;
    clip_t *cl = &s_clips[s];
    if (cl->ogg) {
        /* stream it: only the voice and music channels have a decoder heap */
        c->dec = open_decoder(cl, ch < 2 ? s_heap[ch] : NULL);
        if (!c->dec)
            return;
        c->channels = stb_vorbis_get_info(c->dec).channels;
        if (c->channels < 1)
            c->channels = 1;
        c->ended = false;
        c->buf_start = 0;
        c->buf_len = 0;
    } else if (!load_clip(cl)) {
        return;
    }
    c->clip = s;
    c->pos = 0;
    c->step = ((uint64_t)cl->rate << 32) / AUDIO_RATE;
    c->vol = vol;
    c->loop = loop;
    c->active = true;
    if (ch == CH_VOICE) /* what is being said, for the test harness and the story checks */
        log_msg(LOG_DEBUG, "BOT voice %s\n", cl->name);
}

/* Keep the decoded samples of a streaming channel ahead of its position. */
static void stream_fill(channel_t *c, uint32_t until)
{
    uint32_t idx = (uint32_t)(c->pos >> 32);
    if (idx > c->buf_start) { /* forget what has been played */
        uint32_t drop = idx - c->buf_start;
        if (drop > (uint32_t)c->buf_len)
            drop = (uint32_t)c->buf_len;
        memmove(c->buf, c->buf + drop, (size_t)(c->buf_len - (int)drop) * sizeof(int16_t));
        c->buf_len -= (int)drop;
        c->buf_start += drop;
    }
    int empty_reads = 0;
    while (!c->ended && c->buf_start + (uint32_t)c->buf_len < until && c->buf_len < STREAM_BUF) {
        static short tmp[1024 * 2];
        int want = STREAM_BUF - c->buf_len;
        if (want > 1024)
            want = 1024;
        int n = stb_vorbis_get_samples_short_interleaved(c->dec, c->channels, tmp, want * c->channels);
        if (n <= 0) {
            if (c->loop && empty_reads++ == 0 && stb_vorbis_seek_start(c->dec))
                continue; /* music: round again */
            c->ended = true;
            break;
        }
        empty_reads = 0;
        for (int i = 0; i < n; i++) {
            int acc = 0;
            for (int k = 0; k < c->channels; k++)
                acc += tmp[i * c->channels + k];
            c->buf[c->buf_len++] = (int16_t)(acc / c->channels);
        }
    }
}

void voice_say(sound_t s)
{
    s_qlen = 0;
    start(CH_VOICE, s, s_vol_voice, false);
}

void voice_queue(sound_t s)
{
    if (s < 0 || s >= s_nclips || !load_clip(&s_clips[s]))
        return; /* read now: the mixer should not wait for the card */
    if (!s_ch[CH_VOICE].active && s_qlen == 0) {
        start(CH_VOICE, s, s_vol_voice, false);
        return;
    }
    if (s_qlen < QUEUE_LEN)
        s_queue[s_qlen++] = s;
}

void voice_stop(void)
{
    s_qlen = 0;
    s_ch[CH_VOICE].active = false;
    stream_close(&s_ch[CH_VOICE]);
}

bool voice_busy(void) { return s_ch[CH_VOICE].active || s_qlen > 0; }

void voice_hold(void)
{
    channel_t *c = &s_ch[CH_VOICE];
    if (!s_hold.saved) {
        s_hold.saved = true;
        s_hold.clip = c->active ? c->clip : -1;
        s_hold.sample = c->active ? (uint32_t)(c->pos >> 32) : 0;
        memcpy(s_hold.queue, s_queue, sizeof(s_queue));
        s_hold.qlen = s_qlen;
    }
    voice_stop();
    s_muffle = true;
}

void voice_resume(bool restore)
{
    voice_stop();
    s_muffle = false;
    if (!s_hold.saved)
        return;
    s_hold.saved = false;
    if (!restore)
        return;
    if (s_hold.clip >= 0) {
        start(CH_VOICE, s_hold.clip, s_vol_voice, false);
        channel_t *c = &s_ch[CH_VOICE];
        /* a little before the word it stopped on, so the sentence is understood */
        uint32_t back = (uint32_t)s_clips[s_hold.clip].rate / 3;
        uint32_t at = s_hold.sample > back ? s_hold.sample - back : 0;
        if (c->active && at > 0) {
            if (c->dec) {
                if (stb_vorbis_seek(c->dec, at)) {
                    c->buf_start = at;
                    c->buf_len = 0;
                    c->pos = (uint64_t)at << 32;
                }
            } else {
                c->pos = (uint64_t)at << 32;
            }
        }
    }
    memcpy(s_queue, s_hold.queue, sizeof(s_queue));
    s_qlen = s_hold.qlen;
    if (!s_ch[CH_VOICE].active && s_qlen > 0) { /* it was between two lines */
        sound_t next = s_queue[0];
        memmove(s_queue, s_queue + 1, (size_t)(--s_qlen) * sizeof(sound_t));
        start(CH_VOICE, next, s_vol_voice, false);
    }
}

sound_t voice_current(void) { return s_ch[CH_VOICE].active ? s_ch[CH_VOICE].clip : -1; }

int voice_pos_ms(void)
{
    if (!s_ch[CH_VOICE].active)
        return -1;
    const clip_t *c = &s_clips[s_ch[CH_VOICE].clip];
    return (int)((int64_t)(s_ch[CH_VOICE].pos >> 32) * 1000 / (c->rate > 0 ? c->rate : 1));
}

void sfx_play(sound_t s)
{
    /* pick a free effect channel, else the one closest to its end */
    int best = CH_SFX0;
    for (int i = CH_SFX0; i < NUM_CH; i++) {
        if (!s_ch[i].active) {
            best = i;
            break;
        }
        if (s_ch[i].pos > s_ch[best].pos)
            best = i;
    }
    start(best, s, s_vol_sfx, false);
}

void music_play(sound_t s)
{
    if (s_ch[CH_MUSIC].active && s_ch[CH_MUSIC].clip == s)
        return;
    start(CH_MUSIC, s, s_vol_music, true);
}

void music_stop(void)
{
    s_ch[CH_MUSIC].active = false;
    stream_close(&s_ch[CH_MUSIC]);
}

void audio_set_volumes(int music, int voice, int sfx)
{
    s_vol_music = clampi(music, 0, 100) * 256 / 100;
    s_vol_voice = clampi(voice, 0, 100) * 256 / 100;
    s_vol_sfx = clampi(sfx, 0, 100) * 256 / 100;
    s_ch[CH_MUSIC].vol = s_vol_music;
}

static int s_vlevel; /* the loudness of the voice in the last frames (voice_level) */

int voice_level(void) { return s_vlevel; }

void audio_render(int16_t *out, int frames)
{
    int64_t vsum = 0; /* the voice, before its volume: how loud it speaks */
    /* music ducks to ~35% while the voice speaks, with a short ramp */
    int target = voice_busy() ? 90 : (s_muffle ? 120 : 256);
    s_duck += (target > s_duck) ? 6 : (target < s_duck ? -16 : 0);
    s_duck = clampi(s_duck, 90, 256);

    for (int c = 0; c < NUM_CH; c++) /* decode what this frame will play */
        if (s_ch[c].active && s_ch[c].dec)
            stream_fill(&s_ch[c], (uint32_t)((s_ch[c].pos + s_ch[c].step * (uint64_t)frames) >> 32) + 2);

    /* 1.0: channel by channel into a sum per sample, then the sums to the output. The channels do
       not touch each other and the sums are integers: the same sound as sample by sample, without
       eight channels' tests for every sample. */
    static int32_t *mix;
    static int mix_cap;
    if (frames > mix_cap) {
        int32_t *m = realloc(mix, (size_t)frames * sizeof(*m));
        if (!m)
            return;
        mix = m;
        mix_cap = frames;
    }
    memset(mix, 0, (size_t)(frames > 0 ? frames : 0) * sizeof(*mix));
    for (int c = 0; c < NUM_CH; c++) {
        channel_t *ch = &s_ch[c];
        for (int i = 0; i < frames && ch->active; i++) {
            const clip_t *cl = &s_clips[ch->clip];
            uint32_t idx = (uint32_t)(ch->pos >> 32);
            int32_t s0, s1;
            if (ch->dec) {
                uint32_t end = ch->buf_start + (uint32_t)ch->buf_len;
                if (idx >= end) {
                    if (!ch->ended) /* should not happen: the buffer is filled ahead */
                        stream_fill(ch, idx + 2);
                    end = ch->buf_start + (uint32_t)ch->buf_len;
                }
                if (idx >= end) { /* the clip is over */
                    ch->active = false;
                    stream_close(ch);
                    if (c == CH_VOICE && s_qlen > 0) {
                        sound_t next = s_queue[0];
                        memmove(s_queue, s_queue + 1, (size_t)(--s_qlen) * sizeof(sound_t));
                        start(CH_VOICE, next, s_vol_voice, false);
                        stream_fill(ch, (uint32_t)(frames - i) + 2);
                    }
                    continue;
                }
                s0 = ch->buf[idx - ch->buf_start];
                s1 = (idx + 1 < end) ? ch->buf[idx + 1 - ch->buf_start] : 0;
            } else {
                if (idx >= (uint32_t)cl->len) {
                    if (ch->loop) {
                        ch->pos -= (uint64_t)cl->len << 32;
                        idx = (uint32_t)(ch->pos >> 32);
                    } else {
                        ch->active = false;
                        continue;
                    }
                }
                s0 = cl->pcm[idx];
                s1 = (idx + 1 < (uint32_t)cl->len) ? cl->pcm[idx + 1] : (ch->loop ? cl->pcm[0] : 0);
            }
            int32_t frac = (int32_t)((ch->pos >> 16) & 0xffff);
            int32_t v = s0 + (((s1 - s0) * frac) >> 16);
            if (c == CH_VOICE)
                vsum += v < 0 ? -v : v;
            int32_t gain = ch->vol;
            if (c == CH_MUSIC)
                gain = gain * s_duck >> 8;
            mix[i] += (v * gain) >> 8;
            ch->pos += ch->step;
        }
    }
    for (int i = 0; i < frames; i++) {
        int32_t acc = mix[i];
        /* soft clip */
        if (acc > 28000)
            acc = 28000 + (acc - 28000) / 4;
        if (acc < -28000)
            acc = -28000 + (acc + 28000) / 4;
        int16_t s = (int16_t)clampi(acc, -32768, 32767);
        out[2 * i] = s;
        out[2 * i + 1] = s;
    }
    /* speech averages ~2500 (of 32767) on the loud syllables: that is the top of the bounce */
    int level = frames > 0 ? clampi((int)(vsum / frames) * 256 / 2500, 0, 256) : 0;
    s_vlevel = level > s_vlevel ? (s_vlevel + level * 3) / 4 : (s_vlevel * 3 + level) / 4;
}
