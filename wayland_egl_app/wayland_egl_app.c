/**
********************************************************************************
* Copyright (C) 2021 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : wayland_egl_app.c
*
* @brief   : opengl 3d example application
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
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <SOIL.h>

#include "wayland_egl.h"
#include "matrix.h"
#include "nc_utils.h"
#include "nc_opengl_shader.h"
#include "nc_opengl_init.h"

/*
********************************************************************************
*               DEFINES
********************************************************************************
*/

/*
********************************************************************************
*               VARIABLE DECLARATIONS
********************************************************************************
*/

const char* objFile = "misc/texture/MarsCuriosityRover.obj";
const char* textureFile = "misc/texture/texture_";

static int running = 1;

Vec3  *vertices;
Vec3  *normals;
Vec2  *uvs;
int obj_f[10] = {0,};

/*
********************************************************************************
*               FUNCTION DEFINITIONS
********************************************************************************
*/

void gl_initialize(struct window *window)
{
    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);

    // OpenGL ES 쉐이더 프로그램 생성
    window->gl.program = nc_createProgram(vertexShaderSourceEx, fragmentShaderSourceEx);
    glUseProgram(window->gl.program);
    // Get a handle for our "MVP" uniform
    window->gl.MatrixID = glGetUniformLocation(window->gl.program, "MVP");

    window->gl.ViewMatrixID = glGetUniformLocation(window->gl.program, "V");
    window->gl.ModelMatrixID = glGetUniformLocation(window->gl.program, "M");

    // Get a handle for our "myTextureSampler" uniform
    window->gl.TextureID  = glGetUniformLocation(window->gl.program, "myTextureSampler");
    // Set our "myTextureSampler" sampler to user Texture Unit 0
    glUniform1i(window->gl.TextureID, 0);

    // Get a handle for our "LightPosition" uniform
    window->gl.LightID = glGetUniformLocation(window->gl.program, "LightPosition_worldspace");

    // Read our .obj file
    window->gl.objcount = nc_loadOBJ(objFile, (Vector3**)&vertices, (Vector2**)&uvs, (Vector3**)&normals, obj_f);

    if(window->gl.objcount > 0)
    {
        int objversize=0;
        int objuvsize=0;
        char buf[128];
        uint32_t i=0;

        for(i=0;i<window->gl.objcount;i++)
        {
            if(i==0)
            {
                // Load it into a VBO
                glGenBuffers(1, &window->gl.vertexbuffer[i]);
                glBindBuffer(GL_ARRAY_BUFFER, window->gl.vertexbuffer[i]);
                glBufferData(GL_ARRAY_BUFFER, obj_f[i] * 3 *sizeof(Vec3), &vertices[0], GL_STATIC_DRAW);

                glGenBuffers(1, &window->gl.uvbuffer[i]);
                glBindBuffer(GL_ARRAY_BUFFER, window->gl.uvbuffer[i]);
                glBufferData(GL_ARRAY_BUFFER, obj_f[i] * 3 * sizeof(Vec2), &uvs[0], GL_STATIC_DRAW);

                glGenBuffers(1, &window->gl.normalbuffer[i]);
                glBindBuffer(GL_ARRAY_BUFFER, window->gl.normalbuffer[i]);
                glBufferData(GL_ARRAY_BUFFER, obj_f[i] * 3 *sizeof(Vec3), &normals[0], GL_STATIC_DRAW);

                objversize = obj_f[i] * 3;
                objuvsize = obj_f[i] * 3;
            }else
            {
                // Load it into a VBO
                glGenBuffers(1, &window->gl.vertexbuffer[i]);
                glBindBuffer(GL_ARRAY_BUFFER, window->gl.vertexbuffer[i]);
                glBufferData(GL_ARRAY_BUFFER, obj_f[i] * 3 *sizeof(Vec3), &vertices[objversize], GL_STATIC_DRAW);

                glGenBuffers(1, &window->gl.uvbuffer[i]);
                glBindBuffer(GL_ARRAY_BUFFER, window->gl.uvbuffer[i]);
                glBufferData(GL_ARRAY_BUFFER, obj_f[i] * 3 * sizeof(Vec2), &uvs[objuvsize], GL_STATIC_DRAW);

                glGenBuffers(1, &window->gl.normalbuffer[i]);
                glBindBuffer(GL_ARRAY_BUFFER, window->gl.normalbuffer[i]);
                glBufferData(GL_ARRAY_BUFFER, obj_f[i] * 3 *sizeof(Vec3), &normals[objversize], GL_STATIC_DRAW);

                objversize = objversize + obj_f[i] * 3;
                objuvsize = objuvsize + obj_f[i] * 3;
            }


            sprintf(buf,"%s%d.png",textureFile,i+1);
            window->gl.Texture[i] = nc_loadTexture(buf);
        }

    }else
    {
        printf("nc_loadOBJ Error \n");
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glViewport(0, 0, window->geometry.width, window->geometry.height);
}


void render(void *data, struct wl_callback *callback, uint32_t time)
{
    static float angle = 0.0f;
    struct window *window = (struct window*)data;
    uint32_t i = 0;

    assert(window->callback == callback);
    window->callback = NULL;

    if (callback)
        wl_callback_destroy(callback);

    if (!window->configured)
        return;

    (void)time;

    // Clear the screen
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use our shader
    glUseProgram(window->gl.program);

    struct mat4f transform = mat4f_identity;
    transform = mat4f_multiply(transform, mat4f_perspective());
    transform = mat4f_multiply(transform, mat4f_translation(0, 0, -4));
    transform = mat4f_multiply(transform, mat4f_scale(5,5,5));
    transform = mat4f_multiply(transform, mat4f_rotate_x(2 * (float)pi * 0.02f));//(angle*0.3)));
    transform = mat4f_multiply(transform, mat4f_rotate_y(2 * (float)pi * angle));

    glUniformMatrix4fv(window->gl.MatrixID, 1, GL_FALSE, mat4f_gl(&transform));
    glUniformMatrix4fv(window->gl.ModelMatrixID, 1, GL_FALSE, mat4f_gl(&mat4f_identity));
    glUniformMatrix4fv(window->gl.ViewMatrixID, 1, GL_FALSE,  mat4f_gl(&mat4f_identity));

    GLfloat lightPos[3] = { 4.0f, 4.0f, 4.0f };
    glUniform3fv(window->gl.LightID, 1, lightPos);

    for(i=0;i<window->gl.objcount;i++)
    {
        // Bind our texture in Texture Unit 0
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, window->gl.Texture[i]);
        // Set our "myTextureSampler" sampler to user Texture Unit 0
        glUniform1i(window->gl.TextureID, i);

        // 1rst attribute buffer : vertices
        GLint vertexHandle = glGetAttribLocation(window->gl.program, "vertexPosition_modelspace");
        GLint uvHandle = glGetAttribLocation(window->gl.program, "vertexUV");
        GLint norHandle = glGetAttribLocation(window->gl.program, "vertexNormal_modelspace");

        glEnableVertexAttribArray(vertexHandle);
        glBindBuffer(GL_ARRAY_BUFFER, window->gl.vertexbuffer[i]);
        glVertexAttribPointer(
            vertexHandle,                  // attribute
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
        );
        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(uvHandle);
        glBindBuffer(GL_ARRAY_BUFFER, window->gl.uvbuffer[i]);
        glVertexAttribPointer(
            uvHandle,                                // attribute
            2,                                // size
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalized?
            0,                                // stride
            (void*)0                          // array buffer offset
        );

        // 3rd attribute buffer : normals
        glEnableVertexAttribArray(norHandle);
        glBindBuffer(GL_ARRAY_BUFFER, window->gl.normalbuffer[i]);
        glVertexAttribPointer(
            norHandle,                                // attribute
            3,                                // size
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalized?
            0,                                // stride
            (void*)0                          // array buffer offset
        );

        // Draw the triangles !
        glDrawArrays(GL_TRIANGLES, 0, obj_f[i]*3);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    }

    angle+=0.001f;

    nc_wayland_display_draw(window,(void *)render);
}

static void signal_int()
{
    running = 0;
}

static void	usage(int error_code)
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

    memset(&display, 0, sizeof(display));
    memset(&window, 0, sizeof(window));

    window.display = &display;
    display.window = &window;
    window.window_size.width  = WINDOW_WIDTH;
    window.window_size.height = WINDOW_HEIGHT;

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

    nc_wayland_display_init(&display,(void *)render);

    gl_initialize(&window);

    sigint.sa_handler = (sighandler_t)signal_int;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &sigint, NULL);

    while (running && ret != -1)
    {
        ret = wl_display_dispatch(display.display);
    }

    nc_wayland_display_destroy(&display);

    return 0;
}