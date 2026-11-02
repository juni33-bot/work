#ifndef __V4L2_INTERFACE_H__
#define __V4L2_INTERFACE_H__

#include <stdbool.h>
#include <linux/videodev2.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLEAR(x)            memset(&(x), 0, sizeof(x))
#define USER_BUF_VDMA       (50)
#define DEFAULT_BUFFER_NUM  (3) // number of capture buffers

/*
 * The Video Path index is as follows.
 *   OFRC:  V0[0 ~ 9]
 *          V1[50 ~ 59]
 *   CNN:   V0[10 ~ 19]
 *          V1[60 ~ 69]
 *   CODA:  V0[20 ~ 29]
 *          V1[70 ~ 79]
 *   CSI:   V0[30 ~ 39]
 *          V1[80 ~ 89]
 *  Reserved: V0[40 ~ 49]
 *            V1[90 ~ 99]
 */
#define VISION1_DEVICE_START_OFFSET   50
#define OFRC_DEVICE_NUM(x)            ((x) == 0 ?  0 :      VISION1_DEVICE_START_OFFSET)
#define CNN_DEVICE_NUM(x)             ((x) == 0 ? 10 : 10 + VISION1_DEVICE_START_OFFSET)
#define CODA_DEVICE_NUM(x)            ((x) == 0 ? 20 : 20 + VISION1_DEVICE_START_OFFSET)
#define CSI_DEVICE_NUM(x)             ((x) == 0 ? 30 : 30 + VISION1_DEVICE_START_OFFSET)
#define RESERVED_DEVICE_NUM(x)        ((x) == 0 ? 40 : 40 + VISION1_DEVICE_START_OFFSET)

#define GET_DATA_IDX(x) \
    (((x) >= OFRC_DEVICE_NUM(VISION0) && (x) < CNN_DEVICE_NUM(VISION0)) || ((x) >= OFRC_DEVICE_NUM(VISION1) && (x) < CNN_DEVICE_NUM(VISION1)) ? OFRC_MODE : \
     ((x) >= CNN_DEVICE_NUM(VISION0) && (x) < CODA_DEVICE_NUM(VISION0)) || ((x) >= CNN_DEVICE_NUM(VISION1) && (x) < CODA_DEVICE_NUM(VISION1)) ? CNN_MODE : \
     ((x) >= CODA_DEVICE_NUM(VISION0) && (x) < CSI_DEVICE_NUM(VISION0)) || ((x) >= CODA_DEVICE_NUM(VISION1) && (x) < CSI_DEVICE_NUM(VISION1)) ? CODA_MODE : \
     ((x) >= CSI_DEVICE_NUM(VISION0) && (x) < RESERVED_DEVICE_NUM(VISION0)) || ((x) >= CSI_DEVICE_NUM(VISION1) && (x) < RESERVED_DEVICE_NUM(VISION1)) ? CSI_MODE : -1)

typedef enum {
    VISION0 = 0,
    VISION1,
} E_VISION_NUM;

/*
*  Not Support : MODE_DS_TO_CROP
*/
typedef enum {
    MODE_BYPASS = 0,
    MODE_CROP,
    MODE_DS,
    MODE_CROP_TO_DS
} E_IMG_PROCESS;

typedef enum {
    OFRC_MODE = 0,
    CNN_MODE,
    CODA_MODE,
    CSI_MODE /* BAYER or YUV */
} E_RAW_DATA_IDX;

typedef enum {
    PLANAR = 0,
    INTERLEAVE,
} E_CNN_DMA_MODE;

typedef enum {
    ISP_LDC_CTRL = 0,
    ISP_AE_CTRL,
    ISP_AWB_CTRL,
    ISP_TUNE_CTRL
} E_ISP_CTRL;

struct video_buffer {
    void *start;
    size_t length;
};

struct vbuffer {
    void   *start;
    size_t  length;
    int filled;  // 1: filled with camera data, 0:camera data has been glTexDirectMapped
    int invalid; // 1: already swapped, 0:not swappedto texture
};

struct vd_buffer{
    int     video_fd;
    vbuffer *buffers;
    int fill_buffer_inx; // 1: filled with camera data, 0: not filled
    int video_device_num;
};

typedef struct {
    struct vd_buffer video_buf;
    int data_idx;   // ofrc, cnn, coda, yuv
    char dma_mode;   // planar. interleave
    unsigned int img_process;    // crop, ds, cropds
    unsigned int pixformat;
    unsigned int crop_x_start;
    unsigned int crop_y_start;
    unsigned int crop_width;
    unsigned int crop_height;
    unsigned int ds_width;
    unsigned int ds_height;
    unsigned int sensor_width;
    unsigned int sensor_height;
} st_nc_v4l2_config;

int nc_v4l2_set_vdma_cnn_crop_ds(int fd, char dma_mode, unsigned int img_process,
                                unsigned int pos_x, unsigned int pos_y,
                                unsigned int crop_width, unsigned int crop_height,
                                unsigned int ds_width, unsigned int ds_height);

int nc_v4l2_set_vdma_coda_crop_ds(int fd, unsigned int img_process,
                                unsigned int pos_x, unsigned int pos_y,
                                unsigned int crop_width, unsigned int crop_height,
                                unsigned int ds_width, unsigned int ds_height);
int nc_v4l2_open(int idx, bool isblock);
int nc_v4l2_close(int fd);
int nc_v4l2_set_input(int fd, unsigned int index, int type);
int nc_v4l2_get_capability(int fd, struct v4l2_capability *cap);
void nc_v4l2_print_capability(const struct v4l2_capability *cap);
int nc_v4l2_set_fps(int fd, unsigned int fps);
int nc_v4l2_set_format(int fd, unsigned int width, unsigned int height, unsigned int pixelformat);
int nc_v4l2_get_ctrl(int fd, int id, int *value);
int nc_v4l2_set_ctrl(int fd, unsigned int id, int *value);
int nc_v4l2_alloc_mapped(int fd, unsigned int count, struct vbuffer *buffers);
int nc_v4l2_free_mapped(int fd, size_t count, struct vbuffer *buffers);
int nc_v4l2_query_buffer(int fd, size_t index, struct v4l2_buffer *buf);
int nc_v4l2_queue_buffer(int fd, size_t index);
int nc_v4l2_dequeue_buffer(int fd, struct v4l2_buffer *buf);
int nc_v4l2_start_capture(int fd);
int nc_v4l2_stop_capture(int fd);
void nc_vdma_memory_dump_image(int ch, const void *p);
int nc_v4l2_get_format(int fd, int *width, int *height);
int nc_v4l2_get_frame_size(int fd, unsigned int pixelformat);
int nc_v4l2_init_device_and_stream_on(st_nc_v4l2_config *s_nc_config, int video_buffer_num);
void nc_v4l2_show_user_config(st_nc_v4l2_config *s_nc_config, int video_max);
int nc_v4l2_isp_ctrl(int fd, unsigned char func_num, unsigned char cfg_value);
#ifdef __cplusplus
}
#endif
#endif  //__V4L2_INTERFACE_H__
