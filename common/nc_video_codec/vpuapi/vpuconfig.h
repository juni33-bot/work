//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
//
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
//
// The entire notice above must be reproduced on all authorized copies.
//
// Description  :
//-----------------------------------------------------------------------------

#ifndef _VPU_CONFIG_H_
#define _VPU_CONFIG_H_
#include "../config.h"


#define ENC_STREAM_BUF_COUNT 5
#define ENC_STREAM_BUF_SIZE  0xF00000

#define BODA950_CODE                    0x9500
#define CODA960_CODE                    0x9600
#define CODA980_CODE                    0x9800

#define WAVE517_CODE                    0x5170
#define WAVE537_CODE                    0x5370
#define WAVE511_CODE                    0x5110
#define WAVE521_CODE                    0x5210
#define WAVE521C_CODE                   0x521c
#define WAVE521C_DUAL_CODE              0x521d  // wave521 dual core
#define WAVE521E1_CODE                  0x5211

#define PRODUCT_CODE_W_SERIES(x)        (x == WAVE517_CODE || x == WAVE537_CODE || x == WAVE511_CODE || x == WAVE521_CODE || x == WAVE521E1_CODE || x == WAVE521C_CODE || x == WAVE521C_DUAL_CODE)
#define PRODUCT_CODE_NOT_W_SERIES(x)    (x == BODA950_CODE || x == CODA960_CODE || x == CODA980_CODE)

#define WAVE517_WORKBUF_SIZE            (2*1024*1024)
#define WAVE521ENC_WORKBUF_SIZE         (128*1024)      //HEVC 128K, AVC 40K
#define WAVE521DEC_WORKBUF_SIZE         (1784*1024)

#define MAX_INST_HANDLE_SIZE            48              /* DO NOT CHANGE THIS VALUE */
#define MAX_NUM_INSTANCE                4
#define MAX_NUM_VPU_CORE                1
#define MAX_NUM_VCORE                   1

    #define MAX_ENC_AVC_PIC_WIDTH       4096
    #define MAX_ENC_AVC_PIC_HEIGHT      2304
#define MAX_ENC_PIC_WIDTH               4096
#define MAX_ENC_PIC_HEIGHT              2304
#define MIN_ENC_PIC_WIDTH               96
#define MIN_ENC_PIC_HEIGHT              16

#define W5_MIN_ENC_PIC_WIDTH            256
#define W5_MIN_ENC_PIC_HEIGHT           128
#define W5_MAX_ENC_PIC_WIDTH            8192
#define W5_MAX_ENC_PIC_HEIGHT           8192

#define MAX_DEC_PIC_WIDTH               4096
#define MAX_DEC_PIC_HEIGHT              2304

#define MAX_CTU_NUM                     0x4000      // CTU num for max resolution = 8192x8192/(64x64)
#define MAX_SUB_CTU_NUM	                (MAX_CTU_NUM*4)
#define MAX_MB_NUM                      0x40000     // MB num for max resolution = 8192x8192/(16x16)

//  Application specific configuration
#if   defined(SUPPORT_DEC_RINGBUFFER_PERFORMANCE) || defined (SUPPORT_INTERNAL_PERFORMANCE)
#define VPU_ENC_TIMEOUT                 60000
#define VPU_DEC_TIMEOUT                 (60000*10)
#define VPU_BUSY_CHECK_TIMEOUT          (10000*10)
#else
#define VPU_ENC_TIMEOUT                 60000
#define VPU_DEC_TIMEOUT                 60000
#define VPU_BUSY_CHECK_TIMEOUT          10000
#endif

// codec specific configuration
#define CBCR_INTERLEAVE			        1 //[default 1 for BW checking with CnMViedo Conformance] 0 (chroma separate mode), 1 (chroma interleave mode) // if the type of tiledmap uses the kind of MB_RASTER_MAP. must set to enable CBCR_INTERLEAVE

#define HOST_ENDIAN                     VDI_128BIT_LITTLE_ENDIAN
#define VPU_FRAME_ENDIAN                HOST_ENDIAN
#define VPU_STREAM_ENDIAN               HOST_ENDIAN
#define VPU_USER_DATA_ENDIAN            HOST_ENDIAN
#define VPU_SOURCE_ENDIAN               HOST_ENDIAN
#define DRAM_BUS_WIDTH                  16

// for WAVE Encoder
#define USE_SRC_PRP_AXI         0
#define USE_SRC_PRI_AXI         1
#define DEFAULT_SRC_AXI         USE_SRC_PRP_AXI

/************************************************************************/
/* VPU COMMON MEMORY                                                    */
/************************************************************************/
#define VLC_BUF_NUM                     (3)
#define REPORT_QUEUE_COUNT              (16)
#define COMMAND_QUEUE_DEPTH             (4)

        #ifdef SUPPORT_TESTCASE_CQ_16
            #define ENC_SRC_BUF_NUM             (20+COMMAND_QUEUE_DEPTH)
        #else
            #define ENC_SRC_BUF_NUM             20       // [FIX ME]
        #endif

#define ONE_TASKBUF_SIZE_FOR_W5DEC_CQ         (8*1024*1024)   /* upto 8Kx4K, need 8Mbyte per task*/
#define ONE_TASKBUF_SIZE_FOR_W5ENC_CQ         (8*1024*1024)  /* upto 8Kx8K, need 8Mbyte per task.*/
#define ONE_TASKBUF_SIZE_FOR_W511DEC_CQ       (8*1024*1024)  /* upto 8Kx8K, need 8Mbyte per task.*/

/* FW will return one TASKBUF size base on MaxCPB (according to the SPEC), but this size will be quite big depend on profile/level.*/
/* ex) main10, 8kx8k = 180Mbytes will be returned */
/* Thus, if host can set size limitation for one TASKBUF size. (but, small size limitation can cause processing error)  */
#define ONE_TASKBUF_MAX_SIZE_LIMIT_DEC          (8*1024*1024)
#define ONE_TASKBUF_MAX_SIZE_LIMIT_ENC          (20*1024*1024)

#define W5_REMAP_INDEX0                 0
#define W5_REMAP_INDEX1                 1
#define W5_REMAP_MAX_SIZE               (1024*1024)

#define WAVE5_MAX_CODE_BUF_SIZE         (2*1024*1024)
#define WAVE5_TEMPBUF_OFFSET            WAVE5_MAX_CODE_BUF_SIZE
#define WAVE5_TEMPBUF_SIZE              (1024*1024)

    #define ONE_TASKBUF_SIZE_FOR_CQ     0
    #define SIZE_COMMON                 (WAVE5_MAX_CODE_BUF_SIZE + WAVE5_TEMPBUF_SIZE)

//=====4. VPU REPORT MEMORY  ======================//
#define SIZE_REPORT_BUF                 (0x10000)

#define STREAM_END_SIZE                 0
#define STREAM_END_SET_FLAG             0
#define STREAM_END_CLEAR_FLAG           -1
#define EXPLICIT_END_SET_FLAG           -2

#define UPDATE_NEW_BS_BUF               0

#define USE_BIT_INTERNAL_BUF            1
#define USE_IP_INTERNAL_BUF             1
#define USE_DBKY_INTERNAL_BUF           1
#define USE_DBKC_INTERNAL_BUF           1
#define USE_OVL_INTERNAL_BUF            1
#define USE_BTP_INTERNAL_BUF            1
#define USE_ME_INTERNAL_BUF             1

/* WAVE410 only */
#define USE_BPU_INTERNAL_BUF            1
#define USE_VCE_IP_INTERNAL_BUF         1
#define USE_VCE_LF_ROW_INTERNAL_BUF     1

/* WAVE420 only */
#define USE_IMD_INTERNAL_BUF            1
#define USE_RDO_INTERNAL_BUF            1
#define USE_LF_INTERNAL_BUF             1


#define WAVE5_UPPER_PROC_AXI_ID     0x0

#define WAVE5_PROC_AXI_ID           0x0
#define WAVE5_PRP_AXI_ID            0x0
#define WAVE5_FBD_Y_AXI_ID          0x0
#define WAVE5_FBC_Y_AXI_ID          0x0
#define WAVE5_FBD_C_AXI_ID          0x0
#define WAVE5_FBC_C_AXI_ID          0x0
#define WAVE5_SEC_AXI_ID            0x0
#define WAVE5_PRI_AXI_ID            0x0


#endif  /* _VPU_CONFIG_H_ */

