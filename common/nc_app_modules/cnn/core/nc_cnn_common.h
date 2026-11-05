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
* @file    : nc_cnn_common.h
*
* @brief   : nc_cnn_common header
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2022.09.07
*
* @version : 1.0.0
********************************************************************************
* @note
* 2022.09.07 / 1.0.0 / Initial released.
*
********************************************************************************
*/

#ifndef __NC_CNN_COMMON_H__
#define __NC_CNN_COMMON_H__

#include <stdint.h>
#include <stdlib.h>

#define MAX_CNN_CLASS_CNT               (20)
#define MAX_CNN_RESULT_CNT_OF_CLASS     (100)

#define MAX_POINT_CNT_OF_LANE           (50)
#define MAX_LANE_DET_CNT                (10)

#define NPU_INPUT_WIDTH                 (640)
#define NPU_INPUT_HEIGHT                (384)

#define NPU_INPUT_DATA_SIZE             (NPU_INPUT_WIDTH * NPU_INPUT_HEIGHT)

typedef struct {
    int x;
    int y;
} stIntPoint;

////////////////////////////////
//      for draw              //
////////////////////////////////
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} stRGB24;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} stRGBA32;


typedef struct {
    int max_class_cnt;
    char** class_names;
    stRGB24* class_colors;
} stObjDrawInfo;

typedef struct {
    uint32_t width;
    uint32_t height;
    int max_class_cnt;
    char** class_names;
    stRGBA32* class_colors;
} stSegDrawInfo;

typedef struct {
    int max_lane_num;
    stRGB24* index_colors;
#ifdef USE_UFLD_NETWORK_DEBUGGING
    int* lane_anchor_info;
    float* row_anchor;
    float* col_anchor;
    int row_anchor_num;
    int col_anchor_num;
    float row_anchor_min;
    float row_anchor_max;
    float col_anchor_min;
    float col_anchor_max;
    int row_cell_num;
    int col_cell_num;
#endif
} stLaneDrawInfo;

////////////////////////////////
//      for segmentation      //
////////////////////////////////
// linked list for segmentation
typedef struct STSegInfo {
    stIntPoint pos;
    uint8_t val;
    struct STSegInfo *next;
} stSegInfo;


////////////////////////////////
//     for object detection   //
////////////////////////////////
typedef struct {
    float x;
    float y;
    float w;
    float h;
} stBBox;

typedef struct {
    float x;
    float y;
} stPoint;

typedef struct {
    stBBox bbox;
    int track_id; // -1, if not use tracker
    float prob;
} stObjInfo;

typedef struct {
    int class_id; // unique class id
    int obj_cnt;
    stObjInfo objs[MAX_CNN_RESULT_CNT_OF_CLASS];
} stClassObjs;

typedef struct {
    int class_id; // unique class id
    int class_dummy;
} stClassSegs;

#ifdef USE_UFLD_NETWORK_DEBUGGING
typedef struct {
    float x; // x coordinate of max_indices cell 
    float y; // y coordinate of max_indices cell
    float index_min;
    float index_max;

    stPoint final_point;
} stUFLD_dbg_info;
#endif

typedef struct {
    stPoint point[MAX_POINT_CNT_OF_LANE];
    int point_cnt;
    int lane_class;
#ifdef USE_UFLD_NETWORK_DEBUGGING
    stUFLD_dbg_info ufld_dbg_info[MAX_POINT_CNT_OF_LANE];
    int dbg_info_cnt;
#endif
} stLaneDet;

typedef struct {
    // uint64_t timestamp;
    stClassObjs class_objs[MAX_CNN_CLASS_CNT];
    stClassSegs class_segs;
    void* seg;
    stLaneDet lane_det[MAX_LANE_DET_CNT];
} stCnnPostprocessingResults;

#endif // __NC_CNN_COMMON_H__
