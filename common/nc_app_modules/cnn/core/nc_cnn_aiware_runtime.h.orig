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
* @file    : nc_cnn_aiware_runtime.h
*
* @brief   : nc_cnn_aiware_runtime header
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2021.10.08.
*
* @version : 1.0.0
********************************************************************************
* @note
* 10.08.2021 / 1.0.0 / Initial released.
*
********************************************************************************
*/

#ifndef __CNN_AIW_RUNTIME_H__
#define __CNN_AIW_RUNTIME_H__

#include "aiware/common/c/binary.h"
#include "aiware/common/c/version.h"
#include "aiware/runtime/c/device.h"
#include "aiware/runtime/c/program.h"
#include "aiware/runtime/c/programset.h"
#include "aiware/runtime/c/version.h"
#include "aiware/common/c/types.h"

#include "nc_cnn_network_includes.h"
#include "nc_cnn_worker_for_postprocess.h"
#include "nc_cnn_common.h"

/**
 ***********************************************************
 ****************      DON'T modify below        ***********
 ***********************************************************
*/
#define APACHE6_BLK_CNN_AIW_BASE        (0x08000000U)
#define SFR_REMAP_BASE                  (0U)

#define TILESIZE                        64
#define TILESIZE_SQR                    (TILESIZE * TILESIZE)
#define CELLSIZE                        8
#define CELLSIZE_SQR                    (CELLSIZE * CELLSIZE)
#define CELLSIZE_TWI                    (2 * CELLSIZE)
#define CELLSIZE_QUA                    (4 * CELLSIZE)

#define MAX_MQ_MSG_CNT                  20
#define MQ_NAME_DATA                    "/post_cnn_data"

#define RGB_CNT                         (3)

#define FLOAT_TYPE                      (1U)
#define UINT8_TYPE                      (0U)

#define AIW_VERSION_CODE(major, minor, patch) \
    (((major) << 16) + ((minor) << 8) + (patch))

typedef enum {
    CNN_TILED_DT_FLOAT,
    CNN_TILED_DT_U8,
} E_CNN_TILED_DT;

typedef struct cnn_output_info {
    uint32_t width;
    uint32_t height;
    uint32_t channel;
    E_CNN_TILED_DT data_type;
    void *tiled_data;
    uint32_t cam_ch;
    E_NETWORK_UID net_id;

    uint32_t index_of_total; // index of total tensor count
    uint32_t total_tensor_cnt; // tensor total count : (ex) yolov5 has 3 tensor output by 1 frame

    aiwTensorInfo tinfo_in;
    aiwTensorInfo *tinfo;
    aiw_u64_t buffer_size;
} cnn_output_info;

typedef struct cnn_config {
    E_NETWORK_UID network_id;
    int program_idx;
    aiwProgram *program;
    aiwTensor *input_tensor;
    aiwTensorInfo input_tinfo;
    PtrFuncPostprc callback_pp;
} cnn_config;

typedef struct inference_result_msg {
    E_NETWORK_UID network_id;
    uint64_t time_stamp;
    cnn_output_info cnn_output;
    PtrFuncPostprc ptr_pp;
} inference_result_msg;

typedef struct {
    uint32_t cam_ch;
    uint8_t *ptr_cnn_buf;
    uint64_t time_stamp_us;
    E_NETWORK_UID net_id;
} stCnnData;

typedef struct st_npu_input_info {
    unsigned int w;
    unsigned int h;
    unsigned int rgb_size;
} st_npu_input_info;

typedef struct {
    uint64_t time_stamp;
    int cam_channel;
    E_NETWORK_UID net_id;
    E_NETWORK_TASK net_task;
    stObjDrawInfo draw_info; // To be removed (cnn)
    stSegDrawInfo seg_info;
    stLaneDrawInfo lane_draw_info;
    stCnnPostprocessingResults cnn_result;
} pp_result_buf;

extern int nc_aiw_init_cnn(void);
extern int nc_aiw_add_network_to_builder(const char *path, E_NETWORK_UID uid, PtrFuncPostprc ptr_pp_func);
extern int nc_aiw_finish_network_builder(void);
extern void nc_aiw_run_cnn(unsigned char *planar_rgb, uint64_t time_stamp, uint32_t cam_ch, E_NETWORK_UID net_id);
extern int nc_get_cnn_networks_num(void);
extern int nc_get_cnn_network_input_resol(int uid, aiwTensorInfo *tinfo);
extern int nc_get_cnn_networks_id(void);
#endif // __CNN_AIW_RUNTIME_H__
