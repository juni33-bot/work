/**
********************************************************************************
* Copyright (C) 2024 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : nc_opengl_init.h
*
* @brief   : nc_opengl_init header
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

#ifndef __NC_OPENGL_INIT_H__
#define __NC_OPENGL_INIT_H__

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

typedef struct {
    float x, y;
}Vector2;

typedef struct {
    float x, y, z;
}Vector3;

GLuint nc_loadTexture(const char * imagepath);
GLuint nc_loadBMP_custom(const char * imagepath);
int nc_loadOBJ(const char * path, Vector3 **pVertices, Vector2 **pUvs, Vector3 **pNormals, int *pObjCount);
GLuint nc_createProgram(const char* vertexSource, const char* fragmentSource);

#endif // __NC_OPENGL_INIT_H__