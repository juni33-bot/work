#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nc_dsr_set.h"

void nc_dsr_set_ds_input(int dsr_fd, DSR_Data_t* inData, uint8_t* in_addr, img_input_config config)
{
    DSR_RxConfig_t* input_data = &inData->input_img;

    input_data->dma_base_hi = (uint32_t)(((uint64_t)in_addr) >> 32);
    input_data->dma_base_lo = (uint32_t)(uint64_t)in_addr;
    input_data->config.bit.endian_change = 1;
    input_data->config.bit.img_format = (unsigned char)(config.format & 0x03);
    input_data->size.bit.hor_size = (unsigned short int)(config.width & 0xFFFF);
    input_data->size.bit.ver_size = (unsigned short int)(config.height & 0xFFFF);

    if (ioctl(dsr_fd, DSR_SYSCALL_CMD_SET_INPUT, input_data) == -1) {
        perror("Error setting input configuration");
    }
}

void nc_dsr_set_ds_output(int dsr_fd, DSR_Data_t* inData, uint8_t* out_addr, img_output_config config)
{
    DSR_TxConfig_t* output_data = &inData->output_img;

    output_data->dma_base_hi = (uint32_t)(((uint64_t)out_addr) >> 32);
    output_data->dma_base_lo = (uint32_t)(uint64_t)out_addr;
    output_data->config.bit.endian_change = 1;
    output_data->config.bit.convert = 0;
    output_data->config.bit.img_format = (unsigned char)(config.format & 0x03);

    if (ioctl(dsr_fd, DSR_SYSCALL_CMD_SET_OUTPUT, output_data) == -1) {
        perror("Error setting output configuration");
    }
}

void nc_dsr_set_crop_ds(int dsr_fd, DSR_Data_t* inData, st_nc_dsr_config config)
{
    DSR_Crop_t *stCrop = &inData->crop;
    DSR_Downscale_t *stDs = &inData->ds;

    stCrop->crop_enable = config.crop_enable;
    stCrop->size.bit.hor = (unsigned short int)(config.crop_width & 0xFFFF);
    stCrop->size.bit.ver = (unsigned short int)(config.crop_height & 0xFFFF);
    stCrop->start_pos.bit.hor = (unsigned short int)(config.crop_x & 0xFFFF);
    stCrop->start_pos.bit.ver = (unsigned short int)(config.crop_y & 0xFFFF);
    stDs->config.bit.ds_enable = (unsigned char)(config.ds_enable & 0x01);
    stDs->config.bit.alpha = 0;
    stDs->size.bit.hor = (unsigned short int)(config.ds_width & 0xFFFF);
    stDs->size.bit.ver = (unsigned short int)(config.ds_height & 0xFFFF);
    stDs->flip = config.flip;

    if (ioctl(dsr_fd, DSR_SYSCALL_CMD_SET_CROP, stCrop) == -1) {
        perror("Error setting crop configuration");
    }

    if (ioctl(dsr_fd, DSR_SYSCALL_CMD_SET_DOWNSACLER, stDs) == -1) {
        perror("Error setting downscale configuration");
    }
}

void nc_dsr_set_rot(int dsr_fd, DSR_Data_t* inData, uint8_t* in_addr, uint8_t* out_addr, st_nc_dsr_config config)
{
    RO_Config_t* input_data = &inData->rotator;

    // Set input DMA addresses
    uint64_t addr = (uint64_t)(uintptr_t)in_addr;
    input_data->dma_in_hi = (uint32_t)(addr >> 32);
    input_data->dma_in_lo = (uint32_t)(addr & 0xFFFFFFFF);

    // Set output DMA addresses
    addr = (uint64_t)(uintptr_t)out_addr;
    printf("Config width: %d, height: %d\n", config.in_width, config.in_height);

    // Configure rotation parameters
    input_data->config.bit.rx_endian_change = 1;
    input_data->config.bit.rx_format = (unsigned char)(config.in_format & 0x03);
    input_data->config.bit.tx_endian_change = 1;
    input_data->config.bit.tx_format = (unsigned char)(config.out_format & 0x03);
    input_data->config.bit.rotator_mode = (unsigned char)(config.rot_mode & 0x03);
    input_data->config.bit.tx_convert_opt = (unsigned char)(config.rot_convert & 0x07);
    input_data->size.bit.hor_size = (unsigned short)config.in_width;
    input_data->size.bit.ver_size = (unsigned short)config.in_height;

    if (ioctl(dsr_fd, DSR_SYSCALL_CMD_SET_ROTATOR, input_data) == -1) {
        perror("Error setting rotator configuration");
    }
}

int nc_dsr_start(int dsr_fd)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000;
    unsigned int chk = 0, count_dsr=0;

    // printf("downscale started\n");

    if (ioctl(dsr_fd, DSR_SYSCALL_CMD_START, NULL) == -1) {
            perror("Error reading variable information");
            return -1;
    }

    if (ioctl(dsr_fd, DSR_SYSCALL_CMD_STATE, &chk) == -1) {
            perror("Error reading fixed information");
            return -1;
    }

    while(!chk)
    {
        nanosleep(&ts, NULL);
        count_dsr++;

        if(count_dsr>1000) {
            chk=1;
            printf("Error Dsr!!!\n");
            return -1;
        }

        if (ioctl(dsr_fd, DSR_SYSCALL_CMD_STATE, &chk) == -1) {
            perror("Error reading fixed information");
        }
    }

    // printf("downscale stopped\n");

    return count_dsr;
}

int nc_rot_start(int dsr_fd)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000;
    unsigned int chk = 0, count_dsr=0;

    printf("rotator started\n");

    if (ioctl(dsr_fd, ROTATOR_SYSCALL_CMD_START, NULL) == -1) {
        perror("Error reading variable information");
    }

    if (ioctl(dsr_fd, ROTATOR_SYSCALL_CMD_STATE, &chk) == -1) {
        perror("Error reading fixed information");
    }

    while(!chk)
    {
        nanosleep(&ts, NULL);
        count_dsr++;

        if(count_dsr>1000) {
            chk=1;
            printf("Error Rotator!!!\n");
            return -1;
        }

        if (ioctl(dsr_fd, ROTATOR_SYSCALL_CMD_STATE, &chk) == -1) {
            perror("Error reading fixed information");
        }
    }

    printf("rotator stopped\n");

    return count_dsr;
}
