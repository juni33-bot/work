#include "nc_run_mode.h"
#include "nc_meas.h"
#include "nc_traj.h"

#include <stdio.h>
#include <string.h>

#define NC_RUN_MODE_MAX_CH  (NC_MEAS_MAX_CH)

static volatile int g_run_mode = NC_RUN_MODE_NORMAL;
/* eligible frame counter per channel (render thread only) */
static uint32_t     g_eligible_cnt[NC_RUN_MODE_MAX_CH];

void nc_run_mode_set(E_NC_RUN_MODE mode)
{
    if (mode != NC_RUN_MODE_NORMAL && mode != NC_RUN_MODE_LIGHT) return;
    if ((int)mode == __atomic_load_n(&g_run_mode, __ATOMIC_ACQUIRE)) return;

    __atomic_store_n(&g_run_mode, (int)mode, __ATOMIC_RELEASE);
    memset(g_eligible_cnt, 0, sizeof(g_eligible_cnt));
    /* frame interval / k change with mode: drop old state */
    nc_meas_reset();
    nc_traj_reset();
    printf("[run_mode] %s (NPU input 1/%d)\n",
           mode == NC_RUN_MODE_LIGHT ? "LIGHT" : "NORMAL", nc_run_mode_npu_div());
}

E_NC_RUN_MODE nc_run_mode_get(void)
{
    return (E_NC_RUN_MODE)__atomic_load_n(&g_run_mode, __ATOMIC_ACQUIRE);
}

int nc_run_mode_npu_div(void)
{
    return (nc_run_mode_get() == NC_RUN_MODE_LIGHT) ? 2 : 1;
}

int nc_run_mode_should_send(uint32_t cam_ch)
{
    int div = nc_run_mode_npu_div();
    if (div <= 1) return 1;
    if (cam_ch >= NC_RUN_MODE_MAX_CH) return 1;

    uint32_t cnt = g_eligible_cnt[cam_ch]++;
    return (cnt % (uint32_t)div) == 0;
}
