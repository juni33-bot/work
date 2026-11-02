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
* @file    : nc_cnn_segmentation_postprocess.c
*
* @brief   : nc_cnn_segmentation_postprocess source
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include "stdio.h"

#include "nc_cnn_config_parser.h"
#include "nc_ts_fsync_flipflop_buffers.h"
#include "nc_utils.h"
#include "nc_cnn_segmentation_postprocess.h"
#define DRAW_FREESPACE_WITHOUT_BG       // increase drawing speed (not drawing background)

/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                             for segmentation                                //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////
static stNetwork_info* g_netinfo;
static void nc_postprocess_segmentation(cnn_output_info *cnn_output, stCnnPostprocessingResults *out_cnn_results)
{
    unsigned int output_w = cnn_output->width;
    unsigned int output_h = cnn_output->height;
    static char* segs[4] = {NULL, NULL, NULL, NULL};

    if (cnn_output->cam_ch < 4) {
        if (segs[cnn_output->cam_ch] == NULL) {
            segs[cnn_output->cam_ch] = (char*)malloc(sizeof(char) * output_w * output_h);
        }
        memcpy(segs[cnn_output->cam_ch], (char*)cnn_output->tiled_data, sizeof(char) * output_w * output_h);
        out_cnn_results->seg = segs[cnn_output->cam_ch];
    }

}
// #define PRINT_AVERAGE_PROCESS_TIME
void postprocess_freespace(struct cnn_output_info *cnn_output)
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
    static stCnnPostprocessingResults *seg_results = NULL;
    int segnum = (int)g_netinfo->seg_class_num;
    if (cnn_output->index_of_total == 0) {
    // get gui writable buffer & index from flip-flop(double) buffers
        pp_buf = (pp_result_buf *)nc_tsfs_ff_get_writable_buffer_and_set_timestamp(cnn_output->cam_ch+SEGMENT_NETWORK, &buf_write_idx, time_stamp);
        seg_results = &(pp_buf->cnn_result);
    }
    nc_postprocess_segmentation(cnn_output, seg_results);
    if ((cnn_output->index_of_total+1) == cnn_output->total_tensor_cnt && pp_buf != NULL) {
        pp_buf->time_stamp = time_stamp;
        pp_buf->seg_info.width = cnn_output->width;
        pp_buf->seg_info.height = cnn_output->height;
        pp_buf->seg_info.max_class_cnt = (int)segnum;
        pp_buf->seg_info.class_names = (char**)NULL;
        pp_buf->seg_info.class_colors = (stRGBA32*)g_netinfo->seg_class_color;
        pp_buf->cam_channel = cnn_output->cam_ch;
        pp_buf->net_id = cnn_output->net_id;
        pp_buf->net_task = SEGMENTATION;

        nc_tsfs_ff_finish_write_buf(cnn_output->cam_ch+SEGMENT_NETWORK);
    }
    #ifdef PRINT_AVERAGE_PROCESS_TIME
    framecnt++;
    elapsed_ms += nc_elapsed_time(start_time);
    if (framecnt % (30 * 30) == 0) {
        printf("[%s][frame:%08u] average postprocess time :\t%lf ms\n", __FUNCTION__, framecnt, elapsed_ms/(double)framecnt);
    }
    #endif

    #ifdef PRINT_DRAW_TIME
    elapsed_ms = nc_elapsed_time(start_time);
    printf("draw time (free space) : %llu ms%s\n", elapsed_ms, (elapsed_ms > 33)?" > 33 ms":"");
    #endif
}

int nc_postprocess_segmentation_inference_result(unsigned int canvas_w, unsigned int canvas_h, struct inference_result_msg *inference_res)
{
    (void) canvas_w;
    (void) canvas_h;
    int ret = 0;
    
    if((inference_res->network_id != NETWORK_PELEE_SEG) && (inference_res->network_id != NETWORK_TRI_CHIMERA)) {
        return -1;
    }
    
    g_netinfo = nc_cnn_get_network_info(inference_res->network_id);
    if (g_netinfo == NULL){
        printf("%s, nc_cnn_get_network_info fail. net_id:%d\n", __FUNCTION__, inference_res->network_id);
        return -1;
    }

    postprocess_freespace(&(inference_res->cnn_output));

    return ret;
}

void nc_rgb_blend_n_scale_up(cnn_output_info cnn_output, unsigned int canvas_w, unsigned int canvas_h, unsigned char *canvas)
{
    int outidx = 0;
    uint32_t in_x_idx, in_y_idx;
    uint32_t xpos, ypos;
    float h_ratio_coeff = (float)canvas_w / (float)cnn_output.width;
    float v_ratio_coeff = (float)canvas_h / (float)cnn_output.height;
    unsigned char* tiled_data = (unsigned char*)cnn_output.tiled_data;
    char need_blend;
    unsigned char r_value, g_value, b_value;

    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t x_up = 0;
    uint32_t y_up = 0;

    for (y=0; y<cnn_output.height; y++)
    {
        in_y_idx = y;
        for (x=0; x<cnn_output.width; x++)
        {
            in_x_idx = x;
            switch (tiled_data[outidx]) {
            case 0x00: // road
                need_blend = 1;
                r_value = 0x00;
                g_value = 0x00;
                b_value = 0xFF;
                break;
            case 0x01: // background
                need_blend = 0;
                break;
            default:
                need_blend = 0;
                printf("warning unknown output pixel value %d\n", tiled_data[outidx]);
                break;
            }

            if (need_blend)
            {
                ypos = (uint32_t)((float)in_y_idx * v_ratio_coeff);
                if (ypos > 0) {
                    xpos = (uint32_t)((float)in_x_idx * h_ratio_coeff);
                    for ( y_up = 0; y_up < (uint32_t)h_ratio_coeff; y_up++) {
                        for ( x_up = 0; x_up < (uint32_t)h_ratio_coeff; x_up++) {
                            canvas[((ypos + y_up) * canvas_w + (xpos + x_up))*3 + 0] = (unsigned char)((canvas[((ypos + y_up) * canvas_w + (xpos + x_up))*3 + 0]) + (r_value)) / 2;
                            canvas[((ypos + y_up) * canvas_w + (xpos + x_up))*3 + 1] = (unsigned char)((canvas[((ypos + y_up) * canvas_w + (xpos + x_up))*3 + 1]) + (g_value)) / 2;
                            canvas[((ypos + y_up) * canvas_w + (xpos + x_up))*3 + 2] = (unsigned char)((canvas[((ypos + y_up) * canvas_w + (xpos + x_up))*3 + 2]) + (b_value)) / 2;
                        }
                    }
                }
            }

            outidx++;
        }
    }
}

void nc_tiled_to_scanline_n_scale_up(struct cnn_output_info cnn_output, unsigned int canvas_w, unsigned int canvas_h, unsigned int *canvas)
{
    unsigned int value;
    uint32_t in_x_idx, in_y_idx;
    uint32_t xpos, ypos;
    int outidx = 0;
    float h_ratio_coeff = (float)canvas_w / (float)cnn_output.width;
    float v_ratio_coeff = (float)canvas_h / (float)cnn_output.height;
    unsigned int range = 8;
    unsigned int width_ratio,height_ratio,width_compare,height_compare;
    //printf("ratio: %d, y_offset: %d\n", ratio, y_offset);
    width_ratio = cnn_output.width / TILESIZE;
    height_ratio = cnn_output.height / TILESIZE;
    width_compare = cnn_output.width - TILESIZE * width_ratio;
    height_compare = cnn_output.height - TILESIZE * height_ratio;
    (width_compare > 0) ? width_ratio++ : width_ratio;
    (height_compare > 0) ? height_ratio++ : height_ratio;
    unsigned char* tiled_data = (unsigned char*)cnn_output.tiled_data;

    uint32_t x_super = 0, y_super = 0;
    uint32_t x = 0, y = 0;
    uint32_t x_sub = 0, y_sub = 0;
    uint32_t x_up = 0, y_up = 0;

    for ( y_super = 0; y_super < height_ratio ; y_super++) {
        for ( x_super = 0; x_super < width_ratio; x_super++) {
            for ( y_sub = 0; y_sub < range; y_sub++) {
                    if(y_super == (cnn_output.height / TILESIZE))
                    {
                        if(y_sub >= height_compare/range)
                        {
                            outidx+=TILESIZE*range;
                            continue;
                        }
                    }
                for ( x_sub = 0; x_sub < range; x_sub++) {
                    if(x_super == (cnn_output.width / TILESIZE))
                    {
                        if(x_sub >= width_compare/range)
                        {
                            outidx+=TILESIZE;
                            continue;
                        }
                    }
                    for (y = 0; y < range; y++) {
                        in_y_idx = y_super * TILESIZE + y_sub * range + y;
                        for (x = 0; x < range; x++) {
#ifdef DRAW_FREESPACE_WITHOUT_BG
                            // increase drawing speed (not drawing background)
                            if (tiled_data[outidx] == 0x01) {
                                outidx++;
                                continue;
                            }
#endif
                            in_x_idx = x_super * TILESIZE + x_sub * range + x;
                            switch (tiled_data[outidx]) {
                            case 0x00: // road
                                value = 0x400000FFUL;
                                break;
                            case 0x01: // background
                                value = 0x00000000UL;
                                break;
                            default:
                                printf("warning unknown output pixel value %d\n", tiled_data[outidx]);
                                break;
                            }
                            ypos = (uint32_t)((float)in_y_idx * v_ratio_coeff);
                            if (ypos > 0) {
                                xpos = (uint32_t)((float)in_x_idx * h_ratio_coeff);
                                for ( y_up = 0; y_up < (uint32_t)h_ratio_coeff; y_up++) {
                                    for ( x_up = 0; x_up < (uint32_t)h_ratio_coeff; x_up++) {
                                        canvas[(ypos + y_up) * canvas_w + (xpos + x_up)] = value;
                                    }
                                }
                            }
                            outidx++;
                        }
                    }
                }
            }
        }
    }
}

void nc_tiled_to_scanline(struct cnn_output_info cnn_output, unsigned int *scanline)
{
    uint32_t value = 0;
    uint32_t in_x_idx, in_y_idx;
    int32_t outidx = 0;

    uint32_t width = cnn_output.width;
    uint32_t height = cnn_output.height;
    unsigned char *tiled = (unsigned char*)cnn_output.tiled_data;

    uint32_t x_super = 0, y_super = 0;
    uint32_t x_sub = 0, y_sub = 0;
    uint32_t x = 0, y = 0;

    //printf("v_ratio: %f, h_ratio: %f\n", v_ratio, h_ratio);
    for (y_super = 0; y_super < height / TILESIZE; y_super++) {
        for (x_super = 0; x_super < width / TILESIZE; x_super++) {
            for (y_sub = 0; y_sub < 8; y_sub++) {
                for (x_sub = 0; x_sub < 8; x_sub++) {
                    for (y = 0; y < 8; y++) {
                        in_y_idx = y_super * TILESIZE + y_sub * 8 + y;
                        for (x = 0; x < 8; x++) {
                            in_x_idx = x_super * TILESIZE + x_sub * 8 + x;
                            switch (tiled[outidx]) {
                            case 0x00: // road
                                value = 0x800000FFUL;
                                break;
                            case 0x01: // background
                                value = 0x00000000UL;
                                break;
                            default:
                                printf("warning unknown output pixel value %d\n", tiled[outidx]);
                                break;
                            }
                            scanline[in_y_idx * width + in_x_idx] = value;
                            outidx++;
                        }
                    }
                }
            }
        }
    }
}

void nc_argmax_opt2(const uint8_t *input, uint32_t npu_out_w, uint32_t npu_out_h, uint8_t *output)
{
    typedef int8_t Type;
    uint32_t *overlay_u32 = (uint32_t *)output;
    const uint32_t plane_size = npu_out_w * npu_out_h;
    const Type *in = (const Type *)&input[plane_size * 0];
    const uint32_t h_tile_cnt = (npu_out_w + TILESIZE - 1) / TILESIZE;

    memset(overlay_u32, 0, npu_out_w * sizeof(uint32_t));

    for (uint32_t x = 0; x < npu_out_w; x += CELLSIZE) {
        uint32_t b = (x / TILESIZE) * TILESIZE_SQR;
        uint32_t d = (x % TILESIZE) / CELLSIZE * CELLSIZE_SQR;
        for (uint32_t y = 0; y < npu_out_h; y += CELLSIZE) {
            uint32_t a = (y / TILESIZE) * TILESIZE_SQR * h_tile_cnt;
            uint32_t c = (y % TILESIZE) / CELLSIZE * (TILESIZE / CELLSIZE) * CELLSIZE_SQR;
            for (uint8_t cx = 0; cx < CELLSIZE; ++cx) {
                if (overlay_u32[(x + cx)] == 0) {
                    for (uint8_t cy = 0; cy < CELLSIZE; ++cy) {
                        if (in[a + b + c + d + cy * CELLSIZE + cx] == 0x00) {    // road
                            overlay_u32[(x + cx)] = (y + cy);
                            break;
                        }
                    }
                }
            }
        }
    }
}

#if 0
/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                      linked list for segmentation                           //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////
stSegInfo* nc_create_seginfo_topnode(int x, int y, uint32_t val)
{
    stSegInfo* top = (stSegInfo*)malloc(sizeof(stSegInfo));
    top->next = NULL;
    top->pos.x = x;
    top->pos.y = y;
    top->val = val;

    return top;
}

stSegInfo* nc_create_seginfo_node(stSegInfo* parent, int x, int y, uint32_t val)
{
    if (!parent) {
        printf("[Error] %s - parent is NULL\n", __FUNCTION__);
        return NULL;
    }
    else {
        // printf("....... create next node ..........\n");
        stSegInfo* node = (stSegInfo*)malloc(sizeof(stSegInfo));
        node->next = NULL;
        node->pos.x = x;
        node->pos.y = y;
        node->val = val;
        parent->next = node;

        return node;
    }
}

int nc_get_seg_list_cnt(stSegInfo* head)
{
    int cnt = 0;
    if(!head) {
        cnt = 0;
    }
    else {
        cnt++; // head self count
        while(head->next) {
            cnt++;
            head = head->next;
        }
    }

    return cnt;
}

void nc_print_seg_list(stSegInfo* head)
{
    int cnt = 0;
    if(!head) {
        return;
    }
    else {
        printf ("[%d] x(%f) y(%f) val(%u)\n", cnt, head->pos.x, head->pos.y, head->val);
        cnt++; // head self count
        while(head->next) {
            head = head->next;
            printf ("[%d] x(%f) y(%f) val(%u)\n", cnt, head->pos.x, head->pos.y, head->val);
            cnt++;
        }
    }
}

void nc_destroy_seginfo_node(stSegInfo* node)
{
    if (!node) {
        printf("[Error] %s - node is NULL\n", __FUNCTION__);
        return;
    }

    if (node->next) {
        nc_destroy_seginfo_node(node->next);
    }
    printf ("..... delete node (%f, %f)\n", node->pos.x, node->pos.y);
    free(node);
    node = NULL;
}

void *destory_linked_list_task(void *data)
{
    if (!data) {
        printf("[Error] %s - data is NULL\n", __FUNCTION__);
        return NULL;
    }

    stSegInfo* head = (stSegInfo*)data;

    nc_destroy_seginfo_node(head);

    return NULL;
}

void nc_launch_destory_linked_list_worker(stSegInfo* head)
{
    if (!head) {
        printf("[Error] %s - head is NULL\n", __FUNCTION__);
        return;
    }

    pthread_t p_thread;
    if (pthread_create(&p_thread, NULL, destory_linked_list_task, (void *)head) < 0) {
        perror("thread create error : destory_linked_list_task");
    }
}
#endif
