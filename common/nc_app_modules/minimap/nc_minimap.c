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
 */

#define MM_MAX_OBS          (64)        // detections per frame
#define MM_MAX_TRACK        (48)        // minimap tracks per channel
#define MM_MAX_ALIAS        (4)         // (class, tracker id) keys per minimap track
#define MM_BEHIND_M         (3.0f)      // road shown behind the camera (ego body)

/* per-vehicle alpha-beta filter (metres) */
#define MM_PIX_NOISE        (2.0f)      // bbox bottom noise (canvas px) used to scale the gain
#define MM_AZ_MAX           (0.55f)     // forward gain for precise (near) measurements
#define MM_AZ_MIN           (0.15f)     // forward gain floor for far, noisy measurements
#define MM_SIGMA_REF_M      (0.30f)     // noise at which the gain starts dropping
#define MM_AX               (0.50f)     // lateral gain
#define MM_VZ_MAX           (25.0f)     // max relative forward speed (m/s, 90 km/h)
#define MM_VX_MAX           (3.0f)      // max lateral speed (m/s)
#define MM_TRACK_GAP_US     (500000ULL) // longer gap -> restart the filter / no re-association
#define MM_HOLD_US          (300000ULL) // keep drawing a vehicle this long after its last detection
#define MM_DROP_US          (1000000ULL)// forget a track not detected this long
#define MM_MIN_UPDATES      (2)         // detections needed before a vehicle is drawn

/* association gates (m) */
#define MM_GATE_Z_MIN       (2.0f)
#define MM_GATE_Z_RATIO     (0.15f)
#define MM_GATE_X           (1.5f)
#define MM_DUP_Z_MIN        (1.0f)      // same vehicle detected twice (two classes) in one frame
#define MM_DUP_Z_RATIO      (0.06f)
#define MM_DUP_X            (0.8f)

/* class vote */
#define MM_CLS_DECAY        (0.8f)
#define MM_CLS_SWITCH       (1.0f)      // other class must lead by this much to switch

/* bbox-width distance (pitch independent), fused with the ground distance */
#define MM_W_PIX_NOISE      (3.0f)      // bbox width noise (canvas px, about 1 NPU column)
#define MM_W_SCALE_STD      (0.10f)     // vehicle width prior error before calibration
#define MM_W_SCALE_STD_CAL  (0.05f)     // after MM_W_CAL_MIN near-range calibrations
#define MM_W_CAL_MAX_Z      (15.0f)     // learn the per-vehicle width scale only this close
#define MM_W_CAL_MIN        (10)
#define MM_W_MIN_PX         (8.0f)      // narrower boxes: width distance not used

/* lanes: consecutive network points are joined (like the camera overlay).
 * A gap longer than this is a straight interpolation between two real points
 * (rows are about 50 px apart, i.e. ~14 m -> ~60 m beyond 14 m): drawn thin/faint. */
#define MM_LANE_INTERP_GAP_M (8.0f)
#define MM_LANE_W_PX        (3.0f)
#define MM_LANE_W_INTERP_PX (1.5f)

/* marker for vehicles beyond the panel edge */
#define MM_ARROW_PX         (10.0f)

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
    float     x, z, vx, vz;     // filtered state at t_us
    uint64_t  t_us;             // detection time of the last filter update
    int       n_upd;
    uint64_t  last_seen_disp_us;
    int       in_range;         // range hysteresis state
    float     cls_score[MM_CLS_N];
    int       cls;
    float     color[MM_CLS_N][4];
    int       has_color[MM_CLS_N];
    int       highlight;
    float     w_scale;          // ground distance / width distance, learned at near range
    int       w_cal_n;
} stMmTrack;

typedef struct {
    stNcMinimapObj in;
    int    ok;                  // road position available
    int    cls;
    float  xm, zm;              // measured road position used by the filter (fused)
    float  sig_z, sig_x;        // measurement noise (m)
    float  zg, sig_g;           // ground distance (box bottom + per-frame horizon)
    float  zw;                  // bbox-width distance (0 = not used)
    float  vp_used;
    uint64_t geom_t_us;         // capture time of the lane frame whose horizon was used
    int    same_frame;          // geometry of the detection's own lane frame
    float  x_old, z_old;        // previous method (shown box + EMA horizon), diagnostics
    int    track;               // index into tracks, -1 = none
} stMmObs;

typedef struct {
    uint64_t  t_disp_us;
    int       obs_cnt;
    stMmObs   obs[MM_MAX_OBS];
    uint32_t  next_id;
    stMmTrack track[MM_MAX_TRACK];
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

static void mm_vote(stMmTrack *t, int cls, float weight)
{
    for (int i = 0; i < MM_CLS_N; i++) t->cls_score[i] *= MM_CLS_DECAY;
    t->cls_score[cls] += weight;
    if (cls != t->cls && t->cls_score[cls] > t->cls_score[t->cls] + MM_CLS_SWITCH) t->cls = cls;
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
    // a key belongs to one track only
    for (int i = 0; i < MM_MAX_TRACK; i++) {
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
    t->w_scale = 1.0f;
    return t;
}

/* Fuse the ground distance with the bbox-width distance of the track's class.
 * The width does not move with pitch; its error does not grow with z^2, so it
 * takes over at long range. Visible width = W cos(a) + L sin(a), a = view angle. */
static void mm_fuse(stMmTrack *t, stMmObs *o, float focal)
{
    o->zm = o->zg; o->sig_z = o->sig_g; o->zw = 0.0f;
    if (focal <= 0.0f || t->cls == MM_CLS_MOTO || o->in.meas.w < MM_W_MIN_PX) return;

    float a = atanf(fabsf(o->xm) / fmaxf(o->zg, 1.0f));
    float w_vis = k_cls_w[t->cls] * cosf(a) + k_cls_l[t->cls] * sinf(a);
    float zw0 = focal * w_vis / o->in.meas.w;

    // learn this vehicle's width at near range, where the ground distance is precise
    if (o->zg < MM_W_CAL_MAX_Z && o->in.t_det_us != t->t_us) {
        float ratio = o->zg / zw0;
        if (ratio > 0.6f && ratio < 1.6f) {
            t->w_scale += 0.1f * (ratio - t->w_scale);
            t->w_cal_n++;
        }
    }
    o->zw = zw0 * t->w_scale;

    float ss = t->w_cal_n >= MM_W_CAL_MIN ? MM_W_SCALE_STD_CAL : MM_W_SCALE_STD;
    float rel = MM_W_PIX_NOISE / o->in.meas.w;
    float sig_w = o->zw * sqrtf(rel * rel + ss * ss);
    float ig = 1.0f / (o->sig_g * o->sig_g), iw = 1.0f / (sig_w * sig_w);
    o->zm = (o->zg * ig + o->zw * iw) / (ig + iw);
    o->sig_z = 1.0f / sqrtf(ig + iw);
}

/* alpha-beta update with a physical rate limit. Gains drop with measurement
 * noise (far vehicles), so near vehicles stay responsive. */
static void mm_filter(stMmTrack *t, const stMmObs *o)
{
    uint64_t tm = o->in.t_det_us;

    if (t->n_upd == 0 || tm <= t->t_us || tm - t->t_us > MM_TRACK_GAP_US) {
        if (t->n_upd > 0 && tm <= t->t_us) return;    // same / older frame
        t->x = o->xm; t->z = o->zm;
        t->vx = t->vz = 0.0f;
        t->t_us = tm;
        t->n_upd = 1;
        mm_vote(t, o->cls, 1.0f);
        return;
    }

    float dt = (float)(tm - t->t_us) * 1e-6f;
    float zp = t->z + t->vz * dt;
    float xp = t->x + t->vx * dt;

    // residual limited to what a vehicle can do in dt plus measurement noise
    float rz = o->zm - zp, lz = MM_VZ_MAX * dt + 3.0f * o->sig_z;
    float rx = o->xm - xp, lx = MM_VX_MAX * dt + 3.0f * o->sig_x;
    if (rz > lz) rz = lz; else if (rz < -lz) rz = -lz;
    if (rx > lx) rx = lx; else if (rx < -lx) rx = -lx;

    float az = MM_AZ_MAX * (MM_SIGMA_REF_M / fmaxf(o->sig_z, 1e-3f));
    if (az > MM_AZ_MAX) az = MM_AZ_MAX;
    if (az < MM_AZ_MIN) az = MM_AZ_MIN;
    float bz = az * az / (2.0f - az);
    float bx = MM_AX * MM_AX / (2.0f - MM_AX);

    t->z  = zp + az * rz;
    t->vz += bz * rz / dt;
    t->x  = xp + MM_AX * rx;
    t->vx += bx * rx / dt;
    t->vz = fmaxf(-MM_VZ_MAX, fminf(MM_VZ_MAX, t->vz));
    t->vx = fmaxf(-MM_VX_MAX, fminf(MM_VX_MAX, t->vx));
    t->t_us = tm;
    t->n_upd++;
    mm_vote(t, o->cls, 1.0f);
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

    // ground contact point of the measured box, with the horizon of its own frame
    stNcLdwGeom g;
    int gr = nc_ldw_app_get_geom(ch, in->t_det_us, &g);
    float u = in->meas.x + in->meas.w * 0.5f, v = in->meas.y + in->meas.h;
    if (gr >= 0 && nc_ldw_geom_to_road(ch, &g, u, v, &o->xm, &o->zg) == 0 && o->zg > 0.0f) {
        o->ok = 1;
        o->vp_used = g.vp_y;
        o->geom_t_us = g.t_us;
        o->same_frame = (gr == 0);
        o->sig_g = MM_PIX_NOISE * o->zg / (v - g.vp_y);   // dz/dv = z / (v - vp)
        o->sig_x = 0.1f + 0.01f * o->zg;
        o->zm = o->zg; o->sig_z = o->sig_g;               // until fused with the track's class
    }

    // previous method for comparison: drawn (predicted) box + EMA horizon
    if (nc_bevlog_enabled()) {
        stNcLdwGeom gc;
        float k = 0.0f, vp = 0.0f;
        if (nc_ldw_app_get_calib(ch, &k, &vp, NULL) == 0) {
            gc.t_us = 0; gc.k = k; gc.vp_y = vp; gc.pitch_measured = 0;
            if (nc_ldw_geom_to_road(ch, &gc, in->shown.x + in->shown.w * 0.5f, in->shown.y + in->shown.h,
                                    &o->x_old, &o->z_old) != 0) {
                o->x_old = o->z_old = 0.0f;
            }
        }
    }
}

/* match this frame's detections to minimap tracks and update the filters */
static void mm_process(uint32_t ch)
{
    stMmChannel *c = &g_mm[ch];
    const uint64_t now = c->t_disp_us;
    const float focal = nc_ldw_app_focal_px(ch);

    for (int i = 0; i < MM_MAX_TRACK; i++) {
        stMmTrack *t = &c->track[i];
        if (!t->used) continue;
        t->highlight = 0;
        if (now > t->last_seen_disp_us && now - t->last_seen_disp_us > MM_DROP_US) t->used = 0;
    }

    // 1. known (class, tracker id)
    for (int n = 0; n < c->obs_cnt; n++) {
        stMmObs *o = &c->obs[n];
        if (!o->ok) continue;
        stMmTrack *t = mm_find_alias(c, o->in.class_name, o->in.track_id);
        if (t == NULL) continue;
        o->track = (int)(t - c->track);
        mm_fuse(t, o, focal);
        mm_filter(t, o);
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
            float gz, gx, dz, dx;
            int is_dup = (t->t_us == o->in.t_det_us);  // already updated by another box of this frame
            if (is_dup) {
                gz = fmaxf(MM_DUP_Z_MIN, MM_DUP_Z_RATIO * o->zm);
                gx = MM_DUP_X;
                dz = o->zm - t->z;
                dx = o->xm - t->x;
            } else {
                if (o->in.t_det_us < t->t_us || o->in.t_det_us - t->t_us > MM_TRACK_GAP_US) continue;
                float dt = (float)(o->in.t_det_us - t->t_us) * 1e-6f;
                gz = fmaxf(MM_GATE_Z_MIN, MM_GATE_Z_RATIO * o->zm);
                gx = MM_GATE_X;
                dz = o->zm - (t->z + t->vz * dt);
                dx = o->xm - (t->x + t->vx * dt);
            }
            float cost = (dz / gz) * (dz / gz) + (dx / gx) * (dx / gx);
            if (cost < best_cost) { best_cost = cost; best = i; dup = is_dup; }
        }
        if (best < 0) continue;

        stMmTrack *t = &c->track[best];
        mm_add_alias(c, t, o->in.class_name, o->in.track_id);
        o->track = best;
        if (dup) mm_vote(t, o->cls, 0.5f);           // two classes for one vehicle: half a vote
        else   { mm_fuse(t, o, focal); mm_filter(t, o); }
    }

    // 3. new vehicles
    for (int n = 0; n < c->obs_cnt; n++) {
        stMmObs *o = &c->obs[n];
        if (!o->ok || o->track >= 0) continue;
        stMmTrack *t = mm_new_track(c, o);
        mm_add_alias(c, t, o->in.class_name, o->in.track_id);
        o->track = (int)(t - c->track);
        mm_fuse(t, o, focal);
        mm_filter(t, o);
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
        else if (t->in_range && t->z > NC_MINIMAP_RANGE_M + NC_MINIMAP_RANGE_HYST_M) t->in_range = 0;
    }
}

static int mm_track_drawn(const stMmChannel *c, const stMmTrack *t)
{
    return t->used && t->in_range && t->n_upd >= MM_MIN_UPDATES &&
           c->t_disp_us >= t->last_seen_disp_us && c->t_disp_us - t->last_seen_disp_us <= MM_HOLD_US;
}

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
                      "%.2f,%d,%u,%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f",
                      (unsigned long long)o->in.t_disp_us, (unsigned long long)o->in.t_det_us,
                      (unsigned long long)o->geom_t_us, ch, o->in.track_id,
                      o->in.class_name ? o->in.class_name : "?",
                      (double)o->in.meas.x, (double)o->in.meas.y, (double)o->in.meas.w, (double)o->in.meas.h,
                      o->in.pred_valid,
                      (double)o->in.shown.x, (double)o->in.shown.y, (double)o->in.shown.w, (double)o->in.shown.h,
                      (double)(o->in.meas.x + o->in.meas.w * 0.5f), (double)(o->in.meas.y + o->in.meas.h),
                      (double)vp, (double)k,
                      (double)o->xm, (double)(o->ok ? o->zg : 0.0f),
                      (double)(t ? t->x : 0.0f), (double)(t ? t->z : 0.0f),
                      t ? mm_track_drawn(c, t) : 0,
                      (double)o->vp_used, o->same_frame, t ? t->id : 0u,
                      t ? cls_name[t->cls] : "-",
                      (double)o->x_old, (double)o->z_old,
                      (double)o->zg, (double)o->zw, (double)(t ? t->w_scale : 0.0f), (double)o->zm);
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

/* road rectangle (x centre, z from z0 to z1), must already be inside the panel */
static void mm_road_rect(float xc, float w_m, float z0, float z1, const float color[4], int fill,
                         const struct gl_npu_program *p)
{
    float left = mm_px(xc - w_m * 0.5f), right = mm_px(xc + w_m * 0.5f);
    float top  = mm_py(z1),              bot   = mm_py(z0);
    if (right - left < 1.0f || bot - top < 1.0f) return;

    if (fill) mm_fill_rect(left, top, right - left, bot - top, color, p);
    else      mm_rect(left, top, right - left, bot - top, color, p);
}

/* line with its own width (nc_opengl_draw_line is fixed at 6 px) */
static void mm_line(float x0, float y0, float x1, float y1, int dashed, float width,
                    const float color[4], const struct gl_npu_program *p)
{
    float v[6];
    nc_opengl_create_line_vertex(x0, y0, x1, y1, v);

    glUseProgram(p->program);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glVertexAttribPointer(p->pos_handle, 3, GL_FLOAT, GL_FALSE, 0, v);
    glEnableVertexAttribArray(p->pos_handle);
    if (dashed) {
        glUniform2f((GLint)p->viewport_size_handle, (float)VIEWPORT_WIDTH, (float)VIEWPORT_HEIGHT);
        glUniform1i((GLint)p->drawing_type_handle, DRAW_NPU_TYPE_DASHED);
    } else {
        glUniform1i((GLint)p->drawing_type_handle, DRAW_NPU_TYPE_SOLID);
    }
    glUniform4fv((GLint)p->color_handle, 1, color);
    glLineWidth((GLfloat)width);
    glDrawArrays(GL_LINES, 0, 2);
    glDisable(GL_BLEND);
    glDisableVertexAttribArray(p->pos_handle);
    glUseProgram(0);
}

/* filled triangle (canvas px) */
static void mm_tri(float x0, float y0, float x1, float y1, float x2, float y2, const float color[4],
                   const struct gl_npu_program *p)
{
    float v[9];
    nc_opengl_pixel_to_ndc(x0, y0, &v[0], &v[1]); v[2] = 0.0f;
    nc_opengl_pixel_to_ndc(x1, y1, &v[3], &v[4]); v[5] = 0.0f;
    nc_opengl_pixel_to_ndc(x2, y2, &v[6], &v[7]); v[8] = 0.0f;

    glUseProgram(p->program);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glVertexAttribPointer(p->pos_handle, 3, GL_FLOAT, GL_FALSE, 0, v);
    glEnableVertexAttribArray(p->pos_handle);
    glUniform1i((GLint)p->drawing_type_handle, DRAW_NPU_TYPE_SOLID);
    glUniform4fv((GLint)p->color_handle, 1, color);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDisable(GL_BLEND);
    glDisableVertexAttribArray(p->pos_handle);
    glUseProgram(0);
}

/* road segment clipped to the panel (Liang-Barsky). dashed: 1/0 */
static void mm_road_seg(float x0, float z0, float x1, float z1, int dashed, float width,
                        const float color[4], const struct gl_npu_program *p)
{
    const float lat = NC_MINIMAP_LAT_RANGE_M;
    float t0 = 0.0f, t1 = 1.0f, dx = x1 - x0, dz = z1 - z0;
    const float pq[4][2] = {
        { -dx, x0 + lat }, { dx, lat - x0 },
        { -dz, z0 + MM_BEHIND_M }, { dz, NC_MINIMAP_RANGE_M - z0 },
    };
    for (int i = 0; i < 4; i++) {
        float pp = pq[i][0], q = pq[i][1];
        if (fabsf(pp) < 1e-9f) { if (q < 0.0f) return; continue; }
        float r = q / pp;
        if (pp < 0.0f) { if (r > t1) return; if (r > t0) t0 = r; }
        else           { if (r < t0) return; if (r < t1) t1 = r; }
    }
    mm_line(mm_px(x0 + t0 * dx), mm_py(z0 + t0 * dz), mm_px(x0 + t1 * dx), mm_py(z0 + t1 * dz),
            dashed, width, color, p);
}

/* Lane lines from the network points of the newest lane frame (time-averaged by
 * UFLD tracking), transformed with the horizon of those same averaged points so
 * the drawn lanes keep their width. Consecutive points are joined, like the
 * camera overlay; nothing is drawn beyond the last point (no extrapolation).
 * A long gap (> MM_LANE_INTERP_GAP_M) is a straight interpolation between two
 * real points and is drawn thin and faint.
 * Style follows the network lane class: 1 = solid, 2 = dashed, other = dashed
 * (unknown is never drawn as solid). Ego-lane boundaries white, others grey. */
static void mm_draw_lanes(uint32_t ch, const struct gl_npu_program *p)
{
    stNcLdwLanes *L = &g_mm_lanes;
    stNcLdwGeom g;
    if (nc_ldw_app_get_lanes(ch, L) != 0) return;
    if (nc_ldw_app_get_geom(ch, L->t_us, &g) < 0) return;
    g.vp_y = g.vp_lane_y;

    const float ego_col[4] = {1.0f, 1.0f, 1.0f, 0.95f};
    const float adj_col[4] = {0.70f, 0.70f, 0.70f, 0.90f};

    for (int i = 0; i < L->lane_num; i++) {
        const stLaneDet *ln = &L->lane[i];
        int n = ln->point_cnt;
        if (n > MAX_POINT_CNT_OF_LANE) n = MAX_POINT_CNT_OF_LANE;
        if (n < 2) continue;

        float xs[MAX_POINT_CNT_OF_LANE], zs[MAX_POINT_CNT_OF_LANE];
        int m = 0;
        for (int j = 0; j < n; j++) {
            float u = ln->point[j].x, v = ln->point[j].y;
            if (!isfinite(u) || !isfinite(v) || (u <= 0.0f && v <= 0.0f)) continue;
            float x, z;
            if (nc_ldw_geom_to_road(ch, &g, u, v, &x, &z) != 0 || z <= 0.0f) continue;
            int k = m++;                                  // insertion by distance
            while (k > 0 && zs[k - 1] > z) { zs[k] = zs[k - 1]; xs[k] = xs[k - 1]; k--; }
            zs[k] = z; xs[k] = x;
        }

        int ego = (i == (int)NC_LDW_EGO_LANE.left_boundary_index ||
                   i == (int)NC_LDW_EGO_LANE.right_boundary_index);
        int dashed = (ln->lane_class != 1);
        const float *col = ego ? ego_col : adj_col;
        for (int j = 0; j + 1 < m; j++) {
            if (zs[j] >= NC_MINIMAP_RANGE_M) break;      // clipped at the panel top
            if (zs[j + 1] - zs[j] > MM_LANE_INTERP_GAP_M) {
                float fc[4] = {col[0], col[1], col[2], col[3] * 0.5f};
                mm_road_seg(xs[j], zs[j], xs[j + 1], zs[j + 1], dashed, MM_LANE_W_INTERP_PX, fc, p);
            } else {
                mm_road_seg(xs[j], zs[j], xs[j + 1], zs[j + 1], dashed, MM_LANE_W_PX, col, p);
            }
        }
    }
}

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

    mm_draw_lanes(ch, p);

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
    const float lat = NC_MINIMAP_LAT_RANGE_M;
    for (int i = 0; i < cnt; i++) {
        const stMmTrack *t = &c->track[order[i]];
        float w = k_cls_w[t->cls], l = k_cls_l[t->cls];

        float fc[4] = {0.8f, 0.2f, 0.2f, 1.0f};
        if (t->has_color[t->cls]) memcpy(fc, t->color[t->cls], sizeof(fc));
        else for (int k = 0; k < MM_CLS_N; k++) if (t->has_color[k]) { memcpy(fc, t->color[k], sizeof(fc)); break; }
        if (t->highlight) memcpy(fc, cutin, sizeof(fc));
        fc[3] = 0.95f;

        // beyond the panel edge: arrow at the edge instead of a cut or moved box.
        // Forward: the rear (the position that matters) must be inside; a long
        // vehicle's front beyond 40 m is shortened to the 40 m line.
        int over_top = t->z > NC_MINIMAP_RANGE_M - 1.0f;
        int over_l = t->x - w * 0.5f < -lat, over_r = t->x + w * 0.5f > lat;
        if (!over_top && t->z + l > NC_MINIMAP_RANGE_M) l = NC_MINIMAP_RANGE_M - t->z;
        if (over_top || over_l || over_r) {
            const float a = MM_ARROW_PX;
            float ax = mm_px(fmaxf(-lat, fminf(lat, t->x)));
            float ay = mm_py(fminf(NC_MINIMAP_RANGE_M, fmaxf(-MM_BEHIND_M, t->z)));
            ax = fmaxf(k_px + a, fminf(k_px + k_pw - a, ax));
            ay = fmaxf(k_py + a, fminf(k_py + k_ph - a, ay));
            if (over_l)      mm_tri(k_px + 1.0f, ay, k_px + 1.0f + a, ay - a * 0.7f, k_px + 1.0f + a, ay + a * 0.7f, fc, p);
            else if (over_r) mm_tri(k_px + k_pw - 1.0f, ay, k_px + k_pw - 1.0f - a, ay - a * 0.7f, k_px + k_pw - 1.0f - a, ay + a * 0.7f, fc, p);
            else             mm_tri(ax, k_py + 1.0f, ax - a * 0.7f, k_py + 1.0f + a, ax + a * 0.7f, k_py + 1.0f + a, fc, p);
            continue;
        }

        mm_road_rect(t->x, w, t->z, t->z + l, fc, 1, p);
        mm_road_rect(t->x, w, t->z, t->z + l, t->highlight ? cutin : edge, 0, p);
    }

    // ego vehicle
    const float ego[4] = {1.0f, 1.0f, 1.0f, 0.95f};
    const float ego_win[4] = {0.2f, 0.4f, 0.8f, 1.0f};
    mm_road_rect(0.0f, MM_EGO_W_M, -MM_EGO_REAR_M, MM_EGO_FRONT_M, ego, 1, p);
    mm_road_rect(0.0f, MM_EGO_W_M * 0.7f, 0.0f, 0.8f, ego_win, 1, p);   // windshield = front
}
