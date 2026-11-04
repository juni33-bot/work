/**
********************************************************************************
* Copyright (C) 2024 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : nc_cnn_tracker.c
*
* @brief   : nc_cnn_tracker source
*
* @author  : Software Development Team.  NextChip Inc.
*
* @date    : 2024.09.23.
*
* @version : 1.0.0
********************************************************************************
* @note
*
********************************************************************************
*/

#include "nc_cnn_config_parser.h"
#include "nc_bytetrack_wrapper.h"
#include "nc_cnn_tracker.h"

#include <string.h>
#include <stdio.h>

// #define SHOW_BYTETRACK_TIMEMEASURE

uint32_t tracked_out_bbox_cnt = 0;
stObjInfo *tracked_objs_info_arr = NULL;

// TODO: ignore specific class
// static int is_not_tracking_class (int class_id) {
//     #ifdef IGN_SPECIFIC_CLASS_TRACKING
//     if (strcmp(yolov5_class_names[class_id],"air") == 0) {
//         return 1;
//     }
//     #endif
//     return 0;
// }

int nc_init_bytetrackers (int fps, int cam_ch, E_NETWORK_UID  net_id)
{
    stNetwork_info* net_info = NULL;

    net_info = nc_cnn_get_network_info(net_id);
    if (net_info == NULL){
        printf("%s, nc_cnn_get_network_info fail. net_id:%d\n", __FUNCTION__, (int)net_id);
        return -1;
    }

    tracked_objs_info_arr = (stObjInfo *)malloc(sizeof(stObjInfo) * MAX_CNN_RESULT_CNT_OF_CLASS);
    return nc_init_bytetracker(fps, fps/2, net_info->class_id, net_info->class_num, cam_ch);
}

void nc_deInit_bytetrackers (int cam_ch)
{
    // deinit byte tracker
    nc_destroy_byettracker(cam_ch);

    if (tracked_objs_info_arr != NULL) {
        free(tracked_objs_info_arr);
    }
}

void nc_cnn_track(stCnnPostprocessingResults *out_cnn_results, E_NETWORK_UID  net_id, int cam_ch)
{
    int i = 0;
    stNetwork_info* net_info = NULL;

    net_info = nc_cnn_get_network_info(net_id);
    if (net_info == NULL){
        printf("%s, nc_cnn_get_network_info fail. net_id:%d\n", __FUNCTION__, (int)net_id);
        return;
    }

    stCnnDetectedObjInfo detected_objs[net_info->class_num][MAX_CNN_RESULT_CNT_OF_CLASS] = {0,};
    int each_class_det_cnt[net_info->class_num] = {0,};

    // Formatting with tracker input
    for (i = 0; i < net_info->class_num; i++)
    {
        for(int bidx = 0; bidx < out_cnn_results->class_objs[i].obj_cnt; bidx++)
        {
            // TODO: ignore specific class
            // if not tracking class -> don't transfer to bytetracker..
            // if (is_not_tracking_class(i)) {
            //     continue;
            // }

            int idx = each_class_det_cnt[i];
            if (idx == MAX_CNN_RESULT_CNT_OF_CLASS) {
                continue;
            }
            detected_objs[i][idx].class_id = out_cnn_results->class_objs[i].class_id;
            detected_objs[i][idx].x = out_cnn_results->class_objs[i].objs[bidx].bbox.x;
            detected_objs[i][idx].y = out_cnn_results->class_objs[i].objs[bidx].bbox.y;
            detected_objs[i][idx].w = out_cnn_results->class_objs[i].objs[bidx].bbox.w;
            detected_objs[i][idx].h = out_cnn_results->class_objs[i].objs[bidx].bbox.h;
            detected_objs[i][idx].prob = out_cnn_results->class_objs[i].objs[bidx].prob;
            each_class_det_cnt[i]++;

            #if 0
            float prob = detected_objs[pred_class][idx].prob;
            if (prob < 0.9) {
                printf("............... class id : %d / prob : %f ..............\n", pred_class, prob);
            }
            #endif
        }
    }

    #ifdef SHOW_BYTETRACK_TIMEMEASURE
        uint64_t elapsed_ms = 0;
    #endif

    // Update tracker
    for (i = 0; i < net_info->class_num; i++)
    {
        if (each_class_det_cnt[i] < 1) continue;

        // TODO: ignore specific class
        // if not tracking class
        // if (is_not_tracking_class(i)) continue;

    #ifdef SHOW_BYTETRACK_TIMEMEASURE
        uint64_t start_time;
        start_time = nc_get_mono_time();
    #endif

        tracked_out_bbox_cnt = 0;
        memset(tracked_objs_info_arr, 0, sizeof(stObjInfo) * MAX_CNN_RESULT_CNT_OF_CLASS);
        nc_update_bytetracked_objs_per_frame(i, detected_objs[i], each_class_det_cnt[i], tracked_objs_info_arr, &tracked_out_bbox_cnt, cam_ch);

        out_cnn_results->class_objs[i].class_id = i;
        out_cnn_results->class_objs[i].obj_cnt = tracked_out_bbox_cnt;
    #ifdef SHOW_BYTETRACK_TIMEMEASURE
        elapsed_ms += nc_elapsed_time(start_time);
    #endif

        for (uint32_t bidx = 0; bidx < tracked_out_bbox_cnt; bidx++) {
            out_cnn_results->class_objs[i].objs[bidx] = tracked_objs_info_arr[bidx];
        }
    }

    #ifdef SHOW_BYTETRACK_TIMEMEASURE
    printf("....... bytetrack update time (total w/o draw) ch=%d : %lu ms%s\n", cam_ch, elapsed_ms, (elapsed_ms > 5)?" > 5 ms":"");
    #endif
}
