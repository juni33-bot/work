/**
********************************************************************************
* Copyright (C) 2021 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : wayland_cam_app.c
*
* @brief   : wayland_cam application
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
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <pthread.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <time.h>
#include <errno.h>
#include <SOIL.h>
#include <sys/wait.h>

#include "wayland_egl.h"
#include "nc_opengl_init.h"
#include "v4l2_interface.h"
#include "nc_utils.h"
#include "nc_opengl_shader.h"
#include "nc_opengl_interface.h"
#include "nc_opengl_ttf_font.h"

/*
********************************************************************************
*               DEFINES
********************************************************************************
*/

#define VIS0_MAX_CH         (1)
#define VIS1_MAX_CH         (0)
#define VIDEO_MAX_CH        (VIS0_MAX_CH + VIS1_MAX_CH)

#define VIDEO_BUFFER_NUM    (3)

#define VIDEO_WIDTH         (1920)
#define VIDEO_HEIGHT        (1080)

// #define USE_JIG_APP

// #define USE_YUV_SHADER

/*
********************************************************************************
*               VARIABLE DECLARATIONS
********************************************************************************
*/

struct gl_font_program g_font_prog;
Font font_38;

unsigned char* image_data;
static int running = 1;
st_nc_v4l2_config v4l2_config[VIDEO_MAX_CH];
struct viewport g_viewport[VIDEO_MAX_CH];

/*
********************************************************************************
*               FUNCTION DEFINITIONS
********************************************************************************
*/

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
    #ifdef USE_YUV_SHADER
        v4l2_config[i].video_buf.video_device_num = OFRC_DEVICE_NUM(VISION0) + i;
    #else
        v4l2_config[i].video_buf.video_device_num = CNN_DEVICE_NUM(VISION0) + i;
    #endif
        v4l2_config[i].video_buf.video_fd         = -1;
        v4l2_config[i].dma_mode                   = INTERLEAVE;
        v4l2_config[i].img_process                = MODE_DS;
    #ifdef USE_YUV_SHADER
        v4l2_config[i].pixformat                  = V4L2_PIX_FMT_YUYV;
    #else
        v4l2_config[i].pixformat                  = V4L2_PIX_FMT_RGB24;
    #endif
        v4l2_config[i].crop_x_start               = 0;
        v4l2_config[i].crop_y_start               = 0;
        v4l2_config[i].crop_width                 = 0;
        v4l2_config[i].crop_height                = 0;
        v4l2_config[i].ds_width                   = VIDEO_WIDTH;
        v4l2_config[i].ds_height                  = VIDEO_HEIGHT;
    }

    for(j = VIS0_MAX_CH; j < VIDEO_MAX_CH; j++) {
    #ifdef USE_YUV_SHADER
        v4l2_config[j].video_buf.video_device_num = OFRC_DEVICE_NUM(VISION1) + j;
    #else
        v4l2_config[j].video_buf.video_device_num = CNN_DEVICE_NUM(VISION1) + j;
    #endif
        v4l2_config[j].video_buf.video_fd         = -1;
        v4l2_config[j].dma_mode                   = INTERLEAVE;
        v4l2_config[j].img_process                = MODE_DS;
    #ifdef USE_YUV_SHADER
        v4l2_config[j].pixformat                  = V4L2_PIX_FMT_YUYV;
    #else
        v4l2_config[j].pixformat                  = V4L2_PIX_FMT_RGB24;
    #endif
        v4l2_config[j].crop_x_start               = 0;
        v4l2_config[j].crop_y_start               = 0;
        v4l2_config[j].crop_width                 = 0;
        v4l2_config[j].crop_height                = 0;
        v4l2_config[j].ds_width                   = VIDEO_WIDTH;
        v4l2_config[j].ds_height                  = VIDEO_HEIGHT;
    }
}

int v4l2_initialize(void)
{
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

void gl_initialize(struct window *window)
{
    int width, height, channels;
    char buf[128];

    sprintf(buf, "misc/image/nextchip.png");
    image_data = SOIL_load_image(buf, &width, &height, &channels, SOIL_LOAD_RGBA);

    // init video shader

#ifdef USE_YUV_SHADER
    nc_opengl_init_video_shader(window, 1);
#else
    nc_opengl_init_video_shader(window, 0);
#endif

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
    // init font shader
    nc_opengl_init_font_shader(&g_font_prog);

    // set viewport
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void render(void *data, struct wl_callback *callback, uint32_t time)
{
    static struct timespec begin, end, set_time;
    static uint64_t fpstime = 0;
    static uint64_t fpscount = 0;
    static uint64_t fpscount_00 = 0;
    static uint64_t frametime = 0;
    static uint64_t opengl_time = 0;

    (void)time;

    clock_gettime(CLOCK_MONOTONIC, &begin);
    struct window *window = (struct window *)data;

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
            struct v4l2_buffer video_buf;
            CLEAR(video_buf);

            // Queue the buffer for capturing a new frame
            if (nc_v4l2_dequeue_buffer(v4l2_config[i].video_buf.video_fd, &video_buf) == -1) {
                //printf("Error VIDIOC_DQBUF buffer %d\n", video_buf.index);
            }else
            {
                if(nc_v4l2_queue_buffer(v4l2_config[i].video_buf.video_fd, video_buf.index) == -1) {
                    printf("Error VIDIOC_QBUF buffer %d\n", video_buf.index);
                }

                // Upload texture from video buffer
                glBindTexture(GL_TEXTURE_2D, window->gl.texture[i]);

                if(v4l2_config[i].data_idx == OFRC_MODE) {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, v4l2_config[i].sensor_width/2, v4l2_config[i].sensor_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, v4l2_config[i].video_buf.buffers[video_buf.index].start);
                } else {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, VIDEO_WIDTH, VIDEO_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, v4l2_config[i].video_buf.buffers[video_buf.index].start);
                }
            }
        }

        // draw video texture
        glViewport(g_viewport[i].x, g_viewport[i].y, g_viewport[i].width, g_viewport[i].height);
        nc_opengl_draw_texture(window->gl.texture[i], window);
    }

    fpscount+=1;

    char buftext[256];

    sprintf(buftext,"GL: %lums, Frame: %lums/%lufps", opengl_time, frametime, fpscount_00);

    glViewport(0,0, WINDOW_WIDTH, WINDOW_HEIGHT);
    float textcolor[3] = {1.0, 0.0, 0.0}; // R, G, B
    nc_opengl_draw_text(&font_38, buftext, 10, 1020, 1.0f, textcolor, WINDOW_WIDTH, WINDOW_HEIGHT, g_font_prog);

    clock_gettime(CLOCK_MONOTONIC, &end);

    opengl_time = ((end.tv_sec - begin.tv_sec)*1000 + (end.tv_nsec - begin.tv_nsec)/1000000);

    frametime = ((end.tv_sec - set_time.tv_sec)*1000 + (end.tv_nsec - set_time.tv_nsec)/1000000);

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

#ifdef USE_JIG_APP
static int jig_process(void)
{
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    } else if (pid == 0) {
        const char *app_name = "./misc/isp_jig_vision0";
        const char *args[] = {app_name, NULL};

        execvp(app_name, (char* const*)args);

        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }

    return 0;
}
#endif

static void signal_int()
{
    running = 0;
}

static void usage(int error_code)
{
    fprintf(stderr, "Usage: simple-egl [OPTIONS]\n\n"
        "  -f\tRun in fullscreen mode\n"
        "  -o\tCreate an opaque surface\n"
        "  -h\tThis help text\n\n");

    exit(error_code);
}

int main(int argc, char **argv)
{
    struct sigaction sigint;
    struct display display;
    struct window  window;
    int i, ret = 0;

    for (i = 1; i < argc; i++) {
        if (strcmp("-f", argv[i]) == 0)
            window.fullscreen = 1;
        else if (strcmp("-o", argv[i]) == 0)
            window.opaque = 1;
        else if (strcmp("-h", argv[i]) == 0)
            usage(EXIT_SUCCESS);
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

    window.display = &display;
    display.window = &window;
    window.window_size.width  = WINDOW_WIDTH;
    window.window_size.height = WINDOW_HEIGHT;

    set_viewport_config();

    nc_wayland_display_init(&display,(void *)render);

    ret = v4l2_initialize();
    if(ret < 0) {
        printf("Error v4l2_initialize\n");
        return -1;
    }

#ifdef USE_JIG_APP
    jig_process();
#endif

    gl_initialize(&window);

    while (running && ret != -1)
    {
        ret = wl_display_dispatch(display.display);
    }

    nc_wayland_display_destroy(&display);

    return 0;
}
