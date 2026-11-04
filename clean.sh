#!/bin/bash

set -e
OUTPUT_DIR=${PWD}/../output/user_data/applications

echo clean npu_app
pushd npu_app
make clean
popd

echo clean wayland_egl_app
pushd wayland_egl_app
make clean
popd

echo clean wayland_cam_app
pushd wayland_cam_app
make clean
popd

echo clean wayland_npu_app
pushd wayland_npu_app
make clean
popd

echo clean vcodec_app
pushd vcodec_app
make clean
popd

echo clean dsr_app
pushd dsr_app
make clean
popd

echo clean tflite_det_app
pushd tflite_det_app
rm -rf app_build
popd

rm -rfv ${OUTPUT_DIR}
