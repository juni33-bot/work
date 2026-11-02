#ifndef DSR_MEM_H
#define DSR_MEM_H

#include "nc_dsr_type.h"

int dsr_device_init(const char* device_path, int* device_fd);
int dsr_device_deinit(int* device_fd);
int dsr_downscale(int device_fd, DSR_Data_t* dsr_data, img_input_config input_config, img_output_config output_config, st_nc_dsr_config crop_cfg, int flip);
int dsr_rotator(int device_fd, DSR_Data_t* dsr_data, st_nc_dsr_config config);

#endif