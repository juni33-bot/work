#!/bin/sh

echo "start applications..."

export APP_PATH=/mnt/user_data/applications

source ${APP_PATH}/run_app_wayland_egl_display.sh
