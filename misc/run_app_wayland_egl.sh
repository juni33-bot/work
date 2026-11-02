#!/bin/sh

if [ ! $APP_PATH ]; then
APP_PATH=.
fi

export LD_LIBRARY_PATH=${APP_PATH}/misc/lib
export XDG_RUNTIME_DIR=/var/run
export WAYLAND_DISPLAY=wayland-1

${APP_PATH}/app_wayland_egl &
