#ifndef NC_RUN_MODE_H
#define NC_RUN_MODE_H

/*
 * nc_run_mode : NPU input rate mode
 *
 *  NORMAL : every eligible frame of a channel is sent to NPU
 *  LIGHT  : only every 2nd eligible frame of a channel is sent to NPU
 *           (NPU load / result rate per channel halved).
 *           Frames in between get no new detection result; trajectory
 *           module fills them with predict-only.
 *
 * "eligible" = frame that would be sent in NORMAL mode
 * (e.g. 4ch already alternates ch0/2 and ch1/3 every render frame).
 *
 * Mode is meant to be set once at startup (before tracker init),
 * because ByteTrack is initialized with the effective per-channel fps.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NC_RUN_MODE_NORMAL = 0,
    NC_RUN_MODE_LIGHT  = 1,
} E_NC_RUN_MODE;

/* set mode. On change, measurement and trajectory state are reset. */
void nc_run_mode_set(E_NC_RUN_MODE mode);
E_NC_RUN_MODE nc_run_mode_get(void);

/* NPU input decimation factor: NORMAL=1, LIGHT=2 */
int nc_run_mode_npu_div(void);

/* call once per eligible frame of cam_ch (render thread only).
 * Returns 1 if this frame should be sent to NPU. */
int nc_run_mode_should_send(uint32_t cam_ch);

#ifdef __cplusplus
}
#endif

#endif /* NC_RUN_MODE_H */
