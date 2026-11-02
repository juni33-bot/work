/**
********************************************************************************
* Copyright (C) 2024 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : nc_opengl_ttf_font.h
*
* @brief   : nc_opengl_ttf_font header
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

#ifndef __NC_OPENGL_TTF_FONT_H__
#define __NC_OPENGL_TTF_FONT_H__

#include <stdio.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include "wayland_egl.h"
#include "nc_opengl_init.h"

struct gl_font_program
{
    GLuint program;
    GLuint vertex_handle;
    GLuint texcoord_handle;

    GLuint screen_size_handle;
    GLuint color_handle;
    GLuint texture_handle;
};

typedef struct {
    GLuint textureID;
    int width;
    int height;
    int bearingX;
    int bearingY;
    long advance;
} Character;

typedef struct {
    Character characters[128];  // Assuming ASCII characters
} Font;

void nc_opengl_load_font(const char *fontPath, int fontSize, Font *font);
void nc_opengl_draw_text(Font *font, const char *text, float x, float y, float scale, float color[3], int screen_width, int screen_height, struct gl_font_program font_prog);
void nc_opengl_init_font_shader(struct gl_font_program *font_prog);

#endif // __NC_OPENGL_TTF_FONT_H__