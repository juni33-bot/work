#include <string.h>
#include "matrix.h"

struct mat4f mat4f_identity = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
};

Mat4 mat4_multiply(Mat4 a, Mat4 b) {
    Mat4 result = {0};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.data[i][j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                result.data[i][j] += a.data[i][k] * b.data[k][j];
            }
        }
    }
    return result;
}

// Function to calculate the dot product of two Vec3 vectors
float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Return the given matrix in a format understood by OpenGL.
float* mat4f_gl(struct mat4f* m) {
    // Since it's already in colum-major order, we just return the address of the
    // first element.
    return &m->x11;
}

struct mat4f mat4f_multiply(struct mat4f a, struct mat4f b) {
    struct mat4f result;

    result.x11 = a.x11 * b.x11 + a.x12 * b.x21 + a.x13 * b.x31 + a.x14 * b.x41;
    result.x21 = a.x21 * b.x11 + a.x22 * b.x21 + a.x23 * b.x31 + a.x24 * b.x41;
    result.x31 = a.x31 * b.x11 + a.x32 * b.x21 + a.x33 * b.x31 + a.x34 * b.x41;
    result.x41 = a.x41 * b.x11 + a.x42 * b.x21 + a.x43 * b.x31 + a.x44 * b.x41;

    result.x12 = a.x11 * b.x12 + a.x12 * b.x22 + a.x13 * b.x32 + a.x14 * b.x42;
    result.x22 = a.x21 * b.x12 + a.x22 * b.x22 + a.x23 * b.x32 + a.x24 * b.x42;
    result.x32 = a.x31 * b.x12 + a.x32 * b.x22 + a.x33 * b.x32 + a.x34 * b.x42;
    result.x42 = a.x41 * b.x12 + a.x42 * b.x22 + a.x43 * b.x32 + a.x44 * b.x42;

    result.x13 = a.x11 * b.x13 + a.x12 * b.x23 + a.x13 * b.x33 + a.x14 * b.x43;
    result.x23 = a.x21 * b.x13 + a.x22 * b.x23 + a.x23 * b.x33 + a.x24 * b.x43;
    result.x33 = a.x31 * b.x13 + a.x32 * b.x23 + a.x33 * b.x33 + a.x34 * b.x43;
    result.x43 = a.x41 * b.x13 + a.x42 * b.x23 + a.x43 * b.x33 + a.x44 * b.x43;

    result.x14 = a.x11 * b.x14 + a.x12 * b.x24 + a.x13 * b.x34 + a.x14 * b.x44;
    result.x24 = a.x21 * b.x14 + a.x22 * b.x24 + a.x23 * b.x34 + a.x24 * b.x44;
    result.x34 = a.x31 * b.x14 + a.x32 * b.x24 + a.x33 * b.x34 + a.x34 * b.x44;
    result.x44 = a.x41 * b.x14 + a.x42 * b.x24 + a.x43 * b.x34 + a.x44 * b.x44;

    return result;
}

struct mat4f mat4f_scale(float x, float y, float z) {
    struct mat4f result;

    result.x11 = x; result.x21 = 0; result.x31 = 0; result.x41 = 0;
    result.x12 = 0; result.x22 = y; result.x32 = 0; result.x42 = 0;
    result.x13 = 0; result.x23 = 0; result.x33 = z; result.x43 = 0;
    result.x14 = 0; result.x24 = 0; result.x34 = 0; result.x44 = 1;

    return result;
}

struct mat4f mat4f_translation(float x, float y, float z) {
    struct mat4f result;

    result.x11 = 1; result.x21 = 0; result.x31 = 0; result.x41 = 0;
    result.x12 = 0; result.x22 = 1; result.x32 = 0; result.x42 = 0;
    result.x13 = 0; result.x23 = 0; result.x33 = 1; result.x43 = 0;
    result.x14 = x; result.x24 = y; result.x34 = z; result.x44 = 1;

    return result;
}

struct mat4f mat4f_rotate_z(float theta) {
    struct mat4f result;

    result.x11 =  cos(theta); result.x21 = sin(theta); result.x31 = 0; result.x41 = 0;
    result.x12 = -sin(theta); result.x22 = cos(theta); result.x32 = 0; result.x42 = 0;
    result.x13 =           0; result.x23 =          0; result.x33 = 1; result.x43 = 0;
    result.x14 =           0; result.x24 =          0; result.x34 = 0; result.x44 = 1;

    return result;
}

struct mat4f mat4f_rotate_y(float theta) {
    struct mat4f result;

    result.x11 = cos(theta); result.x21 = 0; result.x31 = -sin(theta); result.x41 = 0;
    result.x12 =          0; result.x22 = 1; result.x32 =           0; result.x42 = 0;
    result.x13 = sin(theta); result.x23 = 0; result.x33 =  cos(theta); result.x43 = 0;
    result.x14 =          0; result.x24 = 0; result.x34 =           0; result.x44 = 1;

    return result;
}

struct mat4f mat4f_rotate_x(float theta) {
    struct mat4f result;

    result.x11 = 1; result.x21 = 0;           result.x31 = 0;          result.x41 = 0;
    result.x12 = 0; result.x22 = cos(theta);  result.x32 = sin(theta); result.x42 = 0;
    result.x13 = 0; result.x23 = -sin(theta); result.x33 = cos(theta); result.x43 = 0;
    result.x14 = 0; result.x24 = 0;           result.x34 = 0;          result.x44 = 1;

    return result;
}

struct mat4f mat4f_perspective() {
    struct mat4f result;
    // Based on http://www.songho.ca/opengl/gl_projectionmatrix.html, which I don't
    // really understand. I just copied the final result.

    const float
        r = 1,//0.5,  // Half of the viewport width (at the near plane)
        t = 1,//0.5,  // Half of the viewport height (at the near plane)
        n = 1,  // Distance to near clipping plane
        f = 10;  // Distance to far clipping plane

    // Note that while n and f are given as positive integers above,
    // the camera is looking in the negative direction. So we will see
    // stuff between z = -n and z = -f.

    result.x11 = n / r; result.x21 = 0;     result.x31 = 0;                     result.x41 = 0;
    result.x12 = 0;     result.x22 = n / t; result.x32 = 0;                     result.x42 = 0;
    result.x13 = 0;     result.x23 = 0;     result.x33 = (-f - n) / (f - n);    result.x43 = -1;
    result.x14 = 0;     result.x24 = 0;     result.x34 = (2 * f * n) / (n - f); result.x44 = 0;

    return result;
}

// Function to create an identity matrix
Mat4 mat4_identity() {
    Mat4 m = {0};
    m.data[0][0] = m.data[1][1] = m.data[2][2] = m.data[3][3] = 1.0f;
    return m;
}

// Function to create a perspective projection matrix
Mat4 mat4_perspective(float fov, float aspect, float near, float far) {
    Mat4 m = {0};
    float tanHalfFov = tanf(fov * 0.5f);
    m.data[0][0] = 1.0f / (aspect * tanHalfFov);
    m.data[1][1] = 1.0f / tanHalfFov;
    m.data[2][2] = -(far + near) / (far - near);
    m.data[2][3] = -1.0f;
    m.data[3][2] = -(2.0f * far * near) / (far - near);
    return m;
}

// Function to create a look-at view matrix
Mat4 mat4_lookAt(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 f = {target.x - eye.x, target.y - eye.y, target.z - eye.z};
    Vec3 f_normalized;

    memset(&f_normalized, 0, sizeof(Vec3));

    float length = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
    if (length != 0.0f) {
        f_normalized.x = f.x / length;
        f_normalized.y = f.y / length;
        f_normalized.z = f.z / length;
    }
    Vec3 r;

    memset(&r, 0, sizeof(Vec3));

    r.x = up.y * f_normalized.z - up.z * f_normalized.y;
    r.y = up.z * f_normalized.x - up.x * f_normalized.z;
    r.z = up.x * f_normalized.y - up.y * f_normalized.x;
    float r_length = sqrtf(r.x * r.x + r.y * r.y + r.z * r.z);
    if (r_length != 0.0f) {
        r.x /= r_length;
        r.y /= r_length;
        r.z /= r_length;
    }
    Vec3 u;

    memset(&u, 0, sizeof(Vec3));

    u.x = f_normalized.y * r.z - f_normalized.z * r.y;
    u.y = f_normalized.z * r.x - f_normalized.x * r.z;
    u.z = f_normalized.x * r.y - f_normalized.y * r.x;
    Mat4 view = mat4_identity();
    view.data[0][0] = r.x;
    view.data[1][0] = r.y;
    view.data[2][0] = r.z;
    view.data[0][1] = u.x;
    view.data[1][1] = u.y;
    view.data[2][1] = u.z;
    view.data[0][2] = -f_normalized.x;
    view.data[1][2] = -f_normalized.y;
    view.data[2][2] = -f_normalized.z;
    view.data[3][0] = -vec3_dot(r, eye);
    view.data[3][1] = -vec3_dot(u, eye);
    view.data[3][2] = vec3_dot(f_normalized, eye);
    return view;
}
