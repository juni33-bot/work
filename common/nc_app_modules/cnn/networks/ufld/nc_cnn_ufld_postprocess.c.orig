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
* @file    : nc_cnn_ufld_postprocess.c
*
* @brief   : nc_cnn_ufld_postprocess source
*
* @author  : AI SW team.  NextChip Inc.
*
* @date    : 2024.12.
*
* @version : 1.0.0
********************************************************************************
* @note
* 2024.12. / 1.0.0 / Initial released.
*
********************************************************************************
*/

#include "nc_cnn_aiware_runtime.h"
#include "nc_cnn_ufld_postprocess.h"
#include "nc_cnn_anchor.h"
#define _USE_MATH_DEFINES
#include <math.h>

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "nc_utils.h"
#include "nc_cnn_common.h"
#include "nc_cnn_config_parser.h"
#include "nc_ts_fsync_flipflop_buffers.h"

stNetwork_info* g_ufld_net_info = NULL;

#ifdef USE_UFLD_TRACKING

#define LANE_QUEUE_SIZE 21
#define MAX_POINT_NUM   18
#define MAX_CATEGORY_NUM 10

typedef stPoint TRACKING_POINT[MAX_POINT_NUM];
typedef struct {
    TRACKING_POINT point_chunk[LANE_QUEUE_SIZE];
    unsigned int chunk_num;

    unsigned int front;
    unsigned int rear;

    TRACKING_POINT point_sum;
    unsigned int valid_num[MAX_POINT_NUM];
    unsigned int point_num[LANE_QUEUE_SIZE];
    unsigned int max_point_num;
}LANE_POINT_QUEUE;


typedef int TRACKING_CATEGORY;
typedef struct {
    TRACKING_CATEGORY ctgr[LANE_QUEUE_SIZE];
    unsigned int front;
    unsigned int rear;

    unsigned int size;

    unsigned int category_cnt[MAX_CATEGORY_NUM];
    unsigned int max_category_index;
}LANE_CATEGORY_QUEUE;

static LANE_POINT_QUEUE POINT_QUEUE[MAX_LANE_DET_CNT] = {{{{{0,0},},},0,0,0,{{0,0},},{0,},{0,},0}, };
static LANE_CATEGORY_QUEUE CATEGORY_QUEUE[MAX_LANE_DET_CNT] = {{{0,},0,0,0,{0,},0}, };
static int invalid_frame_counter[MAX_LANE_DET_CNT] = {0, };


int point_queue_is_full(LANE_POINT_QUEUE *q)
{
    return (q->front == (q->rear + 1) % LANE_QUEUE_SIZE);
}

int point_queue_is_empty(LANE_POINT_QUEUE *q)
{
    return (q->front == q->rear);
}

int point_queue_size(LANE_POINT_QUEUE *q)
{
    return (q->chunk_num);
}

void point_queue_reset(LANE_POINT_QUEUE *q)
{
    memset(q, 0, sizeof(LANE_POINT_QUEUE));
}

void point_enqueue(LANE_POINT_QUEUE *q, stPoint *point, int point_cnt)
{
    if(point_queue_is_full(q)) {
        q->front = (q->front + 1) % LANE_QUEUE_SIZE;
        for(int i=0; i<(int)(q->point_num[q->front]); i++)
        {
            q->point_sum[i].x -= q->point_chunk[q->front][i].x;
            q->point_sum[i].y -= q->point_chunk[q->front][i].y;
            --(q->valid_num[i]);
        }
        --(q->chunk_num);

        if(q->point_num[q->front] == q->max_point_num)
        {
            q->max_point_num = 0;
            int cnt = q->rear;
            while(cnt != (int)(q->front))
            {
                if(q->max_point_num < q->point_num[cnt])
                {
                    q->max_point_num = q->point_num[cnt];
                }

                cnt--;
                if(cnt < 0) cnt = LANE_QUEUE_SIZE-1;
            }
        }
        q->point_num[q->front] = 0;
    }

    q->rear = (q->rear + 1) % LANE_QUEUE_SIZE;
    memcpy(q->point_chunk[q->rear], point, sizeof(stPoint) * point_cnt);
    q->point_num[q->rear] = point_cnt;
    ++(q->chunk_num);

    for(int i=0; i<point_cnt; i++)
    {
        q->point_sum[i].x += point[i].x;
        q->point_sum[i].y += point[i].y;
        ++(q->valid_num[i]);
    }
    if(point_cnt > (int)(q->max_point_num)){
        q->max_point_num = point_cnt;
    }
}

void point_popleft(LANE_POINT_QUEUE *q) // delete the oldest data
{
    q->front = (q->front + 1) % LANE_QUEUE_SIZE;
    for(int i=0; i<(int)(q->point_num[q->front]); i++)
    {
        q->point_sum[i].x -= q->point_chunk[q->front][i].x;
        q->point_sum[i].y -= q->point_chunk[q->front][i].y;
        --(q->valid_num[i]);
    }
    --(q->chunk_num);
    
    if(q->point_num[q->front] == q->max_point_num)
    {
        q->max_point_num = 0;
        int cnt = q->rear;
        while(cnt != (int)(q->front))
        {
            if(q->max_point_num < q->point_num[cnt])
            {
                q->max_point_num = q->point_num[cnt];
            }

            cnt--;
            if(cnt < 0) cnt = LANE_QUEUE_SIZE-1;
        }
    }
    q->point_num[q->front] = 0;
}


int category_queue_is_full(LANE_CATEGORY_QUEUE *q)
{
    return (q->front == (q->rear + 1) % LANE_QUEUE_SIZE);
}

int category_queue_is_empty(LANE_CATEGORY_QUEUE *q)
{
    return (q->front == q->rear);
}

void category_queue_reset(LANE_CATEGORY_QUEUE *q)
{
    memset(q, 0, sizeof(LANE_CATEGORY_QUEUE));
}

void category_enqueue(LANE_CATEGORY_QUEUE *q, TRACKING_CATEGORY ctgr)
{
    if(category_queue_is_full(q)) {
        q->front = (q->front + 1) % LANE_QUEUE_SIZE;
        --(q->category_cnt[q->ctgr[q->front]]);
        --(q->size);
    }
    q->rear = (q->rear + 1) % LANE_QUEUE_SIZE;
    q->ctgr[q->rear] = ctgr;
    ++(q->size);
    ++(q->category_cnt[ctgr]);

    if(q->category_cnt[ctgr] >= q->category_cnt[q->max_category_index])
    {
        q->max_category_index = ctgr;
    }
}


static int nc_ufld_tracking_get_current_category(int lane_index, int category)
{
    
    if(category_queue_is_empty(&CATEGORY_QUEUE[lane_index]))
    {
        return category;
    }

    return CATEGORY_QUEUE[lane_index].max_category_index;
}

static void nc_ufld_tracking_update_fixed(int lane_index, stPoint *point, int point_cnt, int category, stLaneDet *result_lane)
{
    TRACKING_POINT result_point_chunk = {{0,0},};

    point_enqueue(&POINT_QUEUE[lane_index], point, point_cnt);
    
    if(category != 0)
    {
        category_enqueue(&CATEGORY_QUEUE[lane_index], category);
    }

    if(!point_queue_is_empty(&POINT_QUEUE[lane_index]))
    {
        for(int i=0; i<(int)(POINT_QUEUE[lane_index].max_point_num); i++)
        {
            result_point_chunk[i].x = POINT_QUEUE[lane_index].point_sum[i].x / (float)(POINT_QUEUE[lane_index].valid_num[i]);
            result_point_chunk[i].y = POINT_QUEUE[lane_index].point_sum[i].y / (float)(POINT_QUEUE[lane_index].valid_num[i]);
        }

        result_lane->lane_class = nc_ufld_tracking_get_current_category(lane_index, category);
        memcpy(result_lane->point, result_point_chunk, sizeof(stPoint) * POINT_QUEUE[lane_index].max_point_num);
        result_lane->point_cnt = POINT_QUEUE[lane_index].max_point_num;
    }
}

static int nc_ufld_tracking_check_reset_condition(int lane_index, stPoint *point, int point_cnt, unsigned int target_w)
{
    int total = 0, avg = 0;
    int mid = point_cnt / 2;
            
    for(int i=0; i<mid; i++)
    {
        total += (int)(point[i].x);
    }
    avg = total / mid;

    if((lane_index==1 && avg>=((int)target_w/2)) || (lane_index==2 && ((int)target_w/2)>=avg)) return 1;
    else return 0;
}

static void nc_ufld_tracking_check_missing_lane(int lane_index, TRACKING_POINT* point, int* point_cnt)
{
    if(*point_cnt == 0)
    {
        if(invalid_frame_counter[lane_index] > (LANE_QUEUE_SIZE-1) * 0.5)
        {
            return;
        }
        ++invalid_frame_counter[lane_index];
        
        if(!point_queue_is_empty(&POINT_QUEUE[lane_index]) && !category_queue_is_empty(&CATEGORY_QUEUE[lane_index]))
        {
            LANE_POINT_QUEUE* point_queue = &POINT_QUEUE[lane_index];
            int rear = point_queue->rear;
            *point_cnt = point_queue->point_num[rear];
            memcpy(point, point_queue->point_chunk[rear], sizeof(stPoint)* (*point_cnt));
            point_popleft(&POINT_QUEUE[lane_index]);
        }
    }
    else
    {
        invalid_frame_counter[lane_index] = 0;
    }
}

static void nc_ufld_tracking_check_linearity_lane(TRACKING_POINT* point_chunk, int* point_cnt)
{
    stPoint* point = (stPoint*) point_chunk;
    double angle_buffer[MAX_POINT_NUM] = {0, };
    double angle_diff_buffer[MAX_POINT_NUM] = {0, };
    double max_diff_angles = 0;

    double angle_stddev = 0;
    double angle_sum = 0;
    double angle_ssum = 0;
    double angle_mean = 0;

    double diff_stddev = 0;
    double diff_sum = 0;
    double diff_ssum = 0;
    double diff_mean = 0;

    int sign_change_cnt = 0;
    int max_sign_change_cnt = 0;
    int curve_check = 0;

    int is_curve = 0;
    int is_straight = 0;

    int dx = 0;
    int dy = 0;

    double diff = 0;
    double abs_val = 0;
    int i = 0;

    if(*point_cnt < 2) return;

    for(i=0; i<((*point_cnt)-1); i++)
    {
        dx = (int)(point[i+1].x) - (int)(point[i].x);
        dy = (int)(point[i+1].y) - (int)(point[i].y);

        if(dx == 0 && dy == 0)
        {
            angle_buffer[i] = 0.0;
        }
        else
        {
            angle_buffer[i] = atan2((double)dy, (double)dx);
        }

        if(i > 0)
        {
            diff = angle_buffer[i] - angle_buffer[i-1];

            if(diff > M_PI) angle_diff_buffer[i-1] = diff - 2 * M_PI;
            else if (diff < -M_PI) angle_diff_buffer[i-1] = diff + 2 * M_PI;
            else angle_diff_buffer[i-1] = diff;

            abs_val = fabs(angle_diff_buffer[i-1]);
            if(max_diff_angles < abs_val) max_diff_angles = abs_val;
            
            if(abs_val >= M_PI/4) curve_check++;
        }
    }

    // calculate angle_std 
    for(i=0; i<((*point_cnt)-1); i++)
    {
        angle_sum += angle_buffer[i];
    }
    angle_mean = angle_sum / ((*point_cnt)-1);
    
    for(i=0; i<((*point_cnt)-1); i++)
    {
        angle_ssum += pow(angle_buffer[i] - angle_mean, 2);
    }
    angle_stddev = sqrt(angle_ssum/((*point_cnt)-1));

    // calculate diff_std 
    for(i=0; i<((*point_cnt)-2); i++)
    {
        diff_sum += angle_diff_buffer[i];
    }
    diff_mean = diff_sum / ((*point_cnt)-2);
    
    for(i=0; i<((*point_cnt)-2); i++)
    {
        diff_ssum += pow(angle_diff_buffer[i] - diff_mean, 2);
    }
    diff_stddev = sqrt(diff_ssum/((*point_cnt)-2));

    // calculate sign_changes
    for(i=0; i<((*point_cnt)-3); i++)
    {
        if((angle_diff_buffer[i] < 0 && angle_diff_buffer[i+1] > 0) || (angle_diff_buffer[i] > 0 && angle_diff_buffer[i+1] < 0)) // sign change?
        {
            sign_change_cnt++;
        }
    }
    max_sign_change_cnt = (int)(((*point_cnt)-2) * 0.2);

    // check stable
    if(angle_stddev < 0.15 && diff_stddev < 0.1 && max_diff_angles < 0.174 && sign_change_cnt < max_sign_change_cnt)  //is_straight
    {
        is_straight = 1;
    }
    if(angle_stddev < 0.4 && diff_stddev < 0.3 && max_diff_angles < M_PI/4 && curve_check == 0) //is_curve
    {
        is_curve = 1;
    }

    if(!is_straight && !is_curve) // not stable
    {
        memset(point, 0, sizeof(stPoint) * (*point_cnt));
        *point_cnt = 0;
    }
}

#if 0
static void nc_ufld_tracking_v1(stCnnPostprocessingResults *out_cnn_results)
{
    stLaneDet *frame_lane = out_cnn_results->lane_det;

    for(int lane_cnt = 0; lane_cnt < g_ufld_net_info->lane_max_num; lane_cnt++)
    {
        if(frame_lane[lane_cnt].point_cnt > 0) 
        {
            nc_ufld_tracking_update_fixed(lane_cnt, frame_lane[lane_cnt].point, frame_lane[lane_cnt].point_cnt, frame_lane[lane_cnt].lane_class, &frame_lane[lane_cnt]);
        }
    }
}
#endif

static void nc_ufld_tracking_v2(stCnnPostprocessingResults *out_cnn_results, unsigned int target_w)
{
    stLaneDet *frame_lane = out_cnn_results->lane_det;
    TRACKING_POINT last_valid_point_chunk = {{0,0},};
    int last_valid_point_cnt = 0;
    int temp_category = 0;
    int check_reset = 0;

    for(int lane_cnt = 0; lane_cnt < g_ufld_net_info->lane_max_num; lane_cnt++)
    {
        memset(last_valid_point_chunk, 0, sizeof(TRACKING_POINT));
        last_valid_point_cnt = 0;
        temp_category = 0;
        check_reset = 0;

        if(frame_lane[lane_cnt].point_cnt > 0)
        {
            temp_category = frame_lane[lane_cnt].lane_class;
            memcpy(last_valid_point_chunk, frame_lane[lane_cnt].point,  frame_lane[lane_cnt].point_cnt * sizeof(stPoint));
            last_valid_point_cnt = frame_lane[lane_cnt].point_cnt;
        }

        if(lane_cnt == 1 || lane_cnt == 2) // reset check with ego lane
        {
            if(frame_lane[lane_cnt].point_cnt > 0 && !point_queue_is_empty(&POINT_QUEUE[lane_cnt]))
            {
                check_reset = nc_ufld_tracking_check_reset_condition(lane_cnt, frame_lane[lane_cnt].point, frame_lane[lane_cnt].point_cnt, target_w);
            
                if(check_reset) 
                {
                    frame_lane[lane_cnt].point_cnt = 0;

                    for(int i=0; i<g_ufld_net_info->lane_max_num ; i++)
                    {
                        point_queue_reset(&POINT_QUEUE[i]);
                        category_queue_reset(&CATEGORY_QUEUE[i]);
                    }
                    continue;
                }
            }
        }

        nc_ufld_tracking_check_missing_lane(lane_cnt, &last_valid_point_chunk, &last_valid_point_cnt);
        nc_ufld_tracking_check_linearity_lane(&last_valid_point_chunk, &last_valid_point_cnt);

        if(last_valid_point_cnt > 0)
        {
            nc_ufld_tracking_update_fixed(lane_cnt, (stPoint *)last_valid_point_chunk, last_valid_point_cnt, temp_category, &frame_lane[lane_cnt]);
        }
        else
        {
            frame_lane[lane_cnt].point_cnt = 0;
        }
    }
}

#endif


static void nc_ufld_detections(lane_data *ld, E_LANE_ANCHOR anchor_type, unsigned int target_w, unsigned int target_h, cnn_output_info *cnn_output, stCnnPostprocessingResults *out_cnn_results)
{
    uint8_t *max_indices = ld->max_indices;
    uint8_t *valid_anchor = ld->valid_anchor;
    int num_lane = cnn_output->width;
    int num_anchor = cnn_output->height;
    int num_cell = cnn_output->channel;
    float *output_data = (float*)cnn_output->tiled_data;
    int anchor_cnt = 0;
    int cell_cnt = 0;
    float max_output = 0;
    int valid_total = 0;

    float output_total = 0;
    int num = 0;
    int index_min = 0;
    int index_max = 0;
    float sum = 0;
    float sigma = 0;
    float final_x_point = 0, final_y_point = 0;
    float result = 0;
    float mean = 0;
    int stddev = 0;
    int idx = 0;
    uint8_t max_index = 0;

    for(int lane_cnt = 0; lane_cnt < g_ufld_net_info->lane_max_num; lane_cnt++)
    {
        if(g_ufld_net_info->lane_anchor_info[lane_cnt] != anchor_type)
        {
            continue;
        }

        valid_total = 0;

        for(anchor_cnt = 0; anchor_cnt < num_anchor; anchor_cnt++)
        {
            valid_total += valid_anchor[lane_cnt + (num_lane * anchor_cnt)];
        }

        if(valid_total > g_ufld_net_info->lane_anchor_th[anchor_type])
        {
            for(anchor_cnt = 0; anchor_cnt < num_anchor; anchor_cnt++) 
            {
                if(valid_anchor[lane_cnt + (num_lane * anchor_cnt)])
                {
                    output_total = 0;
                    num = 0;
                    index_min = 0;
                    index_max = 0;
                    sum = 0;
                    sigma = 0;
                    final_x_point = 0;
                    final_y_point = 0;
                    result = 0;
                    mean = 0;
                    stddev = 0;
                    max_output = 0;
                    idx = 0;
                    max_index = max_indices[lane_cnt + (num_lane * anchor_cnt)];

                    // calculate standard deviation
                    for(cell_cnt = 0; cell_cnt < num_cell; cell_cnt++)
                    {
                        if(output_data[(anchor_cnt * num_lane) + lane_cnt + (num_lane * num_anchor * cell_cnt)] >= 0)
                        {
                            output_total += output_data[(anchor_cnt * num_lane) + lane_cnt + (num_lane * num_anchor * cell_cnt)];
                            num++;
                        }
                    }

                    if(num > 0) {
                        mean = output_total / (float)num;
                    }

                    for(cell_cnt = 0; cell_cnt < num_cell; cell_cnt++)
                    {
                        if(output_data[(anchor_cnt * num_lane) + lane_cnt + (num_lane * num_anchor * cell_cnt)] >= 0)
                        {
                            sum += (float)pow((double)(output_data[(anchor_cnt * num_lane) + lane_cnt + (num_lane * num_anchor * cell_cnt)]- mean), 2.0);
                        }
                    }
                    
                    if(num > 0) {
                        stddev = (int)ceil(sqrt(sum/(float)num)); 
                    }

                    // decide cell index range
                    index_min = ((max_index - stddev) >= 0 ? (max_index - stddev) : 0); 
                    index_max = ((max_index + stddev) <= (num_cell - 1) ? (max_index + stddev) : (num_cell - 1));

                    // softmax operation & calculate expected value of coordinate
                    max_output = output_data[(anchor_cnt * num_lane) + lane_cnt + (num_lane * num_anchor * max_index)];

                    for(cell_cnt = index_min ; cell_cnt <= index_max ; cell_cnt++)
                    {
                        sigma += exp(output_data[(anchor_cnt * num_lane) + lane_cnt + (num_lane * num_anchor * cell_cnt)] - max_output);
                    }

                    for(cell_cnt = index_min ; cell_cnt <= index_max ; cell_cnt++)
                    {
                        result += ((exp(output_data[(anchor_cnt * num_lane) + lane_cnt + (num_lane * num_anchor * cell_cnt)] - max_output) / sigma) * (float)cell_cnt);
                    }

                    if(anchor_type == ROW_ANCHOR)
                    {
                        final_x_point = (round(result) / (float)(num_cell-1)) * (float)target_w;
                        final_y_point = g_ufld_net_info->lane_row_anchor[anchor_cnt] * (float)target_h;
                    }
                    else if(anchor_type == COL_ANCHOR)
                    {
                        final_y_point = (round(result) / (float)(num_cell-1)) * (float)target_h;
                        final_x_point = g_ufld_net_info->lane_col_anchor[anchor_cnt] * (float)target_w;
                    } 
                    else
                    {
                        printf("UFLD post-process error: not valid anchor type!! \n");
                    }

                    idx = out_cnn_results->lane_det[lane_cnt].point_cnt;
                    out_cnn_results->lane_det[lane_cnt].point[idx].x = final_x_point;
                    out_cnn_results->lane_det[lane_cnt].point[idx].y = final_y_point;
                    out_cnn_results->lane_det[lane_cnt].point_cnt++;
#ifdef USE_UFLD_NETWORK_DEBUGGING
                    idx = out_cnn_results->lane_det[lane_cnt].dbg_info_cnt;
#if defined(DBG_ROW_ANCHOR) 
                    if(anchor_type == ROW_ANCHOR)
                    {
                        out_cnn_results->lane_det[lane_cnt].ufld_dbg_info[idx].x = ((float)max_index / (float)(num_cell-1)) * (float)target_w;
                        out_cnn_results->lane_det[lane_cnt].ufld_dbg_info[idx].y = g_ufld_net_info->lane_row_anchor[anchor_cnt] * (float)target_h;
                        out_cnn_results->lane_det[lane_cnt].ufld_dbg_info[idx].index_min = ((float)index_min / (float)(num_cell-1)) * (float)target_w;
                        out_cnn_results->lane_det[lane_cnt].ufld_dbg_info[idx].index_max = ((float)(index_max+1) / (float)(num_cell-1)) * (float)target_w;
                        out_cnn_results->lane_det[lane_cnt].ufld_dbg_info[idx].final_point.x = final_x_point;
                        out_cnn_results->lane_det[lane_cnt].ufld_dbg_info[idx].final_point.y = final_y_point;
                    }
#elif defined(DBG_COL_ANCHOR)
                    if(anchor_type == COL_ANCHOR)
                    {
                        out_cnn_results->lane_det[lane_cnt].ufld_dbg_info[idx].y = ((float)max_index / (float)(num_cell-1)) * (float)target_h;
                        out_cnn_results->lane_det[lane_cnt].ufld_dbg_info[idx].x = g_ufld_net_info->lane_col_anchor[anchor_cnt] * (float)target_w;
                        out_cnn_results->lane_det[lane_cnt].ufld_dbg_info[idx].index_min = ((float)index_min / (float)(num_cell-1)) * (float)target_h;
                        out_cnn_results->lane_det[lane_cnt].ufld_dbg_info[idx].index_max = ((float)(index_max+1) / (float)(num_cell-1)) * (float)target_h;
                        out_cnn_results->lane_det[lane_cnt].ufld_dbg_info[idx].final_point.x = final_x_point;
                        out_cnn_results->lane_det[lane_cnt].ufld_dbg_info[idx].final_point.y = final_y_point;
                    }
#endif
                    out_cnn_results->lane_det[lane_cnt].dbg_info_cnt++;
#endif
                }
            }
        }
    }
}

static void nc_ufld_decide_lane_class(cnn_output_info *cnn_output, stCnnPostprocessingResults *out_cnn_results)
{
    uint8_t *lane_class = (uint8_t*)cnn_output->tiled_data;

    for(int i=0; i< (int)(cnn_output->width); i++)
    {
        if(out_cnn_results->lane_det[i].point_cnt > 0){
            out_cnn_results->lane_det[i].lane_class = (int)lane_class[i];
        }
    }
}

static void nc_postprocess_ufld_detections(unsigned int target_w, unsigned int target_h, cnn_output_info *cnn_output, stCnnPostprocessingResults *out_cnn_results)
{
    static lane_data ld;

    switch(cnn_output->index_of_total) {
        case 0:  // 0 max_row_indices
            ld.max_indices = (uint8_t *)malloc(cnn_output->width * cnn_output->height * sizeof(uint8_t));
            memset(ld.max_indices, 0, cnn_output->width * cnn_output->height * sizeof(uint8_t));
            memcpy(ld.max_indices, cnn_output->tiled_data, cnn_output->width * cnn_output->height * sizeof(uint8_t));
            break;

        case 1:  // 1 valid_row
            ld.valid_anchor = (uint8_t *)malloc(cnn_output->width * cnn_output->height * sizeof(uint8_t));
            memset(ld.valid_anchor, 0, cnn_output->width * cnn_output->height * sizeof(uint8_t));
            memcpy(ld.valid_anchor, cnn_output->tiled_data, cnn_output->width * cnn_output->height * sizeof(uint8_t));
            break;

        case 2:  // 2 out_row
            nc_ufld_detections(&ld, ROW_ANCHOR, target_w, target_h, cnn_output, out_cnn_results);
            free(ld.max_indices);
            free(ld.valid_anchor);
            break;

        case 3:  // 3 max_col_indices
            ld.max_indices = (uint8_t *)malloc(cnn_output->width * cnn_output->height * sizeof(uint8_t));
            memset(ld.max_indices, 0, cnn_output->width * cnn_output->height * sizeof(uint8_t));
            memcpy(ld.max_indices, cnn_output->tiled_data, cnn_output->width * cnn_output->height * sizeof(uint8_t));
            break;

        case 4: // 4 valid_col
            ld.valid_anchor = (uint8_t *)malloc(cnn_output->width * cnn_output->height * sizeof(uint8_t));
            memset(ld.valid_anchor, 0, cnn_output->width * cnn_output->height * sizeof(uint8_t));
            memcpy(ld.valid_anchor, cnn_output->tiled_data, cnn_output->width * cnn_output->height * sizeof(uint8_t));
            break;

        case 5:  // 5 out_col
            nc_ufld_detections(&ld, COL_ANCHOR, target_w, target_h, cnn_output, out_cnn_results);
            free(ld.max_indices);
            free(ld.valid_anchor);
            break;

        case 6: // 6 class
            nc_ufld_decide_lane_class(cnn_output, out_cnn_results);

#ifdef USE_UFLD_TRACKING
            // nc_ufld_tracking_v1(out_cnn_results);
            nc_ufld_tracking_v2(out_cnn_results, target_w);
#endif
            break;

        default:
            printf("UFLD post-process error: not valid tensor index!!\n");
            break;
    }
}

// #define PRINT_POST_PROCESS_TIME
// #define PRINT_AVERAGE_PROCESS_TIME
void postprocess_ufld_detections(unsigned int canvas_w, unsigned int canvas_h, cnn_output_info *cnn_output)
{
    #ifdef PRINT_AVERAGE_PROCESS_TIME
    static uint32_t framecnt = 0;
    uint64_t start_time = nc_get_mono_time();
    static uint64_t elapsed_ms = 0;
    #endif
    #ifdef PRINT_POST_PROCESS_TIME
    static uint64_t start_time;
    #endif

    uint64_t time_stamp = 0;
    int buf_write_idx = 0;
    static pp_result_buf *pp_buf = NULL;
    static stCnnPostprocessingResults *ufld_results = NULL;
    if (cnn_output->index_of_total == 0) {
        #ifdef PRINT_POST_PROCESS_TIME
        start_time = nc_get_mono_us_time();
        #endif
        // get gui writable buffer & index from flip-flop(double) buffers
        pp_buf = (pp_result_buf *)nc_tsfs_ff_get_writable_buffer_and_set_timestamp(cnn_output->cam_ch+LANE_NETWORK, &buf_write_idx, time_stamp);
        ufld_results = &(pp_buf->cnn_result);
        memset(ufld_results, 0, sizeof(stCnnPostprocessingResults));
    }
    nc_postprocess_ufld_detections(canvas_w, canvas_h, cnn_output, ufld_results);

    // draw after all tensor output(per frame) processed
    if ((cnn_output->index_of_total+1) == cnn_output->total_tensor_cnt && pp_buf != NULL) {
        pp_buf->time_stamp = time_stamp;
        pp_buf->lane_draw_info.max_lane_num = g_ufld_net_info->lane_max_num;
        pp_buf->lane_draw_info.index_colors = (stRGB24*)g_ufld_net_info->lane_draw_color;
#ifdef USE_UFLD_NETWORK_DEBUGGING
        pp_buf->lane_draw_info.lane_anchor_info = g_ufld_net_info->lane_anchor_info;
        pp_buf->lane_draw_info.row_anchor = g_ufld_net_info->lane_row_anchor;
        pp_buf->lane_draw_info.col_anchor = g_ufld_net_info->lane_col_anchor;
        pp_buf->lane_draw_info.row_anchor_num = g_ufld_net_info->lane_row_anchor_num;
        pp_buf->lane_draw_info.col_anchor_num = g_ufld_net_info->lane_col_anchor_num;
        pp_buf->lane_draw_info.row_anchor_min = g_ufld_net_info->lane_row_anchor_min;
        pp_buf->lane_draw_info.row_anchor_max = g_ufld_net_info->lane_row_anchor_max;
        pp_buf->lane_draw_info.col_anchor_min = g_ufld_net_info->lane_col_anchor_min;
        pp_buf->lane_draw_info.col_anchor_max = g_ufld_net_info->lane_col_anchor_max;
        pp_buf->lane_draw_info.row_cell_num = g_ufld_net_info->lane_row_cell_num;
        pp_buf->lane_draw_info.col_cell_num = g_ufld_net_info->lane_col_cell_num;
#endif
        pp_buf->cam_channel = cnn_output->cam_ch;
        pp_buf->net_id = cnn_output->net_id;
        pp_buf->net_task = LANE;
        nc_tsfs_ff_finish_write_buf(cnn_output->cam_ch+LANE_NETWORK);
        #ifdef PRINT_POST_PROCESS_TIME
        printf("p_t = %lu\n",nc_elapsed_us_time(start_time));
        #endif
    }

    #ifdef PRINT_AVERAGE_PROCESS_TIME
    framecnt++;
    elapsed_ms += nc_elapsed_time(start_time);
    if (framecnt % (30 * 30) == 0) {
        printf("[%s][frame:%08u] average cnn time :\t%lf ms\n", __FUNCTION__, framecnt, elapsed_ms/(double)framecnt);
    }
    #endif
}



/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                    for post-process ufld inference result                   //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////
int nc_postprocess_ufld_inference_result(unsigned int canvas_w, unsigned int canvas_h, struct inference_result_msg *inference_res)
{
    int ret = 0;

    if((inference_res->network_id != NETWORK_UFLD_LANE) && (inference_res->network_id != NETWORK_TRI_CHIMERA)) {
        return -1;
    }

    g_ufld_net_info = nc_cnn_get_network_info(inference_res->network_id);
    if (g_ufld_net_info == NULL){
        printf("%s, nc_cnn_get_network_info fail. net_id:%d\n", __FUNCTION__, inference_res->network_id);
        return -1;
    }

    postprocess_ufld_detections(canvas_w, canvas_h, &(inference_res->cnn_output));
    return ret;
}