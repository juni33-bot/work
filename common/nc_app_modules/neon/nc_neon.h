/**
********************************************************************************
* Copyright (C) 2021 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************
*/
/**
********************************************************************************
* @file    : nc_neon.h
*
* @brief   : neon simd api header
*
* @author  : SW Solution team.  NextChip Inc.
*
* @date    : 2022.09.29.
*
* @version : 1.0.0
********************************************************************************
* @note
* 09.02.2022 / 1.0.0 / Initial released.
*
********************************************************************************
*/

/**
 * nc_neon.h
 *
 */
#ifndef __NC_NEON_H__
#define __NC_NEON_H__

#include <assert.h>
#include "aiware/common/c/binary.h"

#define OK 0
#define ERR -1

int32_t nc_resize_image_bilinear_yuyv(uint8_t* src, uint32_t src_stride, uint32_t src_width, uint32_t src_height,
                                   uint8_t* dst, uint32_t dst_stride, uint32_t dst_width, uint32_t dst_height);
int32_t nc_pip_image_bilinear_yuyv(uint8_t *src, int32_t src_w, int32_t src_h, int32_t trg_w, int32_t trg_h,
                                uint8_t *pip_plane, int32_t pip_w, int32_t pip_h,
                                int32_t x_ofs, int32_t y_ofs);
void nc_ScaleYUY2RowDown2Box_NEON(uint8_t* src_ptr, int32_t src_stride,
                               uint8_t* dst, int32_t dst_width);
void nc_ScaleYUY2Down2Box(uint8_t* src, int32_t src_stride, int32_t src_height,
                       uint8_t* dst, int32_t dst_stride, int32_t dst_width);

int32_t nc_resize_yuy2_halfwidth(uint8_t *yuv_src, uint8_t *yuv_dst, int32_t src_width, int32_t src_height);
int32_t nc_resize_yuy2_hafwid_verti_scaledn(uint8_t *yuv_src, uint8_t *yuv_dst, int32_t src_width, int32_t src_height, int32_t dst_height);

int32_t nc_img_yuyv_packed_to_planar(uint8_t *yuv422Plane, uint8_t *yuy2Plane, int32_t width, int32_t height);
int32_t nc_img_yuyv_planar_to_packed(uint8_t *yuv422Plane, uint8_t *yuy2Plane, int32_t width, int32_t height);
int32_t nc_img_nv12_to_yuv422(uint8_t* yuv_ptr, uint8_t* nv_ptr, int32_t width, int32_t height);
int32_t nc_img_rgb24_packed_to_tiled_planar(uint8_t *rgb_Packed, uint8_t *rgb_Plannar, int32_t width, int32_t height);

void nc_img_vresize_linear_c(const int32_t **src, uint8_t *dst, const int16_t *beta, int32_t width);

/**
 * Mixer : ARGB(8888) x YUYV(422) = YUYV(422)
 */
#define nc_simd_Mix_Rgba2Yuyv  neon_Mix_Rgba2Yuyv
#define nc_simd_Mix_Bgra2Yuyv  neon_Mix_Bgra2Yuyv
void nc_simd_Mix_Bgra2Yuyv(uint8_t *src, uint8_t *dst, int32_t w, int32_t h);
void nc_simd_Mix_Rgba2Yuyv(uint8_t *src, uint8_t *dst, int32_t w, int32_t h);

int nc_neon_get_data_NCHW_float(aiwTensorInfo *tinfo, unsigned char *tiled, float *scanline);
int nc_neon_get_data_NCHW_float_cellrow(aiwTensorInfo *tinfo, unsigned char *tiled, float *scanline);
int nc_neon_get_data_NCHW_uint8_cellrow(aiwTensorInfo *tinfo, unsigned char *tiled, int64_t *scanline);
int nc_neon_tiled_to_scanline_n_scale_up(unsigned int npu_seg_out_w, unsigned int npu_seg_out_h, unsigned int canvas_w, unsigned int canvas_h, unsigned int *canvas, unsigned char *cnn_output);

void nc_rgb_planar_to_interleaved_neon(uint8_t* R, uint8_t* G, uint8_t* B, uint8_t* interleaved, int width, int height);
void nc_rgb_interleaved_to_planar_neon(unsigned char* interleaved, unsigned char* R, unsigned char* G, unsigned char* B, int w, int h);

#endif  // __NC_NEON_H__



