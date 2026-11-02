/**
********************************************************************************
* Copyright (C) 2021 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of 
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with 
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : neon_app.c
*
* @brief   : resize application for camera
*
* @author  : SW Solution team.  NextChip Inc.
*
* @date    : 2022.09.02.
*
* @version : 1.0.0
********************************************************************************
* @note
* 09.02.2022 / 1.0.0 / Initial released.
* 
********************************************************************************
*/ 
/*
********************************************************************************
*               INCLUDES
********************************************************************************
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/lp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <termios.h>
#include <errno.h>
#include <sys/signal.h>
#include <time.h>
#include <signal.h>
#include <poll.h>
#include <mqueue.h>
#include <pthread.h>
#include <linux/types.h>
#include <signal.h>
#include <poll.h>

#include "isp.h"
#include "isp_ioctl.h"

#include "nc_gpio.h"
#include "nc_utils.h"
#include "nc_jig_uart.h"
#include "nc_isp_helper.h"
#include "nc_app_config_parser.h"

#include "nc_neon.h"
#include "nc_opencv_wrapper.h"


#include <arm_neon.h>
/*
********************************************************************************
*               DEFINES
********************************************************************************
*/
// #define ENABLE_RESIZE_NC
// #define ENABLE_RESIZE_OPCV
// #define ENABLE_CONVERT_YUYV_TO_RGB
// #define ENABLE_CONVERT_NV12_TO_YUYV
#define ENABLE_CONVERT_TO_RGB24_TILED
// #define ENABLE_PIP_DOUBLE_PICTURE

#define MEM_CHECK       0
#define TIME_CHECK      1
#define COPY_TO_CURR    0
#define FILE_DUMP       1
#define EXIT_LOOP       0

#define RESIZE_TARGET_WIDTH     640
#define RESIZE_TARGET_HEIGHT    384


#define INI_FILE "misc/apache.ini"
#define OFRC_SYNC_WAIT_TIME 500

#define PIXEL_PER_BYTE_NV12          3 / 2
#define PIXEL_PER_BYTE_YUY2          2
#define PIXEL_PER_BYTE_RGB           3
#define PIXEL_PER_BYTE_YUV444        PIXEL_PER_BYTE_RGB
#define PIXEL_PER_BYTE_ARGB          4

/*
********************************************************************************
               TYPEDEFS
********************************************************************************
*/

/*
********************************************************************************
*               VARIABLE DECLARATIONS
********************************************************************************
*/
static int      g_isp_fd;
static int      g_ofrc_loop = 1;
static struct isp_resolution g_isp_resolution = {
    .width = IMAGESIZE_H_ACTIVE,
    .height = IMAGESIZE_V_ACTIVE
};

/*
********************************************************************************
*               STATIC FUNCTION DEFINITIONS
********************************************************************************
*/
static int nc_img_crop_yuyv__c(uint8_t* src_ptr, int32_t src_width, int32_t src_hight, int32_t src_offset,  
                               uint8_t* dst_ptr, int32_t dst_width, int32_t dst_hight)
{
    int ret = 0;
    int lvCnt = 0;

    int lvStart_x = src_offset % (src_width * 2);
    int lvStart_y = src_offset / (src_width * 2);
    int src_stride = src_width * 2;
    int dst_stride = dst_width * 2;

    for(lvCnt = 0; lvCnt < dst_hight; lvCnt++)
    {
        memcpy(dst_ptr + (lvCnt * dst_stride), src_ptr + (lvStart_x * 2) + ((lvStart_y + lvCnt) * src_stride), dst_stride);
    }

    return ret;
}
static int nc_img_paste_yuyv__c(uint8_t* src_ptr, int32_t src_width, int32_t src_height, int32_t dst_offset,
                                uint8_t* dst_ptr, int32_t dst_width, int32_t dst_height)
{
    int ret = 0;
    int lvCnt = 0;

    int lvStart_x = dst_offset % (dst_width*2);
    int lvStart_y = dst_offset / (dst_width*2);
    int src_stride = src_width * 2;
    int dst_stride = dst_width * 2;

    if (src_stride > (dst_stride - (lvStart_x * 2)))
        src_stride = (dst_stride - (lvStart_x * 2));

    if (src_height > (dst_height - lvStart_y))
        src_height = (dst_height - lvStart_y);

    for(lvCnt = 0; lvCnt < src_height; lvCnt++)
    {
        memcpy(dst_ptr + (lvStart_x * 2) + ((lvStart_y + lvCnt) * dst_stride), src_ptr + (lvCnt * src_width * 2), src_stride);
    }

    return ret;
}

static int nc_img_yuv422_to_yuv420sp_sub4w1h__c(uint8_t* yuv_ptr, uint8_t* nv_ptr, int32_t width, int32_t height)
{
    int ret = OK;
    int lvCnt = 0;

    uint8_t *Yplane, *UVplane;
    int32_t img_langth = ((width * height) / 4);
    
    Yplane = nv_ptr;
    UVplane = Yplane + (width * height);


    for(lvCnt = 0; lvCnt < img_langth; lvCnt++)
    {
        Yplane[(lvCnt * 4) + 0] = yuv_ptr[(lvCnt * 8) + 0];         // Y0
        Yplane[(lvCnt * 4) + 1] = yuv_ptr[(lvCnt * 8) + 2];         // Y1
        Yplane[(lvCnt * 4) + 2] = yuv_ptr[(lvCnt * 8) + 4];         // Y2
        Yplane[(lvCnt * 4) + 3] = yuv_ptr[(lvCnt * 8) + 6];         // Y3

        UVplane[(lvCnt * 2) + 0] = (yuv_ptr[(lvCnt * 8) + 1] + yuv_ptr[(lvCnt * 8) + 5]) / 2;   // U
        UVplane[(lvCnt * 2) + 1] = (yuv_ptr[(lvCnt * 8) + 3] + yuv_ptr[(lvCnt * 8) + 7]) / 2;   // V
    }

    return ret;
}
static int nc_img_yuv420sp_sub4w1h_to_yuv422__c(uint8_t* yuv_ptr, uint8_t* nv_ptr, int32_t width, int32_t height)
{
    int ret = OK;
    int lvCnt = 0;
    int lvCnt_W = 0, lvCnt_H = 0;

    uint8_t *Yplane, *UVplane;
    int32_t img_langth = ((width * height) / 4);
    
    Yplane = nv_ptr;
    UVplane = nv_ptr + (width * height);

    for(lvCnt = 0; lvCnt < img_langth; lvCnt++)
    {
        yuv_ptr[(lvCnt * 8) + 0] = Yplane[(lvCnt * 4) + 0];     // Y0
        yuv_ptr[(lvCnt * 8) + 1] = UVplane[(lvCnt * 4) + 0];    // U0
        yuv_ptr[(lvCnt * 8) + 2] = Yplane[(lvCnt * 4) + 1];     // Y1
        yuv_ptr[(lvCnt * 8) + 3] = UVplane[(lvCnt * 4) + 1];    // V0

        
        yuv_ptr[(lvCnt * 8) + 4] = Yplane[(lvCnt * 4) + 2];     // Y2
        yuv_ptr[(lvCnt * 8) + 5] = UVplane[(lvCnt * 4) + 2];    // U0
        yuv_ptr[(lvCnt * 8) + 6] = Yplane[(lvCnt * 4) + 3];     // Y3
        yuv_ptr[(lvCnt * 8) + 7] = UVplane[(lvCnt * 4) + 3];    // V0
    }



    return ret;
}
static int nc_img_nv12_to_yuv422__c(uint8_t* yuv_ptr, uint8_t* nv_ptr, int32_t width, int32_t height)
{
    int32_t ret = OK;
    int32_t lvCnt_W = 0, lvCnt_H = 0;

    uint8_t *Yplane, *UVplane;
    
    Yplane = nv_ptr;
    UVplane = nv_ptr + (width * height);

    for(lvCnt_H = 0; lvCnt_H < height; lvCnt_H++)
    {
        for(lvCnt_W = 0; lvCnt_W < width/2; lvCnt_W++)
        {
            yuv_ptr[((lvCnt_W * 4) + 0 + ((width * 2) * lvCnt_H))] = Yplane[(lvCnt_W  * 2) + (width)*(lvCnt_H) + 0];     // Y0
            yuv_ptr[((lvCnt_W * 4) + 1 + ((width * 2) * lvCnt_H))] = UVplane[(lvCnt_W * 2) + (width)*(lvCnt_H/2) + 0];    // U0
            yuv_ptr[((lvCnt_W * 4) + 2 + ((width * 2) * lvCnt_H))] = Yplane[(lvCnt_W  * 2) + (width)*(lvCnt_H) + 1];     // Y1
            yuv_ptr[((lvCnt_W * 4) + 3 + ((width * 2) * lvCnt_H))] = UVplane[(lvCnt_W * 2) + (width)*(lvCnt_H/2) + 1];    // V0
        }
    }


    return ret;
}
#define TILESIZE    64
static int nc_rgb24_packed_to_tiled__c(struct img_info *src_info, struct img_info *dst_info)
{
    int ret = OK;
    int in_x_idx, in_y_idx;
    int outidx = 0;
    int plane_offset = dst_info->width * dst_info->height;
    
    for (long y_super = 0; y_super < (src_info->height / TILESIZE); y_super++)
    {
        for (long x_super = 0; x_super < (src_info->width / TILESIZE); x_super++)
        {
            for (long y_sub = 0; y_sub < 8; y_sub++)
            {
                for (long x_sub = 0; x_sub < 8; x_sub++)
                {
                    for (long y_pix = 0; y_pix < 8; y_pix++)
                    {
                        in_y_idx = y_super * TILESIZE + y_sub * 8 + y_pix;

                        for (long x_pix = 0; x_pix < 8; x_pix++)
                        {
                            in_x_idx = x_super * TILESIZE*3 + x_sub * 8*3 + x_pix*3;
                            dst_info->buff[outidx + (plane_offset * 0)] = src_info->buff[in_y_idx*(src_info->width*3) + in_x_idx + 0];  // r
                            dst_info->buff[outidx + (plane_offset * 1)] = src_info->buff[in_y_idx*(src_info->width*3) + in_x_idx + 1];  // g
                            dst_info->buff[outidx + (plane_offset * 2)] = src_info->buff[in_y_idx*(src_info->width*3) + in_x_idx + 2];  // b
                            outidx++;
                        }
                    }
                }
            }
        }
    }
    printf("done! n");

    return ret;
}
/*
********************************************************************************
*               FUNCTION DEFINITIONS
********************************************************************************
*/
int ofrc_get_sync(struct pollfd *p)
{
    char ch;
    static int timeout_ms = OFRC_SYNC_WAIT_TIME;
    static int poll_cnt = 0;
    int synced;

    synced = poll(p, 1, timeout_ms);
    if (!synced) {
        printf("[ofrc_get_sync] timeout!!! (%d ms)\n", timeout_ms);
        return -1;
    }
    read(p->fd, &ch, 1);
    lseek(p->fd, 0, SEEK_SET);

    poll_cnt++;
    if (poll_cnt == 2)
        timeout_ms = 37; // 30frames needs 33.3ms so make timeout with 10% buffers

    return atoi(&ch);
}

int ofrc_draw(unsigned char ofrc_index)
{
    int ret;

    if ((ret = ioctl(g_isp_fd, NC_ISP_IOCTL_OFRC_BUFFER_SYNC, &ofrc_index)) < 0) {
        printf("NC_ISP_IOCTL_OFRC_BUFFER_SYNC failure(%d)\n", ret);
    }

    return ret;
}

void ofrc_get_oriSrc(uint8_t* ofrc_buf_ori, int32_t size, char *ppfilename)
{
    char *filename = "./ofrc_wr_0_1860_2880.yuv";
    // char *filename = "./ofrc_wr_0_1280x720.nv12";
    FILE *fp;

    if (ppfilename != NULL)
        filename = ppfilename;

    fp = fopen(filename, "rb");
    if (fp != NULL) {
        fread(ofrc_buf_ori, size, 1, fp);
        fclose(fp);
        printf("%s read success\n", filename);
    }
}

void ofrc_dump(uint8_t* ofrc_buf_base, int32_t img_size)
{
    static int cnt = 0;
    char filename[255] = {0,};
    FILE *fp;

    sprintf(filename, "./ofrc_%d.raw", cnt);
    fp = fopen(filename, "wb");
    if (fp != NULL) {
        fwrite(ofrc_buf_base, img_size, 1, fp);
        fclose(fp);
        printf("%d\n", cnt++);
    }
}

void *ofrc_task(void *arg)
{
    int isp_fd = *((int *)arg);
    int ofrc_index;
    struct pollfd poll_handle;
    uint64_t s_time;
    uint64_t s_time_buf, s_time_buf_Max = 0, s_time_buf_Min = 1000*33;
    int test_cnt = 0;
    int img_stride = 0;
    uint8_t *ofrc_curr_buf = NULL;

#if defined(ENABLE_RESIZE_NC)
    struct img_info src_img;
    struct img_info dst_img;
#elif defined(ENABLE_RESIZE_OPCV)
    struct img_info src_img;
    struct img_info before_resize_img;
    struct img_info after_resize_img;
    struct img_info resized_packed_img;
    struct img_info pip_img;
#elif defined(ENABLE_CONVERT_YUYV_TO_RGB)
    struct img_info src_img;
    struct img_info rgb_img;
#elif defined(ENABLE_CONVERT_NV12_TO_YUYV)
    struct img_info nv12_img;
    struct img_info dst_img;
#elif defined(ENABLE_CONVERT_TO_RGB24_TILED)
    struct img_info src_img;
    struct img_info before_resize_img;
    struct img_info after_resize_img;
    struct img_info resized_packed_img;
    struct img_info rgb24_packed_img;
    struct img_info rgb24_tiled_img;

    uint64_t memory_size_check = 0;
    uint64_t memory_size_sum = 0;
#elif defined(ENABLE_PIP_DOUBLE_PICTURE)
    struct img_info src_img;
    struct img_info crop1_img;
    struct img_info crop2_img;
    struct img_info resize1_img;
    struct img_info resize2_img;
    struct img_info dst_img;

    uint64_t s_time_crop = 0;
    uint64_t s_time_resize = 0;
    uint64_t s_time_paste = 0;
    uint64_t s_time_total = 0;

    uint64_t s_time_avr_crop = 0;
    uint64_t s_time_avr_resize = 0;
    uint64_t s_time_avr_paste = 0;
    uint64_t s_time_avr_total = 0;
    
    uint64_t s_time_min_crop = 33000;
    uint64_t s_time_min_resize = 1000000;
    uint64_t s_time_min_paste = 33000;
    uint64_t s_time_min_total = 1000000;
    
    uint64_t s_time_max_crop = 0;
    uint64_t s_time_max_resize = 0;
    uint64_t s_time_max_paste = 0;
    uint64_t s_time_max_total = 0;
#endif


    printf("ofrc_task() start!! \n");
    poll_handle = nc_open_ofrc_sync_pollfd();
    if (!poll_handle.fd) {
        printf("nc_open_ofrc_sync_pollfd() failure\n");
        return (void *)ENOENT;
    }

    s_ofrc_buf_info ofrc_buf_info;
    int ret = nc_mmap_ofrc_bufs(isp_fd, &ofrc_buf_info);
    if (ret < 0) {
         printf("nc_mmap_ofrc_bufs failed\n");
         return (void*)ENOMEM;
    }

#if defined(ENABLE_RESIZE_NC)    
    src_img.width = g_isp_resolution.width;
    src_img.height = g_isp_resolution.height;
    src_img.buff = (uint8_t *)malloc(src_img.width * src_img.height * PIXEL_PER_BYTE_YUY2);

    dst_img.width = RESIZE_TARGET_WIDTH;
    dst_img.height = RESIZE_TARGET_HEIGHT;
    dst_img.buff = (uint8_t *)malloc(dst_img.width * dst_img.height * PIXEL_PER_BYTE_YUY2);
#elif defined(ENABLE_RESIZE_OPCV)
    src_img.width = g_isp_resolution.width;
    src_img.height = g_isp_resolution.height;
    src_img.buff = (uint8_t *)malloc(src_img.width * src_img.height * PIXEL_PER_BYTE_YUY2);

    before_resize_img.width = g_isp_resolution.width;
    before_resize_img.height = g_isp_resolution.height;
    before_resize_img.buff = (uint8_t *)malloc(before_resize_img.width * before_resize_img.height * PIXEL_PER_BYTE_YUY2);

    after_resize_img.width = RESIZE_TARGET_WIDTH;
    after_resize_img.height = RESIZE_TARGET_HEIGHT;
    after_resize_img.buff = (uint8_t *)malloc(after_resize_img.width * after_resize_img.height * PIXEL_PER_BYTE_YUY2);

    resized_packed_img.width = after_resize_img.width;
    resized_packed_img.height = after_resize_img.height;
    resized_packed_img.buff = (uint8_t *)malloc(resized_packed_img.width * resized_packed_img.height * PIXEL_PER_BYTE_YUY2);
    
    pip_img.width = g_isp_resolution.width;
    pip_img.height = g_isp_resolution.height;
    pip_img.buff = (uint8_t *)malloc(pip_img.width * pip_img.height * PIXEL_PER_BYTE_YUY2);
    memset(pip_img.buff, 0, pip_img.width * pip_img.height * PIXEL_PER_BYTE_YUY2);
#elif defined(ENABLE_CONVERT_YUYV_TO_RGB)
    src_img.width = g_isp_resolution.width;
    src_img.height = g_isp_resolution.height;
    src_img.buff = (uint8_t *)malloc(src_img.width * src_img.height * PIXEL_PER_BYTE_YUY2);
    
    rgb_img.width = g_isp_resolution.width;
    rgb_img.height = g_isp_resolution.height;
    rgb_img.buff = (uint8_t *)malloc(rgb_img.width * rgb_img.height * PIXEL_PER_BYTE_RGB);
#elif defined(ENABLE_CONVERT_NV12_TO_YUYV)
    nv12_img.width = g_isp_resolution.width;
    nv12_img.height = g_isp_resolution.height;
    nv12_img.buff = (uint8_t *)malloc(nv12_img.width * nv12_img.height * PIXEL_PER_BYTE_NV12);
    ofrc_get_oriSrc(nv12_img.buff, (nv12_img.width * nv12_img.height * PIXEL_PER_BYTE_NV12), "./ofrc_wr_0_1280x720.nv12");

    dst_img.width = g_isp_resolution.width;
    dst_img.height = g_isp_resolution.height;
    dst_img.buff = (uint8_t *)malloc(dst_img.width * dst_img.height * PIXEL_PER_BYTE_RGB);
#elif defined(ENABLE_CONVERT_TO_RGB24_TILED)
    src_img.width = g_isp_resolution.width;
    src_img.height = g_isp_resolution.height;
    src_img.buff = (uint8_t *)malloc(src_img.width * src_img.height * PIXEL_PER_BYTE_YUY2);

    before_resize_img.width = g_isp_resolution.width;
    before_resize_img.height = g_isp_resolution.height;
    before_resize_img.buff = (uint8_t *)malloc(before_resize_img.width * before_resize_img.height * PIXEL_PER_BYTE_YUY2);

    after_resize_img.width = RESIZE_TARGET_WIDTH;
    after_resize_img.height = RESIZE_TARGET_HEIGHT;
    after_resize_img.buff = (uint8_t *)malloc(after_resize_img.width * after_resize_img.height * PIXEL_PER_BYTE_YUY2);

    resized_packed_img.width = after_resize_img.width;
    resized_packed_img.height = after_resize_img.height;
    resized_packed_img.buff = (uint8_t *)malloc(resized_packed_img.width * resized_packed_img.height * PIXEL_PER_BYTE_YUY2);

    rgb24_packed_img.width = resized_packed_img.width;
    rgb24_packed_img.height = resized_packed_img.height;
    rgb24_packed_img.buff = (uint8_t *)malloc(rgb24_packed_img.width * rgb24_packed_img.height * PIXEL_PER_BYTE_RGB);
    memset(rgb24_packed_img.buff, 0, rgb24_packed_img.width * rgb24_packed_img.height * PIXEL_PER_BYTE_RGB);
    
    rgb24_tiled_img.width = rgb24_packed_img.width;
    rgb24_tiled_img.height = rgb24_packed_img.height;
    rgb24_tiled_img.buff = (uint8_t *)malloc(rgb24_tiled_img.width * rgb24_tiled_img.height * PIXEL_PER_BYTE_RGB);
    memset(rgb24_tiled_img.buff, 0, rgb24_tiled_img.width * rgb24_tiled_img.height * PIXEL_PER_BYTE_RGB);
#elif defined(ENABLE_PIP_DOUBLE_PICTURE)
    #define CL_IV_CRP_OFFSET    0
    #define CL_II_CRP_OFFSET    (976 * 1860 + 25) * 2
    #define MT_IV_PASTE_OFFSET  720 * 1280 * PIXEL_PER_BYTE_YUY2

    src_img.width = 1860;
    src_img.height = 2880;
    src_img.buff = (uint8_t *)malloc(src_img.width * src_img.height * PIXEL_PER_BYTE_YUY2);
    ofrc_get_oriSrc(src_img.buff, (src_img.width * src_img.height * PIXEL_PER_BYTE_YUY2), 0);

    crop1_img.width = 1860;
    crop1_img.height = 2200;
    crop1_img.buff = (uint8_t *)malloc(crop1_img.width * crop1_img.height * PIXEL_PER_BYTE_YUY2);

    crop2_img.width = 936;
    crop2_img.height = 1668;
    crop2_img.buff = (uint8_t *)malloc(crop2_img.width * crop2_img.height * PIXEL_PER_BYTE_YUY2);

    resize1_img.width = 720;
    resize1_img.height = 640;
    resize1_img.buff = (uint8_t *)malloc(resize1_img.width * resize1_img.height * PIXEL_PER_BYTE_YUY2);

    resize2_img.width = 720;
    resize2_img.height = 1280;
    resize2_img.buff = (uint8_t *)malloc(resize2_img.width * resize2_img.height * PIXEL_PER_BYTE_YUY2);

    dst_img.width = 720;
    dst_img.height = 1920;
    dst_img.buff = (uint8_t *)malloc(dst_img.width * dst_img.height * PIXEL_PER_BYTE_YUY2);
#endif
    
    while (g_ofrc_loop) {
        // wait with poll
        ofrc_index = nc_get_ofrc_sync_idx(&poll_handle);
        if (ofrc_index < 0)
            continue;

        test_cnt++;
        s_time_buf = 0;
        s_time = nc_get_mono_us_time();

#if defined(ENABLE_RESIZE_NC)
        img_stride = g_isp_resolution.width * PIXEL_PER_BYTE_YUY2;
        ofrc_curr_buf = ((uint8_t *) ofrc_buf_info.ofrc_mmap_buf) + (img_stride * g_isp_resolution.height * ofrc_index);
        memcpy(src_img.buff, ofrc_curr_buf, img_stride * g_isp_resolution.height);
        if (src_img.width == (dst_img.width * 2)) {
            if (src_img.height == dst_img.height) {
                nc_resize_yuy2_halfwidth(src_img.buff, dst_img.buff, src_img.width, src_img.height);
            }
            else
            {
                nc_resize_yuy2_hafwid_verti_scaledn(src_img.buff, dst_img.buff, src_img.width, src_img.height, dst_img.height);
            }
            nc_img_paste_yuyv__c(dst_img.buff, dst_img.width, dst_img.height, 0,
                                ofrc_curr_buf, g_isp_resolution.width, g_isp_resolution.height);
        }
        else
        {
            nc_pip_image_bilinear_yuyv(src_img.buff, src_img.width, src_img.height, dst_img.width, dst_img.height,
                                    ofrc_curr_buf, g_isp_resolution.width, g_isp_resolution.height,
                                    0, 0);
        }
#if TIME_CHECK
        if (test_cnt > 30) {
            s_time_buf = nc_elapsed_us_time(s_time);
            if (s_time_buf > s_time_buf_Max)
                s_time_buf_Max = s_time_buf;
            if (s_time_buf < s_time_buf_Min)
                s_time_buf_Min = s_time_buf;
            
            printf("[total time] Now : %lld us , Max : %lld us , Min %lld us  \n", s_time_buf, s_time_buf_Max, s_time_buf_Min);
        }
#endif
#if FILE_DUMP
        if (test_cnt == 30) {
            ofrc_dump(src_img.buff, src_img.width * src_img.height * PIXEL_PER_BYTE_YUY2);
            ofrc_dump(dst_img.buff, dst_img.width * dst_img.height * PIXEL_PER_BYTE_YUY2);
            ofrc_dump(ofrc_curr_buf, g_isp_resolution.width * g_isp_resolution.height * PIXEL_PER_BYTE_YUY2);
        }
#endif
        nc_ofrc_draw(isp_fd, &ofrc_buf_info, ofrc_index, OFF_MIX_GUI, NULL);
#if EXIT_LOOP
        if (test_cnt > 30)
            break;
#endif
#elif defined(ENABLE_RESIZE_OPCV)    // copy -> resize(openCV) -> dump
        if (test_cnt > 19) {
            ofrc_curr_buf = ((uint8_t *) ofrc_buf_info.ofrc_mmap_buf) + (g_isp_resolution.width * g_isp_resolution.height * PIXEL_PER_BYTE_YUY2 * ofrc_index);
            memcpy(src_img.buff, ofrc_curr_buf, src_img.width * src_img.height * PIXEL_PER_BYTE_YUY2);
            nc_img_yuyv_packed_to_planar(src_img.buff, before_resize_img.buff, src_img.width, src_img.height);
            nc_opencv_yuv422p_resize(&before_resize_img, &after_resize_img);
            nc_img_yuyv_planar_to_packed(after_resize_img.buff, resized_packed_img.buff, after_resize_img.width, after_resize_img.height);
#if TIME_CHECK
        if (test_cnt > 30) {
            s_time_buf = nc_elapsed_us_time(s_time);
            if (s_time_buf > s_time_buf_Max)
                s_time_buf_Max = s_time_buf;
            if (s_time_buf < s_time_buf_Min)
                s_time_buf_Min = s_time_buf;
            
            printf("[total time] Now : %lld us , Max : %lld us , Min %lld us  \n", s_time_buf, s_time_buf_Max, s_time_buf_Min);
        }
#endif
#if COPY_TO_CURR
            nc_img_paste_yuyv__c(resized_packed_img.buff, resized_packed_img.width, resized_packed_img.height, 0,
                                pip_img.buff, pip_img.width, pip_img.height);
            memcpy(ofrc_curr_buf, pip_img.buff, pip_img.width * pip_img.height * PIXEL_PER_BYTE_YUY2);
#endif
#if FILE_DUMP
            if (test_cnt == 30)
            {
                ofrc_dump(src_img.buff, g_isp_resolution.width * g_isp_resolution.height * PIXEL_PER_BYTE_YUY2);
                ofrc_dump(before_resize_img.buff, before_resize_img.width * before_resize_img.height * PIXEL_PER_BYTE_YUY2);
                ofrc_dump(after_resize_img.buff, after_resize_img.width * after_resize_img.height * PIXEL_PER_BYTE_YUY2);
                ofrc_dump(resized_packed_img.buff,  resized_packed_img.width * resized_packed_img.height * PIXEL_PER_BYTE_YUY2);
                ofrc_dump(pip_img.buff,  pip_img.width * pip_img.height * PIXEL_PER_BYTE_YUY2);
            }
#endif
            nc_ofrc_draw(isp_fd, &ofrc_buf_info, ofrc_index, OFF_MIX_GUI, NULL);
#if EXIT_LOOP 
            if (test_cnt > 19)
                break;
#endif
        }
#elif defined(ENABLE_CONVERT_YUYV_TO_RGB)
        if (test_cnt > 19) {
            ofrc_curr_buf = ((uint8_t *) ofrc_buf_info.ofrc_mmap_buf) + (g_isp_resolution.width * g_isp_resolution.height * PIXEL_PER_BYTE_YUY2 * ofrc_index);
            memcpy(src_img.buff, ofrc_curr_buf, g_isp_resolution.width * g_isp_resolution.height * PIXEL_PER_BYTE_YUY2);
            nc_opencv_yuv422_to_rgb(&src_img, &rgb_img);
#if TIME_CHECK
        if (test_cnt > 30) {
            s_time_buf = nc_elapsed_us_time(s_time);
            if (s_time_buf > s_time_buf_Max)
                s_time_buf_Max = s_time_buf;
            if (s_time_buf < s_time_buf_Min)
                s_time_buf_Min = s_time_buf;
            
            printf("[convert time] Now : %lld us , Max : %lld us , Min %lld us  \n", s_time_buf, s_time_buf_Max, s_time_buf_Min);
        }
#endif
#if FILE_DUMP
            if (test_cnt == 20) {
                ofrc_dump(src_img.buff, g_isp_resolution.width * g_isp_resolution.height * PIXEL_PER_BYTE_YUY2);
                ofrc_dump(rgb_img.buff, g_isp_resolution.width * g_isp_resolution.height * PIXEL_PER_BYTE_RGB);
            }
#endif
            nc_ofrc_draw(isp_fd, &ofrc_buf_info, ofrc_index, OFF_MIX_GUI, NULL);
#if EXIT_LOOP
            if (test_cnt > 19)
                break;
#endif
        }
#elif defined(ENABLE_CONVERT_NV12_TO_YUYV)
        if (test_cnt > 19) {
            img_stride = g_isp_resolution.width * PIXEL_PER_BYTE_YUY2;
            ofrc_curr_buf = ((uint8_t *) ofrc_buf_info.ofrc_mmap_buf) + (img_stride * g_isp_resolution.height * ofrc_index);
            nc_img_nv12_to_yuv422(nv12_img.buff, dst_img.buff, nv12_img.width, nv12_img.height);
#if TIME_CHECK
            if (test_cnt > 30) {
                s_time_buf = nc_elapsed_us_time(s_time);
                if (s_time_buf > s_time_buf_Max)
                    s_time_buf_Max = s_time_buf;
                if (s_time_buf < s_time_buf_Min)
                    s_time_buf_Min = s_time_buf;
                
                printf("[total time] Now : %lld us , Max : %lld us , Min %lld us  \n", s_time_buf, s_time_buf_Max, s_time_buf_Min);
            }
#endif
#if COPY_TO_CURR
            memcpy(ofrc_curr_buf, dst_img.buff, dst_img.width * dst_img.height * PIXEL_PER_BYTE_YUY2);
#endif
#if FILE_DUMP
            if (test_cnt == 20)
            {
                ofrc_dump(nv12_img.buff, nv12_img.width * nv12_img.height * PIXEL_PER_BYTE_NV12);
                ofrc_dump(dst_img.buff, dst_img.width * dst_img.height * PIXEL_PER_BYTE_YUY2);
            }
#endif
            nc_ofrc_draw(isp_fd, &ofrc_buf_info, ofrc_index, OFF_MIX_GUI, NULL);
#if EXIT_LOOP
            if (test_cnt > 19)
                break;
#endif
        }
#elif defined(ENABLE_CONVERT_TO_RGB24_TILED)  // src img copy -> yuv packed to planar -> resize -> yuv planar to packed -> yuv422 packed to rgb24 packed -> rgb24 packed to rgb24 tiled -> dump(rgb24 packed, rgb24 tiled)
        if (test_cnt > 19) {
#if MEM_CHECK
            memory_size_check = nc_get_free_mem_size();
#endif
            img_stride = g_isp_resolution.width * PIXEL_PER_BYTE_YUY2;
            ofrc_curr_buf = ((uint8_t *) ofrc_buf_info.ofrc_mmap_buf) + (img_stride * g_isp_resolution.height * ofrc_index);
            memcpy(src_img.buff, ofrc_curr_buf, img_stride * g_isp_resolution.height);
            #if 0       // OpenCV resize
            nc_img_yuyv_packed_to_planar(src_img.buff, before_resize_img.buff, before_resize_img.width, before_resize_img.height);
            nc_opencv_yuv422p_resize(&before_resize_img, &after_resize_img);
            nc_img_yuyv_planar_to_packed(after_resize_img.buff, resized_packed_img.buff, after_resize_img.width, after_resize_img.height);
            #elif 0     // Ne10 normal resize
            nc_resize_image_bilinear_yuyv(src_img.buff, src_img.width*2, src_img.width, src_img.height, 
                                       resized_packed_img.buff, resized_packed_img.width*2, resized_packed_img.width, resized_packed_img.height);
            #else       // Ne10 half width resize
            nc_resize_yuy2_hafwid_verti_scaledn(src_img.buff, resized_packed_img.buff, src_img.width, src_img.height, resized_packed_img.height);
            #endif
            nc_opencv_yuv422_to_rgb(&resized_packed_img, &rgb24_packed_img);
            nc_img_rgb24_packed_to_tiled_planar(rgb24_packed_img.buff, rgb24_tiled_img.buff, rgb24_tiled_img.width, rgb24_tiled_img.height);
#if MEM_CHECK
            memory_size_check -= nc_get_free_mem_size();
            memory_size_sum += memory_size_check;
            printf("expanded memory: %lld byte, %lld byte, %lld \n", memory_size_check, memory_size_sum, test_cnt);
#endif
#if TIME_CHECK
            if (test_cnt > 30) {
                s_time_buf = nc_elapsed_us_time(s_time);
                if (s_time_buf > s_time_buf_Max)
                    s_time_buf_Max = s_time_buf;
                if (s_time_buf < s_time_buf_Min)
                    s_time_buf_Min = s_time_buf;
                
                printf("[total time] Now : %lld us , Max : %lld us , Min %lld us  \n", s_time_buf, s_time_buf_Max, s_time_buf_Min);
            }
#endif
        }
#if FILE_DUMP
        if (test_cnt == 20) {
            ofrc_dump(after_resize_img.buff, after_resize_img.width * after_resize_img.height * PIXEL_PER_BYTE_YUY2);
            
            ofrc_dump(rgb24_packed_img.buff, rgb24_packed_img.width * rgb24_packed_img.height * PIXEL_PER_BYTE_RGB);
            ofrc_dump(rgb24_tiled_img.buff, rgb24_tiled_img.width * rgb24_tiled_img.height * PIXEL_PER_BYTE_RGB);

            ofrc_dump(rgb24_tiled_img.buff, rgb24_tiled_img.width * rgb24_tiled_img.height);
            ofrc_dump(rgb24_tiled_img.buff + (rgb24_tiled_img.width * rgb24_tiled_img.height), rgb24_tiled_img.width * rgb24_tiled_img.height);
            ofrc_dump(rgb24_tiled_img.buff + (rgb24_tiled_img.width * rgb24_tiled_img.height) * 2, rgb24_tiled_img.width * rgb24_tiled_img.height);
        }
#endif
        nc_ofrc_draw(isp_fd, &ofrc_buf_info, ofrc_index, OFF_MIX_GUI, NULL);
#if EXIT_LOOP
        if (test_cnt > 19)
            break;
#endif
#elif defined(ENABLE_PIP_DOUBLE_PICTURE) // img load -> crop -> resize(scale down) -> paste -> dump
        s_time_buf = nc_elapsed_us_time(s_time);
        nc_img_crop_yuyv__c(src_img.buff, src_img.width, src_img.height, CL_IV_CRP_OFFSET,
                        crop1_img.buff, crop1_img.width, crop1_img.height);
        nc_img_crop_yuyv__c(src_img.buff, src_img.width, src_img.height, CL_II_CRP_OFFSET,
                        crop2_img.buff, crop2_img.width, crop2_img.height);
        s_time_crop = nc_elapsed_us_time(s_time) - s_time_buf;

        s_time_buf = nc_elapsed_us_time(s_time);
        nc_resize_image_bilinear_yuyv(crop1_img.buff, crop1_img.width * PIXEL_PER_BYTE_YUY2, crop1_img.width, crop1_img.height,
                                   resize1_img.buff, resize1_img.width * PIXEL_PER_BYTE_YUY2, resize1_img.width, resize1_img.height);    
        nc_resize_image_bilinear_yuyv(crop2_img.buff, crop2_img.width * PIXEL_PER_BYTE_YUY2, crop2_img.width, crop2_img.height,
                                   resize2_img.buff, resize2_img.width * PIXEL_PER_BYTE_YUY2, resize2_img.width, resize2_img.height);
        s_time_resize = nc_elapsed_us_time(s_time) - s_time_buf;

        s_time_buf = nc_elapsed_us_time(s_time);
        nc_img_paste_yuyv__c(resize1_img.buff, resize1_img.width, resize1_img.height, MT_IV_PASTE_OFFSET,
                         dst_img.buff, dst_img.width, dst_img.height);
        nc_img_paste_yuyv__c(resize2_img.buff, resize2_img.width, resize2_img.height, 0,
                         dst_img.buff, dst_img.width, dst_img.height);
        s_time_total = nc_elapsed_us_time(s_time);
        s_time_paste = s_time_total - s_time_buf;
#if TIME_CHECK
        if (test_cnt > 30) {
            if (s_time_crop > s_time_max_crop)
                s_time_max_crop = s_time_crop;
            if (s_time_crop < s_time_min_crop)
                s_time_min_crop = s_time_crop;
            printf("[crop time]     Now : %lld us , Max : %lld us , Min %lld us  \n", s_time_crop, s_time_max_crop, s_time_min_crop);
            
            if (s_time_resize > s_time_max_resize)
                s_time_max_resize = s_time_resize;
            if (s_time_resize < s_time_min_resize)
                s_time_min_resize = s_time_resize;
            printf("[resize time]   Now : %lld us , Max : %lld us , Min %lld us  \n", s_time_resize, s_time_max_resize, s_time_min_resize);
            
            if (s_time_paste > s_time_max_paste)
                s_time_max_paste = s_time_paste;
            if (s_time_paste < s_time_min_paste)
                s_time_min_paste = s_time_paste;
            printf("[paste time]    Now : %lld us , Max : %lld us , Min %lld us  \n", s_time_paste, s_time_max_paste, s_time_min_paste);
            
            if (s_time_total > s_time_max_total)
                s_time_max_total = s_time_total;
            if (s_time_total < s_time_min_total)
                s_time_min_total = s_time_total;
            printf("[total time]    Now : %lld us , Max : %lld us , Min %lld us  \n", s_time_total, s_time_max_total, s_time_min_total);
            printf(" \n");
        }
#endif
        s_time_avr_crop += s_time_crop;
        s_time_avr_resize += s_time_resize;
        s_time_avr_paste += s_time_paste;
        s_time_avr_total += s_time_total;
#if FILE_DUMP
        if (test_cnt == 20) {
            ofrc_dump(dst_img.buff, 720 * 1920 * PIXEL_PER_BYTE_YUY2);
        }
#endif
        nc_ofrc_draw(isp_fd, &ofrc_buf_info, ofrc_index, OFF_MIX_GUI, NULL);
#if EXIT_LOOP
        if (test_cnt > 19) {
            
#if TIME_CHECK
            printf("\n\n\n\n\n");
            printf("crop time: %lld \n", s_time_avr_crop/test_cnt);
            printf("resize time: %lld \n", s_time_avr_resize/test_cnt);
            printf("paste time: %lld \n", s_time_avr_paste/test_cnt);
            printf("total time: %lld \n", s_time_avr_total/test_cnt);
#endif
            break;
        }
#endif
#endif
    }

#if defined(ENABLE_RESIZE_NC)
    if (src_img.buff)
        free(src_img.buff);
    if (dst_img.buff)
        free(dst_img.buff);
#elif defined(ENABLE_RESIZE_OPCV)
    if (src_img.buff)
        free(src_img.buff);
    if (before_resize_img.buff)
        free(before_resize_img.buff);
    if (after_resize_img.buff)
        free(after_resize_img.buff);
    if (resized_packed_img.buff)
        free(resized_packed_img.buff);
    if (pip_img.buff)
        free(pip_img.buff);
#elif defined(ENABLE_CONVERT_YUYV_TO_RGB)
    if (src_img.buff)
        free(src_img.buff);
    if (rgb_img.buff)
        free(rgb_img.buff);
#elif defined(ENABLE_CONVERT_NV12_TO_YUYV)
    if (nv12_img.buff)
        free(nv12_img.buff);
    if (dst_img.buff)
        free(dst_img.buff);
#elif defined(ENABLE_CONVERT_TO_RGB24_TILED)
    if (src_img.buff)
        free(src_img.buff);
    if (before_resize_img.buff)
        free(before_resize_img.buff);
    if (after_resize_img.buff)
        free(after_resize_img.buff);
    if (resized_packed_img.buff)
        free(resized_packed_img.buff);
    if (rgb24_packed_img.buff)
        free(rgb24_packed_img.buff);
    if (rgb24_tiled_img.buff)
        free(rgb24_tiled_img.buff);
#elif defined(ENABLE_PIP_DOUBLE_PICTURE)
    if (src_img.buff)
        free(src_img.buff);
    if (crop1_img.buff)
        free(crop1_img.buff);
    if (crop2_img.buff)
        free(crop2_img.buff);
    if (resize1_img.buff)
        free(resize1_img.buff);
    if (resize2_img.buff)
        free(resize2_img.buff);
    if (dst_img.buff)
        free(dst_img.buff);
#endif

    nc_munmap_ofrc_bufs(&ofrc_buf_info);
    nc_close_ofrc_sync_pollfd(&poll_handle);
    
    return NULL;
}

void sig_int_handler(int dummy)
{
    g_ofrc_loop = 0;
}

int main( int argc, char ** argv)
{
    int ret;
    stConfig_ini config_ini;
    pthread_t p_thread;
    int thr_id;
    int status;

    signal(SIGINT, sig_int_handler);

    srand(time(NULL));

    nc_init_path_localizer();
    /* Open ISP data binary */
    g_isp_fd = nc_open_isp_device();
    if (g_isp_fd < 0) {
        printf("fail to nc_open_isp_device() g_isp_fd=%d\n", g_isp_fd);
        exit(1);
    }

    /* Open JIG Uart */
    jig_process_init(B57600, g_isp_fd);

    /* Load Config from ini file */
    if (nc_app_config_parse(nc_localize_path((const char*)INI_FILE), &config_ini) < 0) {
        printf("Can't load %s\n", nc_localize_path((const char*)INI_FILE));
        nc_close_isp_device(g_isp_fd);
        exit(1);
    }

    /* Load ISP binary to isp register */
    if (nc_load_isp_data(g_isp_fd, nc_localize_path(config_ini.isp_data_file)) < 0) {
        perror("isp data loading failure !!!\n");
        nc_close_isp_device(g_isp_fd);
        exit(1);
    }

    /* Reset CIS only when cis interface is not serdes*/
    if (!config_ini.is_serdes) {
        nc_cis_reset_by_gpio();
    }

    /* Load Lut binary for LDC */
    if (nc_load_ldc_data(g_isp_fd, nc_localize_path(config_ini.ldc_lut_file)) < 0) {
        perror("ldc data loading failure !!!\n");
        nc_close_isp_device(g_isp_fd);
        exit(1);
    }

    /* Initialize ISP */
    if ((ret = nc_init_isp(g_isp_fd, &config_ini.is_serdes)) < 0) {
        printf("NC_ISP_IOCTL_INIT_ISP failure(%d)\n", ret);
        nc_close_isp_device(g_isp_fd);
        exit(1);
    }

    /* Initialize OFRC */
    if ((ret = nc_init_ofrc(g_isp_fd)) < 0) {
        printf("NC_ISP_IOCTL_INIT_OFRC failure(%d)\n", ret);
        nc_close_isp_device(g_isp_fd);
        exit(1);
    }

    /* Obtain Resolution of ISP */
    if ((ret = nc_isp_get_resolution(g_isp_fd, &g_isp_resolution)) < 0) {
        printf("Error : nc_isp_get_resolution[%d]\n", ret);
        nc_close_isp_device(g_isp_fd);
        exit(1);
    }
    printf("ISP RESOLUTION: %d x %d\n", g_isp_resolution.width, g_isp_resolution.height);

    // Create ofrc mixer task
    thr_id = pthread_create(&p_thread, NULL, ofrc_task, (void *)&g_isp_fd);
    if (thr_id < 0) {
        perror("thread create error : ofrc_task\n");
        nc_close_isp_device(g_isp_fd);
        exit(1);
    }

    pthread_join(p_thread, (void **)&status);
    printf("neon_app is terminated\n");
    nc_close_isp_device(g_isp_fd);

    return 0;
}
