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
* @file    : nc_ts_fsync_flipflop_buffers.h
*
* @brief   : nc thread-safe flip-flop buffers for frame sync header (max 10 instances available)
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

#ifndef NC_TS_FSYNC_FLIPFLOP_BUFFERS_H
#define NC_TS_FSYNC_FLIPFLOP_BUFFERS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pthread.h"

#define FIXED_FSYNC_FLIP_FLOP_BUF_CNT (2)

extern int nc_tsfs_ff_create_buffers (unsigned int owner, int each_buf_size);
extern void nc_tsfs_ff_destroy_buffers (unsigned int owner);
extern void* nc_tsfs_ff_get_addr_of_buffer (unsigned int owner, int idx);
extern int nc_tsfs_ff_get_buf_size (unsigned int owner);

// for read
extern void* nc_tsfs_ff_get_readable_buffer_and_timestamp (unsigned int owner, uint64_t* ret_timestamp);
extern void nc_tsfs_ff_finish_read_buf (unsigned int owner);

// for write
extern void* nc_tsfs_ff_get_writable_buffer_and_set_timestamp (unsigned int owner, int* ret_idx, uint64_t set_timestamp);
extern void nc_tsfs_ff_finish_write_buf (unsigned int owner);

#endif // #ifndef NC_TS_FSYNC_FLIPFLOP_BUFFERS_H
