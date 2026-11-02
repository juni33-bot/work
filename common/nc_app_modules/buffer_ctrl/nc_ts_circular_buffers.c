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
* @file    : nc_ts_circular_buffers.c
*
* @brief   : nc_ts_circular_buffers source (thread-safe circular buffers)
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

#include "nc_ts_circular_buffers.h"

// #define PRINT_DEBUG

#define MAX_TSB_CNT (20)

/**
 * @brief buffer management
 *
 */

int g_buf_tcnt[MAX_TSB_CNT] = {0,};
int g_buf_size[MAX_TSB_CNT] = {0,};
uint64_t g_head[MAX_TSB_CNT] = {0,};
uint64_t g_tail[MAX_TSB_CNT] = {0,};

typedef struct {
    void* data_ptr;
    size_t data_size;
} s_element;

pthread_spinlock_t g_write_spinlock[MAX_TSB_CNT];
s_element g_circular_queue[MAX_TSB_CNT][MAX_CBQ_BUF_CNT] = {{{NULL,0},},};

void lock_cb_write_spinlock(int owner) { pthread_spin_lock(&g_write_spinlock[owner]); }
void unlock_cb_write_spinlock(int owner) { pthread_spin_unlock(&g_write_spinlock[owner]); }

int nc_tscb_create_buffers(unsigned int owner, int max_cnt, int buf_max_size)
{
    if (owner >= MAX_TSB_CNT) return -1;

    if (max_cnt > (int)MAX_CBQ_BUF_CNT) {
        #ifdef PRINT_DEBUG
        printf("[error] can't create thread-safe circular buffer : max count(%d) > pre-defined max buffer count (%d)\n", max_cnt, (int)MAX_CBQ_BUF_CNT);
        #endif
        return -1;
    }

    g_buf_tcnt[owner] = max_cnt;
    g_buf_size[owner] = buf_max_size;
    g_tail[owner] = 0;
    g_head[owner] = 0;
    pthread_spin_init(&g_write_spinlock[owner], 0);

    for (int i = 0; i < g_buf_tcnt[owner]; i++) {
        g_circular_queue[owner][i].data_ptr = malloc(g_buf_size[owner]);
        g_circular_queue[owner][i].data_size = 0;
    }

    return 0;
}

void nc_tscb_destroy_buffers(unsigned int owner)
{
    if (owner >= MAX_TSB_CNT) return;

    lock_cb_write_spinlock(owner);
    g_buf_tcnt[owner] = 0;
    g_buf_size[owner] = 0;
    g_tail[owner] = 0;
    g_head[owner] = 0;
    for (int i = 0; i < g_buf_tcnt[owner]; i++) {
        if (g_circular_queue[owner][i].data_ptr) {
            free(g_circular_queue[owner][i].data_ptr);
            g_circular_queue[owner][i].data_ptr = NULL;
            g_circular_queue[owner][i].data_size = 0;
        }
    }
    unlock_cb_write_spinlock(owner);

    pthread_spin_destroy(&g_write_spinlock[owner]);
}

int nc_tscb_max_count (unsigned int owner)
{
    if (owner >= MAX_TSB_CNT) return -1;
    return g_buf_tcnt[owner];
}

int nc_tscb_current_count (unsigned int owner)
{
    if (owner >= MAX_TSB_CNT) return -1;
    return g_tail[owner] - g_head[owner];
}

int nc_tscb_max_buf_size(unsigned int owner)
{
    if (owner >= MAX_TSB_CNT) return -1;
    return g_buf_size[owner];
}

int nc_tscb_enqueue(unsigned int owner, void* data, size_t size)
{
    if (owner >= MAX_TSB_CNT) return -1;
    if (!data || size == 0) return -1;

    lock_cb_write_spinlock(owner);
    int tail = g_tail[owner];
    int head = g_head[owner];

    if ((tail - head) >= g_buf_tcnt[owner]) {
        #ifdef PRINT_DEBUG
        // queue is full
        printf("[error] queue is full : tail(%d) - head(%d) >= g_buf_tcnt(%d) --- c_cnt(%d)\n", tail, head, g_buf_tcnt[owner], tail - head);
        #endif
        unlock_cb_write_spinlock(owner);
        return -1;
    }

    int c_idx = tail % g_buf_tcnt[owner];
    memcpy(g_circular_queue[owner][c_idx].data_ptr, data, size);
    g_circular_queue[owner][c_idx].data_size = size;

    g_tail[owner]++;

    unlock_cb_write_spinlock(owner);

    return 0;
}

int nc_tscb_dequeue(unsigned int owner, void* ret_data, size_t *ret_size)
{
    if (owner >= MAX_TSB_CNT) return -1;
    if (!ret_data) return -1;

    lock_cb_write_spinlock(owner);
    int tail = g_tail[owner];
    unlock_cb_write_spinlock(owner);
    int head = g_head[owner];

    if (head == tail) {
        #ifdef PRINT_DEBUG
        // queue is empty
        printf("[error] queue is empty : tail(%d) head(%d) c_cnt(%d)\n", tail, head, tail - head);
        #endif
        return -1;
    }

    int c_idx = head % g_buf_tcnt[owner];
    memcpy(ret_data, g_circular_queue[owner][c_idx].data_ptr, g_circular_queue[owner][c_idx].data_size);
    *ret_size = g_circular_queue[owner][c_idx].data_size;

    g_head[owner]++;

    return 0;
}
