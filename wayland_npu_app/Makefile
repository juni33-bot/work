#############################################################
#
# Makefile for NextChip APACHE6 Linux Driver Test Application
#
#############################################################

#############################################################
# pre-set

#############################################################
# adjust-set

#############################################################
# set cnn network - PELEESEG / PELEEDET / YOLOV5 / YOLOV8 / UFLD / TRICHIMERA
CNN_NETWORK_SETTING = TRICHIMERA

ifeq ($(CNN_NETWORK_SETTING), PELEEDET)
USE_CNN_PELEEDET_NETWORK = yes
SHOW_PELEE_DETECT = yes
endif

ifeq ($(CNN_NETWORK_SETTING), PELEESEG)
USE_CNN_PELEESEG_NETWORK = yes
SHOW_PELEE_SEG    = yes
endif

ifeq ($(CNN_NETWORK_SETTING), YOLOV5)
USE_CNN_YOLOV5_NETWORK = yes
SHOW_YOLOV5_DETECT = yes
endif

ifeq ($(CNN_NETWORK_SETTING), YOLOV8)
USE_CNN_YOLOV8_NETWORK = yes
SHOW_YOLOV8_DETECT = yes
endif

ifeq ($(CNN_NETWORK_SETTING), UFLD)
USE_CNN_UFLD_NETWORK = yes
SHOW_UFLD_LANE = yes
USE_UFLD_TRACKING = yes
# USE_UFLD_NETWORK_DEBUGGING = row
endif

ifeq ($(CNN_NETWORK_SETTING), TRICHIMERA)
USE_CNN_TRICHIMERA_NETWORK = yes
SHOW_TRI_CHIMERA = yes
USE_CNN_YOLOV8_NETWORK = yes
USE_CNN_PELEESEG_NETWORK = yes
USE_CNN_UFLD_NETWORK = yes
USE_UFLD_TRACKING = yes
endif

# USE_8MP_VI          = yes
# USE_BYTETRACK       = yes
# USE_ADAS_LD         = yes

ARCH                ?= arm64
# ARCH                ?= arm
ifeq ($(ARCH), arm64)
CROSS_COMPILE       ?= aarch64-none-linux-gnu-
else ifeq ($(ARCH), arm)
CROSS_COMPILE       ?= arm-linux-gnueabi-
endif

#############################################################

CC                  = ${CROSS_COMPILE}gcc
CXX                 = ${CROSS_COMPILE}g++

#############################################################

THIS_DIR            = .
COMMON_DIR          = $(PWD)/../common
NC_APP_MODULES      = $(COMMON_DIR)/nc_app_modules
3RD_PARTY_DIR       = $(COMMON_DIR)/third_party
LIBDRM_DIR          = $(3RD_PARTY_DIR)/libdrm
OPENGL_DIR          = $(3RD_PARTY_DIR)/opengl
SOIL_DIR            = $(3RD_PARTY_DIR)/soil
INILIB_DIR          = $(3RD_PARTY_DIR)/iniLib
AIWARE_LIB_DIR      = $(COMMON_DIR)/aiware
NC_NEON_DIR         = $(NC_APP_MODULES)/neon
NC_CNN_DIR          = $(NC_APP_MODULES)/cnn
NC_CNN_CORE_DIR     = $(NC_APP_MODULES)/cnn/core
NC_CNN_NETWORKS_DIR = $(NC_APP_MODULES)/cnn/networks
NC_OPENGL_DIR       = $(NC_APP_MODULES)/opengl
WAYLANDEGL_DIR      = $(NC_APP_MODULES)/wayland_egl
NC_CNN_NETWORKS_PELEE_DIR = $(NC_APP_MODULES)/cnn/networks/pelee
NC_CNN_NETWORKS_YOLOV5_DIR = $(NC_APP_MODULES)/cnn/networks/yolov5
NC_CNN_NETWORKS_YOLOV8_DIR = $(NC_APP_MODULES)/cnn/networks/yolov8
NC_CNN_NETWORKS_UFLD_DIR = $(NC_APP_MODULES)/cnn/networks/ufld
NC_CNN_NETWORKS_TRICHIMERA_DIR = $(NC_APP_MODULES)/cnn/networks/trichimera

ifeq ($(USE_BYTETRACK), yes)
EIGEN_DIR           = $(3RD_PARTY_DIR)/eigen
BYTETRACK_CPP_DIR   = $(3RD_PARTY_DIR)/ByteTrack-cpp
endif
FREETYPE_DIR        = $(3RD_PARTY_DIR)/freetype
LIBPNG_DIR          = $(3RD_PARTY_DIR)/libpng
OPENCV_DIR          = $(3RD_PARTY_DIR)/opencv2

#############################################################

INC_DIR             += -I./
INC_DIR             += -I$(COMMON_DIR)
INC_DIR             += -I$(3RD_PARTY_DIR)
INC_DIR             += -I$(INILIB_DIR)
INC_DIR             += -I$(LIBDRM_DIR)/include

INC_DIR             += -I$(OPENGL_DIR)/
INC_DIR             += -I$(OPENGL_DIR)/include
INC_DIR             += -I$(OPENGL_DIR)/include/khronos/original
INC_DIR             += -I$(OPENGL_DIR)/include/wayland

INC_DIR             += -I$(NC_APP_MODULES)/
INC_DIR             += -I$(NC_APP_MODULES)/v4l2
INC_DIR             += -I$(NC_APP_MODULES)/utils
INC_DIR             += -I$(NC_APP_MODULES)/wrapper
INC_DIR             += -I$(NC_APP_MODULES)/vdma_cnn_drv
INC_DIR             += -I$(NC_APP_MODULES)/buffer_ctrl
INC_DIR             += -I$(NC_APP_MODULES)/gui
INC_DIR             += -I$(NC_APP_MODULES)/config

INC_DIR             += -I$(NC_CNN_DIR)
INC_DIR             += -I$(NC_CNN_CORE_DIR)
INC_DIR             += -I$(NC_CNN_NETWORKS_DIR)
INC_DIR             += -I$(NC_CNN_NETWORKS_PELEE_DIR)
INC_DIR             += -I$(NC_CNN_NETWORKS_YOLOV5_DIR)
INC_DIR             += -I$(NC_CNN_NETWORKS_YOLOV8_DIR)
INC_DIR             += -I$(NC_CNN_NETWORKS_UFLD_DIR)
INC_DIR             += -I$(NC_CNN_NETWORKS_TRICHIMERA_DIR)
INC_DIR             += -I$(SOIL_DIR)/include

INC_DIR             += -I$(WAYLANDEGL_DIR)/
INC_DIR             += -I$(NC_NEON_DIR)/
INC_DIR             += -I$(NC_OPENGL_DIR)/

ifeq ($(USE_BYTETRACK), yes)
INC_DIR             += -I$(EIGEN_DIR)/include/eigen3
INC_DIR             += -I$(BYTETRACK_CPP_DIR)/include
endif

INC_DIR             += -I$(FREETYPE_DIR)/include

ifeq ($(USE_8MP_VI), yes)
INC_DIR             += -I$(NC_APP_MODULES)/dmabuf_ctrl
INC_DIR             += -I$(NC_APP_MODULES)/dsr
endif

ifeq ($(USE_ADAS_LD), yes)
INC_DIR             += -I$(NC_APP_MODULES)/adas/ADAS_LD_Lib
endif
#############################################################

ifeq ($(ARCH), arm64)
CFLAGS_TEST         := -D_REENTRANT -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS_TEST         += -mcpu=cortex-a53+crypto
CFLAGS_TEST         += -DUSE_NEON
else ifeq ($(ARCH), arm)
CFLAGS_TEST         := --static
CFLAGS_TEST         += -mcpu=cortex-a53
CFLAGS_TEST         += -mfpu=neon-fp-armv8
CFLAGS_TEST         += -mfloat-abi=softfp
CFLAGS_TEST         += -DUSE_NEON
endif
CFLAGS_TEST         += -DBOL_ -rdynamic
CFLAGS_TEST         += -O2
CFLAGS_TEST         += -g -std=c++11
CFLAGS_TEST         += -fno-inline -fno-omit-frame-pointer
CFLAGS_TEST         += $(INC_DIR)
CFLAGS_TEST         += -fopenmp
ifeq ($(USE_BYTETRACK), yes)
CFLAGS_TEST         += -Wall -Wextra -Wformat=2 -Wpedantic -Wconversion -fstack-protector-all
# CFLAGS_TEST         += -Werror
else
CFLAGS_TEST         += -Wall -Wextra -Wformat=2 -Wpedantic -Wconversion -fstack-protector-all
CFLAGS_TEST         += -Werror
endif
CFLAGS_TEST         += -DAIWARE_DEVICE_SUPPORTED

ifeq ($(USE_8MP_VI), yes)
CFLAGS_TEST         += -DUSE_8MP_VI
endif

ifeq ($(USE_CNN_PELEEDET_NETWORK), yes)
CFLAGS_TEST         += -DUSE_CNN_PELEEDET_NETWORK
CFLAGS_TEST         += -DDETECT_NETWORK=0
ifeq ($(SHOW_PELEE_DETECT), yes)
CFLAGS_TEST         += -DSHOW_PELEE_DETECT
endif
endif

ifeq ($(USE_CNN_PELEESEG_NETWORK), yes)
CFLAGS_TEST         += -DUSE_CNN_PELEESEG_NETWORK
CFLAGS_TEST         += -DSEGMENT_NETWORK=4

ifeq ($(SHOW_PELEE_SEG), yes)
CFLAGS_TEST         += -DSHOW_PELEE_SEG
endif
endif

ifeq ($(USE_CNN_YOLOV5_NETWORK), yes)
CFLAGS_TEST         += -DUSE_CNN_YOLOV5_NETWORK
CFLAGS_TEST         += -DDETECT_NETWORK=0
ifeq ($(SHOW_YOLOV5_DETECT), yes)
CFLAGS_TEST         += -DSHOW_YOLOV5_DETECT
endif
endif

ifeq ($(USE_CNN_YOLOV8_NETWORK), yes)
CFLAGS_TEST         += -DUSE_CNN_YOLOV8_NETWORK
CFLAGS_TEST         += -DDETECT_NETWORK=0
ifeq ($(SHOW_YOLOV8_DETECT), yes)
CFLAGS_TEST         += -DSHOW_YOLOV8_DETECT
endif
endif

ifeq ($(USE_CNN_UFLD_NETWORK), yes)
CFLAGS_TEST         += -DUSE_CNN_UFLD_NETWORK
CFLAGS_TEST         += -DLANE_NETWORK=8

ifeq ($(SHOW_UFLD_LANE), yes)
CFLAGS_TEST         += -DSHOW_UFLD_LANE
endif
ifeq ($(USE_UFLD_TRACKING), yes)
CFLAGS_TEST         += -DUSE_UFLD_TRACKING
endif
ifeq ($(USE_UFLD_NETWORK_DEBUGGING), row)
CFLAGS_TEST         += -DUSE_UFLD_NETWORK_DEBUGGING
CFLAGS_TEST         += -DDBG_ROW_ANCHOR
else ifeq ($(USE_UFLD_NETWORK_DEBUGGING), col)
CFLAGS_TEST         += -DUSE_UFLD_NETWORK_DEBUGGING
CFLAGS_TEST         += -DDBG_COL_ANCHOR
endif
endif

ifeq ($(USE_CNN_TRICHIMERA_NETWORK), yes)
CFLAGS_TEST         += -DUSE_CNN_TRICHIMERA_NETWORK
ifeq ($(SHOW_TRI_CHIMERA), yes)
CFLAGS_TEST         += -DSHOW_TRI_CHIMERA
endif
ifeq ($(USE_UFLD_TRACKING), yes)
CFLAGS_TEST         += -DUSE_UFLD_TRACKING
endif
endif

CFLAGS_TEST         += -DUID_TSB_GUI=0

ifeq ($(USE_BYTETRACK), yes)
CFLAGS_TEST         += -DUSE_BYTETRACK
CFLAGS_TEST         += -DUSE_REMOVED_TRACK_LIMITED
# CFLAGS_TEST         += -DIGN_SPECIFIC_CLASS_TRACKING
endif

ifeq ($(USE_ADAS_LD), yes)
CFLAGS_TEST     += -DUSE_ADAS_LD
endif

# add link library
LIBRARY             += $(AIWARE_LIB_DIR)/lib_$(ARCH)/libaiware-runtime-selftestdata.a
LIBRARY             += $(AIWARE_LIB_DIR)/lib_$(ARCH)/libaiware-runtime-device-lib-c.a
LIBRARY             += $(AIWARE_LIB_DIR)/lib_$(ARCH)/libaiware-common-lib-c.so
LIBRARY             += $(AIWARE_LIB_DIR)/lib_$(ARCH)/libaiware-runtime-common-lib-c.so
LIBRARY             += $(AIWARE_LIB_DIR)/lib_$(ARCH)/libaiware-quant-reorder-lib-c.so

LIBRARY             += $(LIBDRM_DIR)/lib/libdrm.so $(LIBDRM_DIR)/lib/libc.so.6 $(LIBDRM_DIR)/lib/ld-linux-aarch64.so.1
LIBRARY             += $(LIBDRM_DIR)/lib/libdl.so.2 $(LIBDRM_DIR)/lib/libpthread.so.0 $(LIBDRM_DIR)/lib/librt.so.1

LIBRARY             += $(OPENGL_DIR)/lib_$(ARCH)/libEGL.so $(OPENGL_DIR)/lib_$(ARCH)/libEGL.so.1 $(OPENGL_DIR)/lib_$(ARCH)/libEGL.so.1.4.0
LIBRARY             += $(OPENGL_DIR)/lib_$(ARCH)/libffi.so $(OPENGL_DIR)/lib_$(ARCH)/libffi.so.8 $(OPENGL_DIR)/lib_$(ARCH)/libffi.so.8.1.0
LIBRARY             += $(OPENGL_DIR)/lib_$(ARCH)/libgbm.so $(OPENGL_DIR)/lib_$(ARCH)/libgbm.so.1 $(OPENGL_DIR)/lib_$(ARCH)/libgbm.so.1.0.0
LIBRARY             += $(OPENGL_DIR)/lib_$(ARCH)/libGLESv1_CM.so $(OPENGL_DIR)/lib_$(ARCH)/libGLESv1_CM.so.1 $(OPENGL_DIR)/lib_$(ARCH)/libGLESv1_CM.so.1.1.0
LIBRARY             += $(OPENGL_DIR)/lib_$(ARCH)/libGLESv2.so $(OPENGL_DIR)/lib_$(ARCH)/libGLESv2.so.2 $(OPENGL_DIR)/lib_$(ARCH)/libGLESv2.so.2.1.0
LIBRARY             += $(OPENGL_DIR)/lib_$(ARCH)/libmali.so $(OPENGL_DIR)/lib_$(ARCH)/libmali.so.0 $(OPENGL_DIR)/lib_$(ARCH)/libmali.so.0.44.0
LIBRARY             += $(OPENGL_DIR)/lib_$(ARCH)/libOpenCL.so $(OPENGL_DIR)/lib_$(ARCH)/libOpenCL.so.2 $(OPENGL_DIR)/lib_$(ARCH)/libOpenCL.so.2.1.0

LIBRARY             += $(OPENGL_DIR)/lib_$(ARCH)/libwayland-client.so $(OPENGL_DIR)/lib_$(ARCH)/libwayland-client.so.0 $(OPENGL_DIR)/lib_$(ARCH)/libwayland-client.so.0.20.0
LIBRARY             += $(OPENGL_DIR)/lib_$(ARCH)/libwayland-egl.so $(OPENGL_DIR)/lib_$(ARCH)/libwayland-egl.so.1 $(OPENGL_DIR)/lib_$(ARCH)/libwayland-egl.so.1.0.0
LIBRARY             += $(OPENGL_DIR)/lib_$(ARCH)/libwayland-server.so $(OPENGL_DIR)/lib_$(ARCH)/libwayland-server.so.0 $(OPENGL_DIR)/lib_$(ARCH)/libwayland-server.so.0.20.0
LIBRARY             += $(OPENGL_DIR)/lib_$(ARCH)/libwayland-cursor.so $(OPENGL_DIR)/lib_$(ARCH)/libwayland-cursor.so.0 $(OPENGL_DIR)/lib_$(ARCH)/libwayland-cursor.so.0.0.0

LIBRARY             += $(SOIL_DIR)/lib/libsoil.a
LIBRARY             += $(FREETYPE_DIR)/lib/libfreetype.so $(LIBPNG_DIR)/libpng16.so $(OPENCV_DIR)/lib/libz.so

ifeq ($(USE_ADAS_LD), yes)
LIBRARY             += $(NC_APP_MODULES)/adas/ADAS_LD_Lib/ADAS_LD_Lib.a
endif

LIBRARY             += -lm -lpthread -lrt

#############################################################

# Test App
CSRCS               := wayland_npu_app.c
CSRCS               += $(NC_APP_MODULES)/v4l2/v4l2_interface.c
CSRCS               += $(NC_APP_MODULES)/config/nc_cnn_config_parser.c
CSRCS               += $(NC_APP_MODULES)/utils/nc_utils.c
CSRCS               += $(INILIB_DIR)/ini.c
CSRCS               += $(NC_APP_MODULES)/config/nc_app_config_parser.c
CSRCS               += $(NC_CNN_CORE_DIR)/nc_cnn_aiware_runtime.c
CSRCS               += $(NC_CNN_CORE_DIR)/nc_cnn_communicator.c
CSRCS               += $(NC_CNN_CORE_DIR)/nc_cnn_worker_for_postprocess.c
CSRCS               += $(NC_CNN_CORE_DIR)/nc_cnn_anchor.c
ifeq ($(USE_CNN_PELEESEG_NETWORK), yes)
CSRCS               += $(NC_CNN_NETWORKS_DIR)/nc_cnn_segmentation_postprocess.c
endif
ifeq ($(USE_CNN_PELEEDET_NETWORK), yes)
CSRCS               += $(NC_CNN_NETWORKS_PELEE_DIR)/nc_cnn_pelee_postprocess.c
endif
ifeq ($(USE_CNN_YOLOV5_NETWORK), yes)
CSRCS               += $(NC_CNN_NETWORKS_YOLOV5_DIR)/nc_cnn_yolov5_postprocess.c
endif
ifeq ($(USE_CNN_YOLOV8_NETWORK), yes)
CSRCS               += $(NC_CNN_NETWORKS_YOLOV8_DIR)/nc_cnn_yolov8_postprocess.c
endif
ifeq ($(USE_CNN_UFLD_NETWORK), yes)
CSRCS               += $(NC_CNN_NETWORKS_UFLD_DIR)/nc_cnn_ufld_postprocess.c
endif
ifeq ($(USE_CNN_TRICHIMERA_NETWORK), yes)
CSRCS               += $(NC_CNN_NETWORKS_TRICHIMERA_DIR)/nc_cnn_trichimera_postprocess.c
endif
CSRCS               += $(NC_APP_MODULES)/buffer_ctrl/nc_ts_fsync_flipflop_buffers.c
CSRCS               += $(NC_APP_MODULES)/buffer_ctrl/nc_ts_fsync_circular_buffers.c

CSRCS              += $(WAYLANDEGL_DIR)/wayland_egl.c
CSRCS              += $(NC_OPENGL_DIR)/nc_opengl_interface.c
CSRCS              += $(NC_OPENGL_DIR)/nc_opengl_ttf_font.c
CSRCS              += $(NC_OPENGL_DIR)/nc_opengl_init.c
CSRCS              += $(NC_OPENGL_DIR)/nc_opengl_shader.c
CSRCS              += $(NC_NEON_DIR)/nc_neon.c

ifeq ($(USE_BYTETRACK), yes)
CSRCS               += $(NC_CNN_CORE_DIR)/nc_cnn_tracker.c
CPPSRCS             += $(NC_APP_MODULES)/wrapper/nc_bytetrack_wrapper.cpp
CPPSRCS             += $(BYTETRACK_CPP_DIR)/src/BYTETracker.cpp
CPPSRCS             += $(BYTETRACK_CPP_DIR)/src/KalmanFilter.cpp
CPPSRCS             += $(BYTETRACK_CPP_DIR)/src/Object.cpp
CPPSRCS             += $(BYTETRACK_CPP_DIR)/src/Rect.cpp
CPPSRCS             += $(BYTETRACK_CPP_DIR)/src/STrack.cpp
CPPSRCS             += $(BYTETRACK_CPP_DIR)/src/lapjv.cpp
endif

ifeq ($(USE_8MP_VI), yes)
CSRCS               += $(NC_APP_MODULES)/dmabuf_ctrl/nc_dmabuf_ctrl_helper.c
CSRCS               += $(NC_APP_MODULES)/dsr/nc_dsr_api.c
CSRCS               += $(NC_APP_MODULES)/dsr/nc_dsr_set.c
CSRCS               += $(NC_APP_MODULES)/dsr/nc_dsr_helper.c
endif

TARGET_TEST         := app_wayland_npu

all:
	${CXX} $(CFLAGS_TEST) $(CPPSRCS) $(CSRCS) -o $(TARGET_TEST) $(LIBRARY)

clean:
	rm -rf $(TARGET_TEST)
