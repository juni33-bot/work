#!/bin/sh

if [ ! $APP_PATH ]; then
APP_PATH=.
fi

export XDG_RUNTIME_DIR=/var/run
export WAYLAND_DISPLAY=wayland-1

${APP_PATH}/app_wayland_cam &
