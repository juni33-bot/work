/**
********************************************************************************
* Copyright (C) 2024 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : nc_opengl_interface.c
*
* @brief   : nc_opengl_interface source
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

#include <stdio.h>

#include "nc_opengl_interface.h"
#include "nc_opengl_shader.h"

void nc_opengl_pixel_to_ndc(float pixel_x, float pixel_y, float* ndc_x, float* ndc_y)
{
    float viewport_x = 0.0f;
    float viewport_y = 0.0f;
    float viewport_width = VIEWPORT_WIDTH;
    float viewport_height = VIEWPORT_HEIGHT;

    *ndc_x = 2.0f * ((float)pixel_x - viewport_x) / viewport_width - 1.0f;
    *ndc_y = 1.0f - 2.0f * ((float)pixel_y - viewport_y) / viewport_height;
}

void nc_opengl_create_rect_vertex(float pixel_x, float pixel_y, float pixel_width, float pixel_height, float vertexCoords[12])
{
    float ndc_x, ndc_y;

    // left top
    nc_opengl_pixel_to_ndc(pixel_x, pixel_y, &ndc_x, &ndc_y);
    vertexCoords[0] = ndc_x;
    vertexCoords[1] = ndc_y;
    vertexCoords[2] = 0.0f;

    // left bottom
    nc_opengl_pixel_to_ndc(pixel_x, pixel_y + pixel_height, &ndc_x, &ndc_y);
    vertexCoords[3] = ndc_x;
    vertexCoords[4] = ndc_y;
    vertexCoords[5] = 0.0f;

    // right bottom
    nc_opengl_pixel_to_ndc(pixel_x + pixel_width, pixel_y + pixel_height, &ndc_x, &ndc_y);
    vertexCoords[6] = ndc_x;
    vertexCoords[7] = ndc_y;
    vertexCoords[8] = 0.0f;

    // right top
    nc_opengl_pixel_to_ndc(pixel_x + pixel_width, pixel_y, &ndc_x, &ndc_y);
    vertexCoords[9] = ndc_x;
    vertexCoords[10] = ndc_y;
    vertexCoords[11] = 0.0f;
}

void nc_opengl_create_line_vertex(float st_x, float st_y, float end_x, float end_y, float vertexCoords[6])
{
    float ndc_x, ndc_y;

    nc_opengl_pixel_to_ndc(st_x, st_y, &ndc_x, &ndc_y);
    vertexCoords[0] = ndc_x;
    vertexCoords[1] = ndc_y;
    vertexCoords[2] = 0.0f;

    nc_opengl_pixel_to_ndc(end_x, end_y, &ndc_x, &ndc_y);
    vertexCoords[3] = ndc_x;
    vertexCoords[4] = ndc_y;
    vertexCoords[5] = 0.0f;
}

void nc_opengl_draw_rectangle(float x, float y, float w, float h, float color[4], struct gl_npu_program g_npu_prog)
{
    float vertexCoords[12];

    nc_opengl_create_rect_vertex(x, y, w, h, vertexCoords);

    // Bind shader
    glUseProgram(g_npu_prog.program);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Update vertex
    glVertexAttribPointer(g_npu_prog.pos_handle, 3, GL_FLOAT, GL_FALSE, 0, vertexCoords);
    glEnableVertexAttribArray(g_npu_prog.pos_handle);

    // Update Drawing Type
    glUniform1i(g_npu_prog.drawing_type_handle, DRAW_NPU_TYPE_SOLID);

    // Update color value (r, g, b, a)
    glUniform4fv(g_npu_prog.color_handle, 1, color);

    // Set line width
    glLineWidth((GLfloat)3.0);

    // Draw Rectangle
    glDrawArrays(GL_LINE_LOOP, 0, 4);

    // Release used resources
    glDisable(GL_BLEND);
    glDisableVertexAttribArray(g_npu_prog.pos_handle);
    glUseProgram(0);
}

void nc_opengl_draw_segmentation(GLuint texture, float** colors, int class_cnt, struct gl_npu_program g_npu_prog)
{
    // Convert geometry
    float image_vertices[] = {
        // x, y
        -1.0f, -1.0f, 0.0f, // bottom left
        -1.0f, 1.0f, 0.0f,  // top left
        1.0f, -1.0f, 0.0f,  // bottom right
        1.0f, 1.0f, 0.0f,   // top right
    };

    float texCoordBuffer[] = {
        0.0f, 1.0f, // bottom left
        0.0f, 0.0f, // top left
        1.0f, 1.0f, // bottom right
        1.0f, 0.0f  // top right
    };

    // Bind shader
    glUseProgram(g_npu_prog.program);

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);

    // Update Drawing Type
    glUniform1i(g_npu_prog.drawing_type_handle, DRAW_NPU_TYPE_SEG);

    // Update vertex
    glVertexAttribPointer(g_npu_prog.pos_handle, 3, GL_FLOAT, GL_FALSE, 0, (const float *)image_vertices);
    glEnableVertexAttribArray(g_npu_prog.pos_handle);

    // Bind texcoord buffer object
    glVertexAttribPointer(g_npu_prog.texcoord_handle, 2, GL_FLOAT, GL_FALSE, 0, (const float *)texCoordBuffer);
    glEnableVertexAttribArray(g_npu_prog.texcoord_handle);

    // Update colors_cnt
    glUniform4fv(g_npu_prog.colors_array_handle, class_cnt, (GLfloat*)colors);

    // Bind texture object
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(g_npu_prog.texture_handle, 0);

    // Draw Rectangle
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Release used resources
    glDisable(GL_BLEND);
    glDisableVertexAttribArray(g_npu_prog.pos_handle);
    glDisableVertexAttribArray(g_npu_prog.texcoord_handle);
    glUseProgram(0);
}

void nc_opengl_draw_line(float st_x, float st_y, float end_x, float end_y, int lane_class, float color[4], struct gl_npu_program g_npu_prog)
{
    float vertexCoords[6];

    nc_opengl_create_line_vertex(st_x, st_y, end_x, end_y, vertexCoords);
    
    glUseProgram(g_npu_prog.program);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glVertexAttribPointer(g_npu_prog.pos_handle, 3, GL_FLOAT, GL_FALSE, 0, vertexCoords);
    glEnableVertexAttribArray(g_npu_prog.pos_handle);
    
    if(lane_class == 2) //dashed line
    {
        glUniform2f(g_npu_prog.viewport_size_handle, (float)VIEWPORT_WIDTH, (float)VIEWPORT_HEIGHT);
        glUniform1i(g_npu_prog.drawing_type_handle, DRAW_NPU_TYPE_DASHED);
    }
    else //solid line
    {
        glUniform1i(g_npu_prog.drawing_type_handle, DRAW_NPU_TYPE_SOLID);
    }
    glUniform4fv(g_npu_prog.color_handle, 1, color);

    glLineWidth((GLfloat)6.0);
    
    glDrawArrays(GL_LINES, 0, (GLsizei) 2);
    
    glDisable(GL_BLEND);
    glDisableVertexAttribArray(g_npu_prog.pos_handle);
    glUseProgram(0);
}

#ifdef USE_UFLD_NETWORK_DEBUGGING
void nc_opengl_draw_debugging_grid_line(float st_x, float st_y, float end_x, float end_y, int lane_class, float color[4], struct gl_npu_program g_npu_prog)
{
    float vertexCoords[6];

    nc_opengl_create_line_vertex(st_x, st_y, end_x, end_y, vertexCoords);

    glUseProgram(g_npu_prog.program);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glVertexAttribPointer(g_npu_prog.pos_handle, 3, GL_FLOAT, GL_FALSE, 0, vertexCoords);
    glEnableVertexAttribArray(g_npu_prog.pos_handle);
    
    if(lane_class == 2) //dashed line
    {
        glUniform2f(g_npu_prog.viewport_size_handle, (float)VIEWPORT_WIDTH, (float)VIEWPORT_HEIGHT);
        glUniform1i(g_npu_prog.drawing_type_handle, DRAW_NPU_TYPE_DASHED);
    }
    else //solid line
    {
        glUniform1i(g_npu_prog.drawing_type_handle, DRAW_NPU_TYPE_SOLID);
    }
    glUniform4fv(g_npu_prog.color_handle, 1, color);

    glLineWidth((GLfloat)3.0);
    
    glDrawArrays(GL_LINES, 0, (GLsizei) 2);
    
    glDisable(GL_BLEND);
    glDisableVertexAttribArray(g_npu_prog.pos_handle);
    glUseProgram(0);
}
#endif

void nc_opengl_init_npu_shader(struct gl_npu_program *npu_prog)
{
    // Initialize Shader
    npu_prog->program = nc_createProgram(vertexShaderNpu, fragmentShaderNpu);

    // Bind shader attributes
    npu_prog->pos_handle = glGetAttribLocation(npu_prog->program, "a_position");
    npu_prog->texcoord_handle = glGetAttribLocation(npu_prog->program, "a_texcoord");

    // Bind shader uniforms
    npu_prog->color_handle = glGetUniformLocation(npu_prog->program, "color");
    npu_prog->drawing_type_handle = glGetUniformLocation(npu_prog->program, "drawingType");
    npu_prog->viewport_size_handle = glGetUniformLocation(npu_prog->program, "viewportSize");
    npu_prog->texture_handle = glGetUniformLocation(npu_prog->program, "s_texture");
    npu_prog->colors_array_handle = glGetUniformLocation(npu_prog->program, "colors_array");
}

void nc_opengl_draw_texture(GLint texture, window *window)
{
    // Use our shader
    glUseProgram(window->gl.program);

    // Update vertex
    glEnableVertexAttribArray(window->gl.positionHandle);
    glBindBuffer(GL_ARRAY_BUFFER, window->gl.vertexbuffer[0]);
    glVertexAttribPointer(window->gl.positionHandle, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Update texcoord
    glEnableVertexAttribArray(window->gl.texCoordHandle);
    glBindBuffer(GL_ARRAY_BUFFER, window->gl.coordtexbuffer);
    glVertexAttribPointer(window->gl.texCoordHandle, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Bind texture object
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(window->gl.textureID[0], 0);

    // Draw call
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Release used resources
    glDisableVertexAttribArray(window->gl.positionHandle);
    glDisableVertexAttribArray(window->gl.texCoordHandle);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

void nc_opengl_init_video_shader(window *window, int use_yuv_shader)
{
    // Initialize Shader
    if(use_yuv_shader) {
        window->gl.program = nc_createProgram(vertexShaderSourceVideo, fragmentShaderSourceVideo_yuv);
    } else {
        window->gl.program = nc_createProgram(vertexShaderSourceVideo, fragmentShaderSourceVideo);
    }

    // Bind shader attributes
    window->gl.positionHandle = glGetAttribLocation(window->gl.program, "aPosition");
    window->gl.texCoordHandle = glGetAttribLocation(window->gl.program, "aTexCoord");

    // Bind shader uniforms
    window->gl.textureID[0] = glGetUniformLocation(window->gl.program, "texture1");

    // Initialize VBO
    float image_vertices[] = {
        // x, y
        -1.0f, -1.0f, 0.0f, // bottom left
        -1.0f, 1.0f, 0.0f,  // top left
        1.0f, -1.0f, 0.0f,  // bottom right
        1.0f, 1.0f, 0.0f,   // top right
    };

    float texCoordBuffer[] = {
        0.0f, 1.0f, // bottom left
        0.0f, 0.0f, // top left
        1.0f, 1.0f, // bottom right
        1.0f, 0.0f  // top right
    };

    glGenBuffers(1, &window->gl.vertexbuffer[0]);
    glBindBuffer(GL_ARRAY_BUFFER, window->gl.vertexbuffer[0]);
    glBufferData(GL_ARRAY_BUFFER,  4*3*sizeof(float), &image_vertices[0], GL_STATIC_DRAW);

    glGenBuffers(1, &window->gl.coordtexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, window->gl.coordtexbuffer);
    glBufferData(GL_ARRAY_BUFFER,  4*2*sizeof(float), &texCoordBuffer[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
