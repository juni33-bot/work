#pragma once

#include <stdio.h>
#include <math.h>

const double pi = 3.141593;

struct mat4f {
    // Column-major order
    float
        x11, x21, x31, x41,
        x12, x22, x32, x42,
        x13, x23, x33, x43,
        x14, x24, x34, x44;
};

typedef struct {
    float data[4][4];
} Mat4;

typedef struct {
    float x, y;
} Vec2;

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    Vec2 position;
    Vec2 uv;
} Vertex;

struct Point3f{
    float x ,y ,z;
};

extern struct mat4f mat4f_identity;

Mat4 mat4_multiply(Mat4 a, Mat4 b);
// Function to calculate the dot product of two Vec3 vectors
float vec3_dot(Vec3 a, Vec3 b);
// Return the given matrix in a format understood by OpenGL.
float* mat4f_gl(struct mat4f* m);
struct mat4f mat4f_multiply(struct mat4f a, struct mat4f b);
struct mat4f mat4f_scale(float x, float y, float z);
struct mat4f mat4f_translation(float x, float y, float z);
struct mat4f mat4f_rotate_z(float theta);
struct mat4f mat4f_rotate_y(float theta);
struct mat4f mat4f_rotate_x(float theta);
struct mat4f mat4f_perspective();
// Function to create an identity matrix
Mat4 mat4_identity();
// Function to create a perspective projection matrix
Mat4 mat4_perspective(float fov, float aspect, float near, float far);
// Function to create a look-at view matrix
Mat4 mat4_lookAt(Vec3 eye, Vec3 target, Vec3 up);


