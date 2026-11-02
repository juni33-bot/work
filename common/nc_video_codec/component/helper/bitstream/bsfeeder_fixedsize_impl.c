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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "vpuapifunc.h"
#include "main_helper.h"

#define MAX_FEEDING_SIZE        0x400000        /* 4MBytes */

#ifdef SUPPORT_DEC_RINGBUFFER_PERFORMANCE
#define DEFAULT_FEEDING_SIZE    0x400000        /* 4MBytes */
#else
#define DEFAULT_FEEDING_SIZE    0x20000         /* 128KBytes */
#endif

typedef struct FeederFixedContext {
    Uint32          feedingSize;
    BOOL            eos;
} FeederFixedContext;

void* BSFeederFixedSize_Create(
    const char* path __attribute__((unused)),
    CodStd      codecId
    )
{
    FeederFixedContext *context = NULL;

    UNREFERENCED_PARAMETER(codecId);

    context = (FeederFixedContext*)osal_malloc(sizeof(FeederFixedContext));
    if (context == NULL) {
        VLOG(ERR, "%s:%d failed to allocate memory\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    context->feedingSize = DEFAULT_FEEDING_SIZE;
    context->eos         = FALSE;

    return (void*)context;
}

BOOL BSFeederFixedSize_Destroy(
    void* feeder
    )
{
    FeederFixedContext* context = (FeederFixedContext*)feeder;

    if (context == NULL) {
        VLOG(ERR, "%s:%d Null handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    osal_free(context);

    return TRUE;
}

static void free_receive_buf(DECODED_DATA* receive_buf, int32 auto_free)
{
    if(auto_free && receive_buf->ptr_decoded_buf) {
        free(receive_buf->ptr_decoded_buf);
    }
    free(receive_buf);
}

Int32 BSFeederFixedSize_Act(
    void*       feeder,
    BSChunk*    chunk
    )
{
    FeederFixedContext*  context = (FeederFixedContext*)feeder;
    Uint32          nRead = 0;
    Uint32          feedingSize;
    Uint32          size_get;
    DECODED_DATA*   receive_buf = NULL;
    int32           auto_free = 0;
    CODEC_ERR_STATE codec_state = CODEC_SUCCESS;

    if (context == NULL) {
        VLOG(ERR, "%s:%d Null handle\n", __FUNCTION__, __LINE__);
        return 0;
    }

    if (!chunk) {
        VLOG(ERR, "%s:%d Invalid parameter! 'chunk' is null\n", __FUNCTION__, __LINE__);
        return 0;
    }

    if (!chunk->data) {
        VLOG(ERR, "%s:%d Invalid parameter! 'chunk->data' is null\n", __FUNCTION__, __LINE__);
        return 0;
    }

    if (context->eos == TRUE) {
        chunk->eos = TRUE;
        return 0;
    }

    feedingSize = context->feedingSize;
    if (feedingSize == 0) {
        Uint32  KB = 1024;
        BOOL    probability10;

        srand((Uint32)time(NULL));
        feedingSize   = GetRandom(0, 0xFFFFFFFF) % MAX_FEEDING_SIZE;
        probability10 = (BOOL)((feedingSize%100) < 10);
        if (feedingSize < KB) {
            if (probability10 == FALSE)
                feedingSize *= 100;
        }
    }

    // VLOG(INFO, "[%s:%d] [before] nc_receive_buf_to_decode() ...............\n", __FUNCTION__, __LINE__);
    if((codec_state = nc_receive_buf_to_decode(&receive_buf, &auto_free)) == CODEC_SUCCESS) {
        size_get = receive_buf->size;

        if(receive_buf->end_of_file == 1) {
            nRead = 0;
        } else {
            nRead = VPU_ALIGN8(size_get);
        }

        if (chunk->size < nRead) {
            VLOG(ERR, "%s:%d chunk->size (%u) < nRead (%u)\n", __FUNCTION__, __LINE__, chunk->size, nRead);
            free_receive_buf(receive_buf, auto_free);
            return 0;
        }

        if (receive_buf) {
            if (nRead) {
                memcpy(chunk->data, receive_buf->ptr_decoded_buf, size_get);
                if (nRead > size_get) {
                    memset(&((char*)chunk->data)[size_get], 0, nRead - size_get);
                }
            }
            free_receive_buf(receive_buf, auto_free);
        }

        if (nRead == 0) {
            context->eos = TRUE;
            chunk->eos   = TRUE;
        }
    } else {
        VLOG(ERR, "nc_receive_buf_to_decode failure ... err state : %d \n", codec_state);
        return FALSE;
    }
    // VLOG(INFO, "[%s:%d] [after end] nc_receive_buf_to_decode() ...............\n", __FUNCTION__, __LINE__);

    return (Int32)nRead;
}

BOOL BSFeederFixedSize_Rewind(
    void*       feeder
    )
{
    FeederFixedContext*  context = (FeederFixedContext*)feeder;

    context->eos = FALSE;

    return TRUE;
}

void BSFeederFixedSize_SetFeedingSize(
    void*   feeder,
    Uint32  feedingSize
    )
{
    FeederFixedContext*  context = (FeederFixedContext*)feeder;
    if (feedingSize > 0) {
        context->feedingSize = feedingSize;
    }
}

