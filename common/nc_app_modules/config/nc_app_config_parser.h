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
* @file    : nc_app_config_parser.h
*
* @brief   : nc app config parser header
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2022.08.30
*
* @version : 1.0.0
********************************************************************************
* @note
* 2022.08.30 / 1.0.0 / Initial released. (bwryu@nextchip)
* 
********************************************************************************
*/ 

#ifndef __NC_APP_CONFIG_PARSER_H__
#define __NC_APP_CONFIG_PARSER_H__

/*
********************************************************************************
               TYPEDEFS
********************************************************************************
*/

#define MAX_CFG_FILE_NAME_LEN 128

typedef struct {
    int   version;
    int   is_serdes;
    char  isp_data_file[MAX_CFG_FILE_NAME_LEN];
    char  ldc_lut_file[MAX_CFG_FILE_NAME_LEN];
} stConfig_ini;

extern int nc_app_config_parse (char* full_path, stConfig_ini* config);

#endif // #ifndef __NC_APP_CONFIG_PARSER_H__
