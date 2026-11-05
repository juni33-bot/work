#ifndef __NC_LDW_H__
#define __NC_LDW_H__

/*
 * Lane Departure Warning (LDW)
 *
 * Input : ego-lane boundaries from the UFLD lane network (lane_det[1] = left,
 *         lane_det[2] = right), in canvas pixel coordinates (y grows downward).
 * Model : flat road + pinhole camera. The ego-lane width in pixels grows
 *         linearly below the vanishing row:  w(y) = k * (y - vp_y).
 *         With an assumed real lane width W (3.5 m), one pixel on row y is
 *         W / w(y) metres wide, so lateral positions can be measured in metres
 *         without camera calibration. (Camera height = W / k comes for free.)
 */

#include <stdint.h>
#include "nc_cnn_common.h"

typedef enum {
    NC_LDW_LEFT_EGO_LANE_BOUNDARY_INDEX = 1,
    NC_LDW_RIGHT_EGO_LANE_BOUNDARY_INDEX = 2
} eNcLdwEgoLaneBoundaryIndex;

typedef struct {
    eNcLdwEgoLaneBoundaryIndex left_boundary_index;
    eNcLdwEgoLaneBoundaryIndex right_boundary_index;
} stNcLdwEgoLane;

extern const stNcLdwEgoLane NC_LDW_EGO_LANE;

#define NC_LDW_DEFAULT_LANE_WIDTH_M     (3.5f)

typedef enum {
    NC_LDW_STATE_INVALID = 0,   // ego lane not available
    NC_LDW_STATE_NORMAL,        // inside the lane
    NC_LDW_STATE_WARN_LEFT,     // departing over the left line
    NC_LDW_STATE_WARN_RIGHT     // departing over the right line
} eNcLdwState;

typedef struct {
    float img_w;                // canvas width of the lane points (px)
    float img_h;                // canvas height of the lane points (px)

    float lane_width_m;         // assumed lane width (3.5 m)
    float vehicle_width_m;      // vehicle body width
    float cam_x_px;             // image column of the camera optical axis
    float cam_offset_m;         // camera mount position from vehicle centre (+: right)

    float ref_y_px;             // evaluation row (just above the hood)
    float fit_y_min_px;         // only points below this row are used to fit lines
    float focal_px;             // focal length in px (optional, <= 0: unknown)

    float warn_margin_m;        // warn when vehicle side is closer than this to a line

    /* TLC warning: TLC < reaction_time_s + measured latency + tlc_margin_s */
    float reaction_time_s;      // driver reaction time
    float tlc_margin_s;         // extra safety margin
    float tlc_max_dist_m;       // TLC warning only when closer than this
    float min_lat_vel_mps;      // lateral speed needed for a TLC warning
    int   trigger_frames;       // consecutive frames needed to raise a warning
    float hold_s;               // keep the warning at least this long
    float lost_timeout_s;       // lanes missing longer than this -> INVALID
} stNcLdwConfig;

typedef struct {
    eNcLdwState state;
    int   valid;                // 1: ego lane geometry available
    int   left_detected;        // 1: left line detected, 0: inferred from width
    int   right_detected;

    float x_left_px;            // left/right line columns at ref_y_px
    float x_right_px;
    float vehicle_x_px;         // vehicle centre column at ref_y_px
    float m_per_px;             // metres per pixel at ref_y_px

    float offset_m;             // vehicle centre - lane centre (+: right of centre)
    float dist_left_m;          // vehicle left side to left line (<0: crossed)
    float dist_right_m;         // vehicle right side to right line (<0: crossed)
    float lat_vel_mps;          // lateral velocity (+: moving right)
    float tlc_s;                // time to line crossing (<0: not approaching)
    float tlc_thresh_s;         // reaction_time_s + latency + tlc_margin_s

    /* ego-lane boundaries as lines x = a*y + b (canvas px), for any row y */
    float left_a, left_b;
    float right_a, right_b;
    uint64_t data_time_us;      // time of the lane data used for this result

    float vp_y_px;              // calibrated vanishing row
    float cam_height_m;         // estimated camera height (sanity check)
    float latency_ms;           // processing latency used for the warning decision
} stNcLdwResult;

typedef struct {
    stNcLdwConfig cfg;
    stNcLdwResult res;

    // calibration: w(y) = k * (y - vp_y)
    int   calib_valid;
    float calib_k;
    float calib_vp_y;

    // temporal state
    uint32_t last_sig;
    int      has_last;
    float    last_offset_m;
    uint64_t last_update_us;
    uint64_t last_valid_us;     // data time of the last valid result (velocity)
    uint64_t last_wall_us;      // wall time of the last valid result (lost timeout)
    int      left_cnt;
    int      right_cnt;
    uint64_t warn_until_us;
    int      turn_left;
    int      turn_right;
} stNcLdwContext;

void nc_ldw_default_config(stNcLdwConfig *cfg, float img_w, float img_h);
void nc_ldw_init(stNcLdwContext *ctx, const stNcLdwConfig *cfg);

/* Lane departure calculation. Feed one lane result.
 * data_time_us: capture time of the lane data (0 = unknown, now_us is used)
 * now_us      : current CLOCK_MONOTONIC time in us
 * Returns 1 when new lane data was processed, 0 when the data was the same
 * as last time (result kept). */
int nc_ldw_update(stNcLdwContext *ctx, const stLaneDet *lanes, int lane_num,
                  uint64_t data_time_us, uint64_t now_us);

/* Stop LDW while no lane results are available (e.g. model SKIP mode):
 * clears the warning and restarts velocity tracking. Calibration is kept. */
void nc_ldw_suspend(stNcLdwContext *ctx);

/* Suppress warnings on the side the driver is signalling. */
void nc_ldw_set_turn_signal(stNcLdwContext *ctx, int left_on, int right_on);

/* Road-plane position of pixel (u, v) using the lane-width scale.
 * x_m: lateral distance from vehicle centre (+: right).
 * z_m: forward distance from the camera (needs cfg.focal_px > 0, else -1).
 * Returns 0 on success, -1 if not calibrated or v is above the horizon. */
int nc_ldw_pixel_to_road(const stNcLdwContext *ctx, float u, float v, float *x_m, float *z_m);

const char *nc_ldw_state_name(eNcLdwState state);

/*
 * Application layer (built when USE_LDW is defined, e.g. app_wayland_npu).
 * Keeps one LDW context per camera channel, prints state changes and draws
 * the overlay. The app only calls these three functions.
 */
#ifdef USE_LDW
#include "nc_opengl_interface.h"
#include "nc_opengl_ttf_font.h"
#include "nc_cnn_aiware_runtime.h"

#define NC_LDW_MAX_CH   (8)

typedef struct {
    Font *font_small;
    Font *font_large;
    struct gl_font_program *font_prog;
    struct gl_npu_program *npu_prog;
} stNcLdwDrawRes;

/* ch_num channels, lane points in canvas_w x canvas_h coordinates */
void nc_ldw_app_init(int ch_num, float canvas_w, float canvas_h);

/* Call while the lane result buffer is locked. lane_buf may be NULL. */
void nc_ldw_app_process(uint32_t ch, const pp_result_buf *lane_buf);

/* Draw the LDW overlay of one channel into its viewport. */
void nc_ldw_app_draw(uint32_t ch, struct viewport vp, const stNcLdwDrawRes *res);

/*
 * Ego-lane area for other modules (cut-in prediction). Thread-safe.
 * Coordinates are canvas px (same as lane/bbox results), y grows downward.
 *
 * nc_ldw_get_ego_lane(): left/right boundary x on row y.
 *   time_us (optional): data time of the lane result, to pick the closest one.
 *   Returns 0 on success, -1 if no ego lane (no lanes, SKIP mode, y above horizon).
 *
 * nc_ldw_is_in_ego_lane(): 1 = (x, y) inside the ego lane, 0 = outside,
 *   -1 = unknown (no ego lane).
 */
int nc_ldw_get_ego_lane(uint32_t ch, float y, float *x_left, float *x_right, uint64_t *time_us);
int nc_ldw_is_in_ego_lane(uint32_t ch, float x, float y);
#endif

#endif
