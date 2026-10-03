/* Deva's Awesome Adventures - audio: clip registry with lazy decoding (OGG/WAV, mono), a small
 * mixer (voice + music + effects) rendering 44.1 kHz stereo, voice queue and
 * automatic music ducking while the voice speaks.
 * SPDX-License-Identifier: MIT
 */
#ifndef DEVA_AUDIO_H
#define DEVA_AUDIO_H

#include "common.h"

typedef int sound_t; /* clip index, -1 = none */

enum { SND_VOICE, SND_SFX, SND_MUSIC };

bool audio_init(const char *data_dir);
void audio_free(void);

sound_t snd_find(int kind, const char *name); /* -1 if missing */
bool snd_exists(int kind, const char *name);
int snd_length_frames(sound_t s); /* length in video frames (decodes if needed) */

void voice_say(sound_t s);   /* interrupt and play now */
void voice_queue(sound_t s); /* play after what is speaking/queued */
void voice_stop(void);
void voice_hold(void);            /* pause: remember what the voice says (and the queue), then quiet */
void voice_resume(bool restore);  /* go on from there (restore) or forget it */
bool voice_busy(void);
sound_t voice_current(void); /* clip being spoken, -1 if none */
int voice_pos_ms(void);      /* position inside it, -1 if none */
/* 0.13: how loud the voice is right now, 0..256 (quick to rise, slower to fall): the characters
   bounce with what they say */
int voice_level(void);

void sfx_play(sound_t s);
void music_play(sound_t s); /* loops; the same track keeps playing */
void music_stop(void);

void audio_set_volumes(int music, int voice, int sfx); /* 0..100 */
void audio_render(int16_t *stereo, int frames);
void audio_prefetch_step(void); /* reads a little of what is still on the card; call once per frame */
long audio_memory_bytes(void);  /* bytes of sound held in memory */

/* convenience wrappers by name */
static inline void say(const char *id) { voice_say(snd_find(SND_VOICE, id)); }
static inline void say_then(const char *id) { voice_queue(snd_find(SND_VOICE, id)); }
static inline void sfx(const char *id) { sfx_play(snd_find(SND_SFX, id)); }

#endif
