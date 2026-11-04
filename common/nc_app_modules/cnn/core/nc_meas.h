#ifndef NC_MEAS_H
#define NC_MEAS_H

/*
 * nc_meas : per-camera-channel latency / frame-interval measurement
 *
 * All times are based on the frame timestamp (cnn_output->time_stamp_us),
 * which is taken with nc_get_mono_us_time() right after V4L2 DQBUF
 * in the render loop.
 *
 *  - pipeline latency : DQBUF -> detection postprocess done (result published)
 *  - display latency  : DQBUF -> first draw of that result in render loop
 *  - frame interval   : time between consecutive NPU input frames of a channel
 *                       (= one Kalman predict step)
 *
 * Values are smoothed with EMA (NC_MEAS_EMA_ALPHA).
 * Thread-safe: writers (postprocess worker / render loop) and readers may run
 * in different threads.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NC_MEAS_MAX_CH          (8)
#define NC_MEAS_EMA_ALPHA       (0.1f)
/* samples above this are treated as a stall/gap and dropped (us) */
#define NC_MEAS_MAX_VALID_US    (1000000ULL)

/* clear all measurement state (all channels) */
void nc_meas_reset(void);

/* call once per frame when detection postprocess result is published */
void nc_meas_on_pipeline_done(uint32_t cam_ch, uint64_t frame_ts_us);

/* call from render loop when a detection result is drawn.
 * Only the first draw of each new frame_ts_us is counted. */
void nc_meas_on_display(uint32_t cam_ch, uint64_t frame_ts_us);

/* DQBUF -> display latency (ms). Falls back to pipeline latency if no
 * display sample yet. Returns 0.0f if no sample at all. */
float nc_meas_get_latency_ms(uint32_t cam_ch);

/* DQBUF -> postprocess done latency (ms). 0.0f if no sample. */
float nc_meas_get_pipeline_latency_ms(uint32_t cam_ch);

/* NPU input frame interval of the channel (ms). 0.0f if no sample. */
float nc_meas_get_frame_interval_ms(uint32_t cam_ch);

#ifdef __cplusplus
}
#endif

#endif /* NC_MEAS_H */
