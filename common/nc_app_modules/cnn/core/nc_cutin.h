#ifndef NC_CUTIN_H
#define NC_CUTIN_H

/*
 * nc_cutin : cut-in warning from predicted trajectories + ego lane
 *
 * Render thread only (no lock). Per displayed frame of a channel:
 *   nc_cutin_begin_frame(ch, disp_ts)
 *   for each predicted vehicle: nc_cutin_eval(...)   -> 1: draw as cut-in
 *   nc_cutin_end_frame(ch)                            -> 1: a warning just turned on
 *
 * Decision is made in lane coordinates, u = (x - x_left) / (x_right - x_left),
 * u in [0,1] = inside the ego lane. Curves / ego yaw shift cars and lanes
 * together, so u cancels them out; only real lateral motion changes u.
 *
 * Cut-in candidate (one frame):
 *   bottom-center now    : outside the ego lane (u < 0 or u > 1)
 *   bottom-center future : inside with margin (MARGIN <= u <= 1 - MARGIN)
 * Warning ON after CUTIN_ON_FRAMES candidate frames in a row,
 * OFF after CUTIN_OFF_FRAMES non-candidate frames (no flicker).
 */

#include <stdint.h>
#include "nc_cnn_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NC_CUTIN_MAX_CH          (8)
#define NC_CUTIN_MAX_TRACKS      (64)     /* per channel */
#define NC_CUTIN_FUTURE_MS       (800)    /* horizon of the future point (longer than the drawn line:
                                           * the smooth future velocity reacts later) */
#define NC_CUTIN_LANE_MARGIN     (0.10f)  /* future point must be this far inside (lane width ratio) */
#define NC_CUTIN_ON_FRAMES       (3)
#define NC_CUTIN_OFF_FRAMES      (10)
#define NC_CUTIN_LANE_STALE_US   (200000ULL) /* ignore lane result older than this */
#define NC_CUTIN_MIN_LANE_W_PX   (120.0f) /* narrower lane (near vanishing point, far
                                           * extrapolation) -> no decision at that height */
#define NC_CUTIN_REF_Y           (1000.0f) /* ego lane-change detection height (near bottom) */
#define NC_CUTIN_LC_JUMP_RATIO   (0.40f)  /* lane center jump > ratio * lane width = ego lane change */
#define NC_CUTIN_LC_HOLD_FRAMES  (15)     /* pause decisions after ego lane change (~0.5 s) */

/* Board bring-up: print decision counts once per ~1 s per channel.
 * Uncomment NC_CUTIN_STATS to enable (tuning / debugging). */
/* #define NC_CUTIN_STATS */
#define NC_CUTIN_STATS_FRAMES    (27)

/*
 * Ego lane boundary, 1: ok, 0: no lane at this y (our convention).
 * y: screen px (same space as bbox, 1920x1080).
 * lane_ts_us: capture ts of the lane result (monotonic us), 0 = unknown.
 * Defined in nc_cutin.c:
 *   USE_LDW    : adapter to Junwoo's nc_ldw_get_ego_lane() (0 = ok, -1 = fail)
 *   otherwise  : fixed straight-road trapezoid stub (testing only)
 */
int nc_ldw_get_ego_lane_x(int ch, float y, float *x_left, float *x_right, uint64_t *lane_ts_us);

void nc_cutin_reset(void);

/* [render] start of a displayed frame of ch */
void nc_cutin_begin_frame(int ch, uint64_t disp_ts_us);

/* [render] now_box: predicted box at the displayed frame (top-left x,y,w,h).
 * fut_cx/fut_cy: predicted center after the horizon (nc_traj_get_future_center).
 * Returns 1 while the warning for this track is ON. */
int nc_cutin_eval(int ch, int class_id, int track_id, const stBBox *now_box,
                  float fut_cx, float fut_cy, uint64_t disp_ts_us);

/* [render] end of the frame. Tracks not evaluated this frame count as
 * non-candidates. Returns 1 if any warning turned OFF->ON this frame
 * (edge, for the recording trigger). */
int nc_cutin_end_frame(int ch);

/* [render] number of tracks with warning ON in ch (after end_frame) */
int nc_cutin_active_count(int ch);

#ifdef __cplusplus
}
#endif

#endif /* NC_CUTIN_H */
