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
#include <strings.h>
#include <stdio.h>

// #define SHOW_BYTETRACK_TIMEMEASURE

#define BYTETRACK_TRACK_BUFFER  (15)  /* -> 0.5 s lost-track tolerance */

/* Vehicle group: the network flips car/bus/truck on the same vehicle (mostly with
 * distance), so these classes share one tracker and each track keeps a class vote.
 * Far boxes are unreliable: a vote weighs (h / FULL_H)^2 * prob, capped at 1. */
#define CLS_VOTE_MAX_CH     (8)
#define CLS_VOTE_MAX_TRACKS (128)
#define CLS_VOTE_DECAY      (0.97f)
#define CLS_VOTE_SWITCH     (2.0f)      // other class must lead by this much to switch
#define CLS_VOTE_FULL_H     (80.0f)     // px at 1080p output scale
#define CLS_VOTE_MIN_IOU    (0.3f)      // track box vs. detection, to read the class back

typedef struct {
    int      used;
    int      track_id;
    int      class_idx;                 // voted class
    uint32_t last_frame;
    float    score[MAX_CNN_CLASS_CNT];
} stClsVote;

static stClsVote g_cls_vote[CLS_VOTE_MAX_CH][CLS_VOTE_MAX_TRACKS];
static uint32_t  g_cls_vote_frame[CLS_VOTE_MAX_CH];

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

static int is_vehicle_group_class(const char *name)
{
    return !strcasecmp(name, "car") || !strcasecmp(name, "bus")
        || !strcasecmp(name, "tru") || !strcasecmp(name, "truck");
}

static float det_track_iou(const stCnnDetectedObjInfo *d, const stBBox *b)
{
    float iw = ((d->x + d->w < b->x + b->w) ? d->x + d->w : b->x + b->w) - ((d->x > b->x) ? d->x : b->x);
    float ih = ((d->y + d->h < b->y + b->h) ? d->y + d->h : b->y + b->h) - ((d->y > b->y) ? d->y : b->y);
    if (iw <= 0.0f || ih <= 0.0f) return 0.0f;
    float inter = iw * ih;
    return inter / (d->w * d->h + b->w * b->h - inter);
}

/* track ids never repeat within a tracker, so the oldest slot is safe to reuse */
static stClsVote *cls_vote_get(int cam_ch, int track_id)
{
    stClsVote *tbl = g_cls_vote[cam_ch];
    stClsVote *slot = NULL;

    for (int i = 0; i < CLS_VOTE_MAX_TRACKS; i++) {
        stClsVote *v = &tbl[i];
        if (v->used && v->track_id == track_id) return v;
        if (slot != NULL && !slot->used) continue;
        if (slot == NULL || !v->used || v->last_frame < slot->last_frame) slot = v;
    }

    memset(slot, 0, sizeof(*slot));
    slot->track_id = track_id;
    slot->class_idx = -1;
    slot->used = 1;
    return slot;
}

/* det == NULL: tracked but no detection matched, keep the current class */
static int cls_vote_update(stClsVote *v, const stCnnDetectedObjInfo *det, uint32_t frame)
{
    v->last_frame = frame;
    if (det == NULL) return v->class_idx;

    int cls = (int)det->class_id;
    float k = det->h / CLS_VOTE_FULL_H;
    if (k > 1.0f) k = 1.0f;

    for (int i = 0; i < MAX_CNN_CLASS_CNT; i++) v->score[i] *= CLS_VOTE_DECAY;
    v->score[cls] += k * k * det->prob;

    if (v->class_idx < 0) {
        v->class_idx = cls;
    } else if (cls != v->class_idx && v->score[cls] > v->score[v->class_idx] + CLS_VOTE_SWITCH) {
        v->class_idx = cls;
    }
    return v->class_idx;
}

/* one tracker for the group (the first class's), results go to each track's voted class */
static void track_vehicle_group(stCnnPostprocessingResults *out_cnn_results, int tracker_class,
                                stCnnDetectedObjInfo *dets, uint32_t det_cnt, int cam_ch)
{
    tracked_out_bbox_cnt = 0;
    memset(tracked_objs_info_arr, 0, sizeof(stObjInfo) * MAX_CNN_RESULT_CNT_OF_CLASS);
    nc_update_bytetracked_objs_per_frame((uint16_t)tracker_class, dets, det_cnt, tracked_objs_info_arr, &tracked_out_bbox_cnt, cam_ch);

    int use_vote = (cam_ch >= 0 && cam_ch < CLS_VOTE_MAX_CH);
    uint32_t frame = use_vote ? ++g_cls_vote_frame[cam_ch] : 0;

    for (uint32_t bidx = 0; bidx < tracked_out_bbox_cnt; bidx++) {
        const stObjInfo *o = &tracked_objs_info_arr[bidx];

        const stCnnDetectedObjInfo *det = NULL;
        float best_iou = CLS_VOTE_MIN_IOU;
        for (uint32_t d = 0; d < det_cnt; d++) {
            float iou = det_track_iou(&dets[d], &o->bbox);
            if (iou >= best_iou) {
                best_iou = iou;
                det = &dets[d];
            }
        }

        int cls = (det != NULL) ? (int)det->class_id : -1;
        if (use_vote) cls = cls_vote_update(cls_vote_get(cam_ch, o->track_id), det, frame);
        if (cls < 0) cls = tracker_class;

        stClassObjs *co = &out_cnn_results->class_objs[cls];
        if (co->obj_cnt < MAX_CNN_RESULT_CNT_OF_CLASS) co->objs[co->obj_cnt++] = *o;
    }
}

int nc_init_bytetrackers (int fps, int cam_ch, E_NETWORK_UID  net_id)
{
    stNetwork_info* net_info = NULL;

    net_info = nc_cnn_get_network_info(net_id);
    if (net_info == NULL){
        printf("%s, nc_cnn_get_network_info fail. net_id:%d\n", __FUNCTION__, (int)net_id);
        return -1;
    }

    if (cam_ch >= 0 && cam_ch < CLS_VOTE_MAX_CH) {
        memset(g_cls_vote[cam_ch], 0, sizeof(g_cls_vote[cam_ch]));
        g_cls_vote_frame[cam_ch] = 0;
    }

    tracked_objs_info_arr = (stObjInfo *)malloc(sizeof(stObjInfo) * MAX_CNN_RESULT_CNT_OF_CLASS);
    /* ByteTrack: max_time_lost(frames) = fps/30 * track_buffer.
     * Fixed track_buffer keeps lost-track tolerance at 0.5 s for any fps
     * (30fps: 15 frames, LIGHT 15fps: 7 frames). Same as fps/2 at 30fps. */
    return nc_init_bytetracker(fps, BYTETRACK_TRACK_BUFFER, net_info->class_id, net_info->class_num, cam_ch);
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

    // Vehicle group needs at least two classes to be worth merging
    int in_vehicle_group[net_info->class_num] = {0,};
    int vehicle_tracker_class = -1;
    int vehicle_class_cnt = 0;
    for (i = 0; i < net_info->class_num; i++) {
        if (!is_vehicle_group_class(net_info->class_name[i])) continue;
        in_vehicle_group[i] = 1;
        if (vehicle_tracker_class < 0) vehicle_tracker_class = i;
        vehicle_class_cnt++;
    }
    if (vehicle_class_cnt < 2) {
        memset(in_vehicle_group, 0, sizeof(in_vehicle_group));
        vehicle_tracker_class = -1;
    }

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

    // Merge the vehicle group, the voted classes are filled in again below
    stCnnDetectedObjInfo vehicle_dets[MAX_CNN_RESULT_CNT_OF_CLASS];
    uint32_t vehicle_det_cnt = 0;
    for (i = 0; i < net_info->class_num; i++)
    {
        if (!in_vehicle_group[i]) continue;
        for (int bidx = 0; bidx < each_class_det_cnt[i] && vehicle_det_cnt < MAX_CNN_RESULT_CNT_OF_CLASS; bidx++) {
            vehicle_dets[vehicle_det_cnt] = detected_objs[i][bidx];
            vehicle_dets[vehicle_det_cnt].class_id = i;
            vehicle_det_cnt++;
        }
        out_cnn_results->class_objs[i].class_id = i;
        out_cnn_results->class_objs[i].obj_cnt = 0;
    }
    if (vehicle_det_cnt > 0) {
        track_vehicle_group(out_cnn_results, vehicle_tracker_class, vehicle_dets, vehicle_det_cnt, cam_ch);
    }

    // Update tracker
    for (i = 0; i < net_info->class_num; i++)
    {
        if (in_vehicle_group[i] || each_class_det_cnt[i] < 1) continue;

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
