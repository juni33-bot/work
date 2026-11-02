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
* @file    : nc_ts_flipflop_buffers.c
*
* @brief   : nc thread-safe flip-flop buffers source (max 10 instances available)
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

/*
********************************************************************************
*               INCLUDES
********************************************************************************
*/

#include "nc_ts_flipflop_buffers.h"
#include "nc_utils.h"

// define
#define MAX_TSB_CNT (20)

typedef struct {
    int buf_size;
    int write_idx;
    int read_idx;
    void* buffer_ptr[FIXED_FLIP_FLOP_BUF_CNT];
    pthread_spinlock_t read_spinlock;
    pthread_spinlock_t write_spinlock;
} s_tsff_element;

// variable
s_tsff_element g_tsff_buf_arr[MAX_TSB_CNT];

// function
void lock_tsff_read_spinlock(int owner) { pthread_spin_lock(&g_tsff_buf_arr[owner].read_spinlock); }
void unlock_tsff_read_spinlock(int owner) { pthread_spin_unlock(&g_tsff_buf_arr[owner].read_spinlock); }
void lock_tsff_write_spinlock(int owner) { pthread_spin_lock(&g_tsff_buf_arr[owner].write_spinlock); }
void unlock_tsff_write_spinlock(int owner) { pthread_spin_unlock(&g_tsff_buf_arr[owner].write_spinlock); }

// owner value range : 0 ~ (MAX_TSB_CNT-1)
int nc_tsff_create_buffers(unsigned int owner, int each_buf_size)
{
    if(owner >= (int)MAX_TSB_CNT) {
        printf("[Error] fail to create tsff buffer...\n");
        return -1;
    }

    g_tsff_buf_arr[owner].buf_size = each_buf_size;
    g_tsff_buf_arr[owner].write_idx = -1;
    g_tsff_buf_arr[owner].read_idx = -1;
    pthread_spin_init(&g_tsff_buf_arr[owner].read_spinlock, 0);
    pthread_spin_init(&g_tsff_buf_arr[owner].write_spinlock, 0);
    for (int i = 0; i < FIXED_FLIP_FLOP_BUF_CNT; i++) {
        g_tsff_buf_arr[owner].buffer_ptr[i] = malloc(g_tsff_buf_arr[owner].buf_size);
    }

    printf("[Info] %s() called... owner(%d) each buffer size(%d)....\n", __FUNCTION__, owner, each_buf_size);
    return 0;
}

void nc_tsff_destroy_buffers(unsigned int owner)
{
    g_tsff_buf_arr[owner].buf_size = 0;
    g_tsff_buf_arr[owner].write_idx = -1;
    g_tsff_buf_arr[owner].read_idx = -1;
    pthread_spin_destroy(&g_tsff_buf_arr[owner].read_spinlock);
    pthread_spin_destroy(&g_tsff_buf_arr[owner].write_spinlock);
    for (int i = 0; i < FIXED_FLIP_FLOP_BUF_CNT; i++) {
        if (g_tsff_buf_arr[owner].buffer_ptr[i]) {
            free(g_tsff_buf_arr[owner].buffer_ptr[i]);
            g_tsff_buf_arr[owner].buffer_ptr[i] = NULL;
        }
    }
}

void* nc_tsff_get_addr_of_buffer(unsigned int owner, int idx)
{
    lock_tsff_write_spinlock(owner);
    if (idx >= (int)FIXED_FLIP_FLOP_BUF_CNT) {
        unlock_tsff_write_spinlock(owner);
        return NULL;
    }
    void *ptr = g_tsff_buf_arr[owner].buffer_ptr[idx];
    unlock_tsff_write_spinlock(owner);
    return ptr;
}

int nc_tsff_get_buf_size (unsigned int owner)
{
    return g_tsff_buf_arr[owner].buf_size;
}

// for reading
void* nc_tsff_get_readable_buffer(unsigned int owner)
{
    lock_tsff_read_spinlock(owner);
    if (g_tsff_buf_arr[owner].read_idx < 0) {
        return NULL;
    }
    return g_tsff_buf_arr[owner].buffer_ptr[g_tsff_buf_arr[owner].read_idx];
}

void nc_tsff_finish_read_buf(unsigned int owner)
{
    unlock_tsff_read_spinlock(owner);
}

// for writing
void* nc_tsff_get_writable_buffer (unsigned int owner, int* ret_idx)
{
    lock_tsff_write_spinlock(owner);

    if (g_tsff_buf_arr[owner].write_idx < 0) {
        g_tsff_buf_arr[owner].write_idx = 0;
    }
    *ret_idx = g_tsff_buf_arr[owner].write_idx;

    return g_tsff_buf_arr[owner].buffer_ptr[*ret_idx];
}

void nc_tsff_finish_write_buf(unsigned int owner)
{
    lock_tsff_read_spinlock(owner);
    g_tsff_buf_arr[owner].read_idx = g_tsff_buf_arr[owner].write_idx;
    unlock_tsff_read_spinlock(owner);

    g_tsff_buf_arr[owner].write_idx = (g_tsff_buf_arr[owner].write_idx + 1) % FIXED_FLIP_FLOP_BUF_CNT;

    unlock_tsff_write_spinlock(owner);
}
