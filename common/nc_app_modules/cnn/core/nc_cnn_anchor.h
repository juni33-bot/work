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
* @file    : nc_cnn_anchor.h
*
* @brief   : nc_cnn_anchor header
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2022.11.10
*
* @version : 1.0.0
********************************************************************************
* @note
* 2022.11.10 / 1.0.0 / Initial released.
*
********************************************************************************
*/


#ifndef __NC_CNN_ANCHOR_H__
#define __NC_CNN_ANCHOR_H__

#include "nc_cnn_common.h"

/***************************************************************
 **************** for anchor object detection ******************
 ***************************************************************/
enum {
    OBJ_CLASS_ID_0,
    OBJ_CLASS_ID_1,
    OBJ_CLASS_ID_2,
    OBJ_CLASS_ID_3,
    OBJ_CLASS_ID_4,
    OBJ_CLASS_ID_5,
    OBJ_CLASS_ID_6,
    OBJ_CLASS_ID_7,
    OBJ_CLASS_ID_8,
    OBJ_CLASS_ID_9,
    MAX_CLASS_ID_CNT
};

#define BOX_MAX_NUM                 9999

typedef struct detection {
    stBBox bbox;
    float prob[MAX_CLASS_ID_CNT]; //classes
    float objectness;
} detection;

typedef struct {
    uint8_t *max_indices;
    uint8_t *valid_anchor;
} lane_data;

extern float nc_Logistic_activate(float x);
extern void nc_Nms_box_yolov8(detection *dets, int *total, int classes, float thresh);
extern void nc_Nms_box(detection *dets, int *total, int classes, float thresh);
extern void nc_Nms_box_by_class(detection *dets, int *total, int classes, float thresh);

#endif // __NC_CNN_ANCHOR_H__
