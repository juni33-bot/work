#include "nc_traj.h"
#include "nc_class.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

/* Kalman noise (pixel units) */
#define NC_TRAJ_MEAS_STD_PX     (4.0f)    /* measurement noise std */
#define NC_TRAJ_ACCEL_STD_PX    (800.0f)  /* process noise: accel std (px/s^2) */
#define NC_TRAJ_INIT_VEL_STD    (500.0f)  /* initial velocity std (px/s) */

/* ---------------- shared state (worker writes, guarded by g_lock) ---------------- */

/* one axis of the CV filter: state [p, v], covariance 2x2.
 * [cx,cy,vx,vy] with independent x/y noise is block diagonal, so it is
 * stored as two 2-state filters (same result, fewer ops). */
typedef struct {
    float p, v;
    float P00, P01, P11;
} stKfAxis;

typedef struct {
    int         used;
    int         class_id;
    int         track_id;
    int         is_vehicle;
    float       w, h;
    uint64_t    last_meas_us;
    stKfAxis    kx, ky;
    stTrajPoint hist[NC_TRAJ_HIST_LEN];
    int         hist_head;   /* next write index */
    int         hist_cnt;
} stTrajTrack;

static stTrajTrack     g_tracks[NC_TRAJ_MAX_CH][NC_TRAJ_MAX_TRACKS];
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile uint32_t g_gen = 1;          /* bumped on reset */
static volatile float  g_max_predict_ms = (float)NC_TRAJ_MAX_PREDICT_MS_DEFAULT;

/* ---------------- render snapshot (render thread only, no lock) ---------------- */

typedef struct {
    int      class_id, track_id;
    int      predictable;
    float    cx, cy, vx, vy, w, h;
    uint64_t last_meas_us;
} stTrajSnap;

typedef struct {
    uint32_t   gen;
    uint64_t   target_us;
    int        cnt;
    stTrajSnap s[NC_TRAJ_MAX_TRACKS];
} stTrajSnapCh;

static stTrajSnapCh g_snap[NC_TRAJ_MAX_CH];

/* ---------------- Kalman ---------------- */

static void kf_init(stKfAxis *k, float z)
{
    k->p = z;
    k->v = 0.0f;
    k->P00 = NC_TRAJ_MEAS_STD_PX * NC_TRAJ_MEAS_STD_PX;
    k->P01 = 0.0f;
    k->P11 = NC_TRAJ_INIT_VEL_STD * NC_TRAJ_INIT_VEL_STD;
}

/* predict by dt (s) with white-noise acceleration model */
static void kf_predict(stKfAxis *k, float dt)
{
    const float q   = NC_TRAJ_ACCEL_STD_PX * NC_TRAJ_ACCEL_STD_PX;
    const float dt2 = dt * dt;

    k->p += k->v * dt;
    /* P = F P F' + Q */
    float P00 = k->P00 + 2.0f * dt * k->P01 + dt2 * k->P11;
    float P01 = k->P01 + dt * k->P11;
    float P11 = k->P11;
    P00 += q * dt2 * dt2 * 0.25f;
    P01 += q * dt2 * dt * 0.5f;
    P11 += q * dt2;
    k->P00 = P00; k->P01 = P01; k->P11 = P11;
}

/* position measurement update */
static void kf_update(stKfAxis *k, float z)
{
    const float r = NC_TRAJ_MEAS_STD_PX * NC_TRAJ_MEAS_STD_PX;
    float s  = k->P00 + r;
    float k0 = k->P00 / s;
    float k1 = k->P01 / s;
    float y  = z - k->p;

    k->p += k0 * y;
    k->v += k1 * y;
    float P00 = (1.0f - k0) * k->P00;
    float P01 = (1.0f - k0) * k->P01;
    float P11 = k->P11 - k1 * k->P01;
    k->P00 = P00; k->P01 = P01; k->P11 = P11;
}

/* ---------------- helpers (caller holds g_lock) ---------------- */

static stTrajTrack *find_track(int ch, int class_id, int track_id)
{
    for (int i = 0; i < NC_TRAJ_MAX_TRACKS; i++) {
        stTrajTrack *t = &g_tracks[ch][i];
        if (t->used && t->class_id == class_id && t->track_id == track_id) return t;
    }
    return NULL;
}

static stTrajTrack *alloc_track(int ch)
{
    for (int i = 0; i < NC_TRAJ_MAX_TRACKS; i++) {
        if (!g_tracks[ch][i].used) return &g_tracks[ch][i];
    }
    return NULL;
}

static void hist_push(stTrajTrack *t, float cx, float cy, uint64_t ts)
{
    t->hist[t->hist_head].cx = cx;
    t->hist[t->hist_head].cy = cy;
    t->hist[t->hist_head].ts_us = ts;
    t->hist_head = (t->hist_head + 1) % NC_TRAJ_HIST_LEN;
    if (t->hist_cnt < NC_TRAJ_HIST_LEN) t->hist_cnt++;
}

static void track_start(stTrajTrack *t, int class_id, int track_id, int is_vehicle,
                        float cx, float cy, float w, float h, uint64_t ts)
{
    memset(t, 0, sizeof(*t));
    t->used = 1;
    t->class_id = class_id;
    t->track_id = track_id;
    t->is_vehicle = is_vehicle;
    kf_init(&t->kx, cx);
    kf_init(&t->ky, cy);
    t->w = w; t->h = h;
    t->last_meas_us = ts;
    hist_push(t, cx, cy, ts);
}

/* ---------------- API ---------------- */

void nc_traj_reset(void)
{
    pthread_mutex_lock(&g_lock);
    memset(g_tracks, 0, sizeof(g_tracks));
    __atomic_add_fetch(&g_gen, 1, __ATOMIC_ACQ_REL); /* invalidates render snapshots */
    pthread_mutex_unlock(&g_lock);
}

void nc_traj_set_max_predict_ms(float ms)
{
    if (ms < 0.0f) ms = 0.0f;
    g_max_predict_ms = ms;
}

float nc_traj_get_max_predict_ms(void)
{
    return g_max_predict_ms;
}

void nc_traj_update(int ch, E_NETWORK_UID net_id,
                    const stCnnPostprocessingResults *results, uint64_t frame_ts_us)
{
    if (ch < 0 || ch >= NC_TRAJ_MAX_CH || results == NULL || frame_ts_us == 0) return;

    pthread_mutex_lock(&g_lock);

    for (int c = 0; c < MAX_CNN_CLASS_CNT; c++) {
        const stClassObjs *co = &results->class_objs[c];
        if (co->obj_cnt <= 0) continue;

        int is_vehicle = nc_class_is_vehicle(net_id, c);

        for (int n = 0; n < co->obj_cnt && n < MAX_CNN_RESULT_CNT_OF_CLASS; n++) {
            const stObjInfo *o = &co->objs[n];
            if (o->track_id < 0) continue;

            float cx = o->bbox.x + o->bbox.w * 0.5f;
            float cy = o->bbox.y + o->bbox.h * 0.5f;

            stTrajTrack *t = find_track(ch, c, o->track_id);
            if (t == NULL) {
                t = alloc_track(ch);
                if (t == NULL) {
                    static int warned = 0;
                    if (!warned) { printf("[nc_traj] ch%d track table full\n", ch); warned = 1; }
                    continue;
                }
                track_start(t, c, o->track_id, is_vehicle, cx, cy, o->bbox.w, o->bbox.h, frame_ts_us);
                continue;
            }

            if (frame_ts_us <= t->last_meas_us) continue; /* duplicate / out of order */
            uint64_t dt_us = frame_ts_us - t->last_meas_us;
            if (dt_us > NC_TRAJ_STALE_US) {
                /* too long gap: velocity is meaningless, restart */
                track_start(t, c, o->track_id, is_vehicle, cx, cy, o->bbox.w, o->bbox.h, frame_ts_us);
                continue;
            }

            float dt = (float)dt_us * 1e-6f;
            kf_predict(&t->kx, dt);
            kf_predict(&t->ky, dt);
            kf_update(&t->kx, cx);
            kf_update(&t->ky, cy);
            t->w = o->bbox.w;
            t->h = o->bbox.h;
            t->last_meas_us = frame_ts_us;
            hist_push(t, cx, cy, frame_ts_us);
        }
    }

    /* drop tracks that disappeared */
    for (int i = 0; i < NC_TRAJ_MAX_TRACKS; i++) {
        stTrajTrack *t = &g_tracks[ch][i];
        if (t->used && frame_ts_us > t->last_meas_us
            && frame_ts_us - t->last_meas_us > NC_TRAJ_STALE_US) {
            memset(t, 0, sizeof(*t));
        }
    }

    pthread_mutex_unlock(&g_lock);
}

void nc_traj_predict_frame(int ch, uint64_t target_time_us)
{
    if (ch < 0 || ch >= NC_TRAJ_MAX_CH) return;
    stTrajSnapCh *sn = &g_snap[ch];
    int cnt = 0;

    /* one short lock per frame: copy only what prediction needs */
    pthread_mutex_lock(&g_lock);
    for (int i = 0; i < NC_TRAJ_MAX_TRACKS; i++) {
        const stTrajTrack *t = &g_tracks[ch][i];
        if (!t->used) continue;
        stTrajSnap *s = &sn->s[cnt++];
        s->class_id = t->class_id;
        s->track_id = t->track_id;
        s->predictable = t->is_vehicle && (t->hist_cnt >= NC_TRAJ_MIN_HIST);
        s->cx = t->kx.p; s->vx = t->kx.v;
        s->cy = t->ky.p; s->vy = t->ky.v;
        s->w = t->w; s->h = t->h;
        s->last_meas_us = t->last_meas_us;
    }
    sn->gen = __atomic_load_n(&g_gen, __ATOMIC_ACQUIRE);
    pthread_mutex_unlock(&g_lock);

    sn->cnt = cnt;
    sn->target_us = target_time_us;
}

/* render snapshot lookup: predictable track or NULL */
static const stTrajSnap *snap_find(int ch, int class_id, int track_id)
{
    if (ch < 0 || ch >= NC_TRAJ_MAX_CH || track_id < 0) return NULL;
    const stTrajSnapCh *sn = &g_snap[ch];
    if (sn->gen != __atomic_load_n(&g_gen, __ATOMIC_ACQUIRE)) return NULL; /* reset since snapshot */

    for (int i = 0; i < sn->cnt; i++) {
        const stTrajSnap *s = &sn->s[i];
        if (s->class_id != class_id || s->track_id != track_id) continue;
        return s->predictable ? s : NULL;
    }
    return NULL;
}

/* dt (s) from last measurement to the displayed frame, capped */
static float snap_now_dt(const stTrajSnap *s, uint64_t target_time_us)
{
    float dt = 0.0f;
    if (target_time_us > s->last_meas_us) {
        dt = (float)(target_time_us - s->last_meas_us) * 1e-6f;
    }
    float max_dt = g_max_predict_ms * 1e-3f;
    if (dt > max_dt) dt = max_dt;
    return dt;
}

int nc_traj_get_predicted_box(int ch, int class_id, int track_id,
                              uint64_t target_time_us, stBBox *out)
{
    if (out == NULL) return 0;
    const stTrajSnap *s = snap_find(ch, class_id, track_id);
    if (s != NULL) {
        float dt = snap_now_dt(s, target_time_us);

        float cx = s->cx + s->vx * dt;
        float cy = s->cy + s->vy * dt;
        out->x = cx - s->w * 0.5f;
        out->y = cy - s->h * 0.5f;
        out->w = s->w;
        out->h = s->h;
        return 1;
    }
    return 0;
}

int nc_traj_get_future_center(int ch, int class_id, int track_id,
                              uint64_t target_time_us, float horizon_ms,
                              float *cx, float *cy)
{
    if (cx == NULL || cy == NULL) return 0;
    const stTrajSnap *s = snap_find(ch, class_id, track_id);
    if (s == NULL) return 0;

    if (horizon_ms < 0.0f) horizon_ms = 0.0f;
    if (horizon_ms > (float)NC_TRAJ_FUTURE_MS_MAX) horizon_ms = (float)NC_TRAJ_FUTURE_MS_MAX;
    float dt = snap_now_dt(s, target_time_us) + horizon_ms * 1e-3f;

    *cx = s->cx + s->vx * dt;
    *cy = s->cy + s->vy * dt;
    return 1;
}

int nc_traj_get_history(int ch, int class_id, int track_id,
                        stTrajPoint *out, int max_cnt)
{
    if (ch < 0 || ch >= NC_TRAJ_MAX_CH || out == NULL || max_cnt <= 0) return 0;
    int n = 0;

    pthread_mutex_lock(&g_lock);
    stTrajTrack *t = find_track(ch, class_id, track_id);
    if (t != NULL) {
        n = t->hist_cnt < max_cnt ? t->hist_cnt : max_cnt;
        int start = (t->hist_head - t->hist_cnt + NC_TRAJ_HIST_LEN) % NC_TRAJ_HIST_LEN;
        int skip = t->hist_cnt - n; /* keep newest n */
        for (int i = 0; i < n; i++) {
            out[i] = t->hist[(start + skip + i) % NC_TRAJ_HIST_LEN];
        }
    }
    pthread_mutex_unlock(&g_lock);
    return n;
}
