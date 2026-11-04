#!/bin/sh

if [ ! $APP_PATH ]; then
APP_PATH=.
# APP_PATH=/nfs/applications
fi

# echo 0 > /proc/sys/kernel/printk

export LD_LIBRARY_PATH=${APP_PATH}/misc/lib

MODEL_FILE_PATH=${APP_PATH}/misc/networks/tflite_det/ssd_mobilenet_v2_coco.tflite

INPUT_FILE_PATH=${APP_PATH}/misc/image_gpu/airport.jpg

${APP_PATH}/app_tflite_det ${MODEL_FILE_PATH} ${INPUT_FILE_PATH}