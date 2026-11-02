/**
********************************************************************************
* Copyright (C) 2022 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************
*/
/**
********************************************************************************
* @file    : nc_vcodec_config_parser.c
*
* @brief   : implementation of nc app config parser
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2025.11.07
*
* @version : 1.0.0
********************************************************************************
* @note
* 2025.11.07 / 1.0.0 / Initial released.
*
********************************************************************************
*/

/*
********************************************************************************
*               INCLUDES
********************************************************************************
*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "iniLib/ini.h"
#include "nc_vcodec_config_parser.h"
#include "utils/nc_utils.h"

/*
********************************************************************************
*               DEFINES
********************************************************************************
*/

#define MATCH(s, n) (strcmp((section), (s)) == 0 && strcmp((name), (n)) == 0)

/*
********************************************************************************
*               FUNCTION DEFINITIONS
********************************************************************************
*/

static RC_MODE str_to_rc_mode(const char* str) {
    if (strcasecmp(str, "cbr") == 0) return RC_MODE_CBR;
    if (strcasecmp(str, "vbr") == 0) return RC_MODE_VBR;
    return RC_MODE_CBR;
}

static CODEC_TYPE str_to_codec(const char* str) {
    if (strcasecmp(str, "h264") == 0) return AVC;
    if (strcasecmp(str, "h265") == 0) return HEVC;
    return HEVC;
}

static output_mode_t str_to_mode(const char *str){
    if (!str) return RUN_INVALID_OP_MODE;
    if (strcasecmp(str,"encoder_stream") == 0) return RUN_ENC_NC_STREAM;
    if (strcasecmp(str,"encoder_dump") == 0)   return RUN_ENC_FILE_DUMP;
    if (strcasecmp(str,"rtsp") == 0)           return RUN_ENC_NC_RTSP;
    if (strcasecmp(str,"decoder") == 0)        return RUN_DEC_FILE_DUMP;
    return RUN_ENC_NC_RTSP;
}

static int nc_enc_ini_handler(void* user, const char* section, const char* name, const char* value) {
    vcodec_app_config_t* pConfig = (vcodec_app_config_t*)user;

    /* app */
    if (MATCH("app", "mode"))
    {
        output_mode_t op_mode;
        op_mode = str_to_mode(value);
        
        if (op_mode == RUN_INVALID_OP_MODE)
        {
            fprintf(stderr, "[ERR] invalid [app].mode='%s'\n", value);
            return 1; // or keep default
        }
        pConfig->mode = op_mode;
    }
 
    /* encoder */
    if (MATCH("encoder", "width"))         {pConfig->enc_config.enc_width = ALIGN_DOWN(atoi(value), 8);}
    else if (MATCH("encoder", "height"))   {pConfig->enc_config.enc_height = ALIGN_DOWN(atoi(value), 8);}
    else if (MATCH("encoder", "rc_mode"))  {pConfig->enc_config.enc_mode = str_to_rc_mode(value);}
    else if (MATCH("encoder", "codec"))    {pConfig->enc_config.enc_codec_type = str_to_codec(value);}
    else if (MATCH("encoder", "fps"))      {pConfig->enc_config.enc_framerate = atoi(value);}
    else if (MATCH("encoder", "bitrate"))  {pConfig->enc_config.enc_bitrate = atoi(value);}
    else if (MATCH("encoder", "gop"))      {pConfig->enc_config.enc_gop = atoi(value);}
    else if (MATCH("encoder", "qp_start")) {pConfig->enc_config.enc_qp = atoi(value);}
    else if (MATCH("encoder", "qp_min"))   {pConfig->enc_config.enc_minqp = atoi(value);}
    else if (MATCH("encoder", "qp_max"))   {pConfig->enc_config.enc_maxqp = atoi(value);}
    
    /* decoder */
    if (MATCH("decoder","output_codec_type")){ pConfig->dec_config.dec_codec_type = str_to_codec(value);}
    else if (MATCH("decoder","fps"))         { pConfig->dec_config.dec_framerate = atoi(value);}
       
    return 1;
}

int nc_vcodec_app_config_parse (char* full_path, vcodec_app_config_t* config)
{
    /* default setting */
    memset(config, 0, sizeof(vcodec_app_config_t));
    
    if(ini_parse(full_path, nc_enc_ini_handler, config) < 0) 
    {
        printf("Can't load %s\n", full_path);
        return -1;
    }
    
    return 0;
}
