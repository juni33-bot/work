/**
********************************************************************************
* Copyright (C) 2024 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : nc_cnn_draw_cairo.h
*
* @brief   : nc_cnn_draw_cairo header
*
* @author  : Software Development Team.  NextChip Inc.
*
* @date    : 2024.09.20.
*
* @version : 1.0.0
********************************************************************************
* @note
*
********************************************************************************
*/

#ifndef __NC_CNN_DRAW_CAIRO_H__
#define __NC_CNN_DRAW_CAIRO_H__

#include <stdint.h>
#include "nc_cnn_common.h"

struct cnn_output_info; // To be removed (cnn)
// cairo control
extern void nc_cairo_init_cavas (unsigned int  canvas_w, unsigned int canvas_h);
extern cairo_t* nc_cairo_clear_and_ready_writebuffer (uint64_t time_stamp);
extern void nc_cairo_draw_extended_infos_and_logo (uint64_t framecnt);
extern void nc_cairo_finish_draw_writebuffer(void);
extern void nc_cairo_destroy (void);

// draw object detection
extern void nc_cairo_draw_object_detections (cairo_t *canvas, stObjDrawInfo* drawInfo, stCnnPostprocessingResults *cnn_results);

// draw segmentation
#if 0
extern void nc_opencv_draw_freespace (unsigned int canvas_w, unsigned int canvas_h, unsigned int *canvas, cnn_output_info cnn_output); // To be removed (cnn)
#endif
extern void nc_cairo_draw_argmax_freespace (unsigned int canvas_w, unsigned int canvas_h, cairo_t *cr, cnn_output_info cnn_output); // To be removed (cnn)
extern void nc_direct_draw_freespace_to_canvas (unsigned int canvas_w, unsigned int canvas_h, unsigned int *canvas, cnn_output_info cnn_output); // To be removed (cnn)
// extern void *cnn_draw_task(void *arg);

#endif // __NC_CNN_DRAW_CAIRO_H__