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
* @file    : nc_ts_fsync_circular_buffers.c
*
* @brief   : nc_ts_fsync_circular_buffers source (thread-safe circular buffers)
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

/*
********************************************************************************
*               INCLUDES
********************************************************************************
*/

#include "nc_ts_fsync_circular_buffers.h"

// #define PRINT_DEBUG

#define MAX_TSB_CNT (20)

/**
 * @brief buffer management
 *
 */

uint64_t g_fs_buf_tcnt[MAX_TSB_CNT] = {0,};
uint64_t g_fs_buf_size[MAX_TSB_CNT] = {0,};
uint64_t g_fs_head[MAX_TSB_CNT] = {0,};
uint64_t g_fs_tail[MAX_TSB_CNT] = {0,};

pthread_spinlock_t g_fs_write_spinlock[MAX_TSB_CNT];
stFrameElement g_fs_circular_queue[MAX_TSB_CNT][MAX_FSYNC_CBQ_BUF_CNT] = {{{0,0,NULL},},};

void lock_fscb_write_spinlock(int owner) { pthread_spin_lock(&g_fs_write_spinlock[owner]); }
void unlock_fscb_write_spinlock(int owner) { pthread_spin_unlock(&g_fs_write_spinlock[owner]); }

int nc_tsfs_cb_create_buffers(unsigned int owner, int max_cnt, int buf_max_size)
{
    uint64_t i = 0;

    if (owner >= MAX_TSB_CNT) return -1;

    if (max_cnt > (int)MAX_FSYNC_CBQ_BUF_CNT) {
        #ifdef PRINT_DEBUG
        printf("[error] can't create thread-safe circular buffer : max count(%d) > pre-defined max buffer count (%d)\n", max_cnt, (int)MAX_FSYNC_CBQ_BUF_CNT);
        #endif
        return -1;
    }

    g_fs_buf_tcnt[owner] = max_cnt;
    g_fs_buf_size[owner] = buf_max_size;
    g_fs_tail[owner] = 0;
    g_fs_head[owner] = 0;
    pthread_spin_init(&g_fs_write_spinlock[owner], 0);

    for (i = 0; i < g_fs_buf_tcnt[owner]; i++) {
        g_fs_circular_queue[owner][i].data_ptr = malloc(g_fs_buf_size[owner]);
        g_fs_circular_queue[owner][i].data_size = 0;
    }

    return 0;
}

void nc_tsfs_cb_destroy_buffers(unsigned int owner)
{
    uint64_t i = 0;

    if (owner >= MAX_TSB_CNT) return;

    lock_fscb_write_spinlock(owner);
    g_fs_buf_tcnt[owner] = 0;
    g_fs_buf_size[owner] = 0;
    g_fs_tail[owner] = 0;
    g_fs_head[owner] = 0;
    for (i = 0; i < g_fs_buf_tcnt[owner]; i++) {
        if (g_fs_circular_queue[owner][i].data_ptr) {
            free(g_fs_circular_queue[owner][i].data_ptr);
            g_fs_circular_queue[owner][i].data_ptr = NULL;
            g_fs_circular_queue[owner][i].data_size = 0;
        }
    }
    unlock_fscb_write_spinlock(owner);

    pthread_spin_destroy(&g_fs_write_spinlock[owner]);
}

uint64_t nc_tsfs_cb_max_count (unsigned int owner)
{
    if (owner >= MAX_TSB_CNT) return -1;
    return g_fs_buf_tcnt[owner];
}

uint64_t nc_tsfs_cb_current_count (unsigned int owner)
{
    if (owner >= MAX_TSB_CNT) return -1;
    return g_fs_tail[owner] - g_fs_head[owner];
}

uint64_t nc_tsfs_cb_max_buf_size(unsigned int owner)
{
    if (owner >= MAX_TSB_CNT) return -1;
    return g_fs_buf_size[owner];
}

int nc_tsfs_cb_enqueue(unsigned int owner, uint64_t time_stamp, void* data, size_t size)
{
    uint64_t c_idx = 0;

    if (owner >= MAX_TSB_CNT) return -1;
    if (!data || size == 0) return -1;

    lock_fscb_write_spinlock(owner);
    uint64_t tail = g_fs_tail[owner];
    uint64_t head = g_fs_head[owner];

    if ((tail - head) >= g_fs_buf_tcnt[owner]) {
        #ifdef PRINT_DEBUG
        // queue is full
        printf("[error] queue is full : tail(%d) - head(%d) >= g_fs_buf_tcnt(%d) --- c_cnt(%d)\n", tail, head, g_fs_buf_tcnt[owner], tail - head);
        #endif
        unlock_fscb_write_spinlock(owner);
        return -1;
    }

    c_idx = tail % g_fs_buf_tcnt[owner];
    g_fs_circular_queue[owner][c_idx].time_stamp = time_stamp;
    g_fs_circular_queue[owner][c_idx].data_size = size;
    memcpy(g_fs_circular_queue[owner][c_idx].data_ptr, data, size);

    g_fs_tail[owner]++;

    unlock_fscb_write_spinlock(owner);

    return 0;
}

int nc_tsfs_cb_get_time_stamp_of_current(unsigned int owner, uint64_t *ts)
{
    if(nc_tsfs_cb_current_count(owner) < 1) return -1;
    *ts = g_fs_circular_queue[owner][g_fs_head[owner] % g_fs_buf_tcnt[owner]].time_stamp;
    return 0;
}

int nc_tsfs_cb_dequeue_start(unsigned int owner, stFrameElement* ret_data)
{
    uint64_t c_idx = 0;

    if (owner >= MAX_TSB_CNT) return -1;
    if (!ret_data) return -1;
    // if (!ret_data->data_ptr) return -1;

    lock_fscb_write_spinlock(owner);
    uint64_t tail = g_fs_tail[owner];
    unlock_fscb_write_spinlock(owner);
    uint64_t head = g_fs_head[owner];

    if (head == tail) {
        #ifdef PRINT_DEBUG
        // queue is empty
        printf("[error] queue is empty : tail(%d) head(%d) c_cnt(%d)\n", tail, head, tail - head);
        #endif
        return -1;
    }

    c_idx = head % g_fs_buf_tcnt[owner];
    ret_data->time_stamp = g_fs_circular_queue[owner][c_idx].time_stamp;
    ret_data->data_size = g_fs_circular_queue[owner][c_idx].data_size;
    // if (cp_data) {
    //     memcpy(ret_data->data_ptr, g_fs_circular_queue[owner][c_idx].data_ptr, ret_data->data_size);
    // }
    ret_data->data_ptr = g_fs_circular_queue[owner][c_idx].data_ptr;
    // g_fs_head[owner]++;

    return 0;
}

int nc_tsfs_cb_dequeue_finish(unsigned int owner)
{
    if (owner >= MAX_TSB_CNT) return -1;

    g_fs_head[owner]++;

    return 0;
}
