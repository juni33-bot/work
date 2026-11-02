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
* @file    : nc_ts_flipflop_buffers.h
*
* @brief   : nc thread-safe flip-flop buffers header (max 10 instances available)
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

#ifndef NC_TS_FLIPFLOP_BUFFERS_H
#define NC_TS_FLIPFLOP_BUFFERS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pthread.h"

#define FIXED_FLIP_FLOP_BUF_CNT (2)

extern int nc_tsff_create_buffers (unsigned int owner, int each_buf_size);
extern void nc_tsff_destroy_buffers (unsigned int owner);
extern void* nc_tsff_get_addr_of_buffer (unsigned int owner, int idx);
extern int nc_tsff_get_buf_size (unsigned int owner);

// for read
extern void* nc_tsff_get_readable_buffer (unsigned int owner);
extern void nc_tsff_finish_read_buf (unsigned int owner);

// for write
extern void* nc_tsff_get_writable_buffer (unsigned int owner, int* ret_idx);
extern void nc_tsff_finish_write_buf (unsigned int owner);

#endif // #ifndef NC_TS_FLIPFLOP_BUFFERS_H
