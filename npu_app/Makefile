#############################################################
#
# Makefile for NextChip APACHE6 Linux Driver Test Application
#
#############################################################
#############################################################
# Save cnn output bin file
DUMP_CNN_OUTPUT = no

# Select whether to use postprocesses and to save result image
USE_POSTPROCESS = yes
ifeq ($(USE_POSTPROCESS), yes)
SAVE_OUTPUT_RESULT_IMAGE = yes
SAVE_META_DATA = no
endif

#############################################################
ARCH                ?= arm64
#ARCH                ?= arm
ifeq ($(ARCH), arm64)
CROSS_COMPILE       ?= aarch64-none-linux-gnu-
else ifeq ($(ARCH), arm)
CROSS_COMPILE       ?= arm-linux-gnueabi-
endif

TARGET_TEST         := app_npu

CC                  = ${CROSS_COMPILE}gcc
CXX                 = ${CROSS_COMPILE}g++

COMMON_DIR          = $(PWD)/../common
AIWARE_LIB_DIR      = $(COMMON_DIR)/aiware
NC_APP_MODULES      = $(COMMON_DIR)/nc_app_modules
3RD_PARTY_DIR       = $(COMMON_DIR)/third_party
OPENCV_DIR          = $(3RD_PARTY_DIR)/opencv2
OPEN_OTHERS_DIR      = $(3RD_PARTY_DIR)/opencv_others
NC_CNN_DIR          = $(NC_APP_MODULES)/cnn
NC_CNN_CORE_DIR     = $(NC_APP_MODULES)/cnn/core
ifeq ($(USE_POSTPROCESS), yes)
INILIB_DIR          = $(3RD_PARTY_DIR)/iniLib
CAIRO_DIR           = $(3RD_PARTY_DIR)/cairo
NC_CNN_NETWORKS_DIR     = $(NC_APP_MODULES)/cnn/networks
NC_CNN_NETWORKS_YOLOV5S_DIR = $(NC_APP_MODULES)/cnn/networks/yolov5
ifeq ($(SAVE_META_DATA), yes)
JSON_DIR            = $(3RD_PARTY_DIR)/json-c
endif
endif
LIBDRM_DIR          = $(3RD_PARTY_DIR)/libdrm

INC_DIR             += -I./
INC_DIR             += -I$(COMMON_DIR)
INC_DIR             += -I$(3RD_PARTY_DIR)
INC_DIR             += -I$(OPENCV_DIR)
INC_DIR             += -I$(NC_APP_MODULES)/utils
INC_DIR             += -I$(OPEN_OTHERS_DIR)
INC_DIR             += -I$(NC_CNN_DIR)
INC_DIR             += -I$(NC_CNN_CORE_DIR)
ifeq ($(USE_POSTPROCESS), yes)
INC_DIR             += -I$(INILIB_DIR)
INC_DIR             += -I$(NC_APP_MODULES)/wrapper
INC_DIR             += -I$(NC_APP_MODULES)/config
INC_DIR             += -I$(CAIRO_DIR)
INC_DIR             += -I$(NC_CNN_NETWORKS_DIR)
INC_DIR             += -I$(NC_CNN_NETWORKS_YOLOV5S_DIR)
ifeq ($(SAVE_META_DATA), yes)
INC_DIR             += -I$(JSON_DIR)
endif
endif

ifeq ($(ARCH), arm64)
CFLAGS_TEST         :=
CFLAGS_TEST         += -mcpu=cortex-a53+crypto
else ifeq ($(ARCH), arm)
CFLAGS_TEST         := --static
CFLAGS_TEST         += -mcpu=cortex-a53
CFLAGS_TEST         += -mfpu=neon-fp-armv8
CFLAGS_TEST         += -mfloat-abi=softfp
endif
CFLAGS_TEST         += -O2
CFLAGS_TEST         += -g -std=c++11
CFLAGS_TEST         += -fno-inline -fno-omit-frame-pointer
CFLAGS_TEST         += $(INC_DIR)
CFLAGS_TEST         += -Wall -Wextra -Wformat=2 -Wpedantic -Wconversion -fstack-protector-all
# CFLAGS_TEST         += -Werror

ifeq ($(DUMP_CNN_OUTPUT), yes)
CFLAGS_TEST         += -DDUMP_CNN_OUTPUT
endif

ifeq ($(USE_POSTPROCESS), yes)
CFLAGS_TEST         += -DUSE_POSTPROCESS
CFLAGS_TEST         += -DUSE_CNN_YOLOV5_NETWORK
ifeq ($(SAVE_OUTPUT_RESULT_IMAGE), yes)
CFLAGS_TEST         += -DSHOW_CLASS_LABEL_BOX
CFLAGS_TEST         += -DSAVE_OUTPUT_RESULT_IMAGE
endif
ifeq ($(SAVE_META_DATA), yes)
CFLAGS_TEST         += -DSAVE_META_DATA
endif
endif

# Add link library
LIBRARY             += $(OPEN_OTHERS_DIR)/libjpeg.so.8
LIBRARY             += $(OPEN_OTHERS_DIR)/libpng16.so.16
LIBRARY             += $(OPEN_OTHERS_DIR)/libjasper.so.4
LIBRARY             += $(OPEN_OTHERS_DIR)/libtiff.so.5
LIBRARY             += $(AIWARE_LIB_DIR)/lib_$(ARCH)/libaiware-runtime-selftestdata.a
LIBRARY             += $(AIWARE_LIB_DIR)/lib_$(ARCH)/libaiware-runtime-device-lib-c.a $(AIWARE_LIB_DIR)/lib_$(ARCH)/libaiware-common-lib-c.so $(AIWARE_LIB_DIR)/lib_$(ARCH)/libaiware-runtime-common-lib-c.so $(AIWARE_LIB_DIR)/lib_$(ARCH)/libaiware-quant-reorder-lib-c.so
LIBRARY             += $(OPENCV_DIR)/lib/libopencv_core.so $(OPENCV_DIR)/lib/libopencv_imgproc.so $(OPENCV_DIR)/lib/libz.so
LIBRARY             += $(OPENCV_DIR)/lib/libopencv_imgcodecs.so
ifeq ($(USE_POSTPROCESS), yes)
LIBRARY             += $(CAIRO_DIR)/lib_$(ARCH)/libcairo.a
LIBRARY             += $(CAIRO_DIR)/lib_$(ARCH)/libpixman-1.a
ifeq ($(SAVE_META_DATA), yes)
LIBRARY             += $(JSON_DIR)/libjson-c.a
endif
endif

LIBRARY             += $(LIBDRM_DIR)/lib/libdrm.so $(LIBDRM_DIR)/lib/libc.so.6 $(LIBDRM_DIR)/lib/ld-linux-aarch64.so.1
LIBRARY             += $(LIBDRM_DIR)/lib/libdl.so.2 $(LIBDRM_DIR)/lib/libpthread.so.0 $(LIBDRM_DIR)/lib/librt.so.1
LIBRARY             += -lm -lpthread -lrt

#Test App
CSRCS               := npu_app.c

# Related cnn network
CSRCS               += $(NC_APP_MODULES)/utils/nc_utils.c
ifeq ($(USE_POSTPROCESS), yes)
CSRCS               += $(INILIB_DIR)/ini.c
CSRCS               += $(NC_APP_MODULES)/config/nc_cnn_config_parser.c
CSRCS               += $(NC_CNN_CORE_DIR)/nc_cnn_anchor.c
ifeq ($(SAVE_META_DATA), yes)
CSRCS               += $(JSON_DIR)/jsonc_writer.c
endif
endif

OBJS                := $(CSRCS:.c=.o) $(CPPSRCS:.cpp=.o)

all:
	${CXX} $(CFLAGS_TEST) $(CPPSRCS) $(CSRCS) -o $(TARGET_TEST) $(LIBRARY)

clean:
	rm -rf $(TARGET_TEST)
