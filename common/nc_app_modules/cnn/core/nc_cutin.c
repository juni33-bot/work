#include "nc_cutin.h"

#include <stdio.h>
#include <string.h>

/* ---------------- ego lane source ---------------- */

#ifdef USE_LDW
#include "nc_ldw.h"

/* Junwoo's function returns 0 = ok, -1 = fail; convert to 1 = ok, 0 = fail.
 * His code is used as is (no change on his side). */
int nc_ldw_get_ego_lane_x(int ch, float y, float *x_left, float *x_right, uint64_t *lane_ts_us)
{
    if (ch < 0) return 0;
    return nc_ldw_get_ego_lane((uint32_t)ch, y, x_left, x_right, lane_ts_us) == 0 ? 1 : 0;
}

#else /* !USE_LDW */

/* Fixed straight-road trapezoid in 1920x1080 screen px:
 * lane edges meet at the vanishing point, half width STUB_HALF_W at the bottom.
 * Only for testing the cut-in path; the real nc_ldw_get_ego_lane_x overrides it. */
#define STUB_SCREEN_W   (1920.0f)
#define STUB_SCREEN_H   (1080.0f)
#define STUB_VP_X       (STUB_SCREEN_W * 0.5f)
#define STUB_VP_Y       (STUB_SCREEN_H * 0.42f)
#define STUB_HALF_W     (STUB_SCREEN_W * 0.30f)

int nc_ldw_get_ego_lane_x(int ch, float y, float *x_left, float *x_right, uint64_t *lane_ts_us)
{
    (void)ch;
    if (x_left == NULL || x_right == NULL) return 0;
    if (y <= STUB_VP_Y || y > STUB_SCREEN_H) return 0;

    float r = (y - STUB_VP_Y) / (STUB_SCREEN_H - STUB_VP_Y);  /* 0 at VP, 1 at bottom */
    *x_left  = STUB_VP_X - STUB_HALF_W * r;
    *x_right = STUB_VP_X + STUB_HALF_W * r;
    if (lane_ts_us) *lane_ts_us = 0; /* unknown -> treated as fresh */
    return 1;
}
#endif /* USE_LDW */

/* ---------------- per-track hysteresis state (render thread only) ---------------- */

typedef struct {
    int used;
    int class_id, track_id;
    int on;            /* warning ON */
    int hit_cnt;       /* consecutive candidate frames */
    int miss_cnt;      /* consecutive non-candidate frames */
    int seen;          /* evaluated in the current frame */
    int candidate;     /* candidate in the current frame */
} stCutinTrack;

typedef struct {
    stCutinTrack t[NC_CUTIN_MAX_TRACKS];
    int rose;          /* some track turned ON in this frame */
    /* ego lane-change detection */
    int   ref_valid;   /* ref_cx/ref_w hold the last valid lane at NC_CUTIN_REF_Y */
    float ref_cx, ref_w;
    int   hold;        /* remaining frames with decisions paused */
#ifdef NC_CUTIN_STATS
    /* why no warning: counts over NC_CUTIN_STATS_FRAMES frames */
    int st_frames, st_eval, st_hold, st_no_lane, st_inside_now, st_not_enter, st_cand;
#endif
} stCutinCh;

static stCutinCh g_cutin[NC_CUTIN_MAX_CH];

static stCutinTrack *get_track(int ch, int class_id, int track_id)
{
    stCutinTrack *free_slot = NULL;
    for (int i = 0; i < NC_CUTIN_MAX_TRACKS; i++) {
        stCutinTrack *t = &g_cutin[ch].t[i];
        if (t->used) {
            if (t->class_id == class_id && t->track_id == track_id) return t;
        } else if (free_slot == NULL) {
            free_slot = t;
        }
    }
    if (free_slot) {
        memset(free_slot, 0, sizeof(*free_slot));
        free_slot->used = 1;
        free_slot->class_id = class_id;
        free_slot->track_id = track_id;
    }
    return free_slot;
}

/* lane coordinate u at (x, y). 1: ok */
static int lane_u(int ch, float x, float y, uint64_t disp_ts_us, float *u)
{
    float xl, xr;
    uint64_t lts = 0;
    if (!nc_ldw_get_ego_lane_x(ch, y, &xl, &xr, &lts)) return 0;
    if (lts != 0 && disp_ts_us > lts && disp_ts_us - lts > NC_CUTIN_LANE_STALE_US) return 0;
    float w = xr - xl;
    if (w < NC_CUTIN_MIN_LANE_W_PX) return 0; /* too far / unstable ratio */
    *u = (x - xl) / w;
    return 1;
}

/* ---------------- API ---------------- */

void nc_cutin_reset(void)
{
    memset(g_cutin, 0, sizeof(g_cutin));
}

/* Ego lane change: when our car crosses a line, the lane module switches
 * lanes and x_left/x_right jump by about one lane width in one frame.
 * Compare the lane center near the bottom with the last valid one. */
static void check_ego_lane_change(int ch, uint64_t disp_ts_us)
{
    stCutinCh *c = &g_cutin[ch];
    float xl, xr;
    uint64_t lts = 0;

    if (c->hold > 0) c->hold--;

    if (!nc_ldw_get_ego_lane_x(ch, NC_CUTIN_REF_Y, &xl, &xr, &lts)) return;
    if (lts != 0 && disp_ts_us > lts && disp_ts_us - lts > NC_CUTIN_LANE_STALE_US) return;
    float w = xr - xl;
    if (w < NC_CUTIN_MIN_LANE_W_PX) return;
    float cx = (xl + xr) * 0.5f;

    if (c->ref_valid) {
        float d = cx - c->ref_cx;
        if (d < 0.0f) d = -d;
        if (d > NC_CUTIN_LC_JUMP_RATIO * c->ref_w) {
            c->hold = NC_CUTIN_LC_HOLD_FRAMES;
            for (int i = 0; i < NC_CUTIN_MAX_TRACKS; i++) c->t[i].hit_cnt = 0;
            printf("[nc_cutin] ch%d ego lane change (center %.0f -> %.0f px), hold %d frames\n",
                   ch, (double)c->ref_cx, (double)cx, NC_CUTIN_LC_HOLD_FRAMES);
        }
    }
    c->ref_valid = 1;
    c->ref_cx = cx;
    c->ref_w = w;
}

void nc_cutin_begin_frame(int ch, uint64_t disp_ts_us)
{
    if (ch < 0 || ch >= NC_CUTIN_MAX_CH) return;
    g_cutin[ch].rose = 0;
    for (int i = 0; i < NC_CUTIN_MAX_TRACKS; i++) {
        g_cutin[ch].t[i].seen = 0;
        g_cutin[ch].t[i].candidate = 0;
    }
    check_ego_lane_change(ch, disp_ts_us);
}

int nc_cutin_eval(int ch, int class_id, int track_id, const stBBox *now_box,
                  float fut_cx, float fut_cy, uint64_t disp_ts_us)
{
    if (ch < 0 || ch >= NC_CUTIN_MAX_CH || now_box == NULL || track_id < 0) return 0;
    stCutinTrack *t = get_track(ch, class_id, track_id);
    if (t == NULL) return 0; /* table full */

    /* bottom-center (where the car touches the road) now and after the horizon */
    float nx = now_box->x + now_box->w * 0.5f;
    float ny = now_box->y + now_box->h;
    float fx = fut_cx;
    float fy = fut_cy + now_box->h * 0.5f;

    float u_now, u_fut;
    int cand = 0;
#ifdef NC_CUTIN_STATS
    stCutinCh *sc = &g_cutin[ch];
    sc->st_eval++;
#endif
    if (g_cutin[ch].hold != 0) {
        /* paused right after our own lane change */
#ifdef NC_CUTIN_STATS
        sc->st_hold++;
#endif
    } else if (lane_u(ch, nx, ny, disp_ts_us, &u_now) && lane_u(ch, fx, fy, disp_ts_us, &u_fut)) {
        int outside_now = (u_now < 0.0f || u_now > 1.0f);
        int inside_fut  = (u_fut >= NC_CUTIN_LANE_MARGIN && u_fut <= 1.0f - NC_CUTIN_LANE_MARGIN);
        cand = outside_now && inside_fut;
#ifdef NC_CUTIN_STATS
        if (!outside_now)     sc->st_inside_now++;
        else if (!inside_fut) sc->st_not_enter++;
        else                  sc->st_cand++;
#endif
    } else {
#ifdef NC_CUTIN_STATS
        sc->st_no_lane++;
#endif
    }
    /* no lane at this height (far car, lane lost) -> not a candidate */

    t->seen = 1;
    t->candidate = cand;
    return t->on;  /* state from previous frames; updated in end_frame */
}

int nc_cutin_end_frame(int ch)
{
    if (ch < 0 || ch >= NC_CUTIN_MAX_CH) return 0;
    stCutinCh *c = &g_cutin[ch];

    for (int i = 0; i < NC_CUTIN_MAX_TRACKS; i++) {
        stCutinTrack *t = &c->t[i];
        if (!t->used) continue;

        if (t->candidate) {
            t->miss_cnt = 0;
            if (t->hit_cnt < NC_CUTIN_ON_FRAMES) t->hit_cnt++;
            if (!t->on && t->hit_cnt >= NC_CUTIN_ON_FRAMES) {
                t->on = 1;
                c->rose = 1;
                printf("[nc_cutin] ch%d cut-in ON  class:%d id:%d\n", ch, t->class_id, t->track_id);
            }
        } else {
            t->hit_cnt = 0;
            t->miss_cnt++;
            if (t->on && t->miss_cnt >= NC_CUTIN_OFF_FRAMES) {
                t->on = 0;
                printf("[nc_cutin] ch%d cut-in OFF class:%d id:%d\n", ch, t->class_id, t->track_id);
            }
            /* forget tracks that are gone or long quiet (unseen tracks also count misses) */
            if (!t->on && t->miss_cnt >= 4 * NC_CUTIN_OFF_FRAMES) {
                memset(t, 0, sizeof(*t));
            }
        }
    }
#ifdef NC_CUTIN_STATS
    if (++c->st_frames >= NC_CUTIN_STATS_FRAMES) {
        /* eval: predicted vehicles judged, no_lane: lane missing / narrower than MIN_LANE_W,
         * inside: already in ego lane, not_enter: outside and stays outside, cand: cut-in candidate */
        printf("[nc_cutin] ch%d stats/%dfr eval:%d hold:%d no_lane:%d inside:%d not_enter:%d cand:%d on:%d\n",
               ch, c->st_frames, c->st_eval, c->st_hold, c->st_no_lane, c->st_inside_now,
               c->st_not_enter, c->st_cand, nc_cutin_active_count(ch));
        c->st_frames = c->st_eval = c->st_hold = c->st_no_lane = 0;
        c->st_inside_now = c->st_not_enter = c->st_cand = 0;
    }
#endif
    return c->rose;
}

int nc_cutin_active_count(int ch)
{
    if (ch < 0 || ch >= NC_CUTIN_MAX_CH) return 0;
    int n = 0;
    for (int i = 0; i < NC_CUTIN_MAX_TRACKS; i++) {
        if (g_cutin[ch].t[i].used && g_cutin[ch].t[i].on) n++;
    }
    return n;
}
