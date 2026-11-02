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
* @file    : nc_cnn_trichimera_postprocess.c
*
* @brief   : nc_cnn_trichimera_postprocess source
*
* @author  : AI SW team.  NextChip Inc.
*
* @date    : 2024.04.25
*
* @version : 1.0.0
********************************************************************************
* @note
* 2022.11.10 / 1.0.0 / Initial released.
*
********************************************************************************
*/

#include "nc_cnn_aiware_runtime.h"

#define DETECT_IDX 3
#define SEGMENT_IDX 4
#define LANE_IDX 5

#define DETECT_NUM 3
#define SEGMENT_NUM 1
#define LANE_NUM 7

/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                 for process and draw trichimera inference result            //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////
int nc_postprocess_trichimera_inference_result(unsigned int canvas_w, unsigned int canvas_h, struct inference_result_msg *inference_res)
{
    int ret = 0;
    int out_index = inference_res->cnn_output.index_of_total;

    if(inference_res->network_id != NETWORK_TRI_CHIMERA) {
        return -1;
    }

    if(out_index < DETECT_IDX){      //index 0,1,2 
        inference_res->cnn_output.total_tensor_cnt = DETECT_NUM;
        nc_postprocess_yolov8_inference_result(canvas_w, canvas_h, inference_res);

    } 
    if(out_index+1 == SEGMENT_IDX){  //index 3
        inference_res->cnn_output.index_of_total = 0;
        inference_res->cnn_output.total_tensor_cnt = SEGMENT_NUM;
        nc_postprocess_segmentation_inference_result(canvas_w, canvas_h, inference_res);
    }

    if(out_index+1 >= LANE_IDX){     //index 4~10
        inference_res->cnn_output.total_tensor_cnt = LANE_NUM;
        inference_res->cnn_output.index_of_total = out_index-4;
        nc_postprocess_ufld_inference_result(canvas_w, canvas_h, inference_res);
    }

    return ret;
}