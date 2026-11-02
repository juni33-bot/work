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
* @file    : nc_cnn_yolov5_postprocess.h
*
* @brief   : nc_cnn_yolov5_postprocess header
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


#ifndef __NC_CNN_YOLOV5_POSTPROCESS_H__
#define __NC_CNN_YOLOV5_POSTPROCESS_H__

#include "nc_cnn_aiware_runtime.h"

extern int nc_postprocess_yolov5_inference_result(unsigned int canvas_w, unsigned int canvas_h,
                                                        struct inference_result_msg *inference_res);

#endif // __NC_CNN_YOLOV5_POSTPROCESS_H__
