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
* @file    : nc_app_config_parser.c
*
* @brief   : implementation of nc app config parser
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2022.08.30
*
* @version : 1.0.0
********************************************************************************
* @note
* 2022.08.30 / 1.0.0 / Initial released.
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
#include "nc_app_config_parser.h"

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

static int nc_ini_handler(void* user, const char* section,
                          const char* name, const char* value)
{
    stConfig_ini* pConfig = (stConfig_ini*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("soc", "version")) {
        pConfig->version = 0;
        pConfig->version = atoi(value);
    } else if (MATCH("isp", "serdes")) {
        pConfig->is_serdes = 0;
        pConfig->is_serdes = atoi(value);
    } else if (MATCH("isp", "file")) {
        memset(pConfig->isp_data_file, 0, MAX_CFG_FILE_NAME_LEN);
        strncpy(pConfig->isp_data_file, value, MAX_CFG_FILE_NAME_LEN -1);
        pConfig->isp_data_file[MAX_CFG_FILE_NAME_LEN - 1] = '\0';
    } else if (MATCH("isp", "lut")) {
        memset(pConfig->ldc_lut_file, 0, MAX_CFG_FILE_NAME_LEN);
        strncpy(pConfig->ldc_lut_file, value, MAX_CFG_FILE_NAME_LEN-1);
        pConfig->ldc_lut_file[MAX_CFG_FILE_NAME_LEN - 1] = '\0';
    } else {
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

int nc_app_config_parse (char* full_path, stConfig_ini* config)
{
    memset(config, 0, sizeof(stConfig_ini));
    /* Load Config from ini file */
    if (ini_parse(full_path, nc_ini_handler, config) < 0) {
        printf("Can't load %s\n", full_path);
        return -1;
    }

    return 0;
}
