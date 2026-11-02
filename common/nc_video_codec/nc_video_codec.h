/**
********************************************************************************
* Copyright (C) 2021 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : nc_video_codec.h
*
* @brief   : nc_video_codec header
*
* @author  : Software Development Team.  NextChip Inc.
*
* @date    : 2024.04.23.
*
* @version : 1.0.0
********************************************************************************
* @note
* 04.23.2024 / 1.0.0 / Initial released.
*
********************************************************************************
*/

#ifndef __NC_VIDEO_CODEC_H__
#define __NC_VIDEO_CODEC_H__

/*
********************************************************************************
*               TYPEDEFS
********************************************************************************
*/

typedef uint64_t            Uint64;
typedef uint32_t            Uint32;
typedef uint16_t            Uint16;
typedef uint8_t             Uint8;

typedef int64_t             int64;
typedef int32_t             int32;
typedef int16_t             int16;
typedef int8_t              int8;

typedef void (*callback_t)(Uint8*, Uint32);
typedef void (*callback_t2)(void);

typedef enum EN_CODEC_TYPE{
    AVC = 0,
    HEVC = 12
} CODEC_TYPE;

typedef enum {
    RC_MODE_VBR = 0,
    RC_MODE_CBR = 1,
} RC_MODE;

typedef enum EN_CODEC_ERR_STATE{
    CODEC_SUCCESS,

    // for encoder
    ERR_ENC_INVALID_PRODUCT_ID,
    ERR_ENC_LOAD_FIRMWARE,
    ERR_ENC_INVALID_PARAM,
    ERR_ENC_CREATE_EVENT_LISTNER,
    ERR_ENC_CREATE_PIPELINE,
    ERR_ENC_FAIL_TO_OPEN,
    ERR_ENC_SEND_BUF,
    ERR_ENC_RECEIVE_BUF,

    // for decoder
    ERR_DEC_INVALID_PRODUCT_ID,
    ERR_DEC_LOAD_FIRMWARE,
    ERR_DEC_INVALID_PARAM,
    ERR_DEC_CREATE_EVENT_LISTNER,
    ERR_DEC_CREATE_PIPELINE,
    ERR_DEC_FAIL_TO_OPEN,
    ERR_DEC_SEND_BUF,
    ERR_DEC_RECEIVE_BUF,

    CODEC_ERR_MAX_CNT
} CODEC_ERR_STATE;

typedef struct ST_ENCParameter{
    Uint32           enc_width;
    Uint32           enc_height;
    Uint32           enc_qp;
    Uint32           enc_minqp;
    Uint32           enc_maxqp;
    Uint32           enc_bitrate;
    RC_MODE          enc_mode;
    Uint32           enc_framerate;
    Uint32           enc_gop;
    CODEC_TYPE       enc_codec_type;
} ENCParameter;

typedef struct ST_DECParameter{
    CODEC_TYPE       dec_codec_type;
    Uint32           dec_framerate;
} DECParameter;

typedef enum {
    RUN_ENC_NC_STREAM = 1,
    RUN_ENC_FILE_DUMP,
    RUN_ENC_NC_RTSP,
    RUN_DEC_FILE_DUMP,
    RUN_INVALID_OP_MODE,
} output_mode_t;
/*
********************************************************************************
*               DEFINES
********************************************************************************
*/

/*
********************************************************************************
*               FUNCTION DEFINITIONS
********************************************************************************
*/

CODEC_ERR_STATE nc_init_encoder(ENCParameter *get_param);
CODEC_ERR_STATE nc_start_encoder(void);
void nc_stop_encoder(void);

CODEC_ERR_STATE nc_init_decoder(DECParameter *get_param);
CODEC_ERR_STATE nc_start_decoder(void);
void nc_stop_decoder(void);

void nc_display_libncvcodec_info(void);

void nc_unlock_encoding_done(void);

CODEC_ERR_STATE nc_send_buf_to_encode(Uint8 *ptr_video_buf, int32 auto_free);
CODEC_ERR_STATE nc_send_buf_to_decode(Uint8 *ptr_video_buf, Uint32 size, int32 eof, int32 auto_free);

void nc_register_encode_callback(callback_t cb);
void nc_register_decode_callback(callback_t cb);
void nc_register_last_frame_decoded_callback(callback_t2 cb);

#endif // #ifndef __NC_VIDEO_CODEC_H__