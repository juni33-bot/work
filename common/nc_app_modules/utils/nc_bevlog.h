#ifndef __NC_BEVLOG_H__
#define __NC_BEVLOG_H__

/*
 * BEV / minimap diagnostic CSV log.
 *
 * Disabled unless the environment variable NC_BEV_LOG is set to a directory:
 *     NC_BEV_LOG=/tmp ./app_wayland_npu
 * Files (overwritten each run):
 *     bev_obj.csv   render thread, one row per vehicle per displayed frame
 *     bev_det.csv   worker thread, raw detections before the tracker
 *     bev_lane.csv  render thread, one row per new lane result (LDW)
 *     bev_frame.csv render thread, frame timing
 *     bev_lanepts.csv render thread, one row per lane line per new lane result:
 *                   the points the minimap gets (after the UFLD average), pts = u:v;u:v;...
 *     bev_lanefit.csv render thread, one row per new lane result: minimap lane model
 *     bev_coast.csv render thread, one row per coasting vehicle per displayed frame
 * When disabled every call returns immediately.
 */

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NC_BEVLOG_OBJ = 0,
    NC_BEVLOG_DET,
    NC_BEVLOG_LANE,
    NC_BEVLOG_FRAME,
    NC_BEVLOG_LANEPTS,
    NC_BEVLOG_LANEFIT,
    NC_BEVLOG_COAST,
    NC_BEVLOG_NUM
} eNcBevLogFile;

/* 1 when logging is on (opens the files on first call) */
int nc_bevlog_enabled(void);

/* printf-style row to one file (thread-safe). Header is written once. */
void nc_bevlog_row(eNcBevLogFile f, const char *fmt, ...)
#ifdef __GNUC__
    __attribute__((format(printf, 2, 3)))
#endif
    ;

#ifdef __cplusplus
}
#endif

#endif
