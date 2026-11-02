/**
********************************************************************************
* Copyright (C) 2024 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : nc_opengl_init.c
*
* @brief   : nc_opengl_init source
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

#include "nc_opengl_init.h"

GLuint nc_loadTexture(const char * imagepath)
{
    //printf("Loading image %s\n", imagepath);

    int width, height, channels;
    unsigned char* image_data = SOIL_load_image(imagepath, &width, &height, &channels, SOIL_LOAD_AUTO);
    if (!image_data) {
        printf("Failed to load DDS image: %s\n", imagepath);
        return 0;
    }

    // Create one OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set image data to texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

    // Setting filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // TEXTURE MEMORY OFF
    SOIL_free_image_data(image_data);

    // Return the ID of the texture we just created
    return textureID;
}

GLuint nc_loadBMP_custom(const char * imagepath)
{
    printf("Loading Bmp image %s\n", imagepath);

    // Data read from the header of the BMP file
    unsigned char header[54];
    unsigned int dataPos;
    unsigned int imageSize;
    unsigned int width, height;
    // Actual RGB data
    unsigned char * data;

    // Open the file
    FILE * file = fopen(imagepath,"rb");
    if (!file)
    {
        printf("Image could not be opened\n");
        return 0;
    }

    // Read the header, i.e. the 54 first bytes
    // If less than 54 byes are read, problem
    if ( fread(header, 1, 54, file)!=54 ){
        printf("Not a correct BMP file\n");
        return false;
    }
    // A BMP files always begins with "BM"
    if ( header[0]!='B' || header[1]!='M' ){
        printf("Not a correct BMP file\n");
        return 0;
    }
    // Make sure this is a 24bpp file
    if ( *(int*)&(header[0x1E])!=0  )
    {
        printf("Not a correct BMP file\n");
        return 0;
    }
    if ( *(int*)&(header[0x1C])!=24 )
    {
        printf("Not a correct BMP file\n");
        return 0;
    }

    // Read the information about the image
    dataPos    = *(int*)&(header[0x0A]);
    imageSize  = *(int*)&(header[0x22]);
    width      = *(int*)&(header[0x12]);
    height     = *(int*)&(header[0x16]);

    // Some BMP files are misformatted, guess missing information
    if (imageSize==0)
        imageSize=width*height*3; // 3 : one byte for each Red, Green and Blue component
    if (dataPos==0)
        dataPos=54; // The BMP header is done that way

    // Create a buffer
    data = new unsigned char [imageSize];

    // Read the actual data from the file into the buffer
    fread(data,1,imageSize,file);

    // Everything is in memory now, the file wan be closed
    fclose (file);

    // Create one OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Give the image to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    // Poor filtering, or ...
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // ... nice trilinear filtering.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Return the ID of the texture we just created
    return textureID;
}

int nc_loadOBJ(const char * path, Vector3 **pVertices, Vector2 **pUvs, Vector3 **pNormals, int *pObjCount)
{
    printf("Loading OBJ file %s...\n", path);

    int obj_count=0;
    GLuint *vertexIndices, *uvIndices, *normalIndices;

    Vector3   *temp_vertices;
    Vector2   *temp_uvs;
    Vector3   *temp_normals;

    int numVertices = 0;
    int numNormals = 0;
    int numUvs = 0;
    int numIndices = 0;

    FILE * fp = fopen(path, "r");
    if( fp == NULL ){
        printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
        return false;
    }

    // Count the number of vertices, normals, and indices in the file
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == 'v') {
            if (line[1] == ' ') {
                numVertices++;
            } else if (line[1] == 'n') {
                numNormals++;
            } else if (line[1] == 't') {
                numUvs++;
            }
        } else if (line[0] == 'f') {
            numIndices++;
            pObjCount[obj_count-1]=numIndices;
        }else if (line[0] == 'g') {
            obj_count++;
        }
    }

    for(int i=obj_count-1;i>0;i--)
    {
        pObjCount[i]=pObjCount[i]-pObjCount[i-1];
    }

    *pVertices = (Vector3 *)malloc(numIndices * 3 * sizeof(Vector3));
    *pNormals = (Vector3 *)malloc(numIndices  * 3 * sizeof(Vector3));
    *pUvs = (Vector2 *)malloc(numIndices  * 3 * sizeof(Vector2));

    // Allocate memory for vertices, normals, and indices
    temp_vertices = (Vector3 *)malloc(numVertices  * sizeof(Vector3));
    temp_normals = (Vector3 *)malloc(numNormals * sizeof(Vector3));
    temp_uvs = (Vector2 *)malloc(numUvs * sizeof(Vector2));

    vertexIndices = (GLuint *)malloc(numIndices * sizeof(GLuint)*3);
    uvIndices = (GLuint *)malloc(numIndices * sizeof(GLuint)*3);
    normalIndices = (GLuint *)malloc(numIndices * sizeof(GLuint)*3);

    // Read vertices, normals, and indices from the file
    rewind(fp);

    int vertexIndex = 0;
    int uvsIndex = 0;
    int normalIndex = 0;
    unsigned int indexIndex = 0;
    while (fgets(line, sizeof(line), fp))
    {
        if (line[0] == 'v')
        {
            if (line[1] == ' ')
            {
                sscanf(line, "v %f %f %f", &temp_vertices[vertexIndex].x, &temp_vertices[vertexIndex].y, &temp_vertices[vertexIndex].z);
                vertexIndex++;
            }else  if (line[1] == 't')
            {
                GLfloat uv_x, uv_y;
                sscanf(line, "vt %f %f\n", &uv_x, &uv_y );

                uv_y = -uv_y;// Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.

                temp_uvs[uvsIndex].x=uv_x;
                temp_uvs[uvsIndex].y=uv_y;
                uvsIndex++;
            }else if (line[1] == 'n')
            {
                sscanf(line, "vn %f %f %f\n", &temp_normals[normalIndex].x, &temp_normals[normalIndex].y, &temp_normals[normalIndex].z);
                normalIndex++;
            }
        }else if (line[0] == 'f')
        {
            sscanf(line, "f %u/%u/%u %u/%u/%u %u/%u/%u\n", &vertexIndices[indexIndex], &uvIndices[indexIndex], &normalIndices[indexIndex], &vertexIndices[indexIndex+1], &uvIndices[indexIndex+1], &normalIndices[indexIndex+1], &vertexIndices[indexIndex+2], &uvIndices[indexIndex+2], &normalIndices[indexIndex+2] );
            indexIndex += 3;
        }
    }

    // For each vertex of each triangle
    for( unsigned int i=0; i<indexIndex; i++ ){
        // Get the indices of its attributes
        unsigned int vIndex = vertexIndices[i]-1;
        unsigned int uIndex = uvIndices[i]-1;
        unsigned int nIndex = normalIndices[i]-1;

        // Get the attributes thanks to the index
        (*pVertices)[i] = temp_vertices[ vIndex ];
        (*pUvs)[i] = temp_uvs[ uIndex ];
        (*pNormals)[i] = temp_normals[ nIndex ];
    }

    fclose(fp);

    free(temp_vertices);
    free(temp_uvs);
    free(temp_normals);

    free(vertexIndices);
    free(uvIndices);
    free(normalIndices);

    return obj_count;
}

// Compiles the OpenGL ES shader.
GLuint compileShader(GLenum shaderType, const char* source)
{
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint compileStatus;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus != GL_TRUE) {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        GLchar* infoLog = (GLchar*)malloc(infoLogLength + 1);
        glGetShaderInfoLog(shader, infoLogLength, NULL, infoLog);
        printf("Shader compilation failed:\n%s\n", infoLog);
        free(infoLog);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

// Create an OpenGLES program.
GLuint nc_createProgram(const char* vertexSource, const char* fragmentSource)
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        GLchar* infoLog = (GLchar*)malloc(infoLogLength + 1);
        glGetProgramInfoLog(program, infoLogLength, NULL, infoLog);
        printf("Program link failed:\n%s\n", infoLog);
        free(infoLog);
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}
