#!/bin/sh

if [ ! $APP_PATH ]; then
APP_PATH=.
fi

export LD_LIBRARY_PATH=${APP_PATH}/../misc/lib
export XDG_RUNTIME_DIR=/var/run
export WAYLAND_DISPLAY=wayland-1

(
    dmesg -n 1
    echo ">>> Starting VISION0 Update (0x2B)..."
    ${APP_PATH}/i2c_downloader_app 0x2B

    sleep 1

    echo ">>> Starting VISION1 Update (0x2F)..."
    ${APP_PATH}/i2c_downloader_app 0x2F
    dmesg -n 7
) &