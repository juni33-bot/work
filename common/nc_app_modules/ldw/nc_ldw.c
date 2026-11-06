#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

#include "nc_ldw.h"

/*
 * Shared functions from teammates (not merged yet). When their files (or
 * nc_c_stub.c) are in the build, uncomment the includes below and every line
 * marked "// [TEAM]".
 *   nc_meas_get_latency_ms() : capture -> NPU result latency (ms)
 *   nc_rec_trigger(type)     : start recording, 0 = lane departure
 *   nc_model_get_mode()      : MODE_NORMAL / MODE_LIGHT / MODE_SKIP
 */
// #include "nc_meas.h"     // [TEAM]
// #include "nc_rec.h"      // [TEAM]
// #include "nc_model.h"    // [TEAM]

#define NC_REC_TYPE_LANE_DEPARTURE  (0)

/* Print lane_class of every lane once a second (to confirm 0/1/2 meaning
 * on the board). Set to 0 when done. */
#define NC_LDW_LOG_LANE_CLASS       (0)

const stNcLdwEgoLane NC_LDW_EGO_LANE = {
    NC_LDW_LEFT_EGO_LANE_BOUNDARY_INDEX,
    NC_LDW_RIGHT_EGO_LANE_BOUNDARY_INDEX
};

#define LDW_MIN_FIT_POINTS          (3)
#define LDW_MAX_FIT_RMS_RATIO       (0.02f)   // max fit residual, ratio of img_w
#define LDW_MIN_LANE_PX_RATIO       (0.15f)   // min lane width at ref row, ratio of img_w
#define LDW_MAX_LANE_PX_RATIO       (1.50f)
#define LDW_CALIB_ALPHA             (0.10f)   // EMA weight for k / vp_y
#define LDW_VEL_ALPHA               (0.30f)   // EMA weight for lateral velocity
#define LDW_MAX_DT_S                (1.0f)
#define LDW_CLASS_ALPHA             (0.15f)   // EMA weight for the dashed/solid vote (~0.5 s)


static float us_to_s(uint64_t us)
{
    return (float)((double)us / 1000000.0);
}

static uint64_t s_to_us(float s)
{
    return (uint64_t)((double)s * 1000000.0);
}

/* Least-squares fit x = a*y + b using points with y_min <= y <= y_max. */
static int fit_lane_line(const stLaneDet *ln, float y_min, float y_max, float max_rms,
                         float *a, float *b)
{
    double sy = 0.0, syy = 0.0, sx = 0.0, sxy = 0.0;
    int n = 0;
    int cnt = ln->point_cnt;

    if (cnt > MAX_POINT_CNT_OF_LANE) cnt = MAX_POINT_CNT_OF_LANE;

    for (int i = 0; i < cnt; i++) {
        double x = (double)ln->point[i].x;
        double y = (double)ln->point[i].y;
        if (y < (double)y_min || y > (double)y_max) continue;
        sy += y; syy += y * y; sx += x; sxy += x * y;
        n++;
    }
    if (n < LDW_MIN_FIT_POINTS) return -1;

    double dn = (double)n;
    double denom = dn * syy - sy * sy;
    if (fabs(denom) < 1e-6) return -1;

    double fa = (dn * sxy - sy * sx) / denom;
    double fb = (sx - fa * sy) / dn;

    double err = 0.0;
    for (int i = 0; i < cnt; i++) {
        double x = (double)ln->point[i].x;
        double y = (double)ln->point[i].y;
        if (y < (double)y_min || y > (double)y_max) continue;
        double r = x - (fa * y + fb);
        err += r * r;
    }
    if (sqrt(err / dn) > (double)max_rms) return -1;

    *a = (float)fa;
    *b = (float)fb;
    return 0;
}

/* side 0 = left, 1 = right. Unknown class (-1) keeps the previous vote. */
static void ldw_vote_dashed(stNcLdwContext *ctx, int side, int lane_class)
{
    float *v = &ctx->dashed_score[side];
    if (lane_class < 0) return;
    float d = lane_class == ctx->cfg.dashed_class ? 1.0f : 0.0f;
    if (*v < 0.0f) *v = d;
    else           *v += LDW_CLASS_ALPHA * (d - *v);
}

void nc_ldw_default_config(stNcLdwConfig *cfg, float img_w, float img_h)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->img_w           = img_w;
    cfg->img_h           = img_h;
    cfg->lane_width_m    = NC_LDW_DEFAULT_LANE_WIDTH_M;
    cfg->vehicle_width_m = 1.85f;
    cfg->cam_x_px        = img_w * 0.5f;
    cfg->cam_offset_m    = 0.0f;
    cfg->ref_y_px        = img_h * 0.80f;
    cfg->fit_y_min_px    = img_h * 0.55f;
    cfg->focal_px        = NC_LDW_DEFAULT_FOCAL_PX;   // forward distance only (LDW_FOCAL_PX to override)
    cfg->warn_margin_m   = 0.50f;   // warn when the car edge is closer than this to a line
    cfg->reaction_time_s = 0.8f;    // TLC threshold = 0.8 + latency + 0.7
    cfg->tlc_margin_s    = 0.7f;
    cfg->tlc_max_dist_m  = 1.5f;
    cfg->min_lat_vel_mps = 0.10f;
    cfg->dashed_class    = 2;       // ufld: 0 = none, 1 = solid, 2 = dashed (confirmed on board 11/05)
    cfg->trigger_frames  = 2;
    cfg->hold_s          = 1.0f;
    cfg->lost_timeout_s  = 0.5f;
}

void nc_ldw_init(stNcLdwContext *ctx, const stNcLdwConfig *cfg)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->cfg = *cfg;
    ctx->res.state = NC_LDW_STATE_INVALID;
    ctx->res.tlc_s = -1.0f;
    ctx->dashed_score[0] = -1.0f;
    ctx->dashed_score[1] = -1.0f;
}

void nc_ldw_set_turn_signal(stNcLdwContext *ctx, int left_on, int right_on)
{
    ctx->turn_left = left_on ? 1 : 0;
    ctx->turn_right = right_on ? 1 : 0;
}

static void ldw_reset_tracking(stNcLdwContext *ctx)
{
    ctx->has_last = 0;
    ctx->left_cnt = 0;
    ctx->right_cnt = 0;
    ctx->warn_until_us = 0;
    ctx->res.lat_vel_mps = 0.0f;
    ctx->res.tlc_s = -1.0f;
}

static void ldw_check_lost(stNcLdwContext *ctx, uint64_t now_us)
{
    if (ctx->res.state == NC_LDW_STATE_INVALID) return;
    if (now_us > ctx->last_wall_us &&
        us_to_s(now_us - ctx->last_wall_us) > ctx->cfg.lost_timeout_s) {
        ctx->res.state = NC_LDW_STATE_INVALID;
        ctx->res.level = NC_LDW_LEVEL_NONE;
        ctx->res.valid = 0;
        ldw_reset_tracking(ctx);
    }
}

void nc_ldw_suspend(stNcLdwContext *ctx)
{
    ctx->res.state = NC_LDW_STATE_INVALID;
    ctx->res.level = NC_LDW_LEVEL_NONE;
    ctx->res.valid = 0;
    ldw_reset_tracking(ctx);
}

static void ldw_update_calibration(stNcLdwContext *ctx, float aL, float bL, float aR, float bR)
{
    const stNcLdwConfig *c = &ctx->cfg;
    float k = aR - aL;
    if (k <= 1e-4f) return;

    float vp_y = (bL - bR) / k;
    if (vp_y < c->img_h * 0.05f || vp_y > c->ref_y_px - c->img_h * 0.05f) return;

    float w_ref = k * (c->ref_y_px - vp_y);
    if (w_ref < c->img_w * LDW_MIN_LANE_PX_RATIO || w_ref > c->img_w * LDW_MAX_LANE_PX_RATIO) return;

    if (!ctx->calib_valid) {
        ctx->calib_k = k;
        ctx->calib_vp_y = vp_y;
        ctx->calib_valid = 1;
    } else {
        ctx->calib_k    += LDW_CALIB_ALPHA * (k - ctx->calib_k);
        ctx->calib_vp_y += LDW_CALIB_ALPHA * (vp_y - ctx->calib_vp_y);
    }
}

int nc_ldw_update(stNcLdwContext *ctx, const stLaneDet *lanes, int lane_num,
                  uint64_t data_time_us, uint64_t now_us)
{
    const stNcLdwConfig *c = &ctx->cfg;
    stNcLdwResult *r = &ctx->res;
    float aL = 0.0f, bL = 0.0f, aR = 0.0f, bR = 0.0f;
    int left_ok = 0, right_ok = 0;
    const uint64_t t = data_time_us ? data_time_us : now_us;   // time of this lane data

    /* NULL = no new lane result since the last call: only check the timeout.
     * (Identical points are still a new result: UFLD x is quantised, so a
     *  steady drive often gives exactly the same points twice.) */
    if (lanes == NULL) {
        ldw_check_lost(ctx, now_us);
        return 0;
    }
    ctx->last_update_us = now_us;

    float max_rms = c->img_w * LDW_MAX_FIT_RMS_RATIO;
    float y = c->ref_y_px;

    /* Ego-lane boundaries are lane slots 1 and 2, but left/right is decided by
     * position, not by slot number: the line with the smaller x on the
     * reference row is the left one. A single line is left when it is left
     * of the camera column. */
    {
        const int slot[2] = { (int)NC_LDW_EGO_LANE.left_boundary_index,
                              (int)NC_LDW_EGO_LANE.right_boundary_index };
        int   ok[2] = {0, 0}, cls[2] = {-1, -1};
        float fa[2] = {0.0f, 0.0f}, fb[2] = {0.0f, 0.0f};

        for (int i = 0; i < 2; i++) {
            if (lane_num > slot[i] &&
                fit_lane_line(&lanes[slot[i]], c->fit_y_min_px, c->img_h, max_rms, &fa[i], &fb[i]) == 0) {
                ok[i] = 1;
                cls[i] = lanes[slot[i]].lane_class;
            }
        }

        int l = -1, rgt = -1;     // index (0/1) of the left / right line
        if (ok[0] && ok[1]) {
            int first_is_left = (fa[0] * y + fb[0]) <= (fa[1] * y + fb[1]);
            l   = first_is_left ? 0 : 1;
            rgt = first_is_left ? 1 : 0;
        } else if (ok[0] || ok[1]) {
            int i = ok[0] ? 0 : 1;
            if (fa[i] * y + fb[i] < c->cam_x_px) l = i;
            else                                  rgt = i;
        }

        if (l >= 0)   { left_ok = 1;  aL = fa[l];   bL = fb[l]; }
        if (rgt >= 0) { right_ok = 1; aR = fa[rgt]; bR = fb[rgt]; }
        r->left_class  = l >= 0 ? cls[l] : -1;
        r->right_class = rgt >= 0 ? cls[rgt] : -1;
    }

    /* lane_class flickers between solid and dashed on the same line, so the
     * warning level uses a vote over recent frames, not the current frame. */
    ldw_vote_dashed(ctx, 0, r->left_class);
    ldw_vote_dashed(ctx, 1, r->right_class);

    float xl = aL * y + bL;
    float xr = aR * y + bR;

    if (left_ok && right_ok) {
        float w = xr - xl;
        if (w < c->img_w * LDW_MIN_LANE_PX_RATIO || w > c->img_w * LDW_MAX_LANE_PX_RATIO) {
            left_ok = right_ok = 0;   // inconsistent pair
        } else {
            ldw_update_calibration(ctx, aL, bL, aR, bR);
        }
    }

    /* One line only: infer the other from the calibrated lane width
     * w(y) = k*(y - vp_y), i.e. x_other = x_known +/- k*(y - vp_y). */
    if (left_ok != right_ok) {
        if (!ctx->calib_valid) {
            left_ok = right_ok = 0;
        } else {
            float k = ctx->calib_k, vp = ctx->calib_vp_y;
            if (left_ok) { aR = aL + k; bR = bL - k * vp; }
            else         { aL = aR - k; bL = bR + k * vp; }
            xl = aL * y + bL;
            xr = aR * y + bR;
        }
    }

    if (!left_ok && !right_ok) {
        ldw_check_lost(ctx, now_us);
        return 1;
    }

    /* Both lines are 'lane_width_m' apart on this row -> metres per pixel. */
    float s = c->lane_width_m / (xr - xl);
    float half_vw = c->vehicle_width_m * 0.5f;
    float veh_x = c->cam_x_px - c->cam_offset_m / s;

    r->valid          = 1;
    r->left_detected  = left_ok;
    r->right_detected = right_ok;
    r->x_left_px      = xl;
    r->x_right_px     = xr;
    r->vehicle_x_px   = veh_x;
    r->m_per_px       = s;
    r->offset_m       = (veh_x - 0.5f * (xl + xr)) * s;
    r->dist_left_m    = (veh_x - xl) * s - half_vw;
    r->dist_right_m   = (xr - veh_x) * s - half_vw;
    r->vp_y_px        = ctx->calib_valid ? ctx->calib_vp_y : 0.0f;
    r->cam_height_m   = ctx->calib_valid ? c->lane_width_m / ctx->calib_k : 0.0f;
    r->left_a         = aL;
    r->left_b         = bL;
    r->right_a        = aR;
    r->right_b        = bR;
    r->data_time_us   = t;

    /* Lateral velocity from the offset change. A jump of about one lane width
     * means the network re-indexed the lanes (a line was crossed): remove the
     * jump so the velocity stays continuous, and restart the warning logic. */
    int lane_changed = 0;
    float dt = ctx->has_last && t > ctx->last_valid_us ? us_to_s(t - ctx->last_valid_us) : 0.0f;
    if (ctx->has_last && dt > 0.0f && dt <= LDW_MAX_DT_S) {
        float d = r->offset_m - ctx->last_offset_m;
        if (fabsf(d) > c->lane_width_m * 0.5f) {
            lane_changed = 1;
            d -= copysignf(c->lane_width_m, d);
            ctx->left_cnt = 0;
            ctx->right_cnt = 0;
            ctx->warn_until_us = 0;
            ctx->dashed_score[0] = -1.0f;   // the lines changed sides
            ctx->dashed_score[1] = -1.0f;
        }
        r->lat_vel_mps += LDW_VEL_ALPHA * (d / dt - r->lat_vel_mps);
    } else {
        r->lat_vel_mps = 0.0f;
    }
    ctx->has_last = 1;
    ctx->last_offset_m = r->offset_m;
    ctx->last_valid_us = t;
    ctx->last_wall_us = now_us;

    /* TLC threshold = reaction time + measured latency + margin.
     * The lane result is already 'latency' old when we see it, so the
     * driver has that much less time left: it is added to the threshold. */
    float latency_ms = 0.0f;
    // latency_ms = nc_meas_get_latency_ms();     // [TEAM]
    if (latency_ms < 0.0f) latency_ms = 0.0f;
    r->latency_ms   = latency_ms;
    r->tlc_thresh_s = c->reaction_time_s + latency_ms * 0.001f + c->tlc_margin_s;

    float dl = r->dist_left_m;
    float dr = r->dist_right_m;

    r->tlc_s = -1.0f;
    if (r->lat_vel_mps > c->min_lat_vel_mps)
        r->tlc_s = dr > 0.0f ? dr / r->lat_vel_mps : 0.0f;
    else if (r->lat_vel_mps < -c->min_lat_vel_mps)
        r->tlc_s = dl > 0.0f ? dl / -r->lat_vel_mps : 0.0f;

    /* Warning decision.
     * A side warns when the vehicle is close to (or over) that line and not
     * moving away from it, or when TLC < tlc_thresh_s while approaching it.
     * trigger_frames debounces the start; hold_s is the minimum display time. */
    int moving_left  = r->lat_vel_mps < -c->min_lat_vel_mps;
    int moving_right = r->lat_vel_mps >  c->min_lat_vel_mps;
    int cond_l = !ctx->turn_left && !lane_changed &&
                 ((dl < c->warn_margin_m && !moving_right) ||
                  (moving_left && r->tlc_s >= 0.0f && r->tlc_s < r->tlc_thresh_s &&
                   dl < c->tlc_max_dist_m));
    int cond_r = !ctx->turn_right && !lane_changed &&
                 ((dr < c->warn_margin_m && !moving_left) ||
                  (moving_right && r->tlc_s >= 0.0f && r->tlc_s < r->tlc_thresh_s &&
                   dr < c->tlc_max_dist_m));

    ctx->left_cnt  = cond_l ? ctx->left_cnt + 1 : 0;
    ctx->right_cnt = cond_r ? ctx->right_cnt + 1 : 0;

    int fire_l = ctx->left_cnt  >= c->trigger_frames;
    int fire_r = ctx->right_cnt >= c->trigger_frames;
    eNcLdwState next = NC_LDW_STATE_NORMAL;

    if (fire_l && fire_r)
        next = dl <= dr ? NC_LDW_STATE_WARN_LEFT : NC_LDW_STATE_WARN_RIGHT;
    else if (fire_l)
        next = NC_LDW_STATE_WARN_LEFT;
    else if (fire_r)
        next = NC_LDW_STATE_WARN_RIGHT;
    else if (!lane_changed && t < ctx->warn_until_us &&
             (r->state == NC_LDW_STATE_WARN_LEFT || r->state == NC_LDW_STATE_WARN_RIGHT))
        next = r->state;   /* minimum display time */

    int prev_warn = (r->state == NC_LDW_STATE_WARN_LEFT || r->state == NC_LDW_STATE_WARN_RIGHT);
    int warn      = (next == NC_LDW_STATE_WARN_LEFT || next == NC_LDW_STATE_WARN_RIGHT);

    if (warn && next != r->state)
        ctx->warn_until_us = t + s_to_us(c->hold_s);   // new warning or side switch

    /* Recording: only when the warning turns on (off -> on), not every frame
     * and not on a left <-> right switch. */
    if (!prev_warn && warn) {
        // nc_rec_trigger(NC_REC_TYPE_LANE_DEPARTURE);   // [TEAM]
    }
    r->state = next;

    /* Strength: crossing a dashed line is allowed (lane change) -> weak alert,
     * solid or unknown line -> strong warning. */
    if (next == NC_LDW_STATE_WARN_LEFT)
        r->level = ctx->dashed_score[0] >= 0.5f ? NC_LDW_LEVEL_WEAK : NC_LDW_LEVEL_STRONG;
    else if (next == NC_LDW_STATE_WARN_RIGHT)
        r->level = ctx->dashed_score[1] >= 0.5f ? NC_LDW_LEVEL_WEAK : NC_LDW_LEVEL_STRONG;
    else
        r->level = NC_LDW_LEVEL_NONE;

    return 1;
}

int nc_ldw_pixel_to_road(const stNcLdwContext *ctx, float u, float v, float *x_m, float *z_m)
{
    const stNcLdwConfig *c = &ctx->cfg;
    if (!ctx->calib_valid) return -1;

    float w = ctx->calib_k * (v - ctx->calib_vp_y);   // lane width in px on row v
    if (w <= 1.0f) return -1;

    float s = c->lane_width_m / w;                     // metres per pixel on row v
    if (x_m) *x_m = (u - c->cam_x_px) * s + c->cam_offset_m;
    if (z_m) *z_m = c->focal_px > 0.0f ? c->focal_px * s : -1.0f;
    return 0;
}

const char *nc_ldw_state_name(eNcLdwState state)
{
    switch (state) {
    case NC_LDW_STATE_NORMAL:     return "NORMAL";
    case NC_LDW_STATE_WARN_LEFT:  return "WARN_LEFT";
    case NC_LDW_STATE_WARN_RIGHT: return "WARN_RIGHT";
    default:                      return "INVALID";
    }
}

/*
********************************************************************************
*               APPLICATION LAYER (app_wayland_npu)
********************************************************************************
*/
#ifdef USE_LDW
#include <pthread.h>
#include "nc_bevlog.h"

static stNcLdwContext g_ldw_ctx[NC_LDW_MAX_CH];     // render thread only
static int g_ldw_ch_num = 0;
static float g_ldw_canvas_w = 0.0f;
static float g_ldw_canvas_h = 0.0f;
static int g_ldw_suspended[NC_LDW_MAX_CH];

/* New-result detection: the lane result buffer is a flip-flop (2 buffers),
 * so a new result moves the readable pointer. The timestamp also changes once
 * the input timestamp is passed through. */
static const void *g_ldw_last_buf[NC_LDW_MAX_CH];
static uint64_t g_ldw_last_ts[NC_LDW_MAX_CH];

/* Ego-lane snapshot shared with other threads (cut-in prediction). */
typedef struct {
    int      valid;
    float    left_a, left_b, right_a, right_b;
    float    vp_y;
    uint64_t time_us;
    int      calib_valid;       // pixel -> road conversion available
    float    calib_k;
} stLdwEgoSnapshot;

static stLdwEgoSnapshot g_ldw_ego[NC_LDW_MAX_CH];
static pthread_mutex_t g_ldw_ego_lock = PTHREAD_MUTEX_INITIALIZER;

/* Road geometry of recent lane frames, looked up by capture time so that a
 * detection uses the horizon of its own frame (guarded by g_ldw_ego_lock). */
#define LDW_GEOM_RING   (16)
typedef struct {
    stNcLdwGeom g[LDW_GEOM_RING];
    int         head;           // next write index
    int         cnt;
} stLdwGeomRing;
static stLdwGeomRing g_ldw_geom[NC_LDW_MAX_CH];

/* latest lane points (guarded by g_ldw_ego_lock) */
static stNcLdwLanes g_ldw_lanes[NC_LDW_MAX_CH];

static void ldw_publish_geom(uint32_t ch, const stNcLdwContext *ctx, const pp_result_buf *lane_buf)
{
    if (lane_buf == NULL || lane_buf->time_stamp == 0 || !ctx->calib_valid) return;

    stNcLdwGeom g;
    g.t_us = lane_buf->time_stamp;
    g.k = ctx->calib_k;
    g.vp_y = ctx->calib_vp_y;                 // calibrated (slow EMA) horizon

    /* no line fitted this time -> res still holds the previous result */
    const stNcLdwResult *r = &ctx->res;
    int fresh = r->valid && r->data_time_us == lane_buf->time_stamp;
    g.left_ok  = fresh && r->left_detected;
    g.right_ok = fresh && r->right_detected;
    g.k_inst   = (g.left_ok && g.right_ok) ? r->right_a - r->left_a : 0.0f;
    g.vp_inst  = g.k_inst > 1e-4f ? (r->left_b - r->right_b) / g.k_inst : 0.0f;
    if (g.k_inst <= 1e-4f) g.k_inst = 0.0f;

    int n = lane_buf->lane_draw_info.max_lane_num;
    if (n > MAX_LANE_DET_CNT) n = MAX_LANE_DET_CNT;
    if (n < 0) n = 0;

    pthread_mutex_lock(&g_ldw_ego_lock);
    stLdwGeomRing *rg = &g_ldw_geom[ch];
    int last = (rg->head + LDW_GEOM_RING - 1) % LDW_GEOM_RING;
    if (rg->cnt > 0 && rg->g[last].t_us == g.t_us) {
        rg->g[last] = g;                                  // same frame again: refresh
    } else {
        rg->g[rg->head] = g;
        rg->head = (rg->head + 1) % LDW_GEOM_RING;
        if (rg->cnt < LDW_GEOM_RING) rg->cnt++;
    }
    g_ldw_lanes[ch].t_us = g.t_us;
    g_ldw_lanes[ch].lane_num = n;
    memcpy(g_ldw_lanes[ch].lane, lane_buf->cnn_result.lane_det, sizeof(stLaneDet) * (size_t)n);
    pthread_mutex_unlock(&g_ldw_ego_lock);
}

/* lane_class of lane i in the buffer (-1: no points / no buffer), diagnostics */
static int ldw_lane_class(const pp_result_buf *b, int i)
{
    if (b == NULL || i >= b->lane_draw_info.max_lane_num || i >= MAX_LANE_DET_CNT) return -1;
    return b->cnn_result.lane_det[i].point_cnt > 0 ? b->cnn_result.lane_det[i].lane_class : -1;
}

static uint64_t ldw_now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000u + (uint64_t)ts.tv_nsec / 1000u;
}

/* SKIP mode: the lane network is not running, so there are no new lanes. */
static int ldw_model_skip(void)
{
    // return nc_model_get_mode() == MODE_SKIP;   // [TEAM]
    return 0;
}

static void ldw_publish_ego(uint32_t ch, const stNcLdwContext *ctx)
{
    const stNcLdwResult *r = &ctx->res;

    pthread_mutex_lock(&g_ldw_ego_lock);
    g_ldw_ego[ch].valid   = (r->valid && r->state != NC_LDW_STATE_INVALID && ctx->calib_valid) ? 1 : 0;
    g_ldw_ego[ch].left_a  = r->left_a;
    g_ldw_ego[ch].left_b  = r->left_b;
    g_ldw_ego[ch].right_a = r->right_a;
    g_ldw_ego[ch].right_b = r->right_b;
    g_ldw_ego[ch].vp_y    = ctx->calib_vp_y;
    g_ldw_ego[ch].time_us = r->data_time_us;
    g_ldw_ego[ch].calib_valid = ctx->calib_valid;
    g_ldw_ego[ch].calib_k     = ctx->calib_k;
    pthread_mutex_unlock(&g_ldw_ego_lock);
}

#if NC_LDW_LOG_LANE_CLASS
static void ldw_log_lane_class(uint32_t ch, const pp_result_buf *lane_buf, uint64_t now_us)
{
    static uint64_t last_us[NC_LDW_MAX_CH];
    if (now_us - last_us[ch] < 1000000u) return;
    last_us[ch] = now_us;

    const stLaneDet *ln = lane_buf->cnn_result.lane_det;
    int n = lane_buf->lane_draw_info.max_lane_num;
    if (n > MAX_LANE_DET_CNT) n = MAX_LANE_DET_CNT;

    printf("[LDW] ch%u lane_class:", ch);
    for (int i = 0; i < n; i++) {
        if (ln[i].point_cnt > 0) {
            // bottom-most point x, to check which slot is left/right
            int b = 0;
            for (int j = 1; j < ln[i].point_cnt && j < MAX_POINT_CNT_OF_LANE; j++)
                if (ln[i].point[j].y > ln[i].point[b].y) b = j;
            printf(" [%d]class=%d(x=%.0f)", i, ln[i].lane_class, (double)ln[i].point[b].x);
        } else {
            printf(" [%d]-", i);
        }
    }
    printf("\n");
}
#endif

/* Board tuning without a rebuild: environment variables override the
 * defaults, e.g.  LDW_WARN_MARGIN=0.6 ./run_app_wayland_npu.sh  */
static void ldw_env_float(const char *name, float *v)
{
    const char *e = getenv(name);
    char *end = NULL;
    if (!e || !*e) return;
    float f = strtof(e, &end);
    if (end != e) *v = f;
    else printf("[LDW] ignoring %s=%s (not a number)\n", name, e);
}

static void ldw_env_int(const char *name, int *v)
{
    float f = (float)*v;
    ldw_env_float(name, &f);
    *v = (int)lroundf(f);
}

static void ldw_config_from_env(stNcLdwConfig *cfg)
{
    ldw_env_float("LDW_WARN_MARGIN",  &cfg->warn_margin_m);
    ldw_env_float("LDW_REACTION",     &cfg->reaction_time_s);
    ldw_env_float("LDW_TLC_MARGIN",   &cfg->tlc_margin_s);
    ldw_env_float("LDW_TLC_MAX_DIST", &cfg->tlc_max_dist_m);
    ldw_env_float("LDW_MIN_VEL",      &cfg->min_lat_vel_mps);
    ldw_env_float("LDW_VEHICLE_WIDTH",&cfg->vehicle_width_m);
    ldw_env_float("LDW_FOCAL_PX",     &cfg->focal_px);
    ldw_env_float("LDW_REF_Y",        &cfg->ref_y_px);
    ldw_env_float("LDW_CAM_X",        &cfg->cam_x_px);
    ldw_env_float("LDW_HOLD",         &cfg->hold_s);
    ldw_env_int  ("LDW_TRIGGER_FRAMES", &cfg->trigger_frames);
    ldw_env_int  ("LDW_DASHED_CLASS",   &cfg->dashed_class);
    if (cfg->trigger_frames < 1) cfg->trigger_frames = 1;
}

void nc_ldw_app_init(int ch_num, float canvas_w, float canvas_h)
{
    stNcLdwConfig cfg;

    if (ch_num > NC_LDW_MAX_CH) ch_num = NC_LDW_MAX_CH;
    if (ch_num < 0) ch_num = 0;
    g_ldw_ch_num = ch_num;
    g_ldw_canvas_w = canvas_w;
    g_ldw_canvas_h = canvas_h;

    nc_ldw_default_config(&cfg, canvas_w, canvas_h);
    ldw_config_from_env(&cfg);
    printf("[LDW] config: warn_margin=%.2fm tlc<%.2f+lat+%.2fs (dist<%.2fm, vel>%.2fm/s) "
           "trigger=%d frames ref_y=%.0f cam_x=%.0f dashed_class=%d focal=%.1fpx\n",
           (double)cfg.warn_margin_m, (double)cfg.reaction_time_s, (double)cfg.tlc_margin_s,
           (double)cfg.tlc_max_dist_m, (double)cfg.min_lat_vel_mps, cfg.trigger_frames,
           (double)cfg.ref_y_px, (double)cfg.cam_x_px, cfg.dashed_class, (double)cfg.focal_px);
    for (int i = 0; i < ch_num; i++) {
        nc_ldw_init(&g_ldw_ctx[i], &cfg);
        g_ldw_suspended[i] = 0;
        g_ldw_last_buf[i] = NULL;
        g_ldw_last_ts[i] = 0;
    }
    /* constants needed to recompute road positions offline */
    nc_bevlog_row(NC_BEVLOG_LANE, "# cfg canvas_w=%.0f canvas_h=%.0f focal_px=%.3f lane_width_m=%.3f cam_x_px=%.2f "
                  "cam_offset_m=%.3f ref_y_px=%.1f fit_y_min_px=%.1f calib_alpha=%.3f",
                  (double)canvas_w, (double)canvas_h, (double)cfg.focal_px, (double)cfg.lane_width_m,
                  (double)cfg.cam_x_px, (double)cfg.cam_offset_m, (double)cfg.ref_y_px,
                  (double)cfg.fit_y_min_px, (double)LDW_CALIB_ALPHA);

    pthread_mutex_lock(&g_ldw_ego_lock);
    memset(g_ldw_ego, 0, sizeof(g_ldw_ego));
    memset(g_ldw_geom, 0, sizeof(g_ldw_geom));
    memset(g_ldw_lanes, 0, sizeof(g_ldw_lanes));
    pthread_mutex_unlock(&g_ldw_ego_lock);
}

void nc_ldw_app_process(uint32_t ch, const pp_result_buf *lane_buf)
{
    if ((int)ch >= g_ldw_ch_num) return;

    stNcLdwContext *ctx = &g_ldw_ctx[ch];
    eNcLdwState prev = ctx->res.state;
    uint64_t now_us = ldw_now_us();

    /* SKIP mode: no lane results -> stop LDW (no warning, no recording).
     * The "LDW OFF" text is drawn by the model-switch module. */
    if (ldw_model_skip()) {
        if (!g_ldw_suspended[ch]) {
            nc_ldw_suspend(ctx);
            g_ldw_suspended[ch] = 1;
            ldw_publish_ego(ch, ctx);
            printf("[LDW] ch%u suspended (model SKIP mode)\n", ch);
        }
        return;
    }
    if (g_ldw_suspended[ch]) {
        /* back from SKIP: the buffer may still hold the lane result from before
         * SKIP; its pointer was already seen, so it is ignored until new data comes */
        g_ldw_suspended[ch] = 0;
        printf("[LDW] ch%u resumed\n", ch);
    }

    int new_data = 0;
    int is_new = lane_buf != NULL &&
                 ((const void *)lane_buf != g_ldw_last_buf[ch] || lane_buf->time_stamp != g_ldw_last_ts[ch]);

    if (is_new) {
        g_ldw_last_buf[ch] = lane_buf;
        g_ldw_last_ts[ch] = lane_buf->time_stamp;
#if NC_LDW_LOG_LANE_CLASS
        ldw_log_lane_class(ch, lane_buf, now_us);
#endif
        // time_stamp stays 0 until the input timestamp is passed through (then it is used)
        new_data = nc_ldw_update(ctx, lane_buf->cnn_result.lane_det, lane_buf->lane_draw_info.max_lane_num,
                                 lane_buf->time_stamp, now_us);
    } else {
        nc_ldw_update(ctx, NULL, 0, 0, now_us);    // no new result: timeout check only
    }
    ldw_publish_ego(ch, ctx);
    if (new_data) ldw_publish_geom(ch, ctx, lane_buf);

    const stNcLdwResult *r = &ctx->res;

    /* diagnostics: one row per new lane result. k/vp_inst only when both lines were measured. */
    if (new_data && nc_bevlog_enabled()) {
        /* no line fitted this time -> r still holds the previous result: log as not detected */
        int fresh = lane_buf != NULL && r->valid && r->data_time_us == lane_buf->time_stamp;
        int both = fresh && r->left_detected && r->right_detected;
        float k_inst = both ? r->right_a - r->left_a : 0.0f;
        float vp_inst = (both && k_inst > 1e-4f) ? (r->left_b - r->right_b) / k_inst : 0.0f;
        nc_bevlog_row(NC_BEVLOG_LANE,
                      "%llu,%llu,%u,%d,%d,%.5f,%.2f,%.5f,%.2f,%.5f,%.2f,%.5f,%.2f,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%llu,%.1f,"
                      "%d,%d,%d,%d",
                      (unsigned long long)now_us, (unsigned long long)(lane_buf ? lane_buf->time_stamp : 0), ch,
                      fresh ? r->left_detected : 0, fresh ? r->right_detected : 0,
                      (double)r->left_a, (double)r->left_b, (double)r->right_a, (double)r->right_b,
                      (double)k_inst, (double)vp_inst,
                      (double)ctx->calib_k, (double)ctx->calib_vp_y,
                      (int)r->state, (double)r->dist_left_m, (double)r->dist_right_m,
                      (double)r->lat_vel_mps, (double)r->tlc_s, (double)r->tlc_thresh_s,
                      ctx->left_cnt, ctx->right_cnt, (unsigned long long)ctx->warn_until_us,
                      (double)r->latency_ms,
                      ldw_lane_class(lane_buf, 0), ldw_lane_class(lane_buf, 1),
                      ldw_lane_class(lane_buf, 2), ldw_lane_class(lane_buf, 3));
    }
    if (r->state != prev) {
        printf("[LDW] ch%u %s -> %s%s (left %.2fm, right %.2fm, offset %+.2fm, vel %+.2fm/s, "
               "tlc %.2fs < %.2fs?, latency %.0fms, class L%d R%d, dashed vote L%.2f R%.2f)\n",
               ch, nc_ldw_state_name(prev), nc_ldw_state_name(r->state),
               r->level == NC_LDW_LEVEL_WEAK ? "(weak)" : r->level == NC_LDW_LEVEL_STRONG ? "(strong)" : "",
               (double)r->dist_left_m, (double)r->dist_right_m, (double)r->offset_m,
               (double)r->lat_vel_mps, (double)r->tlc_s, (double)r->tlc_thresh_s,
               (double)r->latency_ms, r->left_class, r->right_class,
               (double)ctx->dashed_score[0], (double)ctx->dashed_score[1]);
    }
}

int nc_ldw_get_ego_lane(uint32_t ch, float y, float *x_left, float *x_right, uint64_t *time_us)
{
    stLdwEgoSnapshot e;

    if (ch >= NC_LDW_MAX_CH) return -1;

    pthread_mutex_lock(&g_ldw_ego_lock);
    e = g_ldw_ego[ch];
    pthread_mutex_unlock(&g_ldw_ego_lock);

    if (!e.valid) return -1;
    if (y <= e.vp_y + 1.0f) return -1;          // at/above the horizon: no lane width

    float xl = e.left_a * y + e.left_b;
    float xr = e.right_a * y + e.right_b;
    if (xr <= xl) return -1;

    if (x_left)  *x_left = xl;
    if (x_right) *x_right = xr;
    if (time_us) *time_us = e.time_us;
    return 0;
}

int nc_ldw_is_in_ego_lane(uint32_t ch, float x, float y)
{
    float xl, xr;
    if (nc_ldw_get_ego_lane(ch, y, &xl, &xr, NULL) != 0) return -1;
    return (x >= xl && x <= xr) ? 1 : 0;
}

int nc_ldw_app_get_geom(uint32_t ch, uint64_t t_us, stNcLdwGeom *g)
{
    if ((int)ch >= g_ldw_ch_num || g == NULL) return -1;

    int found = -1;
    pthread_mutex_lock(&g_ldw_ego_lock);
    const stLdwGeomRing *rg = &g_ldw_geom[ch];
    if (rg->cnt > 0) {
        int newest = (rg->head + LDW_GEOM_RING - 1) % LDW_GEOM_RING;
        if (t_us == 0) {
            *g = rg->g[newest];
            found = 0;
        } else {
            uint64_t best = UINT64_MAX;
            for (int i = 0; i < rg->cnt; i++) {
                const stNcLdwGeom *e = &rg->g[(newest + LDW_GEOM_RING - i) % LDW_GEOM_RING];
                uint64_t d = e->t_us > t_us ? e->t_us - t_us : t_us - e->t_us;
                if (d < best) { best = d; *g = *e; found = (d == 0) ? 0 : 1; }
            }
        }
    }
    pthread_mutex_unlock(&g_ldw_ego_lock);
    return found;
}

int nc_ldw_geom_to_road(uint32_t ch, const stNcLdwGeom *g, float u, float v, float *x_m, float *z_m)
{
    if ((int)ch >= g_ldw_ch_num || g == NULL || g->k <= 0.0f) return -1;

    /* cfg is fixed after nc_ldw_app_init(), safe to read from any thread */
    const stNcLdwConfig *c = &g_ldw_ctx[ch].cfg;

    float w = g->k * (v - g->vp_y);                    // lane width in px on row v
    if (w <= 1.0f) return -1;

    float s = c->lane_width_m / w;                     // metres per pixel on row v
    if (x_m) *x_m = (u - c->cam_x_px) * s + c->cam_offset_m;
    if (z_m) *z_m = c->focal_px > 0.0f ? c->focal_px * s : -1.0f;
    return 0;
}

int nc_ldw_app_pixel_to_road(uint32_t ch, float u, float v, float *x_m, float *z_m)
{
    stNcLdwGeom g;
    if (nc_ldw_app_get_geom(ch, 0, &g) < 0) return -1;
    return nc_ldw_geom_to_road(ch, &g, u, v, x_m, z_m);
}

float nc_ldw_app_focal_px(uint32_t ch)
{
    if ((int)ch >= g_ldw_ch_num) return 0.0f;
    return g_ldw_ctx[ch].cfg.focal_px;          // fixed after nc_ldw_app_init()
}

int nc_ldw_app_get_lanes(uint32_t ch, stNcLdwLanes *out)
{
    if ((int)ch >= g_ldw_ch_num || out == NULL) return -1;
    pthread_mutex_lock(&g_ldw_ego_lock);
    *out = g_ldw_lanes[ch];
    pthread_mutex_unlock(&g_ldw_ego_lock);
    return out->t_us != 0 ? 0 : -1;
}

int nc_ldw_app_get_calib(uint32_t ch, float *k, float *vp_y, uint64_t *lane_time_us)
{
    stLdwEgoSnapshot e;

    if ((int)ch >= g_ldw_ch_num) return -1;

    pthread_mutex_lock(&g_ldw_ego_lock);
    e = g_ldw_ego[ch];
    pthread_mutex_unlock(&g_ldw_ego_lock);

    if (!e.calib_valid) return -1;
    if (k) *k = e.calib_k;
    if (vp_y) *vp_y = e.vp_y;
    if (lane_time_us) *lane_time_us = e.time_us;
    return 0;
}

void nc_ldw_app_draw(uint32_t ch, struct viewport vp, const stNcLdwDrawRes *res)
{
    if ((int)ch >= g_ldw_ch_num || res == NULL || vp.height <= 0) return;
    if (g_ldw_suspended[ch]) return;     // SKIP mode: "LDW OFF" is drawn elsewhere

    const stNcLdwContext *ctx = &g_ldw_ctx[ch];
    const stNcLdwResult *r = &ctx->res;
    const float cw = g_ldw_canvas_w;
    const float chh = g_ldw_canvas_h;
    const int sw = (int)cw;
    const int sh = (int)chh;
    float text_scale = chh / (float)vp.height;
    char buftext[160];

    glViewport(vp.x, vp.y, vp.width, vp.height);

    if (!r->valid) {
        float gray[3] = {0.7f, 0.7f, 0.7f};
        nc_opengl_draw_text(res->font_small, "LDW: no lane", 10.0f, chh - 110.0f,
                            text_scale, gray, sw, sh, *res->font_prog);
        return;
    }

    int warn = (r->state == NC_LDW_STATE_WARN_LEFT || r->state == NC_LDW_STATE_WARN_RIGHT);
    float y = ctx->cfg.ref_y_px;

    // measurement row between both lines and the vehicle centre marker
    float row_color[4] = {1.0f, 1.0f, 0.0f, 1.0f};
    float veh_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    nc_opengl_draw_line(r->x_left_px, y, r->x_right_px, y, 1, row_color, *res->npu_prog);
    nc_opengl_draw_rectangle(r->vehicle_x_px - 8.0f, y - 8.0f, 16.0f, 16.0f, veh_color, *res->npu_prog);

    // distances
    float info_color[3] = {1.0f, 1.0f, 1.0f};
    if (r->tlc_s >= 0.0f)
        snprintf(buftext, sizeof(buftext), "LDW  L %.2fm  R %.2fm  v %+.2fm/s  TLC %.1fs (warn<%.1fs, lat %.0fms)",
                 (double)r->dist_left_m, (double)r->dist_right_m, (double)r->lat_vel_mps,
                 (double)r->tlc_s, (double)r->tlc_thresh_s, (double)r->latency_ms);
    else
        snprintf(buftext, sizeof(buftext), "LDW  L %.2fm  R %.2fm  v %+.2fm/s  TLC -  (warn<%.1fs, lat %.0fms)",
                 (double)r->dist_left_m, (double)r->dist_right_m, (double)r->lat_vel_mps,
                 (double)r->tlc_thresh_s, (double)r->latency_ms);
    nc_opengl_draw_text(res->font_small, buftext, 10.0f, chh - 110.0f,
                        text_scale, info_color, sw, sh, *res->font_prog);

    if (warn) {
        /* strong (solid / unknown line): thick red frame
         * weak   (dashed line)         : thin yellow frame */
        int strong = (r->level != NC_LDW_LEVEL_WEAK);
        float col4[4] = {1.0f, strong ? 0.0f : 0.85f, 0.0f, 1.0f};
        float col3[3] = {col4[0], col4[1], col4[2]};
        int frames = strong ? 4 : 2;

        for (int i = 0; i < frames; i++) {
            float m = 4.0f + (float)i * 4.0f;
            nc_opengl_draw_rectangle(m, m, cw - 2.0f * m, chh - 2.0f * m, col4, *res->npu_prog);
        }

        const char *msg;
        if (r->state == NC_LDW_STATE_WARN_LEFT)
            msg = strong ? "<<< LANE DEPARTURE LEFT" : "<<< LEFT (dashed)";
        else
            msg = strong ? "LANE DEPARTURE RIGHT >>>" : "RIGHT (dashed) >>>";
        nc_opengl_draw_text(res->font_large, msg, cw * 0.5f - 300.0f, chh - 180.0f,
                            text_scale, col3, sw, sh, *res->font_prog);
    }
}

#endif /* USE_LDW */
