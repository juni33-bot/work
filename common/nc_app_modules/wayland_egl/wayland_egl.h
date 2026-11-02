
#ifndef _WAYLAND_EGL_H_
#define _WAYLAND_EGL_H_

#define WINDOW_WIDTH    1920
#define WINDOW_HEIGHT   1080

#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>

struct display {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_shell *shell;
    struct wl_seat *seat;
    struct wl_pointer *pointer;
    struct wl_keyboard *keyboard;
    struct wl_shm *shm;
    struct wl_cursor_theme *cursor_theme;
    struct wl_cursor *default_cursor;
    struct wl_surface *cursor_surface;
    struct {
        EGLDisplay dpy;
        EGLContext ctx;
        EGLConfig conf;
    } egl;
    struct window *window;
};

struct geometry {
    int width, height;
};

struct window {
    struct display *display;
    struct geometry geometry, window_size;
    struct {
        GLuint objcount;
        GLuint modelMatrixLoc;
        GLuint MatrixID;
        GLuint ViewMatrixID;
        GLuint ModelMatrixID;
        GLuint TextureID;
        GLuint LightID;

        GLint vertexHandle;
        GLint uvHandle;
        GLint norHandle;

        GLuint Texture[10];
        GLuint uvbuffer[10];
        GLuint normalbuffer[10];

        GLuint positionHandle ;
        GLuint texCoordHandle ;

        GLuint drawingtypeHandle ;
        GLuint alphaHandle ;
        GLuint colorHandle ;

        GLuint texture[4];
        GLuint textureID[4];

        GLuint vertexbuffer[10];
        GLuint coordtexbuffer;

        GLuint program;

        GLuint rotation_uniform;
        GLuint pos;
        GLuint col;
    } gl;
    struct {
        GLuint FontTexture;
        GLuint FontTextureID;
        GLuint vertexBufferID;
        GLuint uvBufferID;

        GLuint colorModifier;

        GLuint program;
    } font;

    struct wl_egl_window *native;
    struct wl_surface *surface;
    struct wl_shell_surface *shell_surface;
    EGLSurface egl_surface;
    struct wl_callback *callback;
    int fullscreen, configured, opaque;
};

struct viewport{
    GLuint x;
    GLuint y;
    GLuint width;
    GLuint height;
};

void nc_wayland_display_init(display *pdisplay, void * redraw);
void nc_wayland_display_draw(window *pdisplay, void * redraw);
int nc_wayland_display_destroy(display *pdisplay);

#endif
