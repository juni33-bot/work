#ifndef DSR_SET_H
#define DSR_SET_H

#include "nc_dsr_type.h"

void nc_dsr_set_ds_input(int dsr_fd, DSR_Data_t* inData, uint8_t* in_addr, img_input_config config);
void nc_dsr_set_ds_output(int dsr_fd, DSR_Data_t* inData, uint8_t* out_addr, img_output_config config);
void nc_dsr_set_crop_ds(int dsr_fd, DSR_Data_t* inData, st_nc_dsr_config config);
void nc_dsr_set_rot(int dsr_fd, DSR_Data_t* inData, uint8_t* in_addr, uint8_t* out_addr, st_nc_dsr_config config);
int nc_dsr_start(int dsr_fd);
int nc_rot_start(int dsr_fd);
#endif