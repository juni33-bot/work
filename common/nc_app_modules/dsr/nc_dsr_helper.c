#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "nc_dsr_api.h"
#include "nc_dsr_helper.h"

int open_device_and_dma_buffers(const char *device_path, int *device_fd, int *input_fd, int *output_fd, size_t buff_size)
{
    if (dsr_device_init(device_path, device_fd) < 0) {
        printf("Error opening device\n");
        return -1;
    }

    if (nc_dmabuf_ctrl_open() < 0) {
        printf("Error opening DMA buffer control\n");
        return -1;
    }

    *input_fd = nc_dmabuf_ctrl_alloc_dma_fd(buff_size);
    *output_fd = nc_dmabuf_ctrl_alloc_dma_fd(buff_size);

    return 0;
}

int dsr_setup_buffer(DSR_Data_t* dsr_buf_info, int dsrfd, dma_alloc_info *info, int input_buffer_fd, int output_buffer_fd, size_t buffer_size) 
{  
    info->dmabuf_fd_in = input_buffer_fd;
    info->dmabuf_fd_out = output_buffer_fd;
    info->buffer_size = buffer_size;

    if (ioctl(dsrfd, DSR_SYSCALL_CMD_SET_IN_ADDR, info) < 0) {
        perror("Error: cannot set input DMA address");
        return -1;
    }

    if (ioctl(dsrfd, DSR_SYSCALL_CMD_SET_OUT_ADDR, info) < 0) {
        perror("Error: cannot set output DMA address");
        return -1;
    }

    for (int i = 0; i < BUFF_NUM; i++) {
        dsr_buf_info->dsr_in_buf[i] = mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, input_buffer_fd, 0);
        if (dsr_buf_info->dsr_in_buf[i] == MAP_FAILED) {
            perror("Input mmap failed");
            for (int j = 0; j < i; j++) {
                munmap(dsr_buf_info->dsr_in_buf[j], buffer_size);
            }
            return -1;
        }
    }

    for (int i = 0; i < BUFF_NUM; i++) {
        dsr_buf_info->dsr_out_buf[i] = mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, output_buffer_fd, 0);
        if (dsr_buf_info->dsr_out_buf[i] == MAP_FAILED) {
            perror("Output mmap failed");
            for (int j = 0; j < i; j++) {
                munmap(dsr_buf_info->dsr_out_buf[j], buffer_size);
            }
            for (int j = 0; j < BUFF_NUM; j++) {
                munmap(dsr_buf_info->dsr_in_buf[j], buffer_size);
            }
            return -1;
        }
    }

    if (ioctl(dsrfd, DSR_SYSCALL_CMD_IN_PHY_ADDR, &dsr_buf_info->in_phys) == -1) {
        printf("nc_dsr_mmap_in_phyaddr() failure\n");
    }

    if (ioctl(dsrfd, DSR_SYSCALL_CMD_OUT_PHY_ADDR, &dsr_buf_info->out_phys) == -1) {
        printf("nc_dsr_mmap_out_phyaddr() failure\n");
    }
    
    /* init CPU access to the DMA buffer */
    nc_dmabuf_ctrl_begin_cpu_access(input_buffer_fd);
    nc_dmabuf_ctrl_begin_cpu_access(output_buffer_fd);

    return 0;
}

int process_image(const char *input_file, const char *output_file, size_t buff_size, DSR_Data_t *dsr_data,
                  int device_fd, st_nc_dsr_config *dsr_config, img_input_config *input_config,
                  img_output_config *output_config, dma_alloc_info *info)
{
    FILE *fp = fopen(input_file, "rb");
    if (!fp) {
        printf("Error opening file %s\n", input_file);
        return -1;
    }

    fread(dsr_data->dsr_in_buf[0], 1, buff_size, fp);
    fclose(fp);
    
    dsr_downscale(device_fd, dsr_data, *input_config, *output_config, *dsr_config, 0);
    dsr_rotator(device_fd, dsr_data, *dsr_config);

    fp = fopen(output_file, "wb");
    if (!fp) {
        printf("Error: cannot open output file %s\n", output_file);
        return -1;
    }

    fwrite(dsr_data->dsr_out_buf[0], 1, buff_size, fp); 
    fclose(fp);

    nc_dmabuf_ctrl_end_cpu_access(info->dmabuf_fd_in);
    nc_dmabuf_ctrl_end_cpu_access(info->dmabuf_fd_out);

    nc_dmabuf_ctrl_free_dma_fd(info->dmabuf_fd_in);
    nc_dmabuf_ctrl_free_dma_fd(info->dmabuf_fd_out);

    nc_dmabuf_ctrl_close();

    return 0;
}

int dsr_config_crop(st_nc_dsr_config *dsr_config, int crop_enable, int crop_x, int crop_y, int crop_width, int crop_height) 
{
    if(crop_x > crop_width || crop_y > crop_height) {
        printf("Error: crop_x or crop_y is greater than crop_width or crop_height\n");
        return -1;
    }

    dsr_config->crop_enable = crop_enable;
    dsr_config->crop_x = crop_x;
    dsr_config->crop_y = crop_y;
    dsr_config->crop_width = crop_width;
    dsr_config->crop_height = crop_height;

    printf("Crop configuration : crop_enable = %d, crop_x = %d, crop_y = %d, crop_width = %d, crop_height = %d\n", 
        dsr_config->crop_enable, dsr_config->crop_x, dsr_config->crop_y, dsr_config->crop_width, dsr_config->crop_height);

    return 0;
}

int dsr_config_downscale(st_nc_dsr_config *dsr_config, int ds_enable, int ds_width, int ds_height) 
{
    dsr_config->ds_enable = ds_enable;
    dsr_config->ds_width = ds_width;
    dsr_config->ds_height = ds_height;

    printf("Downscale configuration : ds_enable = %d, ds_width = %d, ds_height = %d\n", 
        dsr_config->ds_enable, dsr_config->ds_width, dsr_config->ds_height);

    return 0;
}

void dsr_config_rotator(st_nc_dsr_config *dsr_config, int flip, int rot_mode, int rot_convert, int in_format, int out_format, int in_width, int in_height) 
{
    dsr_config->flip = flip;
    dsr_config->rot_mode = (dsr_rot_mod_t)rot_mode;
    dsr_config->rot_convert = (dsr_ROT_CON_t)rot_convert;
    dsr_config->in_format = in_format;
    dsr_config->out_format = out_format;
    dsr_config->in_width = in_width;
    dsr_config->in_height = in_height;

    printf("Rotator configuration : flip = %d, rot_mode = %d, rot_convert = %d, in_format = %d, out_format = %d, in_width = %d, in_height = %d\n", 
        dsr_config->flip, dsr_config->rot_mode, dsr_config->rot_convert, dsr_config->in_format, dsr_config->out_format, dsr_config->in_width, dsr_config->in_height);
}

void dsr_initialize(st_nc_dsr_config *dsr_config, img_input_config *input_config, img_output_config *output_config, int flip) 
{
    input_config->format = IMG_FORMAT_RGB888;
    input_config->width = 1280;
    input_config->height = 720;
    output_config->format = IMG_FORMAT_RGB888;

    dsr_config_crop(dsr_config, 0, 0, 0, 416, 240);
    dsr_config_downscale(dsr_config, 1, 640, 480);
    dsr_config_rotator(dsr_config, flip, ROT_MODE_90, ROT_U0_V0, 0, 0, 4096, 2160);
}
