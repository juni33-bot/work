#!/bin/bash

###########
# utility function

function exit_on_error
{
        if [ $? -ne 0 ] ; then
                echo -e "\n\n\n!!!!!!!!!!!!!!!!!!!!! Fail to build !!!!!!!!!!!!!!!!!!\n\n\n"
                exit
        fi
}

###########

OUTPUT_DIR=${PWD}/../output/user_data/applications
mkdir -p ${OUTPUT_DIR}

echo -e "\n================= build npu_app ================="
pushd npu_app
make
cp -prvf app_npu ${OUTPUT_DIR}/
popd

echo -e "\n================= build wayland_egl_app ================="
pushd wayland_egl_app
make
cp -prvf app_wayland_egl ${OUTPUT_DIR}/
popd

echo -e "\n================= build wayland_cam_app ================="
pushd wayland_cam_app
make
cp -prvf app_wayland_cam ${OUTPUT_DIR}/
popd

echo -e "\n================= build wayland_npu_app ================="
pushd wayland_npu_app
make
cp -prvf app_wayland_npu ${OUTPUT_DIR}/
popd

echo -e "\n================= build vcodec_app ================="
pushd vcodec_app
make
cp -prvf app_vcodec ${OUTPUT_DIR}/
popd

echo -e "\n================= build dsr_app ================="
pushd dsr_app
make
cp -prvf app_dsr ${OUTPUT_DIR}/
popd

echo -e "\n================= build tflite_det_app ================="
pushd tflite_det_app
echo "current directory: $(pwd)"
mkdir app_build; pushd app_build
cmake -DCMAKE_TOOLCHAIN_FILE=../../tool.cmake \
-DTFLITE_ENABLE_GPU=ON -DTFLITE_ENABLE_XNNPACK=OFF \
-DTFLITE_HOST_TOOLS_DIR=../../common/third_party/flatc/bin \
-DBUILD_SHARED_LIBS=ON ..
cmake --build . -j
cp -prvf app_tflite_det ${OUTPUT_DIR}/
if test -f tensorflow-lite/libtensorflow-lite.so; then 
  cp -prvf tensorflow-lite/libtensorflow-lite.so ${OUTPUT_DIR}/misc/lib/
fi
popd; popd

echo -e "\n================= copy misc files to ${OUTPUT_DIR} ================="

if [ ! -d "${OUTPUT_DIR}/misc" ]; then
  cp -prvf ${PWD}/misc ${OUTPUT_DIR}/
  mv -v ${OUTPUT_DIR}/misc/run*.sh ${OUTPUT_DIR}/
  mv -v ${OUTPUT_DIR}/misc/set*.sh ${OUTPUT_DIR}/
  mv -v ${OUTPUT_DIR}/misc/nc_module.ini ${OUTPUT_DIR}/
  mv -v ${OUTPUT_DIR}/misc/nc_module_load.sh ${OUTPUT_DIR}/
  mv -v ${OUTPUT_DIR}/misc/start*.sh ${OUTPUT_DIR}/../
  if [ ! -d "${OUTPUT_DIR}/misc/vcodec" ]; then
    mkdir -p ${OUTPUT_DIR}/misc/vcodec
  fi
fi

echo -e "\nBuild done :)\n"
