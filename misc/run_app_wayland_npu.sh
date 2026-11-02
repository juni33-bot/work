#!/bin/sh

if [ ! $APP_PATH ]; then
APP_PATH=.
fi

export XDG_RUNTIME_DIR=/var/run
export WAYLAND_DISPLAY=wayland-1
#export LD_LIBRARY_PATH=${APP_PATH}/misc/lib:${APP_PATH}/misc/lib/opencv2/lib
export LD_LIBRARY_PATH=${APP_PATH}/misc/lib
#insmod ${APP_PATH}/misc/apache6fb.ko
insmod ${APP_PATH}/../modules/aiware_drv_ng.ko
insmod ${APP_PATH}/../modules/aiware_drv_ng-apache6.ko
insmod ${APP_PATH}/../modules/nc_dmabuf_ctrl.ko
insmod ${APP_PATH}/../modules/nc_dsr_drv.ko

# devmem 0x07014108 32 0x24

# echo 5 > /proc/sys/kernel/printk

${APP_PATH}/app_wayland_npu &
