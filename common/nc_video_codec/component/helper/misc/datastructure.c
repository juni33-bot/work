//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
// 
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
// 
// The entire notice above must be reproduced on all authorized copies.
// 
// Description  : 
//-----------------------------------------------------------------------------

#include "main_helper.h"

Queue* Queue_Create(
    Uint32      itemCount,
    Uint32      itemSize
    )
{
    Queue* queue = NULL; 

    if ((queue=(Queue *)osal_malloc(sizeof(Queue))) == NULL)
        return NULL;
    queue->size   = itemCount;
    queue->itemSize = itemSize;
    queue->count  = 0;
    queue->front  = 0;
    queue->rear   = 0;
    queue->buffer = (Uint8*)osal_malloc(itemCount*itemSize);
    queue->lock = NULL;

    return queue;
}

Queue* Queue_Create_With_Lock(
    Uint32      itemCount,
    Uint32      itemSize
    )
{
    Queue* queue = NULL; 

    if ((queue=(Queue *)osal_malloc(sizeof(Queue))) == NULL)
        return NULL;
    queue->size   = itemCount;
    queue->itemSize = itemSize;
    queue->count  = 0;
    queue->front  = 0;
    queue->rear   = 0;
    queue->buffer = (Uint8*)osal_malloc(itemCount*itemSize);
    queue->lock = osal_mutex_create();

    return queue;
}

void Queue_Destroy(
    Queue*      queue
    )
{
    if (queue == NULL) 
        return;

    if (queue->buffer)
        osal_free(queue->buffer);
    if (queue->lock)
        osal_mutex_destroy(queue->lock);
    osal_free(queue);
}

BOOL Queue_Enqueue(
    Queue*      queue, 
    void*       data
    )
{
    Uint8*      ptr;
    Uint32      offset;

    if (queue == NULL) return FALSE;
    if (data  == NULL) return FALSE;

    if (queue->lock)
        osal_mutex_lock(queue->lock);

    /* Queue is full */
    if (queue->count == queue->size) {
        if (queue->lock)
            osal_mutex_unlock(queue->lock);
        return -1;
    }
    offset = queue->rear * queue->itemSize;

    ptr = &queue->buffer[offset];
    osal_memcpy(ptr, data, queue->itemSize);
    queue->rear++;
    queue->rear %= queue->size;
    queue->count++;

    if (queue->lock)
        osal_mutex_unlock(queue->lock);

    return TRUE;
}

void* Queue_Dequeue(
    Queue*      queue
    )
{
    void* data;
    Uint32   offset;

    if (queue == NULL) 
        return NULL;

    if (queue->lock)
        osal_mutex_lock(queue->lock);

    /* Queue is empty */
    if (queue->count == 0) {
        if (queue->lock)
            osal_mutex_unlock(queue->lock);
        return NULL;
    }
    offset = queue->front * queue->itemSize;
    data   = (void*)&queue->buffer[offset];
    queue->front++;
    queue->front %= queue->size;
    queue->count--;

    if (queue->lock)
        osal_mutex_unlock(queue->lock);
    return data;
}

void Queue_Flush(
    Queue*      queue
    )
{
    if (queue == NULL) 
        return;
    if (queue->lock)
        osal_mutex_lock(queue->lock);
    queue->count = 0;
    queue->front = 0;
    queue->rear  = 0;
    if (queue->lock)
        osal_mutex_unlock(queue->lock);
    return;
}

void* Queue_Peek(
    Queue*      queue
    )
{
    Uint32      offset;
    void*       temp;

    if (queue == NULL) 
        return NULL;

    /* Queue is empty */
    if (queue->count == 0) {
        return NULL;
    }
    offset = queue->front * queue->itemSize;
    temp = (void*)&queue->buffer[offset];

    return  temp;
}

Uint32   Queue_Get_Cnt(
    Queue*      queue
    )
{
    Uint32      cnt;

    if (queue == NULL) 
        return 0;
    if (queue->lock)
        osal_mutex_lock(queue->lock);
    cnt = queue->count;
    if (queue->lock)
        osal_mutex_unlock(queue->lock);
    return cnt;
}

BOOL Queue_IsFull(
    Queue*      queue
    )
{
    if (queue == NULL) {
        return FALSE;
    }

    return (queue->count == queue->size);
}

Queue* Queue_Copy(
    Queue*  dstQ,
    Queue*  srcQ
    )
{
    Queue*   queue = NULL; 
    Uint32   bufferSize;

    if (dstQ == NULL) {
        if ((queue=(Queue *)osal_malloc(sizeof(Queue))) == NULL)
            return NULL;
        osal_memset((void*)queue, 0x00, sizeof(Queue));
    } 
    else {
        queue = dstQ;
    }

    bufferSize      = srcQ->size * srcQ->itemSize;
    queue->size     = srcQ->size;
    queue->itemSize = srcQ->itemSize;
    queue->count    = srcQ->count;
    queue->front    = srcQ->front;
    queue->rear     = srcQ->rear;
    if (queue->buffer) {
        osal_free(queue->buffer);
    }
    queue->buffer   = (Uint8*)osal_malloc(bufferSize);

    osal_memcpy(queue->buffer, srcQ->buffer, bufferSize);

    return queue;
}


