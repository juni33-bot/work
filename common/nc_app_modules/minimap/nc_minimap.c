#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <math.h>

#include "nc_minimap.h"
#include "nc_ldw.h"
#include "nc_bevlog.h"

/*
 * Road coordinates (m): x = lateral from vehicle centre (+: right),
 *                       z = forward from the camera.
 * Panel coordinates   : canvas px, y grows downward (same as other overlays).
 *
 * Position pipeline is the same as the first minimap (14:34 build):
 * drawn box bottom-centre -> LDW road model with the calibrated horizon ->
 * EMA 0.35 per displayed frame. On top of it only display logic was added
 * (track joining over class / id changes, class vote, range hysteresis,
 * short hold, opaque panel, lanes from the network points).
 * Drawing uses only the existing rectangle / line / quad primitives.
 */

#define MM_MAX_OBS          (64)        // detections per frame
#define MM_MAX_TRACK        (48)        // minimap tracks per channel
#define MM_MAX_ALIAS        (4)         // (class, tracker id) keys per minimap track
#define MM_BEHIND_M         (3.0f)      // road shown behind the camera (ego body)

#define MM_PRED_LANE_MAX    (4)         // lanes each side of the ego lane for the nearest-vehicle search
#define MM_SMOOTH_ALPHA     (0.35f)     // position EMA per displayed frame (as before)
#define MM_TRACK_GAP_US     (500000ULL) // longer gap -> restart smoothing / no re-association
#define MM_HOLD_US          (300000ULL) // keep drawing a vehicle this long after its last detection
#define MM_DROP_US          (1000000ULL)// forget a track not detected this long

/* association gates (m) */
#define MM_GATE_Z_MIN       (2.0f)
#define MM_GATE_Z_RATIO     (0.15f)
#define MM_GATE_X           (1.5f)
#define MM_DUP_Z_MIN        (1.0f)      // same vehicle detected twice (two classes) in one frame
#define MM_DUP_Z_RATIO      (0.06f)
#define MM_DUP_X            (0.8f)

/* class vote (once per detection frame) */
#define MM_CLS_DECAY        (0.8f)
#define MM_CLS_SWITCH       (1.0f)      // other class must lead by this much to switch

/* lanes: the network points of each line are fitted in road coordinates with
 * x = a + b*z (+ c*z^2) and the fit is drawn over the whole panel height
 * (behind the camera to NC_MINIMAP_RANGE_M). The part outside the real points
 * is an extrapolation and is drawn fainter. */
#define MM_LANE_MAX_Z_M      (60.0f)    // ignore points farther than this (noisy)
#define MM_LANE_MIN_SPAN_M   (3.0f)     // need points over at least this z span
#define MM_LANE_QUAD_SPAN_M  (8.0f)     // quadratic term only with this much span and >= 4 points
#define MM_LANE_MAX_CURV     (0.003f)   // |c| limit (1/(2R), R ~ 170 m): no wild bends
#define MM_LANE_SEG_M        (4.0f)     // drawn segment length
#define MM_LANE_EXTRAP_ALPHA (0.55f)    // alpha factor of the extrapolated part

/* ego vehicle body relative to the camera (m) */
#define MM_EGO_W_M          (1.85f)
#define MM_EGO_FRONT_M      (2.0f)      // bumper ahead of the camera
#define MM_EGO_REAR_M       (2.5f)      // body behind the camera

enum { MM_CLS_CAR = 0, MM_CLS_TRUCK, MM_CLS_BUS, MM_CLS_MOTO, MM_CLS_N };
static const float k_cls_w[MM_CLS_N] = { 1.85f, 2.3f,  2.5f, 0.8f };
static const float k_cls_l[MM_CLS_N] = { 4.5f,  7.0f, 11.0f, 2.0f };

typedef struct {
    const char *class_name;     // pointer from the network ini (stable)
    int         track_id;
} stMmAlias;

typedef struct {
    int       used;
    uint32_t  id;
    stMmAlias alias[MM_MAX_ALIAS];
    int       alias_next;
    float     x, z;             // smoothed position
    int       n_upd;
    uint64_t  last_upd_disp_us; // displayed frame of the last position update
    uint64_t  last_det_us;      // detection frame of the last class vote
    uint64_t  last_dup_det_us;  // detection frame of the last duplicate (half) vote
    uint64_t  last_seen_disp_us;
    int       in_range;         // range hysteresis state
    float     cls_score[MM_CLS_N];
    int       cls;
    float     color[MM_CLS_N][4];
    int       has_color[MM_CLS_N];
    int       highlight;
    float     v_prev;           // raw box bottom (px) of the last update (pitch estimate)
    uint64_t  v_prev_us;        // displayed frame of v_prev
    int       fut_n;            // smoothed predicted offset available (0 = none)
    float     fut_dx, fut_dz;   // predicted position - position (m), smoothed
    int       lane_ok;          // lane is valid
    int       lane;             // lane relative to the ego lane (0 = ego, +: right)
} stMmTrack;

typedef struct {
    stNcMinimapObj in;
    int    ok;                  // road position available
    int    cls;
    float  xm, zm;              // road position of the drawn box bottom-centre (pitch compensated)
    float  u, v_raw, v_used;    // box bottom-centre (px): raw / after pitch compensation
    int    fut_ok;              // predicted road position available
    float  fxm, fzm;            // road position of the predicted bottom-centre (pitch compensated)
    stNcLdwGeom geom;           // geometry used for the road position
    float  vp_used;
    uint64_t geom_t_us;
    int    track;               // index into tracks, -1 = none
} stMmObs;

typedef struct {
    uint64_t  t_disp_us;
    int       obs_cnt;
    stMmObs   obs[MM_MAX_OBS];
    uint32_t  next_id;
    stMmTrack track[MM_MAX_TRACK];
    uint64_t  prev_disp_us;     // previous displayed frame
    float     pitch_px;         // pitch offset of the box bottoms (px, + = down)
    uint64_t  pitch_us;         // displayed frame of pitch_px
} stMmChannel;

static stMmChannel  g_mm[NC_LDW_MAX_CH];    // render thread only
static stNcLdwLanes g_mm_lanes;             // scratch for drawing (render thread only)

/* ------------------------------------------------------------------------- */
/* classes                                                                    */

static int mm_class_of(const char *name)
{
    if (name == NULL) return MM_CLS_CAR;
    if (!strcasecmp(name, "bus")) return MM_CLS_BUS;
    if (!strcasecmp(name, "tru") || !strcasecmp(name, "truck")) return MM_CLS_TRUCK;
    if (!strcasecmp(name, "mot") || !strcasecmp(name, "motorcycle") || !strcasecmp(name, "motorbike") ||
        !strcasecmp(name, "bik") || !strcasecmp(name, "bike") || !strcasecmp(name, "bicycle") ||
        !strcasecmp(name, "cycle"))
        return MM_CLS_MOTO;
    return MM_CLS_CAR;
}

/* one vote per detection frame (the same detections are reported every displayed frame) */
static void mm_vote(stMmTrack *t, const stMmObs *o, float weight)
{
    uint64_t *last = weight >= 1.0f ? &t->last_det_us : &t->last_dup_det_us;
    if (o->in.t_det_us == *last) return;
    *last = o->in.t_det_us;
    for (int i = 0; i < MM_CLS_N; i++) t->cls_score[i] *= MM_CLS_DECAY;
    t->cls_score[o->cls] += weight;
    if (o->cls != t->cls && t->cls_score[o->cls] > t->cls_score[t->cls] + MM_CLS_SWITCH) t->cls = o->cls;
}

static void mm_take_color(stMmTrack *t, const stMmObs *o)
{
    if (o->in.highlight) return;                 // cut-in colour is not the class colour
    memcpy(t->color[o->cls], o->in.color, sizeof(t->color[0]));
    t->has_color[o->cls] = 1;
}

/* ------------------------------------------------------------------------- */
/* tracks                                                                     */

static stMmTrack *mm_find_alias(stMmChannel *c, const char *cls_name, int track_id)
{
    if (track_id < 0) return NULL;
    for (int i = 0; i < MM_MAX_TRACK; i++) {
        stMmTrack *t = &c->track[i];
        if (!t->used) continue;
        for (int a = 0; a < MM_MAX_ALIAS; a++)
            if (t->alias[a].class_name == cls_name && t->alias[a].track_id == track_id) return t;
    }
    return NULL;
}

static void mm_add_alias(stMmChannel *c, stMmTrack *t, const char *cls_name, int track_id)
{
    if (track_id < 0) return;
    for (int i = 0; i < MM_MAX_TRACK; i++) {            // a key belongs to one track only
        stMmTrack *o = &c->track[i];
        if (!o->used || o == t) continue;
        for (int a = 0; a < MM_MAX_ALIAS; a++)
            if (o->alias[a].class_name == cls_name && o->alias[a].track_id == track_id)
                o->alias[a].track_id = -1;
    }
    for (int a = 0; a < MM_MAX_ALIAS; a++)
        if (t->alias[a].class_name == cls_name && t->alias[a].track_id == track_id) return;
    t->alias[t->alias_next].class_name = cls_name;
    t->alias[t->alias_next].track_id = track_id;
    t->alias_next = (t->alias_next + 1) % MM_MAX_ALIAS;
}

static stMmTrack *mm_new_track(stMmChannel *c, const stMmObs *o)
{
    stMmTrack *t = NULL;
    for (int i = 0; i < MM_MAX_TRACK && t == NULL; i++)
        if (!c->track[i].used) t = &c->track[i];
    if (t == NULL) {                                  // full: reuse the longest unseen one
        t = &c->track[0];
        for (int i = 1; i < MM_MAX_TRACK; i++)
            if (c->track[i].last_seen_disp_us < t->last_seen_disp_us) t = &c->track[i];
    }
    memset(t, 0, sizeof(*t));
    t->used = 1;
    t->id = ++c->next_id;
    for (int a = 0; a < MM_MAX_ALIAS; a++) t->alias[a].track_id = -1;
    t->cls = o->cls;
    return t;
}

/* EMA per displayed frame, as in the first minimap */
static void mm_smooth(stMmTrack *t, const stMmObs *o, uint64_t now)
{
    if (t->n_upd > 0 && t->last_upd_disp_us == now) return;          // already updated this frame
    int restart = (t->n_upd == 0 || now < t->last_upd_disp_us || now - t->last_upd_disp_us > MM_TRACK_GAP_US);

    // predicted offset: same EMA as the position; dropped when this frame has none
    if (!o->fut_ok) {
        t->fut_n = 0;
    } else {
        float dx = o->fxm - o->xm, dz = o->fzm - o->zm;
        if (restart || t->fut_n == 0) { t->fut_dx = dx; t->fut_dz = dz; t->fut_n = 1; }
        else {
            t->fut_dx += MM_SMOOTH_ALPHA * (dx - t->fut_dx);
            t->fut_dz += MM_SMOOTH_ALPHA * (dz - t->fut_dz);
            t->fut_n++;
        }
    }

    if (restart) {
        t->x = o->xm; t->z = o->zm;
        t->n_upd = 1;
    } else {
        t->x += MM_SMOOTH_ALPHA * (o->xm - t->x);
        t->z += MM_SMOOTH_ALPHA * (o->zm - t->z);
        t->n_upd++;
    }
    t->last_upd_disp_us = now;
    t->v_prev = o->v_raw;
    t->v_prev_us = now;
    mm_vote(t, o, 1.0f);
}

/* ------------------------------------------------------------------------- */
/* frame input                                                                */

void nc_minimap_begin_frame(uint32_t ch, uint64_t t_disp_us)
{
    if (ch >= NC_LDW_MAX_CH) return;
    g_mm[ch].obs_cnt = 0;
    g_mm[ch].t_disp_us = t_disp_us;
}

void nc_minimap_add_obj(uint32_t ch, const stNcMinimapObj *in)
{
    if (ch >= NC_LDW_MAX_CH || in == NULL) return;
    stMmChannel *c = &g_mm[ch];
    if (c->obs_cnt >= MM_MAX_OBS) return;

    stMmObs *o = &c->obs[c->obs_cnt++];
    memset(o, 0, sizeof(*o));
    o->in = *in;
    o->cls = mm_class_of(in->class_name);
    o->track = -1;

    // ground contact point of the drawn box with the newest (calibrated) geometry.
    // Road position is computed in mm_process() after the pitch compensation.
    o->u = in->shown.x + in->shown.w * 0.5f;
    o->v_raw = o->v_used = in->shown.y + in->shown.h;
    if (nc_ldw_app_get_geom(ch, 0, &o->geom) >= 0) {
        o->ok = 1;
        o->vp_used = o->geom.vp_y;
        o->geom_t_us = o->geom.t_us;
    }
}

/* pitch offset from the common bottom move of the known vehicles, then the road
 * position of every detection with bottom - offset */
static void mm_pitch_and_road(uint32_t ch)
{
    stMmChannel *c = &g_mm[ch];
    const uint64_t now = c->t_disp_us;

#if NC_MINIMAP_PITCH_COMP
    if (now != c->pitch_us) {
        float d[MM_MAX_OBS];
        int nd = 0;
        for (int n = 0; n < c->obs_cnt; n++) {
            const stMmObs *o = &c->obs[n];
            if (!o->ok) continue;
            const stMmTrack *t = mm_find_alias(c, o->in.class_name, o->in.track_id);
            if (t == NULL || t->v_prev_us == 0 || t->v_prev_us != c->prev_disp_us) continue;
            float dv = o->v_raw - t->v_prev;
            if (fabsf(dv) > NC_MINIMAP_PITCH_OUTLIER_PX) continue;      // re-association / jump
            int k = nd++;                                              // insertion sort
            while (k > 0 && d[k - 1] > dv) { d[k] = d[k - 1]; k--; }
            d[k] = dv;
        }
        if (c->pitch_us == 0 || now < c->pitch_us || now - c->pitch_us > MM_TRACK_GAP_US) {
            c->pitch_px = 0.0f;                                        // long gap: start again
        } else {
            float dt = (float)(now - c->pitch_us) * 1e-6f;
            c->pitch_px *= expf(-dt / NC_MINIMAP_PITCH_TAU_S);
            if (nd >= NC_MINIMAP_PITCH_MIN_VEH)
                c->pitch_px += (nd % 2) ? d[nd / 2] : 0.5f * (d[nd / 2 - 1] + d[nd / 2]);
            c->pitch_px = fmaxf(-NC_MINIMAP_PITCH_MAX_PX, fminf(NC_MINIMAP_PITCH_MAX_PX, c->pitch_px));
        }
        c->pitch_us = now;
    }
#endif

    for (int n = 0; n < c->obs_cnt; n++) {
        stMmObs *o = &c->obs[n];
        if (!o->ok) continue;
#if NC_MINIMAP_PITCH_COMP
        o->v_used = o->v_raw - c->pitch_px;
#endif
        if (nc_ldw_geom_to_road(ch, &o->geom, o->u, o->v_used, &o->xm, &o->zm) != 0 || o->zm <= 0.0f) {
            o->ok = 0;
            continue;
        }
#if NC_MINIMAP_SHOW_PRED
        // predicted point: same geometry and pitch offset as the current point
        if (o->in.fut_valid) {
            float fv = o->in.fut_v - (o->v_raw - o->v_used);
            if (nc_ldw_geom_to_road(ch, &o->geom, o->in.fut_u, fv, &o->fxm, &o->fzm) == 0 && o->fzm > 0.0f)
                o->fut_ok = 1;
        }
#endif
    }
}

/* match this frame's detections to minimap tracks and update them */
static void mm_process(uint32_t ch)
{
    stMmChannel *c = &g_mm[ch];
    const uint64_t now = c->t_disp_us;

    for (int i = 0; i < MM_MAX_TRACK; i++) {
        stMmTrack *t = &c->track[i];
        if (!t->used) continue;
        t->highlight = 0;
        if (now > t->last_seen_disp_us && now - t->last_seen_disp_us > MM_DROP_US) t->used = 0;
    }

    mm_pitch_and_road(ch);

    // 1. known (class, tracker id)
    for (int n = 0; n < c->obs_cnt; n++) {
        stMmObs *o = &c->obs[n];
        if (!o->ok) continue;
        stMmTrack *t = mm_find_alias(c, o->in.class_name, o->in.track_id);
        if (t == NULL) continue;
        o->track = (int)(t - c->track);
        mm_smooth(t, o, now);
    }

    // 2. new key: same vehicle with another class / tracker id, matched by position
    for (int n = 0; n < c->obs_cnt; n++) {
        stMmObs *o = &c->obs[n];
        if (!o->ok || o->track >= 0) continue;

        int best = -1, dup = 0;
        float best_cost = 1.0f;
        for (int i = 0; i < MM_MAX_TRACK; i++) {
            stMmTrack *t = &c->track[i];
            if (!t->used || t->n_upd == 0) continue;
            int is_dup = (t->last_upd_disp_us == now);   // already updated by another box of this frame
            if (!is_dup && (now < t->last_upd_disp_us || now - t->last_upd_disp_us > MM_TRACK_GAP_US)) continue;
            float gz = is_dup ? fmaxf(MM_DUP_Z_MIN, MM_DUP_Z_RATIO * o->zm) : fmaxf(MM_GATE_Z_MIN, MM_GATE_Z_RATIO * o->zm);
            float gx = is_dup ? MM_DUP_X : MM_GATE_X;
            float dz = o->zm - t->z, dx = o->xm - t->x;
            float cost = (dz / gz) * (dz / gz) + (dx / gx) * (dx / gx);
            if (cost < best_cost) { best_cost = cost; best = i; dup = is_dup; }
        }
        if (best < 0) continue;

        stMmTrack *t = &c->track[best];
        mm_add_alias(c, t, o->in.class_name, o->in.track_id);
        o->track = best;
        if (dup) mm_vote(t, o, 0.5f);                // two classes for one vehicle: half a vote
        else     mm_smooth(t, o, now);
    }

    // 3. new vehicles
    for (int n = 0; n < c->obs_cnt; n++) {
        stMmObs *o = &c->obs[n];
        if (!o->ok || o->track >= 0) continue;
        stMmTrack *t = mm_new_track(c, o);
        mm_add_alias(c, t, o->in.class_name, o->in.track_id);
        o->track = (int)(t - c->track);
        mm_smooth(t, o, now);
    }

    // seen / colour / cut-in
    for (int n = 0; n < c->obs_cnt; n++) {
        stMmObs *o = &c->obs[n];
        if (o->track < 0) continue;
        stMmTrack *t = &c->track[o->track];
        t->last_seen_disp_us = now;
        t->highlight |= o->in.highlight;
        mm_take_color(t, o);
    }

    // range hysteresis
    for (int i = 0; i < MM_MAX_TRACK; i++) {
        stMmTrack *t = &c->track[i];
        if (!t->used) continue;
        if (!t->in_range && t->z <= NC_MINIMAP_RANGE_M - NC_MINIMAP_RANGE_HYST_M) t->in_range = 1;
        else if (t->in_range && t->z > NC_MINIMAP_RANGE_M) t->in_range = 0;
    }
    c->prev_disp_us = now;
}

static int mm_track_drawn(const stMmChannel *c, const stMmTrack *t)
{
    return t->used && t->in_range &&
           c->t_disp_us >= t->last_seen_disp_us && c->t_disp_us - t->last_seen_disp_us <= MM_HOLD_US;
}

/* diagnostics (NC_BEV_LOG). Columns: see nc_bevlog.c */
static void mm_log(uint32_t ch)
{
    if (!nc_bevlog_enabled()) return;
    stMmChannel *c = &g_mm[ch];
    float k = 0.0f, vp = 0.0f;
    nc_ldw_app_get_calib(ch, &k, &vp, NULL);

    static const char *const cls_name[MM_CLS_N] = { "car", "truck", "bus", "moto" };
    for (int n = 0; n < c->obs_cnt; n++) {
        const stMmObs *o = &c->obs[n];
        const stMmTrack *t = o->track >= 0 ? &c->track[o->track] : NULL;
        nc_bevlog_row(NC_BEVLOG_OBJ,
                      "%llu,%llu,%llu,%u,%d,%s,%.1f,%.1f,%.1f,%.1f,%d,%.1f,%.1f,%.1f,%.1f,"
                      "%.1f,%.1f,%.2f,%.5f,%.3f,%.3f,%.3f,%.3f,%d,"
                      "%.2f,%d,%u,%s",
                      (unsigned long long)o->in.t_disp_us, (unsigned long long)o->in.t_det_us,
                      (unsigned long long)o->geom_t_us, ch, o->in.track_id,
                      o->in.class_name ? o->in.class_name : "?",
                      (double)o->in.meas.x, (double)o->in.meas.y, (double)o->in.meas.w, (double)o->in.meas.h,
                      o->in.pred_valid,
                      (double)o->in.shown.x, (double)o->in.shown.y, (double)o->in.shown.w, (double)o->in.shown.h,
                      (double)o->u, (double)o->v_used,          // v_used: after pitch compensation
                      (double)vp, (double)k,
                      (double)o->xm, (double)(o->ok ? o->zm : 0.0f),
                      (double)(t ? t->x : 0.0f), (double)(t ? t->z : 0.0f),
                      t ? mm_track_drawn(c, t) : 0,
                      (double)o->vp_used, (o->geom_t_us == o->in.t_det_us) ? 1 : 0, t ? t->id : 0u,
                      t ? cls_name[t->cls] : "-");
    }
}

/* ------------------------------------------------------------------------- */
/* drawing helpers                                                            */

static const float k_px = NC_MINIMAP_PANEL_X;
static const float k_py = NC_MINIMAP_PANEL_Y;
static const float k_pw = NC_MINIMAP_PANEL_W;
static const float k_ph = NC_MINIMAP_PANEL_H;

static float mm_sx(void) { return k_pw / (2.0f * NC_MINIMAP_LAT_RANGE_M); }
static float mm_sz(void) { return k_ph / (NC_MINIMAP_RANGE_M + MM_BEHIND_M); }

static float mm_px(float x_m) { return k_px + k_pw * 0.5f + x_m * mm_sx(); }
static float mm_py(float z_m) { return k_py + k_ph - (z_m + MM_BEHIND_M) * mm_sz(); }

/* filled quad: same shader / primitive as the first minimap background */
static void mm_fill_rect(float x, float y, float w, float h, const float color[4],
                         const struct gl_npu_program *p)
{
    float v[12];
    nc_opengl_create_rect_vertex(x, y, w, h, v);     // LT, LB, RB, RT -> fan

    glUseProgram(p->program);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glVertexAttribPointer(p->pos_handle, 3, GL_FLOAT, GL_FALSE, 0, v);
    glEnableVertexAttribArray(p->pos_handle);
    glUniform1i((GLint)p->drawing_type_handle, DRAW_NPU_TYPE_SOLID);
    glUniform4fv((GLint)p->color_handle, 1, color);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisable(GL_BLEND);
    glDisableVertexAttribArray(p->pos_handle);
    glUseProgram(0);
}

static void mm_rect(float x, float y, float w, float h, const float color[4],
                    const struct gl_npu_program *p)
{
    float c[4] = {color[0], color[1], color[2], color[3]};
    nc_opengl_draw_rectangle(x, y, w, h, c, *p);
}

/* road rectangle (x centre, z from z0 to z1) clipped to the panel */
static void mm_road_rect(float xc, float w_m, float z0, float z1, const float color[4], int fill,
                         const struct gl_npu_program *p)
{
    float left = mm_px(xc - w_m * 0.5f), right = mm_px(xc + w_m * 0.5f);
    float top  = mm_py(z1),              bot   = mm_py(z0);

    if (left < k_px) left = k_px;
    if (right > k_px + k_pw) right = k_px + k_pw;
    if (top < k_py) top = k_py;
    if (bot > k_py + k_ph) bot = k_py + k_ph;
    if (right - left < 1.0f || bot - top < 1.0f) return;

    if (fill) mm_fill_rect(left, top, right - left, bot - top, color, p);
    else      mm_rect(left, top, right - left, bot - top, color, p);
}

/* filled convex or star-shaped polygon (triangle fan), canvas px, at most MM_POLY_MAX points */
#define MM_POLY_MAX (10)
static void mm_fill_poly(const float (*xy)[2], int n, const float color[4],
                         const struct gl_npu_program *p)
{
    float v[MM_POLY_MAX * 3];
    for (int i = 0; i < n; i++) {
        nc_opengl_pixel_to_ndc(xy[i][0], xy[i][1], &v[i * 3], &v[i * 3 + 1]);
        v[i * 3 + 2] = 0.0f;
    }

    glUseProgram(p->program);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glVertexAttribPointer(p->pos_handle, 3, GL_FLOAT, GL_FALSE, 0, v);
    glEnableVertexAttribArray(p->pos_handle);
    glUniform1i((GLint)p->drawing_type_handle, DRAW_NPU_TYPE_SOLID);
    glUniform4fv((GLint)p->color_handle, 1, color);
    glDrawArrays(GL_TRIANGLE_FAN, 0, n);
    glDisable(GL_BLEND);
    glDisableVertexAttribArray(p->pos_handle);
    glUseProgram(0);
}

/* Part t0..t1 of a road segment inside the panel (Liang-Barsky). Returns 0 if none. */
static int mm_clip_seg(float x0, float z0, float x1, float z1, float *t0_out, float *t1_out)
{
    const float lat = NC_MINIMAP_LAT_RANGE_M;
    float t0 = 0.0f, t1 = 1.0f, dx = x1 - x0, dz = z1 - z0;
    const float pq[4][2] = {
        { -dx, x0 + lat }, { dx, lat - x0 },
        { -dz, z0 + MM_BEHIND_M }, { dz, NC_MINIMAP_RANGE_M - z0 },
    };
    for (int i = 0; i < 4; i++) {
        float pp = pq[i][0], q = pq[i][1];
        if (fabsf(pp) < 1e-9f) { if (q < 0.0f) return 0; continue; }
        float r = q / pp;
        if (pp < 0.0f) { if (r > t1) return 0; if (r > t0) t0 = r; }
        else           { if (r < t0) return 0; if (r < t1) t1 = r; }
    }
    *t0_out = t0;
    *t1_out = t1;
    return 1;
}

/* road segment clipped to the panel, drawn with nc_opengl_draw_line */
static void mm_road_seg(float x0, float z0, float x1, float z1, int dashed, const float color[4],
                        const struct gl_npu_program *p)
{
    float t0, t1, dx = x1 - x0, dz = z1 - z0;
    if (!mm_clip_seg(x0, z0, x1, z1, &t0, &t1)) return;
    float c[4] = {color[0], color[1], color[2], color[3]};
    nc_opengl_draw_line(mm_px(x0 + t0 * dx), mm_py(z0 + t0 * dz), mm_px(x0 + t1 * dx), mm_py(z0 + t1 * dz),
                        dashed ? 2 : 1, c, *p);
}

/* weighted least squares x = a + b z (+ c z^2). Near points weigh more (their
 * road position is more accurate). Returns 0 on success. */
static int mm_lane_fit(const float *xs, const float *zs, int m, int quad, float coef[3])
{
    double S[5] = {0}, T[3] = {0};               // sum w z^k (k 0..4), sum w x z^k (k 0..2)
    for (int j = 0; j < m; j++) {
        double w = 1.0 / (1.0 + zs[j] * 0.1), zk = 1.0;
        for (int k = 0; k < 5; k++) { S[k] += w * zk; if (k < 3) T[k] += w * xs[j] * zk; zk *= zs[j]; }
    }
    coef[0] = coef[1] = coef[2] = 0.0f;
    if (quad) {                                   // 3x3 normal equations (Cramer)
        double A[3][3] = { {S[0], S[1], S[2]}, {S[1], S[2], S[3]}, {S[2], S[3], S[4]} };
        double det = A[0][0] * (A[1][1] * A[2][2] - A[1][2] * A[2][1])
                   - A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0])
                   + A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0]);
        if (fabs(det) > 1e-9) {
            double r[3];
            for (int col = 0; col < 3; col++) {
                double B[3][3];
                memcpy(B, A, sizeof(B));
                for (int row = 0; row < 3; row++) B[row][col] = T[row];
                r[col] = (B[0][0] * (B[1][1] * B[2][2] - B[1][2] * B[2][1])
                        - B[0][1] * (B[1][0] * B[2][2] - B[1][2] * B[2][0])
                        + B[0][2] * (B[1][0] * B[2][1] - B[1][1] * B[2][0])) / det;
            }
            if (fabs(r[2]) <= MM_LANE_MAX_CURV) {
                coef[0] = (float)r[0]; coef[1] = (float)r[1]; coef[2] = (float)r[2];
                return 0;
            }
        }
        // singular or too curved: fall back to a straight line
    }
    double det = S[0] * S[2] - S[1] * S[1];
    if (fabs(det) < 1e-9) return -1;
    coef[1] = (float)((S[0] * T[1] - S[1] * T[0]) / det);
    coef[0] = (float)((T[0] - coef[1] * S[1]) / S[0]);
    return 0;
}

/* Fitted ego-lane boundaries of the frame being drawn ([0] left, [1] right) */
typedef struct {
    int   ok[2];
    float cf[2][3];
} stMmEgoFit;

/* Lane lines from the network points of the newest lane frame, with the same
 * geometry as the vehicles, fitted and drawn over the whole panel height.
 * Style: lane class 1 = solid, 2 = dashed, other = dashed (never guessed solid);
 * NC_MINIMAP_USE_LANE_CLASS 0 = all dashed. Ego-lane boundaries white, others grey. */
static void mm_draw_lanes(uint32_t ch, stMmEgoFit *ego_fit, const struct gl_npu_program *p)
{
    stNcLdwLanes *L = &g_mm_lanes;
    stNcLdwGeom g;
    if (nc_ldw_app_get_lanes(ch, L) != 0) return;
    if (nc_ldw_app_get_geom(ch, 0, &g) < 0) return;

    const float ego_col[4] = {1.0f, 1.0f, 1.0f, 0.95f};
    const float adj_col[4] = {0.70f, 0.70f, 0.70f, 0.90f};

    for (int i = 0; i < L->lane_num; i++) {
        const stLaneDet *ln = &L->lane[i];
        int n = ln->point_cnt;
        if (n > MAX_POINT_CNT_OF_LANE) n = MAX_POINT_CNT_OF_LANE;
        if (n < 2) continue;

        float xs[MAX_POINT_CNT_OF_LANE], zs[MAX_POINT_CNT_OF_LANE];
        float zmin = 1e9f, zmax = -1e9f;
        int m = 0;
        for (int j = 0; j < n; j++) {
            float u = ln->point[j].x, v = ln->point[j].y;
            if (!isfinite(u) || !isfinite(v) || (u <= 0.0f && v <= 0.0f)) continue;
            float x, z;
            if (nc_ldw_geom_to_road(ch, &g, u, v, &x, &z) != 0 || z <= 0.0f || z > MM_LANE_MAX_Z_M) continue;
            xs[m] = x; zs[m] = z; m++;
            if (z < zmin) zmin = z;
            if (z > zmax) zmax = z;
        }
        if (m < 2 || zmax - zmin < MM_LANE_MIN_SPAN_M) continue;

        float cf[3];
        int quad = (m >= 4 && zmax - zmin >= MM_LANE_QUAD_SPAN_M);
        if (mm_lane_fit(xs, zs, m, quad, cf) != 0) continue;

        int ego = (i == (int)NC_LDW_EGO_LANE.left_boundary_index ||
                   i == (int)NC_LDW_EGO_LANE.right_boundary_index);
        for (int s = 0; s < 2; s++) {
            if (i == (int)(s ? NC_LDW_EGO_LANE.right_boundary_index : NC_LDW_EGO_LANE.left_boundary_index)) {
                ego_fit->ok[s] = 1;
                memcpy(ego_fit->cf[s], cf, sizeof(cf));
            }
        }
        int dashed = NC_MINIMAP_USE_LANE_CLASS ? (ln->lane_class != 1) : 1;
        const float *col = ego ? ego_col : adj_col;
        const float fcol[4] = {col[0], col[1], col[2], col[3] * MM_LANE_EXTRAP_ALPHA};

        // segment ends: panel bottom .. panel top, split at the real-point range
        float zb[16];
        int nb = 0;
        const float z_lo = -MM_BEHIND_M, z_hi = NC_MINIMAP_RANGE_M;
        const float cuts[2] = { zmin, zmax };
        zb[nb++] = z_lo;
        for (float z = z_lo + MM_LANE_SEG_M; ; z += MM_LANE_SEG_M) {
            for (int k = 0; k < 2; k++)                   // insert the cuts in order
                if (cuts[k] > zb[nb - 1] && cuts[k] < fminf(z, z_hi) && nb < 15) zb[nb++] = cuts[k];
            if (z >= z_hi || nb >= 15) { zb[nb++] = z_hi; break; }
            zb[nb++] = z;
        }
        for (int k = 0; k + 1 < nb; k++) {
            float z0 = zb[k], z1 = zb[k + 1];
            if (z1 - z0 < 0.05f) continue;
            float x0 = cf[0] + cf[1] * z0 + cf[2] * z0 * z0;
            float x1 = cf[0] + cf[1] * z1 + cf[2] * z1 * z1;
            float zm = 0.5f * (z0 + z1);
            mm_road_seg(x0, z0, x1, z1, dashed, (zm >= zmin && zm <= zmax) ? col : fcol, p);
        }
    }
}

#if NC_MINIMAP_SHOW_PRED
/* Lane of a vehicle relative to the ego lane, measured from the ego-lane centre
 * at the vehicle's distance so that it follows curves and the ego offset.
 * Keeps the previous lane until the vehicle is clearly past the boundary. */
static int mm_track_lane(stMmTrack *t, const stMmEgoFit *e)
{
    const float w = NC_LDW_DEFAULT_LANE_WIDTH_M;
    float xs[2] = {0.0f, 0.0f}, centre = 0.0f;
    for (int s = 0; s < 2; s++)
        if (e->ok[s]) xs[s] = e->cf[s][0] + e->cf[s][1] * t->z + e->cf[s][2] * t->z * t->z;
    if (e->ok[0] && e->ok[1]) centre = 0.5f * (xs[0] + xs[1]);
    else if (e->ok[0])        centre = xs[0] + 0.5f * w;
    else if (e->ok[1])        centre = xs[1] - 0.5f * w;

    float pos = (t->x - centre) / w;
    if (!t->lane_ok || fabsf(pos - (float)t->lane) > 0.5f + NC_MINIMAP_PRED_LANE_HYST) {
        t->lane = (int)lroundf(pos);
        t->lane_ok = 1;
    }
    return t->lane;
}

/* Arrow from road point (x0, z0) along (dx, dz): dark outline, body that widens
 * and fades in towards the head, filled head. The head is left out when the
 * arrow leaves the panel. */
static void mm_draw_pred(float x0, float z0, float dx, float dz, const float color[4],
                         const struct gl_npu_program *p)
{
    const float bw = NC_MINIMAP_PRED_BODY_W * 0.5f, e = NC_MINIMAP_PRED_OUTLINE_PX;
    const float taper = 0.55f;                       // start width / head-end width
    const int   nseg = 6;
    float t0, t1;
    if (!mm_clip_seg(x0, z0, x0 + dx, z0 + dz, &t0, &t1)) return;

    float ax = mm_px(x0), ay = mm_py(z0);
    float vx = mm_px(x0 + dx) - ax, vy = mm_py(z0 + dz) - ay;
    float len = sqrtf(vx * vx + vy * vy);
    if (len < 1.0f) return;
    float ux = vx / len, uy = vy / len, nx = -uy, ny = ux;

    float hl = fminf(NC_MINIMAP_PRED_HEAD_LEN, 0.4f * len), hw = fmaxf(hl * 0.6f, NC_MINIMAP_PRED_BODY_W);
    float tipx = ax + vx, tipy = ay + vy;
    float neckx = tipx - ux * hl, necky = tipy - uy * hl;
    float wing = hw + 2.0f * e;
    int head = (t1 >= 1.0f);
    for (float s = -1.0f; head && s <= 1.0f; s += 2.0f) {
        float wx = neckx + (float)s * nx * wing, wy = necky + (float)s * ny * wing;
        if (wx < k_px || wx > k_px + k_pw || wy < k_py || wy > k_py + k_ph) head = 0;
    }
    float te = head ? 1.0f - hl / len : t1;          // body end
    if (te <= t0) return;

    float sx = ax + t0 * vx, sy = ay + t0 * vy, ex = ax + te * vx, ey = ay + te * vy;
    float ws = bw * taper + e, we = bw + e;

    // outline: one polygon, so that it is not blended twice
    const float dark[4] = {0.0f, 0.0f, 0.0f, 0.6f};
    if (head) {
        const float o[9][2] = {
            { ex, ey },
            { sx + nx * ws, sy + ny * ws }, { ex + nx * we, ey + ny * we },
            { neckx + nx * wing, necky + ny * wing }, { tipx + ux * 2.0f * e, tipy + uy * 2.0f * e },
            { neckx - nx * wing, necky - ny * wing }, { ex - nx * we, ey - ny * we },
            { sx - nx * ws, sy - ny * ws }, { sx + nx * ws, sy + ny * ws },
        };
        mm_fill_poly(o, 9, dark, p);
    } else {
        const float o[4][2] = {
            { sx + nx * ws, sy + ny * ws }, { ex + nx * we, ey + ny * we },
            { ex - nx * we, ey - ny * we }, { sx - nx * ws, sy - ny * ws },
        };
        mm_fill_poly(o, 4, dark, p);
    }

    // body
    for (int k = 0; k < nseg; k++) {
        float f0 = (float)k / (float)nseg, f1 = (float)(k + 1) / (float)nseg;
        float w0 = bw * (taper + (1.0f - taper) * f0), w1 = bw * (taper + (1.0f - taper) * f1);
        float px0 = sx + (ex - sx) * f0, py0 = sy + (ey - sy) * f0;
        float px1 = sx + (ex - sx) * f1, py1 = sy + (ey - sy) * f1;
        const float q[4][2] = {
            { px0 + nx * w0, py0 + ny * w0 }, { px1 + nx * w1, py1 + ny * w1 },
            { px1 - nx * w1, py1 - ny * w1 }, { px0 - nx * w0, py0 - ny * w0 },
        };
        float col[4] = {color[0], color[1], color[2], color[3] * (0.35f + 0.65f * f1)};
        mm_fill_poly(q, 4, col, p);
    }

    if (head) {
        const float h[3][2] = {
            { neckx + nx * hw, necky + ny * hw }, { tipx, tipy }, { neckx - nx * hw, necky - ny * hw },
        };
        mm_fill_poly(h, 3, color, p);
    }
}
#endif

static void mm_text(const stNcMinimapDrawRes *res, const char *s, float x, float y_canvas,
                    float scale, float color[3])
{
    // text y is measured from the bottom
    nc_opengl_draw_text(res->font, s, x, (float)VIEWPORT_HEIGHT - y_canvas, scale, color,
                        VIEWPORT_WIDTH, VIEWPORT_HEIGHT, *res->font_prog);
}

/* ------------------------------------------------------------------------- */

void nc_minimap_draw(uint32_t ch, struct viewport vp, const stNcMinimapDrawRes *res)
{
    if (ch >= NC_LDW_MAX_CH || res == NULL || vp.height == 0) return;
    stMmChannel *c = &g_mm[ch];
    const struct gl_npu_program *p = res->npu_prog;
    float text_scale = (float)VIEWPORT_HEIGHT / (float)vp.height * 0.8f;
    char buf[32];

    mm_process(ch);
    mm_log(ch);

    glViewport((GLint)vp.x, (GLint)vp.y, (GLsizei)vp.width, (GLsizei)vp.height);

    // background + frame
    const float bg[4]    = {0.05f, 0.05f, 0.07f, NC_MINIMAP_BG_ALPHA};
    const float frame[4] = {0.7f, 0.7f, 0.7f, 1.0f};
    mm_fill_rect(k_px, k_py, k_pw, k_ph, bg, p);
    mm_rect(k_px, k_py, k_pw, k_ph, frame, p);

    stNcLdwGeom g;
    if (nc_ldw_app_get_geom(ch, 0, &g) < 0) {
        float gray[3] = {0.8f, 0.8f, 0.8f};
        mm_text(res, "MINIMAP: no lane", k_px + 10.0f, k_py + 30.0f, text_scale, gray);
        return;
    }

    // distance grid every 10 m (labels just below the line)
    float grid_txt[3] = {0.75f, 0.75f, 0.75f};
    for (int d = 10; d <= (int)NC_MINIMAP_RANGE_M; d += 10) {
        float y = mm_py((float)d);
        float gcol[4] = {0.5f, 0.5f, 0.5f, 0.5f};
        nc_opengl_draw_line(k_px, y, k_px + k_pw, y, 1, gcol, *p);
        snprintf(buf, sizeof(buf), "%dm", d);
        mm_text(res, buf, k_px + 4.0f, y + 20.0f, text_scale, grid_txt);
    }

    stMmEgoFit ego_fit;
    memset(&ego_fit, 0, sizeof(ego_fit));
    mm_draw_lanes(ch, &ego_fit, p);

    // vehicles, far first so that near ones are drawn on top
    int order[MM_MAX_TRACK], cnt = 0;
    for (int i = 0; i < MM_MAX_TRACK; i++)
        if (mm_track_drawn(c, &c->track[i])) order[cnt++] = i;
    for (int i = 1; i < cnt; i++) {
        int t = order[i], j = i;
        while (j > 0 && c->track[order[j - 1]].z < c->track[t].z) { order[j] = order[j - 1]; j--; }
        order[j] = t;
    }

    const float cutin[4] = {1.0f, 0.9f, 0.0f, 1.0f};
    const float edge[4]  = {1.0f, 1.0f, 1.0f, 0.9f};
    for (int i = 0; i < cnt; i++) {
        const stMmTrack *t = &c->track[order[i]];
        float fc[4] = {0.8f, 0.2f, 0.2f, 1.0f};
        if (t->has_color[t->cls]) memcpy(fc, t->color[t->cls], sizeof(fc));
        else for (int k = 0; k < MM_CLS_N; k++) if (t->has_color[k]) { memcpy(fc, t->color[k], sizeof(fc)); break; }
        if (t->highlight) memcpy(fc, cutin, sizeof(fc));
        fc[3] = 0.95f;

        float w = k_cls_w[t->cls], l = k_cls_l[t->cls];
        mm_road_rect(t->x, w, t->z, t->z + l, fc, 1, p);
        mm_road_rect(t->x, w, t->z, t->z + l, t->highlight ? cutin : edge, 0, p);
    }

#if NC_MINIMAP_SHOW_PRED
    // predicted trajectory of the nearest vehicle of each lane, from its front edge:
    // forward, leaning by the predicted lateral motion. Drawn over the boxes so that it
    // stays visible on the vehicle ahead. order[] is far first: the last one of a lane is the nearest.
    const float pred_col[4] = NC_MINIMAP_PRED_COLOR;
    const float pred_cutin[4] = NC_MINIMAP_PRED_CUTIN_COLOR;
    int nearest[2 * MM_PRED_LANE_MAX + 1];
    for (int k = 0; k < 2 * MM_PRED_LANE_MAX + 1; k++) nearest[k] = -1;
    for (int i = 0; i < cnt; i++) {
        int lane = mm_track_lane(&c->track[order[i]], &ego_fit);
        if (lane >= -MM_PRED_LANE_MAX && lane <= MM_PRED_LANE_MAX) nearest[lane + MM_PRED_LANE_MAX] = order[i];
    }
    for (int k = 0; k < 2 * MM_PRED_LANE_MAX + 1; k++) {
        if (nearest[k] < 0) continue;
        const stMmTrack *t = &c->track[nearest[k]];
        if (t->fut_n == 0) continue;
        float len = sqrtf(t->fut_dx * t->fut_dx + t->fut_dz * t->fut_dz);
        if (len > NC_MINIMAP_PRED_MAX_M) continue;
        mm_draw_pred(t->x, t->z + k_cls_l[t->cls], t->fut_dx, NC_MINIMAP_PRED_FWD_M,
                     t->highlight ? pred_cutin : pred_col, p);
    }
#endif

    // ego vehicle
    const float ego[4] = {1.0f, 1.0f, 1.0f, 0.95f};
    const float ego_win[4] = {0.2f, 0.4f, 0.8f, 1.0f};
    mm_road_rect(0.0f, MM_EGO_W_M, -MM_EGO_REAR_M, MM_EGO_FRONT_M, ego, 1, p);
    mm_road_rect(0.0f, MM_EGO_W_M * 0.7f, 0.0f, 0.8f, ego_win, 1, p);   // windshield = front
}
