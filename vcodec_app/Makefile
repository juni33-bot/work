#############################################################
#
# Makefile for NextChip APACHE6 Linux Video Codec Test Application
#
#############################################################

PRODUCT := WAVE521C

ARCH                ?= arm64
#ARCH                ?= arm
ifeq ($(ARCH), arm64)
CROSS_COMPILE       ?= aarch64-none-linux-gnu-
else ifeq ($(ARCH), arm)
CROSS_COMPILE       ?= arm-linux-gnueabi-
endif

TARGET_TEST         := app_vcodec

CC                  = ${CROSS_COMPILE}gcc
CXX                 = ${CROSS_COMPILE}g++

COMMON_DIR          = $(PWD)/../common
3RD_PARTY_DIR       = $(COMMON_DIR)/third_party
NC_APP_MODULES      = $(COMMON_DIR)/nc_app_modules
NC_VCODEC_DIR       = $(COMMON_DIR)/nc_video_codec
NC_STREAMER_DIR     = $(NC_APP_MODULES)/streamer
INILIB_DIR          = $(3RD_PARTY_DIR)/iniLib

INC_DIR             += -I./
INC_DIR             += -I$(3RD_PARTY_DIR)
INC_DIR             += -I$(NC_APP_MODULES)
INC_DIR             += -I$(NC_APP_MODULES)/utils
INC_DIR             += -I$(NC_APP_MODULES)/v4l2
INC_DIR             += -I$(NC_STREAMER_DIR)
INC_DIR             += -I$(NC_VCODEC_DIR)
INC_DIR             += -I$(INILIB_DIR)
INC_DIR             += -I$(NC_APP_MODULES)/config

ifeq ($(ARCH), arm64)
CFLAGS_TEST         :=
CFLAGS_TEST         += -mcpu=cortex-a53+crypto
CFLAGS_TEST         += -DUSE_NEON
else ifeq ($(ARCH), arm)
CFLAGS_TEST         := --static
CFLAGS_TEST         += -mcpu=cortex-a53
CFLAGS_TEST         += -mfpu=neon-fp-armv8
CFLAGS_TEST         += -mfloat-abi=softfp
CFLAGS_TEST         += -DUSE_NEON
endif
CFLAGS_TEST         += -DBOL_
CFLAGS_TEST         += -O2
CFLAGS_TEST         += -g -std=c++11
CFLAGS_TEST         += -fno-inline -fno-omit-frame-pointer
CFLAGS_TEST         += $(INC_DIR)
CFLAGS_TEST         += -D$(PRODUCT)
CFLAGS_TEST         += -DPLATFORM_LINUX
CFLAGS_TEST         += -D_FILE_OFFSET_BITS=64
CFLAGS_TEST         += -D_LARGEFILE_SOURCE
# CFLAGS_TEST         += -Wall -Wextra -Wformat=2 -Wpedantic -Wconversion -Werror -fstack-protector-all
CFLAGS_TEST         += -g -I. -Wl,--fatal-warning

# add link library
LIBRARY             += -lm -lpthread -lrt
LIBRARY             += $(NC_VCODEC_DIR)/lib_nc_vcodec.a

# Test App
CSRCS               := vcodec_app.c
CSRCS               += $(NC_APP_MODULES)/config/nc_vcodec_config_parser.c

# app module
CSRCS               += $(NC_APP_MODULES)/utils/nc_utils.c
CSRCS               += $(NC_APP_MODULES)/v4l2/v4l2_interface.c
CSRCS               += $(NC_STREAMER_DIR)/nc_streamer.c
CSRCS               += $(INILIB_DIR)/ini.c

all:
	${CXX} $(CFLAGS_TEST) $(CPPSRCS) $(CSRCS) -o $(TARGET_TEST) $(LIBRARY)

clean:
	rm -rf $(TARGET_TEST)
