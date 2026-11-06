#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>

#include "nc_bevlog.h"

static const char *const k_name[NC_BEVLOG_NUM] = {
    "bev_obj.csv", "bev_det.csv", "bev_lane.csv", "bev_frame.csv", "bev_lanepts.csv", "bev_lanefit.csv",
    "bev_coast.csv"
};

/* column headers (keep in sync with the callers) */
static const char *const k_header[NC_BEVLOG_NUM] = {
    /* obj: one vehicle per displayed frame (render thread) */
    "t_disp_us,t_det_us,t_lane_us,ch,track_id,class,"
    "meas_x,meas_y,meas_w,meas_h,pred_valid,pred_x,pred_y,pred_w,pred_h,"
    "u_used,v_used,vp_cal,k_cal,x_raw_m,z_raw_m,x_shown_m,z_shown_m,shown,"
    "vp_used,geom_same_frame,mm_id,disp_class",
    /* det: raw detection before the tracker (worker thread) */
    "t_det_us,ch,class,x,y,w,h,prob",
    /* lane: new lane result (render thread, LDW) */
    "t_now_us,t_lane_us,ch,left_ok,right_ok,aL,bL,aR,bR,k_inst,vp_inst,k_cal,vp_cal,"
    "state,dist_l_m,dist_r_m,lat_vel_mps,tlc_s,tlc_thresh_s,cnt_l,cnt_r,warn_until_us,latency_ms,"
    "class0,class1,class2,class3",
    /* frame: render timing */
    "t_us,gl_ms,frame_ms",
    /* lanepts: lane points used by the minimap (render thread), pts = u:v;u:v;... */
    "t_lane_us,ch,lane,lane_class,point_cnt,k_cal,vp_cal,pts",
    /* lanefit: minimap lane model (render thread). x(z) = c0 + h z + c_shown z^2 */
    "t_lane_us,ch,both,n_pts,k,vp,c_raw,c_filt,c_shown,curve_on,c0,h",
    /* coast: vehicle drawn beyond the image side (render thread), not measured */
    "t_disp_us,t_start_us,ch,mm_id,x_m,z_m,v_mps,alpha"
};

static FILE *g_fp[NC_BEVLOG_NUM];
static int g_state = 0;                 /* 0: not checked, 1: on, -1: off */
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static int bevlog_open_locked(void)
{
    if (g_state != 0) return g_state;

    const char *dir = getenv("NC_BEV_LOG");
    if (dir == NULL || dir[0] == '\0') { g_state = -1; return g_state; }

    for (int i = 0; i < NC_BEVLOG_NUM; i++) {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", dir, k_name[i]);
        g_fp[i] = fopen(path, "w");
        if (g_fp[i] == NULL) {
            printf("[BEVLOG] cannot open %s, logging off\n", path);
            for (int j = 0; j < i; j++) { fclose(g_fp[j]); g_fp[j] = NULL; }
            g_state = -1;
            return g_state;
        }
        setvbuf(g_fp[i], NULL, _IOFBF, 256 * 1024);
        fprintf(g_fp[i], "%s\n", k_header[i]);
    }
    printf("[BEVLOG] logging to %s/bev_*.csv\n", dir);
    g_state = 1;
    return g_state;
}

int nc_bevlog_enabled(void)
{
    int s = __atomic_load_n(&g_state, __ATOMIC_ACQUIRE);
    if (s != 0) return s > 0;
    pthread_mutex_lock(&g_lock);
    s = bevlog_open_locked();
    pthread_mutex_unlock(&g_lock);
    return s > 0;
}

void nc_bevlog_row(eNcBevLogFile f, const char *fmt, ...)
{
    if ((int)f < 0 || f >= NC_BEVLOG_NUM || !nc_bevlog_enabled()) return;

    va_list ap;
    va_start(ap, fmt);
    pthread_mutex_lock(&g_lock);
    vfprintf(g_fp[f], fmt, ap);
    fputc('\n', g_fp[f]);
    /* the app is stopped with kill: flush about once a second (every 30 frames) */
    static unsigned frame_rows = 0;
    if (f == NC_BEVLOG_FRAME && ++frame_rows % 30u == 0u) {
        for (int i = 0; i < NC_BEVLOG_NUM; i++) fflush(g_fp[i]);
    }
    pthread_mutex_unlock(&g_lock);
    va_end(ap);
}
