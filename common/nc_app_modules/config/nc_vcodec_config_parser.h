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
* @file    : nc_vcodec_config_parser.c.h
*
* @brief   : nc vcodec config parser header
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2025.10.30
*
* @version : 1.0.0
********************************************************************************
* @note
* 2025.10.30 / 1.0.0 / Initial released. (yeulo@nextchip)
* 
********************************************************************************
*/ 

#ifndef __NC_VOCDEC_CONFIG_PARSER_H__
#define __NC_VOCDEC_CONFIG_PARSER_H__


/*
********************************************************************************
               includes
********************************************************************************
*/

#include "nc_video_codec.h"

/*
********************************************************************************
               TYPEDEFS
********************************************************************************
*/

#define MAX_CFG_FILE_NAME_LEN 128

typedef struct
{
    output_mode_t mode;
    ENCParameter enc_config;
    DECParameter dec_config;
} vcodec_app_config_t;

extern int nc_vcodec_app_config_parse (char* full_path, vcodec_app_config_t* config);

#endif // #ifndef __NC_VOCDEC_CONFIG_PARSER_H__
