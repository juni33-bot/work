#!/bin/sh

if [ ! $APP_PATH]; then
APP_PATH=.
fi

export LD_LIBRARY_PATH=${APP_PATH}/misc/lib

#echo "npu scu ongo reset 0x7f"
#devmem 0x06804a00 32 0x7f
#sleep 1
#devmem 0x06804a00 32 0x0

#echo "BUS_PRE1_CLKMUX"
#devmem 0x05011144 32 0x00
#devmem 0x05011144

#echo "NPU_PRE1_CLKMUX"
#devmem 0x06804400 32 0x00
#devmem 0x06804400

#echo "VAL_NPU_AXI_CLKMUX"
#devmem 0x06804600 32 0x00
#devmem 0x06804600

# echo "VAL_BUS_MAIN_CLKDIV"
# devmem 0x05012050 32 0x5
# devmem 0x05012050

# echo "VAL_NPU_CORE_CLKDIV"
# devmem 0x06804500 32 0x4
# devmem 0x06804500

# echo "VAL_NPU_AXI_CLKDIV"
# devmem 0x06804700 32 0x4
# devmem 0x06804700

insmod ${APP_PATH}/../modules/aiware_drv_ng.ko
insmod ${APP_PATH}/../modules/aiware_drv_ng-apache6.ko

${APP_PATH}/app_npu ${APP_PATH}/misc/networks/Yolov5s/yolov5s_coco_640x384_apache6sr250_aiw4939.aiwbin ${APP_PATH}/misc/image_npu/png_list_640x384.txt ${APP_PATH}/yolov5s_cnn_output