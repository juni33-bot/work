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
* @file    : nc_cnn_network_includes.h
*
* @brief   : nc_cnn_network_includes header
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2022.10.26
*
* @version : 1.0.0
********************************************************************************
* @note
* 2022.10.26 / 1.0.0 / Initial released.
*
********************************************************************************
*/

#ifndef __NC_CNN_NETWORK_INCLUDES_H__
#define __NC_CNN_NETWORK_INCLUDES_H__

/**
 *****************************************************************
 * !!! include custom cnn network source (pelee / yolo... etc) !!!
 *****************************************************************
 */
typedef enum {
    NETWORK_YOLOV8_DET = 0,
    NETWORK_YOLOV5_DET = 1,
    NETWORK_PELEE_SEG = 2,
    NETWORK_PELEE_DET,
    NETWORK_UFLD_LANE,
    NETWORK_TRI_CHIMERA,
    MAX_NETWORK_IDS
} E_NETWORK_UID;

typedef enum {
    DETECTION = 0,
    SEGMENTATION = 1,
    LANE = 2,
    MAX_NETWORK_TASK
} E_NETWORK_TASK;

#ifdef USE_CNN_PELEEDET_NETWORK
#include "nc_cnn_pelee_postprocess.h"
#endif
#ifdef USE_CNN_YOLOV5_NETWORK
#include "nc_cnn_yolov5_postprocess.h"
#endif
#ifdef USE_CNN_YOLOV8_NETWORK
#include "nc_cnn_yolov8_postprocess.h"
#endif
#ifdef USE_CNN_PELEESEG_NETWORK
#include "nc_cnn_segmentation_postprocess.h"
#endif
#ifdef USE_CNN_UFLD_NETWORK
#include "nc_cnn_ufld_postprocess.h"
#endif
#ifdef USE_CNN_TRICHIMERA_NETWORK
#include "nc_cnn_trichimera_postprocess.h"
#endif
/****************************************************************/


#endif // __NC_CNN_NETWORK_INCLUDES_H__
