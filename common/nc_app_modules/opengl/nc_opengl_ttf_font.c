/**
********************************************************************************
* Copyright (C) 2024 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : nc_opengl_ttf_font.c
*
* @brief   : nc_opengl_ttf_font source
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

#include "nc_opengl_ttf_font.h"
#include "nc_opengl_shader.h"

#include <ft2build.h>
#include FT_FREETYPE_H

void nc_opengl_load_font(const char *fontPath, int fontSize, Font *font)
{
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        fprintf(stderr, "Could not init FreeType Library\n");
        return;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath, 0, &face)) {
        fprintf(stderr, "Failed to load font\n");
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            fprintf(stderr, "Failed to load Glyph\n");
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_ALPHA,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_ALPHA,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        font->characters[c].textureID = texture;
        font->characters[c].width = face->glyph->bitmap.width;
        font->characters[c].height = face->glyph->bitmap.rows;
        font->characters[c].bearingX = face->glyph->bitmap_left;
        font->characters[c].bearingY = face->glyph->bitmap_top;
        font->characters[c].advance = face->glyph->advance.x;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void nc_opengl_draw_text(Font *font, const char *text, float x, float y, float scale, float color[3], int screen_width, int screen_height, struct gl_font_program font_prog)
{
    // geometry
    GLfloat vertexPositions[8]; // 4 vertices * 2 components (x, y)
    GLfloat vertexTexCoords[8]; // 4 vertices * 2 components (s, t)

    // Bind shader
    glUseProgram(font_prog.program);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnableVertexAttribArray(font_prog.vertex_handle);
    glEnableVertexAttribArray(font_prog.texcoord_handle);

    // Update screen_size
    glUniform2f(font_prog.screen_size_handle, (float)screen_width, (float)screen_height);

    // Update color
    glUniform3fv(font_prog.color_handle, 1, color);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(font_prog.texture_handle, 0);

    for (const char *c = text; *c != '\0'; c++) {
        Character ch = font->characters[(unsigned char)*c];

        float xpos = x + (float)ch.bearingX * scale;
        float ypos = y - (float)(ch.height - ch.bearingY) * scale;

        float w = (float)ch.width * scale;
        float h = (float)ch.height * scale;

        vertexPositions[0] = xpos;        vertexPositions[1] = ypos + h;
        vertexPositions[2] = xpos;        vertexPositions[3] = ypos;
        vertexPositions[4] = xpos + w;    vertexPositions[5] = ypos + h;
        vertexPositions[6] = xpos + w;    vertexPositions[7] = ypos;

        vertexTexCoords[0] = 0.0f; vertexTexCoords[1] = 0.0f;
        vertexTexCoords[2] = 0.0f; vertexTexCoords[3] = 1.0f;
        vertexTexCoords[4] = 1.0f; vertexTexCoords[5] = 0.0f;
        vertexTexCoords[6] = 1.0f; vertexTexCoords[7] = 1.0f;

        // Bind texture object
        glBindTexture(GL_TEXTURE_2D, ch.textureID);

        // Update vertex, texcoord
        glVertexAttribPointer(font_prog.vertex_handle, 2, GL_FLOAT, GL_FALSE, 0, vertexPositions);
        glVertexAttribPointer(font_prog.texcoord_handle, 2, GL_FLOAT, GL_FALSE, 0, vertexTexCoords);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        x += (float)(ch.advance >> 6) * scale;
    }

    // Release used resources
    glDisable(GL_BLEND);
    glDisableVertexAttribArray(font_prog.vertex_handle);
    glDisableVertexAttribArray(font_prog.texcoord_handle);
    glUseProgram(0);
}

void nc_opengl_init_font_shader(struct gl_font_program *font_prog)
{
    // Initialize Shader
    font_prog->program = nc_createProgram(vertexShaderFont, fragmentShaderFont);

    // Bind shader attributes
    font_prog->vertex_handle = glGetAttribLocation(font_prog->program, "a_position");
    font_prog->texcoord_handle = glGetAttribLocation(font_prog->program, "a_texcoord");

    // Bind shader uniforms
    font_prog->screen_size_handle = glGetUniformLocation(font_prog->program, "screenSize");
    font_prog->color_handle = glGetUniformLocation(font_prog->program, "color");
    font_prog->texture_handle = glGetUniformLocation(font_prog->program, "s_texture");
}
