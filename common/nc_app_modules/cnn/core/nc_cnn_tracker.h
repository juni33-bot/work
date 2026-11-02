/**
********************************************************************************
* Copyright (C) 2024 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : nc_cnn_tracker.h
*
* @brief   : nc_cnn_tracker header
*
* @author  : Software Development Team.  NextChip Inc.
*
* @date    : 2024.09.23.
*
* @version : 1.0.0
********************************************************************************
* @note
*
********************************************************************************
*/

#ifndef __NC_CNN_TRACKER_H__
#define __NC_CNN_TRACKER_H__

#include "nc_cnn_common.h"

int nc_init_bytetrackers (int fps, int cam_ch, E_NETWORK_UID net_id);
void nc_deInit_bytetrackers (int cam_ch);
void nc_cnn_track(stCnnPostprocessingResults *out_cnn_results, E_NETWORK_UID net_id, int cam_ch);

#endif // __NC_CNN_TRACKER_H__
