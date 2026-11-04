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
* @file    : nc_cnn_yolov5_postprocess.c
*
* @brief   : nc_cnn_yolov5_postprocess source
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

#include "nc_cnn_aiware_runtime.h"
#include "nc_cnn_yolov5_postprocess.h"
#include "nc_cnn_anchor.h"
#include "math.h"

#include "stdio.h"
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "nc_utils.h"
#include "nc_cnn_common.h"
#include "nc_cnn_config_parser.h"
#include "nc_ts_fsync_flipflop_buffers.h"
#ifdef USE_BYTETRACK
#include "nc_cnn_tracker.h"
#endif

// #define SHOW_NMS_TIMEMEASURE

stNetwork_info* g_yolov5_net_info = NULL;

static void nc_yolov5_detections(detection *dets, int *det_num, unsigned int target_w, unsigned int target_h, cnn_output_info *cnn_output, int output_idx)
{
    unsigned int output_w = cnn_output->width;
    unsigned int output_h = cnn_output->height;
    float *output_tiled_data = (float*)cnn_output->tiled_data;

    int side_x = 0, side_y = 0, obj_index = 0;

    int gridside_width = output_w;
    int gridside_height = output_h;

    int image_height = NPU_INPUT_HEIGHT;
    int image_width = NPU_INPUT_WIDTH;

    int cell_size_x = image_width / gridside_width;
    int cell_size_y = image_height / gridside_height;

    int boxes_num = 3;//YOLOV5_ANCHORS_NUM;
    int anchor_num = g_yolov5_net_info->anchor_num;

    const int screen_size = gridside_width * gridside_height;
    const int box_size = (5 + g_yolov5_net_info->class_num) * screen_size;

    float *yolov5_anchors = (float*)g_yolov5_net_info->anchor_size;

    for (side_y = 0; side_y < gridside_height; ++side_y) {
        for (side_x = 0; side_x < gridside_width; ++side_x) {
            for (obj_index = 0; obj_index < boxes_num; ++obj_index)
            {
                if (*det_num >= BOX_MAX_NUM)
                {
                    return;
                }

                int box_index = side_y * gridside_width + side_x + obj_index * box_size;
                float confidence = output_tiled_data[box_index + 4 * screen_size];
                confidence = nc_Logistic_activate(confidence);
                dets[*det_num].objectness = (confidence > (float)g_yolov5_net_info->obj_th) ? confidence : 0;

                if (dets[*det_num].objectness>0)
                {
                    float x = (float)(nc_Logistic_activate(output_tiled_data[box_index + 0 * screen_size])*2 - 0.5 + side_x) * (float)cell_size_x;
                    float y = (float)(nc_Logistic_activate(output_tiled_data[box_index + 1 * screen_size])*2 - 0.5 + side_y) * (float)cell_size_y;
                    float w = yolov5_anchors[output_idx*boxes_num+obj_index] * (float)pow(2*nc_Logistic_activate(output_tiled_data[box_index + 2 * screen_size]),2);
                    float h = yolov5_anchors[output_idx*boxes_num+obj_index+anchor_num] * (float)pow(2*nc_Logistic_activate(output_tiled_data[box_index + 3 * screen_size]),2);

                    stBBox b;
                    b.x = x * (float)target_w / (float)image_width ;
                    b.y = y * (float)target_h / (float)image_height;
                    b.w = w * (float)target_w / (float)image_width ;
                    b.h = h * (float)target_h / (float)image_height;

                    dets[*det_num].bbox = b;

                    for (int class_index = 0; class_index < g_yolov5_net_info->class_num; ++class_index) {
                        float curr_score_pre = (float)(output_tiled_data[box_index + (5 + class_index) * screen_size]);
                        float curr_score = nc_Logistic_activate(curr_score_pre);
                        dets[*det_num].prob[class_index] = (curr_score > (float)g_yolov5_net_info->det_th) ? curr_score : 0;
#if 0
                        if (dets[*det_num].prob[class_index] > 0.) {
                            printf("class_index : %d / prob : %f\n", class_index, dets[*det_num].prob[class_index]);
                        }
#endif
                    }

                    (*det_num)++;
                }
            }
        }
    }

    #if 0
    if (*det_num > (int)MAX_CNN_RESULT_CNT_OF_CLASS) {
        printf("[%s:%d]................... *det_num(%d) > MAX_CNN_RESULT_CNT_OF_CLASS(%d) ..........\n", __FUNCTION__, __LINE__, *det_num, MAX_CNN_RESULT_CNT_OF_CLASS);
    }
    #endif
}

static void nc_yolov5_store_postprocess_results(detection *dets, int *det_num, stCnnPostprocessingResults *out_cnn_results)
{
    int i,j =0;

    memset(out_cnn_results, 0, sizeof(stCnnPostprocessingResults));
    for (i = 0; i < *det_num; i++)
    {
        int pred_class = 0;
        float best_score = 0;
        int class_index = 0;
        for (class_index = 0; class_index < g_yolov5_net_info->class_num; class_index++) {
            float curr_score = dets[i].prob[class_index];
            if (curr_score > best_score) {
                best_score = curr_score;
                pred_class = class_index;
            }
        }

        float box_x = (dets[i].bbox.x - dets[i].bbox.w / 2);
        float box_y = (dets[i].bbox.y - dets[i].bbox.h / 2);
        float box_w = (dets[i].bbox.w);
        float box_h = (dets[i].bbox.h);

        for (j = 0; j < g_yolov5_net_info->class_num; j++)
        {
            if ((pred_class == j) & (dets[i].objectness*best_score >= (float)g_yolov5_net_info->det_th))
            {
                int idx = out_cnn_results->class_objs[pred_class].obj_cnt;
                if (idx == MAX_CNN_RESULT_CNT_OF_CLASS) {
                    continue;
                }
                out_cnn_results->class_objs[pred_class].class_id = pred_class;
                out_cnn_results->class_objs[pred_class].objs[idx].track_id = -1;
                out_cnn_results->class_objs[pred_class].objs[idx].prob = dets[i].objectness*best_score;
                out_cnn_results->class_objs[pred_class].objs[idx].bbox.x = box_x;
                out_cnn_results->class_objs[pred_class].objs[idx].bbox.y = box_y;
                out_cnn_results->class_objs[pred_class].objs[idx].bbox.w = box_w;
                out_cnn_results->class_objs[pred_class].objs[idx].bbox.h = box_h;
                out_cnn_results->class_objs[pred_class].obj_cnt ++;
            }
        }
    }
}

static void nc_postprocess_yolov5_detections(unsigned int target_w, unsigned int target_h, cnn_output_info *cnn_output, stCnnPostprocessingResults *out_cnn_results)
{
    static uint32_t framecnt = 0;
    static detection dets[BOX_MAX_NUM];
    static int det_num = 0;

    if (cnn_output->index_of_total == 0) {
        memset(dets, 0, sizeof(dets));
        det_num = 0;
    }

    nc_yolov5_detections(dets, &det_num, target_w, target_h, cnn_output, cnn_output->index_of_total);

    // nms code
    if((cnn_output->index_of_total+1) == cnn_output->total_tensor_cnt)
    {
#ifdef SHOW_NMS_TIMEMEASURE
        uint64_t start_time;
        start_time = nc_get_mono_time();
#endif
        nc_Nms_box(dets, &det_num, g_yolov5_net_info->class_num, (float)g_yolov5_net_info->nms_th);
#ifdef SHOW_NMS_TIMEMEASURE
        static uint64_t elapsed_ms = 0;
        static int max_det_num = 0;
        elapsed_ms = nc_elapsed_time(start_time);
        if (det_num > max_det_num) max_det_num = det_num;
        printf("[frame:%08u] nms time :\t%llu ms (det_num:%d)%s (max det_num:%d)\n", framecnt, elapsed_ms, det_num, (elapsed_ms > 15)?" > 15 ms":"", max_det_num);
#endif

        nc_yolov5_store_postprocess_results(dets, &det_num, out_cnn_results);
#ifdef USE_BYTETRACK
        // Execute tracker
        nc_cnn_track(out_cnn_results, cnn_output->net_id, cnn_output->cam_ch);
#endif
        framecnt++;
    }
}


// #define PRINT_AVERAGE_PROCESS_TIME
void postprocess_yolov5_detections(unsigned int canvas_w, unsigned int canvas_h, cnn_output_info *cnn_output)
{
    #ifdef PRINT_DRAW_TIME
    int elapsed_ms;
    uint64_t start_time = nc_get_mono_time();
    #endif

    #ifdef PRINT_AVERAGE_PROCESS_TIME
    static uint32_t framecnt = 0;
    uint64_t start_time = nc_get_mono_time();
    static uint64_t elapsed_ms = 0;
    #endif

    uint64_t time_stamp = 0;
    int buf_write_idx = 0;
    static pp_result_buf *pp_buf = NULL;
    static stCnnPostprocessingResults *yolov5_detect_results = NULL;
    if (cnn_output->index_of_total == 0) {
        // get gui writable buffer & index from flip-flop(double) buffers
        pp_buf = (pp_result_buf *)nc_tsfs_ff_get_writable_buffer_and_set_timestamp(cnn_output->cam_ch+DETECT_NETWORK, &buf_write_idx, time_stamp);
        yolov5_detect_results = &(pp_buf->cnn_result);
    }
    nc_postprocess_yolov5_detections(canvas_w, canvas_h, cnn_output, yolov5_detect_results);

    // draw after all tensor output(per frame) processed
    if ((cnn_output->index_of_total+1) == cnn_output->total_tensor_cnt && pp_buf != NULL) {
        pp_buf->time_stamp = time_stamp;
        pp_buf->draw_info.max_class_cnt = g_yolov5_net_info->class_num;
        pp_buf->draw_info.class_names = g_yolov5_net_info->class_name;
        pp_buf->draw_info.class_colors = (stRGB24*)g_yolov5_net_info->class_color;
        pp_buf->cam_channel = cnn_output->cam_ch;
        pp_buf->net_id = cnn_output->net_id;
        pp_buf->net_task = DETECTION;

        nc_tsfs_ff_finish_write_buf(cnn_output->cam_ch+DETECT_NETWORK);
    }

    #ifdef PRINT_AVERAGE_PROCESS_TIME
    framecnt++;
    elapsed_ms += nc_elapsed_time(start_time);
    if (framecnt % (30 * 30) == 0) {
        printf("[%s][frame:%08u] average cnn time :\t%lf ms\n", __FUNCTION__, framecnt, elapsed_ms/(double)framecnt);
    }
    #endif

    #ifdef PRINT_DRAW_TIME
    elapsed_ms = nc_elapsed_time(start_time);
    printf("draw time (obj detect) : %llu ms%s\n", elapsed_ms, (elapsed_ms > 33)?" > 33 ms":"");
    #endif
}



/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                  for process and draw yolov5 inference result               //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////
int nc_postprocess_yolov5_inference_result(unsigned int canvas_w, unsigned int canvas_h, struct inference_result_msg *inference_res)
{
    int ret = 0;

    if(inference_res->network_id != NETWORK_YOLOV5_DET) {
        return -1;
    }

    g_yolov5_net_info = nc_cnn_get_network_info(inference_res->network_id);
    if (g_yolov5_net_info == NULL){
        printf("%s, nc_cnn_get_network_info fail. net_id:%d\n", __FUNCTION__, inference_res->network_id);
        return -1;
    }

    postprocess_yolov5_detections(canvas_w, canvas_h, &(inference_res->cnn_output));
    return ret;
}