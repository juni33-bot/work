#ifndef DSR_HELPER_H
#define DSR_HELPER_H

#include "nc_dsr_api.h"
#include "nc_dsr_type.h"

int open_device_and_dma_buffers(const char *device_path, int *device_fd, int *input_fd, int *output_fd, size_t buff_size);
int dsr_setup_buffer(DSR_Data_t* dsr_buf_info, int dsrfd, dma_alloc_info *info, int input_buffer_fd, int output_buffer_fd, size_t buffer_size);
int process_image(const char *input_file, const char *output_file, size_t buff_size, DSR_Data_t *dsr_data,
                  int device_fd, st_nc_dsr_config *dsr_config, img_input_config *input_config,
                  img_output_config *output_config, dma_alloc_info *info);
int dsr_config_crop(st_nc_dsr_config *dsr_config, int crop_enable, int crop_x, int crop_y, int crop_width, int crop_height);
int dsr_config_downscale(st_nc_dsr_config *dsr_config, int ds_enable, int ds_width, int ds_height);
void dsr_config_rotator(st_nc_dsr_config *dsr_config, int flip, int rot_mode, int rot_convert, int in_format, int out_format, int in_width, int in_height);
void dsr_initialize(st_nc_dsr_config *dsr_config, img_input_config *input_config, img_output_config *output_config, int flip);

#endif
