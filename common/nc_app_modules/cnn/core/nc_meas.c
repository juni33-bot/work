#include "nc_meas.h"
#include "nc_utils.h"

#include <pthread.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

/* #define NC_MEAS_DEBUG_PRINT */
#define NC_MEAS_DEBUG_PRINT_PERIOD  (90)

typedef struct {
    float    ema_ms;
    int      valid;
} stMeasEma;

typedef struct {
    stMeasEma pipeline;
    stMeasEma display;
    stMeasEma interval;
    uint64_t  last_input_ts_us;   /* last frame ts seen at pipeline done */
    uint64_t  last_display_ts_us; /* last frame ts counted at display */
#ifdef NC_MEAS_DEBUG_PRINT
    uint32_t  sample_cnt;
#endif
} stMeasCh;

static stMeasCh        g_meas[NC_MEAS_MAX_CH];
static pthread_mutex_t g_meas_lock = PTHREAD_MUTEX_INITIALIZER;

static void ema_update(stMeasEma *e, uint64_t sample_us)
{
    float ms = (float)sample_us / 1000.0f;
    if (!e->valid) {
        e->ema_ms = ms;
        e->valid = 1;
    } else {
        e->ema_ms += NC_MEAS_EMA_ALPHA * (ms - e->ema_ms);
    }
}

/* now >= ts and not a stall */
static int sample_ok(uint64_t now_us, uint64_t ts_us)
{
    return (ts_us != 0) && (now_us >= ts_us) && ((now_us - ts_us) <= NC_MEAS_MAX_VALID_US);
}

void nc_meas_reset(void)
{
    pthread_mutex_lock(&g_meas_lock);
    memset(g_meas, 0, sizeof(g_meas));
    pthread_mutex_unlock(&g_meas_lock);
}

void nc_meas_on_pipeline_done(uint32_t cam_ch, uint64_t frame_ts_us)
{
    if (cam_ch >= NC_MEAS_MAX_CH || frame_ts_us == 0) return;

    uint64_t now_us = nc_get_mono_us_time();

    pthread_mutex_lock(&g_meas_lock);
    stMeasCh *m = &g_meas[cam_ch];

    if (sample_ok(now_us, frame_ts_us)) {
        ema_update(&m->pipeline, now_us - frame_ts_us);
    }

    /* frame interval: diff of consecutive input timestamps of this channel.
     * A large gap (pause / stall) is dropped and the next frame re-seeds. */
    if (m->last_input_ts_us != 0 && sample_ok(frame_ts_us, m->last_input_ts_us)
        && frame_ts_us != m->last_input_ts_us) {
        ema_update(&m->interval, frame_ts_us - m->last_input_ts_us);
    }
    m->last_input_ts_us = frame_ts_us;

#ifdef NC_MEAS_DEBUG_PRINT
    if (++m->sample_cnt % NC_MEAS_DEBUG_PRINT_PERIOD == 0) {
        printf("[MEAS] ch=%u pipeline=%.1fms display=%.1fms interval=%.1fms\n",
               cam_ch, m->pipeline.ema_ms, m->display.ema_ms, m->interval.ema_ms);
    }
#endif
    pthread_mutex_unlock(&g_meas_lock);
}

void nc_meas_on_display(uint32_t cam_ch, uint64_t frame_ts_us)
{
    if (cam_ch >= NC_MEAS_MAX_CH || frame_ts_us == 0) return;

    uint64_t now_us = nc_get_mono_us_time();

    pthread_mutex_lock(&g_meas_lock);
    stMeasCh *m = &g_meas[cam_ch];
    /* same result is drawn on several render frames: count only first draw */
    if (frame_ts_us != m->last_display_ts_us) {
        m->last_display_ts_us = frame_ts_us;
        if (sample_ok(now_us, frame_ts_us)) {
            ema_update(&m->display, now_us - frame_ts_us);
        }
    }
    pthread_mutex_unlock(&g_meas_lock);
}

static float get_ema(uint32_t cam_ch, size_t offset)
{
    float ret = 0.0f;
    if (cam_ch >= NC_MEAS_MAX_CH) return ret;

    pthread_mutex_lock(&g_meas_lock);
    const stMeasEma *e = (const stMeasEma *)((const char *)&g_meas[cam_ch] + offset);
    if (e->valid) ret = e->ema_ms;
    pthread_mutex_unlock(&g_meas_lock);
    return ret;
}

float nc_meas_get_latency_ms(uint32_t cam_ch)
{
    float ret = 0.0f;
    if (cam_ch >= NC_MEAS_MAX_CH) return ret;

    pthread_mutex_lock(&g_meas_lock);
    const stMeasCh *m = &g_meas[cam_ch];
    if (m->display.valid)       ret = m->display.ema_ms;
    else if (m->pipeline.valid) ret = m->pipeline.ema_ms;
    pthread_mutex_unlock(&g_meas_lock);
    return ret;
}

float nc_meas_get_pipeline_latency_ms(uint32_t cam_ch)
{
    return get_ema(cam_ch, offsetof(stMeasCh, pipeline));
}

float nc_meas_get_frame_interval_ms(uint32_t cam_ch)
{
    return get_ema(cam_ch, offsetof(stMeasCh, interval));
}
