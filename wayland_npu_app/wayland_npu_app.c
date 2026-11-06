/**
********************************************************************************
* Copyright (C) 2021 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : wayland_npu_app.c
*
* @brief   : wayland_npu application
*
* @author  : Software Development Team.  NextChip Inc.
*
* @date    : 2024.04.26.
*
* @version : 1.0.0
********************************************************************************
* @note
*
********************************************************************************
*/

/*
********************************************************************************
*               INCLUDES
********************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <signal.h>
#include <linux/input.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <time.h>
#include <errno.h>
#include <SOIL.h>
#include <sys/eventfd.h>
#include <arm_neon.h>
#include <omp.h>

#include "wayland_egl.h"
#include "nc_opengl_init.h"
#include "v4l2_interface.h"
#include "nc_opengl_shader.h"
#include "nc_opengl_interface.h"
#include "nc_ts_fsync_flipflop_buffers.h"
#include "nc_app_config_parser.h"
#include "nc_cnn_aiware_runtime.h"
#include "nc_cnn_communicator.h"
#include "nc_cnn_worker_for_postprocess.h"
#include "nc_neon.h"
#include "nc_utils.h"
#ifdef AIWARE_DEVICE_SUPPORTED
#include "aiware/runtime/c/aiwaredevice.h"
#endif
#include "nc_opengl_ttf_font.h"
#ifdef USE_BYTETRACK
#include "nc_cnn_tracker.h"
#endif
#include "nc_traj.h"
#include "nc_cutin.h"
#include "nc_bevlog.h"
#ifdef USE_LDW
#include "nc_ldw.h"
#include "nc_minimap.h"
#include "nc_class.h"
#endif

#ifdef USE_8MP_VI
#include "nc_dsr_helper.h"
#include "nc_dsr_set.h"
#include "nc_dmabuf_ctrl_helper.h"
#endif

#ifdef USE_ADAS_LD
#include"ADAS_LD_Lib.h"
#endif

/*
********************************************************************************
*               DEFINES
********************************************************************************
*/

#define VIS0_MAX_CH         (1)
#define VIS1_MAX_CH         (0)
#define VIDEO_MAX_CH        (VIS0_MAX_CH + VIS1_MAX_CH)

#define VIDEO_BUFFER_NUM    (3)

#ifdef USE_8MP_VI
#define VIDEO_WIDTH         MAX_WIDTH_FOR_VDMA_CNN_DS
#define VIDEO_HEIGHT        MAX_HEIGHT_FOR_VDMA_CNN_DS
#else
#define VIDEO_WIDTH         NPU_INPUT_WIDTH
#define VIDEO_HEIGHT        NPU_INPUT_HEIGHT
#endif

#define CAP_FPS             (30)

#define MQ_NAME_CNN_BUF     "/cnn_data"
#define DEV_FILE_DSR        "/dev/dsr"

#ifdef SHOW_PELEE_SEG
    #define NETWORK_FILE_PELEE_SEG      "misc/networks/peleeseg/peleeseg_640x384_apache6sr250_aiw4939.aiwbin"
#endif
#ifdef SHOW_PELEE_DETECT
    #define NETWORK_FILE_PELEE_DET      "misc/networks/peleeDet/peleedet_10class_640x384_apache6sr250_aiw4939.aiwbin"
#endif
#ifdef SHOW_YOLOV5_DETECT
    #define NETWORK_FILE_YOLOV5_DET     "misc/networks/Yolov5s/yolov5s_coco_640x384_apache6sr250_aiw4939.aiwbin"
#endif
#ifdef SHOW_YOLOV8_DETECT
    #define NETWORK_FILE_YOLOV8_DET     "misc/networks/Yolov8/yolov8s_coco_640x384_apache6sr250_aiw4939.aiwbin"
#endif
#ifdef SHOW_UFLD_LANE
    #define NETWORK_FILE_UFLD_LANE      "misc/networks/ufld/ufld_a6sr250_aiw4939.aiwbin"
#endif
#ifdef SHOW_TRI_CHIMERA
    #define NETWORK_FILE_TRI_CHIMERA    "misc/networks/trichimera/trichimera_640x384_a6sr_aiw4939.aiwbin"
#endif
#define COLOR_PALETTE_CNT    (10)
typedef enum {
    RGBA_R = 0,
    RGBA_G,
    RGBA_B,
    RGBA_A,
    RGBA_CNT,
} E_RGBA_IDX;

/*    don't modify  */
#define MAX_WIDTH_FOR_VDMA_CNN_DS   (1920)
#define MAX_HEIGHT_FOR_VDMA_CNN_DS  (1080)
/********************/

/*
********************************************************************************
*               VARIABLE DECLARATIONS
********************************************************************************
*/

st_npu_input_info npu_input_info;
struct gl_npu_program g_npu_prog;
GLuint g_seg_texture;
struct gl_font_program g_font_prog;
Font font_38;
Font font_24;

unsigned char* image_data;
static int running = 1;

st_nc_v4l2_config v4l2_config[VIDEO_MAX_CH];

#ifdef USE_FILE_INPUT
#ifdef USE_8MP_VI
#error "USE_FILE_INPUT does not support USE_8MP_VI"
#endif
/*
 * File input mode: decode a video file with ffmpeg (popen) instead of the camera.
 * ffmpeg outputs interleaved RGB24 at NPU input resolution, paced at real time (-re).
 * Only the first video stream is decoded.
 * Default: the videos in g_file_input_default_videos play in order, then repeat. Each video
 * gets its own ffmpeg, started when the previous one ends. Missing files are skipped.
 * "-i <file>": that one video loops forever (-stream_loop -1). Audio / metadata tracks of
 * phone .mov files can stop -stream_loop in ffmpeg 4.x, so if the stream still ends,
 * ffmpeg is restarted and the video plays from the start again.
 * The pipe fd is stored in video_fd so that existing "video_fd != -1" checks keep working.
 */
#define FILE_INPUT_FFMPEG_PATH      "misc/ffmpeg"
#define FILE_INPUT_FRAME_SIZE       (NPU_INPUT_WIDTH * NPU_INPUT_HEIGHT * 3)

#define FILE_INPUT_PATH_MAX         (256)

static const char *const g_file_input_default_videos[] = {
    "misc/image_npu/1920_1080_0001.mp4",
    "misc/image_npu/1920_1080_0002.mp4",
    "misc/image_npu/1920_1080_0003.mp4",
};
#define FILE_INPUT_MAX_VIDEOS       (sizeof(g_file_input_default_videos) / sizeof(g_file_input_default_videos[0]))

static const char *g_file_input_path = NULL;    // set by "-i <file>", NULL -> default list
static char g_file_input_videos[FILE_INPUT_MAX_VIDEOS][FILE_INPUT_PATH_MAX];
static int g_file_input_video_cnt = 0;
static int g_file_input_video_idx[VIDEO_MAX_CH];           // video now playing on each channel
static char g_file_input_ffmpeg[FILE_INPUT_PATH_MAX];
static FILE *g_file_input_fp[VIDEO_MAX_CH];
static unsigned char *g_file_input_frame[VIDEO_MAX_CH];   // interleaved RGB24
#define FILE_INPUT_MAX_RESTART      (3)    // consecutive restarts without a frame -> give up

static void file_input_add_video(const char *path)
{
    if (access(path, R_OK) != 0) {
        printf("[error] input video file not found: %s\n", path);
        return;
    }
    snprintf(g_file_input_videos[g_file_input_video_cnt++], FILE_INPUT_PATH_MAX, "%s", path);
}

// start ffmpeg on the channel's current video. returns NULL on failure
static FILE *file_input_open(int ch)
{
    char cmd[1024];
    const char *video = g_file_input_videos[g_file_input_video_idx[ch]];

    snprintf(cmd, sizeof(cmd),
             "%s -loglevel error -nostdin %s-re -i \"%s\" "
             "-map 0:v:0 -an -vf scale=%d:%d -pix_fmt rgb24 -f rawvideo -",
             g_file_input_ffmpeg, (g_file_input_video_cnt == 1) ? "-stream_loop -1 " : "",
             video, NPU_INPUT_WIDTH, NPU_INPUT_HEIGHT);
    printf("[file input] ch%d : %s\n", ch, cmd);
    return popen(cmd, "r");
}

static int file_input_initialize(void)
{
    snprintf(g_file_input_ffmpeg, sizeof(g_file_input_ffmpeg), "%s", nc_localize_path(FILE_INPUT_FFMPEG_PATH));
    if (g_file_input_path) {
        file_input_add_video(g_file_input_path);
    } else {
        for (size_t i = 0; i < FILE_INPUT_MAX_VIDEOS; i++) {
            file_input_add_video(nc_localize_path(g_file_input_default_videos[i]));
        }
    }

    if (g_file_input_video_cnt == 0) {
        return -1;
    }

    for (int i = 0; i < VIDEO_MAX_CH; i++) {
        g_file_input_frame[i] = (unsigned char *)malloc(FILE_INPUT_FRAME_SIZE);
        if (g_file_input_frame[i] == NULL) {
            printf("[error] file input frame buffer alloc failure!\n");
            return -1;
        }

        g_file_input_fp[i] = file_input_open(i);
        if (g_file_input_fp[i] == NULL) {
            perror("[error] popen(ffmpeg) failure");
            return -1;
        }
        v4l2_config[i].video_buf.video_fd = fileno(g_file_input_fp[i]);
    }

    return 0;
}

// read one full frame (blocking). returns 0 on success, -1 on EOF/error
static int file_input_read_frame(int ch)
{
    static int restart_cnt[VIDEO_MAX_CH];
    size_t total = 0;

    while (total < FILE_INPUT_FRAME_SIZE) {
        size_t n = fread(g_file_input_frame[ch] + total, 1, FILE_INPUT_FRAME_SIZE - total, g_file_input_fp[ch]);
        if (n == 0) {
            // stream ended: go to the next video (single video: restart it)
            pclose(g_file_input_fp[ch]);
            g_file_input_fp[ch] = NULL;
            g_file_input_video_idx[ch] = (g_file_input_video_idx[ch] + 1) % g_file_input_video_cnt;
            if (restart_cnt[ch] >= FILE_INPUT_MAX_RESTART ||
                (g_file_input_fp[ch] = file_input_open(ch)) == NULL) {
                printf("[error] ffmpeg stream ended (ch%d). check ffmpeg / video file\n", ch);
                v4l2_config[ch].video_buf.video_fd = -1;    // fall back to logo image
                return -1;
            }
            restart_cnt[ch]++;
            v4l2_config[ch].video_buf.video_fd = fileno(g_file_input_fp[ch]);
            total = 0;                                  // drop the partial frame
            continue;
        }
        total += n;
    }

    restart_cnt[ch] = 0;
    return 0;
}

static void file_input_deinitialize(void)
{
    for (int i = 0; i < VIDEO_MAX_CH; i++) {
        if (g_file_input_fp[i]) {
            pclose(g_file_input_fp[i]);
            g_file_input_fp[i] = NULL;
        }
        if (g_file_input_frame[i]) {
            free(g_file_input_frame[i]);
            g_file_input_frame[i] = NULL;
        }
    }
}
#endif /* USE_FILE_INPUT */

struct viewport g_viewport[VIDEO_MAX_CH];

#ifdef USE_8MP_VI
size_t BUFF_SIZE;
int dsr_fd;
DSR_Data_t dsr_info;
dma_alloc_info dma_info;
st_nc_dsr_config dsr_config;
img_input_config dsr_input_config;
img_output_config dsr_output_config;
#endif
/*
********************************************************************************
*               FUNCTION DEFINITIONS
********************************************************************************
*/
#ifdef USE_8MP_VI
static int dsr_init(void)
{
    int input_fd = 0, output_fd = 0;
    long page_size = sysconf(_SC_PAGESIZE);

    dsr_input_config.format = IMG_FORMAT_RGB888;
    dsr_input_config.width = MAX_WIDTH_FOR_VDMA_CNN_DS;
    dsr_input_config.height = MAX_HEIGHT_FOR_VDMA_CNN_DS;
    dsr_output_config.format = IMG_FORMAT_RGB888;

    BUFF_SIZE = dsr_input_config.width * dsr_input_config.height * 3;
    BUFF_SIZE = ((BUFF_SIZE + page_size - 1) / page_size) * page_size;


    if(dsr_config_downscale(&dsr_config, 1, NPU_INPUT_WIDTH, NPU_INPUT_HEIGHT) < 0){
        perror("Error: DSR config fail");
        return -1;
    }

    if(open_device_and_dma_buffers(DEV_FILE_DSR, &dsr_fd, &input_fd, &output_fd, BUFF_SIZE) < 0){
        perror("Error: DSR open fail");
        return -1;
    }

    if(dsr_setup_buffer(&dsr_info, dsr_fd, &dma_info, input_fd, output_fd, BUFF_SIZE) < 0){
        perror("Error: DSR setup fail");
        return -1;
    }

    return 0;
}

static int dsr_deinit(void)
{
    nc_dmabuf_ctrl_end_cpu_access(dma_info.dmabuf_fd_in);
    nc_dmabuf_ctrl_end_cpu_access(dma_info.dmabuf_fd_out);

    nc_dmabuf_ctrl_free_dma_fd(dma_info.dmabuf_fd_in);
    nc_dmabuf_ctrl_free_dma_fd(dma_info.dmabuf_fd_out);

    nc_dmabuf_ctrl_close();

    for (int i = 0; i < BUFF_NUM; i++) {
        if (dsr_info.dsr_in_buf[i] != MAP_FAILED) {
            munmap(dsr_info.dsr_in_buf[i], BUFF_SIZE);
        }
        if (dsr_info.dsr_out_buf[i] != MAP_FAILED) {
            munmap(dsr_info.dsr_out_buf[i], BUFF_SIZE);
        }
    }

    dsr_device_deinit(&dsr_fd);

    return 0;
}
#endif

void set_viewport_config(void)
{
#if(VIDEO_MAX_CH == 1)
    // full screen view
    g_viewport[0].x         = 0;
    g_viewport[0].y         = 0;
    g_viewport[0].width     = WINDOW_WIDTH;
    g_viewport[0].height    = WINDOW_HEIGHT;
#elif(VIDEO_MAX_CH > 1)
    // quad view
    for(int i = 0; i < VIDEO_MAX_CH; i++)
    {
        g_viewport[i].width     = WINDOW_WIDTH/2;
        g_viewport[i].height    = WINDOW_HEIGHT/2;

        // Set the view position for each channel
        switch(i)
        {
            case 0:
                g_viewport[i].x = 0;
                g_viewport[i].y = WINDOW_HEIGHT/2;
                break;
            case 1:
                g_viewport[i].x = WINDOW_WIDTH/2;
                g_viewport[i].y = WINDOW_HEIGHT/2;
                break;
            case 2:
                g_viewport[i].x = 0;
                g_viewport[i].y = 0;
                break;
            case 3:
                g_viewport[i].x = WINDOW_WIDTH/2;
                g_viewport[i].y = 0;
                break;
            default:
                break;
        }
    }
#endif
}

void set_v4l2_config(void)
{
    int i = 0, j = 0;

    for(i = 0; i < VIS0_MAX_CH; i++) {
        v4l2_config[i].video_buf.video_device_num = CNN_DEVICE_NUM(VISION0) + i;
        v4l2_config[i].video_buf.video_fd         = -1;
#ifdef USE_8MP_VI
        v4l2_config[i].dma_mode                   = INTERLEAVE;
#else
        v4l2_config[i].dma_mode                   = PLANAR;
#endif
        v4l2_config[i].img_process                = MODE_DS;
        v4l2_config[i].pixformat                  = V4L2_PIX_FMT_RGB24;
        v4l2_config[i].crop_x_start               = 0;
        v4l2_config[i].crop_y_start               = 0;
        v4l2_config[i].crop_width                 = 0;
        v4l2_config[i].crop_height                = 0;
#ifdef USE_8MP_VI
        v4l2_config[i].ds_width                   = MAX_WIDTH_FOR_VDMA_CNN_DS;
        v4l2_config[i].ds_height                  = MAX_HEIGHT_FOR_VDMA_CNN_DS;
#else
        v4l2_config[i].ds_width                   = VIDEO_WIDTH;
        v4l2_config[i].ds_height                  = VIDEO_HEIGHT;
#endif
    }

    for(j = VIS0_MAX_CH; j < VIDEO_MAX_CH; j++) {
        v4l2_config[j].video_buf.video_device_num = CNN_DEVICE_NUM(VISION1) + j;
        v4l2_config[j].video_buf.video_fd         = -1;
#ifdef USE_8MP_VI
        v4l2_config[j].dma_mode                   = INTERLEAVE;
#else
        v4l2_config[j].dma_mode                   = PLANAR;
#endif
        v4l2_config[j].img_process                = MODE_DS;
        v4l2_config[j].pixformat                  = V4L2_PIX_FMT_RGB24;
        v4l2_config[j].crop_x_start               = 0;
        v4l2_config[j].crop_y_start               = 0;
        v4l2_config[j].crop_width                 = 0;
        v4l2_config[j].crop_height                = 0;
#ifdef USE_8MP_VI
        v4l2_config[j].ds_width                   = MAX_WIDTH_FOR_VDMA_CNN_DS;
        v4l2_config[j].ds_height                  = MAX_HEIGHT_FOR_VDMA_CNN_DS;
#else
        v4l2_config[j].ds_width                   = VIDEO_WIDTH;
        v4l2_config[j].ds_height                  = VIDEO_HEIGHT;
#endif
    }
}

int send_cnn_buf (uint8_t *ptr_cnn_buf, uint64_t time_stamp_us, uint32_t cam_ch, E_NETWORK_UID net_id)
{
    int ret = 0;
    stCnnData *cnn_data;
    struct mq_attr attr;
    attr.mq_maxmsg = MAX_MQ_MSG_CNT;
    attr.mq_msgsize = sizeof(stCnnData*);
    int oflag = O_WRONLY | O_CREAT;
    mqd_t mfd = mq_open(MQ_NAME_CNN_BUF, oflag, 0666, &attr);
    if (mfd == -1) {
        perror("mq open error");
        return -1;
    }

    cnn_data = (stCnnData*)malloc(sizeof(stCnnData));
    cnn_data->cam_ch = cam_ch;
    cnn_data->ptr_cnn_buf = ptr_cnn_buf;
    cnn_data->time_stamp_us = time_stamp_us;
    cnn_data->net_id = net_id;

    if ((ret = mq_send(mfd, (const char *)&cnn_data, attr.mq_msgsize, 1)) == -1) {
        printf("errno of mq_send = %d\n", errno);
    }
    mq_close(mfd);

    return ret;
}

int receive_cnn_buf (stCnnData **out_cnn_buf)
{
    int ret = 0;
    struct mq_attr attr;
    attr.mq_maxmsg = MAX_MQ_MSG_CNT;
    attr.mq_msgsize = sizeof(stCnnData *);
    int oflag = O_RDONLY | O_CREAT;
    mqd_t mfd = mq_open(MQ_NAME_CNN_BUF, oflag, 0666, &attr);
    if (mfd == -1) {
        perror("mq open error");
        return -1;
    }

    if ((ret = (int32_t)mq_receive(mfd, (char*)out_cnn_buf, attr.mq_msgsize, NULL)) == -1) {
        printf("errno of mq_receive = %d\n", errno);
    }
    mq_close(mfd);

    return ret;
}

int v4l2_initialize(void)
{
#ifdef USE_FILE_INPUT
    return file_input_initialize();
#endif
    for(int i = 0; i < VIDEO_MAX_CH; i++)
    {
        v4l2_config[i].video_buf.video_fd = nc_v4l2_open(v4l2_config[i].video_buf.video_device_num, true);
        if(v4l2_config[i].video_buf.video_fd == errno) {
            printf("[error] nc_v4l2_open() failure!\n");
        } else {
            if(nc_v4l2_init_device_and_stream_on(&v4l2_config[i], VIDEO_BUFFER_NUM) < 0) {
                printf("[error] nc_v4l2_init_device_and_stream_on() failure!\n");
                return -1;
            }
        }
    }

    nc_v4l2_show_user_config(&v4l2_config[0], VIDEO_MAX_CH);

    return 0;
}

// capture timestamp (monotonic us) of the video frame currently displayed per channel.
// Written and read only in the render thread.
static uint64_t g_disp_frame_ts_us[VIDEO_MAX_CH];

// 1: draw the prediction line + end marker on the video. The minimap arrow and the cut-in
// judgement do not depend on this: they use the future center, which is always computed.
#define SHOW_PRED_LINE      (0)

// prediction line shorter than this (screen px) is not drawn
#define PRED_LINE_MIN_PX    (15.0f)

// 1: append road distance "z m(x)" to each bbox label (debug, labels may overlap)
#define SHOW_OBJ_DISTANCE   (0)

// cut-in warning box / text color (yellow: car class is already red)
static float g_cutin_color[4] = {1.0f, 0.9f, 0.0f, 1.0f};

void nc_draw_gl_npu(struct viewport viewport, int network_task, pp_result_buf *net_result, struct gl_npu_program g_npu_prog)
{
    stCnnPostprocessingResults *det_result = &net_result->cnn_result;
    stObjDrawInfo *draw_cnn = &net_result->draw_info;
    stSegDrawInfo *draw_seg = &net_result->seg_info;
    stLaneDrawInfo *draw_lane = &net_result->lane_draw_info;

    int max_class_cnt = draw_cnn->max_class_cnt;
    int max_seg_class_cnt = draw_seg->max_class_cnt;
    float target_view_ratio = 1.f;
    char buftext[128];

    target_view_ratio = (float)WINDOW_HEIGHT / (float)viewport.height;

    glViewport(viewport.x, viewport.y, viewport.width, viewport.height);

    if(network_task == DETECTION)
    {
        float** color = (float**)malloc(max_class_cnt * sizeof(float*));
        for (int i = 0; i < max_class_cnt; ++i) {
            color[i] = (float*)malloc(RGBA_CNT * sizeof(float));
        }

        for (int j = 0; j < max_class_cnt; ++j) {
            color[j][RGBA_R] = (float)(draw_cnn->class_colors[j].r) / 255.0f;
            color[j][RGBA_G] = (float)(draw_cnn->class_colors[j].g) / 255.0f;
            color[j][RGBA_B] = (float)(draw_cnn->class_colors[j].b) / 255.0f;
            color[j][RGBA_A] = 1.0f;
        }

        // cut-in judgement runs once per displayed frame of this channel
        int cut_ch = (net_result->cam_channel < VIDEO_MAX_CH) ? (int)net_result->cam_channel : -1;
        if (cut_ch >= 0) nc_cutin_begin_frame(cut_ch, g_disp_frame_ts_us[cut_ch]);
#ifdef USE_LDW
        if (cut_ch >= 0) nc_minimap_begin_frame((uint32_t)cut_ch, g_disp_frame_ts_us[cut_ch]);
#endif

        for(int i=0; i<draw_cnn->max_class_cnt; i++){
            for(int bidx = 0; bidx < det_result->class_objs[i].obj_cnt; bidx++) {
                stObjInfo obj_info = det_result->class_objs[i].objs[bidx];
                // predicted box if available (vehicle, enough history), else measured box.
                // Only the drawn copy changes; det_result / tracker input stay measured.
                int   has_future = 0;
                int   pred_valid = 0;
                float fut_cx = 0.f, fut_cy = 0.f;
                float cut_cx = 0.f, cut_cy = 0.f;
                if (net_result->cam_channel < VIDEO_MAX_CH) {
                    int      tch    = (int)net_result->cam_channel;
                    uint64_t disp_t = g_disp_frame_ts_us[tch];
                    stBBox pred_box;
                    if (nc_traj_get_predicted_box(tch, i, obj_info.track_id, disp_t, &pred_box)) {
                        obj_info.bbox = pred_box;
                        pred_valid = 1;
                        // future center (same snapshot) for the prediction line
                        has_future = nc_traj_get_future_center(tch, i, obj_info.track_id, disp_t,
                                                               (float)NC_TRAJ_FUTURE_MS_DEFAULT,
                                                               &fut_cx, &fut_cy);
                        if (has_future) {
                            nc_traj_get_future_center(tch, i, obj_info.track_id, disp_t,
                                                      (float)NC_CUTIN_FUTURE_MS, &cut_cx, &cut_cy);
                        }
                    }
                }
                // cut-in: future bottom-center enters the ego lane (lane coordinates)
                int is_cutin = 0;
                if (has_future) {
                    is_cutin = nc_cutin_eval(cut_ch, i, obj_info.track_id, &obj_info.bbox,
                                             cut_cx, cut_cy, g_disp_frame_ts_us[cut_ch]);
                }
                float *dcolor = is_cutin ? g_cutin_color : color[i];

                nc_opengl_draw_rectangle(obj_info.bbox.x, obj_info.bbox.y, obj_info.bbox.w, obj_info.bbox.h, dcolor, g_npu_prog);
                if (is_cutin) { // thicker box
                    nc_opengl_draw_rectangle(obj_info.bbox.x + 3.f, obj_info.bbox.y + 3.f,
                                             obj_info.bbox.w - 6.f, obj_info.bbox.h - 6.f, dcolor, g_npu_prog);
                }

#if SHOW_PRED_LINE
                // prediction line: current (predicted-now) center -> center after FUTURE_MS, dashed
                // Hidden when shorter than PRED_LINE_MIN_PX: cars followed at similar speed barely
                // move on screen, and their short line only shows bbox jitter.
                float cur_cx = obj_info.bbox.x + obj_info.bbox.w * 0.5f;
                float cur_cy = obj_info.bbox.y + obj_info.bbox.h * 0.5f;
                float ldx = fut_cx - cur_cx, ldy = fut_cy - cur_cy;
                if (has_future && (ldx * ldx + ldy * ldy) >= PRED_LINE_MIN_PX * PRED_LINE_MIN_PX) {
                    nc_opengl_draw_line(cur_cx, cur_cy, fut_cx, fut_cy, 2 /* dashed */, dcolor, g_npu_prog);
                    nc_opengl_draw_rectangle(fut_cx - 4.f, fut_cy - 4.f, 8.f, 8.f, dcolor, g_npu_prog); // end marker
                }
#endif

                // draw bbox label
#ifdef USE_BYTETRACK
                // show track id (not cnn probability)
                if (obj_info.track_id < 0) sprintf(buftext, "%s:%0.2f", draw_cnn->class_names[i], obj_info.prob);
                else sprintf(buftext, "[%d]%s:%0.2f", obj_info.track_id, draw_cnn->class_names[i], obj_info.prob);
#else
                sprintf(buftext, "%s:%0.2f", draw_cnn->class_names[i], obj_info.prob);
#endif
                if (is_cutin) sprintf(buftext, "[%d]CUT-IN", obj_info.track_id);
#ifdef USE_LDW
                // minimap: vehicles only (position from the bbox bottom-center)
                if (cut_ch >= 0 && nc_class_is_vehicle(net_result->net_id, i)) {
                    stNcMinimapObj mo;
                    memset(&mo, 0, sizeof(mo));
                    mo.class_name = draw_cnn->class_names[i];
                    mo.track_id   = obj_info.track_id;
                    mo.meas       = det_result->class_objs[i].objs[bidx].bbox;   // tracker output
                    mo.shown      = obj_info.bbox;                               // drawn (predicted) box
                    mo.pred_valid = pred_valid;
                    mo.t_det_us   = net_result->time_stamp;
                    mo.t_disp_us  = g_disp_frame_ts_us[cut_ch];
                    memcpy(mo.color, dcolor, sizeof(mo.color));
                    mo.highlight  = is_cutin;
                    // predicted bottom-centre after NC_TRAJ_FUTURE_MS_DEFAULT (box size kept)
                    mo.fut_valid  = has_future;
                    mo.fut_u      = fut_cx;
                    mo.fut_v      = fut_cy + obj_info.bbox.h * 0.5f;
                    nc_minimap_add_obj((uint32_t)cut_ch, &mo);
                }
#endif
#if defined(USE_LDW) && SHOW_OBJ_DISTANCE
                // distance check: road position of the bbox bottom-center (ground contact point)
                if (cut_ch >= 0) {
                    float x_m, z_m;
                    if (nc_ldw_app_pixel_to_road((uint32_t)cut_ch,
                                                 obj_info.bbox.x + obj_info.bbox.w * 0.5f,
                                                 obj_info.bbox.y + obj_info.bbox.h,
                                                 &x_m, &z_m) == 0 && z_m > 0.f) {
                        size_t n = strlen(buftext);
                        snprintf(buftext + n, sizeof(buftext) - n, " %.1fm(%+.1f)", z_m, x_m);
                    }
                }
#endif
                float textcolor[3] = {dcolor[RGBA_R], dcolor[RGBA_G], dcolor[RGBA_B]};
                nc_opengl_draw_text(&font_24, buftext, obj_info.bbox.x, (float)WINDOW_HEIGHT - (obj_info.bbox.y-12), target_view_ratio, textcolor, WINDOW_WIDTH, WINDOW_HEIGHT, g_font_prog);
            }
        }

        if (cut_ch >= 0) {
            if (nc_cutin_end_frame(cut_ch)) {
                // TODO(merge): nc_rec_trigger(1) -- cut-in warning just turned ON (edge only)
            }
            if (nc_cutin_active_count(cut_ch) > 0) {
                float wcolor[3] = {g_cutin_color[RGBA_R], g_cutin_color[RGBA_G], g_cutin_color[RGBA_B]};
                nc_opengl_draw_text(&font_38, "CUT-IN WARNING", (float)WINDOW_WIDTH * 0.5f - 160.f, 1020.f,
                                    target_view_ratio, wcolor, WINDOW_WIDTH, WINDOW_HEIGHT, g_font_prog);
            }
        }

        for (int i = 0; i < max_class_cnt; ++i) {
            free(color[i]);
        }
        free(color);
    }
    else if(network_task == SEGMENTATION)
    {
        glBindTexture(GL_TEXTURE_2D, g_seg_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, draw_seg->width, draw_seg->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, det_result->seg);

        float color[COLOR_PALETTE_CNT][RGBA_CNT];

        for (int j = 0; j < max_seg_class_cnt; ++j) {
            color[j][RGBA_R] = (float)(draw_seg->class_colors[j].r) / 255.0f;
            color[j][RGBA_G] = (float)(draw_seg->class_colors[j].g) / 255.0f;
            color[j][RGBA_B] = (float)(draw_seg->class_colors[j].b) / 255.0f;
            color[j][RGBA_A] = (float)(draw_seg->class_colors[j].a) / 255.0f;
        }
        nc_opengl_draw_segmentation(g_seg_texture, (float**)color, draw_seg->max_class_cnt, g_npu_prog);
    }
    else if(network_task == LANE)
    {
        float color[COLOR_PALETTE_CNT][RGBA_CNT];

        for (int j = 0; j < draw_lane->max_lane_num; ++j) {
            color[j][RGBA_R] = (float)(draw_lane->index_colors[j].r) / 255.0f;
            color[j][RGBA_G] = (float)(draw_lane->index_colors[j].g) / 255.0f;
            color[j][RGBA_B] = (float)(draw_lane->index_colors[j].b) / 255.0f;
            color[j][RGBA_A] = 1.0f;
        }

        for(int i = 0; i < draw_lane->max_lane_num; i++)
        {
            int point_num = det_result->lane_det[i].point_cnt;
            int lane_class = det_result->lane_det[i].lane_class;

            if(point_num == 0) continue;

            for(int j=0; j<point_num-1; j++)
            {
                float st_x = det_result->lane_det[i].point[j].x;
                float st_y = det_result->lane_det[i].point[j].y;
                float end_x = det_result->lane_det[i].point[j+1].x;
                float end_y = det_result->lane_det[i].point[j+1].y;

                nc_opengl_draw_line(st_x, st_y, end_x, end_y, lane_class, color[i], g_npu_prog);
            }
        }

#ifdef USE_UFLD_NETWORK_DEBUGGING
        float color_table[6][RGBA_CNT] = {{1.0f,1.0f,1.0f,0.15f}, // white (grid)
                                          {1.0f,0.0f,0.0f,1.0f},  // red (index 0)
                                          {0.0f,0.0f,1.0f,1.0f},  // blue (index 1)
                                          {1.0f,1.0f,0.0f,1.0f},  // yellow (index 2)
                                          {0.0f,1.0f,0.0f,1.0f},  // green (index 3)
                                          {0.0f,0.0f,0.0f,1.0f}}; // black (final point)
        stUFLD_dbg_info* debug_info = NULL;
        int info_cnt = 0;

#if defined(DBG_ROW_ANCHOR)

        float row_cell_num = (float)(draw_lane->row_cell_num);
        float row_cell_width = WINDOW_WIDTH/(float)(row_cell_num-1);
        float row_cell_height = (draw_lane->row_anchor_max - draw_lane->row_anchor_min) * WINDOW_HEIGHT * (1/ (float)(draw_lane->row_anchor_num - 1));

        //draw row anchor grid
        for(int i=0 ; i<(draw_lane->row_anchor_num); i++)
        {
            nc_opengl_draw_debugging_grid_line(0, draw_lane->row_anchor[i] * WINDOW_HEIGHT, WINDOW_WIDTH-1, draw_lane->row_anchor[i] * WINDOW_HEIGHT, 1, color_table[0], g_npu_prog);
        }

        for(float j=0; j<row_cell_num; j++)
        {
            nc_opengl_draw_debugging_grid_line(j/(row_cell_num-1)*WINDOW_WIDTH, 0, j/(row_cell_num-1)*WINDOW_WIDTH, WINDOW_HEIGHT-1, 1, color_table[0], g_npu_prog);
        }

        for(int i = 0; i < draw_lane->max_lane_num; i++)
        {
            if(draw_lane->lane_anchor_info[i] != 0) continue;

            info_cnt = det_result->lane_det[i].dbg_info_cnt;
            if(info_cnt == 0) continue;

            for(int j=0; j<info_cnt; j++)
            {
                debug_info = &(det_result->lane_det[i].ufld_dbg_info[j]);

                //draw max_indices
                nc_opengl_draw_rectangle(debug_info->x, debug_info->y, row_cell_width, row_cell_height , color_table[i+1], g_npu_prog);

                //draw stddev
                nc_opengl_draw_line(debug_info->index_min, debug_info->y + row_cell_height/2, debug_info->index_max, debug_info->y + row_cell_height/2, 1, color_table[i+1], g_npu_prog);

                //draw final result (point)
                nc_opengl_draw_rectangle(debug_info->final_point.x-3, debug_info->final_point.y-3, 6, 6, color_table[5], g_npu_prog);
            }
        }

#elif defined(DBG_COL_ANCHOR)

        float col_cell_num = (float)(draw_lane->col_cell_num);
        float col_cell_width = (draw_lane->col_anchor_max - draw_lane->col_anchor_min) * WINDOW_WIDTH * (1/ (float)(draw_lane->col_anchor_num - 1));
        float col_cell_height = WINDOW_HEIGHT/(float)(col_cell_num-1);

        //draw column anchor grid
        for(int i=0 ; i<(draw_lane->col_anchor_num); i++)
        {
            nc_opengl_draw_debugging_grid_line(draw_lane->col_anchor[i] * WINDOW_WIDTH, 0, draw_lane->col_anchor[i] * WINDOW_WIDTH, WINDOW_HEIGHT-1, 1, color_table[0], g_npu_prog);
        }

        for(float j=0; j<col_cell_num; j++)
        {
            nc_opengl_draw_debugging_grid_line(0, j/(col_cell_num-1)*WINDOW_HEIGHT, WINDOW_WIDTH-1, j/(col_cell_num-1)*WINDOW_HEIGHT, 1, color_table[0], g_npu_prog);
        }

        for(int i = 0; i < draw_lane->max_lane_num; i++)
        {
            if(draw_lane->lane_anchor_info[i] != 1) continue;

            info_cnt = det_result->lane_det[i].dbg_info_cnt;
            if(info_cnt == 0) continue;

            for(int j=0; j<info_cnt; j++)
            {
                debug_info = &(det_result->lane_det[i].ufld_dbg_info[j]);

                //draw max_indices
                nc_opengl_draw_rectangle(debug_info->x, debug_info->y, col_cell_width, col_cell_height , color_table[i+1], g_npu_prog);

                //draw stddev
                nc_opengl_draw_line(debug_info->x + col_cell_width/2, debug_info->index_min, debug_info->x + col_cell_width/2, debug_info->index_max, 1, color_table[i+1], g_npu_prog);

                //draw final result (point)
                nc_opengl_draw_rectangle(debug_info->final_point.x-3, debug_info->final_point.y-3, 6, 6, color_table[5], g_npu_prog);
            }
        }
#endif
#endif
    }
    else
    {
        // printf("Invalid network %d\n", sel_network);
    }
}

void gl_initialize(struct window *window)
{
    int width, height, channels;
    char buf[128];

    sprintf(buf, "misc/image/nextchip_s.png");
    image_data = SOIL_load_image(buf, &width, &height, &channels, SOIL_LOAD_RGBA);

    // init video shader
    nc_opengl_init_video_shader(window, 0);

    // init texture for video draw
    for(int i=0;i<VIDEO_MAX_CH;i++)
    {
        glGenTextures(1, &window->gl.texture[i]);
        glBindTexture(GL_TEXTURE_2D, window->gl.texture[i]);

        //stbi_set_flip_vertically_on_load(true);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
        //glGenerateMipmap(GL_TEXTURE_2D);

        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // Load the font using the freetype library.
    nc_opengl_load_font("misc/font/NotoSans-Regular.ttf", 38, &font_38);
    nc_opengl_load_font("misc/font/NotoSans-Regular.ttf", 24, &font_24);
    // init font shader
    nc_opengl_init_font_shader(&g_font_prog);

    // init npu shader
    nc_opengl_init_npu_shader(&g_npu_prog);

    // init texture for segmentation draw
    glGenTextures(1, &g_seg_texture);
    glBindTexture(GL_TEXTURE_2D, g_seg_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // set viewport
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

// #define SHOW_FPS
#ifdef SHOW_FPS
float calc_fps_at_loop_ent1(int update_period_fcnt)
{
    static uint64_t s_time = 0;
    static float fps = 0.f;
    static uint64_t fcnt = 0;
    uint64_t elapsed_ms = 0;
    fcnt++;
    if (fcnt % update_period_fcnt == 1) {
        if (s_time == 0) {
            s_time = nc_get_mono_time();
        } else {
            elapsed_ms = nc_elapsed_time(s_time);
            // printf("fcnt(%d) elapsed_ms(%d)\n", fcnt, elapsed_ms);
            if (elapsed_ms > 0) fps = (float)((fcnt-1) / (elapsed_ms/1000.f));

            // re-init
            fcnt = 1;
            s_time = nc_get_mono_time();
        }
    }

    return fps;
}
#endif

void render(void *data, struct wl_callback *callback, uint32_t time)
{
    static struct timespec begin, end, set_time;
    static uint64_t fpstime = 0;
    static uint64_t fpscount = 0;
    static uint64_t fpscount_00 = 0;
    static uint64_t frametime = 0;
    static uint64_t opengl_time = 0;
    static uint64_t framecnt = 0;
    int networkOrder[VIDEO_MAX_CH];

    (void)time;

    clock_gettime(CLOCK_MONOTONIC, &begin);
    struct window *window = (struct window *)data;
    // uint64_t start_time = 0;

    assert(window->callback == callback);
    window->callback = NULL;

    if (callback)
        wl_callback_destroy(callback);

    if (!window->configured)
        return;

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for(int i = 0;i<VIDEO_MAX_CH;i++)
    {
        if (v4l2_config[i].video_buf.video_fd == -1)
        {
            // Upload texture from logo image data
            glBindTexture(GL_TEXTURE_2D, window->gl.texture[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, VIDEO_WIDTH, VIDEO_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

        }else
        {
#ifdef USE_FILE_INPUT
            // file input: read one interleaved RGB24 frame from ffmpeg
            if (file_input_read_frame(i) == 0)
            {
                // capture time of the frame shown this render (also NPU input ts)
                uint64_t frame_ts_us = nc_get_mono_us_time();
                g_disp_frame_ts_us[i] = frame_ts_us;

                // TODO(merge): 서영 FSM -- send this frame to NPU or not
                {
                uint64_t time_stamp_us = frame_ts_us;
                networkOrder[i] = nc_get_cnn_networks_id();
                unsigned char* rgbdata_for_cnn = (unsigned char *)malloc(npu_input_info.rgb_size);
                nc_rgb_interleaved_to_planar_neon(g_file_input_frame[i] \
                                                ,(unsigned char *)rgbdata_for_cnn \
                                                ,(unsigned char *)rgbdata_for_cnn + NPU_INPUT_DATA_SIZE \
                                                ,(unsigned char *)rgbdata_for_cnn + NPU_INPUT_DATA_SIZE*2 \
                                                , NPU_INPUT_WIDTH, NPU_INPUT_HEIGHT);
                send_cnn_buf (rgbdata_for_cnn, time_stamp_us, (uint32_t)i, (E_NETWORK_UID)networkOrder[i]);
                }

                // Upload texture from file frame
                glBindTexture(GL_TEXTURE_2D, window->gl.texture[i]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, VIDEO_WIDTH, VIDEO_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, g_file_input_frame[i]);
        #ifdef SHOW_FPS
                printf(" -- fps(%0.1f)\n", calc_fps_at_loop_ent1(20));
        #endif
            }
#else
            struct v4l2_buffer video_buf;
            CLEAR(video_buf);

            // Queue the buffer for capturing a new frame
            if (nc_v4l2_dequeue_buffer(v4l2_config[i].video_buf.video_fd, &video_buf) == -1) {
                //printf("Error VIDIOC_DQBUF buffer %d\n", video_buf.index);
            }else
            {
                // capture(DQBUF) time of the frame shown this render (also NPU input ts)
                uint64_t frame_ts_us = nc_get_mono_us_time();
                g_disp_frame_ts_us[i] = frame_ts_us;
#if (VIDEO_MAX_CH == 4)
                if (((framecnt % 2 == 0) && (i == 0 || i == 2)) ||
                    ((framecnt % 2 == 1) && (i == 1 || i == 3)))
#endif
                // TODO(merge): 서영 FSM -- send this frame to NPU or not
                {
                    uint64_t time_stamp_us = frame_ts_us;
                    networkOrder[i] = nc_get_cnn_networks_id();
                    unsigned char* rgbdata_for_cnn = (unsigned char *)malloc(npu_input_info.rgb_size);
#ifdef USE_8MP_VI
                    memcpy(dsr_info.dsr_in_buf[0], (uint8_t *)v4l2_config[i].video_buf.buffers[video_buf.index].start, MAX_WIDTH_FOR_VDMA_CNN_DS*MAX_HEIGHT_FOR_VDMA_CNN_DS*RGB_CNT);
                    dsr_downscale(dsr_fd, &dsr_info, dsr_input_config, dsr_output_config, dsr_config, 0);
                    nc_rgb_interleaved_to_planar_neon((unsigned char *)dsr_info.dsr_out_buf[0] \
                                                    ,(unsigned char *)rgbdata_for_cnn \
                                                    ,(unsigned char *)rgbdata_for_cnn + NPU_INPUT_WIDTH*NPU_INPUT_HEIGHT \
                                                    ,(unsigned char *)rgbdata_for_cnn + NPU_INPUT_WIDTH*NPU_INPUT_HEIGHT*2 \
                                                    , NPU_INPUT_WIDTH,  NPU_INPUT_HEIGHT);
#else
                    memcpy(rgbdata_for_cnn, (unsigned char *)v4l2_config[i].video_buf.buffers[video_buf.index].start, npu_input_info.rgb_size); // 600us
#endif
                    send_cnn_buf (rgbdata_for_cnn, time_stamp_us, (uint32_t)i, (E_NETWORK_UID)networkOrder[i]);// 50us
                    // printf("send cnn msg : %llu us\n", nc_elapsed_us_time(start_time));
                }
#ifndef USE_8MP_VI
                unsigned char *interleaved_rgb = (unsigned char *)malloc(NPU_INPUT_WIDTH*NPU_INPUT_HEIGHT*3);
                // start_time = nc_get_mono_us_time();
                nc_rgb_planar_to_interleaved_neon((uint8_t*)v4l2_config[i].video_buf.buffers[video_buf.index].start \
                                            ,(uint8_t*)v4l2_config[i].video_buf.buffers[video_buf.index].start + NPU_INPUT_DATA_SIZE \
                                            ,(uint8_t*)v4l2_config[i].video_buf.buffers[video_buf.index].start + (NPU_INPUT_DATA_SIZE*2) \
                                            ,interleaved_rgb, NPU_INPUT_WIDTH, NPU_INPUT_HEIGHT);// NPU ON:3ms, NPU OFF:600us
                // printf("conv interleaved: %llu us\n", nc_elapsed_us_time(start_time));
#endif
                if(nc_v4l2_queue_buffer(v4l2_config[i].video_buf.video_fd, video_buf.index) == -1) {
                    printf("Error VIDIOC_QBUF buffer %d\n", video_buf.index);
                }

                // Upload texture from video buffer
                glBindTexture(GL_TEXTURE_2D, window->gl.texture[i]);
#ifdef USE_8MP_VI
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, VIDEO_WIDTH, VIDEO_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, v4l2_config[i].video_buf.buffers[video_buf.index].start);
#else
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, VIDEO_WIDTH, VIDEO_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, interleaved_rgb);
                if(interleaved_rgb){
                    // printf("free inter\n");
                    free(interleaved_rgb);
                }
#endif
        #ifdef SHOW_FPS
                printf(" -- fps(%0.1f)\n", calc_fps_at_loop_ent1(20));
        #endif
            }
#endif /* USE_FILE_INPUT */
        }

        // draw video texture
        glViewport(g_viewport[i].x, g_viewport[i].y, g_viewport[i].width, g_viewport[i].height);
        nc_opengl_draw_texture(window->gl.texture[i], window);
    }

#ifdef USE_ADAS_LD
    draw_ld_output_opengl();
#endif

    // draw cnn network result
    for(uint32_t ch = 0; ch < VIDEO_MAX_CH; ch++)
    {
        if (v4l2_config[ch].video_buf.video_fd == -1){

        }
        else{
            uint64_t time_stamp = 0;

        #ifdef DETECT_NETWORK
            pp_result_buf *det_buf = NULL;
            det_buf = (pp_result_buf *)nc_tsfs_ff_get_readable_buffer_and_timestamp(ch+DETECT_NETWORK, &time_stamp);
            if (det_buf) {
                // predict tracks to the capture time of the displayed video frame
                // (one short lock per frame), then draw. Runs before swap.
                nc_traj_predict_frame((int)ch, g_disp_frame_ts_us[ch]);
                nc_draw_gl_npu(g_viewport[ch], det_buf->net_task, det_buf, g_npu_prog);
            }
            nc_tsfs_ff_finish_read_buf(ch+DETECT_NETWORK);
        #endif

        #ifdef SEGMENT_NETWORK
            pp_result_buf *seg_buf = NULL;
            seg_buf = (pp_result_buf *)nc_tsfs_ff_get_readable_buffer_and_timestamp(ch+SEGMENT_NETWORK, &time_stamp);
            if (seg_buf) {
                nc_draw_gl_npu(g_viewport[ch], seg_buf->net_task, seg_buf, g_npu_prog);
            }
            nc_tsfs_ff_finish_read_buf(ch+SEGMENT_NETWORK);
        #endif

        #ifdef LANE_NETWORK
            pp_result_buf *lane_buf = NULL;
            lane_buf = (pp_result_buf *)nc_tsfs_ff_get_readable_buffer_and_timestamp(ch+LANE_NETWORK, &time_stamp);
            if (lane_buf) {
                nc_draw_gl_npu(g_viewport[ch], lane_buf->net_task, lane_buf, g_npu_prog);
            }
        #ifdef USE_LDW
            nc_ldw_app_process(ch, lane_buf);          // inside the buffer lock (before finish_read)
        #endif
            nc_tsfs_ff_finish_read_buf(ch+LANE_NETWORK);
        #ifdef USE_LDW
            {
                stNcLdwDrawRes ldw_res = { &font_24, &font_38, &g_font_prog, &g_npu_prog };
                nc_ldw_app_draw(ch, g_viewport[ch], &ldw_res);

                stNcMinimapDrawRes mm_res = { &font_24, &g_font_prog, &g_npu_prog };
                nc_minimap_draw(ch, g_viewport[ch], &mm_res);
            }
        #endif
        #endif
        }
    }

    framecnt++;
    fpscount+=1;
    char buftext[256];

    sprintf(buftext,"GL: %lums, Frame: %lums/%lufps", opengl_time, frametime, fpscount_00);

    glViewport(0,0, WINDOW_WIDTH, WINDOW_HEIGHT);
    float textcolor[3] = {1.0, 0.0, 0.0}; // R, G, B
    nc_opengl_draw_text(&font_38, buftext, 10, 1020, 1.0f, textcolor, WINDOW_WIDTH, WINDOW_HEIGHT, g_font_prog);

    clock_gettime(CLOCK_MONOTONIC, &end);

    opengl_time = ((end.tv_sec - begin.tv_sec)*1000 + (end.tv_nsec - begin.tv_nsec)/1000000);

    frametime = ((end.tv_sec - set_time.tv_sec)*1000 + (end.tv_nsec - set_time.tv_nsec)/1000000);
    nc_bevlog_row(NC_BEVLOG_FRAME, "%llu,%lu,%lu", (unsigned long long)nc_get_mono_us_time(),
                  (unsigned long)opengl_time, (unsigned long)frametime);

    set_time = end;

    fpstime = fpstime + frametime;

    if(fpstime>=1000)
    {
        if(fpscount_00>0)
        {
            fpscount_00 = (fpscount_00 + fpscount)/2;
        }
        else
        {
            fpscount_00=fpscount;
        }

        fpscount=0;
        fpstime=0;
    }

    nc_wayland_display_draw(window,(void *)render);
}

int npu_init(st_npu_input_info *npu_input_info)
{
    /* Initialize CNN */
    if (nc_aiw_init_cnn() < 0 ) {
        fprintf(stderr, "nc_aiw_init_cnn() failure!!\n");
        return -1;
    }
#ifdef SHOW_YOLOV8_DETECT
    if (nc_aiw_add_network_to_builder(nc_localize_path((const char *)NETWORK_FILE_YOLOV8_DET), NETWORK_YOLOV8_DET, nc_postprocess_yolov8_inference_result) < 0) {
        fprintf(stderr, "nc_aiw_add_network_to_builder() failure!!\n");
        return -1;
    }
#endif
#ifdef SHOW_YOLOV5_DETECT
    if (nc_aiw_add_network_to_builder(nc_localize_path((const char *)NETWORK_FILE_YOLOV5_DET), NETWORK_YOLOV5_DET, nc_postprocess_yolov5_inference_result) < 0) {
        fprintf(stderr, "nc_aiw_add_network_to_builder() failure!!\n");
        return -1;
    }
#endif
#ifdef SHOW_PELEE_SEG
    if (nc_aiw_add_network_to_builder(nc_localize_path((const char *)NETWORK_FILE_PELEE_SEG), NETWORK_PELEE_SEG, nc_postprocess_segmentation_inference_result) < 0) {
        fprintf(stderr, "nc_aiw_add_network_to_builder() failure!!\n");
        return -1;
    }
#endif
#ifdef SHOW_PELEE_DETECT
    if (nc_aiw_add_network_to_builder(nc_localize_path((const char *)NETWORK_FILE_PELEE_DET), NETWORK_PELEE_DET, nc_postprocess_pelee_inference_result) < 0) {
        fprintf(stderr, "nc_aiw_add_network_to_builder() failure!!\n");
        return -1;
    }
#endif
#ifdef SHOW_UFLD_LANE
    if (nc_aiw_add_network_to_builder(nc_localize_path((const char *)NETWORK_FILE_UFLD_LANE), NETWORK_UFLD_LANE, nc_postprocess_ufld_inference_result) < 0) {
        fprintf(stderr, "nc_aiw_add_network_to_builder() failure!!\n");
        return -1;
    }
#endif
#ifdef SHOW_TRI_CHIMERA
    if (nc_aiw_add_network_to_builder(nc_localize_path((const char *)NETWORK_FILE_TRI_CHIMERA), NETWORK_TRI_CHIMERA, nc_postprocess_trichimera_inference_result) < 0) {
        fprintf(stderr, "nc_aiw_add_network_to_builder() failure!!\n");
        return -1;
    }
#endif
    if(nc_aiw_finish_network_builder() < 0 ) {
        fprintf(stderr, "nc_aiw_finish_network_builder() failure!!\n");
        return -1;
    }
    // obtain the input resoltution for the CNN network
    // get information of the input tensor
    aiwTensorInfo in_tinfo;
#ifdef SHOW_YOLOV8_DETECT
    if(nc_get_cnn_network_input_resol(NETWORK_YOLOV8_DET, &in_tinfo) < 0)
    {
        printf("failed to get the input resolution for the CNN network\n");
    }
#endif
#ifdef SHOW_YOLOV5_DETECT
    if(nc_get_cnn_network_input_resol(NETWORK_YOLOV5_DET, &in_tinfo) < 0)
    {
        printf("failed to get the input resolution for the CNN network\n");
    }
#endif
#ifdef SHOW_PELEE_SEG
    if(nc_get_cnn_network_input_resol(NETWORK_PELEE_SEG, &in_tinfo) < 0)
    {
        printf("failed to get the input resolution for the CNN network\n");
    }
#endif
#ifdef SHOW_PELEE_DETECT
    if(nc_get_cnn_network_input_resol(NETWORK_PELEE_DET, &in_tinfo) < 0)
    {
        printf("failed to get the input resolution for the CNN network\n");
    }
#endif
#ifdef SHOW_UFLD_LANE
    if(nc_get_cnn_network_input_resol(NETWORK_UFLD_LANE, &in_tinfo) < 0)
    {
        printf("failed to get the input resolution for the CNN network\n");
    }
#endif
#ifdef SHOW_TRI_CHIMERA
    if(nc_get_cnn_network_input_resol(NETWORK_TRI_CHIMERA, &in_tinfo) < 0)
    {
        printf("failed to get the input resolution for the CNN network\n");
    }
#endif

    npu_input_info->w = in_tinfo.dim.w;// network input width
    npu_input_info->h = in_tinfo.dim.h;// network input height
    npu_input_info->rgb_size = in_tinfo.dim.w * in_tinfo.dim.h * RGB_CNT;

#ifdef USE_BYTETRACK
    for (int i = 0; i < VIDEO_MAX_CH; i++) {
        E_NETWORK_UID net_id;
#ifdef SHOW_PELEE_DETECT
        net_id = NETWORK_PELEE_DET;
#endif
#ifdef SHOW_YOLOV5_DETECT
        net_id = NETWORK_YOLOV5_DET;
#endif
#ifdef SHOW_YOLOV8_DETECT
        net_id = NETWORK_YOLOV8_DET;
#endif
#ifdef SHOW_TRI_CHIMERA
        net_id = NETWORK_TRI_CHIMERA;
#endif
#ifndef SHOW_PELEE_SEG
        if (nc_init_bytetrackers(CAP_FPS, i, net_id) != 0) {
            perror("nc_init_bytetrackers() error");
            return -1;
        }
#endif
    }
#endif

    printf("aiw finish\n");

    return 0;
}

static void signal_int()
{
    running = 0;
    nc_cnn_postprocess_stop();
}

static void usage(int error_code)
{
    fprintf(stderr, "Usage: simple-egl [OPTIONS]\n\n"
        "  -f\tRun in fullscreen mode\n"
        "  -o\tCreate an opaque surface\n"
#ifdef USE_FILE_INPUT
        "  -i <file>\tLoop one video file (default: play misc/image_npu/1920_1080_000[1-3].mp4 in turn)\n"
#endif
        "  -h\tThis help text\n\n");

    exit(error_code);
}

void *cnn_task(void *arg)
{
    (void) arg;
    printf("CNN TASK RUN!!\n");

    while (running) {
        stCnnData *cnn_data = NULL;
        if(receive_cnn_buf(&cnn_data) != -1){
            nc_aiw_run_cnn(cnn_data->ptr_cnn_buf, cnn_data->time_stamp_us, cnn_data->cam_ch, cnn_data->net_id);
            if(cnn_data) {
                free(cnn_data->ptr_cnn_buf);
                free(cnn_data);
            }
        }
    }

    printf("EXIT CNN_TASK!!\n");
    return NULL;
}

int main(int argc, char **argv)
{
    struct sigaction sigint;
    struct display display;
    struct window  window;
    int i, ret = 0;
    pthread_t p_thread[MAX_TASK_CNT];
    int task_cnt = 0;
    int thr_id;
    int status;

    for (i = 1; i < argc; i++) {
        if (strcmp("-f", argv[i]) == 0)
            window.fullscreen = 1;
        else if (strcmp("-o", argv[i]) == 0)
            window.opaque = 1;
        else if (strcmp("-h", argv[i]) == 0)
            usage(EXIT_SUCCESS);
#ifdef USE_FILE_INPUT
        else if (strcmp("-i", argv[i]) == 0 && i + 1 < argc)
            g_file_input_path = argv[++i];
#endif
        else
            usage(EXIT_FAILURE);
    }

    sigint.sa_handler = (sighandler_t)signal_int;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &sigint, NULL);

    memset(&display, 0, sizeof(display));
    memset(&window, 0, sizeof(window));
    memset(&v4l2_config, 0, sizeof(v4l2_config));

    set_v4l2_config();

    nc_init_path_localizer();

    window.display = &display;
    display.window = &window;
    window.window_size.width  = WINDOW_WIDTH;
    window.window_size.height = WINDOW_HEIGHT;

    set_viewport_config();
#ifdef USE_LDW
    nc_ldw_app_init(VIDEO_MAX_CH, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
#endif

    nc_wayland_display_init(&display,(void *)render);

    ret = v4l2_initialize();
    if(ret < 0) {
        printf("Error v4l2_initialize\n");
        return -1;
    }

    gl_initialize(&window);
#ifdef USE_8MP_VI
    dsr_init();
#endif
#ifdef USE_ADAS_LD
    ld_opengl_program_set(&g_npu_prog);
    NC_ADAS_OPEN();
#endif

    // create thread-safe flip-flop buffer
    for (int i = 0; i < VIDEO_MAX_CH; i++) {
        if (v4l2_config[i].video_buf.video_fd == -1){

        }
        else{
    #ifdef DETECT_NETWORK
            int det_buf_size = sizeof(pp_result_buf);
            if(nc_tsfs_ff_create_buffers(i+DETECT_NETWORK, det_buf_size) < 0) {
                exit(1);
            }
    #endif
    #ifdef SEGMENT_NETWORK
            int seg_buf_size = sizeof(pp_result_buf);
            if(nc_tsfs_ff_create_buffers(i+SEGMENT_NETWORK, seg_buf_size) < 0) {
                exit(1);
            }
    #endif
    #ifdef LANE_NETWORK
            int lane_buf_size = sizeof(pp_result_buf);
            if(nc_tsfs_ff_create_buffers(i+LANE_NETWORK, lane_buf_size) < 0) {
                exit(1);
            }
    #endif
        }
    }
    mq_unlink(MQ_NAME_CNN_BUF);

    if(npu_init(&npu_input_info) < 0) {
        printf("failed to init NPU\n");
        return -1;
    }

    printf("create tasks\n");
    thr_id = pthread_create(&p_thread[task_cnt++], NULL, cnn_task, (void *)NULL);
    if (thr_id < 0) {
        perror("thread create error : cnn_task");
        exit(1);
    }

    cnn_postprocess_arg cnn_post_param;
    cnn_post_param.target_width = WINDOW_WIDTH;
    cnn_post_param.target_height = WINDOW_HEIGHT;
    thr_id = pthread_create(&p_thread[task_cnt++], NULL, nc_cnn_postprocess_task, (void *)&cnn_post_param);
    if (thr_id < 0) {
        perror("thread create error : nc_cnn_postprocess_task");
        exit(1);
    }

#ifdef USE_ADAS_LD
    thr_id = pthread_create(&p_thread[task_cnt++], NULL, ld_task, NULL);
    if (thr_id < 0) {
        perror("thread create error : ld_task");
        exit(1);
    }
#endif

    while (running && ret != -1)
    {
        ret = wl_display_dispatch(display.display);
    }

    for(int i =0; i< task_cnt; i++) {
        pthread_join(p_thread[i], (void **)&status);
    }

#ifdef USE_ADAS_LD
    NC_ADAS_CLOSE();
#endif

#ifdef USE_BYTETRACK
    for (int i = 0; i < VIDEO_MAX_CH; i++) {
        nc_deInit_bytetrackers(i);
    }
#endif

#ifdef USE_8MP_VI
    dsr_deinit();
#endif
#ifdef USE_FILE_INPUT
    file_input_deinitialize();
#endif
    nc_wayland_display_destroy(&display);

    return 0;
}
