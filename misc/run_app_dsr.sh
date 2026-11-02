#!/bin/sh

if [ ! $APP_PATH]; then
APP_PATH=.
fi

export LD_LIBRARY_PATH=${APP_PATH}/misc/lib

devmem 0x06004608 32 0x8

insmod ${APP_PATH}/../modules/nc_dmabuf_ctrl.ko
insmod ${APP_PATH}/../modules/nc_dsr_drv.ko

${APP_PATH}/app_dsr
