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
#ifdef PLATFORM_NON_OS
#else
#include <sys/stat.h>
#endif

#include <stdlib.h>
#include <malloc.h>

#include "nc_streamer.h"
#include <pthread.h>
#include <time.h>
#include <unistd.h>

static callback_t encoded_done_callback_ftnptr = NULL;

BitstreamReader BitstreamReader_Create(
    Uint32      type,
    char*       path __attribute__((unused)),
    EndianMode  endian,
    EncHandle*  handle
    )
{
    AbstractBitstreamReader* reader = NULL;
    static osal_file_t fp = NULL;

    reader = (AbstractBitstreamReader*)osal_malloc(sizeof(AbstractBitstreamReader));

    reader->fp      = fp;
    reader->handle  = handle;
    reader->type    = type;
    reader->endian  = endian;

    return reader;
}

void nc_register_encode_callback(callback_t cb)
{
    if (cb) {
        encoded_done_callback_ftnptr = cb;
    }
    else {
        printf("[%s:%d] ... Fail to register encoding callback function \n",__FUNCTION__,__LINE__);
    }
}

void call_encode_done_callback(Uint8* val, Uint32 size)
{
    if (encoded_done_callback_ftnptr) {
        encoded_done_callback_ftnptr(val, size);
    }
    else {
        printf("[%s:%d] ... Fail to register encoding callback function \n",__FUNCTION__,__LINE__);
    }
}

BOOL BitstreamReader_Act(
    BitstreamReader reader,
    PhysicalAddress bitstreamBuffer,        /* ringbuffer only */
    Uint32          bitstreamBufferSize,    /* ringbuffer only */
    Uint32          streamReadSize,
    Comparator      comparator
    )
{
    AbstractBitstreamReader* absReader = (AbstractBitstreamReader*)reader;
    EncHandle       *handle;
    RetCode         ret = RETCODE_SUCCESS;
    PhysicalAddress paRdPtr;
    PhysicalAddress paWrPtr;
    int             size = 0;
    Int32           loadSize = 0;
    PhysicalAddress paBsBufStart = bitstreamBuffer;                         // ringbuffer only
    PhysicalAddress paBsBufEnd   = bitstreamBuffer+bitstreamBufferSize;     // ringbuffer only
    Uint8*          buf          = NULL;
    Uint32          coreIdx;
    BOOL            success      = TRUE;

    if (reader == NULL) {
        VLOG(ERR, "%s:%d Invalid handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    if (streamReadSize == 0) {
        return TRUE;
    }
    handle      = absReader->handle;
    coreIdx     = VPU_HANDLE_CORE_INDEX(*handle);

    ret = VPU_EncGetBitstreamBuffer(*handle, &paRdPtr, &paWrPtr, &size);
    if (size > 0) {
        if (streamReadSize > 0) {
            if ((Uint32)size < streamReadSize) {
                loadSize = size;
            }
            else {
                loadSize = streamReadSize;
            }
        }
        else {
            loadSize = size;
        }

        buf = (Uint8*)osal_malloc(loadSize);
        if (buf == NULL) {
            return FALSE;
        }

        if (absReader->type == BUFFER_MODE_TYPE_RINGBUFFER) {
            if ((paRdPtr+loadSize) > paBsBufEnd) {
                Uint32   room = paBsBufEnd - paRdPtr;
                vdi_read_memory(coreIdx, paRdPtr, buf, room,  absReader->endian);
                vdi_read_memory(coreIdx, paBsBufStart, buf+room, (loadSize-room), absReader->endian);
            }
            else {
                vdi_read_memory(coreIdx, paRdPtr, buf, loadSize, absReader->endian);
            }
        }
        else {
            /* Linebuffer */
            vdi_read_memory(coreIdx, paRdPtr, buf, loadSize, absReader->endian);
        }

        call_encode_done_callback(buf, loadSize);

        if (comparator != NULL) {
            if (Comparator_Act(comparator, buf, loadSize, 0) == FALSE) {
                success = FALSE;
            }
        }
        osal_free(buf);

        //not to call VPU_EncUpdateBitstreamBuffer function if line buffer
		if ( absReader->type == BUFFER_MODE_TYPE_RINGBUFFER || ( absReader->type != BUFFER_MODE_TYPE_RINGBUFFER && absReader->streamBufFull)) {
			ret = VPU_EncUpdateBitstreamBuffer(*handle, loadSize);
			if( ret != RETCODE_SUCCESS ) {
				VLOG(ERR, "VPU_EncUpdateBitstreamBuffer failed Error code is 0x%x \n", ret );
				success = FALSE;
			}
		}
    }

    return success;
}

BOOL BitstreamReader_Destroy(
    BitstreamReader reader
    )
{
    AbstractBitstreamReader* absReader = (AbstractBitstreamReader*)reader;

    if (absReader == NULL) {
        VLOG(ERR, "%s:%d Invalid handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    osal_free(absReader);

    return TRUE;
}