/**
********************************************************************************
* Copyright (C) 2024 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : nc_opengl_shader.c
*
* @brief   : nc_opengl_shader source (GLSL)
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

#include "nc_opengl_shader.h"

const char *fragmentShaderSourceVideo_yuv =
    "precision mediump float;\n"
    "varying vec2 vTexCoord;\n"
    "uniform sampler2D texture1;\n"
    "void main()\n"
    "{\n"
    "    float Y0,Y1,U,V,R0,G0,B0,R1,G1,B1;\n"
    "    Y0 = texture2D(texture1, vTexCoord).r;\n"
    "    U = texture2D(texture1, vTexCoord).g;\n"
    "    Y1 = texture2D(texture1, vTexCoord).b;\n"
    "    V = texture2D(texture1, vTexCoord).a;\n"
    "    U = U - 0.5;\n"
    "    V = V - 0.5;\n"
    "    R0 = Y0 + 1.402 * V;\n"
    "    G0 = Y0 - 0.344136 * U - 0.714136 * V;\n"
    "    B0 = Y0 + 1.772 * U;\n"
    " \n"
    "    R1 = Y1 + 1.402 * V;\n"
    "    G1 = Y1 - 0.344136 * U - 0.714136 * V;\n"
    "    B1 = Y1 + 1.772 * U;\n"
    " \n"
    "    R0 = clamp(R0, 0.0, 1.0);\n"
    "    G0 = clamp(G0, 0.0, 1.0);\n"
    "    B0 = clamp(B0, 0.0, 1.0);\n"
    " \n"
    "    R1 = clamp(R1, 0.0, 1.0);\n"
    "    G1 = clamp(G1, 0.0, 1.0);\n"
    "    B1 = clamp(B1, 0.0, 1.0);\n"
    " \n"
    "    vec3 rgbColor = 0.5 * vec3(R0 + R1, G0 + G1, B0 + B1);\n"
    "    gl_FragColor = vec4(rgbColor, 1.0);\n"
    "}\n";

const char *fragmentShaderSourceVideo =
    "precision mediump float;\n"
    "varying vec2 vTexCoord;\n"
    "uniform sampler2D texture1;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = texture2D(texture1, vTexCoord);\n"
    "}\n";

const char *vertexShaderSourceVideo =
    "attribute vec4  aPosition;\n"
    "attribute vec2  aTexCoord;\n"
    "varying vec2 vTexCoord;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = aPosition;\n"
    "    vTexCoord = aTexCoord;\n"
    "}\n";

const char *fragmentShaderSourceEx =
    // Interpolated values from the vertex shaders
    "precision mediump float;\n"
    "varying vec2 UV;\n"
    "varying vec3 Position_worldspace;\n"
    "varying vec3 Normal_cameraspace;\n"
    "varying vec3 EyeDirection_cameraspace;\n"
    "varying vec3 LightDirection_cameraspace;\n"
    // Values that stay constant for the whole mesh.
    "uniform sampler2D myTextureSampler;\n"
    "uniform mat4 MV;\n"
    "uniform vec3 LightPosition_worldspace;\n"
    "void main(){\n"
        // Light emission properties
        // You probably want to put them as uniforms
        "vec3 LightColor = vec3(1,1,1);\n"
        "float LightPower = 50.0;\n"
        // Material properties
        "vec3 MaterialDiffuseColor = texture2D( myTextureSampler, UV).rgb;\n"
        "vec3 MaterialAmbiantColor = vec3(0.1,0.1,0.1) * MaterialDiffuseColor;\n"
        "vec3 MaterialSpecularColor = vec3(0.3,0.3,0.3);\n"

        // Distance to the light
        "float distance = length( LightPosition_worldspace - Position_worldspace );\n"

        // Normal of the computed fragment, in camera space
        "vec3 n = normalize( Normal_cameraspace );\n"
        // Direction of the light (from the fragment to the light)
        "vec3 l = normalize( LightDirection_cameraspace );\n"
        // Cosine of the angle between the normal and the light direction,
        // clamped above 0
        //  - light is at the vertical of the triangle -> 1
        //  - light is perpendicular to the triangle -> 0
        //  - light is behind the triangle -> 0
        "float cosTheta = max(min(dot(n, l), 1.0), 0.0);\n"

        // Eye vector (towards the camera)
        "vec3 E = normalize(EyeDirection_cameraspace);\n"
        // Direction in which the triangle reflects the light
        "vec3 R = reflect(-l,n);\n"
        // Cosine of the angle between the Eye vector and the Reflect vector,
        // clamped to 0
        //  - Looking into the reflection -> 1
        //  - Looking elsewhere -> < 1
        "float cosAlpha = max(min(dot(E, R), 1.0), 0.0);\n"

        // Ambiant : simulates indirect lighting
        // Diffuse : "color" of the object
        // Specular : reflective highlight, like a mirror
        // Output color = color of the texture at the specified UV
        "gl_FragColor = vec4(MaterialAmbiantColor + MaterialDiffuseColor * LightColor * LightPower * cosTheta / (distance*distance) + MaterialSpecularColor * LightColor * LightPower * pow(cosAlpha,5.0) / (distance*distance),1.0);\n"
    "}\n";

const char *vertexShaderSourceEx =
    "precision mediump float;\n"
    // Input vertex data, different for all executions of this shader.
    "attribute vec3 vertexPosition_modelspace;\n"
    "attribute vec2 vertexUV;\n"
    "attribute vec3 vertexNormal_modelspace;\n"
    // Output data ; will be interpolated for each fragment.
    "varying vec2 UV;\n"
    "varying vec3 Position_worldspace;\n"
    "varying vec3 Normal_cameraspace;\n"
    "varying vec3 EyeDirection_cameraspace;\n"
    "varying vec3 LightDirection_cameraspace;\n"
    // Values that stay constant for the whole mesh.
    "uniform mat4 MVP;\n"
    "uniform mat4 V;\n"
    "uniform mat4 M;\n"
    "uniform vec3 LightPosition_worldspace;\n"
    "void main(){\n"
        // Output position of the vertex, in clip space : MVP * position
    "   gl_Position =  MVP * vec4(vertexPosition_modelspace,1.0);\n"
        // Position of the vertex, in worldspace : M * position
    "   Position_worldspace = (M * vec4(vertexPosition_modelspace,1)).xyz;\n"
        // Vector that goes from the vertex to the camera, in camera space.
        // In camera space, the camera is at the origin (0,0,0).
    "   vec3 vertexPosition_cameraspace = ( V * M * vec4(vertexPosition_modelspace,1)).xyz;\n"
    "   EyeDirection_cameraspace = vec3(0,0,0) - vertexPosition_cameraspace;\n"

        // Vector that goes from the vertex to the light, in camera space. M is ommited because it's identity.
    "   vec3 LightPosition_cameraspace = ( V * vec4(LightPosition_worldspace,1)).xyz;\n"
    "   LightDirection_cameraspace = LightPosition_cameraspace + EyeDirection_cameraspace;\n"

        // Normal of the the vertex, in camera space
    "   Normal_cameraspace = ( V * M * vec4(vertexNormal_modelspace,0)).xyz;\n" // Only correct if ModelMatrix does not scale the model ! Use its inverse transpose if not.
        // UV of the vertex. No special space for this one.
    "   UV = vertexUV;\n"
    "}\n";

const char *fragmentShaderNpu =
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform vec4 color;\n"
    "uniform int drawingType;\n"
    "uniform sampler2D s_texture;\n"
    "uniform vec4 colors_array[10];\n"
    "uniform vec2 viewportSize;\n"
    "in vec2 v_texcoord;\n"
    "flat in vec3 startPos;\n"
    "in vec3 vertPos;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    // Type for drawing solid line
    "   if (drawingType == 0) {\n"
    "       fragColor = color;\n"
    "   }\n"
    // Type for drawing dashed line
    "   if (drawingType == 1) {\n"
    "       vec2 dir = (vertPos.xy-startPos.xy) * viewportSize/2.0;\n"
    "       float dist = length(dir);\n"
    "       if(fract(dist/ 30.0) > 0.5) discard;\n"
    "       fragColor = color;\n"
    "   }\n"
    // Type for drawing semantic segmentation results
    "   if (drawingType == 2) {\n"
    "       float id = texture(s_texture, v_texcoord).r * 255.0;\n"
    "       int index = int(id + 0.5);\n"
    "       fragColor = colors_array[index];\n"
    "   }\n"
    "}\n";

const char *vertexShaderNpu =
    "#version 300 es\n"
    "in vec4 a_position;\n"
    "in vec2 a_texcoord;\n"
    "out vec2 v_texcoord;\n"
    "flat out vec3 startPos;\n"
    "out vec3 vertPos;\n"
    "void main() {\n"
    "    gl_Position = a_position;\n"
    "    v_texcoord = a_texcoord;\n"
    "    startPos = a_position.xyz;\n"
    "    vertPos = a_position.xyz;\n"
    "}\n";

const char *TextVertexShaderSource =
    "precision mediump float;\n"
    // Input vertex data, different for all executions of this shader.
    "attribute vec2 vertexPosition_screenspace;\n"
    "attribute vec2 vertexUV;\n"
    // Output data ; will be interpolated for each fragment.
    "varying vec2 UV;\n"
    "void main(){\n"
        // Output position of the vertex, in clip space
        // map [0..800][0..600] to [-1..1][-1..1]
    "    vec2 vertexPosition_homoneneousspace = vertexPosition_screenspace - vec2(960,540);\n" // [0..800][0..600] -> [-400..400][-300..300]
    "    vertexPosition_homoneneousspace /= vec2(960,540);\n"
    "    gl_Position =  vec4(vertexPosition_homoneneousspace,0,1);\n"
        // UV of the vertex. No special space for this one.
    "    UV = vertexUV;\n"
    "}\n";

const char *TextFragmentShaderSource =
    // Interpolated values from the vertex shaders
    "precision mediump float;\n"
    "varying vec2 UV;\n"
    // Values that stay constant for the whole mesh.
    "uniform sampler2D myTextureSampler;\n"
    "uniform vec4 colorModifier;\n"
    "void main(){\n"
        // Output color = color of the texture at the specified UV
    "    vec4 textureColor = texture2D(myTextureSampler, UV);\n"
    "    vec4 modifiedColor = textureColor * colorModifier;\n"
    "    gl_FragColor = modifiedColor;\n"
    "}\n";

const char *fragmentShaderFont =
    "precision mediump float;\n"
    "varying vec2 TexCoords;\n"
    "uniform sampler2D s_texture;\n"
    "uniform vec3 color;\n"
    "void main() {\n"
    "    vec4 sampled = vec4(1.0, 1.0, 1.0, texture2D(s_texture, TexCoords).a);\n"
    "    gl_FragColor = vec4(color, 1.0) * sampled;\n"
    "}\n";

const char *vertexShaderFont =
    "attribute vec2 a_position;\n"
    "attribute vec2 a_texcoord;\n"
    "uniform vec2 screenSize;\n"
    "varying vec2 TexCoords;\n"
    "void main() {\n"
    "    vec2 normalizedPosition = (a_position / screenSize) * 2.0 - 1.0;\n"
    //"    normalizedPosition.y = -normalizedPosition.y;\n"
    "    gl_Position = vec4(normalizedPosition, 0.0, 1.0);\n"
    "    TexCoords = a_texcoord;\n"
    "}\n";
