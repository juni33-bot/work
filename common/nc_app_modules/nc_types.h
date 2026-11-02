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
* @file    : Type.h
*
* @brief   : Define types
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2021.04.23.
*
* @version : 1.0.0
********************************************************************************
* @note
* 04.23.2021 / 1.0.0 / Initial released.
*
********************************************************************************
*/

#ifndef TYPE_H_
#define TYPE_H_

#include "stdint.h"

/*
********************************************************************************
*               TYPEDEFS
********************************************************************************
*/

typedef uint64_t            UINT64;
typedef uint32_t            UINT32;
typedef uint16_t            UINT16;
typedef uint8_t             UINT8;

typedef int64_t             INT64;
typedef int32_t             INT32;
typedef int16_t             INT16;
typedef int8_t              INT8;

typedef int8_t              CHAR;

typedef float               FLOAT;
typedef double              DOUBLE;

typedef uint8_t             BOOL;

/*
********************************************************************************
*               LOGICAL SYMBOL TYPE
********************************************************************************
*/

#ifndef TRUE
#define TRUE                (1U)
#endif

#ifndef FALSE
#define FALSE               (0U)
#endif

#ifndef OK
#define OK                  (1U)
#endif

#ifndef ERROR
#define ERROR               (0U)
#endif

#ifndef ON
#define ON                  (1U)
#endif

#ifndef OFF
#define OFF                 (0U)
#endif

#ifndef ENABLE
#define ENABLE              (1U)
#endif

#ifndef DISABLE
#define DISABLE             (0U)
#endif

#ifndef HIGH
#define HIGH                (1U)
#endif

#ifndef LOW
#define LOW                 (0U)
#endif

#ifndef NC_START
#define NC_START            (1U)
#endif

#ifndef NC_STOP
#define NC_STOP             (0U)
#endif

#ifndef NULL
#define NULL                (0U)
#endif

/* function return value (ISP) */
typedef UINT32 RTN_STATE;
#define RTN_SUCCESS         (0U)
#define RTN_FAILURE         (1U)

/* function return value */
typedef INT32 NC_ERROR;
#define NC_TIMEOUT          (-4)
#define NC_UNINITIALIZE     (-3)    /* Function calls in the state that the block is not initialized. */
#define NC_INVALID          (-2)    /* Invalid Parameter */
#define NC_FAILURE          (-1)
#define NC_SUCCESS          (0)
#define NC_WAIT             (1)

#define KB                  (1024U)
#define MB                  (1048576U)

#define KHZ                 (1000U)
#define MHZ                 (1000000U)

#define SEC                 (1U)
#define MSEC                (1000U)
#define USEC                (1000000U)
#define NSEC                (1000000000U)

/*
********************************************************************************
*               ASCII CHARACTER TYPE
********************************************************************************
*/

#define CHAR_NUL            (0U)    /* \0 */
#define CHAR_BS             (8U)    /* Backspace */
#define CHAR_LF             (10U)    /* Line feed */
#define CHAR_CR             (13U)    /* \r */
#define CHAR_SHARP          (35U)    /* # */
#define CHAR_PERCENT        (37U)    /* % */
#define CHAR_SLASH          (47U)    /* / */
#define CHAR_0              (48U)
#define CHAR_1              (49U)
#define CHAR_2              (50U)
#define CHAR_3              (51U)
#define CHAR_4              (52U)
#define CHAR_5              (53U)
#define CHAR_6              (54U)
#define CHAR_7              (55U)
#define CHAR_8              (56U)
#define CHAR_9              (57U)
#define CHAR_COLON          (58U)
#define CHAR_EQUALS         (61U)
#define CHAR_QUESTION       (63U)
#define CHAR_AT             (64U)    /* commercial at */
#define CHAR_A              (65U)
#define CHAR_B              (66U)
#define CHAR_C              (67U)
#define CHAR_D              (68U)
#define CHAR_E              (69U)
#define CHAR_F              (70U)
#define CHAR_G              (71U)
#define CHAR_H              (72U)
#define CHAR_I              (73U)
#define CHAR_J              (74U)
#define CHAR_K              (75U)
#define CHAR_L              (76U)
#define CHAR_M              (77U)
#define CHAR_N              (78U)
#define CHAR_O              (79U)
#define CHAR_P              (80U)
#define CHAR_Q              (81U)
#define CHAR_R              (82U)
#define CHAR_S              (83U)
#define CHAR_T              (84U)
#define CHAR_U              (85U)
#define CHAR_V              (86U)
#define CHAR_W              (87U)
#define CHAR_X              (88U)
#define CHAR_Y              (89U)
#define CHAR_Z              (90U)
#define CHAR_a              (97U)
#define CHAR_b              (98U)
#define CHAR_c              (99U)
#define CHAR_d              (100U)
#define CHAR_e              (101U)
#define CHAR_f              (102U)
#define CHAR_g              (103U)
#define CHAR_h              (104U)
#define CHAR_i              (105U)
#define CHAR_j              (106U)
#define CHAR_k              (107U)
#define CHAR_l              (108U)
#define CHAR_m              (109U)
#define CHAR_n              (110U)
#define CHAR_o              (111U)
#define CHAR_p              (112U)
#define CHAR_q              (113U)
#define CHAR_r              (114U)
#define CHAR_s              (115U)
#define CHAR_t              (116U)
#define CHAR_u              (117U)
#define CHAR_v              (118U)
#define CHAR_w              (119U)
#define CHAR_x              (120U)
#define CHAR_y              (121U)
#define CHAR_z              (122U)

/*
********************************************************************************
*               TYPEDEFS
********************************************************************************
*/

/* A function with no argument returning pointer to a void function */
typedef void (*PrHandler)  (UINT32 IrqNum);

typedef union
{
    UINT8 U8T;
    struct
    {
        UINT8 b0 : 1;               /* bit7 */
        UINT8 b1 : 1;               /* bit6 */
        UINT8 b2 : 1;               /* bit5 */
        UINT8 b3 : 1;               /* bit4 */
        UINT8 b4 : 1;               /* bit3 */
        UINT8 b5 : 1;               /* bit2 */
        UINT8 b6 : 1;               /* bit1 */
        UINT8 b7 : 1;               /* bit0 */
    } U8_Bit;
} UINT8_BIT;                        /* BIT : bit access */

typedef union
{
    UINT16 U16T; /* word */
    struct
    {
        UINT16 b0 : 1;              /* bit15 */
        UINT16 b1 : 1;              /* bit14 */
        UINT16 b2 : 1;              /* bit13 */
        UINT16 b3 : 1;              /* bit12 */
        UINT16 b4 : 1;              /* bit11 */
        UINT16 b5 : 1;              /* bit10 */
        UINT16 b6 : 1;              /* bit9 */
        UINT16 b7 : 1;              /* bit8 */
        UINT16 b8 : 1;              /* bit7 */
        UINT16 b9 : 1;              /* bit6 */
        UINT16 b10 : 1;             /* bit5 */
        UINT16 b11 : 1;             /* bit4 */
        UINT16 b12 : 1;             /* bit3 */
        UINT16 b13 : 1;             /* bit2 */
        UINT16 b14 : 1;             /* bit1 */
        UINT16 b15 : 1;             /* bit0 */
    } U16_Bit;
} UINT16_BIT;                       /* BIT : bit access */

typedef union
{
    UINT32 U32T; /* word */
    struct
    {
        UINT32 b0 : 1;              /* bit31 */
        UINT32 b1 : 1;              /* bit30 */
        UINT32 b2 : 1;              /* bit29 */
        UINT32 b3 : 1;              /* bit28 */
        UINT32 b4 : 1;              /* bit27 */
        UINT32 b5 : 1;              /* bit26 */
        UINT32 b6 : 1;              /* bit25 */
        UINT32 b7 : 1;              /* bit24 */
        UINT32 b8 : 1;              /* bit23 */
        UINT32 b9 : 1;              /* bit22 */
        UINT32 b10 : 1;             /* bit21 */
        UINT32 b11 : 1;             /* bit20 */
        UINT32 b12 : 1;             /* bit19 */
        UINT32 b13 : 1;             /* bit18 */
        UINT32 b14 : 1;             /* bit17 */
        UINT32 b15 : 1;             /* bit16 */
        UINT32 b16 : 1;             /* bit15 */
        UINT32 b17 : 1;             /* bit14 */
        UINT32 b18 : 1;             /* bit13 */
        UINT32 b19 : 1;             /* bit12 */
        UINT32 b20 : 1;             /* bit11 */
        UINT32 b21 : 1;             /* bit10 */
        UINT32 b22 : 1;             /* bit9 */
        UINT32 b23 : 1;             /* bit8 */
        UINT32 b24 : 1;             /* bit7 */
        UINT32 b25 : 1;             /* bit6 */
        UINT32 b26 : 1;             /* bit5 */
        UINT32 b27 : 1;             /* bit4 */
        UINT32 b28 : 1;             /* bit3 */
        UINT32 b29 : 1;             /* bit2 */
        UINT32 b30 : 1;             /* bit1 */
        UINT32 b31 : 1;             /* bit0 */
    } U32_Bit;
} UINT32_BIT;                       /* BIT : bit access */

/*
********************************************************************************
*               MACROS
********************************************************************************
*/
/* just for backward compatibility */
#ifndef __linux
#define ALIGN(x, s)                     ((x) + (s)-1U) & ~((s)-1U)
#endif

#define NC_REVERSE(x)                   ((x) = ((x != 0U) ? 0U : 1U))
#define NC_ALIGN(x, s)                  (((x) + s - 1U) & ~(s - 1U))
#define NC_BIT_GET(x, bit)              ((x) & (1UL << (bit)))
#define NC_BIT_SHIFT(x, bit)            (((x) >> (bit)) & 1U)
#define NC_BITS(x, high, low)           ((x) & (((1UL << ((high) + 1U)) - 1U) & ~((1UL << (low)) - 1U)))
#define NC_BITS_SHIFT(x, high, low)     (((x) >> (low)) & ((1UL << ((high) - (low) + 1U)) - 1U))

#ifndef __linux
#define REGRW8(base_addr, offset)       (*(volatile UINT8 *) ((UINT32)(base_addr) + (UINT32)(offset)))
#define REGRW16(base_addr, offset)      (*(volatile UINT16 *)((UINT32)(base_addr) + (UINT32)(offset)))
#define REGRW32(base_addr, offset)      (*(volatile UINT32 *)((UINT32)(base_addr) + (UINT32)(offset)))
#define REGRW64(base_addr, offset)      (*(volatile UINT64 *)((UINT64)(base_addr) + (UINT64)(offset)))
#define REGRW32z(base_addr, offset)     (*(volatile UINT32 *)((UINT32)(base_addr)))
#else
//#include "Linux_Register.h"
extern volatile unsigned int *SFR_Remapped;
#define REGRW8(base_addr, offset)       (*(volatile UINT8 *) ((UINT8*)SFR_Remapped + base_addr + offset - SFR_REMAP_BASE))
#define REGRW16(base_addr, offset)      (*(volatile UINT16 *)((UINT8*)SFR_Remapped + base_addr + offset - SFR_REMAP_BASE))
#define REGRW32(base_addr, offset)      (*(volatile UINT32 *)((UINT8*)SFR_Remapped + base_addr + offset - SFR_REMAP_BASE))
#define REGRW64(base_addr, offset)      (*(volatile UINT64 *)((UINT8*)SFR_Remapped + base_addr + offset - SFR_REMAP_BASE))
#define REGRW32z(base_addr, offset)     (*(volatile UINT32 *)((UINT8*)SFR_Remapped + base_addr))
#endif
#define REGRW32_A(addr)                 (*(volatile UINT32 *)((UINT32)(addr)))

#define ISPRW8(base_addr, offset)       (*(volatile UINT8 *) ((UINT32)(base_addr) + (((UINT32)(offset)) << 2)))
#define ISPRW32(base_addr, offset)      (*(volatile UINT32 *)((UINT32)(base_addr) + (((UINT32)(offset)) << 2)))

#define MEM_ALIGN(s) __attribute__((aligned(s)))

#endif /* TYPE_H_ */

/* End Of File */
