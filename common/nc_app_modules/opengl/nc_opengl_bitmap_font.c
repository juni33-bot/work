/**
********************************************************************************
* Copyright (C) 2024 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : nc_opengl_bitmap_font.c
*
* @brief   : nc_opengl_bitmap_font source
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

#include "nc_opengl_bitmap_font.h"
#include "nc_opengl_init.h"
#include "nc_opengl_shader.h"

void nc_initText2D(window *window){

    // Initialize texture
    window->font.FontTexture = nc_loadTexture("misc/texture/holstein.tga");

    // Initialize VBO
    glGenBuffers(1, &window->font.vertexBufferID);
    glGenBuffers(1, &window->font.uvBufferID);

    // Initialize Shader
    window->font.program = nc_createProgram(TextVertexShaderSource, TextFragmentShaderSource);

    // Initialize uniforms' IDs
    window->font.FontTextureID = glGetUniformLocation(window->font.program, "myTextureSampler" );

    // Get the location of the uniform variable in the shader
    window->font.colorModifier = glGetUniformLocation(window->font.program, "colorModifier");
}

void nc_printText2D(const char *text, int x, int y, int size, float textcolor[4],
                 window *window) {
    unsigned int length = (unsigned int)strlen(text);
    unsigned int numVertices = 6 * length;

    float size_x = (float)(size*0.6);

    float* vertices = (float*)malloc(numVertices * 2 * sizeof(float));
    float* UVs = (float*)malloc(numVertices * 2 * sizeof(float));

    for (unsigned int i = 0; i < length; i++) {
        float xpos = (float)x + (float)i * size_x;
        float ypos = (float)y;

        float uv_x = (float)(text[i] % 16) / 16.0f;
        float uv_y = (float)(text[i] / 16) / 16.0f;

        float uv_x1 = uv_x;
        float uv_x2 = uv_x + 1.0f / 16.0f;
        float uv_y1 = 1.0f - uv_y;
        float uv_y2 = 1.0f - (uv_y + 1.0f / 16.0f);

        // Vertices
        float *vertexPtr = &vertices[i * 12];
        vertexPtr[0] = xpos;         vertexPtr[1] = ypos + (float)size;
        vertexPtr[2] = xpos;         vertexPtr[3] = ypos;
        vertexPtr[4] = xpos + size_x;  vertexPtr[5] = ypos;
        vertexPtr[6] = xpos + size_x;  vertexPtr[7] = ypos;
        vertexPtr[8] = xpos + size_x;  vertexPtr[9] = ypos + (float)size;
        vertexPtr[10] = xpos;        vertexPtr[11] = ypos + (float)size;

        // UVs
        float *uvPtr = &UVs[i * 12];
        uvPtr[0] = uv_x1;  uvPtr[1] = uv_y1;
        uvPtr[2] = uv_x1;  uvPtr[3] = uv_y2;
        uvPtr[4] = uv_x2;  uvPtr[5] = uv_y2;
        uvPtr[6] = uv_x2;  uvPtr[7] = uv_y2;
        uvPtr[8] = uv_x2;  uvPtr[9] = uv_y1;
        uvPtr[10] = uv_x1; uvPtr[11] = uv_y1;
    }

    glBindBuffer(GL_ARRAY_BUFFER, window->font.vertexBufferID);
    glBufferData(GL_ARRAY_BUFFER, length * 6 * 2 * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, window->font.uvBufferID);
    glBufferData(GL_ARRAY_BUFFER, length * 6 * 2 * sizeof(float), &UVs[0], GL_STATIC_DRAW);

    // Bind shader
    glUseProgram(window->font.program);

    // Set the color modifier value (r, g, b, a)
    glUniform4fv(window->font.colorModifier, 1, textcolor);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, window->font.FontTexture);
    // Set our "myTextureSampler" sampler to user Texture Unit 0
    glUniform1i(window->font.FontTextureID, 0);

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, window->font.vertexBufferID);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0 );

    // 2nd attribute buffer : UVs
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, window->font.uvBufferID);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0 );

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw call
    glDrawArrays(GL_TRIANGLES, 0, 6 * length);

    glDisable(GL_BLEND);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    free(vertices);
    free(UVs);
}
