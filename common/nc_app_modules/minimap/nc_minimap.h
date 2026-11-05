#ifndef __NC_MINIMAP_H__
#define __NC_MINIMAP_H__

/*
 * Minimap (bird's-eye view)
 *
 * Draws the ego vehicle, the detected lane lines (network points, solid/dashed
 * from the network lane class) and detected vehicles seen from above, in a
 * panel on the left side of each view. Road positions come from the LDW module
 * (flat road, lane-width scale, calibrated horizon), so it needs USE_LDW.
 * Render thread only.
 *
 * Vehicle positions: bottom-centre of the box drawn on the camera view,
 * smoothed per vehicle (EMA). One minimap track per vehicle even if the class
 * (car/truck) or tracker id changes; class shown = vote over recent frames.
 *
 * Per displayed frame:
 *   nc_minimap_begin_frame(ch, t_disp);      // before adding detections
 *   nc_minimap_add_obj(ch, &obj);            // for each vehicle bbox
 *   nc_minimap_draw(ch, viewport, &res);     // after the camera overlays
 *
 * Forward distance depends on NC_LDW_DEFAULT_HFOV_DEG (see nc_ldw.h).
 */

#include <stdint.h>
#include "nc_opengl_interface.h"
#include "nc_opengl_ttf_font.h"
#include "nc_cnn_common.h"

/* Forward display range (m). Temporarily 25 m (was 40 m) while the far-range
 * jitter is analysed; set back to 40.0f to restore. Display only: positions,
 * smoothing and the NC_BEV_LOG rows are computed for every vehicle. */
#define NC_MINIMAP_DISPLAY_RANGE_M (25.0f)
#define NC_MINIMAP_RANGE_M      NC_MINIMAP_DISPLAY_RANGE_M
#define NC_MINIMAP_RANGE_HYST_M (2.0f)    // appear at <= RANGE-HYST, disappear above RANGE (never drawn beyond RANGE)
#define NC_MINIMAP_LAT_RANGE_M  (9.0f)    // lateral range each side (m): ego lane + 2 lanes
#define NC_MINIMAP_BG_ALPHA     (0.92f)   // panel background opacity

/* 1: lane line style from the UFLD lane_class output (1 = solid, 2 = dashed per
 *    the ini comment; mapping not verified on the board yet)
 * 0: draw every lane line dashed */
#define NC_MINIMAP_USE_LANE_CLASS (1)

/* panel position / size in canvas px (1920 x 1080, y grows downward) */
#define NC_MINIMAP_PANEL_X      (20.0f)
#define NC_MINIMAP_PANEL_Y      (140.0f)
#define NC_MINIMAP_PANEL_W      (300.0f)
#define NC_MINIMAP_PANEL_H      (460.0f)

typedef struct {
    Font *font;                         // small font for distance labels
    struct gl_font_program *font_prog;
    struct gl_npu_program *npu_prog;
} stNcMinimapDrawRes;

/* One detected vehicle. Boxes in canvas px, top-left x,y,w,h. */
typedef struct {
    const char *class_name;     // network class name ("car", "tru", "bus", "mot", ...)
    int         track_id;       // tracker id (< 0: associated by position only)
    stBBox      meas;           // tracker output at t_det_us (diagnostics)
    stBBox      shown;          // box drawn on the camera view (used for the minimap)
    int         pred_valid;     // 1: shown is the nc_traj prediction
    uint64_t    t_det_us;       // capture time of the detection frame
    uint64_t    t_disp_us;      // capture time of the displayed video frame
    float       color[4];       // RGBA of the box on the camera view (class colour)
    int         highlight;      // 1 = cut-in (drawn yellow)
} stNcMinimapObj;

/* Start a new detection list for channel ch (keeps the old list if not called). */
void nc_minimap_begin_frame(uint32_t ch, uint64_t t_disp_us);

void nc_minimap_add_obj(uint32_t ch, const stNcMinimapObj *in);

/* Update the vehicle tracks and draw the minimap of channel ch into its viewport. */
void nc_minimap_draw(uint32_t ch, struct viewport vp, const stNcMinimapDrawRes *res);

#endif
