// app.c
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

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    DSR_Data_t dsr_data;
    st_nc_dsr_config dsr_config;
    img_input_config input_config;
    img_output_config output_config;
    size_t BUFF_SIZE = 0;
    dma_alloc_info info;

    int device_fd = 0, input_fd = 0, output_fd = 0;
    const char *image_type[3] = {"yuv422", "yuv444", "rgb"};
    char input_file[64], output_file[64];
    long page_size = sysconf(_SC_PAGESIZE);

    input_config.format = IMG_FORMAT_YUV444; /* 0: YUV422 or RGB888, 1: YUV444 */
    input_config.width = 4096;
    input_config.height = 2160;
    output_config.format = IMG_FORMAT_YUV444;

    dsr_config_crop(&dsr_config, 0, 0, 0, 416, 240); /* crop_enable, crop_x, crop_y, crop_width, crop_height */
    dsr_config_downscale(&dsr_config, 1, 640, 480); /* ds_enable, ds_width, ds_height */
    dsr_config_rotator(&dsr_config, 1, ROT_MODE_90, ROT_U0_V0, 0, 0, 640, 480); /* flip, rot_mode, rot_convert, in_format, out_format, in_width, in_height */

    int pixel_data = (input_config.format == IMG_FORMAT_RGB888) ? 3 : 2;
    BUFF_SIZE = input_config.width * input_config.height * pixel_data;
    BUFF_SIZE = ((BUFF_SIZE + page_size - 1) / page_size) * page_size;

    if (open_device_and_dma_buffers("/dev/dsr", &device_fd, &input_fd, &output_fd, BUFF_SIZE) < 0) {
        return -1;
    }

    if (dsr_setup_buffer(&dsr_data, device_fd, &info, input_fd, output_fd, BUFF_SIZE) < 0) {
        return -1;
    }

    sprintf(input_file, "test_%s_%dx%d.yuv", image_type[input_config.format], input_config.width, input_config.height);
    sprintf(output_file, "test_%s_%dx%d_output.yuv", image_type[output_config.format], input_config.width, input_config.height);

    if (process_image(input_file, output_file, BUFF_SIZE, &dsr_data, device_fd, &dsr_config,
                      &input_config, &output_config, &info) < 0) {
        return -1;
    }

    return 0;
}
