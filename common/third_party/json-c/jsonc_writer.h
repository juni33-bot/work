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
* @file    : jsonc_writer.h 
*
* @brief   : jsonc_writer header
*
* @author  : AI SW team.  NextChip Inc.  
*
* @date    : 2024.03.18.
*
* @version : 1.0.0
********************************************************************************
* @note
* 2024.03.18 / 1.0.0 / Initial released.
* 
********************************************************************************
*/ 
#ifndef NC_JSONC_WRITER_H
#define NC_JSONC_WRITER_H

#include <stdlib.h>

#define CHANNEL_CNT         (10) /* class num */
#define VALID_CHANNEL_CNT   (10)  /* save class num */
#define BOX_CNT             (512)

typedef struct _box{
    int x;
    int y;
    int w;
    int h;
    float s;
}box_t;

typedef struct _chn{
    int box_cnt;
    box_t box[BOX_CNT];
}chn_t;

typedef struct frame_info{
    int cnt;
    int total_file_cnt;
    chn_t chn[CHANNEL_CNT];
}frame_info_t;

int make_json(frame_info_t *frame, char *result_folder_name);
int make_json_separate(frame_info_t *frame, char *result_folder_name);
#endif
