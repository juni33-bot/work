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

#include "nc_dsr_set.h"
#include "nc_dsr_api.h"

/* dsr device initializer */
int dsr_device_init(const char* device_path, int* device_fd)
{
    *device_fd = open(device_path, O_RDWR);

    if (*device_fd < 0) {
        perror("Error opening device");
        return -1;
    }

    return 0;
}

int dsr_device_deinit(int* device_fd)
{
    close(*device_fd);
    return 0;
}


int dsr_downscale(int device_fd, DSR_Data_t* dsr_data, img_input_config input_config, img_output_config output_config,  st_nc_dsr_config crop_cfg, int flip)
{
    crop_cfg.flip = flip;
    nc_dsr_set_crop_ds(device_fd, dsr_data, crop_cfg);

    nc_dsr_set_ds_input(device_fd, dsr_data, (uint8_t*)(uintptr_t)dsr_data->in_phys, input_config);
    nc_dsr_set_ds_output(device_fd, dsr_data, (uint8_t*)(uintptr_t)dsr_data->out_phys, output_config);
    
    return nc_dsr_start(device_fd);
}

int dsr_rotator(int device_fd, DSR_Data_t* dsr_data, st_nc_dsr_config config)
{
    nc_dsr_set_rot(device_fd, dsr_data, (uint8_t*)(uintptr_t)dsr_data->in_phys, (uint8_t*)(uintptr_t)dsr_data->out_phys, config);

    return nc_rot_start(device_fd);
}