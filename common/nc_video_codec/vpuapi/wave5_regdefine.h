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

#include "../../config.h"

#ifndef __WAVE5_REGISTER_DEFINE_H__
#define __WAVE5_REGISTER_DEFINE_H__

typedef enum {
    W5_INIT_VPU        = 0x0001,
    W5_WAKEUP_VPU      = 0x0002,
    W5_SLEEP_VPU       = 0x0004,
    W5_CREATE_INSTANCE = 0x0008,            /* queuing command */
    W5_FLUSH_INSTANCE  = 0x0010,
    W5_DESTROY_INSTANCE= 0x0020,            /* queuing command */
    W5_INIT_SEQ        = 0x0040,            /* queuing command */
    W5_SET_FB          = 0x0080,
    W5_DEC_PIC         = 0x0100,            /* queuing command */
    W5_ENC_PIC         = 0x0100,            /* queuing command */
    W5_ENC_SET_PARAM   = 0x0200,            /* queuing command */
    W5_QUERY           = 0x4000,
    W5_UPDATE_BS       = 0x8000,
    W5_MAX_VPU_COMD	   = 0x10000,
} W5_VPU_COMMAND;

typedef enum {
    GET_VPU_INFO        = 0,
    SET_WRITE_PROT      = 1,
    GET_RESULT          = 2,
    UPDATE_DISP_FLAG    = 3,
    GET_BW_REPORT       = 4,
    GET_BS_RD_PTR       = 5,    // for decoder
    GET_BS_WR_PTR       = 6,    // for encoder
    GET_SRC_BUF_FLAG    = 7,    // for encoder
    SET_BS_RD_PTR       = 8,    // for decoder
    GET_DEBUG_INFO      = 0x61,
} QUERY_OPT;

#define W5_REG_BASE                     0x00000000
#define W5_CMD_REG_BASE                 0x00000100
#define W5_CMD_REG_END                  0x00000200

/*
 * Common
 */
/* Power On Configuration
 * PO_DEBUG_MODE    [0]     1 - Power On with debug mode
 * USE_PO_CONF      [3]     1 - Use Power-On-Configuration
 */
#define W5_PO_CONF                     (W5_REG_BASE + 0x0000)
#define W5_VCPU_CUR_PC                 (W5_REG_BASE + 0x0004)
#define W5_VCPU_CUR_LR                 (W5_REG_BASE + 0x0008)
#define W5_VPU_PDBG_STEP_MASK_V        (W5_REG_BASE + 0x000C)
#define W5_VPU_PDBG_CTRL               (W5_REG_BASE + 0x0010)         // vCPU debugger ctrl register
#define W5_VPU_PDBG_IDX_REG            (W5_REG_BASE + 0x0014)         // vCPU debugger index register
#define W5_VPU_PDBG_WDATA_REG          (W5_REG_BASE + 0x0018)         // vCPU debugger write data register
#define W5_VPU_PDBG_RDATA_REG          (W5_REG_BASE + 0x001C)         // vCPU debugger read data register

#define W5_VPU_FIO_CTRL_ADDR           (W5_REG_BASE + 0x0020)
#define W5_VPU_FIO_DATA                (W5_REG_BASE + 0x0024)
#define W5_VPU_VINT_REASON_USR         (W5_REG_BASE + 0x0030)
#define W5_VPU_VINT_REASON_CLR         (W5_REG_BASE + 0x0034)
#define W5_VPU_HOST_INT_REQ            (W5_REG_BASE + 0x0038)
#define W5_VPU_VINT_CLEAR              (W5_REG_BASE + 0x003C)
#define W5_VPU_HINT_CLEAR              (W5_REG_BASE + 0x0040)
#define W5_VPU_VPU_INT_STS             (W5_REG_BASE + 0x0044)
#define W5_VPU_VINT_ENABLE             (W5_REG_BASE + 0x0048)
#define W5_VPU_VINT_REASON             (W5_REG_BASE + 0x004C)
#define W5_VPU_RESET_REQ               (W5_REG_BASE + 0x0050)
#define W5_RST_BLOCK_CCLK(_core)       (1<<_core)
#define W5_RST_BLOCK_CCLK_ALL          (0xff)
#define W5_RST_BLOCK_BCLK(_core)       (0x100<<_core)
#define W5_RST_BLOCK_BCLK_ALL          (0xff00)
#define W5_RST_BLOCK_ACLK(_core)       (0x10000<<_core)
#define W5_RST_BLOCK_ACLK_ALL          (0xff0000)
#define W5_RST_BLOCK_VCPU_ALL          (0x3f000000)
#define W5_RST_BLOCK_ALL               (0x3fffffff)
#define W5_VPU_RESET_STATUS            (W5_REG_BASE + 0x0054)

#define W5_VCPU_RESTART                (W5_REG_BASE + 0x0058)
#define W5_VPU_CLK_MASK                (W5_REG_BASE + 0x005C)


/* REMAP_CTRL
 * PAGE SIZE:   [8:0]   0x001 - 4K
 *                      0x002 - 8K
 *                      0x004 - 16K
 *                      ...
 *                      0x100 - 1M
 * REGION ATTR1 [10]    0     - Normal
 *                      1     - Make Bus error for the region
 * REGION ATTR2 [11]    0     - Normal
 *                      1     - Bypass region
 * REMAP INDEX  [15:12]       - 0 ~ 3
 * ENDIAN       [19:16]       - See EndianMode in vdi.h
 * AXI-ID       [23:20]       - Upper AXI-ID
 * BUS_ERROR    [29]    0     - bypass
 *                      1     - Make BUS_ERROR for unmapped region
 * BYPASS_ALL   [30]    1     - Bypass all
 * ENABLE       [31]    1     - Update control register[30:16]
 */
#define W5_VPU_REMAP_CTRL                       (W5_REG_BASE + 0x0060)
#define W5_VPU_REMAP_VADDR                      (W5_REG_BASE + 0x0064)
#define W5_VPU_REMAP_PADDR                      (W5_REG_BASE + 0x0068)
#define W5_VPU_REMAP_CORE_START                 (W5_REG_BASE + 0x006C)
#define W5_VPU_BUSY_STATUS                      (W5_REG_BASE + 0x0070)
#define W5_VPU_HALT_STATUS                      (W5_REG_BASE + 0x0074)
#define W5_VPU_VCPU_STATUS                      (W5_REG_BASE + 0x0078)
#define W5_VPU_RET_PRODUCT_VERSION              (W5_REG_BASE + 0x0094)
/*
    assign vpu_config0          = {conf_map_converter_reg,      // [31]
    conf_map_converter_sig,         // [30]
    8'd0,                        // [29:22]
    conf_std_switch_en,          // [21]
    conf_bg_detect,              // [20]
    conf_3dnr_en,                // [19]
    conf_one_axi_en,             // [18]
    conf_sec_axi_en,             // [17]
    conf_bus_info,               // [16]
    conf_afbc_en,                // [15]
    conf_afbc_version_id,        // [14:12]
    conf_fbc_en,                 // [11]
    conf_fbc_version_id,         // [10:08]
    conf_scaler_en,              // [07]
    conf_scaler_version_id,      // [06:04]
    conf_bwb_en,                 // [03]
    3'd0};                       // [02:00]
*/
#define W5_VPU_RET_VPU_CONFIG0                  (W5_REG_BASE + 0x0098)
/*
    assign vpu_config1          = {4'd0,                        // [31:28]
    conf_perf_timer_en,          // [27]
    conf_multi_core_en,          // [26]
    conf_gcu_en,                 // [25]
    conf_cu_report,              // [24]
    4'd0,                        // [23:20]
    conf_vcore_id_3,             // [19]
    conf_vcore_id_2,             // [18]
    conf_vcore_id_1,             // [17]
    conf_vcore_id_0,             // [16]
    conf_bwb_opt,                // [15]
    7'd0,                        // [14:08]
    conf_cod_std_en_reserved_7,  // [7]
    conf_cod_std_en_reserved_6,  // [6]
    conf_cod_std_en_reserved_5,  // [5]
    conf_cod_std_en_reserved_4,  // [4]
    conf_cod_std_en_reserved_3,  // [3]
    conf_cod_std_en_reserved_2,  // [2]
    conf_cod_std_en_vp9,         // [1]
    conf_cod_std_en_hevc};       // [0]
    }
*/
#define W5_VPU_RET_VPU_CONFIG1                  (W5_REG_BASE + 0x009C)

#define W5_VPU_DBG_REG0							(W5_REG_BASE + 0x00f0)
#define W5_VPU_DBG_REG1							(W5_REG_BASE + 0x00f4)
#define W5_VPU_DBG_REG2							(W5_REG_BASE + 0x00f8)
#define W5_VPU_DBG_REG3							(W5_REG_BASE + 0x00fc)

#ifdef SUPPORT_SW_UART_V2
#define W5_SW_UART_STATUS						W5_VPU_DBG_REG0
#define W5_SW_UART_TX_DATA						W5_VPU_DBG_REG1
#endif

/************************************************************************/
/* PRODUCT INFORMATION                                                  */
/************************************************************************/
#define W5_PRODUCT_NAME                     (W5_REG_BASE + 0x1040)
#define W5_PRODUCT_NUMBER                   (W5_REG_BASE + 0x1044)

/************************************************************************/
/* DECODER/ENCODER COMMON                                               */
/************************************************************************/
#define W5_COMMAND                              (W5_REG_BASE + 0x0100)
#define W5_COMMAND_OPTION                       (W5_REG_BASE + 0x0104)
#define W5_QUERY_OPTION                         (W5_REG_BASE + 0x0104)
#define W5_RET_SUCCESS                          (W5_REG_BASE + 0x0108)
#define W5_RET_FAIL_REASON                      (W5_REG_BASE + 0x010C)
#define W5_RET_QUEUE_FAIL_REASON                (W5_REG_BASE + 0x0110)
#define W5_CMD_INSTANCE_INFO                    (W5_REG_BASE + 0x0110)

#define W5_RET_QUEUE_STATUS                     (W5_REG_BASE + 0x01E0)
#define W5_RET_BS_EMPTY_INST                    (W5_REG_BASE + 0x01E4)
#define W5_RET_QUEUE_CMD_DONE_INST              (W5_REG_BASE + 0x01E8)
#define W5_RET_STAGE0_INSTANCE_INFO             (W5_REG_BASE + 0x01EC)
#define W5_RET_STAGE1_INSTANCE_INFO             (W5_REG_BASE + 0x01F0)
#define W5_RET_STAGE2_INSTANCE_INFO             (W5_REG_BASE + 0x01F4)

#define W5_RET_SEQ_DONE_INSTANCE_INFO           (W5_REG_BASE + 0x01FC)

#define W5_BS_OPTION                            (W5_REG_BASE + 0x0120)

#define W5_RET_VLC_BUF_SIZE                     (W5_REG_BASE + 0x01B0)      // return info when QUERY (GET_RESULT) for en/decoder
#define W5_RET_PARAM_BUF_SIZE                   (W5_REG_BASE + 0x01B4)      // return info when QUERY (GET_RESULT) for en/decoder

#define W5_CMD_SET_FB_ADDR_TASK_BUF             (W5_REG_BASE + 0x01D4)      // set when SET_FB for en/decoder
#define W5_CMD_SET_FB_TASK_BUF_SIZE             (W5_REG_BASE + 0x01D8)
/************************************************************************/
/* INIT_VPU - COMMON                                                    */
/************************************************************************/
/* Note: W5_ADDR_CODE_BASE should be aligned to 4KB */
#define W5_ADDR_CODE_BASE                       (W5_REG_BASE + 0x0110)
#define W5_CODE_SIZE                            (W5_REG_BASE + 0x0114)
#define W5_CODE_PARAM                           (W5_REG_BASE + 0x0118)
#define W5_ADDR_TEMP_BASE                       (W5_REG_BASE + 0x011C)
#define W5_TEMP_SIZE                            (W5_REG_BASE + 0x0120)
#define W5_ADDR_SEC_AXI                         (W5_REG_BASE + 0x0124)
#define W5_SEC_AXI_SIZE                         (W5_REG_BASE + 0x0128)
#define W5_HW_OPTION                            (W5_REG_BASE + 0x012C)

/************************************************************************/
/* CREATE_INSTANCE - COMMON                                             */
/************************************************************************/
#define W5_ADDR_WORK_BASE                       (W5_REG_BASE + 0x0114)
#define W5_WORK_SIZE                            (W5_REG_BASE + 0x0118)
#define W5_CMD_DEC_BS_START_ADDR                (W5_REG_BASE + 0x011C)
#define W5_CMD_DEC_BS_SIZE                      (W5_REG_BASE + 0x0120)
#define W5_CMD_BS_PARAM                         (W5_REG_BASE + 0x0124)
#define W5_CMD_NUM_CQ_DEPTH_M1                  (W5_REG_BASE + 0x013C)
#define W5_CMD_ERR_CONCEAL                      (W5_REG_BASE + 0x0140)

/************************************************************************/
/* DECODER - INIT_SEQ                                                   */
/************************************************************************/
#define W5_BS_RD_PTR                            (W5_REG_BASE + 0x0118)
#define W5_BS_WR_PTR                            (W5_REG_BASE + 0x011C)
/************************************************************************/
/* SET_FRAME_BUF                                                        */
/************************************************************************/
/* SET_FB_OPTION 0x00       REGISTER FRAMEBUFFERS
                 0x01       UPDATE FRAMEBUFFER, just one framebuffer(linear, fbc and mvcol)
 */
#define W5_SFB_OPTION                           (W5_REG_BASE + 0x0104)
#define W5_COMMON_PIC_INFO                      (W5_REG_BASE + 0x0118)
#define W5_PIC_SIZE                             (W5_REG_BASE + 0x011C)
#define W5_SET_FB_NUM                           (W5_REG_BASE + 0x0120)
#define W5_EXTRA_PIC_INFO                       (W5_REG_BASE + 0x0124)

#define W5_ADDR_LUMA_BASE0                      (W5_REG_BASE + 0x0134)
#define W5_ADDR_CB_BASE0                        (W5_REG_BASE + 0x0138)
#define W5_ADDR_CR_BASE0                        (W5_REG_BASE + 0x013C)
#define W5_ADDR_FBC_Y_OFFSET0                   (W5_REG_BASE + 0x013C)       // Compression offset table for Luma
#define W5_ADDR_FBC_C_OFFSET0                   (W5_REG_BASE + 0x0140)       // Compression offset table for Chroma
#define W5_ADDR_LUMA_BASE1                      (W5_REG_BASE + 0x0144)
#define W5_ADDR_CB_ADDR1                        (W5_REG_BASE + 0x0148)
#define W5_ADDR_CR_ADDR1                        (W5_REG_BASE + 0x014C)
#define W5_ADDR_FBC_Y_OFFSET1                   (W5_REG_BASE + 0x014C)       // Compression offset table for Luma
#define W5_ADDR_FBC_C_OFFSET1                   (W5_REG_BASE + 0x0150)       // Compression offset table for Chroma
#define W5_ADDR_LUMA_BASE2                      (W5_REG_BASE + 0x0154)
#define W5_ADDR_CB_ADDR2                        (W5_REG_BASE + 0x0158)
#define W5_ADDR_CR_ADDR2                        (W5_REG_BASE + 0x015C)
#define W5_ADDR_FBC_Y_OFFSET2                   (W5_REG_BASE + 0x015C)       // Compression offset table for Luma
#define W5_ADDR_FBC_C_OFFSET2                   (W5_REG_BASE + 0x0160)       // Compression offset table for Chroma
#define W5_ADDR_LUMA_BASE3                      (W5_REG_BASE + 0x0164)
#define W5_ADDR_CB_ADDR3                        (W5_REG_BASE + 0x0168)
#define W5_ADDR_CR_ADDR3                        (W5_REG_BASE + 0x016C)
#define W5_ADDR_FBC_Y_OFFSET3                   (W5_REG_BASE + 0x016C)       // Compression offset table for Luma
#define W5_ADDR_FBC_C_OFFSET3                   (W5_REG_BASE + 0x0170)       // Compression offset table for Chroma
#define W5_ADDR_LUMA_BASE4                      (W5_REG_BASE + 0x0174)
#define W5_ADDR_CB_ADDR4                        (W5_REG_BASE + 0x0178)
#define W5_ADDR_CR_ADDR4                        (W5_REG_BASE + 0x017C)
#define W5_ADDR_FBC_Y_OFFSET4                   (W5_REG_BASE + 0x017C)       // Compression offset table for Luma
#define W5_ADDR_FBC_C_OFFSET4                   (W5_REG_BASE + 0x0180)       // Compression offset table for Chroma
#define W5_ADDR_LUMA_BASE5                      (W5_REG_BASE + 0x0184)
#define W5_ADDR_CB_ADDR5                        (W5_REG_BASE + 0x0188)
#define W5_ADDR_CR_ADDR5                        (W5_REG_BASE + 0x018C)
#define W5_ADDR_FBC_Y_OFFSET5                   (W5_REG_BASE + 0x018C)       // Compression offset table for Luma
#define W5_ADDR_FBC_C_OFFSET5                   (W5_REG_BASE + 0x0190)       // Compression offset table for Chroma
#define W5_ADDR_LUMA_BASE6                      (W5_REG_BASE + 0x0194)
#define W5_ADDR_CB_ADDR6                        (W5_REG_BASE + 0x0198)
#define W5_ADDR_CR_ADDR6                        (W5_REG_BASE + 0x019C)
#define W5_ADDR_FBC_Y_OFFSET6                   (W5_REG_BASE + 0x019C)       // Compression offset table for Luma
#define W5_ADDR_FBC_C_OFFSET6                   (W5_REG_BASE + 0x01A0)       // Compression offset table for Chroma
#define W5_ADDR_LUMA_BASE7                      (W5_REG_BASE + 0x01A4)
#define W5_ADDR_CB_ADDR7                        (W5_REG_BASE + 0x01A8)
#define W5_ADDR_CR_ADDR7                        (W5_REG_BASE + 0x01AC)
#define W5_ADDR_FBC_Y_OFFSET7                   (W5_REG_BASE + 0x01AC)       // Compression offset table for Luma
#define W5_ADDR_FBC_C_OFFSET7                   (W5_REG_BASE + 0x01B0)       // Compression offset table for Chroma
#define W5_ADDR_MV_COL0                         (W5_REG_BASE + 0x01B4)
#define W5_ADDR_MV_COL1                         (W5_REG_BASE + 0x01B8)
#define W5_ADDR_MV_COL2                         (W5_REG_BASE + 0x01BC)
#define W5_ADDR_MV_COL3                         (W5_REG_BASE + 0x01C0)
#define W5_ADDR_MV_COL4                         (W5_REG_BASE + 0x01C4)
#define W5_ADDR_MV_COL5                         (W5_REG_BASE + 0x01C8)
#define W5_ADDR_MV_COL6                         (W5_REG_BASE + 0x01CC)
#define W5_ADDR_MV_COL7                         (W5_REG_BASE + 0x01D0)

/* UPDATE_FB */
/* CMD_SET_FB_STRIDE [15:0]     - FBC framebuffer stride
                     [31:15]    - Linear framebuffer stride
 */
#define W5_CMD_SET_FB_STRIDE                    (W5_REG_BASE + 0x0118)
#define W5_CMD_SET_FB_INDEX                     (W5_REG_BASE + 0x0120)
#define W5_ADDR_LUMA_BASE                       (W5_REG_BASE + 0x0134)
#define W5_ADDR_CB_BASE                         (W5_REG_BASE + 0x0138)
#define W5_ADDR_CR_BASE                         (W5_REG_BASE + 0x013C)
#define W5_ADDR_MV_COL                          (W5_REG_BASE + 0x0140)
#define W5_ADDR_FBC_Y_BASE                      (W5_REG_BASE + 0x0144)
#define W5_ADDR_FBC_C_BASE                      (W5_REG_BASE + 0x0148)
#define W5_ADDR_FBC_Y_OFFSET                    (W5_REG_BASE + 0x014C)
#define W5_ADDR_FBC_C_OFFSET                    (W5_REG_BASE + 0x0150)

/************************************************************************/
/* DECODER - DEC_PIC                                                    */
/************************************************************************/
#define W5_CMD_DEC_VCORE_INFO                   (W5_REG_BASE + 0x0194)
/* Sequence change enable mask register
 * CMD_SEQ_CHANGE_ENABLE_FLAG [5]   profile_idc
 *                            [16]  pic_width/height_in_luma_sample
 *                            [19]  sps_max_dec_pic_buffering, max_num_reorder, max_latency_increase
 */
#define W5_CMD_SEQ_CHANGE_ENABLE_FLAG           (W5_REG_BASE + 0x0128)
#define W5_CMD_DEC_USER_MASK                    (W5_REG_BASE + 0x012C)
#define W5_CMD_DEC_TEMPORAL_ID_PLUS1            (W5_REG_BASE + 0x0130)
#define W5_CMD_DEC_REL_TEMPORAL_ID              (W5_REG_BASE + 0x0130)
#define W5_CMD_DEC_FORCE_FB_LATENCY_PLUS1       (W5_REG_BASE + 0x0134)
#define W5_USE_SEC_AXI                          (W5_REG_BASE + 0x0150)

/************************************************************************/
/* DECODER - QUERY : GET_VPU_INFO                                       */
/************************************************************************/
#define W5_RET_FW_VERSION                       (W5_REG_BASE + 0x0118)
#define W5_RET_PRODUCT_NAME                     (W5_REG_BASE + 0x011C)
#define W5_RET_PRODUCT_VERSION                  (W5_REG_BASE + 0x0120)
#define W5_RET_STD_DEF0                         (W5_REG_BASE + 0x0124)
#define W5_RET_STD_DEF1                         (W5_REG_BASE + 0x0128)
#define W5_RET_CONF_FEATURE                     (W5_REG_BASE + 0x012C)
#define W5_RET_CONF_DATE                        (W5_REG_BASE + 0x0130)
#define W5_RET_CONF_REVISION                    (W5_REG_BASE + 0x0134)
#define W5_RET_CONF_TYPE                        (W5_REG_BASE + 0x0138)
#define W5_RET_PRODUCT_ID                       (W5_REG_BASE + 0x013C)
#define W5_RET_CUSTOMER_ID                      (W5_REG_BASE + 0x0140)


/************************************************************************/
/* DECODER - QUERY : GET_RESULT                                         */
/************************************************************************/
#define W5_CMD_DEC_ADDR_REPORT_BASE         (W5_REG_BASE + 0x0114)
#define W5_CMD_DEC_REPORT_SIZE              (W5_REG_BASE + 0x0118)
#define W5_CMD_DEC_REPORT_PARAM             (W5_REG_BASE + 0x011C)

#define W5_RET_DEC_BS_RD_PTR                (W5_REG_BASE + 0x011C)
#define W5_RET_DEC_SEQ_PARAM                (W5_REG_BASE + 0x0120)
#define W5_RET_DEC_COLOR_SAMPLE_INFO        (W5_REG_BASE + 0x0124)
#define W5_RET_DEC_ASPECT_RATIO             (W5_REG_BASE + 0x0128)
#define W5_RET_DEC_BIT_RATE                 (W5_REG_BASE + 0x012C)
#define W5_RET_DEC_FRAME_RATE_NR            (W5_REG_BASE + 0x0130)
#define W5_RET_DEC_FRAME_RATE_DR            (W5_REG_BASE + 0x0134)
#define W5_RET_DEC_NUM_REQUIRED_FB          (W5_REG_BASE + 0x0138)
#define W5_RET_DEC_NUM_REORDER_DELAY        (W5_REG_BASE + 0x013C)
#define W5_RET_DEC_SUB_LAYER_INFO           (W5_REG_BASE + 0x0140)
#define W5_RET_DEC_NOTIFICATION             (W5_REG_BASE + 0x0144)
#define W5_RET_DEC_USERDATA_IDC             (W5_REG_BASE + 0x0148)
#define W5_RET_DEC_PIC_SIZE                 (W5_REG_BASE + 0x014C)
#define W5_RET_DEC_CROP_TOP_BOTTOM          (W5_REG_BASE + 0x0150)
#define W5_RET_DEC_CROP_LEFT_RIGHT          (W5_REG_BASE + 0x0154)
#define W5_RET_DEC_AU_START_POS             (W5_REG_BASE + 0x0158)
#define W5_RET_DEC_AU_END_POS               (W5_REG_BASE + 0x015C)
#define W5_RET_DEC_PIC_TYPE                 (W5_REG_BASE + 0x0160)
#define W5_RET_DEC_PIC_POC                  (W5_REG_BASE + 0x0164)
#define W5_RET_DEC_RECOVERY_POINT           (W5_REG_BASE + 0x0168)
#define W5_RET_DEC_DEBUG_INDEX              (W5_REG_BASE + 0x016C)
#define W5_RET_DEC_DECODED_INDEX            (W5_REG_BASE + 0x0170)
#define W5_RET_DEC_DISPLAY_INDEX            (W5_REG_BASE + 0x0174)
#define W5_RET_DEC_REALLOC_INDEX            (W5_REG_BASE + 0x0178)
#define W5_RET_DEC_DISP_IDC                 (W5_REG_BASE + 0x017C)
#define W5_RET_DEC_ERR_CTB_NUM              (W5_REG_BASE + 0x0180)
#define W5_RET_DEC_PIC_PARAM                (W5_REG_BASE + 0x01A0)

#define W5_RET_DEC_HOST_CMD_TICK            (W5_REG_BASE + 0x01B8)
#define W5_RET_DEC_SEEK_START_TICK          (W5_REG_BASE + 0x01BC)
#define W5_RET_DEC_SEEK_END_TICK            (W5_REG_BASE + 0x01C0)
#define W5_RET_DEC_PARSING_START_TICK       (W5_REG_BASE + 0x01C4)
#define W5_RET_DEC_PARSING_END_TICK         (W5_REG_BASE + 0x01C8)
#define W5_RET_DEC_DECODING_START_TICK      (W5_REG_BASE + 0x01CC)
#define W5_RET_DEC_DECODING_ENC_TICK        (W5_REG_BASE + 0x01D0)
#ifdef SUPPORT_SW_UART
#define W5_SW_UART_STATUS					(W5_REG_BASE + 0x01D4)
#define W5_SW_UART_TX_DATA					(W5_REG_BASE + 0x01D8)
//#define W5_RET_DEC_WARN_INFO                (W5_REG_BASE + 0x01D4)
//#define W5_RET_DEC_ERR_INFO                 (W5_REG_BASE + 0x01D8)
#else
#define W5_RET_DEC_WARN_INFO                (W5_REG_BASE + 0x01D4)
#define W5_RET_DEC_ERR_INFO                 (W5_REG_BASE + 0x01D8)
#endif
#define W5_RET_DEC_DECODING_SUCCESS         (W5_REG_BASE + 0x01DC)

/************************************************************************/
/* DECODER - FLUSH_INSTANCE                                             */
/************************************************************************/
#define W5_CMD_FLUSH_INST_OPT               (W5_REG_BASE + 0x104)

/************************************************************************/
/* DECODER - QUERY : UPDATE_DISP_FLAG                                   */
/************************************************************************/
#define W5_CMD_DEC_SET_DISP_IDC             (W5_REG_BASE + 0x0118)
#define W5_CMD_DEC_CLR_DISP_IDC             (W5_REG_BASE + 0x011C)

/************************************************************************/
/* DECODER - QUERY : SET_BS_RD_PTR                                      */
/************************************************************************/
#define W5_RET_QUERY_DEC_SET_BS_RD_PTR      (W5_REG_BASE + 0x011C)

/************************************************************************/
/* DECODER - QUERY : GET_BS_RD_PTR                                      */
/************************************************************************/
#define W5_RET_QUERY_DEC_BS_RD_PTR          (W5_REG_BASE + 0x011C)

/************************************************************************/
/* QUERY : GET_DEBUG_INFO                                               */
/************************************************************************/
#define W5_RET_QUERY_DEBUG_PRI_REASON       (W5_REG_BASE + 0x114)

/************************************************************************/
/* GDI register for Debugging                                           */
/************************************************************************/
#define W5_GDI_BASE                         0x8800
#define W5_GDI_BUS_CTRL                     (W5_GDI_BASE + 0x0F0)
#define W5_GDI_BUS_STATUS                   (W5_GDI_BASE + 0x0F4)

#define W5_BACKBONE_BASE_VCPU               0xFE00
#define W5_BACKBONE_BUS_CTRL_VCPU           (W5_BACKBONE_BASE_VCPU + 0x010)
#define W5_BACKBONE_BUS_STATUS_VCPU         (W5_BACKBONE_BASE_VCPU + 0x014)
#define W5_BACKBONE_PROG_AXI_ID             (W5_BACKBONE_BASE_VCPU + 0x00C)

#define W5_BACKBONE_BASE_VCORE0             0x8E00
#define W5_BACKBONE_BUS_CTRL_VCORE0         (W5_BACKBONE_BASE_VCORE0 + 0x010)
#define W5_BACKBONE_BUS_STATUS_VCORE0       (W5_BACKBONE_BASE_VCORE0 + 0x014)

#define W5_BACKBONE_BASE_VCORE1             0x9E00  // for dual-core product
#define W5_BACKBONE_BUS_CTRL_VCORE1         (W5_BACKBONE_BASE_VCORE1 + 0x010)
#define W5_BACKBONE_BUS_STATUS_VCORE1       (W5_BACKBONE_BASE_VCORE1 + 0x014)

#define W5_COMBINED_BACKBONE_BASE           0xFE00
#define W5_COMBINED_BACKBONE_BUS_CTRL       (W5_COMBINED_BACKBONE_BASE + 0x010)
#define W5_COMBINED_BACKBONE_BUS_STATUS     (W5_COMBINED_BACKBONE_BASE + 0x014)

/************************************************************************/
/*                                                                      */
/*               For  ENCODER                                           */
/*                                                                      */
/************************************************************************/
#define W5_RET_STAGE3_INSTANCE_INFO             (W5_REG_BASE + 0x1F8)
/************************************************************************/
/* ENCODER - CREATE_INSTANCE                                            */
/************************************************************************/
// 0x114 ~ 0x124 : defined above (CREATE_INSTANCE COMMON)
#define W5_CMD_ENC_VCORE_INFO                   (W5_REG_BASE + 0x0194)
#define W5_CMD_ENC_SRC_OPTIONS                  (W5_REG_BASE + 0x0128)

/************************************************************************/
/* ENCODER - SET_FB                                                     */
/************************************************************************/
#define W5_FBC_STRIDE                           (W5_REG_BASE + 0x128)
#define W5_ADDR_SUB_SAMPLED_FB_BASE             (W5_REG_BASE + 0x12C)
#define W5_SUB_SAMPLED_ONE_FB_SIZE              (W5_REG_BASE + 0x130)

/************************************************************************/
/* ENCODER - ENC_SET_PARAM (COMMON & CHANGE_PARAM)                      */
/************************************************************************/
#define W5_CMD_ENC_SEQ_SET_PARAM_OPTION         (W5_REG_BASE + 0x104)
#define W5_CMD_ENC_SEQ_SET_PARAM_ENABLE         (W5_REG_BASE + 0x118)
#define W5_CMD_ENC_SEQ_SRC_SIZE                 (W5_REG_BASE + 0x11C)
#define W5_CMD_ENC_SEQ_CUSTOM_MAP_ENDIAN        (W5_REG_BASE + 0x120)
#define W5_CMD_ENC_SEQ_SPS_PARAM                (W5_REG_BASE + 0x124)
#define W5_CMD_ENC_SEQ_PPS_PARAM                (W5_REG_BASE + 0x128)
#define W5_CMD_ENC_SEQ_GOP_PARAM                (W5_REG_BASE + 0x12C)
#define W5_CMD_ENC_SEQ_INTRA_PARAM              (W5_REG_BASE + 0x130)
#define W5_CMD_ENC_SEQ_CONF_WIN_TOP_BOT         (W5_REG_BASE + 0x134)
#define W5_CMD_ENC_SEQ_CONF_WIN_LEFT_RIGHT      (W5_REG_BASE + 0x138)
#define W5_CMD_ENC_SEQ_RDO_PARAM                (W5_REG_BASE + 0x13C)
#define W5_CMD_ENC_SEQ_INDEPENDENT_SLICE        (W5_REG_BASE + 0x140)
#define W5_CMD_ENC_SEQ_DEPENDENT_SLICE          (W5_REG_BASE + 0x144)
#define W5_CMD_ENC_SEQ_INTRA_REFRESH            (W5_REG_BASE + 0x148)
#define W5_CMD_ENC_SEQ_INPUT_SRC_PARAM          (W5_REG_BASE + 0x14C)

#define W5_CMD_ENC_SEQ_RC_FRAME_RATE            (W5_REG_BASE + 0x150)
#define W5_CMD_ENC_SEQ_RC_TARGET_RATE           (W5_REG_BASE + 0x154)
#define W5_CMD_ENC_SEQ_RC_PARAM                 (W5_REG_BASE + 0x158)
#define W5_CMD_ENC_SEQ_RC_MIN_MAX_QP            (W5_REG_BASE + 0x15C)
#define W5_CMD_ENC_SEQ_RC_BIT_RATIO_LAYER_0_3   (W5_REG_BASE + 0x160)
#define W5_CMD_ENC_SEQ_RC_BIT_RATIO_LAYER_4_7   (W5_REG_BASE + 0x164)
#define W5_CMD_ENC_SEQ_RC_INTER_MIN_MAX_QP      (W5_REG_BASE + 0x168)
#define W5_CMD_ENC_SEQ_RC_WEIGHT_PARAM          (W5_REG_BASE + 0x16C)

#define W5_CMD_ENC_SEQ_ROT_PARAM                (W5_REG_BASE + 0x170)
#define W5_CMD_ENC_SEQ_NUM_UNITS_IN_TICK        (W5_REG_BASE + 0x174)
#define W5_CMD_ENC_SEQ_TIME_SCALE               (W5_REG_BASE + 0x178)
#define W5_CMD_ENC_SEQ_NUM_TICKS_POC_DIFF_ONE   (W5_REG_BASE + 0x17C)

#define W5_CMD_ENC_SEQ_CUSTOM_MD_PU04           (W5_REG_BASE + 0x184)
#define W5_CMD_ENC_SEQ_CUSTOM_MD_PU08           (W5_REG_BASE + 0x188)
#define W5_CMD_ENC_SEQ_CUSTOM_MD_PU16           (W5_REG_BASE + 0x18C)
#define W5_CMD_ENC_SEQ_CUSTOM_MD_PU32           (W5_REG_BASE + 0x190)
#define W5_CMD_ENC_SEQ_CUSTOM_MD_CU08           (W5_REG_BASE + 0x194)
#define W5_CMD_ENC_SEQ_CUSTOM_MD_CU16           (W5_REG_BASE + 0x198)
#define W5_CMD_ENC_SEQ_CUSTOM_MD_CU32           (W5_REG_BASE + 0x19C)
#define W5_CMD_ENC_SEQ_NR_PARAM                 (W5_REG_BASE + 0x1A0)
#define W5_CMD_ENC_SEQ_NR_WEIGHT                (W5_REG_BASE + 0x1A4)
#define W5_CMD_ENC_SEQ_BG_PARAM                 (W5_REG_BASE + 0x1A8)
#define W5_CMD_ENC_SEQ_CUSTOM_LAMBDA_ADDR       (W5_REG_BASE + 0x1AC)
#define W5_CMD_ENC_SEQ_USER_SCALING_LIST_ADDR   (W5_REG_BASE + 0x1B0)
#define W5_CMD_ENC_SEQ_VUI_HRD_PARAM            (W5_REG_BASE + 0x180)
#define W5_CMD_ENC_SEQ_VUI_RBSP_ADDR            (W5_REG_BASE + 0x1B8)
#define W5_CMD_ENC_SEQ_HRD_RBSP_ADDR            (W5_REG_BASE + 0x1BC)

/************************************************************************/
/* ENCODER - ENC_SET_PARAM (CUSTOM_GOP)                                 */
/************************************************************************/
#define W5_CMD_ENC_CUSTOM_GOP_PARAM             (W5_REG_BASE + 0x11C)
#define W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_0       (W5_REG_BASE + 0x120)
#define W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_1       (W5_REG_BASE + 0x124)
#define W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_2       (W5_REG_BASE + 0x128)
#define W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_3       (W5_REG_BASE + 0x12C)
#define W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_4       (W5_REG_BASE + 0x130)
#define W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_5       (W5_REG_BASE + 0x134)
#define W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_6       (W5_REG_BASE + 0x138)
#define W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_7       (W5_REG_BASE + 0x13C)
#define W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_8       (W5_REG_BASE + 0x140)
#define W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_9       (W5_REG_BASE + 0x144)
#define W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_10      (W5_REG_BASE + 0x148)
#define W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_11      (W5_REG_BASE + 0x14C)
#define W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_12      (W5_REG_BASE + 0x150)
#define W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_13      (W5_REG_BASE + 0x154)
#define W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_14      (W5_REG_BASE + 0x158)
#define W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_15      (W5_REG_BASE + 0x15C)

/************************************************************************/
/* ENCODER - ENC_PIC                                                    */
/************************************************************************/
#define W5_CMD_ENC_BS_START_ADDR                (W5_REG_BASE + 0x118)
#define W5_CMD_ENC_BS_SIZE                      (W5_REG_BASE + 0x11C)
#define W5_CMD_ENC_PIC_USE_SEC_AXI              (W5_REG_BASE + 0x124)
#define W5_CMD_ENC_PIC_REPORT_PARAM             (W5_REG_BASE + 0x128)
#define W5_CMD_ENC_PIC_REPORT_ENDIAN            (W5_REG_BASE + 0x12C)

#define W5_CMD_ENC_PIC_CUSTOM_MAP_OPTION_PARAM  (W5_REG_BASE + 0x138)
#define W5_CMD_ENC_PIC_CUSTOM_MAP_OPTION_ADDR   (W5_REG_BASE + 0x13C)
#define W5_CMD_ENC_PIC_SRC_PIC_IDX              (W5_REG_BASE + 0x144)
#define W5_CMD_ENC_PIC_SRC_ADDR_Y               (W5_REG_BASE + 0x148)
#define W5_CMD_ENC_PIC_SRC_ADDR_U               (W5_REG_BASE + 0x14C)
#define W5_CMD_ENC_PIC_SRC_ADDR_V               (W5_REG_BASE + 0x150)
#define W5_CMD_ENC_PIC_SRC_STRIDE               (W5_REG_BASE + 0x154)
#define W5_CMD_ENC_PIC_SRC_FORMAT               (W5_REG_BASE + 0x158)
#define W5_CMD_ENC_PIC_SRC_AXI_SEL              (W5_REG_BASE + 0x160)
#define W5_CMD_ENC_PIC_CODE_OPTION              (W5_REG_BASE + 0x164)
#define W5_CMD_ENC_PIC_PIC_PARAM                (W5_REG_BASE + 0x168)
#define W5_CMD_ENC_PIC_LONGTERM_PIC             (W5_REG_BASE + 0x16C)
#define W5_CMD_ENC_PIC_WP_PIXEL_SIGMA_Y         (W5_REG_BASE + 0x170)
#define W5_CMD_ENC_PIC_WP_PIXEL_SIGMA_C         (W5_REG_BASE + 0x174)
#define W5_CMD_ENC_PIC_WP_PIXEL_MEAN_Y          (W5_REG_BASE + 0x178)
#define W5_CMD_ENC_PIC_WP_PIXEL_MEAN_C          (W5_REG_BASE + 0x17C)
#define W5_CMD_ENC_PIC_LF_PARAM_0               (W5_REG_BASE + 0x180)
#define W5_CMD_ENC_PIC_LF_PARAM_1               (W5_REG_BASE + 0x184)
#define W5_CMD_ENC_PIC_CF50_Y_OFFSET_TABLE_ADDR  (W5_REG_BASE + 0x190)
#define W5_CMD_ENC_PIC_CF50_CB_OFFSET_TABLE_ADDR (W5_REG_BASE + 0x194)
#define W5_CMD_ENC_PIC_CF50_CR_OFFSET_TABLE_ADDR (W5_REG_BASE + 0x198)
#define W5_CMD_ENC_PIC_PREFIX_SEI_NAL_ADDR       (W5_REG_BASE + 0x180)
#define W5_CMD_ENC_PIC_PREFIX_SEI_INFO           (W5_REG_BASE + 0x184)
#define W5_CMD_ENC_PIC_SUFFIX_SEI_NAL_ADDR       (W5_REG_BASE + 0x188)
#define W5_CMD_ENC_PIC_SUFFIX_SEI_INFO           (W5_REG_BASE + 0x18c)

/************************************************************************/
/* ENCODER - QUERY (GET_RESULT)                                         */
/************************************************************************/
#define W5_RET_ENC_NUM_REQUIRED_FB              (W5_REG_BASE + 0x11C)
#define W5_RET_ENC_MIN_SRC_BUF_NUM              (W5_REG_BASE + 0x120)
#define W5_RET_ENC_PIC_TYPE                     (W5_REG_BASE + 0x124)
#define W5_RET_ENC_PIC_POC                      (W5_REG_BASE + 0x128)
#define W5_RET_ENC_PIC_IDX                      (W5_REG_BASE + 0x12C)
#define W5_RET_ENC_PIC_SLICE_NUM                (W5_REG_BASE + 0x130)
#define W5_RET_ENC_PIC_SKIP                     (W5_REG_BASE + 0x134)
#define W5_RET_ENC_PIC_NUM_INTRA                (W5_REG_BASE + 0x138)
#define W5_RET_ENC_PIC_NUM_MERGE                (W5_REG_BASE + 0x13C)

#define W5_RET_ENC_PIC_NUM_SKIP                 (W5_REG_BASE + 0x144)
#define W5_RET_ENC_PIC_AVG_CTU_QP               (W5_REG_BASE + 0x148)
#define W5_RET_ENC_PIC_BYTE                     (W5_REG_BASE + 0x14C)
#define W5_RET_ENC_GOP_PIC_IDX                  (W5_REG_BASE + 0x150)
#define W5_RET_ENC_USED_SRC_IDX                 (W5_REG_BASE + 0x154)
#define W5_RET_ENC_PIC_NUM                      (W5_REG_BASE + 0x158)
#define W5_RET_ENC_VCL_NUT                      (W5_REG_BASE + 0x15C)

#define W5_RET_ENC_PIC_DIST_LOW                 (W5_REG_BASE + 0x164)
#define W5_RET_ENC_PIC_DIST_HIGH                (W5_REG_BASE + 0x168)

#define W5_RET_ENC_PIC_MAX_LATENCY_PICTURES     (W5_REG_BASE + 0x16C)
#define W5_RET_ENC_SVC_LAYER                    (W5_REG_BASE + 0x170)


#define W5_RET_ENC_HOST_CMD_TICK                (W5_REG_BASE + 0x1B8)
#define W5_RET_ENC_PREPARE_START_TICK           (W5_REG_BASE + 0x1BC)
#define W5_RET_ENC_PREPARE_END_TICK             (W5_REG_BASE + 0x1C0)
#define W5_RET_ENC_PROCESSING_START_TICK        (W5_REG_BASE + 0x1C4)
#define W5_RET_ENC_PROCESSING_END_TICK          (W5_REG_BASE + 0x1C8)
#define W5_RET_ENC_ENCODING_START_TICK          (W5_REG_BASE + 0x1CC)
#define W5_RET_ENC_ENCODING_END_TICK            (W5_REG_BASE + 0x1D0)


#define W5_RET_ENC_WARN_INFO                    (W5_REG_BASE + 0x1D4)
#define W5_RET_ENC_ERR_INFO                     (W5_REG_BASE + 0x1D8)
#define W5_RET_ENC_ENCODING_SUCCESS             (W5_REG_BASE + 0x1DC)


/************************************************************************/
/* ENCODER - QUERY (GET_BW_REPORT)                                      */
/************************************************************************/
#define W5_RET_ENC_RD_PTR                       (W5_REG_BASE + 0x114)
#define W5_RET_ENC_WR_PTR                       (W5_REG_BASE + 0x118)
#define W5_CMD_ENC_REASON_SEL                   (W5_REG_BASE + 0x11C)

/************************************************************************/
/* ENCODER - QUERY (GET_BW_REPORT)                                      */
/************************************************************************/
#define RET_QUERY_BW_PRP_AXI_READ               (W5_REG_BASE + 0x118)
#define RET_QUERY_BW_PRP_AXI_WRITE              (W5_REG_BASE + 0x11C)
#define RET_QUERY_BW_FBD_Y_AXI_READ             (W5_REG_BASE + 0x120)
#define RET_QUERY_BW_FBC_Y_AXI_WRITE            (W5_REG_BASE + 0x124)
#define RET_QUERY_BW_FBD_C_AXI_READ             (W5_REG_BASE + 0x128)
#define RET_QUERY_BW_FBC_C_AXI_WRITE            (W5_REG_BASE + 0x12C)
#define RET_QUERY_BW_PRI_AXI_READ               (W5_REG_BASE + 0x130)
#define RET_QUERY_BW_PRI_AXI_WRITE              (W5_REG_BASE + 0x134)
#define RET_QUERY_BW_SEC_AXI_READ               (W5_REG_BASE + 0x138)
#define RET_QUERY_BW_SEC_AXI_WRITE              (W5_REG_BASE + 0x13C)
#define RET_QUERY_BW_PROC_AXI_READ              (W5_REG_BASE + 0x140)
#define RET_QUERY_BW_PROC_AXI_WRITE             (W5_REG_BASE + 0x144)
#define RET_QUERY_BW_BWB_AXI_WRITE              (W5_REG_BASE + 0x148)
#define W5_CMD_BW_OPTION                        (W5_REG_BASE + 0x14C)

/************************************************************************/
/* ENCODER - QUERY (GET_SRC_FLAG)                                       */
/************************************************************************/
#define W5_RET_ENC_SRC_BUF_FLAG                 (W5_REG_BASE + 0x18C)
#define W5_RET_RELEASED_SRC_INSTANCE            (W5_REG_BASE + 0x1EC)


#define W5_ENC_PIC_SUB_FRAME_SYNC_IF            (W5_REG_BASE + 0x0300)

#endif /* __WAVE5_REGISTER_DEFINE_H__ */


#ifndef REGDEFINE_H_INCLUDED
#define REGDEFINE_H_INCLUDED

//------------------------------------------------------------------------------
// REGISTER BASE
//------------------------------------------------------------------------------
#define BIT_BASE                            0x0000
#define GDMA_BASE                           0x1000
#define MBC_BASE                            0x0400
#define ME_BASE                             0x0600
#define MC_BASE                             0x0C00
#define DMAC_BASE                           0x2000

#define BW_BASE                             0x03000000
//------------------------------------------------------------------------------
// HARDWARE REGISTER
//------------------------------------------------------------------------------
// SW Reset command
#define VPU_SW_RESET_BPU_CORE               0x008
#define VPU_SW_RESET_BPU_BUS                0x010
#define VPU_SW_RESET_VCE_CORE               0x020
#define VPU_SW_RESET_VCE_BUS                0x040
#define VPU_SW_RESET_GDI_CORE               0x080
#define VPU_SW_RESET_GDI_BUS                0x100

#define BIT_CODE_RUN                        (BIT_BASE + 0x000)
#define BIT_CODE_DOWN                       (BIT_BASE + 0x004)
#define BIT_INT_REQ                         (BIT_BASE + 0x008)
#define BIT_INT_CLEAR                       (BIT_BASE + 0x00C)
#define BIT_INT_STS                         (BIT_BASE + 0x010)
#define BIT_CODE_RESET                      (BIT_BASE + 0x014)
#define BIT_CUR_PC                          (BIT_BASE + 0x018)
#define BIT_SW_RESET                        (BIT_BASE + 0x024)
#define BIT_SW_RESET_STATUS                 (BIT_BASE + 0x034)

//------------------------------------------------------------------------------
// GLOBAL REGISTER
//------------------------------------------------------------------------------
#define BIT_CODE_BUF_ADDR                   (BIT_BASE + 0x100)
#define BIT_WORK_BUF_ADDR                   (BIT_BASE + 0x104)
#define BIT_PARA_BUF_ADDR                   (BIT_BASE + 0x108)
#define BIT_BIT_STREAM_CTRL                 (BIT_BASE + 0x10C)
#define BIT_FRAME_MEM_CTRL                  (BIT_BASE + 0x110)
#define BIT_BIT_STREAM_PARAM                (BIT_BASE + 0x114)
#define BIT_TEMP_BUF_ADDR                   (BIT_BASE + 0x118)

#define BIT_RD_PTR                          (BIT_BASE + 0x120)
#define BIT_WR_PTR                          (BIT_BASE + 0x124)

#define BIT_ROLLBACK_STATUS                 (BIT_BASE + 0x128)  // internal used in f/w.

#define BIT_AXI_SRAM_USE                    (BIT_BASE + 0x140)
#define BIT_BYTE_POS_FRAME_START            (BIT_BASE + 0x144)
#define BIT_BYTE_POS_FRAME_END              (BIT_BASE + 0x148)
#define BIT_FRAME_CYCLE                     (BIT_BASE + 0x14C)

#define BIT_FRM_DIS_FLG                     (BIT_BASE + 0x150)

#define BIT_BUSY_FLAG                       (BIT_BASE + 0x160)
#define BIT_RUN_COMMAND                     (BIT_BASE + 0x164)
#define BIT_RUN_INDEX                       (BIT_BASE + 0x168)
#define BIT_RUN_COD_STD                     (BIT_BASE + 0x16C)
#define BIT_INT_ENABLE                      (BIT_BASE + 0x170)
#define BIT_INT_REASON                      (BIT_BASE + 0x174)
#define BIT_RUN_AUX_STD                     (BIT_BASE + 0x178)

// MSG REGISTER ADDRESS changed
#define BIT_MSG_0                           (BIT_BASE + 0x130)
#define BIT_MSG_1                           (BIT_BASE + 0x134)
#define BIT_MSG_2                           (BIT_BASE + 0x138)
#define BIT_MSG_3                           (BIT_BASE + 0x13C)

#define MBC_BUSY                            (MBC_BASE + 0x040)
#define MC_BUSY                             (MC_BASE + 0x004)


#define P_MBC_SUB_FRAME_SYNC_IF             (MBC_BASE + 0x1f8)          // CODA980
#define P_MBC_ENC_SUB_FRAME_SYNC            (MBC_BASE + 0x1fc)          // CODA980

//------------------------------------------------------------------------------
// [ENC SEQ INIT] COMMAND
//------------------------------------------------------------------------------
#define CMD_ENC_SEQ_BB_START                (BIT_BASE + 0x180)
#define CMD_ENC_SEQ_BB_SIZE                 (BIT_BASE + 0x184)
#define CMD_ENC_SEQ_OPTION                  (BIT_BASE + 0x188)          // HecEnable,ConstIntraQp, FMO, QPREP, AUD, SLICE, MB BIT
#define CMD_ENC_SEQ_COD_STD                 (BIT_BASE + 0x18C)
#define CMD_ENC_SEQ_SRC_SIZE                (BIT_BASE + 0x190)
#define CMD_ENC_SEQ_SRC_F_RATE              (BIT_BASE + 0x194)
#define CMD_ENC_SEQ_MP4_PARA                (BIT_BASE + 0x198)
#define CMD_ENC_SEQ_263_PARA                (BIT_BASE + 0x19C)
#define CMD_ENC_SEQ_264_PARA                (BIT_BASE + 0x1A0)
#define CMD_ENC_SEQ_SLICE_MODE              (BIT_BASE + 0x1A4)
#define CMD_ENC_SEQ_GOP_NUM                 (BIT_BASE + 0x1A8)
#define CMD_ENC_SEQ_RC_PARA                 (BIT_BASE + 0x1AC)
#define CMD_ENC_SEQ_RC_BUF_SIZE             (BIT_BASE + 0x1B0)
#define CMD_ENC_SEQ_INTRA_REFRESH           (BIT_BASE + 0x1B4)
#define CMD_ENC_SEQ_INTRA_QP                (BIT_BASE + 0x1C4)
#define CMD_ENC_SEQ_RC_QP_MAX               (BIT_BASE + 0x1C8)
#define CMD_ENC_SEQ_RC_GAMMA                (BIT_BASE + 0x1CC)
#define CMD_ENC_SEQ_RC_INTERVAL_MODE        (BIT_BASE + 0x1D0)      // mbInterval[32:2], rcIntervalMode[1:0]
#define CMD_ENC_SEQ_INTRA_WEIGHT            (BIT_BASE + 0x1D4)
#define CMD_ENC_SEQ_ME_OPTION               (BIT_BASE + 0x1D8)
#define CMD_ENC_SEQ_RC_PARA2                (BIT_BASE + 0x1DC)
#define CMD_ENC_SEQ_QP_RANGE_SET            (BIT_BASE + 0x1E0)
#define CMD_ENC_SEQ_RC_MAX_INTRA_SIZE       (BIT_BASE + 0x1F0)


#define CMD_ENC_SEQ_FIRST_MBA               (BIT_BASE + 0x1E4)
#define CMD_ENC_SEQ_HEIGHT_IN_MAP_UNITS     (BIT_BASE + 0x1E8)
#define CMD_ENC_SEQ_OVERLAP_CLIP_SIZE       (BIT_BASE + 0x1EC)

//------------------------------------------------------------------------------
// [ENC SEQ END] COMMAND
//------------------------------------------------------------------------------
#define RET_ENC_SEQ_END_SUCCESS             (BIT_BASE + 0x1C0)
//------------------------------------------------------------------------------
// [ENC PIC RUN] COMMAND
//------------------------------------------------------------------------------

#define CMD_ENC_PIC_SRC_INDEX               (BIT_BASE + 0x180)
#define CMD_ENC_PIC_SRC_STRIDE              (BIT_BASE + 0x184)
#define CMD_ENC_PIC_SRC_ADDR_Y              (BIT_BASE + 0x1A8)
#define CMD_ENC_PIC_SRC_ADDR_CB             (BIT_BASE + 0x1AC)
#define CMD_ENC_PIC_SRC_ADDR_CR             (BIT_BASE + 0x1B0)
#define CMD_ENC_PIC_SRC_BOTTOM_Y            (BIT_BASE + 0x1E8)  //coda980 only
#define CMD_ENC_PIC_SRC_BOTTOM_CB           (BIT_BASE + 0x1EC)  //coda980 only
#define CMD_ENC_PIC_SRC_BOTTOM_CR           (BIT_BASE + 0x1F0)  //coda980 only

#define CMD_ENC_PIC_QS                      (BIT_BASE + 0x18C)
#define CMD_ENC_PIC_ROT_MODE                (BIT_BASE + 0x190)
#define CMD_ENC_PIC_OPTION                  (BIT_BASE + 0x194)
#define CMD_ENC_PIC_BB_START                (BIT_BASE + 0x198)
#define CMD_ENC_PIC_BB_SIZE                 (BIT_BASE + 0x19C)
#define CMD_ENC_PIC_PARA_BASE_ADDR          (BIT_BASE + 0x1A0)
#define CMD_ENC_PIC_SUB_FRAME_SYNC          (BIT_BASE + 0x1A4)


#define RET_ENC_PIC_FRAME_NUM               (BIT_BASE + 0x1C0)
#define RET_ENC_PIC_TYPE                    (BIT_BASE + 0x1C4)
#define RET_ENC_PIC_FRAME_IDX               (BIT_BASE + 0x1C8)
#define RET_ENC_PIC_SLICE_NUM               (BIT_BASE + 0x1CC)
#define RET_ENC_PIC_FLAG                    (BIT_BASE + 0x1D0)
#define RET_ENC_PIC_SUCCESS                 (BIT_BASE + 0x1D8)


//------------------------------------------------------------------------------
// [ENC ROI INIT] COMMAND
//------------------------------------------------------------------------------
#define CMD_ENC_ROI_INFO                    (BIT_BASE + 0x180)      // [7:1]: RoiPicAvgQp, [0]: RoiEn
#define CMD_ENC_ROI_QP_MAP_ADDR             (BIT_BASE + 0x184)      // [31:0] ROI Qp map address


#define RET_ENC_ROI_SUCCESS                 (BIT_BASE + 0x1D8)

//------------------------------------------------------------------------------
// [ENC SET FRAME BUF] COMMAND
//------------------------------------------------------------------------------
#define CMD_SET_FRAME_SUBSAMP_A             (BIT_BASE + 0x188)      //coda960 only
#define CMD_SET_FRAME_SUBSAMP_B             (BIT_BASE + 0x18C)      //coda960 only
#define CMD_SET_FRAME_SUBSAMP_A_MVC         (BIT_BASE + 0x1B0)      //coda960 only
#define CMD_SET_FRAME_SUBSAMP_B_MVC         (BIT_BASE + 0x1B4)      //coda960 only

#define CMD_SET_FRAME_DP_BUF_BASE           (BIT_BASE + 0x1B0)
#define CMD_SET_FRAME_DP_BUF_SIZE           (BIT_BASE + 0x1B4)

//------------------------------------------------------------------------------
// [ENC HEADER] COMMAND
//------------------------------------------------------------------------------
#define CMD_ENC_HEADER_CODE                 (BIT_BASE + 0x180)
#define CMD_ENC_HEADER_BB_START             (BIT_BASE + 0x184)
#define CMD_ENC_HEADER_BB_SIZE              (BIT_BASE + 0x188)
#define CMD_ENC_HEADER_FRAME_CROP_H         (BIT_BASE + 0x18C)
#define CMD_ENC_HEADER_FRAME_CROP_V         (BIT_BASE + 0x190)
#define CMD_ENC_HEADER_CABAC_MODE           (BIT_BASE + 0x194)      // CODA980
#define CMD_ENC_HEADER_CABAC_INIT_IDC       (BIT_BASE + 0x198)      // CODA980
#define CMD_ENC_HEADER_TRANSFORM_8X8        (BIT_BASE + 0x19C)      // CODA980
#define CMD_ENC_HEADER_CHROMA_FORMAT        (BIT_BASE + 0x1A0)      // CODA980
#define CMD_ENC_HEADER_FIELD_FLAG           (BIT_BASE + 0x1A4)
#define CMD_ENC_HEADER_PROFILE              (BIT_BASE + 0x1A8)      // CODA980

#define RET_ENC_HEADER_SUCCESS              (BIT_BASE + 0x1C0)

//------------------------------------------------------------------------------
// [ENC_PARA_SET] COMMAND
//------------------------------------------------------------------------------
#define CMD_ENC_PARA_SET_TYPE               (BIT_BASE + 0x180)
#define RET_ENC_PARA_SET_SIZE               (BIT_BASE + 0x1c0)
#define RET_ENC_PARA_SET_SUCCESS            (BIT_BASE + 0x1C4)

//------------------------------------------------------------------------------
// [ENC PARA CHANGE] COMMAND :
//------------------------------------------------------------------------------
#define CMD_ENC_PARAM_CHANGE_ENABLE         (BIT_BASE + 0x180)      // FrameRateEn[3], BitRateEn[2], IntraQpEn[1], GopEn[0]
#define CMD_ENC_PARAM_CHANGE_GOP_NUM        (BIT_BASE + 0x184)
#define CMD_ENC_PARAM_CHANGE_INTRA_QP       (BIT_BASE + 0x188)
#define CMD_ENC_PARAM_CHANGE_BITRATE        (BIT_BASE + 0x18C)
#define CMD_ENC_PARAM_CHANGE_F_RATE         (BIT_BASE + 0x190)
#define CMD_ENC_PARAM_CHANGE_INTRA_REFRESH  (BIT_BASE + 0x194)      // update param
#define CMD_ENC_PARAM_CHANGE_SLICE_MODE     (BIT_BASE + 0x198)      // update param
#define CMD_ENC_PARAM_CHANGE_HEC_MODE       (BIT_BASE + 0x19C)      // update param
#define CMD_ENC_PARAM_CHANGE_CABAC_MODE     (BIT_BASE + 0x1A0)      // entropyCodingMode==2
#define CMD_ENC_PARAM_CHANGE_PPS_ID         (BIT_BASE + 0x1B4)      // entropyCodingMode==2
#define RET_ENC_SEQ_PARA_CHANGE_SUCCESS     (BIT_BASE + 0x1C0)

//------------------------------------------------------------------------------
// [DEC SEQ INIT] COMMAND
//------------------------------------------------------------------------------
#define CMD_DEC_SEQ_BB_START                (BIT_BASE + 0x180)
#define CMD_DEC_SEQ_BB_SIZE                 (BIT_BASE + 0x184)
#define CMD_DEC_SEQ_OPTION                  (BIT_BASE + 0x188)

#define CMD_DEC_SEQ_MP4_ASP_CLASS           (BIT_BASE + 0x19C)
#define CMD_DEC_SEQ_VC1_STREAM_FMT          (BIT_BASE + 0x19C)
#define CMD_DEC_SEQ_X264_MV_EN              (BIT_BASE + 0x19C)
#define CMD_DEC_SEQ_SPP_CHUNK_SIZE          (BIT_BASE + 0x1A0)

// For MPEG2 only
#define CMD_DEC_SEQ_USER_DATA_OPTION        (BIT_BASE + 0x194)
#define CMD_DEC_SEQ_USER_DATA_BASE_ADDR     (BIT_BASE + 0x1AC)
#define CMD_DEC_SEQ_USER_DATA_BUF_SIZE      (BIT_BASE + 0x1B0)

#define CMD_DEC_SEQ_INIT_ESCAPE             (BIT_BASE + 0x114)

#define RET_DEC_SEQ_BIT_RATE                (BIT_BASE + 0x1B4)
#define RET_DEC_SEQ_EXT_INFO                (BIT_BASE + 0x1B8)
#define RET_DEC_SEQ_SUCCESS                 (BIT_BASE + 0x1C0)
#define RET_DEC_SEQ_SRC_SIZE                (BIT_BASE + 0x1C4)

#define RET_DEC_SEQ_ASPECT                  (BIT_BASE + 0x1C8)
#define RET_DEC_SEQ_FRAME_NEED              (BIT_BASE + 0x1CC)
#define RET_DEC_SEQ_FRAME_DELAY             (BIT_BASE + 0x1D0)
#define RET_DEC_SEQ_INFO                    (BIT_BASE + 0x1D4)
#define RET_DEC_SEQ_VP8_SCALE_INFO          (BIT_BASE + 0x1D4)

#define RET_DEC_SEQ_CROP_LEFT_RIGHT         (BIT_BASE + 0x1D8)
#define RET_DEC_SEQ_CROP_TOP_BOTTOM         (BIT_BASE + 0x1DC)
#define RET_DEC_SEQ_SEQ_ERR_REASON          (BIT_BASE + 0x1E0)


#define RET_DEC_SEQ_FRATE_NR                (BIT_BASE + 0x1E4)
#define RET_DEC_SEQ_FRATE_DR                (BIT_BASE + 0x1E8)
#define RET_DEC_SEQ_HEADER_REPORT           (BIT_BASE + 0x1EC)
#define RET_DEC_SEQ_VUI_INFO                (BIT_BASE + 0x18C)
#define RET_DEC_SEQ_VUI_PIC_STRUCT          (BIT_BASE + 0x1A8)

#define RET_DEC_SEQ_MP2_BAR_LEFT_RIGHT      (BIT_BASE + 0x180)
#define RET_DEC_SEQ_MP2_BAR_TOP_BOTTOM      (BIT_BASE + 0x184)

#define RET_DEC_PIC_MP2_OFFSET1				(BIT_BASE + 0x19C) // for MP2
#define RET_DEC_PIC_MP2_OFFSET2				(BIT_BASE + 0x1A0) // for MP2
#define RET_DEC_PIC_MP2_OFFSET3				(BIT_BASE + 0x1A4) // for MP2
#define RET_DEC_PIC_MP2_OFFSET_NUM			(BIT_BASE + 0x1A8) // for MP2

//------------------------------------------------------------------------------
// [DEC SEQ END] COMMAND
//------------------------------------------------------------------------------
#define RET_DEC_SEQ_END_SUCCESS             (BIT_BASE + 0x1C0)

//------------------------------------------------------------------------------
// [DEC PIC RUN] COMMAND
//----------------------------------------------------
#define CMD_DEC_PIC_ROT_MODE                (BIT_BASE + 0x180)
#define CMD_DEC_PIC_ROT_INDEX               (BIT_BASE + 0x184)
#define CMD_DEC_PIC_ROT_ADDR_Y              (BIT_BASE + 0x188)
#define CMD_DEC_PIC_ROT_ADDR_CB             (BIT_BASE + 0x18C)
#define CMD_DEC_PIC_ROT_ADDR_CR             (BIT_BASE + 0x190)
#define CMD_DEC_PIC_ROT_STRIDE              (BIT_BASE + 0x1B8)
#define CMD_DEC_PIC_ROT_BOTTOM_Y            (BIT_BASE + 0x1E8)    // coda980 only
#define CMD_DEC_PIC_ROT_BOTTOM_CB           (BIT_BASE + 0x1EC)    // coda980 only
#define CMD_DEC_PIC_ROT_BOTTOM_CR           (BIT_BASE + 0x1F0)    // coda980 only

#define CMD_DEC_PIC_OPTION                  (BIT_BASE + 0x194)

#define CMD_DEC_PIC_USER_DATA_BASE_ADDR     (BIT_BASE + 0x1AC)
#define CMD_DEC_PIC_USER_DATA_BUF_SIZE      (BIT_BASE + 0x1B0)

#define CMD_DEC_PIC_NUM_ROWS                (BIT_BASE + 0x1B4)
#define CMD_DEC_PIC_THO_PIC_PARA            (BIT_BASE + 0x198)
#define CMD_DEC_PIC_THO_QMAT_ADDR           (BIT_BASE + 0x1A0)
#define CMD_DEC_PIC_THO_MB_PARA_ADDR        (BIT_BASE + 0x1A4)
#define RET_DEC_PIC_VUI_PIC_STRUCT          (BIT_BASE + 0x1A8)
#define RET_DEC_PIC_AVC_FPA_SEI0            (BIT_BASE + 0x19C)
#define RET_DEC_PIC_AVC_FPA_SEI1            (BIT_BASE + 0x1A0)
#define RET_DEC_PIC_AVC_FPA_SEI2            (BIT_BASE + 0x1A4)
#define RET_DEC_NUM_MB_ROWS                 (BIT_BASE + 0x1B4) // it will be update
#define RET_DEC_PIC_AVC_SEI_RP_INFO         (BIT_BASE + 0x1B4)
#define RET_DEC_PIC_HRD_INFO                (BIT_BASE + 0x1B8)
#define RET_DEC_PIC_SIZE                    (BIT_BASE + 0x1BC)
#define RET_DEC_PIC_FRAME_NUM               (BIT_BASE + 0x1C0)
#define RET_DEC_PIC_FRAME_IDX               (BIT_BASE + 0x1C4)
#define RET_DEC_PIC_DISPLAY_IDX             (BIT_BASE + 0x1C4)
#define RET_DEC_PIC_ERR_MB                  (BIT_BASE + 0x1C8)
#define RET_DEC_PIC_TYPE                    (BIT_BASE + 0x1CC)
#define RET_DEC_PIC_POST                    (BIT_BASE + 0x1D0) // for VC1
#define RET_DEC_PIC_MVC_REPORT              (BIT_BASE + 0x1D0) // for MVC
#define RET_DEC_PIC_OPTION                  (BIT_BASE + 0x1D4)
#define RET_DEC_PIC_SUCCESS                 (BIT_BASE + 0x1D8)
#define RET_DEC_PIC_CUR_IDX                 (BIT_BASE + 0x1DC)
#define RET_DEC_PIC_DECODED_IDX             (BIT_BASE + 0x1DC)
#define RET_DEC_PIC_CROP_LEFT_RIGHT         (BIT_BASE + 0x1E0) // for AVC, MPEG-2
#define RET_DEC_PIC_CROP_TOP_BOTTOM         (BIT_BASE + 0x1E4) // for AVC, MPEG-2
#define RET_DEC_PIC_MODULO_TIME_BASE        (BIT_BASE + 0x1E0) // for MP4
#define RET_DEC_PIC_VOP_TIME_INCREMENT      (BIT_BASE + 0x1E4) // for MP4
#define RET_DEC_PIC_RV_TR                   (BIT_BASE + 0x1E8)
#define RET_DEC_PIC_VP8_PIC_REPORT          (BIT_BASE + 0x1E8)
#define RET_DEC_PIC_ATSC_USER_DATA_INFO     (BIT_BASE + 0x1E8) // H.264, MEPEG2
#define RET_DEC_PIC_VUI_INFO                (BIT_BASE + 0x1EC)
#define RET_DEC_PIC_RV_TR_BFRAME            (BIT_BASE + 0x1EC)
#define RET_DEC_PIC_ASPECT                  (BIT_BASE + 0x1F0)
#define RET_DEC_PIC_VP8_SCALE_INFO          (BIT_BASE + 0x1F0)
#define RET_DEC_PIC_FRATE_NR                (BIT_BASE + 0x1F4)
#define RET_DEC_PIC_FRATE_DR                (BIT_BASE + 0x1F8)
#define RET_DEC_PIC_POC_TOP                 (BIT_BASE + 0x1AC)
#define RET_DEC_PIC_POC_BOT                 (BIT_BASE + 0x1B0)
#define RET_DEC_PIC_POC                     (BIT_BASE + 0x1B0)

//------------------------------------------------------------------------------
// [DEC SET FRAME BUF] COMMAND
//------------------------------------------------------------------------------
#define CMD_SET_FRAME_BUF_NUM               (BIT_BASE + 0x180)
#define CMD_SET_FRAME_BUF_STRIDE            (BIT_BASE + 0x184)

#define CMD_SET_FRAME_SLICE_BB_START        (BIT_BASE + 0x188)
#define CMD_SET_FRAME_SLICE_BB_SIZE         (BIT_BASE + 0x18C)
#define CMD_SET_FRAME_AXI_BIT_ADDR          (BIT_BASE + 0x190)
#define CMD_SET_FRAME_AXI_IPACDC_ADDR       (BIT_BASE + 0x194)
#define CMD_SET_FRAME_AXI_DBKY_ADDR         (BIT_BASE + 0x198)
#define CMD_SET_FRAME_AXI_DBKC_ADDR         (BIT_BASE + 0x19C)
#define CMD_SET_FRAME_AXI_OVL_ADDR          (BIT_BASE + 0x1A0)
#define CMD_SET_FRAME_AXI_BTP_ADDR          (BIT_BASE + 0x1A4)

#define CMD_SET_FRAME_CACHE_SIZE            (BIT_BASE + 0x1A8)
#define CMD_SET_FRAME_CACHE_CONFIG          (BIT_BASE + 0x1AC)
#define CMD_SET_FRAME_MB_BUF_BASE           (BIT_BASE + 0x1B0)
#define CMD_SET_FRAME_MAX_DEC_SIZE          (BIT_BASE + 0x1B8)
#define CMD_SET_FRAME_DELAY                 (BIT_BASE + 0x1BC)

#define RET_SET_FRAME_SUCCESS               (BIT_BASE + 0x1C0)

//------------------------------------------------------------------------------
// [DEC_PARA_SET] COMMAND
//------------------------------------------------------------------------------
#define CMD_DEC_PARA_SET_TYPE               (BIT_BASE + 0x180)
#define CMD_DEC_PARA_SET_SIZE               (BIT_BASE + 0x184)
#define RET_DEC_PARA_SET_SUCCESS            (BIT_BASE + 0x1C0)

//------------------------------------------------------------------------------
// [DEC_BUF_FLUSH] COMMAND
//------------------------------------------------------------------------------
#define RET_DEC_BUF_FLUSH_SUCCESS           (BIT_BASE + 0x1C0)

//------------------------------------------------------------------------------
// [SLEEP/WAKE] COMMAND
//------------------------------------------------------------------------------
#define RET_SLEEP_WAKE_SUCCESS              (BIT_BASE + 0x1C0)

//------------------------------------------------------------------------------
// [SET PIC INFO] COMMAND
//------------------------------------------------------------------------------
#define GDI_PRI_RD_PRIO_L                   (GDMA_BASE + 0x000)
#define GDI_PRI_RD_PRIO_H                   (GDMA_BASE + 0x004)
#define GDI_PRI_WR_PRIO_L                   (GDMA_BASE + 0x008)
#define GDI_PRI_WR_PRIO_H                   (GDMA_BASE + 0x00c)
#define GDI_PRI_RD_LOCK_CNT                 (GDMA_BASE + 0x010)
#define GDI_PRI_WR_LOCK_CNT                 (GDMA_BASE + 0x014)
#define GDI_SEC_RD_PRIO_L                   (GDMA_BASE + 0x018)
#define GDI_SEC_RD_PRIO_H                   (GDMA_BASE + 0x01c)
#define GDI_SEC_WR_PRIO_L                   (GDMA_BASE + 0x020)
#define GDI_SEC_WR_PRIO_H                   (GDMA_BASE + 0x024)
#define GDI_SEC_RD_LOCK_CNT                 (GDMA_BASE + 0x028)
#define GDI_SEC_WR_LOCK_CNT                 (GDMA_BASE + 0x02c)
#define GDI_SEC_CLIENT_EN                   (GDMA_BASE + 0x030)
#define GDI_CONTROL                         (GDMA_BASE + 0x034)
#define GDI_PIC_INIT_HOST                   (GDMA_BASE + 0x038)

#define GDI_HW_VERINFO                      (GDMA_BASE + 0x050)
#define GDI_PINFO_REQ                       (GDMA_BASE + 0x060)
#define GDI_PINFO_ACK                       (GDMA_BASE + 0x064)
#define GDI_PINFO_ADDR                      (GDMA_BASE + 0x068)
#define GDI_PINFO_DATA                      (GDMA_BASE + 0x06c)
#define GDI_BWB_ENABLE                      (GDMA_BASE + 0x070)
#define GDI_BWB_SIZE                        (GDMA_BASE + 0x074)
#define GDI_BWB_STD_STRUCT                  (GDMA_BASE + 0x078)
#define GDI_BWB_STATUS                      (GDMA_BASE + 0x07c)

#define GDI_STATUS                          (GDMA_BASE + 0x080)

#define GDI_DEBUG_0                         (GDMA_BASE + 0x084)
#define GDI_DEBUG_1                         (GDMA_BASE + 0x088)
#define GDI_DEBUG_2                         (GDMA_BASE + 0x08c)
#define GDI_DEBUG_3                         (GDMA_BASE + 0x090)
#define GDI_DEBUG_PROBE_ADDR                (GDMA_BASE + 0x094)
#define GDI_DEBUG_PROBE_DATA                (GDMA_BASE + 0x098)

// write protect
#define GDI_WPROT_ERR_CLR                   (GDMA_BASE + 0x0A0)
#define GDI_WPROT_ERR_RSN                   (GDMA_BASE + 0x0A4)
#define GDI_WPROT_ERR_ADR                   (GDMA_BASE + 0x0A8)
#define GDI_WPROT_RGN_EN                    (GDMA_BASE + 0x0AC)
#define GDI_WPROT_RGN0_STA                  (GDMA_BASE + 0x0B0)
#define GDI_WPROT_RGN0_END                  (GDMA_BASE + 0x0B4)
#define GDI_WPROT_RGN1_STA                  (GDMA_BASE + 0x0B8)
#define GDI_WPROT_RGN1_END                  (GDMA_BASE + 0x0BC)
#define GDI_WPROT_RGN2_STA                  (GDMA_BASE + 0x0C0)
#define GDI_WPROT_RGN2_END                  (GDMA_BASE + 0x0C4)
#define GDI_WPROT_RGN3_STA                  (GDMA_BASE + 0x0C8)
#define GDI_WPROT_RGN3_END                  (GDMA_BASE + 0x0CC)
#define GDI_WPROT_RGN4_STA                  (GDMA_BASE + 0x0D0)
#define GDI_WPROT_RGN4_END                  (GDMA_BASE + 0x0D4)
#define GDI_WPROT_RGN5_STA                  (GDMA_BASE + 0x0D8)
#define GDI_WPROT_RGN5_END                  (GDMA_BASE + 0x0DC)
#define GDI_WPROT_REGIONS                   6

#define GDI_BUS_CTRL                        (GDMA_BASE + 0x0F0)
#define GDI_BUS_STATUS                      (GDMA_BASE + 0x0F4)

#define GDI_SIZE_ERR_FLAG                   (GDMA_BASE + 0x0e0)
#define GDI_ADR_RQ_SIZE_ERR_PRI0            (GDMA_BASE + 0x100)
#define GDI_ADR_RQ_SIZE_ERR_PRI1            (GDMA_BASE + 0x104)
#define GDI_ADR_RQ_SIZE_ERR_PRI1            (GDMA_BASE + 0x104)
#define GDI_ADR_RQ_SIZE_ERR_PRI2            (GDMA_BASE + 0x108)
#define GDI_ADR_WQ_SIZE_ERR_PRI0            (GDMA_BASE + 0x10c)
#define GDI_ADR_WQ_SIZE_ERR_PRI1            (GDMA_BASE + 0x110)
#define GDI_ADR_WQ_SIZE_ERR_PRI2            (GDMA_BASE + 0x114)

#define GDI_ADR_RQ_SIZE_ERR_SEC0            (GDMA_BASE + 0x118)
#define GDI_ADR_RQ_SIZE_ERR_SEC1            (GDMA_BASE + 0x11c)
#define GDI_ADR_RQ_SIZE_ERR_SEC2            (GDMA_BASE + 0x120)

#define GDI_ADR_WQ_SIZE_ERR_SEC0            (GDMA_BASE + 0x124)
#define GDI_ADR_WQ_SIZE_ERR_SEC1            (GDMA_BASE + 0x128)
#define GDI_ADR_WQ_SIZE_ERR_SEC2            (GDMA_BASE + 0x12c)

#define GDI_INFO_CONTROL                    (GDMA_BASE + 0x400)
#define GDI_INFO_PIC_SIZE                   (GDMA_BASE + 0x404)
// GDI 2.0 register
#define GDI_INFO_BASE_Y_TOP                 (GDMA_BASE + 0x408)
#define GDI_INFO_BASE_CB_TOP                (GDMA_BASE + 0x40C)
#define GDI_INFO_BASE_CR_TOP                (GDMA_BASE + 0x410)
#define GDI_INFO_BASE_Y_BOT                 (GDMA_BASE + 0x414)
#define GDI_INFO_BASE_CB_BOT                (GDMA_BASE + 0x418)
#define GDI_INFO_BASE_CR_BOT                (GDMA_BASE + 0x41C)
#define GDI_XY2AXI_LUM_BIT00                (GDMA_BASE + 0x800)
#define GDI_XY2AXI_LUM_BIT1F                (GDMA_BASE + 0x87C)
#define GDI_XY2AXI_CHR_BIT00                (GDMA_BASE + 0x880)
#define GDI_XY2AXI_CHR_BIT1F                (GDMA_BASE + 0x8FC)
#define GDI_XY2AXI_CONFIG                   (GDMA_BASE + 0x900)

//GDI 1.0 register
#define GDI_INFO_BASE_Y                     (GDMA_BASE + 0x408)
#define GDI_INFO_BASE_CB                    (GDMA_BASE + 0x40C)
#define GDI_INFO_BASE_CR                    (GDMA_BASE + 0x410)

#define GDI_XY2_CAS_0                       (GDMA_BASE + 0x800)
#define GDI_XY2_CAS_F                       (GDMA_BASE + 0x83C)

#define GDI_XY2_BA_0                        (GDMA_BASE + 0x840)
#define GDI_XY2_BA_1                        (GDMA_BASE + 0x844)
#define GDI_XY2_BA_2                        (GDMA_BASE + 0x848)
#define GDI_XY2_BA_3                        (GDMA_BASE + 0x84C)

#define GDI_XY2_RAS_0                       (GDMA_BASE + 0x850)
#define GDI_XY2_RAS_F                       (GDMA_BASE + 0x88C)

#define GDI_XY2_RBC_CONFIG                  (GDMA_BASE + 0x890)
#define GDI_RBC2_AXI_0                      (GDMA_BASE + 0x8A0)
#define GDI_RBC2_AXI_1F                     (GDMA_BASE + 0x91C)
#define GDI_TILEDBUF_BASE                   (GDMA_BASE + 0x920)

//------------------------------------------------------------------------------
// Product, Reconfiguration Information
//------------------------------------------------------------------------------
#define DBG_CONFIG_REPORT_0                 (GDMA_BASE + 0x040)    //product name and version
#define DBG_CONFIG_REPORT_1                 (GDMA_BASE + 0x044)    //interface configuration, hardware definition
#define DBG_CONFIG_REPORT_2                 (GDMA_BASE + 0x048)    //standard definition
#define DBG_CONFIG_REPORT_3                 (GDMA_BASE + 0x04C)    //standard detail definition
#define DBG_CONFIG_REPORT_4                 (GDMA_BASE + 0x050)    //definition in cnm_define
#define DBG_CONFIG_REPORT_5                 (GDMA_BASE + 0x054)
#define DBG_CONFIG_REPORT_6                 (GDMA_BASE + 0x058)
#define DBG_CONFIG_REPORT_7                 (GDMA_BASE + 0x05C)

//------------------------------------------------------------------------------
// MEMORY COPY MODULE REGISTER
//------------------------------------------------------------------------------
#define ADDR_DMAC_PIC_RUN                   (DMAC_BASE+0x000)
#define ADDR_DMAC_PIC_STATUS                (DMAC_BASE+0x004)
#define ADDR_DMAC_PIC_OP_MODE               (DMAC_BASE+0x008)
#define ADDR_DMAC_ID                        (DMAC_BASE+0x00c)   //the result muse be 0x4d435059

#define ADDR_DMAC_SRC_BASE_Y                (DMAC_BASE+0x010)
#define ADDR_DMAC_SRC_BASE_CB               (DMAC_BASE+0x014)
#define ADDR_DMAC_SRC_BASE_CR               (DMAC_BASE+0x018)
#define ADDR_DMAC_SRC_STRIDE                (DMAC_BASE+0x01c)

#define ADDR_DMAC_DST_BASE_Y                (DMAC_BASE+0x020)
#define ADDR_DMAC_DST_BASE_CB               (DMAC_BASE+0x024)
#define ADDR_DMAC_DST_BASE_CR               (DMAC_BASE+0x028)
#define ADDR_DMAC_DST_STRIDE                (DMAC_BASE+0x02c)

#define ADDR_DMAC_SRC_MB_POS_X              (DMAC_BASE+0x030)
#define ADDR_DMAC_SRC_MB_POS_Y              (DMAC_BASE+0x034)
#define ADDR_DMAC_SRC_MB_BLK_X              (DMAC_BASE+0x038)
#define ADDR_DMAC_SRC_MB_BLK_Y              (DMAC_BASE+0x03c)

#define ADDR_DMAC_DST_MB_POS_X              (DMAC_BASE+0x040)
#define ADDR_DMAC_DST_MB_POS_Y              (DMAC_BASE+0x044)
#define ADDR_DMAC_DST_MB_BLK_X              (DMAC_BASE+0x048)
#define ADDR_DMAC_DST_MB_BLK_Y              (DMAC_BASE+0x04c)

#define ADDR_DMAC_SET_COLOR_Y               (DMAC_BASE+0x050)
#define ADDR_DMAC_SET_COLOR_CB              (DMAC_BASE+0x054)
#define ADDR_DMAC_SET_COLOR_CR              (DMAC_BASE+0x058)

#define ADDR_DMAC_SUB_SAMPLE_X              (DMAC_BASE+0x060)
#define ADDR_DMAC_SUB_SAMPLE_Y              (DMAC_BASE+0x064)

//------------------------------------------------------------------------------
// DMAC
//------------------------------------------------------------------------------
#define DMAC_DMAC_RUN                       (DMAC_BASE + 0x00)
#define DMAC_SOFT_RESET                     (DMAC_BASE + 0x04)
#define DMAC_DMAC_MODE                      (DMAC_BASE + 0x08)
#define DMAC_DESC_ADDR                      (DMAC_BASE + 0x0c)
#define DMAC_DESC0                          (DMAC_BASE + 0x10)
#define DMAC_DESC1                          (DMAC_BASE + 0x14)
#define DMAC_DESC2                          (DMAC_BASE + 0x18)
#define DMAC_DESC3                          (DMAC_BASE + 0x1c)
#define DMAC_DESC4                          (DMAC_BASE + 0x20)
#define DMAC_DESC5                          (DMAC_BASE + 0x24)
#define DMAC_DESC6                          (DMAC_BASE + 0x28)
#define DMAC_DESC7                          (DMAC_BASE + 0x2c)

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// [FIRMWARE VERSION] COMMAND
// [32:16] project number =>
// [16:0]  version => xxxx.xxxx.xxxxxxxx
//------------------------------------------------------------------------------
#define RET_FW_VER_NUM                      (BIT_BASE + 0x1c0)
#define RET_FW_CODE_REV                     (BIT_BASE + 0x1c4)

#endif