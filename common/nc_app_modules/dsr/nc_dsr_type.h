#ifndef DSR_TYPE_H
#define DSR_TYPE_H

#include <stdint.h>
#include <stddef.h>
#include "nc_dmabuf_ctrl_helper.h"
/* SysCall Write Command */
#define DSR_SYSCALL_CMD_SET_INPUT               (0x11)
#define DSR_SYSCALL_CMD_SET_OUTPUT              (0x12)
#define DSR_SYSCALL_CMD_GET_INPUT               (0x13)
#define DSR_SYSCALL_CMD_GET_OUTPUT              (0x14)
#define DSR_SYSCALL_CMD_SET_CROP                (0x15)
#define DSR_SYSCALL_CMD_SET_DOWNSACLER          (0x16)
#define DSR_SYSCALL_CMD_GET_CROP                (0x17)
#define DSR_SYSCALL_CMD_GET_DOWNSACLER          (0x18)
#define DSR_SYSCALL_CMD_SET_IN_ADDR             (0x19)
#define DSR_SYSCALL_CMD_SET_OUT_ADDR            (0x1a)
#define DSR_SYSCALL_CMD_SET_ROTATOR             (0x1b)
#define DSR_SYSCALL_CMD_GET_ROTATOR             (0x1c)
#define DSR_SYSCALL_CMD_START                   (0x1d)
#define ROTATOR_SYSCALL_CMD_START               (0x1e)
#define DSR_SYSCALL_CMD_STATE                   (0x1f)
#define ROTATOR_SYSCALL_CMD_STATE               (0x20)
#define DSR_SYSCALL_CMD_DMA_ADDR                (0x21)
#define DSR_SYSCALL_CMD_IN_PHY_ADDR             (0x22)
#define DSR_SYSCALL_CMD_OUT_PHY_ADDR            (0x23)
#define NC_ISP_IOCTL_DSR_BUFFER_SYNC            (0x24)

#define BUFF_NUM 2
#define BUFF_MAX_SIZE 4096*2160*3

#define IMG_FORMAT_RGB888  0
#define IMG_FORMAT_YUV444  0
#define IMG_FORMAT_YUV422  1

typedef struct {
    int dmabuf_fd_in;
    int dmabuf_fd_out;
    size_t buffer_size; 
} dma_alloc_info;
typedef enum {
    DSR_FMT_YUV444    =   0,
    DSR_FMT_RGB888    =   0,
    DSR_FMT_YUV422    =   1,
    DSR_FMT_MAX
}dsr_img_fmt_t;

typedef enum {
    ROT_MODE_00     =   0,
    ROT_MODE_90     =   1,
    ROT_MODE_180    =   2,
    ROT_MODE_270    =   3,
    ROT_MOD_MAX
}dsr_rot_mod_t;

typedef enum {
    ROT_U0_V0   =   0,
    ROT_U0_V1   =   1,
    ROT_U1_V0   =   2,
    ROT_U_V_AVG =   3,
    ROT_CON_MAX
}dsr_ROT_CON_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    int format;
}img_input_config, img_output_config;

typedef struct {
    int crop_enable;
    int crop_x;
    int crop_y;
    int crop_width;
    int crop_height;
    int ds_enable;
    int ds_width;
    int ds_height;
    int flip;
    int in_format;
    int out_format;
    dsr_rot_mod_t rot_mode;
    dsr_ROT_CON_t rot_convert;
    int in_width;
    int in_height;
} st_nc_dsr_config;

typedef struct {
    int in_format;
    int out_format;
    dsr_rot_mod_t rot_mode;
    dsr_ROT_CON_t rot_convert;
    int in_width;
    int in_height;
} rot_config;

typedef struct {
    uint32_t crop_enable;             /* Crop Logic Enable control
                                        0 Bypass
                                        1 Crop the image */
    union {
        uint32_t reg;
        struct {
            uint32_t ver      : 16;     /* Vertical Crop start position in pixel */
            uint32_t hor      : 16;     /* Horizontal Crop start position in pixel */
        }bit;
    }start_pos;

    union {
        uint32_t reg;
        struct {
            uint32_t ver      : 16;     /* Vertical Croped image size in pixel */
            uint32_t hor      : 16;     /* Horizontal Croped image size in pixel */
        }bit;
    }size;
}DSR_Crop_t;

typedef struct {
    union {
        uint32_t reg;
        struct {
            uint32_t ds_enable        : 1;      /* DownScale Logic Enable control
                                                0 Bypass
                                                1 DownScale the image */
            uint32_t reserved2        : 7;
            uint32_t alpha            : 2; /* DownScale alpha value table selection to use in DS algorithm
                                            0 -05
                                            1 -075
                                            2 -10
                                            3 -20 */
            uint32_t reserved1        : 22;
        }bit;
    }config;

    union {
        uint32_t reg;
        struct {
            uint32_t ver          : 16;     /* Vertical DownScaled image size in pixel */
            uint32_t hor          : 16;     /* Horizontal DownScaled image size in pixel */
        }bit;
    }size;
    
    uint32_t flip;
}DSR_Downscale_t;

typedef struct {
    union {
        uint32_t reg;
        struct {
            uint32_t ver_size : 16;     /* Source image vertical size 100  4096 in pixel */
            uint32_t hor_size : 16;     /* Source image horizontal size 100  4096 in pixel */
        }bit;
    }size;

    union {
        uint32_t reg;
        struct {
            uint32_t endian_change    : 1;        /* DownScaler logic process with Little Endian
                                                    If source image frame store in memory as Big Endian USER can switch Endian Big to Little with this
                                                    0 Bypass
                                                    1 Big Endian - Little Endian */
            uint32_t reserved2        : 7;
            uint32_t img_format       : 2;        /*  RX source image format
                                                    0 RGB888 or YUV444
                                                    1 YUV422
                                                    2 Reserved
                                                    3 Reserved */
            uint32_t reserved1        : 22;
        }bit;
    }config;
    uint32_t dma_base_lo;
    uint32_t dma_base_hi;
}DSR_RxConfig_t;

typedef struct {
    union {
        uint32_t reg;
        struct {
            uint32_t endian_change    : 1;      /* If USER need store image to memory with Big Endian set this bit 1
                                                0 Bypass
                                                1 Little Endian - Big Endian */
            uint32_t reserved3        : 7;
            uint32_t img_format       : 2;      /* Select output image convert
                                                0 RGB88 or YUV444
                                                1 YUV422
                                                2 Reserved
                                                3 Reserved */
            uint32_t reserved2        : 6;
            uint32_t convert          : 3;      /* It support output image convert option of YUV444 to YUV422
                                                0 U0_V0
                                                1 U0_V1
                                                2 U1_V0
                                                3 U1_V1
                                                4 U_V_AVG */
            uint32_t reserved1        : 13;
        }bit;
    }config;

    uint32_t dma_base_lo;
    uint32_t dma_base_hi;
}DSR_TxConfig_t;

typedef struct {
    union {
        uint32_t reg;
        struct {
            uint32_t rx_endian_change    : 1;        /* rotator logic process with Little Endian
                                                    If source image frame store in memory as Big Endian USER can switch Endian Big to Little with this
                                                    0 Bypass
                                                    1 Big Endian - Little Endian */
            uint32_t reserved1        : 3;
            uint32_t rx_format       : 2;        /*  RX source image format
                                                    0 RGB888 or YUV444
                                                    1 YUV422
                                                    2 Reserved
                                                    3 Reserved */
            uint32_t reserved2        : 2;
            uint32_t tx_endian_change    : 1;        /* rotator logic process with Little Endian
                                                    If source image frame store in memory as Big Endian USER can switch Endian Big to Little with this
                                                    0 Bypass
                                                    1 Big Endian - Little Endian */
            uint32_t reserved3        : 3;
            uint32_t tx_format       : 2;        /*  TX source image format
                                                    0 RGB888 or YUV444
                                                    1 YUV422
                                                    2 Reserved
                                                    3 Reserved */
            uint32_t reserved4        : 2;
            uint32_t tx_convert_opt        : 3; /* It support output image convert option of YUV444 to YUV422
                                                    0: U0_V0
                                                    1: U0_V1
                                                    2: U1_V0
                                                    3: U1_V1
                                                    4: U_V_AVG*/
            uint32_t reserved5        : 1;                
            uint32_t rotator_mode     : 2;      /*ROTATOR Mode selection 0: reserved, 1: 90 rotate (clockwise), 2: 180 rotate, 3: 270 rotate */
            uint32_t reserved6        : 10;                            
        }bit;
    }config;
    
    union {
        uint32_t reg;
        struct {
            uint32_t ver_size : 16;     /* Source image vertical size 100  4096 in pixel */
            uint32_t hor_size : 16;     /* Source image horizontal size 100  4096 in pixel */
        }bit;
    }size;
    
    uint32_t dma_in_lo;
    uint32_t dma_in_hi;
    uint32_t dma_out_lo;
    uint32_t dma_out_hi;
}RO_Config_t;

typedef struct {
        void* dsr_mmap_buf;
        size_t one_dsr_buf_size;
        int buf_cnt;
        void* dsr_in_buf[BUFF_NUM];
        void* dsr_out_buf[BUFF_NUM];

        unsigned long long in_phys;
        unsigned long long out_phys;

        DSR_RxConfig_t      input_img;
        DSR_TxConfig_t      output_img;
        DSR_Crop_t          crop;
        DSR_Downscale_t     ds;
        RO_Config_t         rotator;
} DSR_Data_t;

#endif