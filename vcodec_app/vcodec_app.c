/**
********************************************************************************
* Copyright (C) 2021 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : vcodec_app.c
*
* @brief   : video codec sample application
*
* @author  : Software Development Team.  NextChip Inc.
*
* @date    : 2024.04.24.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/lp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <termios.h>
#include <errno.h>
#include <sys/signal.h>
#include <signal.h>
#include <linux/types.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include <mqueue.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "v4l2_interface.h"
#include "nc_utils.h"
#include "nc_streamer.h"
#include "nc_video_codec.h"
#include "nc_vcodec_config_parser.h"

/*
********************************************************************************
*               DEFINES
********************************************************************************
*/

#define VIDEO_BUFFER_NUM    (3)

// #define USE_ENC_STOP
#ifdef USE_ENC_STOP
#define ENC_STOP_FCNT (15 * 60 * 60 * 24)
#endif

// #define DBG_SEND_MSG

#define INI_FILE "misc/nc_enc_app_config.ini"

/*
********************************************************************************
*               VARIABLE DECLARATIONS
********************************************************************************
*/

static char enc_output_filename[50] = {0,};
FILE *enc_output_fp = NULL;

static char enc_output_size_filename[50] = {0,};
FILE *enc_output_size_fp = NULL;

static char dec_input_size_filename[50] = {0,};
FILE *dec_input_size_fp = NULL;

static char dec_input_filename[50] = {0,};
FILE *dec_input_fp = NULL;

static char dec_output_filename[50] = {0,};
FILE *dec_output_fp = NULL;

st_nc_v4l2_config v4l2_config;

int default_coda_index = CODA_DEVICE_NUM(VISION1);

static int running = 1;

static uint64_t g_dec_total_fcnt = 0;
static uint8_t g_last_decoded_frame_received = 0;

uint8_t *enc_header = NULL;
static FILE *ffmpeg_pipe = NULL;

output_mode_t g_mode;

/*
********************************************************************************
*               FUNCTION DEFINITIONS
********************************************************************************
*/

static int check_resolution_is_ok(int sensor_w, int sensor_h, uint32_t target_w, uint32_t target_h)
{
    if(target_w == 0 || target_h == 0){
        return 0;
    }
    if((sensor_w / target_w) > 2) return 0;
    if((sensor_h / target_h) > 2) return 0;
    return 1;
}

int start_ffmpeg_rtsp_server()
{
    // h264
    // const char *cmd =
    //     "ffmpeg -f h264 -i pipe:0 "
    //     "-flush_packets 1 -fflags nobuffer "
    //     "-c copy -f rtsp -rtsp_transport tcp "
    //     "rtsp://127.0.0.1:8554/mystream";

    // h265
    // const char *cmd =
    //     "ffmpeg -f hevc -i pipe:0 "
    //     "-flush_packets 1 -fflags nobuffer "
    //     "-c copy -f rtsp -rtsp_transport tcp "
    //     "rtsp://127.0.0.1:8554/mystream";

    const char *cmd =
        "./misc/ffmpeg -f hevc -i pipe:0 "
        "-flush_packets 1 -fflags nobuffer "
        "-c copy -f rtsp -rtsp_transport tcp "
        "rtsp://127.0.0.1:8554/mystream";

    ffmpeg_pipe = popen(cmd, "w");
    if (!ffmpeg_pipe) {
        perror("popen failed");
        return -1;
    }

    setvbuf(ffmpeg_pipe, NULL, _IONBF, 0);

    return 0;
}

int v4l2_initialize(unsigned int *width, unsigned int *height)
{
    int sensor_width, sensor_height;
    v4l2_config.video_buf.video_device_num = default_coda_index;
    v4l2_config.video_buf.video_fd         = -1;

    v4l2_config.video_buf.video_fd = nc_v4l2_open(v4l2_config.video_buf.video_device_num, true);
    if(v4l2_config.video_buf.video_fd == errno) {
        printf("[error] nc_v4l2_open() failure!\n");
    } else {
        if(nc_v4l2_get_format(v4l2_config.video_buf.video_fd, &sensor_width, &sensor_height) < 0) {
            printf("nc_v4l2_get_format() failure!\n");
            return -1;
        }

        if(check_resolution_is_ok(sensor_width,sensor_height,*width, *height) != 0)
        {
            *width = ALIGN_DOWN(*width, 8);
            *height = ALIGN_DOWN(*height, 8);

            v4l2_config.img_process                = MODE_DS;
            v4l2_config.pixformat                  = V4L2_PIX_FMT_NV12;
            v4l2_config.ds_width                   = *width;
            v4l2_config.ds_height                  = *height;

            if(nc_v4l2_init_device_and_stream_on(&v4l2_config, VIDEO_BUFFER_NUM) < 0) {
                printf("[error] nc_v4l2_init_device_and_stream_on() failure!\n");
                return -1;
            }
        }
        else
        {
            printf("[error] Resolution NOT SUPPORTED !!\n");
            return -1;
        }
    }

    nc_v4l2_show_user_config(&v4l2_config, 1);

    return 0;
}

float calc_send_fps_at_loop_ent(int update_period_fcnt)
{
    static uint64_t s_time = 0;
    static float fps = 0.f;
    static uint64_t fcnt = 0;
    double elapsed_ms = 0;

    fcnt++;
    if (fcnt % update_period_fcnt == 1) {
        if (s_time == 0) {
            s_time = nc_get_mono_time();
        } else {
            elapsed_ms = (double)nc_elapsed_time(s_time);
            // printf("fcnt(%d) elapsed_ms(%d)\n", fcnt, elapsed_ms);
            if (elapsed_ms > 0.) fps = (float)(fcnt-1) / (float)(elapsed_ms/1000.f);

            // re-init
            fcnt = 1;
            s_time = nc_get_mono_time();
        }
    }

    return fps;
}

static void user_signal_handler(int signum, siginfo_t* si, void* unused)
{
    const char* name = NULL;
    (void) si;
    (void) unused;

    running = 0;

    switch (signum) {
    case SIGABRT: name = "SIGABRT";  break;
    case SIGSEGV:
        name = "SIGSEGV";
        break;
    case SIGBUS:  name = "SIGBUS";   break;
    case SIGILL:  name = "SIGILL";   break;
    case SIGFPE:  name = "SIGFPE";   break;
    case SIGPIPE: name = "SIGPIPE";  break;
    case SIGINT:
    {
        if(g_mode == RUN_ENC_FILE_DUMP)
        {
            if(enc_output_fp) fclose(enc_output_fp);
            if(enc_output_size_fp) fclose(enc_output_size_fp);
        }
        if(dec_input_size_fp) fclose(dec_input_size_fp);
        if(dec_input_fp) fclose(dec_input_fp);
        if(g_mode == RUN_DEC_FILE_DUMP)
        {
            if(dec_output_fp) fclose(dec_output_fp);          
        }
        name = "SIGINT";   break;
    }
    case SIGTERM: name = "SIGTERM";  break;
    }

    if (name) {
        fprintf(stderr, "Caught signal %d (%s)\n", signum, name);
    } else {
        fprintf(stderr, "Unknown signal %d\n", signum);
    }

    if(g_mode != RUN_DEC_FILE_DUMP)
    {
        nc_stop_encoder();
    }
    else
    {
        nc_stop_decoder();
    }

    if(g_mode == RUN_ENC_NC_STREAM)
    {
        nc_shutdown_stream_server();
        free(enc_header);
    }

    sleep(1);

    exit(signum);
}

static void register_user_signal(void)
{
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = user_signal_handler;
    sigemptyset(&sa.sa_mask);

    sigaction( SIGABRT, &sa, NULL );
    sigaction( SIGSEGV, &sa, NULL );
    sigaction( SIGBUS,  &sa, NULL );
    sigaction( SIGILL,  &sa, NULL );
    sigaction( SIGFPE,  &sa, NULL );
    sigaction( SIGPIPE, &sa, NULL );
    sigaction( SIGINT,  &sa, NULL );
    sigaction( SIGTERM, &sa, NULL );
}

void nc_encoded_callback_ftn(Uint8* val, Uint32 size)
{
    switch (g_mode)
    {
        case RUN_ENC_NC_STREAM:
            static int header_size = 0;

            if (!header_size)
            {
                header_size = size;
                enc_header = (Uint8 *)malloc(header_size);
                memcpy(enc_header, val, header_size);
                nc_push_stream_data((unsigned char *)(val), size, 1, enc_header, header_size);
            }
            nc_push_stream_data((unsigned char *)(val), size, 0, enc_header, header_size);
        break;
        
        case RUN_ENC_FILE_DUMP :
            sprintf(enc_output_filename, "misc/vcodec/sample_enc_output.bin");
            sprintf(enc_output_size_filename, "misc/vcodec/sample_data_size.txt");

            if (enc_output_fp == NULL)
            {
                if ((enc_output_fp = fopen(enc_output_filename, "wb+")) == NULL)
                {
                    printf("[%s:%d] ... Fail to open encoding output file\n", __FUNCTION__, __LINE__);
                }
                else
                {
                    fclose(enc_output_fp);
                    if ((enc_output_fp = fopen(enc_output_filename, "ab+")) == NULL)
                    {
                        printf("[%s:%d] ... Fail to open encoding output file\n", __FUNCTION__, __LINE__);
                    }
                }
            }

            if (enc_output_fp != NULL)
            {
                fwrite(val, sizeof(uint8_t), size, enc_output_fp);
                fflush(enc_output_fp);
            }
            else
            {
                printf("[%s:%d] File pointer is NULL\n", __FUNCTION__, __LINE__);
            }

            if (enc_output_size_fp == NULL)
            {
                if ((enc_output_size_fp = fopen(enc_output_size_filename, "w+")) == NULL)
                {
                    printf("[%s:%d] ... Fail to open encoding output size file\n", __FUNCTION__, __LINE__);
                }
                else
                {
                    fclose(enc_output_size_fp);
                    if ((enc_output_size_fp = fopen(enc_output_size_filename, "a+")) == NULL)
                    {
                        printf("[%s:%d] ... Fail to open encoding output size file\n", __FUNCTION__, __LINE__);
                    }
                }
            }

            if (enc_output_size_fp != NULL)
            {
                fprintf(enc_output_size_fp, "%d\n", size);
                fflush(enc_output_size_fp);
            }
            else
            {
                printf("[%s:%d] File pointer is NULL\n", __FUNCTION__, __LINE__);
            }
        break;
        
        case RUN_ENC_NC_RTSP :
            if (!ffmpeg_pipe || !val || size == 0)
            {
                printf("[%s:%d] invalid input\n", __FUNCTION__, __LINE__);
                return;
            }

            size_t written = fwrite(val, 1, size, ffmpeg_pipe);
            // printf("rtsp write (fps : %0.1f)\n", calc_send_fps_at_loop_ent(100));

            if (written != size)
            {
                fprintf(stderr, "[%s:%d] fwrite error: %zu of %u bytes written\n", __FUNCTION__, __LINE__, written, size);
            }
        break;
                
    }
    
    nc_unlock_encoding_done();
}

void nc_decoded_callback_ftn(Uint8* val, Uint32 size)
{
    static uint64_t d_cnt = 0;
    d_cnt++;

    if (d_cnt % 10 == 0) {
        printf("....... decoding is in progress. .... please wait ....\n");
    }

    sprintf(dec_output_filename, "misc/vcodec/sample_dec_output.yuv");

    if(dec_output_fp == NULL) {
        if ((dec_output_fp=fopen(dec_output_filename, "wb+")) == NULL) {
            printf("[%s:%d] ... Fail to open decoding output file\n", __FUNCTION__, __LINE__);
        }
        else {
            fclose(dec_output_fp);
            if ((dec_output_fp=fopen(dec_output_filename, "ab+")) == NULL) {
                printf("[%s:%d] ... Fail to open decoding output file\n", __FUNCTION__, __LINE__);
            }
        }
    }

    if (dec_output_fp != NULL) {
        fwrite(val, sizeof(uint8_t), size, dec_output_fp);
        fflush(dec_output_fp);
    } else {
        printf("[%s:%d] File pointer is NULL\n", __FUNCTION__, __LINE__);
    }
}

void nc_last_frame_decoded_callback_ftn(void)
{
    printf("[%s:%d]........ Last frame decoded .........\n", __FUNCTION__, __LINE__);
    g_last_decoded_frame_received = 1;
}

void *enc_send_task(void* arg)
{
    st_nc_v4l2_config* config = (st_nc_v4l2_config*)arg;
    struct      v4l2_buffer buf;
    int         cam_fd = config->video_buf.video_fd;
    CODEC_ERR_STATE codec_state = CODEC_SUCCESS;
    Uint32 end_of_stream = 0;

    printf("Start encoding task\n");

    while(running)
    {
        Uint8* data = NULL;

        if (nc_v4l2_dequeue_buffer(cam_fd, &buf) < 0) {
            printf("[ERROR] nc_v4l2_dequeue_buffer(errno:%d) failed .......\n", errno);
        } else {
            data = (Uint8*)config->video_buf.buffers[buf.index].start;

#ifdef USE_ENC_STOP
            static uint64_t fcnt = 0;
            fcnt++;
            if (fcnt == (uint64_t)ENC_STOP_FCNT) {
                data = NULL;
                end_of_stream = 1;
            }
#endif

#ifdef DBG_SEND_MSG
            printf("Send video buffer[data:%p ts:%lu] to encoder (fps : %0.1f)\n", data, nc_get_us_from_timeval(&buf.timestamp), calc_send_fps_at_loop_ent(30));
#endif
            if((codec_state = nc_send_buf_to_encode(data, 0)) != CODEC_SUCCESS) {
                printf("nc_send_buf_to_encode failure ... err state : %d \n", codec_state);
            }
            if (nc_v4l2_queue_buffer(cam_fd, buf.index) < 0) {
                printf("v4l2_queu_buffer failed\n");
            }

            if (end_of_stream) {
                printf("@@@@@@@@@@@@ END OF STREAM @@@@@@@@@@@@@\n");
                nc_stop_encoder();
                running = 0;
                break;
            }
        }
    }

    pthread_exit(NULL);
}

void *dec_send_task(void*)
{
    Uint32 end_of_file = 0;
    Uint32 size_read = 0;
    CODEC_ERR_STATE codec_state = CODEC_SUCCESS;

    sprintf(dec_input_size_filename, "misc/vcodec/sample_data_size.txt");
    sprintf(dec_input_filename, "misc/vcodec/sample_enc_output.bin");

    if(dec_input_size_fp == NULL) {
        dec_input_size_fp = fopen(dec_input_size_filename, "r");
        if (dec_input_size_fp == NULL) {
            printf("[%s:%d] ... Fail to open decoding input size file\n", __FUNCTION__, __LINE__);
            return NULL;
        }
    }

    if(dec_input_fp == NULL) {
        dec_input_fp = fopen(dec_input_filename, "r");
        if (dec_input_fp == NULL) {
            printf("[%s:%d] ... Fail to open decoding input file\n", __FUNCTION__, __LINE__);
            return NULL;
        }
    }

    printf("Start decoding task\n");

    while(running)
    {
        char buf[256];
        Uint8 *buffer = NULL;

        if (end_of_file) {
            if (g_last_decoded_frame_received) {
                nc_stop_decoder();
                break;
            }

            usleep(100);
            continue;
        }

        if (fgets(buf, sizeof(buf), dec_input_size_fp) != NULL) {
            buf[strlen(buf)-1] = '\0';
            size_read = 0;
            size_read = atoi(buf);
        }

        if (feof(dec_input_size_fp)){
            end_of_file = 1;
            size_read = 0;
        } else {
            buffer = (Uint8 *)malloc(2*1024*1024);
            fread(buffer, 1, size_read, dec_input_fp);
        }

        if (end_of_file) {
            printf("@@@@@@@@@@@@ END OF FILE @@@@@@@@@@@@@\n");
        }

        printf("Send encoded buffer(ptr:%p size:%d) to decoder\n", buffer, size_read);
        if((codec_state = nc_send_buf_to_decode((Uint8*)buffer, size_read, end_of_file, 1)) != CODEC_SUCCESS) {
            printf("nc_send_buf_to_decode failure ... err state : %d \n", codec_state);
        }
        else {
            g_dec_total_fcnt++;
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    vcodec_app_config_t app_config;
    CODEC_ERR_STATE codec_state = CODEC_SUCCESS;
    pthread_t p_thread[MAX_TASK_CNT];

    int thr_id;
    int t_cnt = 0;
    int ret = 0;
    int status;
    int i = 0;

    if (argc > 1) {
        for (i = 1; i < argc; ++i) {
            printf("arg %d : %s\n", i, argv[i]);
        }
    }

    register_user_signal();

    memset(&app_config, 0, sizeof(app_config));
    memset(&v4l2_config, 0, sizeof(v4l2_config));

    if ((nc_vcodec_app_config_parse(nc_localize_path((const char *)INI_FILE), &app_config) < 0))
    {
        printf("Can't load %s \n", nc_localize_path((const char *)INI_FILE));
        exit(1);
    }
    
    g_mode = app_config.mode;

    ret = v4l2_initialize(&app_config.enc_config.enc_width, &app_config.enc_config.enc_height);
    if(ret < 0) {
        printf("Error v4l2_initialize\n");
        return -1;
    }

    if(app_config.mode == RUN_ENC_NC_RTSP)
    {
        if (start_ffmpeg_rtsp_server() != 0) 
        {
            printf("[%s:%d] start_ffmpeg_rtsp_server fail\n", __FUNCTION__, __LINE__);
            return -1;
        }
    }
       
    nc_init_path_localizer();

    if(app_config.mode == RUN_ENC_NC_STREAM)
    {
        char ip_address[MAX_IFNAME_SIZE] = {'\0', };
        nc_start_stream_server(STREAM_PORT);
        nc_get_local_IPv4("eth0", ip_address);
        printf("\nWaiting for Client connection (server - %s:%d) !!! .........\n", ip_address, STREAM_PORT);
    }

    nc_display_libncvcodec_info();

    if(app_config.mode != RUN_DEC_FILE_DUMP)
    {
        /* Run Encoder */
        thr_id = pthread_create(&p_thread[t_cnt++], NULL, enc_send_task, (void *)&v4l2_config);
        if (thr_id < 0) {
            perror("thread create error : enc_send_task");
        }

        codec_state = (CODEC_ERR_STATE)nc_init_encoder(&app_config.enc_config);
        if(codec_state != CODEC_SUCCESS) {
            printf("nc_init_encoder failure ... err state : %d \n", codec_state);
            return 0;
        }

        nc_register_encode_callback(nc_encoded_callback_ftn);

        codec_state = (CODEC_ERR_STATE)nc_start_encoder();
        if(codec_state != CODEC_SUCCESS) {
            printf("nc_start_encoder failure ... err state : %d \n", codec_state);
            return 0;
        }

        nc_stop_encoder();
    }    
    else
    {
        /*Run Decoder */
        thr_id = pthread_create(&p_thread[t_cnt++], NULL, dec_send_task, NULL);
        if (thr_id < 0) {
            perror("thread create error : dec_send_task");
        }

        codec_state = (CODEC_ERR_STATE)nc_init_decoder(&app_config.dec_config);
        if(codec_state != CODEC_SUCCESS) {
            printf("nc_init_decoder failure ... err state : %d \n", codec_state);
            return 0;
        }

        nc_register_decode_callback(nc_decoded_callback_ftn);
        nc_register_last_frame_decoded_callback(nc_last_frame_decoded_callback_ftn);

        codec_state = (CODEC_ERR_STATE)nc_start_decoder();
        if(codec_state != CODEC_SUCCESS) {
            printf("nc_start_decoder failure ... err state : %d \n", codec_state);
            return 0;
        }

        nc_stop_decoder();
    }

    if(app_config.mode == RUN_ENC_NC_STREAM)
    {
        nc_shutdown_stream_server();
    }

    printf("pthread_join(t_cnt:%d) before terminate\n", t_cnt);
    for (int i = 0; i < t_cnt; i++) {
        pthread_join(p_thread[i], (void **)&status);
    }

    printf("vcodec_app is terminated\n");

    return 0;
}
