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

#include <string.h>
#include "component.h"
#include "cnm_app.h"
#include "misc/debug.h"

#define EXTRA_FRAME_BUFFER_NUM 1
#define MAX_CNT_INT_TIMEOUT 2

#define DEC_DESTROY_TIME_OUT (60000*2) //2 min

/* #define DISPLAY_DEC_PROCESSING_TIME */

typedef enum {
    DEC_INT_STATUS_NONE,        // Interrupt not asserted yet
    DEC_INT_STATUS_EMPTY,       // Need more es
    DEC_INT_STATUS_DONE,        // Interrupt asserted
    DEC_INT_STATUS_TIMEOUT,     // Interrupt not asserted during given time.
} DEC_INT_STATUS;

typedef enum {
    DEC_STATE_NONE,
    DEC_STATE_OPEN_DECODER,
    DEC_STATE_INIT_SEQ,
    DEC_STATE_REGISTER_FB,
    DEC_STATE_DECODING,
    DEC_STATE_CLOSE,
} DecoderState;

typedef struct {
    TestDecConfig       testDecConfig;
    DecOpenParam        decOpenParam;
    DecParam            decParam;
    FrameBufferFormat   wtlFormat;
    DecHandle           handle;
    Uint64              startTimeout;
    Uint64              desStTimeout;
    Uint32              iterationCnt;
    vpu_buffer_t        vbUserData;
    BOOL                doFlush;
    BOOL                stateDoing;
    DecoderState        state;
    DecInitialInfo      initialInfo;
    Uint32              numDecoded;             /*!<< The number of decoded frames */
    Uint32              numOutput;
    PhysicalAddress     decodedAddr;
    Uint32              frameNumToStop;
    BOOL                doingReset;
    Uint32              cyclePerTick;
    Uint32              chromaIDCFlag;
    VpuAttr             attr;
    struct {
        BOOL    enable;
        Uint32  skipCmd;                        /*!<< a skip command to be restored */
    }                   autoErrorRecovery;
} DecoderContext;
static BOOL RegisterFrameBuffers(ComponentImpl* com)
{
    DecoderContext*         ctx               = (DecoderContext*)com->context;
    FrameBuffer*            pFrame            = NULL;
    Uint32                  framebufStride    = 0;
    ParamDecFrameBuffer     paramFb;
    RetCode                 result;
    DecInitialInfo*         codecInfo         = &ctx->initialInfo;
    BOOL                    success;
    CNMComponentParamRet    ret;
    CNMComListenerDecRegisterFb  lsnpRegisterFb;

    ctx->stateDoing = TRUE;
    ret = ComponentGetParameter(com, com->sinkPort.connectedComponent, GET_PARAM_RENDERER_FRAME_BUF, (void*)&paramFb);
    if (ComponentParamReturnTest(ret, &success) == FALSE) {
        return success;
    }

    pFrame               = paramFb.fb;
    framebufStride       = paramFb.stride;
    // VLOG(TRACE, "<%s> COMPRESSED: %d, LINEAR: %d\n", __FUNCTION__, paramFb.nonLinearNum, paramFb.linearNum);
    if ((ctx->attr.productId == PRODUCT_ID_521) && ctx->attr.supportDualCore == TRUE) {
        if (ctx->initialInfo.lumaBitdepth == 8 && ctx->initialInfo.chromaBitdepth == 8)
            result = VPU_DecRegisterFrameBufferEx(ctx->handle, pFrame, paramFb.nonLinearNum, paramFb.linearNum, framebufStride, codecInfo->picHeight, COMPRESSED_FRAME_MAP_DUAL_CORE_8BIT);
        else
            result = VPU_DecRegisterFrameBufferEx(ctx->handle, pFrame, paramFb.nonLinearNum, paramFb.linearNum, framebufStride, codecInfo->picHeight, COMPRESSED_FRAME_MAP_DUAL_CORE_10BIT);
    }
    else {
        result = VPU_DecRegisterFrameBufferEx(ctx->handle, pFrame, paramFb.nonLinearNum, paramFb.linearNum, framebufStride, codecInfo->picHeight, COMPRESSED_FRAME_MAP);
    }

    lsnpRegisterFb.handle          = ctx->handle;
    lsnpRegisterFb.numNonLinearFb  = paramFb.nonLinearNum;
    lsnpRegisterFb.numLinearFb     = paramFb.linearNum;
    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_REGISTER_FB, (void*)&lsnpRegisterFb);

    if (result != RETCODE_SUCCESS) {
        VLOG(ERR, "%s:%d Failed to VPU_DecRegisterFrameBufferEx(%d)\n", __FUNCTION__, __LINE__, result);
        ChekcAndPrintDebugInfo(ctx->handle, FALSE, result);
        return FALSE;
    }


    ctx->stateDoing = FALSE;

    return TRUE;
}

static BOOL SequenceChange(ComponentImpl* com, DecOutputInfo* outputInfo)
{
    DecoderContext* ctx               = (DecoderContext*)com->context;
    DecInitialInfo  initialInfo;
    BOOL            dpbChanged, sizeChanged, bitDepthChanged, chromaIDCChanged;
    BOOL            intReschanged = FALSE;
    Uint32          sequenceChangeFlag = outputInfo->sequenceChanged;
    ParamDecReallocFB decReallocFB;

    dpbChanged      = (sequenceChangeFlag&SEQ_CHANGE_ENABLE_DPB_COUNT) ? TRUE : FALSE;
    sizeChanged     = (sequenceChangeFlag&SEQ_CHANGE_ENABLE_SIZE)      ? TRUE : FALSE;
    bitDepthChanged = (sequenceChangeFlag&SEQ_CHANGE_ENABLE_BITDEPTH)  ? TRUE : FALSE;
    chromaIDCChanged = (sequenceChangeFlag&SEQ_CHANGE_CHROMA_FORMAT_IDC)  ? TRUE : FALSE;

    if (STD_VP9 == ctx->decOpenParam.bitstreamFormat) {
        intReschanged = (sequenceChangeFlag&SEQ_CHANGE_INTER_RES_CHANGE)  ? TRUE : FALSE;
    }

    if (dpbChanged || sizeChanged || bitDepthChanged || intReschanged || chromaIDCChanged) {

        VPU_DecGiveCommand(ctx->handle, DEC_GET_SEQ_INFO, &initialInfo);

        if (FALSE == intReschanged) {
            VLOG(INFO, "----- SEQUENCE CHANGED -----\n");
        } else {
            VLOG(INFO, "----- INTER RESOLUTION CHANGED -----\n");
            decReallocFB.indexInterFrameDecoded = outputInfo->indexInterFrameDecoded;
            decReallocFB.width = initialInfo.picWidth;
            decReallocFB.height = initialInfo.picHeight;
        }
        // Get current(changed) sequence information.
        // Flush all remaining framebuffers of previous sequence.
        VLOG(INFO, "sequenceChanged : %x\n", sequenceChangeFlag);
        VLOG(INFO, "SEQUENCE NO     : %d\n", initialInfo.sequenceNo);
        VLOG(INFO, "DPB COUNT       : %d\n", initialInfo.minFrameBufferCount);
        VLOG(INFO, "BITDEPTH        : LUMA(%d), CHROMA(%d)\n", initialInfo.lumaBitdepth, initialInfo.chromaBitdepth);
        VLOG(INFO, "SIZE            : WIDTH(%d), HEIGHT(%d)\n", initialInfo.picWidth, initialInfo.picHeight);

        if (FALSE == intReschanged) {
            ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_RENDERER_FREE_FRAMEBUFFERS, (void*)&outputInfo->frameDisplayFlag);
            VPU_DecGiveCommand(ctx->handle, DEC_RESET_FRAMEBUF_INFO, NULL);
        } else {
            ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_RENDERER_INTRES_CHANGED_FREE_FRAMEBUFFERS, (void*)&decReallocFB);
        }

        ScalerInfo sclInfo;
        memset(&sclInfo, 0, sizeof(ScalerInfo));
        if (ctx->testDecConfig.scaleDownWidth > 0 || ctx->testDecConfig.scaleDownHeight > 0) {
            sclInfo.scaleWidth  = CalcScaleDown(initialInfo.picWidth, ctx->testDecConfig.scaleDownWidth);
            sclInfo.scaleHeight = CalcScaleDown(initialInfo.picHeight, ctx->testDecConfig.scaleDownHeight);
            VLOG(INFO, "[SCALE INFO] %dx%d to %dx%d\n", initialInfo.picWidth, initialInfo.picHeight, sclInfo.scaleWidth, sclInfo.scaleHeight);
            sclInfo.enScaler    = TRUE;
            if (VPU_DecGiveCommand(ctx->handle, DEC_SET_SCALER_INFO, (void*)&sclInfo) != RETCODE_SUCCESS) {
                VLOG(ERR, "Failed to VPU_DecGiveCommand(DEC_SET_SCALER_INFO)\n");
                return FALSE;
            }
        }
        osal_memcpy((void*)&ctx->initialInfo, (void*)&initialInfo, sizeof(DecInitialInfo));
        if (FALSE == intReschanged) {
            ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_RENDERER_ALLOC_FRAMEBUFFERS, NULL);
            ctx->state = DEC_STATE_REGISTER_FB;
        } else {
            ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_RENDERER_INTRES_CHANGED_ALLOC_FRAMEBUFFERS, (void*)&decReallocFB);
        }
        VLOG(INFO, "----------------------------\n");
    }

    return TRUE;
}

static Int32 CheckChromaFormatFlag(ComponentImpl* com, DecOutputInfo* const outputInfo)
{
    DecoderContext* ctx     = (DecoderContext*)com->context;
    Int32 chromaIDCFlag     = 0;
    Uint32 sequenceChangeFlag = outputInfo->sequenceChanged;

    if (0 != sequenceChangeFlag) {
        DecInfo decInfo;
        VPU_DecGiveCommand(ctx->handle, DEC_GET_SEQ_INFO, &decInfo);
        chromaIDCFlag = decInfo.initialInfo.chromaFormatIDC;
    } else {
        chromaIDCFlag = ctx->chromaIDCFlag;
    }
    return chromaIDCFlag;
}

static BOOL CheckAndDoSequenceChange(ComponentImpl* com, DecOutputInfo* outputInfo)
{
    if (outputInfo->sequenceChanged == 0) {
        return TRUE;
    }
    else {
        return SequenceChange(com, outputInfo);
    }
}

static void ClearDpb(ComponentImpl* com, BOOL backupDpb)
{
    DecoderContext* ctx             = (DecoderContext*)com->context;
    Uint32          timeoutCount;
    Int32           intReason;
    DecOutputInfo   outputInfo;
    BOOL            pause;
    Uint32          idx;
    Uint32          flushedFbs      = 0;
    QueueStatusInfo cqInfo;
    const Uint32    flushTimeout = 100;

    if (TRUE == backupDpb) {
        pause = TRUE;
        ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_COM_PAUSE, (void*)&pause);
    }

    /* Send the renderer the signal to drop all frames.
     * VPU_DecClrDispFlag() is called in SE_PARAM_RENDERER_FLUSH.
     */
    ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_RENDERER_FLUSH, (void*)&flushedFbs);

    while (RETCODE_SUCCESS == VPU_DecGetOutputInfo(ctx->handle, &outputInfo)) {
        if (0 <= outputInfo.indexFrameDisplay) {
            flushedFbs |= outputInfo.indexFrameDisplay;
            VPU_DecClrDispFlag(ctx->handle, outputInfo.indexFrameDisplay);
            VLOG(INFO, "<%s> FLUSH DPB INDEX: %d\n", __FUNCTION__, outputInfo.indexFrameDisplay);
        }
        osal_msleep(1);
    }

    VLOG(INFO, "========== FLUSH FRAMEBUFFER & CMDs ========== \n");
    timeoutCount = 0;
    while (VPU_DecFrameBufferFlush(ctx->handle, NULL, NULL) == RETCODE_VPU_STILL_RUNNING) {
        // Clear an interrupt
        if (0 < (intReason=VPU_WaitInterruptEx(ctx->handle, VPU_WAIT_TIME_OUT_CQ))) {
            VPU_ClearInterruptEx(ctx->handle, intReason);
            VPU_DecGetOutputInfo(ctx->handle, &outputInfo);  // ignore the return value and outputinfo
            if (0 <= outputInfo.indexFrameDisplay) {
                flushedFbs |= outputInfo.indexFrameDisplay;
                VPU_DecClrDispFlag(ctx->handle, outputInfo.indexFrameDisplay);
            }
        }

        if (timeoutCount >= flushTimeout) {
            VLOG(ERR, "NO RESPONSE FROM VPU_DecFrameBufferFlush()\n");
            return;
        }
        timeoutCount++;
        osal_msleep(1);
    }

    VPU_DecGetOutputInfo(ctx->handle, &outputInfo);
    VPU_DecGiveCommand(ctx->handle, DEC_GET_QUEUE_STATUS, &cqInfo);
    VLOG(INFO, "<%s> REPORT_QUEUE(%d), INSTANCE_QUEUE(%d)\n", __FUNCTION__, cqInfo.reportQueueCount, cqInfo.instanceQueueCount);

    if (TRUE == backupDpb) {
        for (idx=0; idx<32; idx++) {
            if (flushedFbs & (1<<idx)) {
                VLOG(INFO, "SET DISPLAY FLAG : %d\n", idx);
                VPU_DecGiveCommand(ctx->handle, DEC_SET_DISPLAY_FLAG , &idx);
            }
        }
        pause = FALSE;
        ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_COM_PAUSE, (void*)&pause);
    }
}

static void ClearCpb(ComponentImpl* com)
{
    DecoderContext* ctx = (DecoderContext*)com->context;
    PhysicalAddress curRdPtr, curWrPtr;

    if (BS_MODE_INTERRUPT == ctx->decOpenParam.bitstreamMode) {
        /* Clear CPB */
        // In order to stop processing bitstream.
        VPU_DecUpdateBitstreamBuffer(ctx->handle, EXPLICIT_END_SET_FLAG);
        VPU_DecGetBitstreamBuffer(ctx->handle, &curRdPtr, &curWrPtr, NULL);
        VPU_DecSetRdPtr(ctx->handle, curWrPtr, TRUE);
        VLOG(INFO, "CLEAR CPB(RD_PTR: %08x, WR_PTR: %08x)\n", curWrPtr, curWrPtr);
    }
}

static BOOL PrepareSkip(ComponentImpl* com)
{
    DecoderContext*   ctx = (DecoderContext*)com->context;

    // Flush the decoder
    if (ctx->doFlush == TRUE) {
        ClearDpb(com, FALSE);
        if (STD_VP9 != ctx->decOpenParam.bitstreamFormat && STD_AV1 != ctx->decOpenParam.bitstreamFormat) {
            ClearCpb(com);
        }
        ctx->doFlush = FALSE;
    }

    return TRUE;
}

static DEC_INT_STATUS HandlingInterruptFlag(ComponentImpl* com)
{
    DecoderContext*      ctx               = (DecoderContext*)com->context;
    DecHandle            handle            = ctx->handle;
    Int32                interruptFlag     = 0;
    Uint32               interruptWaitTime = VPU_WAIT_TIME_OUT_CQ;
    Uint32               interruptTimeout  = VPU_DEC_TIMEOUT;
    DEC_INT_STATUS       status            = DEC_INT_STATUS_NONE;
    CNMComListenerDecInt lsn;

    if (1 < vdi_get_instance_num(VPU_HANDLE_CORE_INDEX(ctx->handle))) {
        interruptWaitTime = VPU_WAIT_TIME_OUT_LONG;//TODO : need to check in customer side
    }

#ifdef SUPPORT_GDB
    if (__VPU_BUSY_TIMEOUT == 0) interruptTimeout = 0;
#endif

    if (ctx->startTimeout == 0ULL) {
        ctx->startTimeout = osal_gettime();
    }
    do {
        interruptFlag = VPU_WaitInterruptEx(handle, interruptWaitTime);
        if (INTERRUPT_TIMEOUT_VALUE == interruptFlag) {
            Uint64   currentTimeout = osal_gettime();
            if (0 < interruptTimeout && (currentTimeout - ctx->startTimeout) > interruptTimeout) {
                VLOG(ERR, "\n INSNTANCE #%d INTERRUPT TIMEOUT.\n", handle->instIndex);
                status = DEC_INT_STATUS_TIMEOUT;
                break;
            }
            interruptFlag = 0;
        }

        if (interruptFlag < 0) {
            VLOG(ERR, "<%s:%d> interruptFlag is negative value! %08x\n", __FUNCTION__, __LINE__, interruptFlag);
        }

        if (interruptFlag > 0) {
            VPU_ClearInterruptEx(handle, interruptFlag);
            ctx->startTimeout = 0ULL;
            status = DEC_INT_STATUS_DONE;
            if (interruptFlag & (1<<INT_WAVE5_INIT_SEQ)) {
                break;
            }

            if (interruptFlag & (1<<INT_WAVE5_DEC_PIC)) {
                break;
            }

            if (interruptFlag & (1<<INT_WAVE5_BSBUF_EMPTY)) {
                status = DEC_INT_STATUS_EMPTY;
                break;
            }
        }
    } while (FALSE);

    if (interruptFlag != 0) {
        lsn.handle   = handle;
        lsn.flag     = interruptFlag;
        lsn.decIndex = ctx->numDecoded;
        ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_INTERRUPT, (void*)&lsn);
    }

    return status;
}

static BOOL DoReset(ComponentImpl* com)
{
    DecoderContext* ctx = (DecoderContext*)com->context;
    DecOutputInfo outputInfo;
    Uint32 ret;
    BOOL pause  = TRUE;
    PhysicalAddress curRdPtr, curWrPtr;
    Uint32 timeout = VPU_DEC_TIMEOUT;
    Uint64 currentTimeout = 0;

#ifdef SUPPORT_GDB
    extern Uint32 __VPU_BUSY_TIMEOUT;
    if (__VPU_BUSY_TIMEOUT == 0) return TRUE;
#endif /* SUPPORT_GDB */

    VLOG(INFO, "========== %s ==========\n", __FUNCTION__);

    ComponentSetParameter(com, com->srcPort.connectedComponent, SET_PARAM_COM_PAUSE, (void*)&pause);
    ComponentSetParameter(com, com->srcPort.connectedComponent, SET_PARAM_FEEDER_RESET, (void*)&pause);

    VPU_DecUpdateBitstreamBuffer(ctx->handle, EXPLICIT_END_SET_FLAG);

    //ClearDpb(com, FALSE);

    while (RETCODE_SUCCESS == VPU_DecGetOutputInfo(ctx->handle, &outputInfo)) {
        if (0 <= outputInfo.indexFrameDisplay) {
            VPU_DecClrDispFlag(ctx->handle, outputInfo.indexFrameDisplay);
            VLOG(INFO, "<%s> FLUSH DPB INDEX: %d\n", __FUNCTION__, outputInfo.indexFrameDisplay);
        }
        osal_msleep(1);
    }

    VLOG(INFO, "> Reset VPU\n");

    if (0ULL == ctx->startTimeout) {
        ctx->startTimeout = osal_gettime();
    }

    ret = VPU_SWReset(ctx->handle->coreIdx, SW_RESET_SAFETY, ctx->handle);
    currentTimeout = osal_gettime();
    if (RETCODE_SUCCESS != ret) {
        if (RETCODE_VPU_STILL_RUNNING == ret) {
            if ((currentTimeout - ctx->startTimeout) > timeout) {
                VLOG(ERR, "\n INSNTANCE #%d VPU SWRest TIMEOUT.\n", ctx->handle->instIndex);
                return FALSE;
            }
            return TRUE;
        } else {
            VLOG(ERR, "<%s:%d> Failed to VPU_SWReset() ret(%d)\n", __FUNCTION__, __LINE__, ret);
            return FALSE;
        }
    }

    //ClearCpb(com);
    if (BS_MODE_INTERRUPT == ctx->decOpenParam.bitstreamMode) {
        VPU_DecGetBitstreamBuffer(ctx->handle, &curRdPtr, &curWrPtr, NULL);
        VPU_DecSetRdPtr(ctx->handle, curWrPtr, TRUE);
    }

    VLOG(INFO, "========== %s ==========\n", __FUNCTION__);

    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_RESET_DONE, NULL);

    VLOG(INFO, "> FLUSH INPUT PORT\n");
    ComponentPortFlush(com);

    pause = FALSE;
    ComponentSetParameter(com, com->srcPort.connectedComponent, SET_PARAM_COM_PAUSE, (void*)&pause);

    ctx->doingReset                     = FALSE;
    ctx->startTimeout               = 0ULL;
    ctx->autoErrorRecovery.enable   = TRUE;
    ctx->autoErrorRecovery.skipCmd  = ctx->decParam.skipframeMode;
    ctx->decParam.skipframeMode     = WAVE_SKIPMODE_NON_IRAP;

    VLOG(INFO, "========== %s Finished==========\n", __FUNCTION__);

    return TRUE;
}




static BOOL Decode(ComponentImpl* com, PortContainerES* in, PortContainerDisplay* out)
{
    DecoderContext*                 ctx           = (DecoderContext*)com->context;
    DecOutputInfo                   decOutputInfo;
    DEC_INT_STATUS                  intStatus;
    CNMComListenerStartDecOneFrame  lsnpPicStart;
    BitStreamMode                   bsMode        = ctx->decOpenParam.bitstreamMode;
    RetCode                         result;
    CNMComListenerDecDone           lsnpPicDone;
    CNMComListenerDecReadyOneFrame  lsnpReadyOneFrame = {0,};
    BOOL                            doDecode;
    QueueStatusInfo                 qStatus;

    lsnpReadyOneFrame.handle = ctx->handle;
    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_READY_ONE_FRAME, (void*)&lsnpReadyOneFrame);

    ctx->stateDoing = TRUE;

    if (PrepareSkip(com) == FALSE) {
        return FALSE;
    }

    /* decode a frame except when the bitstream mode is PIC_END and no data */
    doDecode  = !(bsMode == BS_MODE_PIC_END && in == NULL);
    doDecode &= (BOOL)(com->pause == FALSE);

#ifdef SUPPORT_DEC_RINGBUFFER_PERFORMANCE
    if (TRUE == com->pause && TRUE == ctx->testDecConfig.performance && BS_MODE_INTERRUPT == ctx->testDecConfig.bitstreamMode) {
        return TRUE;
    }
#endif

    VPU_DecGiveCommand(ctx->handle, DEC_GET_QUEUE_STATUS, &qStatus);
    if (COMMAND_QUEUE_DEPTH == qStatus.instanceQueueCount) {
        doDecode = FALSE;
    }

    if (TRUE == doDecode) {

        result = VPU_DecStartOneFrame(ctx->handle, &ctx->decParam);

        lsnpPicStart.result   = result;
        lsnpPicStart.decParam = ctx->decParam;
        ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_START_ONE_FRAME, (void*)&lsnpPicStart);

        if (result == RETCODE_SUCCESS) {
        }
        else if (result == RETCODE_QUEUEING_FAILURE) {
            // Just retry
            if (in) in->reuse = (bsMode == BS_MODE_PIC_END);
        }
        else if (result == RETCODE_VPU_RESPONSE_TIMEOUT) {
            VLOG(ERR, "<%s:%d> Failed to VPU_DecStartOneFrame() ret(%d)\n", __FUNCTION__, __LINE__, result);
            CNMErrorSet(CNM_ERROR_HANGUP);
            HandleDecoderError(ctx->handle, ctx->numDecoded, NULL);
            return FALSE;
        }
        else {
            ChekcAndPrintDebugInfo(ctx->handle, FALSE, result);
            return FALSE;
        }
    }
    else {
        if (in) in->reuse = (bsMode == BS_MODE_PIC_END);
    }


    intStatus=HandlingInterruptFlag(com);


    switch (intStatus) {
    case DEC_INT_STATUS_TIMEOUT:
        ChekcAndPrintDebugInfo(ctx->handle, FALSE, RETCODE_VPU_RESPONSE_TIMEOUT);
        DoReset(com);
        return (TRUE == ctx->testDecConfig.ignoreHangup) ? TRUE : FALSE;
    case DEC_INT_STATUS_EMPTY:
    case DEC_INT_STATUS_NONE:
        return TRUE;
    default:
        break;
    }

    // Get data from the sink component.
    if ((result=VPU_DecGetOutputInfo(ctx->handle, &decOutputInfo)) == RETCODE_SUCCESS) {
        #ifdef DISPLAY_DEC_PROCESSING_TIME
            DisplayDecodedInformation(ctx->handle, ctx->decOpenParam.bitstreamFormat, ctx->numDecoded, &decOutputInfo, ctx->testDecConfig.performance, ctx->cyclePerTick);
        #endif
    }

    lsnpPicDone.handle     = ctx->handle;
    lsnpPicDone.ret        = result;
    lsnpPicDone.decParam   = &ctx->decParam;
    lsnpPicDone.output     = &decOutputInfo;
    lsnpPicDone.numDecoded = ctx->numDecoded;
    lsnpPicDone.vbUser     = ctx->vbUserData;
    lsnpPicDone.bitstreamFormat = ctx->decOpenParam.bitstreamFormat;
    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_GET_OUTPUT_INFO, (void*)&lsnpPicDone);

    if (result == RETCODE_REPORT_NOT_READY) {
        return TRUE; // Not decoded yet. Try again
    }
    else if (result != RETCODE_SUCCESS) {
        /* ERROR */
        VLOG(ERR, "Failed to decode error\n");
        ChekcAndPrintDebugInfo(ctx->handle, FALSE, result);
        return FALSE;
    }

    if ((decOutputInfo.decodingSuccess & 0x01) == 0) {
        VLOG(ERR, "VPU_DecGetOutputInfo decode fail framdIdx %d error(0x%08x) reason(0x%08x), reasonExt(0x%08x)\n",
            ctx->numDecoded, decOutputInfo.decodingSuccess, decOutputInfo.errorReason, decOutputInfo.errorReasonExt);
        if (WAVE5_SYSERR_WATCHDOG_TIMEOUT == decOutputInfo.errorReason) {
            VLOG(ERR, "WAVE5_SYSERR_WATCHDOG_TIMEOUT\n");
        }
        else if (WAVE5_SYSERR_VLC_BUF_FULL == decOutputInfo.errorReason) {
            VLOG(ERR, "VLC_BUFFER FULL\n");
        }
        else if (HEVC_SPECERR_OVER_PICTURE_WIDTH_SIZE == decOutputInfo.errorReason || HEVC_SPECERR_OVER_PICTURE_HEIGHT_SIZE == decOutputInfo.errorReason) {
            VLOG(ERR, "Not supported Width or Height(%dx%d)\n", decOutputInfo.decPicWidth, decOutputInfo.decPicHeight);
        }
    }
    else {
        if (TRUE == ctx->autoErrorRecovery.enable) {
            ctx->decParam.skipframeMode     = ctx->autoErrorRecovery.skipCmd;
            ctx->autoErrorRecovery.enable   = FALSE;
            ctx->autoErrorRecovery.skipCmd  = WAVE_SKIPMODE_WAVE_NONE;
        }
    }

    if (CheckAndDoSequenceChange(com, &decOutputInfo) == FALSE) {
        return FALSE;
    }


    if (decOutputInfo.indexFrameDecoded >= 0 || decOutputInfo.indexFrameDecoded == DECODED_IDX_FLAG_SKIP) {
        ctx->numDecoded++;
        ctx->decodedAddr = decOutputInfo.bytePosFrameStart;
        // Return a used data to a source port.
        out->reuse = FALSE;
    }

    if ((decOutputInfo.indexFrameDisplay >= 0) || (decOutputInfo.indexFrameDisplay == DISPLAY_IDX_FLAG_SEQ_END)) {
        ctx->numOutput++;
        out->last  = (BOOL)(decOutputInfo.indexFrameDisplay == DISPLAY_IDX_FLAG_SEQ_END);
        out->reuse = FALSE;
        out->chromaIDCFlag = CheckChromaFormatFlag(com, &decOutputInfo);
    }
    if (decOutputInfo.indexFrameDisplay == DISPLAY_IDX_FLAG_SEQ_END) {
        ctx->stateDoing = FALSE;
        com->terminate  = TRUE;
    }
    if (out->reuse == FALSE) {
        osal_memcpy((void*)&out->decInfo, (void*)&decOutputInfo, sizeof(DecOutputInfo));
    }

    if (ctx->frameNumToStop > 0) {
        if (ctx->frameNumToStop == ctx->numOutput) {
            com->terminate = TRUE;
        }
    }

    return TRUE;
}

static CNMComponentParamRet GetParameterDecoder(ComponentImpl* from __attribute__((unused)), ComponentImpl* com, GetParameterCMD commandType, void* data)
{
    DecoderContext*             ctx     = (DecoderContext*)com->context;
    BOOL                        result  = TRUE;
    PhysicalAddress             rdPtr, wrPtr;
    Uint32                      room;
    ParamDecBitstreamBufPos*    bsPos   = NULL;
    ParamDecNeedFrameBufferNum* fbNum;
    ParamVpuStatus*             status;
    QueueStatusInfo             cqInfo;
    PortContainerES*            container;
    vpu_buffer_t                vb;

    if (ctx->handle == NULL)  return CNM_COMPONENT_PARAM_NOT_READY;
    if (ctx->doingReset  == TRUE) return CNM_COMPONENT_PARAM_NOT_READY;

    switch(commandType) {
    case GET_PARAM_COM_IS_CONTAINER_CONUSUMED:
        // This query command is sent from the comonponent core.
        // If input data are consumed in sequence, it should return TRUE through PortContainer::consumed.
        container = (PortContainerES*)data;
        vb = container->buf;
        if (vb.phys_addr <= ctx->decodedAddr && ctx->decodedAddr < (vb.phys_addr+vb.size)) {
            container->consumed = TRUE;
            ctx->decodedAddr = 0;
        }
        break;
    case GET_PARAM_DEC_HANDLE:
        *(DecHandle*)data = ctx->handle;
        break;
    case GET_PARAM_DEC_FRAME_BUF_NUM:
        if (ctx->state <= DEC_STATE_INIT_SEQ) return CNM_COMPONENT_PARAM_NOT_READY;
        fbNum = (ParamDecNeedFrameBufferNum*)data;
        fbNum->nonLinearNum = ctx->initialInfo.minFrameBufferCount + EXTRA_FRAME_BUFFER_NUM;   // max_dec_pic_buffering
        if (ctx->decOpenParam.wtlEnable == TRUE) {
            fbNum->linearNum = (ctx->initialInfo.frameBufDelay+1) + EXTRA_FRAME_BUFFER_NUM;     // The frameBufDelay can be zero.
            if ((ctx->decOpenParam.bitstreamFormat == STD_VP9) || (ctx->decOpenParam.bitstreamFormat == STD_AVS2) || (ctx->decOpenParam.bitstreamFormat == STD_AV1)) {
                fbNum->linearNum = fbNum->nonLinearNum;
            }
            if (ctx->testDecConfig.performance == TRUE) {
                if ((ctx->decOpenParam.bitstreamFormat == STD_VP9) || (ctx->decOpenParam.bitstreamFormat == STD_AVS2)) {
                    fbNum->linearNum++;
                    fbNum->nonLinearNum++;
                } else if (ctx->decOpenParam.bitstreamFormat == STD_AV1) {
                    fbNum->linearNum++;
                    fbNum->nonLinearNum++;
                }
                else {
                    fbNum->linearNum += 3;
                }
            }

        }
        else {
            fbNum->linearNum = 0;
        }
        if (TRUE == ctx->testDecConfig.thumbnailMode) {
            fbNum->linearNum    = (TRUE == ctx->decOpenParam.wtlEnable) ? 1 : 0;
            fbNum->nonLinearNum = 1;
        }
        break;
    case GET_PARAM_DEC_BITSTREAM_BUF_POS:
        if (ctx->state < DEC_STATE_INIT_SEQ) return CNM_COMPONENT_PARAM_NOT_READY;
        VPU_DecGetBitstreamBuffer(ctx->handle, &rdPtr, &wrPtr, &room);
        bsPos = (ParamDecBitstreamBufPos*)data;
        bsPos->rdPtr = rdPtr;
        bsPos->wrPtr = wrPtr;
        bsPos->avail = room;
        break;
    case GET_PARAM_DEC_CODEC_INFO:
        if (ctx->state <= DEC_STATE_INIT_SEQ) return CNM_COMPONENT_PARAM_NOT_READY;
        VPU_DecGiveCommand(ctx->handle, DEC_GET_SEQ_INFO, data);
        break;
    case GET_PARAM_VPU_STATUS:
        if (ctx->state != DEC_STATE_DECODING) return CNM_COMPONENT_PARAM_NOT_READY;
        VPU_DecGiveCommand(ctx->handle, DEC_GET_QUEUE_STATUS, &cqInfo);
        status = (ParamVpuStatus*)data;
        status->cq = cqInfo;
        break;
    default:
        result = FALSE;
        break;
    }

    return (result == TRUE) ? CNM_COMPONENT_PARAM_SUCCESS : CNM_COMPONENT_PARAM_FAILURE;
}

static CNMComponentParamRet SetParameterDecoder(ComponentImpl* from __attribute__((unused)), ComponentImpl* com, SetParameterCMD commandType, void* data)
{
    BOOL                result  = TRUE;
    DecoderContext*     ctx     = (DecoderContext*)com->context;
    Int32               skipCmd;
    ParamDecTargetTid*  tid     = NULL;

    switch(commandType) {
    case SET_PARAM_COM_PAUSE:
        com->pause   = *(BOOL*)data;
        break;
    case SET_PARAM_DEC_SKIP_COMMAND:
        skipCmd = *(Int32*)data;
        ctx->decParam.skipframeMode = skipCmd;
        if (skipCmd == WAVE_SKIPMODE_NON_IRAP) {
            Uint32 userDataMask = (1<<H265_USERDATA_FLAG_RECOVERY_POINT);
            ctx->decParam.craAsBlaFlag = TRUE;
            /* For finding recovery point */
            VPU_DecGiveCommand(ctx->handle, ENABLE_REP_USERDATA, &userDataMask);
        }
        else {
            ctx->decParam.craAsBlaFlag = FALSE;
        }
        if (ctx->numDecoded > 0) {
            ctx->doFlush = (BOOL)(ctx->decParam.skipframeMode == WAVE_SKIPMODE_NON_IRAP);
        }
        break;
    case SET_PARAM_DEC_RESET:
        ctx->doingReset  = TRUE;
        break;
    case SET_PARAM_DEC_TARGET_TID:
        tid = (ParamDecTargetTid*)data;
        /*! NOTE: DO NOT CHANGE THE ORDER OF COMMANDS BELOW. */
        VPU_DecGiveCommand(ctx->handle, DEC_SET_TEMPORAL_ID_MODE,   (void*)&tid->tidMode);
        VPU_DecGiveCommand(ctx->handle, DEC_SET_TARGET_TEMPORAL_ID, (void*)&tid->targetTid);
        break;
    case SET_PARAM_DEC_FLUSH:
        ClearDpb(com, TRUE);
        break;
    default:
        result = FALSE;
        break;
    }

    return (result == TRUE) ? CNM_COMPONENT_PARAM_SUCCESS : CNM_COMPONENT_PARAM_FAILURE;
}

static BOOL UpdateBitstream(ComponentImpl* com __attribute__((unused)), DecoderContext* ctx, PortContainerES* in)
{
    RetCode       ret    = RETCODE_SUCCESS;
    BitStreamMode bsMode = ctx->decOpenParam.bitstreamMode;
    BOOL          update = TRUE;
    Uint32        updateSize;

    if (in == NULL) return TRUE;

    if (bsMode == BS_MODE_PIC_END) {
        VPU_DecSetRdPtr(ctx->handle, in->buf.phys_addr, TRUE);
    }
    else {
        if (in->size > 0) {
            PhysicalAddress rdPtr, wrPtr;
            Uint32          room;
            VPU_DecGetBitstreamBuffer(ctx->handle, &rdPtr, &wrPtr, &room);
            if (room < in->size) {
                in->reuse = TRUE;
                return TRUE;
            }
        }
    }

    if (in->last == TRUE) {
        updateSize = (in->size == 0) ? STREAM_END_SET_FLAG : in->size;
    }
    else {
        updateSize = in->size;
        update     = in->size > 0;
    }


    if (update == TRUE) {
        if ((ret=VPU_DecUpdateBitstreamBuffer(ctx->handle, updateSize)) != RETCODE_SUCCESS) {
            VLOG(ERR, "<%s:%d> Failed to VPU_DecUpdateBitstreamBuffer() ret(%d)\n", __FUNCTION__, __LINE__, ret);
            ChekcAndPrintDebugInfo(ctx->handle, FALSE, ret);
            return FALSE;
        }
        if (in->last == TRUE && updateSize != STREAM_END_SET_FLAG) {
            VPU_DecUpdateBitstreamBuffer(ctx->handle, STREAM_END_SET_FLAG);
        }
    }

    in->reuse = FALSE;

    return TRUE;
}

static BOOL OpenDecoder(ComponentImpl* com)
{
    DecoderContext*         ctx     = (DecoderContext*)com->context;
    ParamDecBitstreamBuffer bsBuf;
    CNMComponentParamRet    ret;
    CNMComListenerDecOpen   lspn;

    memset(&lspn, 0, sizeof(CNMComListenerDecOpen));

    BOOL                    success = FALSE;
    BitStreamMode           bsMode  = ctx->decOpenParam.bitstreamMode;
    vpu_buffer_t            vbUserData;
    RetCode                 retCode;

    ctx->stateDoing = TRUE;
    ret = ComponentGetParameter(com, com->srcPort.connectedComponent, GET_PARAM_FEEDER_BITSTREAM_BUF, &bsBuf);
    if (ComponentParamReturnTest(ret, &success) == FALSE) {
        return success;
    }

    ctx->decOpenParam.bitstreamBuffer     = (bsMode == BS_MODE_PIC_END) ? 0 : bsBuf.bs->phys_addr;
    ctx->decOpenParam.bitstreamBufferSize = (bsMode == BS_MODE_PIC_END) ? 0 : bsBuf.bs->size;
    retCode = VPU_DecOpen(&ctx->handle, &ctx->decOpenParam);

    lspn.handle = ctx->handle;
    lspn.ret    = retCode;
    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_OPEN, (void*)&lspn);

    if (retCode != RETCODE_SUCCESS) {
        VLOG(ERR, "<%s:%d> Failed to VPU_DecOpen(ret:%d)\n", __FUNCTION__, __LINE__, retCode);
        if ( retCode == RETCODE_VPU_RESPONSE_TIMEOUT)
            CNMErrorSet(CNM_ERROR_HANGUP);

        return FALSE;
    }
    // VPU_DecGiveCommand(ctx->handle, ENABLE_LOGGING, 0); // for debug : print all sw register of VPU

    if (ctx->vbUserData.size == 0) {
        vbUserData.size = (1320*1024);  /* 40KB * (queue_depth + report_queue_depth+1) = 40KB * (16 + 16 +1) */
        vdi_allocate_dma_memory(ctx->testDecConfig.coreIdx, &vbUserData, DEC_ETC, ctx->handle->instIndex);
    }
    VPU_DecGiveCommand(ctx->handle, SET_ADDR_REP_USERDATA, (void*)&vbUserData.phys_addr);
    VPU_DecGiveCommand(ctx->handle, SET_SIZE_REP_USERDATA, (void*)&vbUserData.size);
    VPU_DecGiveCommand(ctx->handle, ENABLE_REP_USERDATA,   (void*)&ctx->testDecConfig.enableUserData);

    VPU_DecGiveCommand(ctx->handle, SET_CYCLE_PER_TICK,   (void*)&ctx->cyclePerTick);
    if (ctx->testDecConfig.thumbnailMode == TRUE) {
        VPU_DecGiveCommand(ctx->handle, ENABLE_DEC_THUMBNAIL_MODE, NULL);
    }

    ctx->vbUserData = vbUserData;
    ctx->stateDoing = FALSE;


    return TRUE;
}

static BOOL DecodeHeader(ComponentImpl* com, PortContainerES* in, BOOL* done)
{
    DecoderContext*                ctx     = (DecoderContext*)com->context;
    DecHandle                      handle  = ctx->handle;
    Uint32                         coreIdx = ctx->testDecConfig.coreIdx;
    RetCode                        ret     = RETCODE_SUCCESS;
    DEC_INT_STATUS                 status;
    DecInitialInfo*                initialInfo = &ctx->initialInfo;
    SecAxiUse                      secAxiUse;
    CNMComListenerDecCompleteSeq   lsnpCompleteSeq;

    *done = FALSE;

    if (BS_MODE_PIC_END == ctx->decOpenParam.bitstreamMode && NULL == in) {
        return TRUE;
    }

    if (ctx->stateDoing == FALSE) {
        /* previous state done */
        ret = VPU_DecIssueSeqInit(handle);
        if (RETCODE_QUEUEING_FAILURE == ret) {
            return TRUE; // Try again
        }
        ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_ISSUE_SEQ, NULL);
        if (ret != RETCODE_SUCCESS) {
            ChekcAndPrintDebugInfo(ctx->handle, FALSE, ret);
            VLOG(ERR, "%s:%d Failed to VPU_DecIssueSeqInit() ret(%d)\n", __FUNCTION__, __LINE__, ret);
            return FALSE;
        }
    }

    ctx->stateDoing = TRUE;

    while (com->terminate == FALSE) {
        if ((status=HandlingInterruptFlag(com)) == DEC_INT_STATUS_DONE) {
            break;
        }
        else if (status == DEC_INT_STATUS_TIMEOUT) {
            HandleDecoderError(ctx->handle, 0, NULL);
            VPU_DecUpdateBitstreamBuffer(handle, STREAM_END_SIZE);    /* To finish bitstream empty status */
            VPU_SWReset(coreIdx, SW_RESET_SAFETY, handle);
            VPU_DecUpdateBitstreamBuffer(handle, STREAM_END_CLEAR_FLAG);    /* To finish bitstream empty status */
            return FALSE;
        }
        else if (status == DEC_INT_STATUS_EMPTY) {
            return TRUE;
        }
        else if (status == DEC_INT_STATUS_NONE) {
            return TRUE;
        }
        else {
            VLOG(INFO, "%s:%d Unknown interrupt status: %d\n", __FUNCTION__, __LINE__, status);
            return FALSE;
        }
    }

    ret = VPU_DecCompleteSeqInit(handle, initialInfo);
    if ((ctx->attr.productId == PRODUCT_ID_521) && ctx->attr.supportDualCore == TRUE) {
        if ( initialInfo->lumaBitdepth == 8 && initialInfo->chromaBitdepth == 8) {
            ctx->testDecConfig.mapType = COMPRESSED_FRAME_MAP_DUAL_CORE_8BIT;
        }
        else {
            ctx->testDecConfig.mapType = COMPRESSED_FRAME_MAP_DUAL_CORE_10BIT;
        }
    }

    // VPU_EncGiveCommand(ctx->handle, DISABLE_LOGGING, 0); // for debug : stop print all sw register of VPU

    strcpy(lsnpCompleteSeq.refYuvPath, ctx->testDecConfig.refYuvPath);
    lsnpCompleteSeq.ret             = ret;
    lsnpCompleteSeq.initialInfo     = initialInfo;
    lsnpCompleteSeq.wtlFormat       = ctx->wtlFormat;
    lsnpCompleteSeq.cbcrInterleave  = ctx->decOpenParam.cbcrInterleave;
    lsnpCompleteSeq.bitstreamFormat = ctx->decOpenParam.bitstreamFormat;
    ctx->chromaIDCFlag              = initialInfo->chromaFormatIDC;
    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_COMPLETE_SEQ, (void*)&lsnpCompleteSeq);

    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "%s:%d FAILED TO DEC_PIC_HDR: ret(%d), SEQERR(%08x)\n", __FUNCTION__, __LINE__, ret, initialInfo->seqInitErrReason);
        ChekcAndPrintDebugInfo(ctx->handle, FALSE, ret);
        return FALSE;
    }

    if (TRUE == ctx->decOpenParam.wtlEnable) {
        VPU_DecGiveCommand(ctx->handle, DEC_SET_WTL_FRAME_FORMAT, &ctx->wtlFormat);
    }

   /* Set up the secondary AXI is depending on H/W configuration.
    * Note that turn off all the secondary AXI configuration
    * if target ASIC has no memory only for IP, LF and BIT.
    */
    secAxiUse.u.wave.useIpEnable    = (ctx->testDecConfig.secondaryAXI&0x01) ? TRUE : FALSE;
    secAxiUse.u.wave.useLfRowEnable = (ctx->testDecConfig.secondaryAXI&0x02) ? TRUE : FALSE;
    secAxiUse.u.wave.useBitEnable   = (ctx->testDecConfig.secondaryAXI&0x04) ? TRUE : FALSE;
    secAxiUse.u.wave.useSclEnable   = (ctx->testDecConfig.secondaryAXI&0x08) ? TRUE : FALSE;
    VPU_DecGiveCommand(ctx->handle, SET_SEC_AXI, &secAxiUse);
    // Set up scale
    if (ctx->testDecConfig.scaleDownWidth > 0 || ctx->testDecConfig.scaleDownHeight > 0) {
        ScalerInfo sclInfo;
        memset(&sclInfo, 0, sizeof(ScalerInfo));

        sclInfo.scaleWidth  = ctx->testDecConfig.scaleDownWidth;
        sclInfo.scaleHeight = ctx->testDecConfig.scaleDownHeight;
        VLOG(INFO, "[SCALE INFO] %dx%d\n", sclInfo.scaleWidth, sclInfo.scaleHeight);
        sclInfo.enScaler    = TRUE;
        if (VPU_DecGiveCommand(ctx->handle, DEC_SET_SCALER_INFO, (void*)&sclInfo) != RETCODE_SUCCESS) {
            VLOG(ERR, "Failed to VPU_DecGiveCommand(DEC_SET_SCALER_INFO)\n");
            return FALSE;
        }
    }

    ctx->stateDoing = FALSE;
    *done = TRUE;


    return TRUE;
}

static BOOL ExecuteDecoder(ComponentImpl* com, PortContainer* in , PortContainer* out)
{
    DecoderContext* ctx    = (DecoderContext*)com->context;
    BOOL            ret    = FALSE;
    BitStreamMode   bsMode = ctx->decOpenParam.bitstreamMode;
    BOOL            done   = FALSE;

    if (in)  in->reuse = TRUE;
    if (out) out->reuse = TRUE;
    if (ctx->state == DEC_STATE_INIT_SEQ || ctx->state == DEC_STATE_DECODING) {
        if (UpdateBitstream(com, ctx, (PortContainerES*)in) == FALSE) {
            return FALSE;
        }
        if (in) {
            // In ring-buffer mode, it has to return back a container immediately.
            if (bsMode == BS_MODE_PIC_END) {
                if (ctx->state == DEC_STATE_INIT_SEQ) {
                    in->reuse = TRUE;
                }
                in->consumed = FALSE;
            }
            else {
                in->consumed = (in->reuse == FALSE);
            }
        }
    }

    if (ctx->doingReset  == TRUE) {
        if (in) in->reuse = FALSE;
        ctx->startTimeout = 0ULL;
        return DoReset(com);
    }
    switch (ctx->state) {
    case DEC_STATE_OPEN_DECODER:
        ret = OpenDecoder(com);
        if (ctx->stateDoing == FALSE) ctx->state = DEC_STATE_INIT_SEQ;
        break;
    case DEC_STATE_INIT_SEQ:
        ret = DecodeHeader(com, (PortContainerES*)in, &done);
        if (TRUE == done) ctx->state = DEC_STATE_REGISTER_FB;
        break;
    case DEC_STATE_REGISTER_FB:
        ret = RegisterFrameBuffers(com);
        if (ctx->stateDoing == FALSE) {
            ctx->state = DEC_STATE_DECODING;
            #ifdef DISPLAY_DEC_PROCESSING_TIME
                DisplayDecodedInformation(ctx->handle, ctx->decOpenParam.bitstreamFormat, 0, NULL, ctx->testDecConfig.performance, 0);
            #endif
        }
        break;
    case DEC_STATE_DECODING:
        ret = Decode(com, (PortContainerES*)in, (PortContainerDisplay*)out);
        break;
    default:
        ret = FALSE;
        break;
    }

    if (ret == FALSE || com->terminate == TRUE) {
        ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_DECODED_ALL, (void*)ctx->handle);
        if (out) {
            out->reuse = FALSE;
            out->last  = TRUE;
        }
    }

    return ret;
}

static BOOL PrepareDecoder(ComponentImpl* com __attribute__((unused)), BOOL* done)
{
    *done = TRUE;

    return TRUE;
}

static void ReleaseDecoder(ComponentImpl* com)
{
    DecoderContext* ctx         = (DecoderContext*)com->context;
    com->stateDoing = FALSE;
    ctx->iterationCnt = 0;
}

static BOOL DestroyDecoder(ComponentImpl* com)
{
    DecoderContext* ctx         = (DecoderContext*)com->context;
    DEC_INT_STATUS  intStatus;
    BOOL            success     = TRUE;
    Uint32          i           = 0;
    Uint64          currentTime = 0;
    Uint32          timeout = DEC_DESTROY_TIME_OUT;
    RetCode         ret;

    if (NULL != ctx->handle) {
        if (FALSE == com->stateDoing) {
            ctx->desStTimeout = osal_gettime();
            if (RETCODE_SUCCESS != (ret=VPU_DecUpdateBitstreamBuffer(ctx->handle, STREAM_END_SET_FLAG))) {
                VLOG(WARN, "%s:%d Failed to VPU_DecUpdateBitstreamBuffer, ret(%08x) \n",__FUNCTION__, __LINE__, ret);
            }

            com->stateDoing = TRUE;
        }

        while (VPU_DecClose(ctx->handle) == RETCODE_VPU_STILL_RUNNING) {
            if ((intStatus=HandlingInterruptFlag(com)) == DEC_INT_STATUS_TIMEOUT) {
                HandleDecoderError(ctx->handle, ctx->numDecoded, NULL);
                VLOG(ERR, "<%s:%d> NO RESPONSE FROM VPU_DecClose()\n", __FUNCTION__, __LINE__);
                com->stateDoing = FALSE;
                success = FALSE;
                break;
            }
            else if (intStatus == DEC_INT_STATUS_DONE) {
                DecOutputInfo outputInfo;
                // VLOG(INFO, "VPU_DecClose() : CLEAR REMAIN INTERRUPT\n");
                VPU_DecGetOutputInfo(ctx->handle, &outputInfo);
                continue;
            }

            for (i=0; i<MAX_REG_FRAME; i++) {
                VPU_DecClrDispFlag(ctx->handle, i);
            }

            currentTime = osal_gettime();
            if ( (currentTime - ctx->desStTimeout) > timeout) {
                VLOG(ERR, "\n INSNTANCE #%d VPU Close TIMEOUT.\n", ctx->handle->instIndex);
                com->stateDoing = FALSE;
                success = FALSE;
                break;
            }
            osal_msleep(10);
        }
        ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_CLOSE, NULL);
    }

    ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_RENDERER_RELEASE_FRAME_BUFFRES, NULL);

    if (ctx->vbUserData.size) {
        vdi_free_dma_memory(ctx->testDecConfig.coreIdx, &ctx->vbUserData, DEC_ETC, ctx->handle->instIndex);
    }

    VPU_DeInit(ctx->decOpenParam.coreIdx);

    osal_free(ctx);

    com->stateDoing = FALSE;

    return success;
}

static Component CreateDecoder(ComponentImpl* com, CNMComponentConfig* componentParam)
{
    DecoderContext* ctx;
    Uint32          coreIdx      = componentParam->testDecConfig.coreIdx;
    Uint16*         firmware     = (Uint16*)componentParam->bitcode;
    Uint32          firmwareSize = componentParam->sizeOfBitcode;
    RetCode         retCode;

    retCode = VPU_InitWithBitcode(coreIdx, firmware, firmwareSize);
    if (retCode != RETCODE_SUCCESS && retCode != RETCODE_CALLED_BEFORE) {
        VLOG(ERR, "%s:%d Failed to VPU_InitiWithBitcode, ret(%08x)\n", __FUNCTION__, __LINE__, retCode);
        return FALSE;
    }

    com->context = (DecoderContext*)osal_malloc(sizeof(DecoderContext));
    osal_memset(com->context, 0, sizeof(DecoderContext));
    ctx = (DecoderContext*)com->context;

    retCode = PrintVpuProductInfo(coreIdx, &ctx->attr);
    if (retCode == RETCODE_VPU_RESPONSE_TIMEOUT ) {
        CNMErrorSet(CNM_ERROR_HANGUP);
        VLOG(ERR, "<%s:%d> Failed to PrintVpuProductInfo()\n", __FUNCTION__, __LINE__);
        HandleDecoderError(ctx->handle, ctx->numDecoded, NULL);
        return FALSE;
    }
    ctx->cyclePerTick = 32768;
    if (TRUE == ctx->attr.supportNewTimer)
        ctx->cyclePerTick = 256;

    memcpy(&(ctx->decOpenParam), &(componentParam->decOpenParam), sizeof(DecOpenParam));

    ctx->wtlFormat                    = componentParam->testDecConfig.wtlFormat;
    ctx->frameNumToStop               = componentParam->testDecConfig.forceOutNum;
    ctx->testDecConfig                = componentParam->testDecConfig;
    ctx->state                        = DEC_STATE_OPEN_DECODER;
    ctx->stateDoing                   = FALSE;
    // VLOG(INFO, "PRODUCT ID: %d\n", ctx->attr.productId);

    return (Component)com;
}

ComponentImpl waveDecoderComponentImpl = {
    "wave_decoder",
    NULL,
    {0,0,0,0,0},
    {0,0,0,0,0},
    (Uint32)sizeof(PortContainerDisplay),
    5,
    CreateDecoder,
    GetParameterDecoder,
    SetParameterDecoder,
    PrepareDecoder,
    ExecuteDecoder,
    ReleaseDecoder,
    DestroyDecoder,
    0,
    0,
    (ComponentState)0,
    0,
    0,
    {0,NULL,NULL},
    0,
    0,
    (CNMComponentType)0,
    0,
    0,
    NULL,
    0,
    0
};

