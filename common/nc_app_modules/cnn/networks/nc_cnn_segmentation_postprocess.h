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
* @file    : nc_cnn_segmentation_postprocess.h
*
* @brief   : nc_cnn_segmentation_postprocess header
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


#ifndef __NC_CNN_SEGMENTATION_POSTPROCESS_H__
#define __NC_CNN_SEGMENTATION_POSTPROCESS_H__

#include <stdlib.h>
#include "nc_cnn_aiware_runtime.h"

extern void postprocess_freespace(struct cnn_output_info *cnn_output);
extern int nc_postprocess_segmentation_inference_result(unsigned int canvas_w, unsigned int canvas_h, struct inference_result_msg *inference_res);
extern void nc_tiled_to_scanline_n_scale_up(struct cnn_output_info cnn_output, unsigned int canvas_w, unsigned int canvas_h, unsigned int *canvas);
extern void nc_tiled_to_scanline(struct cnn_output_info cnn_output, unsigned int *scanline);
extern void nc_argmax_opt2(const uint8_t *input, uint32_t npu_out_w, uint32_t npu_out_h, uint8_t *output);

#if 0
// linked list for segmentation
extern stSegInfo* nc_create_seginfo_topnode(int x, int y, uint32_t val);
extern stSegInfo* nc_create_seginfo_node(stSegInfo* parent, int x, int y, uint32_t val);
extern void nc_launch_destory_linked_list_worker(stSegInfo* head);
extern int nc_get_seg_list_cnt(stSegInfo* head);
extern void nc_print_seg_list(stSegInfo* head);
#endif

#endif // __NC_CNN_SEGMENTATION_POSTPROCESS_H__
