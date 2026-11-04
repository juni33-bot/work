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
* @file    : nc_cnn_communicator.h
*
* @brief   : nc_cnn_communicator header
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


#ifndef __NC_CNN_COMMUNICATOR_H__
#define __NC_CNN_COMMUNICATOR_H__

#include <stdint.h>
#include <fcntl.h>
#include <mqueue.h>
#include <errno.h>

#include "nc_utils.h"
#include "nc_cnn_aiware_runtime.h"

extern int nc_cnn_send_inference_result (struct cnn_config* config, uint64_t time_stamp, cnn_output_info cnn_output);
extern int nc_cnn_receive_inference_result (inference_result_msg **out_inference_result);
extern void nc_cnn_cleanup_inference_result (inference_result_msg **out_inference_result);
#endif
