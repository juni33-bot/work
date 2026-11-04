#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <stdlib.h>

#include "v4l2_interface.h"
#include "nc_utils.h"

#define ALIGN_16B(x) (((x) + (15)) & ~(15))


#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"

void LOG_(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[app] ");
    vprintf(format, args);
    printf(RESET "\n");
    va_end(args);
}

void LOG_INFO(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf(GREEN "[app] ");
    vprintf(format, args);
    printf(RESET "\n");
    va_end(args);
}

void LOG_WARN(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf(YELLOW "[app] ");
    vprintf(format, args);
    printf(RESET "\n");
    va_end(args);
}

void LOG_ERR(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf(RED "[app] ");
    vprintf(format, args);
    printf(RESET "\n");
    va_end(args);
}

struct image_size_info {
    int width;
    int height;
};

struct v4l2_app_info {
    //Caputre
    struct image_size_info  capture;

    //VDMA
    struct image_size_info  vdma;
    int                     vdma_mode;

};

static v4l2_app_info app_info;



static void nc_v4l2_get_capture_size(int *width, int *height)
{
    *width = app_info.capture.width;
    *height = app_info.capture.height;
}

static void nc_v4l2_get_ds_size(int *width, int *height, int *dma_mode)
{
    *width = app_info.vdma.width;
    *height = app_info.vdma.height;
    *dma_mode = app_info.vdma_mode;
}

int nc_v4l2_open(int idx, bool isblock)
{
    char dev_name[32];
    int fd;

    sprintf(dev_name, "/dev/video%d", idx);
    if (isblock) {
        if ((fd = open(dev_name, O_RDWR|O_CLOEXEC, 0)) < 0) {
            return -errno;
        }
    } else {
        if ((fd = open(dev_name, O_RDWR|O_NONBLOCK|O_CLOEXEC, 0)) < 0) {
            return -errno;
        }
    }

    return fd;
}

int nc_v4l2_close(int fd)
{
    return close(fd);
}

int nc_v4l2_set_input(int fd, unsigned int index, int type)
{
    struct v4l2_input input;

    input.index = index;
    input.type = type;

    /* set input index */
    if (ioctl(fd, VIDIOC_S_INPUT, &input) < 0) {
        LOG_ERR("fail(VIDIOC_S_INPUT)");
        return -1;
    }

    return 0;
}

int nc_v4l2_get_capability(int fd, struct v4l2_capability *cap)
{
    if (!cap) {
        return -1;
    }

    return ioctl(fd, VIDIOC_QUERYCAP, cap);
}

void nc_v4l2_print_capability(const struct v4l2_capability *cap)
{
    if (cap->capabilities & V4L2_CAP_VIDEO_CAPTURE) {
        LOG_INFO("capture");
    }
    if (cap->capabilities & V4L2_CAP_STREAMING) {
        LOG_INFO("streaming");
    }
}

int nc_v4l2_set_fps(int fd, unsigned int fps)
{
    struct v4l2_streamparm params;

    memset(&params, 0, sizeof(params));
    params.parm.capture.timeperframe.numerator = 1;
    params.parm.capture.timeperframe.denominator = fps;
    params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    return ioctl(fd, VIDIOC_S_PARM, &params);
}

int nc_v4l2_get_frame_size(int fd, unsigned int pixelformat)
{
    struct v4l2_frmsizeenum framesize;
    int ret;

    memset(&framesize, 0x00, sizeof(framesize));

    framesize.index = 0;
    framesize.pixel_format = pixelformat;

    ret = ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &framesize);
    if (ret < 0) {
        LOG_ERR("ioctl fail(VIDIOC_ENUM_FRAMESIZES)");
        return ret;
    }

    switch(framesize.type) {
        case V4L2_FRMSIZE_TYPE_DISCRETE:
            // printf("[Frame Info] Size : %dx%d\n", framesize.discrete.width, framesize.discrete.height);
            break;
        case V4L2_FRMSIZE_TYPE_CONTINUOUS:
        case V4L2_FRMSIZE_TYPE_STEPWISE:
            // printf("[Frame Info] Size: %dx%d - %dx%d (stpe w/h(%d/%d))\n",
            //                         framesize.stepwise.min_width,
            //                         framesize.stepwise.min_height,
            //                         framesize.stepwise.max_width,
            //                         framesize.stepwise.max_height,
            //                         framesize.stepwise.step_width,
            //                         framesize.stepwise.step_height);
            break;
    }

    return 0;
}

int nc_v4l2_set_format(int fd, unsigned int width, unsigned int height, unsigned int pixelformat)
{
    struct v4l2_format fmt;
    int ret;
    // char format[10] = {0,};

    memset(&fmt, 0, sizeof(v4l2_format));

    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = width;
    fmt.fmt.pix.height      = height;
    fmt.fmt.pix.pixelformat = pixelformat;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
    fmt.fmt.pix.colorspace  = V4L2_COLORSPACE_SRGB;
    LOG_INFO("Driver Info");
    LOG_INFO("video_width x video_height : %d x %d", width, height);

    ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
    if (ret < 0) {
        LOG_ERR("ioctl fail(VIDIOC_S_FMT)");
        return ret;
    }

    ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
    if (ret < 0) {
        LOG_ERR("ioctl fail(VIDIOC_G_FMT)");
        return ret;
    }

    app_info.capture.width = width;
    app_info.capture.height = height;

    // format[0] = (char)(fmt.fmt.pix.pixelformat&0x000000FF);
    // format[1] = (char)((fmt.fmt.pix.pixelformat&0x0000FF00) >> 8);
    // format[2] = (char)((fmt.fmt.pix.pixelformat&0x00FF0000) >> 16);
    // format[3] = (char)((fmt.fmt.pix.pixelformat&0xFF000000) >> 24);

    // LOG_INFO("type: %d", fmt.type);
    // LOG_INFO("width: %d, height: %d", fmt.fmt.pix.width, fmt.fmt.pix.height);
    // LOG_INFO("pixelformat: %s, field: %d, colorspace: %d",
    //                 format, fmt.fmt.pix.field, fmt.fmt.pix.colorspace);
    return 0;
}


/*
 *  The operation must be performed in the order of `VIDIOC_S_FMT -> VIDIOC_S_SELECTION`.
 */
int nc_v4l2_set_vdma_cnn_crop_ds(int fd, char dma_mode, unsigned int img_process,
                                unsigned int pos_x, unsigned int pos_y,
                                unsigned int crop_width, unsigned int crop_height,
                                unsigned int ds_width, unsigned int ds_height)
{
    struct v4l2_selection img;
    int idx = 0, mode = 0;
    unsigned int cap_width = 0, cap_height = 0;
    float calculate_width = 0.0, calculate_height = 0.0;
    int ret = 0;
    struct v4l2_ext_controls user_ext_controls;
    struct v4l2_ext_control user_control;
    char user_env[USER_BUF_VDMA] = {0,};

    memset(&img, 0, sizeof(v4l2_selection));

    nc_v4l2_get_capture_size((int*)&cap_width, (int*)&cap_height);

    mode = img_process;

    // Verification
    switch(mode) {
        case MODE_CROP:
            if ((pos_x + crop_width) > cap_width) {
                LOG_ERR("The operating condition is not satisfied.");
                LOG_ERR(" - Width : %d > %d", (pos_x + crop_width), cap_width);
                return -1;
            }

            if ((pos_y + crop_height) > cap_height) {
                LOG_ERR("The operating condition is not satisfied.");
                LOG_ERR(" - Height : %d > %d", (pos_y + crop_height), cap_height);
                return -1;
            }
            break;
        case MODE_DS:

            calculate_width = (float)(cap_width / ds_width);
            calculate_height = (float)(cap_height / ds_height);

            if ((calculate_width > 4.0) || (calculate_height > 4.0)) {
                LOG_ERR("It is an impossible ratio.");
                return -1;
            }
            break;
        case MODE_CROP_TO_DS:
            //Crop Verification
            if ((pos_x + crop_width) > cap_width) {
                LOG_ERR("The operating condition is not satisfied.");
                LOG_ERR(" - Width : %d > %d", (pos_x + crop_width), cap_width);
                return -1;
            }

            if ((pos_y + crop_height) > cap_height) {
                LOG_ERR("The operating condition is not satisfied.");
                LOG_ERR(" - Height : %d > %d", (pos_y + crop_height), cap_height);
                return -1;
            }

            //DS Verification
            calculate_width = (float)(crop_width / ds_width);
            calculate_height = (float)(crop_height / ds_height);

            if ((calculate_width > 4.0) || (calculate_height > 4.0)) {
                LOG_ERR("It is an impossible ratio.");
                return -1;
            }
            break;
        case MODE_BYPASS:
            ds_width  = cap_width;
            ds_height = cap_height;
            break;
        default:
            return -1;
            break;
    }

    // Env Set.
    user_env[idx] = CNN_MODE;

    idx = 2;
    if (mode == MODE_CROP) {
        user_env[idx++] = ((crop_width & 0xFF00) >> 8);
        user_env[idx++] = (crop_width & 0x00FF);

        user_env[idx++] = ((crop_height & 0xFF00) >> 8);
        user_env[idx++] = (crop_height & 0x00FF);
    } else { // MODE_DS, MODE_CROP_TO_DS, MODE_BYPASS
        user_env[idx++] = ((ds_width & 0xFF00) >> 8);
        user_env[idx++] = (ds_width & 0x00FF);

        user_env[idx++] = ((ds_height & 0xFF00) >> 8);
        user_env[idx++] = (ds_height & 0x00FF);
    }

    user_env[idx++] = 0; // offset
    user_env[idx++] = dma_mode; // DMA Mode = 0:planar 1:Interleave(packed)
    user_env[idx++] = 0; // Interleave Order = 0:RGB 1: RBG 2: GRB 3:GBR 4:BRG 5:BGR 6/7: RGB
    user_env[idx++] = 0; // Stride_ch0_en
    user_env[idx++] = 1; // Stride_ch1_en
    user_env[idx++] = 1; // Stride_ch2_en
    user_env[idx++] = 0; // TP gen

    user_env[1] = (char)idx;

    user_control.id = V4L2_CTRL_CLASS_USER;
    user_control.ptr = &user_env;

    user_ext_controls.ctrl_class = V4L2_CTRL_ID2CLASS(user_control.id);
    user_ext_controls.count = 1;
    user_ext_controls.controls = &user_control;

    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &user_ext_controls) < 0) {
        LOG_ERR("fail(VIDIOC_S_EXT_CTRLS)");
        return -errno;
    }

    //Mode Set.
    switch(mode) {
        case MODE_CROP:
            img.r.top = pos_y;
            img.r.left = pos_x;
            img.r.width = crop_width << 16 | crop_width;
            img.r.height = crop_height << 16 | crop_height;
            img.flags = V4L2_SEL_FLAG_LE;
            img.target = V4L2_SEL_TGT_CROP_BOUNDS;
            break;

        case MODE_CROP_TO_DS:
            img.r.top = pos_y;
            img.r.left = pos_x;
            img.r.width = ds_width << 16 | crop_width;
            img.r.height = ds_height << 16 | crop_height;
            img.flags = V4L2_SEL_FLAG_LE;
            img.target = V4L2_SEL_TGT_CROP_BOUNDS;
            break;

        case MODE_DS:
        case MODE_BYPASS:
            img.r.top = 0;
            img.r.left = 0;
            img.r.width = ds_width << 16 | cap_width;
            img.r.height = ds_height << 16 | cap_height;
            img.target = V4L2_SEL_TGT_COMPOSE;
            img.flags = V4L2_SEL_FLAG_LE;
            break;

        default:
            break;
    }

    ret = ioctl(fd, VIDIOC_S_SELECTION, &img);

    app_info.vdma.width = ds_width;
    app_info.vdma.height = ds_height;
    app_info.vdma_mode = (user_env[7] == 0) ? 2 : 3;

    return ret;
}

/*
 *  The operation must be performed in the order of `VIDIOC_S_FMT -> VIDIOC_S_SELECTION`.
 */

int nc_v4l2_set_vdma_coda_crop_ds(int fd, unsigned int img_process,
                                    unsigned int pos_x, unsigned int pos_y,
                                    unsigned int crop_width, unsigned int crop_height,
                                    unsigned int ds_width, unsigned int ds_height)
{
    struct v4l2_selection img;
    int idx = 0, mode = 0;
    unsigned int cap_width = 0, cap_height = 0;
    float calculate_width = 0.0, calculate_height = 0.0;
    int ret = 0;
    struct v4l2_ext_controls user_ext_controls;
    struct v4l2_ext_control user_control;
    char user_env[USER_BUF_VDMA] = {0,};

    memset(&img, 0, sizeof(v4l2_selection));

    nc_v4l2_get_capture_size((int*)&cap_width, (int*)&cap_height);

    mode = img_process;

    // Verification
    switch(mode) {
        case MODE_CROP:
            if ((pos_x + crop_width) > cap_width) {
                LOG_ERR("The operating condition is not satisfied.");
                LOG_ERR(" - Width : %d > %d", (pos_x + crop_width), cap_width);
                return -1;
            }

            if ((pos_y + crop_height) > cap_height) {
                LOG_ERR("The operating condition is not satisfied.");
                LOG_ERR(" - Height : %d > %d", (pos_y + crop_height), cap_height);
                return -1;
            }
            break;
        case MODE_DS:
            calculate_width = (float)(cap_width / ds_width);
            calculate_height = (float)(cap_height / ds_height);

            if ((calculate_width > 4.0) || (calculate_height > 4.0)) {
                LOG_ERR("It is an impossible ratio.");
                return -1;
            }
            break;
        case MODE_CROP_TO_DS:
            //Crop Verification
            if ((pos_x + crop_width) > cap_width) {
                LOG_ERR("The operating condition is not satisfied.");
                LOG_ERR(" - Width : %d > %d", (pos_x + crop_width), cap_width);
                return -1;
            }

            if ((pos_y + crop_height) > cap_height) {
                LOG_ERR("The operating condition is not satisfied.");
                LOG_ERR(" - Height : %d > %d", (pos_y + crop_height), cap_height);
                return -1;
            }

            //DS Verification
            calculate_width = (float)(crop_width / ds_width);
            calculate_height = (float)(crop_height / ds_height);

            if ((calculate_width > 4.0) || (calculate_height > 4.0)) {
                LOG_ERR("It is an impossible ratio.");
                return -1;
            }
            break;
        case MODE_BYPASS:
            ds_width  = cap_width;
            ds_height = cap_height;
            break;
        default:
            return -1;
            break;
    }

    // Env Set.
    user_env[idx] = CODA_MODE;

    idx = 2;
    if (mode == MODE_CROP) {
        user_env[idx++] = ((crop_width & 0xFF00) >> 8);
        user_env[idx++] = (crop_width & 0x00FF);

        user_env[idx++] = ((crop_height & 0xFF00) >> 8);
        user_env[idx++] = (crop_height & 0x00FF);
    } else { // MODE_DS, MODE_CROP_TO_DS, MODE_BYPASS
        user_env[idx++] = ((ds_width & 0xFF00) >> 8);
        user_env[idx++] = (ds_width & 0x00FF);

        user_env[idx++] = ((ds_height & 0xFF00) >> 8);
        user_env[idx++] = (ds_height & 0x00FF);
    }

    user_env[idx++] = 0; // offset
    user_env[idx++] = 0;

    user_env[1] = (char)idx;

    user_control.id = V4L2_CTRL_CLASS_USER;
    user_control.ptr = &user_env;

    user_ext_controls.ctrl_class = V4L2_CTRL_ID2CLASS(user_control.id);
    user_ext_controls.count = 1;
    user_ext_controls.controls = &user_control;

    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &user_ext_controls) < 0) {
        LOG_ERR("fail(VIDIOC_S_EXT_CTRLS)");
        return -errno;
    }

    switch(mode) {
        case MODE_CROP:
            img.r.top = pos_y;
            img.r.left = pos_x;
            img.r.width = crop_width << 16 | crop_width;
            img.r.height = crop_height << 16 | crop_height;
            img.flags = V4L2_SEL_FLAG_LE;
            img.target = V4L2_SEL_TGT_CROP_BOUNDS;
            break;

        case MODE_CROP_TO_DS:
            img.r.top = pos_y;
            img.r.left = pos_x;
            img.r.width = ds_width << 16 | crop_width;
            img.r.height = ds_height << 16 | crop_height;
            img.flags = V4L2_SEL_FLAG_LE;
            img.target = V4L2_SEL_TGT_CROP_BOUNDS;
            break;

        case MODE_DS:
        case MODE_BYPASS:
            img.r.top = 0;
            img.r.left = 0;
            img.r.width = ds_width << 16 | cap_width;
            img.r.height = ds_height << 16 | cap_height;
            img.target = V4L2_SEL_TGT_COMPOSE;
            img.flags = V4L2_SEL_FLAG_LE;
            break;

        default:
            break;
    }

    ret = ioctl(fd, VIDIOC_S_SELECTION, &img);

    app_info.vdma.width = ds_width;
    app_info.vdma.height = ds_height;
    app_info.vdma_mode = 2;

    return ret;
}

int nc_v4l2_get_ctrl(int fd, unsigned int id, int *value)
{
    struct v4l2_control ctrl;

    ctrl.id = id;
    ctrl.value = 0;

    if (!value)
        return -EINVAL;

    if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) < 0) {
        return -errno;
    }

    *value = ctrl.value;

    return 0;
}

int nc_v4l2_set_ctrl(int fd, unsigned int id, int *value)
{
    struct v4l2_control ctrl;

    ctrl.id = id;

    if (!value)
        return -EINVAL;

    ctrl.value = *value;

    if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0) {
        return -errno;
    }

    *value = ctrl.value;

    return 0;
}

int nc_v4l2_alloc_mapped(int fd, unsigned int count, struct vbuffer *buffers)
{
    struct v4l2_requestbuffers request;
    __u32 i;
    int ret;

    request.count  = count;
    request.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    request.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(fd, VIDIOC_REQBUFS, &request);
    if (ret < 0) {
        LOG_ERR("VIDIOC_REQBUFS error");
        return ret;
    }

    struct v4l2_buffer buf;
    for (i = 0; i < request.count; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            LOG_ERR("VIDIOC_QUERYBUF error");
            goto free_buffers;
        }

        buffers[i].length = buf.length;
        buffers[i].start =
            mmap(NULL /* start anywhere */ ,
                buf.length,
                PROT_READ | PROT_WRITE /* required */ ,
                MAP_SHARED /* recommended */ ,
                fd, buf.m.offset);

        if (MAP_FAILED == buffers[i].start) {
            LOG_ERR("unable to map buffer %u (%d)", i, errno);
            goto free_buffers;
        }
        // Clear buffer
        memset(buffers[i].start, 0, buffers[i].length);
    }

    return request.count;

free_buffers:
    nc_v4l2_free_mapped(fd, i + 1, buffers);
    return -1;
}

int nc_v4l2_free_mapped(int fd, size_t count, struct vbuffer *buffers)
{
    struct v4l2_requestbuffers request;
    __u32 i;

    for (i = 0; i < count; i++) {
        if (buffers[i].start == NULL)
            continue;

        if (munmap(buffers[i].start, buffers[i].length)) {
            LOG_ERR("unable to unmap buffer %u (%d)", i, errno);
            return -errno;
        }
        buffers[i].start = NULL;
        buffers[i].length = 0;
    }

    request.count  = 0;
    request.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    request.memory = V4L2_MEMORY_MMAP;

    return ioctl(fd, VIDIOC_REQBUFS, &request);
}

int nc_v4l2_query_buffer(int fd, size_t index, struct v4l2_buffer *buf)
{
    buf->type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf->memory = V4L2_MEMORY_MMAP;
    buf->index  = (__u32)index;

    return ioctl(fd, VIDIOC_QUERYBUF, buf);
}

int nc_v4l2_queue_buffer(int fd, size_t index)
{
    struct v4l2_buffer buf;

    memset(&buf, 0 , sizeof(v4l2_buffer));

    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index  = (uint32_t)index;

    return ioctl(fd, VIDIOC_QBUF, &buf);
}

int nc_v4l2_dequeue_buffer(int fd, struct v4l2_buffer *buf)
{
    int res = 0;

    memset(buf, 0, sizeof(struct v4l2_buffer));

    buf->type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf->memory = V4L2_MEMORY_MMAP;

    res = ioctl(fd, VIDIOC_DQBUF, buf);
    return res;
}

int nc_v4l2_start_capture(int fd)
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    return ioctl(fd, VIDIOC_STREAMON, &type);
}

int nc_v4l2_stop_capture(int fd)
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    return ioctl(fd, VIDIOC_STREAMOFF, &type);
}


void nc_vdma_memory_dump_image(int ch, const void *p)
{
    char FileName[20] = {0,};
    static int FrameCnt = 0;
    int width = 0, height = 0, vdma_mode = 0;

    nc_v4l2_get_ds_size(&width, &height, &vdma_mode);

    if (snprintf(FileName, 256, "./Ch%02d_frame_size_%dx%d_%04d.raw", ch, width, height, FrameCnt) >= 256) {
        fprintf(stderr, "Buffer size is not sufficient for FileName\n");
    }

    FrameCnt++;

    FILE *fp=fopen(FileName, "wb");
    fwrite(p, (width * height * vdma_mode), 1, fp);

    fflush(fp);
    fclose(fp);
}

int nc_v4l2_get_format(int fd, int *width, int *height)
{
    struct v4l2_format format;

    memset(&format, 0, sizeof(v4l2_format));

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(fd, VIDIOC_G_FMT, &format) == -1) {
        perror("Failed to get format");
        close(fd);
        return 1;
    }

    *width = format.fmt.pix.width;
    *height = format.fmt.pix.height;

    return 0;
}

int nc_v4l2_get_data_idx(st_nc_v4l2_config *s_nc_config)
{
    s_nc_config->data_idx = GET_DATA_IDX(s_nc_config->video_buf.video_device_num);
    if(s_nc_config->data_idx < 0) {
        printf("GET_DATA_IDX() failure!\n");
        return -1;
    }

    return 0;
}

int nc_v4l2_init_device_and_stream_on(st_nc_v4l2_config *s_nc_config, int video_buffer_num)
{
    int ret;
    int plane_cnt;
    int sensor_width = 0;
    int sensor_height = 0;
    int i = 0;

    ret = nc_v4l2_get_format(s_nc_config->video_buf.video_fd , &sensor_width , &sensor_height);
    if (ret < 0) {
        printf("nc_v4l2_get_format() failure!\n");
        return -1;
    }
    // printf("[app] : nc_v4l2_get_format (width:%d height:%d)\n", sensor_width, sensor_height);

    s_nc_config->sensor_width = sensor_width;
    s_nc_config->sensor_height = sensor_height;

    ret = nc_v4l2_set_input(s_nc_config->video_buf.video_fd, 0, 2);
    if (ret < 0) {
        printf("nc_v4l2_set_input() failure!\n");
        return -1;
    }

    // printf("[app] : nc_v4l2_set_format (width:%d height:%d)\n", sensor_width, sensor_height);
    plane_cnt = nc_v4l2_set_format(s_nc_config->video_buf.video_fd, sensor_width, sensor_height, s_nc_config->pixformat);
    if (plane_cnt < 0) {
        printf("nc_v4l2_set_format() failure!\n");
        return -1;
    }

    ret = nc_v4l2_get_data_idx(s_nc_config);
    if (ret < 0) {
        printf("nc_v4l2_get_data_idx() failure!\n");
        return -1;
    }

    if(s_nc_config->data_idx == CNN_MODE) {
        ret = nc_v4l2_set_vdma_cnn_crop_ds(s_nc_config->video_buf.video_fd,
                                        s_nc_config->dma_mode,
                                        s_nc_config->img_process,
                                        s_nc_config->crop_x_start,
                                        s_nc_config->crop_y_start,
                                        s_nc_config->crop_width,
                                        s_nc_config->crop_height,
                                        s_nc_config->ds_width,
                                        s_nc_config->ds_height);

        if (ret < 0) {
            printf("nc_v4l2_set_vdma_cnn_crop_ds() failure!\n");
            return -1;
        }
    } else if (s_nc_config->data_idx == CODA_MODE) {
        ret = nc_v4l2_set_vdma_coda_crop_ds(s_nc_config->video_buf.video_fd,
                                         s_nc_config->img_process,
                                         s_nc_config->crop_x_start,
                                         s_nc_config->crop_y_start,
                                         s_nc_config->crop_width,
                                         s_nc_config->crop_height,
                                         s_nc_config->ds_width,
                                         s_nc_config->ds_height);

        if (ret < 0) {
            printf("nc_v4l2_set_vdma_coda_crop_ds() failure!\n");
            return -1;
        }
    } else if (s_nc_config->data_idx == OFRC_MODE) {
        /* TBD */
    } else if (s_nc_config->data_idx == CSI_MODE) {
        /* TBD */
    } else {
        printf("Error invalid data_idx\n");
        return -1;
    }

    if(video_buffer_num <= 0) {
        video_buffer_num = DEFAULT_BUFFER_NUM;
        printf("Set the video buffer num to the default value(%d)\n", video_buffer_num);
    }

    s_nc_config->video_buf.buffers = (vbuffer*)calloc(video_buffer_num, sizeof(*s_nc_config->video_buf.buffers));
    if(!s_nc_config->video_buf.buffers) {
        printf("calloc() failure\n");
        return -1;
    }

    ret = nc_v4l2_alloc_mapped(s_nc_config->video_buf.video_fd, video_buffer_num, s_nc_config->video_buf.buffers);
    if(ret < 0) {
        printf("nc_v4l2_alloc_mapped() failure\n");
        return -1;
    }

    if(s_nc_config->video_buf.video_fd != -1)
    {
        for(i = 0 ; i < video_buffer_num ; ++i) {
            ret = nc_v4l2_queue_buffer(s_nc_config->video_buf.video_fd, i);
            if (ret < 0) {
                printf("nc_v4l2_queue_buffer() failure\n");
                return -1;
            }
        }

        ret = nc_v4l2_start_capture(s_nc_config->video_buf.video_fd);
        if (ret < 0) {
            printf("nc_v4l2_start_capture() failure\n");
            return -1;
        }
    }
    return ret;
}

void nc_v4l2_show_user_config(st_nc_v4l2_config *s_nc_config, int video_max)
{
    int i = 0, j = 0, k = 0, l = 0, p = 0, r = 0;

    char sprintf_buffer[20] = {0, };
    char fmt_format[5] = {0, };
    char field_name[10][20] = {
        {"data_idx"},
        {"dma_mode"},
        {"img_process"},
        {"pixformat"},
        {"crop_x_start"},
        {"crop_y_start"},
        {"crop_width"},
        {"crop_height"},
        {"ds_width"},
        {"ds_height"},
    };

    char data_idx_string[4][5] = {
        {"OFRC"},
        {"CNN "},
        {"CODA"},
        {"CSI "}
    };

    char dma_mode_string[2][15] ={
        {"PLANAR    "},
        {"INTERLEAVE"},
    };

    char img_process_string[4][15] = {
        {"BYPASS    "},
        {"CROP      "},
        {"DS        "},
        {"CROP_TO_DS"},
    };

    printf("\n USER CONFIG");

    for(k = 0 ; k < (video_max + 1) ; k++) {
        if (k == 0) {
            printf("\n");
        }
        printf("----------------");
    }

    for(r = 0 ; r < (video_max + 1) ; r++) {
        if (r == 0) {
            printf("\n Channel\t");
        } else {
            printf("\tvideo%d\t",s_nc_config[r-1].video_buf.video_device_num);
        }
    }

    for(p = 0 ; p < (video_max + 1) ; p++) {
        if ((p == 0)){
            printf("\n");
        }
        printf("----------------");
    }

    printf("\n");

    for(i = 0 ; i < 10 ; i++)
    {
        printf(" %s\t" , field_name[i]);

        for(j = 0 ; j < video_max ; j++)
        {
            memset(sprintf_buffer, 0, sizeof(sprintf_buffer));
            switch(i)
            {
            case 0:
                sprintf(sprintf_buffer, "\t%s\t", data_idx_string[(s_nc_config[j].data_idx)]);
                printf("%s", sprintf_buffer);
                break;
            case 1:
                if(s_nc_config[j].data_idx == CNN_MODE) {
                    sprintf(sprintf_buffer, "\t%s", dma_mode_string[(unsigned char)s_nc_config[j].dma_mode]);
                } else {
                    sprintf(sprintf_buffer, "\t%s\t", "-");
                }
                printf("%s", sprintf_buffer);
                break;
            case 2:
                sprintf(sprintf_buffer, "\t%s", img_process_string[s_nc_config[j].img_process]);
                printf("%s", sprintf_buffer);
                break;
            case 3:
                fmt_format[0] = (char)(s_nc_config[j].pixformat&0x000000FF);
                fmt_format[1] = (char)((s_nc_config[j].pixformat&0x0000FF00) >> 8);
                fmt_format[2] = (char)((s_nc_config[j].pixformat&0x00FF0000) >> 16);
                fmt_format[3] = (char)((s_nc_config[j].pixformat&0xFF000000) >> 24);
                fmt_format[4] = '\0';

                sprintf(sprintf_buffer, "\t%s\t", fmt_format);
                printf("%s", sprintf_buffer);
                break;
            case 4:
                if(s_nc_config[j].img_process == MODE_DS || s_nc_config[j].img_process == MODE_BYPASS) {
                    sprintf(sprintf_buffer, "\t%s\t", "-");
                } else {
                    sprintf(sprintf_buffer, "\t%d\t", s_nc_config[j].crop_x_start);
                }
                printf("%s", sprintf_buffer);
                break;
            case 5:
                if(s_nc_config[j].img_process == MODE_DS || s_nc_config[j].img_process == MODE_BYPASS) {
                    sprintf(sprintf_buffer, "\t%s\t", "-");
                } else {
                    sprintf(sprintf_buffer, "\t%d\t", s_nc_config[j].crop_y_start);
                }
                printf("%s", sprintf_buffer);
                break;
            case 6:
                if(s_nc_config[j].img_process == MODE_DS || s_nc_config[j].img_process == MODE_BYPASS) {
                    sprintf(sprintf_buffer, "\t%s\t", "-");
                } else {
                    sprintf(sprintf_buffer, "\t%d\t", s_nc_config[j].crop_width);
                }
                printf("%s", sprintf_buffer);
                break;
            case 7:
                if(s_nc_config[j].img_process == MODE_DS || s_nc_config[j].img_process == MODE_BYPASS) {
                    sprintf(sprintf_buffer, "\t%s\t", "-");
                } else {
                    sprintf(sprintf_buffer, "\t%d\t", s_nc_config[j].crop_height);
                }
                printf("%s", sprintf_buffer);
                break;
            case 8:
                if(s_nc_config[j].img_process == MODE_CROP || s_nc_config[j].img_process == MODE_BYPASS) {
                    sprintf(sprintf_buffer, "\t%s\t", "-");
                } else {
                    sprintf(sprintf_buffer, "\t%d\t", s_nc_config[j].ds_width);
                }
                printf("%s", sprintf_buffer);
                break;
            case 9:
                if(s_nc_config[j].img_process == MODE_CROP || s_nc_config[j].img_process == MODE_BYPASS) {
                    sprintf(sprintf_buffer, "\t%s\t", "-");
                } else {
                    sprintf(sprintf_buffer, "\t%d\t", s_nc_config[j].ds_height);
                }
                printf("%s", sprintf_buffer);break;
            default:
                break;
            }
        }
        printf("\n" );
    }

    for(l = 0 ; l < (video_max + 1) ; l++) {
        printf("----------------");
    }

    printf("\n");
}

int nc_v4l2_isp_ctrl(int fd, unsigned char func_num, unsigned char cfg_value)
{
    struct v4l2_ext_controls user_ext_controls;
    struct v4l2_ext_control user_control;
    unsigned char user_env[DEFAULT_BUFFER_NUM] = {0,};
    int idx = 0;

    user_env[idx++] = 0x81;
    user_env[idx++] = func_num;
    user_env[idx++] = cfg_value;

    user_control.id = V4L2_CTRL_CLASS_USER;
    user_control.ptr = &user_env;

    user_ext_controls.ctrl_class = V4L2_CTRL_ID2CLASS(user_control.id);
    user_ext_controls.count = 1;
    user_ext_controls.controls = &user_control;

    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &user_ext_controls) < 0) {
        LOG_ERR("fail (VIDIOC_S_EXT_CTRLS)\n");
        return -errno;
    }

    return 0;
}