#ifndef NC_TRAJ_H
#define NC_TRAJ_H

/*
 * nc_traj : per-track constant-velocity Kalman trajectory prediction
 *
 * Measured state and predicted output are kept separate:
 *  - nc_traj_update()        : measurement only (postprocess worker thread).
 *                              Reads tracker output, never writes it back,
 *                              so predictions never enter the tracker.
 *  - nc_traj_predict_frame() : render thread, once per displayed frame.
 *                              Takes ONE short lock and snapshots the
 *                              channel's track states.
 *  - nc_traj_get_predicted_box() : render thread, per object, lock-free
 *                              (reads the snapshot only).
 *
 * Track key = (cam_ch, class_id, track_id): ByteTrack runs one tracker per
 * class, so track_id is unique only within a class.
 *
 * Kalman state [cx, cy, vx, vy] (px, px/s), variable dt from frame
 * timestamps. Box size = last measured w/h. All times are monotonic us
 * (same clock as cnn_output->time_stamp_us).
 */

#include <stdint.h>
#include "nc_cnn_common.h"
#include "nc_cnn_network_includes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NC_TRAJ_MAX_CH                  (8)
#define NC_TRAJ_MAX_TRACKS              (128)   /* per channel */
#define NC_TRAJ_HIST_LEN                (16)    /* position history per track */
#define NC_TRAJ_MIN_HIST                (3)     /* measurements needed before predicting */
#define NC_TRAJ_MAX_PREDICT_MS_DEFAULT  (120)   /* extrapolation cap (100~150 ms) */
#define NC_TRAJ_STALE_US                (500000ULL) /* drop track not measured for this long */

typedef struct {
    float    cx, cy;
    uint64_t ts_us;
} stTrajPoint;

/* clear all channels: Kalman states, history, render snapshots */
void nc_traj_reset(void);

/* extrapolation cap (ms). Beyond it the box is held, not extrapolated. */
void  nc_traj_set_max_predict_ms(float ms);
float nc_traj_get_max_predict_ms(void);

/* [worker thread] measurement update, call right after nc_cnn_track().
 * frame_ts_us = capture timestamp of the frame the results belong to. */
void nc_traj_update(int ch, E_NETWORK_UID net_id,
                    const stCnnPostprocessingResults *results, uint64_t frame_ts_us);

/* [render thread] once per displayed frame of ch, before drawing/swap.
 * target_time_us = capture timestamp of the video frame being displayed. */
void nc_traj_predict_frame(int ch, uint64_t target_time_us);

/* [render thread] predicted box at target_time_us (bbox: top-left x,y,w,h).
 * Returns 1 and fills *out if predicted; returns 0 (out untouched) when the
 * caller must draw the measured box: unknown track, track_id < 0,
 * not a vehicle, or history < NC_TRAJ_MIN_HIST. */
int nc_traj_get_predicted_box(int ch, int class_id, int track_id,
                              uint64_t target_time_us, stBBox *out);

/* [any thread] copy position history (oldest first). Returns count. */
int nc_traj_get_history(int ch, int class_id, int track_id,
                        stTrajPoint *out, int max_cnt);

#ifdef __cplusplus
}
#endif

#endif /* NC_TRAJ_H */
