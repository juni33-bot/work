#!/bin/sh

if [ ! $APP_PATH]; then
APP_PATH=.
fi
export LD_LIBRARY_PATH=${APP_PATH}/misc/lib


LOOPBACK_CH1_VID=11

INPUT_FILE_PATH=${APP_PATH}/misc/front_sunny_daytime_highway_0001.mp4

insmod ${APP_PATH}/../modules/v4l2loopback_ko video_nr=$((LOOPBACK_CH1_VID+0)),$((LOOPBACK_CH1_VID+1)),$((LOOPBACK_CH1_VID+2)),$((LOOPBACK_CH1_VID+3)),$((LOOPBACK_CH1_VID+4))

${APP_PATH}/misc/ffmpeg -stream_loop -1 -re -i misc/image_npu/1920_1080_0001.mp4 -video_size 1920x1080 -pix_fmt rgb24 -r 30 -f v4l2 /dev/video$((LOOPBACK_CH1_VID+0)) -pix_fmt rgb24 -r 30 -f v4l2 /dev/video$((LOOPBACK_CH1_VID+1)) -pix_fmt rgb24 -r 30 -f v4l2 /dev/video$((LOOPBACK_CH1_VID+2)) -pix_fmt rgb24 -r 30 -f v4l2 /dev/video$((LOOPBACK_CH1_VID+3))

