/* Deva's Awesome Adventures - the heroine: animation state and layered rendering.
 * SPDX-License-Identifier: MIT
 */
#ifndef DEVA_HERO_H
#define DEVA_HERO_H

#include "common.h"
#include "save.h"

/* HA_WALK (0.13): walking towards us, one foot after the other (the map, the path) */
typedef enum { HA_IDLE, HA_DANCE, HA_OH, HA_WAVE, HA_SLEEP, HA_MOVE, HA_WALK } hero_anim_t;

/* body poses; HA_MOVE holds one of them (the dance moves of "Balla con me"); 0.13: the steps of the
   walk and the two halves of a clap */
typedef enum { POSE_IDLE, POSE_IDLE2, POSE_UP, POSE_STEP, POSE_JUMP, POSE_GIU, POSE_WALK_L, POSE_WALK_R,
               POSE_CLAP_OPEN, POSE_CLAP, POSE_COUNT } hero_pose_t;

/* feelings on her face ("Le emozioni di Deva"): override eyes and mouth, add brows, a tear */
typedef enum { EXPR_NONE, EXPR_FELICE, EXPR_TRISTE, EXPR_ARRABBIATA, EXPR_SPAVENTATA, EXPR_SORPRESA, EXPR_COUNT } hero_expr_t;

typedef struct {
    int x, y;            /* horizontal centre and feet line on screen */
    hero_anim_t anim;
    int t;               /* frames in the current animation */
    int dur;             /* timed animations return to idle; 0 = forever */
    int blink;           /* frames to the next blink */
    bool talking;        /* flap the mouth (set from voice_busy) */
    bool mirror;         /* draw mirrored (reflections) */
    hero_pose_t move;    /* HA_MOVE: pose to hold */
    bool move_flip;
    hero_expr_t expr;    /* EXPR_NONE = the face of the animation */
    bool no_shadow;      /* drawn off screen (to be turned): no shadow on the floor */
} hero_t;

void hero_init(hero_t *h, int x, int feet_y);
void hero_play(hero_t *h, hero_anim_t a, int dur);
void hero_move(hero_t *h, hero_pose_t pose, bool flip, int dur);
void hero_update(hero_t *h);
void hero_draw(const hero_t *h, const progress_t *look);
void hero_draw_zoom(const hero_t *h, const progress_t *look, int k); /* k times bigger, feet on (x, y) */
/* Screen position of the hand that holds the magic wand (her right hand, the
 * left one on screen when the frame is mirrored), for a drawing at zoom k. */
void hero_hand(const hero_t *h, int k, int *x, int *y);

#endif
