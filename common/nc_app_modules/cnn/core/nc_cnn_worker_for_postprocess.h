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
* @file    : nc_cnn_worker_for_postprocess.h
*
* @brief   : nc_cnn_worker_for_postprocess header
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2022.11.07
*
* @version : 1.0.0
********************************************************************************
* @note
* 2022.11.07 / 1.0.0 / Initial released.
*
********************************************************************************
*/

#ifndef __NC_CNN_WORKER_FOR_POSTPROCESS_H__
#define __NC_CNN_WORKER_FOR_POSTPROCESS_H__

#include <stdint.h>

typedef struct {
    unsigned int target_width;
    unsigned int target_height;
} cnn_postprocess_arg;

// define type for postprocess&draw function pointer using cairo
// [parameter] : canvas_w, canvas_h, cairo_context, inference_result
// typedef int (*PtrFuncPostprcAndCairoDraw)(unsigned int, unsigned int, cairo_t*, struct inference_result_msg*);
typedef int (*PtrFuncPostprc)(unsigned int, unsigned int, struct inference_result_msg*);

extern void nc_cnn_postprocess_stop(void);
extern void *nc_cnn_postprocess_task(void *arg);
extern void nc_cnn_set_func_for_postprocess_and_cairo_draw (PtrFuncPostprc fp);
//extern void nc_cnn_set_func_for_postprocess (PtrFuncPostprc fp);

//extern PtrFuncPostprc g_postprocess;
extern PtrFuncPostprc g_postprocess_and_draw_func;
#endif // __NC_CNN_WORKER_FOR_POSTPROCESS_H__
