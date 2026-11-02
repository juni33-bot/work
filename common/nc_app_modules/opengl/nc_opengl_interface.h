/**
********************************************************************************
* Copyright (C) 2024 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : nc_opengl_interface.h
*
* @brief   : nc_opengl_interface header
*
* @author  : Software Development Team.  NextChip Inc.
*
* @date    : 2024.08.09.
*
* @version : 1.0.0
********************************************************************************
* @note
*
********************************************************************************
*/

#ifndef __NC_OPENGL_INTERFACE_H__
#define __NC_OPENGL_INTERFACE_H__

#include <stdio.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include "wayland_egl.h"
#include "nc_opengl_init.h"


#define VIEWPORT_WIDTH       (1920)
#define VIEWPORT_HEIGHT      (1080)


struct gl_npu_program
{
    GLuint program;
    GLuint pos_handle;
    GLuint texcoord_handle;

    GLuint color_handle;
    GLuint drawing_type_handle;
    GLuint viewport_size_handle;
    GLuint texture_handle;
    GLuint colors_array_handle;
};

// Draw type enumeration
enum {
    DRAW_NPU_TYPE_SOLID,
    DRAW_NPU_TYPE_DASHED,
    DRAW_NPU_TYPE_SEG,
    DRAW_NPU_NUM_TYPE
};

void nc_opengl_pixel_to_ndc(float pixel_x, float pixel_y, float* ndc_x, float* ndc_y);
void nc_opengl_create_rect_vertex(float pixel_x, float pixel_y, float pixel_width, float pixel_height, float vertexCoords[12]);
void nc_opengl_create_line_vertex(float st_x, float st_y, float end_x, float end_y, float *vertexCoords);
void nc_opengl_draw_rectangle(float x, float y, float w, float h, float color[4], struct gl_npu_program g_npu_prog);
void nc_opengl_draw_segmentation(GLuint texture, float** colors, int class_cnt, struct gl_npu_program g_npu_prog);
void nc_opengl_draw_line(float st_x, float st_y, float end_x, float end_y, int lane_class, float color[4], struct gl_npu_program g_npu_prog);
#ifdef USE_UFLD_NETWORK_DEBUGGING
void nc_opengl_draw_debugging_grid_line(float st_x, float st_y, float end_x, float end_y, int lane_class, float color[4], struct gl_npu_program g_npu_prog);
#endif
void nc_opengl_init_npu_shader(struct gl_npu_program *npu_prog);

void nc_opengl_draw_texture(GLint texture, window *window);
void nc_opengl_init_video_shader(window *window, int use_yuv_shader);

#endif // __NC_OPENGL_INTERFACE_H__