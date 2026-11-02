/**
********************************************************************************
* Copyright (C) 2024 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : nc_opengl_bitmap_font.h
*
* @brief   : nc_opengl_bitmap_font header
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

#ifndef __NC_OPENGL_BITMAP_FONT_H__
#define __NC_OPENGL_BITMAP_FONT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <signal.h>

#include <linux/input.h>

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <time.h>
#include <errno.h>

#include <SOIL.h>

#include "wayland_egl.h"
#include "nc_opengl_init.h"

void nc_initText2D(window *window);
void nc_printText2D(const char *text, int x, int y, int size, float textcolor[4], window *window);

#endif // __NC_OPENGL_BITMAP_FONT_H__