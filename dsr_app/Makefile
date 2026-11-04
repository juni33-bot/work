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

TARGET_TEST         := app_dsr

CC                  = ${CROSS_COMPILE}gcc
CXX                 = ${CROSS_COMPILE}g++

#############################################################
THIS_DIR			= .
COMMON_DIR          = $(THIS_DIR)/../common
DSR_APP_DIR	    = $(THIS_DIR)

3RD_PARTY_DIR       = $(COMMON_DIR)/third_party
NC_APP_MODULES      = $(COMMON_DIR)/nc_app_modules
INILIB_DIR          = $(3RD_PARTY_DIR)/iniLib
LIBDRM_DIR          = $(3RD_PARTY_DIR)/libdrm
OPENGL_DIR          = $(3RD_PARTY_DIR)/opengl


######################## makefile include ###################
# include $(OPENGL_TPI_DIR)/Makefile


#############################################################

INC_DIR             += -I./
INC_DIR             += -I$(COMMON_DIR)
INC_DIR             += -I$(3RD_PARTY_DIR)
INC_DIR             += -I$(INILIB_DIR)
INC_DIR		    += -I$(NC_APP_MODULES)
INC_DIR		    += -I$(NC_APP_MODULES)/dmabuf_ctrl
INC_DIR		    += -I$(NC_APP_MODULES)/v4l2
INC_DIR		    += -I$(NC_APP_MODULES)/utils
INC_DIR		    += -I$(NC_APP_MODULES)/dsr
INC_DIR             += -I$(OPENGL_DIR)/include
INC_DIR             += -I$(OPENGL_DIR)/include/khronos/original
INC_DIR             += -I$(DSR_APP_DIR)
INC_DIR             += -I$(LIBDRM_DIR)/include

#############################################################

ifeq ($(ARCH), arm64)
CFLAGS_TEST         := -D_REENTRANT -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS_TEST         += -mcpu=cortex-a53+crypto
CFLAGS_TEST         += -fopenmp
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
CFLAGS_TEST         += -fpermissive
CFLAGS_TEST         += -Wall -Wextra -Wformat=2 -Wpedantic -Wconversion -Werror -fstack-protector-all

#############################################################

# add link library
LIBRARY				+= -lm -lpthread -lrt

#############################################################

# Test App
CSRCS               := nc_dsr_app.c
CSRCS               += $(NC_APP_MODULES)/dmabuf_ctrl/nc_dmabuf_ctrl_helper.c
CSRCS	    	    += $(NC_APP_MODULES)/dsr/nc_dsr_api.c
CSRCS	    	    += $(NC_APP_MODULES)/dsr/nc_dsr_set.c
CSRCS               += $(NC_APP_MODULES)/dsr/nc_dsr_helper.c
CSRCS               += $(NC_APP_MODULES)/v4l2/v4l2_interface.c
# Integration code

#############################################################

# CPPSRCS             := $(NC_APP_MODULES)/wrapper/nc_opencv_wrapper.cpp

#############################################################

# OBJS                := $(CSRCS:.c=.o) $(CPPSRCS:.cpp=.o)

#############################################################

all:
	${CXX} $(CFLAGS_TEST) $(CPPSRCS) $(CSRCS) $(FCSRCS) -o $(TARGET_TEST) $(LIBRARY)

clean:
	rm -rf $(TARGET_TEST)
