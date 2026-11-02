#############################################################
#
# Makefile for NextChip APACHE6 Linux video codec library
#
#############################################################

LIB_NAME            = lib_nc_vcodec.a
PRODUCT             := WAVE521C

APP_BUILD_DATE      := \"$(shell date +%Y/%m/%d)\"
APP_BUILD_TIME      := \"$(shell date +%H:%M:%S)\"
GIT_HEAD_HASH       := \"$(shell git rev-parse --short HEAD)\"
APP_VER_MAJOR       := 1
APP_VER_MINOR1      := 0
APP_VER_MINOR2      := 2

define add_define
    DEFINES         += -D$(1)$(if $(value $(1)),=$(value $(1)),)
endef

$(eval $(call add_define,APP_BUILD_DATE))
$(eval $(call add_define,APP_BUILD_TIME))
$(eval $(call add_define,GIT_HEAD_HASH))
$(eval $(call add_define,APP_VER_MAJOR))
$(eval $(call add_define,APP_VER_MINOR1))
$(eval $(call add_define,APP_VER_MINOR2))
## for debug only for AVC CBR ##
# $(eval $(call add_define,SUPPORT_SW_UART_V2))

ARCH                ?= arm64
CROSS_COMPILE       ?= aarch64-none-linux-gnu-
CC                  = ${CROSS_COMPILE}gcc
CXX                 = ${CROSS_COMPILE}g++

VDI_DIR             = ./vdi
COMPONENT_DIR       = ./component
NC_VCODEC_DIR       = ./

NC_APP_MODULES      = ../nc_app_modules
NC_STREAMER_DIR     = $(NC_APP_MODULES)/streamer

INC_DIR             += -I$(NC_VCODEC_DIR)
INC_DIR             += -I$(NC_VCODEC_DIR)/vpuapi
INC_DIR             += -I$(NC_VCODEC_DIR)/component/helper
INC_DIR             += -I$(NC_VCODEC_DIR)/component/helper/misc
INC_DIR             += -I$(NC_VCODEC_DIR)/component/component_interface
INC_DIR             += -I$(NC_VCODEC_DIR)/vdi
INC_DIR             += -I$(NC_VCODEC_DIR)/component/component_encoder
INC_DIR             += -I$(NC_VCODEC_DIR)/component/component_decoder
INC_DIR             += -I$(NC_VCODEC_DIR)/component/Debug
INC_DIR             += -I$(NC_STREAMER_DIR)
INC_DIR             += -I$(NC_APP_MODULES)/v4l2
INC_DIR             += -I$(NC_APP_MODULES)/utils

SRCS                += $(COMPONENT_DIR)/component_encoder/component_enc_encoder.c
SRCS                += $(COMPONENT_DIR)/component_encoder/component_enc_feeder.c
SRCS                += $(COMPONENT_DIR)/component_encoder/component_enc_reader.c
SRCS                += $(COMPONENT_DIR)/component_encoder/encoder_listener.c
SRCS                += $(COMPONENT_DIR)/component_decoder/component_dec_decoder.c
SRCS                += $(COMPONENT_DIR)/component_decoder/component_dec_feeder.c
SRCS                += $(COMPONENT_DIR)/component_decoder/component_dec_renderer.c
SRCS                += $(COMPONENT_DIR)/component_decoder/decoder_listener.c
SRCS                += $(COMPONENT_DIR)/component_interface/cnm_app.c
SRCS                += $(COMPONENT_DIR)/component_interface/cnm_task.c
SRCS                += $(COMPONENT_DIR)/component_interface/component.c
SRCS                += $(COMPONENT_DIR)/helper/main_helper.c
SRCS                += $(COMPONENT_DIR)/helper/vpuhelper.c
SRCS                += $(COMPONENT_DIR)/helper/bitstream/bitstreamfeeder.c
SRCS                += $(COMPONENT_DIR)/helper/bitstream/bsfeeder_fixedsize_impl.c
SRCS                += $(COMPONENT_DIR)/helper/bitstream/bsfeeder_size_plus_es_impl.c
SRCS                += $(COMPONENT_DIR)/helper/bitstream/bitstreamreader.c
SRCS                += $(COMPONENT_DIR)/helper/comparator/bin_comparator_impl.c
SRCS                += $(COMPONENT_DIR)/helper/comparator/comparator.c
SRCS                += $(COMPONENT_DIR)/helper/comparator/md5_comparator_impl.c
SRCS                += $(COMPONENT_DIR)/helper/comparator/yuv_comparator_impl.c
SRCS                += $(COMPONENT_DIR)/helper/misc/cfgParser.c
SRCS                += $(COMPONENT_DIR)/helper/misc/cnm_video_helper.c
SRCS                += $(COMPONENT_DIR)/helper/misc/container.c
SRCS                += $(COMPONENT_DIR)/helper/misc/datastructure.c
SRCS                += $(COMPONENT_DIR)/helper/misc/debug.c
SRCS                += $(COMPONENT_DIR)/helper/yuv/yuvfeeder.c
SRCS                += $(COMPONENT_DIR)/helper/yuv/yuvLoaderfeeder.c
SRCS                += $(COMPONENT_DIR)/helper/yuv/yuvCfbcfeeder.c
SRCS                += $(COMPONENT_DIR)/helper/misc/bw_monitor.c
SRCS                += $(COMPONENT_DIR)/helper/misc/pf_monitor.c
SRCS                += $(VDI_DIR)/vdi.c
SRCS                += $(VDI_DIR)/vdi_osal.c
SRCS                += $(VDI_DIR)/vdi_debug.c
SRCS                += $(NC_VCODEC_DIR)/vpuapi/product.c
SRCS                += $(NC_VCODEC_DIR)/vpuapi/vpuapifunc.c
SRCS                += $(NC_VCODEC_DIR)/vpuapi/vpuapi.c
SRCS                += $(NC_VCODEC_DIR)/vpuapi/wave5.c

CFLAGS              += -mcpu=cortex-a53+crypto
CFLAGS              += -DUSE_NEON
CFLAGS              += -DBOL_
CFLAGS              += -O2
CFLAGS              += -g -std=c++11
CFLAGS              += -fno-inline -fno-omit-frame-pointer
CFLAGS              += $(INC_DIR)
CFLAGS              += -DUID_TSB_GUI=0
CFLAGS              += -D$(PRODUCT)
CFLAGS              += -DPLATFORM_LINUX
CFLAGS              += -D_FILE_OFFSET_BITS=64
CFLAGS              += -D_LARGEFILE_SOURCE
CFLAGS              += -Wall -Wextra -Wformat=2 -Wpedantic -Wconversion -Werror -fstack-protector-all
CFLAGS              += -g -I. -Wl,--fatal-warning

all: $(LIB_NAME)

$(LIB_NAME): $(SRCS:.c=.o)
	ar rcs $@ $^
	rm -f $(SRCS:.c=.o)

%.o: %.c
	$(CXX) $(CFLAGS) $(DEFINES) -c $< -o $@

clean:
	rm -f $(SRCS:.c=.o) $(LIB_NAME)