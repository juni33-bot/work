#############################################################
#
# Makefile for NextChip APACHE6 Linux Drvier Test Application
#
#############################################################

#############################################################
# pre-set

#############################################################
# adjust-set

#############################################################


ARCH                ?= arm64
#ARCH                ?= arm
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
MATRIX_DIR          = $(THIS_DIR)/matrix
COMMON_DIR          = $(THIS_DIR)/../common
3RD_PARTY_DIR       = $(COMMON_DIR)/third_party
NC_APP_MODULES      = $(COMMON_DIR)/nc_app_modules
WAYLANDEGL_DIR      = $(NC_APP_MODULES)/wayland_egl
NC_OPENGL_DIR       = $(NC_APP_MODULES)/opengl
LIBDRM_DIR          = $(3RD_PARTY_DIR)/libdrm
OPENGL_DIR          = $(3RD_PARTY_DIR)/opengl
SOIL_DIR            = $(3RD_PARTY_DIR)/soil

######################## makefile include ###################
# include $(OPENGL_TPI_DIR)/Makefile

#############################################################

INC_DIR             += -I./
INC_DIR             += -I$(COMMON_DIR)
INC_DIR             += -I$(3RD_PARTY_DIR)
INC_DIR             += -I$(INILIB_DIR)
INC_DIR             += -I$(LIBDRM_DIR)/include
INC_DIR             += -I$(NC_APP_MODULES)/utils

INC_DIR             += -I$(OPENGL_DIR)/
INC_DIR             += -I$(OPENGL_DIR)/include
INC_DIR             += -I$(OPENGL_DIR)/include/khronos/original
INC_DIR             += -I$(OPENGL_DIR)/include/wayland

INC_DIR             += -I$(SOIL_DIR)/include
INC_DIR             += -I$(MATRIX_DIR)/
INC_DIR             += -I$(WAYLANDEGL_DIR)/
INC_DIR             += -I$(NC_OPENGL_DIR)/

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
CFLAGS_TEST         += -O0
CFLAGS_TEST         += -g -std=c++11
#CFLAGS_TEST         += -pg
CFLAGS_TEST         += -fno-inline -fno-omit-frame-pointer
CFLAGS_TEST         += $(INC_DIR)
CFLAGS_TEST         += -Wall -Wextra -Wformat=2 -Wpedantic -Wconversion -Werror -fstack-protector-all

#############################################################

# add link library
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

LIBRARY             += -lm -lpthread -lrt

#############################################################

# Test App
CSRCS               := wayland_egl_app.c

FCSRCS              += $(WAYLANDEGL_DIR)/wayland_egl.c
FCSRCS              += $(MATRIX_DIR)/matrix.c
FCSRCS              += $(NC_APP_MODULES)/utils/nc_utils.c
FCSRCS              += $(NC_OPENGL_DIR)/nc_opengl_init.c
FCSRCS              += $(NC_OPENGL_DIR)/nc_opengl_shader.c

TARGET_TEST         := app_wayland_egl


#############################################################

# CPPSRCS             := $(NC_APP_MODULES)/wrapper/nc_opencv_wrapper.cpp

#############################################################

# OBJS                := $(CSRCS:.c=.o) $(CPPSRCS:.cpp=.o)

#############################################################

all:
	${CXX} $(CFLAGS_TEST) $(CPPSRCS) $(CSRCS) $(FCSRCS) -o $(TARGET_TEST) $(LIBRARY)

clean:
	rm -rf $(TARGET_TEST)

