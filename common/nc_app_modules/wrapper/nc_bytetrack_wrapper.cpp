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
* @file    : nc_bytetrack_wrapper.cpp
*
* @brief   : nc_bytetrack_wrapper source
*
* @author  : SoC SW team.  NextChip Inc.  
*
* @date    : 2022.05.27.
*
* @version : 1.0.0
********************************************************************************
* @note
* 2022.05.27 / 1.0.0 / Initial released.
* 
********************************************************************************
*/

/*
********************************************************************************
*               INCLUDES
********************************************************************************
*/
#include <float.h>
#include <stdio.h>
#include <vector>
#include <map>
#include <chrono>

#include "ByteTrack/BYTETracker.h"
#include "nc_bytetrack_wrapper.h"
#include "nc_utils.h"

using namespace std;
using namespace byte_track;
using STrackPtr = std::shared_ptr<STrack>;

// map<unique class_id, BYTETracker pointer>
// static std::map<uint16_t, BYTETracker*> bytetrackers_g;
static std::vector<std::map<uint16_t, BYTETracker*>> bytetrackers_g;
const float predefined_probability = 0.95;

int nc_init_bytetracker(int fps, int track_buffer, uint16_t *class_ids_arr, int class_ids_cnt, int cam_ch)
{
    if (class_ids_arr == NULL || class_ids_cnt < 1) {
        printf("[Error][%s][%d].... class_ids_arr is '%s' / class_ids_cnt(%d) < 1........\n", __FUNCTION__, __LINE__, (class_ids_arr == NULL)?"NULL":"NOT NULL", class_ids_cnt);
        return -1;
    }

    std::map<uint16_t, BYTETracker*> bytetrackers;
    for(int i = 0; i < class_ids_cnt; i++) {
        bytetrackers.insert(std::pair<uint16_t, BYTETracker*>(class_ids_arr[i], new BYTETracker(fps, track_buffer)));
    }
    printf("\nInit ByteTrack : fps(%d), max class cnt(%d) ......!\n", fps, class_ids_cnt);

    bytetrackers_g.push_back(bytetrackers);

    return 0;
}

void nc_destroy_byettracker(int cam_ch)
{
    for (int i = 0; i < bytetrackers_g[cam_ch].size(); i++) {
        if (bytetrackers_g[cam_ch][i] != NULL)
            delete bytetrackers_g[cam_ch][i];
    }
}

void nc_set_bytetracker_thresh_values(uint16_t class_id, float track_thresh, float high_thresh, float match_thresh, int cam_ch)
{
    bytetrackers_g[cam_ch][class_id]->set_thresh_values(track_thresh, high_thresh, match_thresh);
}

int nc_update_bytetracked_objs_per_frame(uint16_t class_id, stCnnDetectedObjInfo *in_objs, uint32_t in_objs_cnt, stObjInfo *out_tracked_objs, uint32_t *out_strack_cnt, int cam_ch)
{
    if (in_objs == NULL || in_objs_cnt < 1){
        // printf("[Error][%s][%d].... in_objs is '%s' / in_objs_cnt(%d) < 1........\n", __FUNCTION__, __LINE__, (in_objs == NULL)?"NULL":"NOT NULL", in_objs_cnt);
        return -1;
    }

    if (out_tracked_objs == NULL) {
        printf("[Error][%s][%d].... out_tracked_objs is NULL ........\n", __FUNCTION__, __LINE__);
        return -2;
    }

    *out_strack_cnt = 0;

    // 1. make objects list
    std::vector<Object> t_objects;
    for (int i = 0; i < in_objs_cnt; i++) {
        Object obj (Rect<float>(in_objs[i].x, in_objs[i].y, in_objs[i].w, in_objs[i].h), class_id, in_objs[i].prob);
        t_objects.push_back(obj);
    }

    // 2. update tracker
    vector<STrackPtr> output_stracks = bytetrackers_g[cam_ch][class_id]->update(t_objects);

    // 3. return output_result of tracking
    *out_strack_cnt = output_stracks.size();
    if (*out_strack_cnt > (int)MAX_CNN_RESULT_CNT_OF_CLASS) {
        *out_strack_cnt = (int)MAX_CNN_RESULT_CNT_OF_CLASS;
    }

    for (int i = 0; i < (int)*out_strack_cnt; i++)
    {
        int track_id = output_stracks[i]->getTrackId();
        Rect<float> rect = output_stracks[i]->getRect();
        out_tracked_objs[i].track_id = track_id;
        out_tracked_objs[i].prob = output_stracks[i]->getScore();
        out_tracked_objs[i].bbox.x = rect.x();
        out_tracked_objs[i].bbox.y = rect.y();
        out_tracked_objs[i].bbox.w = rect.width();
        out_tracked_objs[i].bbox.h = rect.height();
    }

    return 0;
}
