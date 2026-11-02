/**
********************************************************************************
* Copyright (C) 2024 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : nc_cnn_draw_cairo.c
*
* @brief   : nc_cnn_draw_cairo source
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <mqueue.h>
#include <stdint.h>
#include <math.h>

#include "cairo.h"
#include "nc_utils.h"
#include "nc_logo.h"
#include "nc_cnn_aiware_runtime.h"
#include "nc_ts_fsync_flipflop_buffers.h"
#include "nc_ts_fsync_circular_buffers.h"
#include "nc_opencv_wrapper.h"
#include "nc_cnn_segmentation_postprocess.h"
#include "nc_cnn_draw_cairo.h"

// #define SHOW_FPS
// #define SHOW_CNN_THRESHOLD

#ifdef SHOW_FPS
float calc_fps_at_loop_ent(int update_period_fcnt)
{
    static uint64_t s_time = 0;
    static float fps = 0.f;
    static uint64_t fcnt = 0;
    uint64_t elapsed_ms = 0;
    fcnt++;
    if (fcnt % update_period_fcnt == 1) {
        if (s_time == 0) {
            s_time = nc_get_mono_time();
        } else {
            elapsed_ms = nc_elapsed_time(s_time);
            // printf("fcnt(%d) elapsed_ms(%d)\n", fcnt, elapsed_ms);
            if (elapsed_ms > 0) fps = (float)((fcnt-1) / (elapsed_ms/1000.f));

            // re-init
            fcnt = 1;
            s_time = nc_get_mono_time();
        }
    }

    return fps;
}
#endif


/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                               cairo control                                 //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////
static cairo_t* cr;
static cairo_t* cairo[FIXED_FSYNC_FLIP_FLOP_BUF_CNT] = {NULL, };
#ifdef USE_GUI_CQ_BUFFER
unsigned char* gui_buffer = NULL;
#endif
void nc_cairo_init_cavas (unsigned int  canvas_w, unsigned int canvas_h)
{
    // init canvas
    printf("GUI WIDTH: %d, HEIGHT: %d\n", canvas_w, canvas_h);
#ifdef USE_GUI_CQ_BUFFER
    // int gui_buf_size = canvas_w * canvas_h * 4;
    gui_buffer = (unsigned char*)malloc(canvas_w * canvas_h * 4);
    cr = cairo_create( cairo_image_surface_create_for_data( gui_buffer,
                                                            CAIRO_FORMAT_ARGB32, canvas_w, canvas_h, canvas_w*4) );
#else
    for (int i = 0; i < FIXED_FSYNC_FLIP_FLOP_BUF_CNT; i++) {
        cairo[i] = cairo_create( cairo_image_surface_create_for_data(
                                    (unsigned char *)nc_tsfs_ff_get_addr_of_buffer(UID_TSB_GUI, i),
                                    CAIRO_FORMAT_ARGB32, canvas_w, canvas_h, canvas_w*4) );
    }
#endif
}

cairo_t* nc_cairo_clear_and_ready_writebuffer (uint64_t time_stamp)
{
    int gui_write_idx = 0;

#ifndef USE_GUI_CQ_BUFFER
    // get gui writable buffer & index from flip-flop(double) buffers
    nc_tsfs_ff_get_writable_buffer_and_set_timestamp(UID_TSB_GUI, &gui_write_idx, time_stamp);

    // prepare & clear gui buffer(cairo canvas)
    cr = cairo[gui_write_idx];
#endif

    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    return cr;
}

void nc_cairo_draw_extended_infos_and_logo (uint64_t framecnt)
{
    unsigned char text[128] = {0,};

#ifdef SHOW_FPS
    float fps = 0.0f;
    fps = calc_fps_at_loop_ent(30);
    // printf("[%s] ....... fps(%0.1f) ........\n", __FUNCTION__, fps);
#endif
    float ratio = 0.8f;//canvas_w/1280.f;

    // show frame count
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 20*ratio);
    cairo_set_source_rgba(cr, 0, 0, 1.0, 1.0);
    cairo_move_to(cr, 20, 22);

#ifdef SHOW_FPS
    sprintf((char *)text, "Frame Count : %lu FPS : %0.1f", framecnt, fps);
#else
    sprintf((char *)text, "Frame Count : %lu", framecnt);
#endif

    cairo_show_text(cr, (const char *)text);

#ifdef SHOW_CNN_THRESHOLD
    unsigned char text2[128] = {0,};
    cairo_move_to(cr, 10, 45);
    sprintf((char *)text2, "Objectness-TH(%0.1f) Detection-TH(%0.1f)", PELEE_CONFIDENCE_OBJECTNESS_THRESHOLD, PELEE_CONFIDENCE_DETECTION_THRESHOLD);
    cairo_show_text(cr, (const char *)text2);
#endif
}

void nc_cairo_finish_draw_writebuffer(void)
{
#ifndef USE_GUI_CQ_BUFFER
    nc_tsfs_ff_finish_write_buf(UID_TSB_GUI);
#endif
}

void nc_cairo_destroy (void)
{
#ifndef USE_GUI_CQ_BUFFER
    for (int i = 0; i < FIXED_FSYNC_FLIP_FLOP_BUF_CNT; i++) {
        if (cairo[i]) cairo_destroy(cairo[i]);
    }
#else
    if (cr) cairo_destroy(cr);
    free(gui_buffer);
#endif
}

/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                           draw object detection                             //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////
// #define PRINT_AVG_DETECTION_DRAW_TIME
void nc_cairo_draw_object_detections (cairo_t *canvas, stObjDrawInfo* drawInfo, stCnnPostprocessingResults *cnn_results)
{
    static int canvas_w = 0;
    static int canvas_h = 0;
    static float ratio = 1.f;
    if (canvas_w == 0 || canvas_h == 0) {
        cairo_surface_t* cr_surface = cairo_get_target(canvas);
        canvas_w = cairo_image_surface_get_width(cr_surface);
        canvas_h = cairo_image_surface_get_height(cr_surface);
        //ratio = (float)canvas_w/1280;
        ratio = (float)canvas_w/1920;
    }

    char text[128] = {'\0',};
    cairo_select_font_face(canvas, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(canvas, 21*ratio);
    cairo_set_line_width(canvas, 2.0*ratio);

#ifdef PRINT_AVG_DETECTION_DRAW_TIME
    static uint64_t fcnt = 0;
    static uint64_t t_time = 0;
    uint64_t s_time = nc_get_mono_us_time();
    uint64_t el_time = 0;
#endif
    for (int i = 0; i < drawInfo->max_class_cnt; i++) {
        for (int bidx = 0; bidx < cnn_results->class_objs[i].obj_cnt; bidx++) {
            stObjInfo obj_info = cnn_results->class_objs[i].objs[bidx];
            if (obj_info.bbox.w >= 3 && obj_info.bbox.h >= 3)
            {
            #ifdef SHOW_CNN_PROBABILITY
                // show cnn probability (not track id)
                if (obj_info.track_id < 0) sprintf((char *)text, "%s:%0.2f", drawInfo->class_names[i], obj_info.prob);
                else sprintf((char *)text, "%s:%0.2f", drawInfo->class_names[i], obj_info.prob);
            #else
                // show track id (not cnn probability)
                if (obj_info.track_id < 0) sprintf((char *)text, "%s", drawInfo->class_names[i]);
                else sprintf((char *)text, "%s:%d", drawInfo->class_names[i], obj_info.track_id);
            #endif

                cairo_set_source_rgb (canvas, drawInfo->class_colors[i].r, drawInfo->class_colors[i].g, drawInfo->class_colors[i].b);
                cairo_rectangle(canvas, obj_info.bbox.x, obj_info.bbox.y, obj_info.bbox.w, obj_info.bbox.h);

                double lx, ly;
                cairo_text_extents_t c_ext;
                cairo_text_extents(canvas, text, &c_ext);

    #ifdef SHOW_CLASS_LABEL_BOX
                double lw, lh;
                cairo_stroke (canvas);
                lh = c_ext.height+4;
                lw = c_ext.width+4;

            #if 1
                lx = obj_info.bbox.x-ratio;
                ly = obj_info.bbox.y-lh;
                // if (ly - lh < 0) ly = obj_info.bbox.y + obj_info.bbox.h;
            #else
                if (obj_info.bbox.w >= lw) {
                    lx = obj_info.bbox.x+obj_info.bbox.w-lw;
                    // lx = obj_info.bbox.x;
                    ly = obj_info.bbox.y-lh;
                }
                else {
                    lx = obj_info.bbox.x;
                    ly = obj_info.bbox.y-lh;
                }
            #endif

                cairo_rectangle(canvas, lx, ly, lw, lh);
                cairo_fill (canvas);

                cairo_set_source_rgb (canvas, 1.0-drawInfo->class_colors[i].r, 1.0-drawInfo->class_colors[i].g, 1.0-drawInfo->class_colors[i].b);
                cairo_move_to(canvas, lx+1, ly + c_ext.height+1);
                cairo_show_text(canvas, (char *)text);
                cairo_stroke (canvas);
    #else
                lx = (obj_info.bbox.x+obj_info.bbox.w/2)-(c_ext.width/2);
                ly = (obj_info.bbox.y+obj_info.bbox.h/2)-(c_ext.height/2);
                cairo_move_to(canvas, lx, ly);
                cairo_show_text(canvas, (char *)text);
                cairo_stroke (canvas);
    #endif
            }
        }
    }
#ifdef PRINT_AVG_DETECTION_DRAW_TIME
    fcnt++;
    el_time = nc_elapsed_us_time(s_time);
    t_time += el_time;
    if (fcnt % 30 == 0) {
        printf("[%s:%d]........... (detection:fcnt(%llu)) avg draw elapsed : %llu us\n", __FUNCTION__, __LINE__, fcnt, t_time/fcnt);
    }
#endif
}


/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                              draw segmentation                              //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////
#if 0
void nc_opencv_draw_freespace(unsigned int canvas_w, unsigned int canvas_h, unsigned int *canvas, cnn_output_info cnn_output)
{
    uint32_t *npu_canvas;
    struct img_info src_img, dst_img;

    npu_canvas = (uint32_t *)malloc(cnn_output.width*cnn_output.height*sizeof(uint32_t));
    nc_tiled_to_scanline(cnn_output, (unsigned int *)npu_canvas);

    // source image <== npu output
    src_img.width = cnn_output.width;
    src_img.height = cnn_output.height;
    src_img.buff = (uint8_t *)npu_canvas;

    // destination image <== canvas_ptr for display
    dst_img.width = canvas_w;
    dst_img.height = canvas_h;
    dst_img.buff = (uint8_t *)canvas;
    nc_opencv_resize(&src_img, &dst_img);
    free(npu_canvas);
}
#endif

void nc_cairo_draw_argmax_freespace(unsigned int canvas_w, unsigned int canvas_h, cairo_t *cr, cnn_output_info cnn_output)
{
    double h_ratio_coeff;
    double y_pos;
    double v_ratio_coeff;
    uint32_t i = 0;

    uint32_t* freespace = (uint32_t*)malloc(cnn_output.width * sizeof(uint32_t));
    if (freespace == NULL) {
        printf("Failed to allocate freespace memory");
        exit(EXIT_FAILURE);
    }

    memset(freespace, 0, cnn_output.width * sizeof(uint32_t));

    h_ratio_coeff = (double)(canvas_w / cnn_output.width);
    v_ratio_coeff = (double)(canvas_h / cnn_output.height);
    nc_argmax_opt2((unsigned char*)cnn_output.tiled_data, cnn_output.width, cnn_output.height, (uint8_t *)freespace);

    // draw freespace
    //printf("ratio_coeff: %f, %d\n", v_ratio_coeff, h_ratio_coeff);
    cairo_set_source_rgba(cr, 0, 0, 1, 0.5);
    cairo_set_line_width(cr, h_ratio_coeff);
    for (i = 0 ; i < cnn_output.width ; i++) {
        y_pos = ((double)freespace[i] * v_ratio_coeff);
        if (y_pos > 0) {
            cairo_move_to(cr, (double)i*h_ratio_coeff, y_pos);
            cairo_line_to(cr, (double)i*h_ratio_coeff, canvas_h-1);
        }
    }
    cairo_stroke(cr);

    free(freespace);
}

void nc_direct_draw_freespace_to_canvas(unsigned int canvas_w, unsigned int canvas_h, unsigned int *canvas, cnn_output_info cnn_output)
{
    nc_tiled_to_scanline_n_scale_up(cnn_output, canvas_w, canvas_h, canvas);
}
