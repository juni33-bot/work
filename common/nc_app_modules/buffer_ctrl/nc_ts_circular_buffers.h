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
* @file    : nc_ts_circular_buffers.h
*
* @brief   : nc_ts_circular_buffers header (thread-safe circular buffers)
*            for single producer / consumer
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2022.08.09
*
* @version : 1.0.0
********************************************************************************
* @note
* 2022.08.09 / 1.0.0 / Initial released. (bwryu@nextchip)
*
********************************************************************************
*/

#ifndef NC_TS_CIRCULAR_BUFFERS_H
#define NC_TS_CIRCULAR_BUFFERS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pthread.h"

#define MAX_CBQ_BUF_CNT (10)

extern int nc_tscb_create_buffers (unsigned int owner, int max_cnt, int buf_max_size);
extern void nc_tscb_destroy_buffers (unsigned int owner);

extern int nc_tscb_max_count (unsigned int owner);
extern int nc_tscb_current_count (unsigned int owner);
extern int nc_tscb_max_buf_size (unsigned int owner);

extern int nc_tscb_enqueue (unsigned int owner, void* data, size_t size);
extern int nc_tscb_dequeue (unsigned int owner, void* ret_data, size_t *ret_size);

#endif // #ifndef NC_TS_CIRCULAR_BUFFERS_H
