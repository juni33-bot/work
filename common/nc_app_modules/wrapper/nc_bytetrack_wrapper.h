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
* @file    : nc_bytetrack_wrapper.h
*
* @brief   : nc_bytetrack_wrapper header
*
* @author  : SoC SW team.  NextChip Inc.  
*
* @date    : 2022.05.27.
*
* @version : 1.0.0
********************************************************************************
* @note
* 2022.05.27 / 1.0.0 / Initial released.
* 
********************************************************************************
*/ 

#ifndef BYTETRACK_WRAPPER_H
#define BYTETRACK_WRAPPER_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "nc_cnn_common.h"

typedef struct {
    int class_id; // unique class id
    float x;
    float y;
    float w;
    float h;
    float prob;
} stCnnDetectedObjInfo;

// typedef struct {
//     int track_id; // tracked id by BYTETRACK
//     float x;
//     float y;
//     float w;
//     float h;
// } stByteTrackedObjInfo;

#ifdef __cplusplus
extern "C" int nc_init_bytetracker(int fps, int track_buffer, uint16_t *class_ids_arr, int class_ids_cnt, int cam_ch);
extern "C" void nc_destroy_byettracker(int cam_ch);
extern "C" void nc_set_bytetracker_thresh_values(uint16_t class_id, float track_thresh, float high_thresh, float match_thresh, int cam_ch);
extern "C" int nc_update_bytetracked_objs_per_frame(uint16_t class_id, stCnnDetectedObjInfo *in_bboxes, uint32_t in_bbox_cnt, stObjInfo *out_bboxes, uint32_t *out_bbox_cnt, int cam_ch);
#else
extern int nc_init_bytetracker(int fps, int track_buffer, uint16_t *class_ids_arr, int class_ids_cnt, int cam_ch);
extern void nc_destroy_byettracker(int cam_ch);
extern void nc_set_bytetracker_thresh_values(uint16_t class_id, float track_thresh, float high_thresh, float match_thresh, int cam_ch);
extern int nc_update_bytetracked_objs_per_frame(uint16_t class_id, stCnnDetectedObjInfo *in_bboxes, uint32_t in_bbox_cnt, stObjInfo *out_bboxes, uint32_t *out_bbox_cnt, int cam_ch);
#endif

#endif // #ifndef BYTETRACK_WRAPPER_H
