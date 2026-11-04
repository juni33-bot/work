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
* @file    : nc_neon.c
*
* @brief   : frame_mixel api c code
*
* @author  : SW Solution team.  NextChip Inc.
*
* @date    : 2022.09.02.
*
* @version : 1.0.0
********************************************************************************
* @note
* 09.02.2022 / 1.0.0 / Initial released.
*
********************************************************************************
*/


/*
********************************************************************************
*               INCLUDES
********************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arm_neon.h>

#include "nc_neon.h"
#include "nc_types.h"

/*
********************************************************************************
*               DEFINES
********************************************************************************
*/
#define INTER_RESIZE_COEF_BITS (11)
#define INTER_RESIZE_COEF_SCALE (1 << INTER_RESIZE_COEF_BITS)


#define MAX_ESIZE   (16)
#define MIN(a, b)   ((a) > (b) ? (b) : (a))
#define MAX(a, b)   ((a) < (b) ? (b) : (a))

#define VRESIZE_LINEAR_MASK_TABLE_SIZE      (7)
#define BITS                                (INTER_RESIZE_COEF_BITS * 2)
#define DELTA                               (1 << ((INTER_RESIZE_COEF_BITS * 2) - 1))

/*
********************************************************************************
*               FUNCTION DEFINITIONS
********************************************************************************
*/
static inline uint32_t neon_align_size(int32_t sz, int32_t n)
{
    return (sz + n - 1) & (-n);
}
static inline int32_t neon_floor(float a)
{
    return (((a) >= 0) ? ((int32_t) a) : ((int32_t) a - 1));
}
static inline int32_t neon_clip(int32_t x, int32_t a, int32_t b)
{
    return (x >= a ? (x < b ? x : (b - 1)) : a);
}
static inline uint8_t neon_cast_op(int32_t val)
{
    int32_t bits = INTER_RESIZE_COEF_BITS * 2;
    int32_t lvShift = bits;
    int32_t lvDelta = 1 << (bits - 1);
    int32_t temp = MIN (255, MAX (0, (val + lvDelta) >> lvShift));
    return (uint8_t) (temp);
}

static void img_hresize_linear_c(const uint8_t **src,
                                 int32_t **dst,
                                 int32_t count,
                                 const int32_t *xofs,
                                 const int16_t *alpha,
                                 int32_t dwidth,
                                 int32_t cn,
                                 int32_t xmax)
{
    int32_t dx, k;
    int32_t dx0 = 0;

    if(count == 2)
    {
        k = 0;
        const uint8_t *S0 = src[k], *S1 = src[k + 1];
        int32_t *D0 = dst[k], *D1 = dst[k + 1];

        for(dx = dx0; dx < xmax; dx++)
        {
            int32_t sx = xofs[dx];
            int32_t a0 = alpha[dx * 2], a1 = alpha[dx * 2 + 1];
            int32_t t0 = S0[sx] * a0;
            int32_t t1 = S1[sx] * a0;

            t0 += S0[sx + cn] * a1;
            t1 += S1[sx + cn] * a1;
            D0[dx] = t0;
            D1[dx] = t1;
        }

        for(; dx < dwidth; dx++)
        {
            int32_t sx = xofs[dx];
            D0[dx] = (int32_t) S0[sx] * INTER_RESIZE_COEF_SCALE;
            D1[dx] = (int32_t) S1[sx] * INTER_RESIZE_COEF_SCALE;
        }
    }

    if(count == 1)
    {
        k = 0;
        const uint8_t *S = src[k];
        int32_t *D = dst[k];
        for(dx = 0; dx < xmax; dx++)
        {
            int32_t sx = xofs[dx];

            D[dx] = S[sx] * alpha[dx * 2] + S[sx + cn] * alpha[dx * 2 + 1];
        }

        for(; dx < dwidth; dx++)
            D[dx] = (int32_t) S[xofs[dx]] * INTER_RESIZE_COEF_SCALE;
    }
}

void nc_img_vresize_linear_c(const int32_t **src, uint8_t *dst, const int16_t *beta, int32_t width)
{
    int32_t b0 = beta[0], b1 = beta[1];
    const int32_t *S0 = src[0], *S1 = src[1];

    int32_t x = 0;
    for(; x <= ((width / 2) - 4); x += 4)
    {
        int32_t t0, t1;
        t0 = S0[x] * b0 + S1[x] * b1;
        t1 = S0[x + 1] * b0 + S1[x + 1] * b1;
        dst[x] = neon_cast_op(t0);
        dst[x + 1] = neon_cast_op(t1);
        t0 = S0[x + 2] * b0 + S1[x + 2] * b1;
        t1 = S0[x + 3] * b0 + S1[x + 3] * b1;
        dst[x + 2] = neon_cast_op(t0);
        dst[x + 3] = neon_cast_op(t1);
    }

    for(; x < (width/2); x++)
        dst[x] = neon_cast_op (S0[x] * b0 + S1[x] * b1);
}

static void img_resize_cal_offset_linear(int32_t *xofs,
                                         int16_t *ialpha,
                                         int32_t *yofs,
                                         int16_t *ibeta,
                                         int32_t *xmin,
                                         int32_t *xmax,
                                         int32_t ksize,
                                         int32_t ksize2,
                                         int32_t srcw,
                                         int32_t srch,
                                         int32_t dstw,
                                         int32_t dsth,
                                         int32_t channels)
{
    float inv_scale_x = (float) dstw / (float)srcw;
    float inv_scale_y = (float) dsth / (float)srch;

    int32_t cn = channels;
    float scale_x = (float)(1. / inv_scale_x);
    float scale_y = (float)(1. / inv_scale_y);
    int32_t k, sx, sy, dx, dy;


    float fx, fy;

    float cbuf[MAX_ESIZE];


    // horizontal
    for(dx = 0; dx < dstw; dx++)
    {
        fx = (float) (((dx + 0.5) * scale_x) - 0.5);
        sx = neon_floor(fx);
        fx -= (float)sx;

        if(sx < (ksize2 - 1))
        {
            *xmin = dx + 1;
            if(sx < 0)
                fx = 0, sx = 0;
        }

        if((sx + ksize2) >= srcw)
        {
            *xmax = MIN (*xmax, dx);
            if(sx >= (srcw - 1))
                fx = 0, sx = srcw - 1;
        }

        for(k = 0, sx *= cn; k < cn; k++)
            xofs[(dx * cn) + k] = sx + k;

        cbuf[0] = 1.f - fx;
        cbuf[1] = fx;


        for(k = 0; k < ksize; k++)
            ialpha[((dx * cn) * ksize) + k] = (int16_t) (cbuf[k] * INTER_RESIZE_COEF_SCALE);

        for(; k < cn * ksize; k++)
            ialpha[((dx * cn) * ksize) + k] = ialpha[((dx * cn) * ksize) + k - ksize];
    }

    // vertical
    for(dy = 0; dy < dsth; dy++)
    {
        fy = (float) ((dy + 0.5) * scale_y - 0.5);
        sy = neon_floor(fy);
        fy -= (float)sy;

        yofs[dy] = sy;

        cbuf[0] = 1.f - fy;
        cbuf[1] = fy;

        for(k = 0; k < ksize; k++)
            ibeta[(dy * ksize) + k] = (int16_t) (cbuf[k] * INTER_RESIZE_COEF_SCALE);

    }

}

static void img_hresize_4channels_linear_neon(const uint8_t **src, int32_t **dst, int32_t count,
                                              const int32_t *xofs, const int16_t *alpha,
                                              int32_t dwidth, int32_t xmax)
{
    int32_t dx, k;
    int32_t dx0 = 0;

    uint8x8_t dS0_vec, dS1_vec;
    int16x8_t qS0_vec, qS1_vec;
    int16x4_t dS0_0123, dS1_0123;

    int32x4_t qT0_vec, qT1_vec;

    int32_t dx_buf;
    int16x4x2_t alpha_vec_A, alpha_vec_B, alpha_vec_C, alpha_vec_D;

    uint8x8_t dS01_vec, dS02_vec, dS03_vec, dS04_vec;
    int16x8_t qS01_vec, qS02_vec, qS03_vec, qS04_vec;
    int16x4_t dS01_0123, dS01_4567, dS02_0123, dS02_4567, dS03_0123, dS03_4567, dS04_0123, dS04_4567;
    int32x4_t qT01_vec, qT02_vec, qT03_vec, qT04_vec;

    uint8x8_t dS11_vec, dS12_vec, dS13_vec, dS14_vec;
    int16x8_t qS11_vec, qS12_vec, qS13_vec, qS14_vec;
    int16x4_t dS11_0123, dS11_4567, dS12_0123, dS12_4567, dS13_0123, dS13_4567, dS14_0123, dS14_4567;
    int32x4_t qT11_vec, qT12_vec, qT13_vec, qT14_vec;

    int16x4_t dCoeff;
    dCoeff = vdup_n_s16(INTER_RESIZE_COEF_SCALE);

    for(k = 0; k <= count - 2; k++)
    {
        const uint8_t *S0 = src[k], *S1 = src[k + 1];
        int32_t *D0 = dst[k], *D1 = dst[k + 1];

        for(dx = dx0; dx < xmax; dx += 16)
        {
            dx_buf=dx;

            int32_t sx1 = xofs[dx];
            int32_t sx2 = xofs[dx + 4];
            int32_t sx3 = xofs[dx + 8];
            int32_t sx4 = xofs[dx + 12];

            alpha_vec_A = vld2_s16(&alpha[dx * 2]);            // 1 set
            alpha_vec_B = vld2_s16(&alpha[(dx+4) * 2]);        // 2 set
            alpha_vec_C = vld2_s16(&alpha[(dx+8) * 2]);        // 3 set
            alpha_vec_D = vld2_s16(&alpha[(dx+12) * 2]);       // 4 set

            dS01_vec = vld1_u8(&S0[sx1]);
            dS02_vec = vld1_u8(&S0[sx2]);
            dS03_vec = vld1_u8(&S0[sx3]);
            dS04_vec = vld1_u8(&S0[sx4]);
            dS11_vec = vld1_u8(&S1[sx1]);
            dS12_vec = vld1_u8(&S1[sx2]);
            dS13_vec = vld1_u8(&S1[sx3]);
            dS14_vec = vld1_u8(&S1[sx4]);

            qS01_vec = vreinterpretq_s16_u16(vmovl_u8(dS01_vec));
            qS02_vec = vreinterpretq_s16_u16(vmovl_u8(dS02_vec));
            qS03_vec = vreinterpretq_s16_u16(vmovl_u8(dS03_vec));
            qS04_vec = vreinterpretq_s16_u16(vmovl_u8(dS04_vec));
            qS11_vec = vreinterpretq_s16_u16(vmovl_u8(dS11_vec));
            qS12_vec = vreinterpretq_s16_u16(vmovl_u8(dS12_vec));
            qS13_vec = vreinterpretq_s16_u16(vmovl_u8(dS13_vec));
            qS14_vec = vreinterpretq_s16_u16(vmovl_u8(dS14_vec));

            dS01_0123 = vget_low_s16(qS01_vec);
            dS02_0123 = vget_low_s16(qS02_vec);
            dS03_0123 = vget_low_s16(qS03_vec);
            dS04_0123 = vget_low_s16(qS04_vec);
            dS11_0123 = vget_low_s16(qS11_vec);
            dS12_0123 = vget_low_s16(qS12_vec);
            dS13_0123 = vget_low_s16(qS13_vec);
            dS14_0123 = vget_low_s16(qS14_vec);

            dS01_4567 = vget_high_s16(qS01_vec);
            dS02_4567 = vget_high_s16(qS02_vec);
            dS03_4567 = vget_high_s16(qS03_vec);
            dS04_4567 = vget_high_s16(qS04_vec);
            dS11_4567 = vget_high_s16(qS11_vec);
            dS12_4567 = vget_high_s16(qS12_vec);
            dS13_4567 = vget_high_s16(qS13_vec);
            dS14_4567 = vget_high_s16(qS14_vec);

            qT01_vec = vmull_s16(dS01_0123, alpha_vec_A.val[0]);
            qT02_vec = vmull_s16(dS02_0123, alpha_vec_A.val[0]);
            qT03_vec = vmull_s16(dS03_0123, alpha_vec_A.val[0]);
            qT04_vec = vmull_s16(dS04_0123, alpha_vec_A.val[0]);
            qT11_vec = vmull_s16(dS11_0123, alpha_vec_A.val[0]);
            qT12_vec = vmull_s16(dS12_0123, alpha_vec_A.val[0]);
            qT13_vec = vmull_s16(dS13_0123, alpha_vec_A.val[0]);
            qT14_vec = vmull_s16(dS14_0123, alpha_vec_A.val[0]);

            qT01_vec = vmlal_s16(qT01_vec, dS01_4567, alpha_vec_A.val[1]);
            qT02_vec = vmlal_s16(qT02_vec, dS02_4567, alpha_vec_A.val[1]);
            qT03_vec = vmlal_s16(qT03_vec, dS03_4567, alpha_vec_A.val[1]);
            qT04_vec = vmlal_s16(qT04_vec, dS04_4567, alpha_vec_A.val[1]);
            qT11_vec = vmlal_s16(qT11_vec, dS11_4567, alpha_vec_A.val[1]);
            qT12_vec = vmlal_s16(qT12_vec, dS12_4567, alpha_vec_A.val[1]);
            qT13_vec = vmlal_s16(qT13_vec, dS13_4567, alpha_vec_A.val[1]);
            qT14_vec = vmlal_s16(qT14_vec, dS14_4567, alpha_vec_A.val[1]);

            vst1q_s32(&D0[dx_buf], qT01_vec);
            vst1q_s32(&D0[dx_buf + 4], qT02_vec);
            vst1q_s32(&D0[dx_buf + 8], qT03_vec);
            vst1q_s32(&D0[dx_buf + 12], qT04_vec);
            vst1q_s32(&D1[dx_buf], qT11_vec);
            vst1q_s32(&D1[dx_buf + 4], qT12_vec);
            vst1q_s32(&D1[dx_buf + 8], qT13_vec);
            vst1q_s32(&D1[dx_buf + 12], qT14_vec);
        }

        for(; dx < dwidth; dx += 4)
        {
            int32_t sx = xofs[dx];
            dx_buf=dx;

            dS0_vec = vld1_u8(&S0[sx]);
            dS1_vec = vld1_u8(&S1[sx]);

            qS0_vec = vreinterpretq_s16_u16(vmovl_u8(dS0_vec));
            qS1_vec = vreinterpretq_s16_u16(vmovl_u8(dS1_vec));

            dS0_0123 = vget_low_s16(qS0_vec);
            dS1_0123 = vget_low_s16(qS1_vec);

            qT0_vec = vmull_s16(dS0_0123, dCoeff);
            qT1_vec = vmull_s16(dS1_0123, dCoeff);

            vst1q_s32(&D0[dx_buf], qT0_vec);
            vst1q_s32(&D1[dx_buf], qT1_vec);
        }
    }

    for(; k < count; k++)
    {
        const uint8_t *S = src[k];
        int32_t *D = dst[k];

        for(dx = 0; dx < xmax; dx += 16)
        {
            int32_t sx1 = xofs[dx];
            int32_t sx2 = xofs[dx + 4];
            int32_t sx3 = xofs[dx + 8];
            int32_t sx4 = xofs[dx + 12];

            dx_buf=dx;

            alpha_vec_A = vld2_s16(&alpha[dx * 2]);             // 1 set
            alpha_vec_B = vld2_s16(&alpha[(dx + 4) * 2]);       // 2 set
            alpha_vec_C = vld2_s16(&alpha[(dx + 8) * 2]);       // 3 set
            alpha_vec_D = vld2_s16(&alpha[(dx + 12) * 2]);      // 4 set

            dS01_vec = vld1_u8(&S[sx1]);
            dS02_vec = vld1_u8(&S[sx2]);
            dS03_vec = vld1_u8(&S[sx3]);
            dS04_vec = vld1_u8(&S[sx4]);

            qS01_vec = vreinterpretq_s16_u16(vmovl_u8(dS01_vec));
            qS02_vec = vreinterpretq_s16_u16(vmovl_u8(dS02_vec));
            qS03_vec = vreinterpretq_s16_u16(vmovl_u8(dS03_vec));
            qS04_vec = vreinterpretq_s16_u16(vmovl_u8(dS04_vec));


            dS01_0123 = vget_low_s16(qS01_vec);
            dS02_0123 = vget_low_s16(qS02_vec);
            dS03_0123 = vget_low_s16(qS03_vec);
            dS04_0123 = vget_low_s16(qS04_vec);

            dS01_4567 = vget_high_s16(qS01_vec);
            dS02_4567 = vget_high_s16(qS02_vec);
            dS03_4567 = vget_high_s16(qS03_vec);
            dS04_4567 = vget_high_s16(qS04_vec);


            qT01_vec = vmull_s16(dS01_0123, alpha_vec_A.val[0]);
            qT02_vec = vmull_s16(dS02_0123, alpha_vec_A.val[0]);
            qT03_vec = vmull_s16(dS03_0123, alpha_vec_A.val[0]);
            qT04_vec = vmull_s16(dS04_0123, alpha_vec_A.val[0]);

            qT01_vec = vmlal_s16(qT01_vec, dS01_4567, alpha_vec_A.val[1]);
            qT02_vec = vmlal_s16(qT02_vec, dS02_4567, alpha_vec_A.val[1]);
            qT03_vec = vmlal_s16(qT03_vec, dS03_4567, alpha_vec_A.val[1]);
            qT04_vec = vmlal_s16(qT04_vec, dS04_4567, alpha_vec_A.val[1]);

            vst1q_s32(&D[dx_buf], qT01_vec);
            vst1q_s32(&D[dx_buf + 4], qT02_vec);
            vst1q_s32(&D[dx_buf + 8], qT03_vec);
            vst1q_s32(&D[dx_buf + 12], qT04_vec);
        }


        for(; dx < dwidth; dx += 4)
        {
            int32_t sx = xofs[dx];

            dx_buf=dx;

            dS0_vec = vld1_u8(&S[sx]);

            qS0_vec = vreinterpretq_s16_u16(vmovl_u8(dS0_vec));

            dS0_0123 = vget_low_s16(qS0_vec);

            qT0_vec = vmull_s16(dS0_0123, dCoeff);

            vst1q_s32(&D[dx_buf], qT0_vec);
        }
    }
}

static void img_vresize_linear_neon(const int32_t **src, uint8_t *dst, const int16_t *beta, int32_t width)
{
    const uint64_t img_vresize_linear_mask_residual_table[VRESIZE_LINEAR_MASK_TABLE_SIZE] =
    {
        0x00000000000000FF, 0x000000000000FFFF,
        0x0000000000FFFFFF, 0x00000000FFFFFFFF,
        0x000000FFFFFFFFFF, 0x0000FFFFFFFFFFFF,
        0x00FFFFFFFFFFFFFF
    };

    const int32_t *S0 = src[0], *S1 = src[1];

    int32x4_t qS0_0123, qS0_4567, qS1_0123, qS1_4567;
    int32x4_t qS0_89AB, qS0_CDEF, qS1_89AB, qS1_CDEF;
    int32x4_t qT_0123, qT_4567;
    int32x4_t qT_89AB, qT_CDEF;
    int16x4_t dT_0123, dT_4567;
    int16x4_t dT_89AB, dT_CDEF;
    uint16x8_t qT_01234567;
    uint16x8_t qT_89ABCDEF;
    uint8x8_t dT_01234567, dDst_01234567;
    uint8x8_t dT_89ABCDEF;

    int32x2_t dBeta = {};
    dBeta = vset_lane_s32((int32_t)(beta[0]), dBeta, 0);
    dBeta = vset_lane_s32((int32_t)(beta[1]), dBeta, 1);

    int32x4_t qDelta, qMin, qMax;
    qDelta = vdupq_n_s32(DELTA);
    qMin = vdupq_n_s32(0);
    qMax = vdupq_n_s32(255);

    int32_t x = 0;
    int32_t x_buf = 0;

    for(; x <= ((width/2) - 16); x += 16)
    {
        x_buf = x;

        qS0_0123 = vld1q_s32(&S0[x]);
        qS0_4567 = vld1q_s32(&S0[x + 4]);
        qS0_89AB = vld1q_s32(&S0[x + 8]);
        qS0_CDEF = vld1q_s32(&S0[x + 12]);

        qS1_0123 = vld1q_s32(&S1[x]);
        qS1_4567 = vld1q_s32(&S1[x + 4]);
        qS1_89AB = vld1q_s32(&S1[x + 8]);
        qS1_CDEF = vld1q_s32(&S1[x + 12]);

        qT_0123 = vmulq_lane_s32(qS0_0123, dBeta, 0);
        qT_4567 = vmulq_lane_s32(qS0_4567, dBeta, 0);
        qT_89AB = vmulq_lane_s32(qS0_89AB, dBeta, 0);
        qT_CDEF = vmulq_lane_s32(qS0_CDEF, dBeta, 0);

        qT_0123 = vmlaq_lane_s32(qT_0123, qS1_0123, dBeta, 1);
        qT_4567 = vmlaq_lane_s32(qT_4567, qS1_4567, dBeta, 1);
        qT_89AB = vmlaq_lane_s32(qT_89AB, qS1_89AB, dBeta, 1);
        qT_CDEF = vmlaq_lane_s32(qT_CDEF, qS1_CDEF, dBeta, 1);

        qT_0123 = vaddq_s32(qT_0123, qDelta);
        qT_4567 = vaddq_s32(qT_4567, qDelta);
        qT_89AB = vaddq_s32(qT_89AB, qDelta);
        qT_CDEF = vaddq_s32(qT_CDEF, qDelta);

        qT_0123 = vshrq_n_s32(qT_0123, BITS);
        qT_4567 = vshrq_n_s32(qT_4567, BITS);
        qT_89AB = vshrq_n_s32(qT_89AB, BITS);
        qT_CDEF = vshrq_n_s32(qT_CDEF, BITS);

        qT_0123 = vmaxq_s32(qT_0123, qMin);
        qT_4567 = vmaxq_s32(qT_4567, qMin);
        qT_89AB = vmaxq_s32(qT_89AB, qMin);
        qT_CDEF = vmaxq_s32(qT_CDEF, qMin);

        qT_0123 = vminq_s32(qT_0123, qMax);
        qT_4567 = vminq_s32(qT_4567, qMax);
        qT_89AB = vminq_s32(qT_89AB, qMax);
        qT_CDEF = vminq_s32(qT_CDEF, qMax);

        dT_0123 = vmovn_s32(qT_0123);
        dT_4567 = vmovn_s32(qT_4567);
        dT_89AB = vmovn_s32(qT_89AB);
        dT_CDEF = vmovn_s32(qT_CDEF);

        qT_01234567 = vreinterpretq_u16_s16(vcombine_s16(dT_0123, dT_4567));
        qT_89ABCDEF = vreinterpretq_u16_s16(vcombine_s16(dT_89AB, dT_CDEF));

        dT_01234567 = vmovn_u16(qT_01234567);
        dT_89ABCDEF = vmovn_u16(qT_89ABCDEF);

        vst1_u8(&dst[x_buf], dT_01234567);
        vst1_u8(&dst[x_buf+8], dT_89ABCDEF);
    }

    for(; x <= ((width/2) - 8); x += 8)
    {
        qS0_0123 = vld1q_s32(&S0[x]);
        qS0_4567 = vld1q_s32(&S0[x + 4]);
        qS1_0123 = vld1q_s32(&S1[x]);
        qS1_4567 = vld1q_s32(&S1[x + 4]);

        qT_0123 = vmulq_lane_s32(qS0_0123, dBeta, 0);
        qT_4567 = vmulq_lane_s32(qS0_4567, dBeta, 0);
        qT_0123 = vmlaq_lane_s32(qT_0123, qS1_0123, dBeta, 1);
        qT_4567 = vmlaq_lane_s32(qT_4567, qS1_4567, dBeta, 1);

        qT_0123 = vaddq_s32(qT_0123, qDelta);
        qT_4567 = vaddq_s32(qT_4567, qDelta);

        qT_0123 = vshrq_n_s32(qT_0123, BITS);
        qT_4567 = vshrq_n_s32(qT_4567, BITS);

        qT_0123 = vmaxq_s32(qT_0123, qMin);
        qT_4567 = vmaxq_s32(qT_4567, qMin);
        qT_0123 = vminq_s32(qT_0123, qMax);
        qT_4567 = vminq_s32(qT_4567, qMax);

        dT_0123 = vmovn_s32(qT_0123);
        dT_4567 = vmovn_s32(qT_4567);
        qT_01234567 = vreinterpretq_u16_s16(vcombine_s16(dT_0123, dT_4567));
        dT_01234567 = vmovn_u16(qT_01234567);

        vst1_u8(&dst[x], dT_01234567);
    }

    if(x < (width/2))
    {
        uint8x8_t dMask;
        dMask = vld1_u8((uint8_t*)(&img_vresize_linear_mask_residual_table[(width/2) - x - 1]));
        dDst_01234567 = vld1_u8 (&dst[x]);

        qS0_0123 = vld1q_s32(&S0[x]);
        qS0_4567 = vld1q_s32(&S0[x + 4]);
        qS1_0123 = vld1q_s32(&S1[x]);
        qS1_4567 = vld1q_s32(&S1[x + 4]);

        qT_0123 = vmulq_lane_s32(qS0_0123, dBeta, 0);
        qT_4567 = vmulq_lane_s32(qS0_4567, dBeta, 0);
        qT_0123 = vmlaq_lane_s32(qT_0123, qS1_0123, dBeta, 1);
        qT_4567 = vmlaq_lane_s32(qT_4567, qS1_4567, dBeta, 1);

        qT_0123 = vaddq_s32(qT_0123, qDelta);
        qT_4567 = vaddq_s32(qT_4567, qDelta);

        qT_0123 = vshrq_n_s32(qT_0123, BITS);
        qT_4567 = vshrq_n_s32(qT_4567, BITS);

        qT_0123 = vmaxq_s32(qT_0123, qMin);
        qT_4567 = vmaxq_s32(qT_4567, qMin);
        qT_0123 = vminq_s32(qT_0123, qMax);
        qT_4567 = vminq_s32(qT_4567, qMax);

        dT_0123 = vmovn_s32(qT_0123);
        dT_4567 = vmovn_s32(qT_4567);
        qT_01234567 = vreinterpretq_u16_s16(vcombine_s16 (dT_0123, dT_4567));
        dT_01234567 = vmovn_u16(qT_01234567);

        dMask = vbsl_u8(dMask, dT_01234567, dDst_01234567);
        vst1_u8(&dst[x], dMask);
    }
}

static void img_resize_generic_linear_neon(uint8_t *src,
                                           uint8_t *dst,
                                           const int32_t *xofs,
                                           const int16_t *_alpha,
                                           const int32_t *yofs,
                                           const int16_t *_beta,
                                           int32_t xmin,
                                           int32_t xmax,
                                           int32_t ksize,
                                           int32_t srcw,
                                           int32_t srch,
                                           int32_t srcstep,
                                           int32_t dstw,
                                           int32_t dsth,
                                           int32_t dststep,
                                           int32_t channels)
{
    const int16_t *alpha = _alpha;
    const int16_t *beta = _beta;
    int32_t cn = channels;
    srcw *= cn;
    dstw *= cn;

    int32_t bufstep = (int32_t)neon_align_size(dstw, 16);
    int32_t *buffer_ = (int32_t*)malloc(bufstep * ksize * sizeof(int32_t));

    const uint8_t *srows[MAX_ESIZE];
    int32_t *rows[MAX_ESIZE];
    int32_t prev_sy[MAX_ESIZE];
    int32_t k, dy;

    xmin *= cn;
    xmax *= cn;

    for(k = 0; k < ksize; k++)
    {
        prev_sy[k] = -1;
        rows[k] = (int32_t*)(buffer_ + (bufstep * k));
    }

    for(dy = 0; dy < dsth; dy++, beta += ksize)
    {
        int32_t sy0 = yofs[dy], k, k0 = ksize, k1 = 0, ksize2 = ksize / 2;

        for(k = 0; k < ksize; k++)
        {
            int32_t sy = neon_clip(sy0 - ksize2 + 1 + k, 0, srch);
            for(k1 = MAX (k1, k); k1 < ksize; k1++)
            {
                if(sy == prev_sy[k1])
                {
                    if(k1 > k)
                        memcpy(rows[k], rows[k1], bufstep * sizeof (rows[0][0]));
                    break;
                }
            }
            if(k1 == ksize)
                k0 = MIN (k0, k);
            srows[k] = (const uint8_t*) (src + (srcstep * sy));
            prev_sy[k] = sy;
        }

        if(k0 < ksize)
        {
            if(cn == 4)
            {
                img_hresize_4channels_linear_neon(srows + k0, rows + k0, ksize - k0, xofs, alpha,
                                                  dstw, xmax);
            }
            else
            {
                img_hresize_linear_c(srows + k0, rows + k0, ksize - k0, xofs, alpha,
                                     dstw, cn, xmax);
            }
        }

        img_vresize_linear_neon((const int32_t**)rows, (uint8_t*)(dst + (dststep * dy)), beta, dstw);
    }
    if(buffer_)
        free(buffer_);
}

int32_t nc_resize_image_bilinear_yuyv(uint8_t *src, uint32_t src_stride, uint32_t src_width, uint32_t src_height,
                                      uint8_t *dst, uint32_t dst_stride, uint32_t dst_width, uint32_t dst_height)
{
    int32_t ret = 0;

    int32_t cn = 4;
    int32_t xmin = 0;
    int32_t xmax = dst_width;
    int32_t width = dst_width * cn;
    int32_t ksize = 0, ksize2;

    ksize = 2;
    ksize2 = ksize / 2;

    uint8_t *buffer_ = (uint8_t*)malloc((width + dst_height) * (sizeof(int32_t) + (sizeof(float) * ksize)));

    int32_t *xofs = (int32_t*) buffer_;
    int32_t *yofs = xofs + width;
    int16_t *ialpha = (int16_t*) (yofs + dst_height);
    int16_t *ibeta = ialpha + width * ksize;

    img_resize_cal_offset_linear(xofs, ialpha, yofs, ibeta, &xmin, &xmax, ksize, ksize2, src_width, src_height, dst_width, dst_height, cn);

    img_resize_generic_linear_neon(src, dst, xofs, ialpha, yofs, ibeta, xmin, xmax, ksize, src_width, src_height, src_stride, dst_width, dst_height, dst_stride, cn);

    if(buffer_)
        free(buffer_);

    return ret;
}

/* example */
int32_t nc_pip_image_bilinear_yuyv(uint8_t *src, int32_t src_w, int32_t src_h, int32_t trg_w, int32_t trg_h,
                                   uint8_t *pip_plane, int32_t pip_w, int32_t pip_h,
                                   int32_t x_ofs, int32_t y_ofs)
{
    int32_t ret = OK;

    // source info
    int32_t pip_stride = pip_w * 2;
    int32_t src_stride = src_w * 2;

    // destination info
    int32_t dst_width = trg_w;
    int32_t dst_height = trg_h;

    // image start offset
    int32_t dst_start_xpos = x_ofs;
    int32_t dst_start_ypos = y_ofs;

    // over range prevent code
    if((dst_start_xpos + dst_width) > pip_w)
        return ERR;

    if((dst_start_ypos + dst_height) > pip_h)
        return ERR;

    nc_resize_image_bilinear_yuyv(src, src_stride, src_w, src_h,
                              (pip_plane + ((dst_start_ypos * pip_stride) + (dst_start_xpos * 2))), pip_stride, dst_width, dst_height);

    return ret;
}

void nc_ScaleYUY2RowDown2Box_NEON(uint8_t *src_ptr, int32_t src_stride,
                               uint8_t *dst, int32_t dst_width)
{
    uint8_t *src_ptr_nextline = src_ptr + src_stride;

    asm volatile (
        // src_yuy2
        // y0 u01 y1 v01 y2 u23 y3 v23 y4 u45 y5 v45 y6 u67 y7 v67 : 8 pixels (16 bytes)
        // 00 01  02 03  04 05  06 07  08 09  0a 0b  0c 0d  0e 0f
        "1: \n" // 1 loop - src : 64bytes(32pixels) | dst : 32bytes(16pixels)
        "ld4        {v0.16b, v1.16b, v2.16b, v3.16b}, [%[src_yuy2]], #64 \n" // v0,v2 = y | v1 = u | v3 = v
        "ld4        {v16.16b, v17.16b, v18.16b, v19.16b}, [%[src_yuy2_nextline]], #64 \n"

        "uaddlp     v1.8h, v1.16b \n" // U 16 bytes -> 8 shorts.
        "uaddlp     v3.8h, v3.16b \n" // V 16 bytes -> 8 shorts.
        "uadalp     v1.8h, v17.16b                \n"  // U 16 bytes -> 8 shorts.
        "uadalp     v3.8h, v19.16b                \n"  // V 16 bytes -> 8 shorts.
        "rshrn      v1.8b, v1.8h, #2              \n"  // round and pack
        "rshrn      v3.8b, v3.8h, #2              \n"  // round and pack

        // v0 <- y2 y0
        // v1 <- u23 u01
        // v2 <- y3 y1
        // v3 <- u34 v01
        "zip1       v4.16b, v0.16b, v2.16b \n"
        "zip2       v5.16b, v0.16b, v2.16b \n"
        "zip1       v6.16b, v16.16b, v18.16b \n"
        "zip2       v7.16b, v16.16b, v18.16b \n"
        // v0 <- y3 y2 y1 y0

        "uaddlp     v4.8h, v4.16b                 \n"  // row 1 add adjacent
        "uaddlp     v5.8h, v5.16b                 \n"
        "uadalp     v4.8h, v6.16b                 \n"  // += row 2 add adjacent
        "uadalp     v5.8h, v7.16b                 \n"
        "rshrn      v4.8b, v4.8h, #2              \n"  // round and pack
        "rshrn2     v4.16b, v5.8h, #2             \n"

        "uzp1       v6.16b, v4.16b, v5.16b \n"
        "uzp2       v7.16b, v4.16b, v5.16b \n"

        // store : v4 v1 v5 v3 order is YUY2
        "dup        v8.2D, v6.D[0] \n"
        "dup        v9.2D, v1.D[0] \n"
        "dup        v10.2D, v7.D[0] \n"
        "dup        v11.2D, v3.D[0] \n"
        "st4        {v8.8b, v9.8b, v10.8b, v11.8b}, [%[dst_yuy2]], #32 \n"

        "subs       %w[remain_dst_width], %w[remain_dst_width], #16 \n"
        "b.gt       1b \n"
        : [src_yuy2] "+r"(src_ptr)
         ,[src_yuy2_nextline] "+r"(src_ptr_nextline)
         ,[dst_yuy2] "+r"(dst)
         ,[remain_dst_width] "+r"(dst_width)
        :
        : "memory", "cc", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v16", "v17", "v18", "v19"
    );

    return;
}

void nc_ScaleYUY2Down2Box(uint8_t* src, int32_t src_stride, int32_t src_height,
                       uint8_t* dst, int32_t dst_stride, int32_t dst_width)
{
    for (int ypos = 0; ypos < src_height; ypos += 2)
        nc_ScaleYUY2RowDown2Box_NEON((src + (src_stride * ypos)), src_stride, (dst + ((dst_stride * ypos) / 2)), dst_width);

    return;
}

static int32_t halfwidth_stripe(uint8_t *yuv_src_line, uint8_t *yuv_dst_line, int32_t src_width)
{
    int32_t ret = OK;

    asm volatile (
        "1: \n" // 1 loop - src : 64bytes(32pixels) | dst : 32bytes(16pixels)
        "ld4        {v0.16b, v1.16b, v2.16b, v3.16b}, [%[src_yuy2]], #64 \n" // v0,v2 = y | v1 = u | v3 = v

        "uaddlp     v1.8h, v1.16b \n" // U 16 bytes -> 8 shorts.
        "uaddlp     v3.8h, v3.16b \n" // V 16 bytes -> 8 shorts.
        "rshrn      v1.8b, v1.8h, #1              \n"  // round and pack
        "rshrn      v3.8b, v3.8h, #1              \n"  // round and pack

        "zip1       v4.16b, v0.16b, v2.16b \n"
        "zip2       v5.16b, v0.16b, v2.16b \n"

        "uaddlp     v4.8h, v4.16b                 \n"  // row 1 add adjacent
        "uaddlp     v5.8h, v5.16b                 \n"
        "rshrn      v4.8b, v4.8h, #1              \n"  // round and pack
        "rshrn2     v4.16b, v5.8h, #1             \n"

        "uzp1       v6.16b, v4.16b, v5.16b \n"
        "uzp2       v7.16b, v4.16b, v5.16b \n"

        "dup        v8.2D, v6.D[0] \n"
        "dup        v9.2D, v1.D[0] \n"
        "dup        v10.2D, v7.D[0] \n"
        "dup        v11.2D, v3.D[0] \n"
        "st4        {v8.8b, v9.8b, v10.8b, v11.8b}, [%[dst_yuy2]], #32 \n"

        "subs       %w[remain_src_width], %w[remain_src_width], #32 \n"
        "b.gt       1b \n"

        : [src_yuy2] "+r"(yuv_src_line)
         ,[dst_yuy2] "+r"(yuv_dst_line)
         ,[remain_src_width] "+r"(src_width)
        :
        : "memory", "cc", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v16", "v17", "v18", "v19"
    );

    return ret;
}

int32_t nc_resize_yuy2_halfwidth(uint8_t *yuv_src, uint8_t *yuv_dst, int32_t src_width, int32_t src_height)
{
    int32_t ret = OK;
    int32_t stride = src_width * 2;

    for (int ypos = 0; ypos < src_height; ypos++)
        halfwidth_stripe(yuv_src + (stride * ypos), yuv_dst + (stride/2 * ypos), src_width);


    return ret;
}

static void img_resize_cal_offset_linear_stripe(int32_t *yofs, uint16_t *ibeta, int32_t src_size, int32_t dst_size)
{
    float inv_scale_y = (float)dst_size / (float)src_size;
    float scale_y = (float)(1. / inv_scale_y);
    int32_t k, sy, dy;
    float fy;
    float cbuf[2];

    // vertical
    for(dy = 0; dy < dst_size; dy++)
    {
        fy = (float) ((dy + 0.5) * scale_y - 0.5);
        sy = neon_floor(fy);
        fy -= (float)sy;

        yofs[dy] = sy;

        cbuf[0] = 1.f - fy;
        cbuf[1] = fy;

        for(k = 0; k < 2; k++)
            ibeta[(dy * 2) + k] = (uint16_t) (cbuf[k] * (1<<11));    // floating value x 2^8
    }
}

static int32_t hafwid_verti_scaledn_stripe(uint8_t *yuv_src_line, uint8_t *yuv_dst_line, int32_t src_width, uint16_t ibeta1, uint16_t ibeta2)
{
    int32_t ret = OK;
    uint8_t *yuv_src_line_next = yuv_src_line + (src_width * 2);
    uint16_t c_ibeta1 = ibeta1;
    uint16_t c_ibeta2 = ibeta2;
    uint16_t *ptr_ibeta1;
    uint16_t *ptr_ibeta2;
    ptr_ibeta1 = &c_ibeta1;
    ptr_ibeta2 = &c_ibeta2;

    asm volatile (
        // "ldr        w0, asm_ibeta1             \n"
        // "ldr        w1, asm_ibeta2             \n"
        "mov        v8.h[0], w3                     \n"
        "mov        v8.h[1], w4                     \n"
        "1:                                                                         \n" // 1 loop - src : 64bytes(32pixels) | dst : 32bytes(16pixels)
        "ld4        {v0.16b, v1.16b, v2.16b, v3.16b}, [%[src_yuy2]], #64            \n" // v0,v2 = y | v1 = u | v3 = v
        "ld4        {v10.16b, v11.16b, v12.16b, v13.16b}, [%[src_yuy2_next]], #64   \n" // v0,v2 = y | v1 = u | v3 = v

        // half width start
        "uaddlp     v1.8h, v1.16b                   \n"     // U 16 bytes -> 8 shorts.
        "uaddlp     v3.8h, v3.16b                   \n"     // V 16 bytes -> 8 shorts.
        "uaddlp     v11.8h, v11.16b                 \n"     // U 16 bytes -> 8 shorts.
        "uaddlp     v13.8h, v13.16b                 \n"     // V 16 bytes -> 8 shorts.
        "rshrn      v1.8b, v1.8h,   #1              \n"     // round and pack
        "rshrn      v3.8b, v3.8h,   #1              \n"     // round and pack
        "rshrn      v11.8b, v11.8h, #1              \n"     // round and pack
        "rshrn      v13.8b, v13.8h, #1              \n"     // round and pack

        "zip1       v4.16b, v0.16b, v2.16b          \n"     // y first
        "zip1       v14.16b, v10.16b, v12.16b       \n"
        "zip2       v5.16b, v0.16b, v2.16b          \n"     // y second
        "zip2       v15.16b, v10.16b, v12.16b       \n"

        "uaddlp     v4.8h, v4.16b                   \n"     // y first pair add
        "uaddlp     v5.8h, v5.16b                   \n"     // y second pair add
        "uaddlp     v14.8h, v14.16b                 \n"
        "uaddlp     v15.8h, v15.16b                 \n"

        "rshrn      v4.8b, v4.8h,       #1          \n"     // round and pack
        "rshrn      v14.8b, v14.8h,     #1          \n"     // round and pack
        "rshrn2     v4.16b, v5.8h,      #1          \n"
        "rshrn2     v14.16b, v15.8h,    #1          \n"

        "uzp1       v0.16b, v4.16b, v5.16b          \n"     // v5 data 사용 안함 : v0 하단 = v4 홀수번째 + v0 상단 = v5 짝수번째
        "uzp1       v10.16b, v14.16b, v15.16b       \n"
        "uzp2       v2.16b, v4.16b, v5.16b          \n"
        "uzp2       v12.16b, v14.16b, v15.16b       \n"
        // half width until here

        // fisrt line - u8 to u16
        "UXTL       v4.8h, v0.8b                    \n"
        "UXTL       v5.8h, v1.8b                    \n"
        "UXTL       v6.8h, v2.8b                    \n"
        "UXTL       v7.8h, v3.8b                    \n"
        // second line - u8 to u16
        "UXTL       v14.8h, v10.8b                  \n"
        "UXTL       v15.8h, v11.8b                  \n"
        "UXTL       v16.8h, v12.8b                  \n"
        "UXTL       v17.8h, v13.8b                  \n"

        // mul beta1 - first line
        "UMULL      v21.4s, v4.4h, v8.h[0]          \n"
        "UMULL2     v22.4s, v4.8h, v8.h[0]          \n"
        "UMULL      v23.4s, v5.4h, v8.h[0]          \n"
        "UMULL2     v24.4s, v5.8h, v8.h[0]          \n"
        "UMULL      v25.4s, v6.4h, v8.h[0]          \n"
        "UMULL2     v26.4s, v6.8h, v8.h[0]          \n"
        "UMULL      v27.4s, v7.4h, v8.h[0]          \n"
        "UMULL2     v28.4s, v7.8h, v8.h[0]          \n"

        // mla beta2 - second line
        "UMLAL      v21.4s, v14.4h, v8.h[1]         \n"
        "UMLAL2     v22.4s, v14.8h, v8.h[1]         \n"
        "UMLAL      v23.4s, v15.4h, v8.h[1]         \n"
        "UMLAL2     v24.4s, v15.8h, v8.h[1]         \n"
        "UMLAL      v25.4s, v16.4h, v8.h[1]         \n"
        "UMLAL2     v26.4s, v16.8h, v8.h[1]         \n"
        "UMLAL      v27.4s, v17.4h, v8.h[1]         \n"
        "UMLAL2     v28.4s, v17.8h, v8.h[1]         \n"

        // rshrn
        "rshrn      v21.4h, v21.4s,   #11            \n"
        "rshrn2     v21.8h, v22.4s,   #11            \n"
        "rshrn      v23.4h, v23.4s,   #11            \n"
        "rshrn2     v23.8h, v24.4s,   #11            \n"
        "rshrn      v25.4h, v25.4s,   #11            \n"
        "rshrn2     v25.8h, v26.4s,   #11            \n"
        "rshrn      v27.4h, v27.4s,   #11            \n"
        "rshrn2     v27.8h, v28.4s,   #11            \n"

        // compress
        "XTN        v0.8b, v21.8h                   \n"
        "XTN        v1.8b, v23.8h                   \n"
        "XTN        v2.8b, v25.8h                   \n"
        "XTN        v3.8b, v27.8h                   \n"

        //  st
        "st4        {v0.8b, v1.8b, v2.8b, v3.8b}, [%[dst_yuy2]], #32    \n"

        "subs       %w[remain_src_width], %w[remain_src_width], #32     \n"
        "b.gt       1b \n"

        : [src_yuy2] "+r"(yuv_src_line)
         ,[src_yuy2_next] "+r"(yuv_src_line_next)
         ,[dst_yuy2] "+r"(yuv_dst_line)
         ,[asm_ibeta1] "+r"(ptr_ibeta1)
         ,[asm_ibeta2] "+r"(ptr_ibeta2)
         ,[remain_src_width] "+r"(src_width)
        :
        : "memory", "cc", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9",
                          "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19",
                          "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29",
                          "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13"
    );

    return ret;
}

int32_t nc_resize_yuy2_hafwid_verti_scaledn(uint8_t *yuv_src, uint8_t *yuv_dst, int32_t src_width, int32_t src_height, int32_t dst_height)
{
    int32_t ret = OK;
    int32_t stride = src_width * 2;

    uint8_t *buffer_ = (uint8_t*)malloc(dst_height * (sizeof(int32_t) + (sizeof(uint16_t) * 2)));   // (Vertical 점의 개수) x (좌표 1개 + 두점간 비 2개)
    int32_t *yofs = (int32_t*) buffer_;
    uint16_t *ibeta = (uint16_t*) (&yofs[dst_height]);

    img_resize_cal_offset_linear_stripe(yofs, ibeta, src_height, dst_height);

    for (int ypos = 0; ypos < dst_height; ypos++)
        hafwid_verti_scaledn_stripe(yuv_src + (stride * yofs[ypos]), yuv_dst + (stride/2 * ypos), src_width, ibeta[ypos*2], ibeta[ypos*2 + 1]);

    if (buffer_)
        free(buffer_);

    return ret;
}



int32_t nc_img_yuyv_packed_to_planar(uint8_t *yuv_Packed, uint8_t *yuv_Plannar, int32_t width, int32_t height)
{
    int32_t ret = OK;

    uint8_t *Yplane = yuv_Plannar;
    uint8_t *Uplane = &yuv_Plannar[(width*height)];
    uint8_t *Vplane = &yuv_Plannar[(width*height + width*height/2)];
    int32_t img_langth = (width*height/2);


    asm volatile (
        "1: \n" // 1 loop - src : 64bytes(32pixels) | dst : 32bytes(16pixels)

        "ld4        {v0.16b, v1.16b, v2.16b, v3.16b}, [%[src]], #64 \n"

        "zip1       v4.16b, v0.16b, v2.16b \n"
        "zip2       v5.16b, v0.16b, v2.16b \n"

        "st1        {v4.16b, v5.16b}, [%[dst_Y]], #32 \n"
        "st1        {v1.16b}, [%[dst_U]], #16 \n"
        "st1        {v3.16b}, [%[dst_V]], #16 \n"

        "subs       %w[remain_dst_width], %w[remain_dst_width], #16 \n"
        "b.gt       1b \n"

        : [src] "+r"(yuv_Packed)
         ,[dst_Y] "+r"(Yplane)
         ,[dst_U] "+r"(Uplane)
         ,[dst_V] "+r"(Vplane)
         ,[remain_dst_width] "+r"(img_langth)
        :
        : "memory", "cc", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v16", "v17", "v18", "v19"
    );

    return ret;
}

int32_t nc_img_yuyv_planar_to_packed(uint8_t *yuv_Plannar, uint8_t *yuv_Packed, int32_t width, int32_t height)
{
    int32_t ret = OK;

    uint8_t *Yplane = yuv_Plannar;
    uint8_t *Uplane = &yuv_Plannar[(width*height)];
    uint8_t *Vplane = &yuv_Plannar[(width*height + width*height/2)];
    int32_t img_langth = (width*height/2);

    // YUYV 순서 제확인 필요
    asm volatile (
        "1: \n" // 1 loop - src : 64bytes(32pixels) | dst : 32bytes(16pixels)

        "ld2        {v0.16b, v1.16b}, [%[src_Y]], #32 \n"
        "ld1        {v4.16b}, [%[src_U]], #16 \n"
        "ld1        {v3.16b}, [%[src_V]], #16 \n"

        "mov        v2.16b, v1.16b \n"
        "mov        v1.16b, v4.16b \n"

        "st4        {v0.16b, v1.16b, v2.16b, v3.16b}, [%[dst]], #64 \n"     // right -> v0 v2 v1 v3

        "subs       %w[remain_dst_width], %w[remain_dst_width], #16 \n"
        "b.gt       1b \n"

        : [dst] "+r"(yuv_Packed)
         ,[src_Y] "+r"(Yplane)
         ,[src_U] "+r"(Uplane)
         ,[src_V] "+r"(Vplane)
         ,[remain_dst_width] "+r"(img_langth)
        :
        : "memory", "cc", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v16", "v17", "v18", "v19"
    );

    return ret;
}

int32_t nc_img_nv12_to_yuv422(uint8_t* nv_ptr, uint8_t* yuv_ptr, int32_t width, int32_t height)
{
    int32_t ret = OK;
    int32_t lvCnt_H = 0;

    uint8_t *DSTplane;
    uint8_t *Yplane;
    uint8_t *UVplane;

    int32_t lvWidth_target;

    for(lvCnt_H = 0; lvCnt_H < height; lvCnt_H++)
    {
        lvWidth_target = width/2;
        DSTplane = &yuv_ptr[(width*2)*(lvCnt_H)];
        Yplane = &nv_ptr[0                 + (width)*(lvCnt_H)];
        UVplane = &nv_ptr[(width * height) + (width)*(lvCnt_H/2)];

        asm volatile (
            "1: \n"

            "ld2        {v0.16b, v1.16b}, [%[src_Y]], #32 \n"   // Y0 Y1
            "ld2        {v4.16b, v5.16b}, [%[src_UV]], #32 \n"  // U0 V0

            "mov        v2.16b, v1.16b \n"  // Y1
            "mov        v1.16b, v4.16b \n"  // U0
            "mov        v3.16b, v5.16b \n"  // V0

            "st4        {v0.16b, v1.16b, v2.16b, v3.16b}, [%[dst]], #64 \n" // [Y0 U0 Y1 V0]

            "subs       %w[remain_dst_width], %w[remain_dst_width], #16 \n"
            "b.gt       1b \n"


            : [dst] "+r"(DSTplane)
            ,[src_Y] "+r"(Yplane)
            ,[src_UV] "+r"(UVplane)
            ,[remain_dst_width] "+r"(lvWidth_target)
            :
            : "memory", "cc", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v16", "v17", "v18", "v19"
        );

    }

    return ret;
}

#define TILESIZE    64
int32_t nc_img_rgb24_packed_to_tiled_planar(uint8_t *rgb_Packed, uint8_t *rgb_Tiled, int32_t width, int32_t height)
{   // 64_8x8
    int32_t ret = OK;
    long int in_x_idx, in_y_idx;
    int plane_offset = width * height;
    int32_t img_stried = width * 3;

    uint8_t *RGBplane = rgb_Packed;
    uint8_t *Rplane = &rgb_Tiled[plane_offset * 0];
    uint8_t *Gplane = &rgb_Tiled[plane_offset * 1];
    uint8_t *Bplane = &rgb_Tiled[plane_offset * 2];


    for (long y_super = 0; y_super < (height / TILESIZE); y_super++)
    {
        for (long x_super = 0; x_super < (width / TILESIZE); x_super++)
        {
            for (long y_sub = 0; y_sub < 8; y_sub++)
            {
                for (long x_sub = 0; x_sub < 8; x_sub++)
                {
                    for (long y_pix = 0; y_pix < 8; y_pix++)
                    {
                        in_x_idx = (x_super * TILESIZE * 3) + (x_sub * 8 * 3);
                        in_y_idx = (y_super * TILESIZE) + (y_sub * 8) + y_pix;
                        RGBplane = &rgb_Packed[(in_y_idx * img_stried) + in_x_idx];

                        asm volatile (
                                //       R      G      B
                            "ld3        {v0.8b, v1.8b, v2.8b}, [%[src]], #24 \n"
                            "st1        {v0.8b}, [%[dst_R]], #8 \n"     // R Plane
                            "st1        {v1.8b}, [%[dst_G]], #8 \n"     // G Plane
                            "st1        {v2.8b}, [%[dst_B]], #8 \n"     // B Plane
                            :[src] "+r"(RGBplane)
                            ,[dst_R] "+r"(Rplane)
                            ,[dst_G] "+r"(Gplane)
                            ,[dst_B] "+r"(Bplane)
                            :
                            : "memory", "cc", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v16", "v17", "v18", "v19"
                        );
                    }
                }
            }
        }
    }

    return ret;
}

/*
 * Full swing for BT.601
 *     https://en.wikipedia.org/wiki/YUV#Full_swing_for_BT.601
 *
 * Y = ( 77 * R + 150 * G +  29 * B) >> 8 + 0;
 * U = (-43 * R -  84 * G + 127 * B) >> 8 + 128;
 * V = (127 * R - 106 * G -  21 * B) >> 8 + 128;
 */
void neon_Mix_Rgbx2Yuyv(uint8_t *src, uint8_t *dst, int32_t w, int32_t h, int32_t isRgbaNotBgra)
{
    int32_t i, loop;

    int16x8_t   sqw_r, sqw_g, sqw_b;
    int16x8_t   sqw_y, sqw_u, sqw_v;
    int16x8_t   sqw_128 = vmovq_n_s16(128);

    uint8x8x4_t udb4_rgb;
    uint8x8x2_t udb2_yuyv, udb2_org, udb2_uv;
    uint8x8_t   udb_y, udb_u, udb_v;
    uint8x8_t   udb_oy;

    uint8x8_t   udb_a, udb_n;
    uint16x8_t  uqh_y, uqh_u, uqh_v;

    loop = h * w;

    for (i = 0; i < loop; i += 8)
    {
        /* load rgb */
        // 8 bit x 8 lane x 4 vector
        // v0 : r1 r2 r3 r4 r5 r6 r7 r8 r9 r10
        // v1 : g1 g2 g3 g4 g5 g6 g7 g8 g9 g10
        // v2 : b1 b2 b3 b4 b5 b6 b7 b8 b9 b10
        // v3 : a1 a2 a3 a4 a5 a6 a7 a8 a9 a10
        udb4_rgb = vld4_u8(src + i * 4);

        /* check alpha */
        udb_a = udb4_rgb.val[3];

        // 8 bit x 8 lane => 64 bit x 1 lane으로 변경 후 0번 lane값이 0인지 확인 => alpha 값이 있는지 확인
        if (0 != vget_lane_u64(vreinterpret_u64_u8(udb_a), 0))
        {
            // rgb 값 u8에서 u16으로 변경 후 다시 s16으로 변경
            /* long signed rgb */
            if (isRgbaNotBgra) {
                sqw_r = vreinterpretq_s16_u16(vmovl_u8(udb4_rgb.val[0]));
                sqw_g = vreinterpretq_s16_u16(vmovl_u8(udb4_rgb.val[1]));
                sqw_b = vreinterpretq_s16_u16(vmovl_u8(udb4_rgb.val[2]));
            } else {
                sqw_r = vreinterpretq_s16_u16(vmovl_u8(udb4_rgb.val[2]));
                sqw_g = vreinterpretq_s16_u16(vmovl_u8(udb4_rgb.val[1]));
                sqw_b = vreinterpretq_s16_u16(vmovl_u8(udb4_rgb.val[0]));
            }

            /* Y = ( 77 * R + 150 * G +  29 * B) >> 8 + 0; */
            // y = r * 77
            sqw_y = vmulq_n_s16(sqw_r, 77);
            // y = y + (g * 150)
            sqw_y = vmlaq_n_s16(sqw_y, sqw_g, 150);
            // y = y + (b * 29)
            sqw_y = vmlaq_n_s16(sqw_y, sqw_b, 29);
            // y = (y >> 8)
            sqw_y = vshrq_n_s16(sqw_y, 8);

            /* U = (-43 * R -  84 * G + 127 * B) >> 8 + 128; */
            // u = r x -43
            sqw_u = vmulq_n_s16(sqw_r, -43);
            // u = u + (g x -84)
            sqw_u = vmlaq_n_s16(sqw_u, sqw_g, -84);
            // u = u + (b x 127)
            sqw_u = vmlaq_n_s16(sqw_u, sqw_b, 127);
            // u = (u >> 8) + v16x8(128)
            sqw_u = vsraq_n_s16(sqw_128, sqw_u, 8);

            /* V = (127 * R - 106 * G -  21 * B) >> 8 + 128; */
            // v = r x 127
            sqw_v = vmulq_n_s16(sqw_r, 127);
            // v = v + (g x -106)
            sqw_v = vmlaq_n_s16(sqw_v, sqw_g, -106);
            // v = v + (b x -21)
            sqw_v = vmlaq_n_s16(sqw_v, sqw_b, -21);
            // v = (u >> v) + v16x8(128)
            sqw_v = vsraq_n_s16(sqw_128, sqw_v, 8);

            // yuv 값 s16에서 u16으로 변경 후 다시 u8으로 변경
            /* narrow unsigned rgb */
            udb_y = vmovn_u16(vreinterpretq_u16_s16(sqw_y));
            udb_u = vmovn_u16(vreinterpretq_u16_s16(sqw_u));
            udb_v = vmovn_u16(vreinterpretq_u16_s16(sqw_v));

            /* load original yuyv 422 to 444 */
            // v0 : y1 y2 y3 y4 y5 y6 y7 y8
            // v1 : u1 v1 u2 v2 u3 v3 u4 v4
            udb2_org = vld2_u8(dst + i * 2);
            udb_oy = udb2_org.val[0];
            // v0 : u1 u1 u2 u2 u3 u3 u4 u4     // dst u
            // v1 : v1 v1 v2 v2 v3 v3 v4 v4     // dst v
            udb2_uv = vtrn_u8(udb2_org.val[1], udb2_org.val[1]);

            /* alpha' = ~alpha */
            udb_n = vmvn_u8(udb_a);

            /* blending = ((alpha x src + src) + (alpha' x org)) / 2  */
            /* why (+ src)? : (alpha + 1) + alpha' = 0x100 */
            // v16_y = ~alpha x dst_v8_y
            uqh_y = vmull_u8(udb_n, udb_oy);
            // v16_y = v16_y + (alpha x src_v8_y)
            uqh_y = vmlal_u8(uqh_y, udb_a, udb_y);
            // src_v8_y = (v16_y >> 8)
            udb_y = vshrn_n_u16(uqh_y, 8);

            // v16_u = ~alpha x dst_v8_u
            uqh_u = vmull_u8(udb_n, udb2_uv.val[0]);
            // v16_u = v16_u + (alpha x src_v8_u)
            uqh_u = vmlal_u8(uqh_u, udb_a, udb_u);
            // src_v8_u = (v16_u >> 8)
            udb_u = vshrn_n_u16(uqh_u, 8);

            // v16_v = ~alpha x dst_v8_v
            uqh_v = vmull_u8(udb_n, udb2_uv.val[1]);
            // v16_v = v16_v + (alpha x src_v8_v)
            uqh_v = vmlal_u8(uqh_v, udb_a, udb_v);
            // src_v8_v = (v16_v >> 8)
            udb_v = vshrn_n_u16(uqh_v, 8);

            /* 444 to 422 subsampling also - u, v : averaging */
            // val[0] = y1 y2 y3 ... y8
            udb2_yuyv.val[0] = udb_y;
            // uv_v0 : u1 v1 u2 v2 u3 v3 u4 v4
            // uv_v1 : u1 v1 u2 v2 u3 v3 u4 v4
            udb2_uv = vtrn_u8(udb_u, udb_v);
            // val[1] = (u1 + u1)/2 (v1 + v1)/2 ... (v4 + v4)/2 :: (uv_v0 + uv_v1)/2
            // so, val[1] = u1 v1 u2 v2 u3 v3 u4 v4
            udb2_yuyv.val[1] = vhadd_u8(udb2_uv.val[0], udb2_uv.val[1]);

            /* store yuyv */
            vst2_u8(dst + i * 2, udb2_yuyv);
        } // if
    } // for

    return;
}

void neon_Mix_Rgba2Yuyv(uint8_t *src, uint8_t *dst, int32_t w, int32_t h)
{
    neon_Mix_Rgbx2Yuyv(src, dst, w, h, 1);
}

void neon_Mix_Bgra2Yuyv(uint8_t *src, uint8_t *dst, int32_t w, int32_t h)
{
    neon_Mix_Rgbx2Yuyv(src, dst, w, h, 0);
}

#define ELEM_X          (8)
#define ELEM_Y          (8)
#define CELL_X          (8)
#define CELL_Y          (8)
#define CELL8_Y_OFFSET  (64)    //(ELEM_Y*CELL_Y)
#define CELL_SIZE       (8)    //(CELL_X*CELL_Y)
#define TILE_SIZE       (64)    //(CELL_X*CELL_Y)
#define VALID_CELL_X    (tinfo->dim.w>>3)//(tinfo->dim.w/ELEM_X) // 40/8=5, (dim.w / ELEM_X)
#define VALID_CELL_Y    (tinfo->dim.h>>3)//(tinfo->dim.h/ELEM_Y) // 24/8=3, (dim.h / ELEM_Y)
#define CELL1_SIZE      (64)    //(CELL_X*CELL_Y) // 64
#define CELL8_SIZE      (512)   //(CELL1_SIZE*CELL_X) // 64 * 8

int nc_neon_get_data_NCHW_float(aiwTensorInfo *tinfo, unsigned char *tiled, float *scanline)
{
    unsigned int w_idx=0, ch_offs=0, in_buf_offs=0;
    unsigned int offs_tile_x=0, offs_tile_y=0, offs_cell_x=0, offs_cell_y=0;
    unsigned int valid_cell_x=CELL_X, valid_cell_y=CELL_Y;
    unsigned int valid_ele_x=ELEM_X, valid_ele_y=ELEM_Y;
    unsigned int tile_x = 0, tile_y = 0;
    unsigned int x_cell = 0, y_cell = 0;
    unsigned int y = 0;
    int8x8_t s88;
    float32x4_t f324div;
    float32x4x2_t f3242;

    memset(&f3242, 0, sizeof(f3242));

    if(!tinfo || !tiled || !scanline)
    {
        printf("null pointer (%s) 0x%lx, 0x%lx, 0x%lx\n", __func__, (uintptr_t)tinfo, (uintptr_t)tiled, (uintptr_t)scanline);
        return AIW_ERROR;
    }

    const unsigned int num_x_tiles = (tinfo->dim.w + TILE_SIZE - 1) / TILE_SIZE;
    const unsigned int num_y_tiles = (tinfo->dim.h + TILE_SIZE - 1) / TILE_SIZE;
    const unsigned int plane_size  = (tinfo->dim.w * tinfo->dim.h);  // 1 channel size
    const unsigned int tile_y_size = (tinfo->dim.w * CELL8_Y_OFFSET);  // tile y line size
    const unsigned int cell_y_size = (CELL1_SIZE * (tinfo->dim.w)/8);  // cell y line size  
    unsigned int remain_valid_cell_x = ((tinfo->dim.w % TILE_SIZE)+7)/CELL_X;
    unsigned int remain_valid_cell_y = ((tinfo->dim.h % TILE_SIZE)+7)/CELL_Y;
    unsigned int remain_valid_elem_x = (tinfo->dim.w % ELEM_X);
    unsigned int remain_valid_elem_y = (tinfo->dim.h % ELEM_Y);
    const float multiplier = tinfo->exponent > 0 ? (float)(1 << tinfo->exponent) : (float)(1 << -tinfo->exponent);

    if(remain_valid_cell_x == 0) { remain_valid_cell_x = CELL_X; }
    if(remain_valid_cell_y == 0) { remain_valid_cell_y = CELL_Y; }
    if(remain_valid_elem_x == 0) { remain_valid_elem_x = ELEM_X; }
    if(remain_valid_elem_y == 0) { remain_valid_elem_y = ELEM_Y; }

    f324div =vld1q_dup_f32(&multiplier);
    for (long ch = 0; ch < tinfo->dim.ch; ch++)
    {
        valid_cell_x = CELL_X; valid_cell_y = CELL_Y;
        for (tile_y = 0; tile_y < num_y_tiles; tile_y++)
        {
            if(tile_y == (num_y_tiles - 1)) { 
                // Calculate the number of valid cells in the last tile_y.(When tile size is not aligned)
                valid_cell_y = remain_valid_cell_y;
            }
            offs_tile_y = (tile_y * tile_y_size); //Calculate the starting position of the y-axis tile.
            for (tile_x = 0; tile_x < num_x_tiles; tile_x++) // count X_tiles in one channel.
            {
                if(tile_x == (num_x_tiles - 1)) {
                    // Calculate the number of valid cells in the last tile_x.(When tile size is not aligned)
                    valid_cell_x = remain_valid_cell_x;
                }
                // tile0: 0 8 16 24 32 40 48 56, tile1:64 72 80 88 96 104 112 120, 128 136 .......
                offs_tile_x = (tile_x * TILE_SIZE); //Calculate the starting position of the x-axis tile.
                for (y_cell = 0; y_cell < CELL_Y; y_cell++)
                {
                    if(y_cell >= valid_cell_y) { // Invalid ycells are skipped. 
                        in_buf_offs += CELL8_SIZE; // Skip the padding area.
                        continue;
                    }
                    valid_ele_x = ELEM_X;
                    valid_ele_y = ELEM_Y;
                    if(y_cell == (valid_cell_y-1)) { // last y-axis cell
                        valid_ele_y = remain_valid_elem_y;
                    }
                    offs_cell_y = (y_cell*cell_y_size);
                    for (x_cell = 0; x_cell < CELL_X; x_cell++)
                    {
                        if(x_cell >= valid_cell_x) { // Invalid xcells are skipped.
                            in_buf_offs+=CELL1_SIZE; // Skip the padding area
                            continue;
                        }
                        if(x_cell == (valid_cell_x-1)) {
                            valid_ele_x = remain_valid_elem_x;
                        }
                        offs_cell_x = (x_cell*CELL_X);
                        for (y = 0; y < ELEM_Y; y++)
                        {  // 8x8 element
                            if(y >= valid_ele_y) { // Invalid elements are skipped.
                                in_buf_offs+=ELEM_Y;
                                continue;
                            }
                            w_idx = ch_offs + offs_tile_y + offs_tile_x + offs_cell_y + offs_cell_x + (y*tinfo->dim.w);
                            s88=vld1_s8((const signed char*)tiled+in_buf_offs); // read 8 bytes
                            if(tinfo->sign) {
                                f3242.val[0][0] = s88[0];
                                f3242.val[0][1] = s88[1];
                                f3242.val[0][2] = s88[2];
                                f3242.val[0][3] = s88[3];
                            }
                            else {
                                f3242.val[0][0] = (unsigned char)s88[0];
                                f3242.val[0][1] = (unsigned char)s88[1];
                                f3242.val[0][2] = (unsigned char)s88[2];
                                f3242.val[0][3] = (unsigned char)s88[3];
                            }
                            f3242.val[0] = vdivq_f32(f3242.val[0], f324div);  // (val/multiplier)
                            vst1q_f32(scanline+w_idx, f3242.val[0]);
                            if(valid_ele_x > 4) {
                                if(tinfo->sign) {
                                    f3242.val[1][0] = s88[4];
                                    f3242.val[1][1] = s88[5];
                                    f3242.val[1][2] = s88[6];
                                    f3242.val[1][3] = s88[7];
                                }
                                else {
                                    f3242.val[1][0] = (unsigned char)s88[4];
                                    f3242.val[1][1] = (unsigned char)s88[5];
                                    f3242.val[1][2] = (unsigned char)s88[6];
                                    f3242.val[1][3] = (unsigned char)s88[7];
                                }
                                f3242.val[1] = vdivq_f32(f3242.val[1], f324div);  // (
                                vst1q_f32(scanline+w_idx+4, f3242.val[1]);
                            }
                            in_buf_offs+=ELEM_X;  // 8바이트 읽어서 처리
                        }
                    }
                }
            }
        }
        ch_offs += plane_size;
    }

    return AIW_SUCCESS;
}

#define SEG_COLOR     (0x400000FFU)
int nc_neon_tiled_to_scanline_n_scale_up(unsigned int npu_seg_out_w, unsigned int npu_seg_out_h, unsigned int canvas_w, unsigned int canvas_h, unsigned int *canvas, unsigned char *cnn_output) {
    unsigned int in_x_idx, in_y_idx;
    float xpos, ypos;
    int outidx = 0;
    unsigned int seg_valid_cell_x = (((npu_seg_out_w % TILE_SIZE)+7)>>3);
    unsigned int seg_valid_cell_y = (((npu_seg_out_h % TILE_SIZE)+7)>>3);
    unsigned int valid_cell_x=CELL_X, valid_cell_y=CELL_Y;
    const unsigned int num_x_tiles=(npu_seg_out_w+TILE_SIZE-1)/TILE_SIZE;  // calc the number of x-tiles
    const unsigned int num_y_tiles=(npu_seg_out_h+TILE_SIZE-1)/TILE_SIZE; // calc the number of y-tiles
    const float h_ratio = (float)canvas_w/(float)npu_seg_out_w;
    const float v_ratio = (float)canvas_h/(float)npu_seg_out_h;
    const uint32x4_t u324 ={SEG_COLOR, SEG_COLOR, SEG_COLOR, SEG_COLOR};
    uint8x8_t u88;
    unsigned int x_super = 0, y_super = 0;
    unsigned int x_sub = 0, y_sub = 0;
    unsigned int x = 0, y = 0;
    unsigned int y_up = 0;

    if(!canvas || !cnn_output)
    {
        printf("null pointer (%s) 0x%lx, 0x%lx\n", __func__, (uintptr_t)canvas, (uintptr_t)cnn_output);
        return NC_INVALID;
    }

    valid_cell_x = (npu_seg_out_w % TILE_SIZE) ? seg_valid_cell_x : CELL_X;
    valid_cell_y = (npu_seg_out_h % TILE_SIZE) ? seg_valid_cell_y : CELL_Y;
    for (y_super = 0; y_super < num_y_tiles; y_super++) {
        for (x_super = 0; x_super < num_x_tiles; x_super++) {
            for (y_sub = 0; y_sub < CELL_Y; y_sub++) {
                if(y_super == (num_y_tiles-1)) {  // last y tile
                    if(y_sub >= valid_cell_y) {   // skip invalid y cells
                        outidx+=(CELL8_SIZE * valid_cell_y);
                        break;
                    }
                }
                for (x_sub = 0; x_sub < CELL_X; x_sub++) {
                    if(x_super == (num_x_tiles-1)) {  // last x tile
                        if(x_sub >= valid_cell_x) {   // skip invalid x cells
                            outidx+=(CELL1_SIZE * valid_cell_x);
                            break;
                        }
                    }
                    for (y = 0; y < ELEM_Y; y++) {
                        in_y_idx = (y_super * TILESIZE) + (y_sub * CELL_Y + y);
                        in_x_idx = (x_super * TILESIZE) + (x_sub * CELL_X);
                        u88=vld1_u8((unsigned char*)cnn_output+outidx); // read 8 bytes
                        ypos = (float)in_y_idx * v_ratio;

                        for (x = 0; x < ELEM_X; x++) {
                            // increase drawing speed (not drawing background)
                            if (u88[x]) {
                                continue;
                            }
                            xpos = (float)(in_x_idx+x) * h_ratio;
                            if (ypos > 0) {
                                for (y_up = 0; y_up < (unsigned int)v_ratio; y_up++) { // line
#if 1
                                    vst1q_u32(canvas + (unsigned int)((ypos + (float)y_up)* (float)canvas_w +(xpos)), u324);// ARGB x 4 pixels, peleeSeg 320x192
                                    //vst1q_u32(canvas + ((ypos + y_up)* canvas_w +(xpos))+4, u324);// ARGB x 4 pixels, for pavliteSeg 160x96
#else
                                    for (int x_up = 0; x_up < h_ratio; x_up++) {
                                        canvas[(ypos + y_up) * canvas_w + (xpos + x_up)] = SEG_COLOR;
                                    }
#endif
                                }
                            }
                        }
                        outidx+=8;
                    }
                }
            }
        }
    }
    return NC_SUCCESS;
}

int nc_neon_get_data_NCHW_float_cellrow(aiwTensorInfo *tinfo, unsigned char *tiled, float *scanline)
{
    unsigned int w_idx=0, ch_offs=0, in_buf_offs=0;
    unsigned int offs_cell_x=0, offs_cell_y=0;
    unsigned int valid_ele_x=ELEM_X, valid_ele_y=ELEM_Y;
    unsigned int x_cell = 0, y_cell = 0;
    unsigned int y = 0;
    int8x8_t s88;
    float32x4_t f324div;
    float32x4x2_t f3242;

    memset(&f3242, 0, sizeof(f3242));

    if(!tinfo || !tiled || !scanline)
    {
        printf("null pointer (%s) 0x%lx, 0x%lx, 0x%lx\n", __func__, (uintptr_t)tinfo, (uintptr_t)tiled, (uintptr_t)scanline);
        return AIW_ERROR;
    }

    const unsigned int num_x_cells = (tinfo->dim.w + TILE_SIZE - 1) / TILE_SIZE * (TILE_SIZE / CELL_SIZE);
    const unsigned int num_y_cells = (tinfo->dim.h + CELL_SIZE - 1) / CELL_SIZE;
    const unsigned int plane_size  = (tinfo->dim.w * tinfo->dim.h);  // 1 channel size
    const unsigned int cell_y_size = (CELL1_SIZE * (tinfo->dim.w)/8);  // cell y line size  
    unsigned int remain_valid_cell_x = (tinfo->dim.w + 7) / CELL_X; 
    unsigned int remain_valid_elem_x = (tinfo->dim.w % ELEM_X);
    unsigned int remain_valid_elem_y = (tinfo->dim.h % ELEM_Y);
    const float multiplier = tinfo->exponent > 0 ? (float)(1 << tinfo->exponent) : (float)(1 << -tinfo->exponent);
    
    if(remain_valid_elem_x == 0) { remain_valid_elem_x = ELEM_X; }
    if(remain_valid_elem_y == 0) { remain_valid_elem_y = ELEM_Y; }
     
    f324div =vld1q_dup_f32(&multiplier);
    for (long ch = 0; ch < tinfo->dim.ch; ch++)
    {
        for (y_cell = 0; y_cell < num_y_cells; y_cell++) 
        {
            valid_ele_y = ELEM_Y;
            if(y_cell == (num_y_cells-1)) { // last y-axis cell
                valid_ele_y = remain_valid_elem_y; 
            }
            offs_cell_y = (y_cell*cell_y_size); 
            
            for (x_cell = 0; x_cell < num_x_cells; x_cell++) 
            {
                valid_ele_x = ELEM_X;
                if(x_cell >= remain_valid_cell_x) { // Invalid xcells are skipped.
                    in_buf_offs+=CELL1_SIZE; // Skip the padding area
                    continue;
                }
                if(x_cell == (remain_valid_cell_x-1)) {
                    valid_ele_x = remain_valid_elem_x; 
                }
                offs_cell_x = (x_cell*CELL_X);
    
                for (y = 0; y < ELEM_Y; y++)
                {  // 8x8 element
                    if(y >= valid_ele_y) { // Invalid elements are skipped.
                        in_buf_offs+=ELEM_Y;
                        continue;
                    }
                    w_idx = ch_offs + offs_cell_y + offs_cell_x + (y*tinfo->dim.w);
                    s88=vld1_s8((const signed char*)tiled+in_buf_offs); // read 8 bytes
                    
                    if(tinfo->sign) { 
                        f3242.val[0][0] = s88[0];
                        f3242.val[0][1] = s88[1];
                        f3242.val[0][2] = s88[2];
                        f3242.val[0][3] = s88[3];
                    }
                    else {
                        f3242.val[0][0] = (unsigned char)s88[0];
                        f3242.val[0][1] = (unsigned char)s88[1];
                        f3242.val[0][2] = (unsigned char)s88[2];
                        f3242.val[0][3] = (unsigned char)s88[3];
                    }
                    f3242.val[0] = vdivq_f32(f3242.val[0], f324div);  // (val/multiplier)
                    vst1q_f32(scanline+w_idx, f3242.val[0]); 
                    if(valid_ele_x > 4) {
                        if(tinfo->sign) {
                            f3242.val[1][0] = s88[4];
                            f3242.val[1][1] = s88[5];
                            f3242.val[1][2] = s88[6];
                            f3242.val[1][3] = s88[7];
                        }
                        else {
                            f3242.val[1][0] = (unsigned char)s88[4];
                            f3242.val[1][1] = (unsigned char)s88[5];
                            f3242.val[1][2] = (unsigned char)s88[6];
                            f3242.val[1][3] = (unsigned char)s88[7];
                        }
                        f3242.val[1] = vdivq_f32(f3242.val[1], f324div);  
                        vst1q_f32(scanline+w_idx+4, f3242.val[1]); 
                    }
                    in_buf_offs+=ELEM_X;  // 8바이트 읽어서 처리
                }
            }
        }
        ch_offs += plane_size;
    }

    return AIW_SUCCESS;
}

int nc_neon_get_data_NCHW_uint8_cellrow(aiwTensorInfo *tinfo, unsigned char *tiled, int64_t *scanline)
{
    unsigned int w_idx=0, ch_offs=0, in_buf_offs=0;
    unsigned int offs_cell_x=0, offs_cell_y=0;
    unsigned int valid_ele_x=ELEM_X, valid_ele_y=ELEM_Y;
    unsigned int x_cell = 0, y_cell = 0;
    unsigned int y = 0;
    int8x8_t s88;

    memset(&s88, 0, sizeof(s88));

    if(!tinfo || !tiled || !scanline)
    {
        printf("null pointer (%s) 0x%lx, 0x%lx, 0x%lx\n", __func__, (uintptr_t)tinfo, (uintptr_t)tiled, (uintptr_t)scanline);
        return AIW_ERROR;
    }

    const unsigned int num_x_cells = (tinfo->dim.w + TILE_SIZE - 1) / TILE_SIZE * (TILE_SIZE / CELL_SIZE);
    const unsigned int num_y_cells = (tinfo->dim.h + CELL_SIZE - 1) / CELL_SIZE;
    const unsigned int plane_size  = (tinfo->dim.w * tinfo->dim.h); 
    const unsigned int cell_y_size = (CELL1_SIZE * (tinfo->dim.w)/8); 
    unsigned int remain_valid_cell_x = (tinfo->dim.w + 7) / CELL_X; 
    unsigned int remain_valid_elem_x = (tinfo->dim.w % ELEM_X);
    unsigned int remain_valid_elem_y = (tinfo->dim.h % ELEM_Y);
    
    if(remain_valid_elem_x == 0) { remain_valid_elem_x = ELEM_X; }
    if(remain_valid_elem_y == 0) { remain_valid_elem_y = ELEM_Y; }
     
    for (long ch = 0; ch < tinfo->dim.ch; ch++)
    {
        for (y_cell = 0; y_cell < num_y_cells; y_cell++) 
        {
            valid_ele_y = ELEM_Y;
            if(y_cell == (num_y_cells-1)) { 
                valid_ele_y = remain_valid_elem_y; 
            }
            offs_cell_y = (y_cell*cell_y_size); 
            
            for (x_cell = 0; x_cell < num_x_cells; x_cell++) 
            {
                valid_ele_x = ELEM_X;
                if(x_cell >= remain_valid_cell_x) { 
                    in_buf_offs+=CELL1_SIZE;
                    continue;
                }
                if(x_cell == (remain_valid_cell_x-1)) { 
                    valid_ele_x = remain_valid_elem_x; 
                }
                offs_cell_x = (x_cell*CELL_X);
    
                for (y = 0; y < ELEM_Y; y++)
                {  
                    if(y >= valid_ele_y) { 
                        in_buf_offs+=ELEM_Y;
                        continue;
                    }
                    w_idx = ch_offs + offs_cell_y + offs_cell_x + (y*tinfo->dim.w);
                    s88=vld1_s8((const signed char*)tiled+in_buf_offs);
                    memcpy((int8_t*)scanline + w_idx, &s88, 4);

                    if(valid_ele_x > 4) { 
                        memcpy((int8_t*)scanline + w_idx+4, &s88[4], 4);
                    }
                    in_buf_offs+=ELEM_X; 
                }
            }
        }
        ch_offs += plane_size;
    }
    return AIW_SUCCESS;
}



void nc_rgb_planar_to_interleaved_neon(uint8_t* R, uint8_t* G, uint8_t* B, uint8_t* interleaved, int width, int height)
{
    int n = width * height;
    int i;
    uint8x8x3_t v;

    for (i = 0; i < n; i += 8) {
        // Load 8 elements from each R, G, and B arrays into NEON registers
        // Interleave the RGB data and store it in the interleaved array
        v.val[0] = vld1_u8(&R[i]);
        v.val[1] = vld1_u8(&G[i]);
        v.val[2] = vld1_u8(&B[i]);

        vst3_u8(&interleaved[3 * i], v);
    }
}

void nc_rgb_interleaved_to_planar_neon(unsigned char* interleaved, unsigned char* R, unsigned char* G, unsigned char* B, int w, int h)
{
	int n = w * h;
	uint8x8x3_t u883_rgb;

	for(int i=0; i < n; i += 8) {
		u883_rgb = vld3_u8(interleaved + i *3);
		vst1_u8(&R[i], u883_rgb.val[0]);
		vst1_u8(&G[i], u883_rgb.val[1]);
		vst1_u8(&B[i], u883_rgb.val[2]);
	}
}
