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
* @file    : nc_cnn_yolov8_postprocess.h
*
* @brief   : nc_cnn_yolov8_postprocess header
*
* @author  : AI SW team.  NextChip Inc.
*
* @date    : 2024.04.25
*
* @version : 1.0.0
********************************************************************************
* @note
* 2024.04.25 / 1.0.0 / Initial released.
*
********************************************************************************
*/


#ifndef __NC_CNN_YOLOV8_POSTPROCESS_H__
#define __NC_CNN_YOLOV8_POSTPROCESS_H__

#include "nc_cnn_aiware_runtime.h"

extern int nc_postprocess_yolov8_inference_result(unsigned int canvas_w, unsigned int canvas_h,
                                                        struct inference_result_msg *inference_res);

#endif // __NC_CNN_YOLOV8_POSTPROCESS_H__
