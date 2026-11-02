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

#ifndef __VPUAPI_PRODUCT_ABSTRACT_H__
#define __VPUAPI_PRODUCT_ABSTRACT_H__

#include "vpuapifunc.h"

#define IS_CODA_DECODER_HANDLE(_inst)      (_inst->codecMode < AVC_ENC)
#define IS_WAVE_DECODER_HANDLE(_inst)      (_inst->codecMode == W_HEVC_DEC || _inst->codecMode == W_SVAC_DEC || _inst->codecMode == W_AVC_DEC || _inst->codecMode == W_VP9_DEC || _inst->codecMode == W_AVS2_DEC || _inst->codecMode == W_AV1_DEC)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/************************************************************************/
/* COMMON                                                               */
/************************************************************************/
extern Uint32 ProductVpuScan(
    Uint32 coreIdx
    );

extern Int32 ProductVpuGetId(
    Uint32 coreIdx
    );

extern RetCode ProductVpuGetVersion(
    Uint32  coreIdx,
    Uint32* versionInfo,
    Uint32* revision
    );

extern RetCode ProductVpuGetProductInfo(
    Uint32  coreIdx,
    VpuAttr* productInfo
    );

extern RetCode ProductVpuInit(
    Uint32 coreIdx,
    void*  firmware,
    Uint32 size
    );

extern RetCode ProductVpuReInit(
    Uint32 coreIdx,
    void*  firmware,
    Uint32 size
    );

extern Uint32 ProductVpuIsInit(
    Uint32 coreIdx
    );

extern Int32 ProductVpuIsBusy(
    Uint32 coreIdx
    );

extern Int32 ProductVpuWaitInterrupt(
    CodecInst*  instance,
    Int32       timeout
    );

extern RetCode ProductVpuClearInterrupt(
    Uint32      coreIdx,
    Uint32      flags
    );

extern RetCode ProductVpuReset(
    Uint32      coreIdx,
    SWResetMode resetMode
    );

extern RetCode ProductVpuSleepWake(
    Uint32 coreIdx,
    int iSleepWake,
     const Uint16* code,
     Uint32 size
    );

extern RetCode ProductVpuAllocateFramebuffer(
    CodecInst*          instance,
    FrameBuffer*        fbArr,
    TiledMapType        mapType,
    Int32               num,
    Int32               stride,
    Int32               height,
    FrameBufferFormat   format,
    BOOL                cbcrInterleave,
    BOOL                nv21,
    Int32               endian,
    vpu_buffer_t*       vb,
    Int32               gdiIndex,
    FramebufferAllocType fbType
    );

extern RetCode ProductVpuRegisterFramebuffer(
    CodecInst*      instance
    );

extern Int32 ProductCalculateFrameBufSize(
    CodecInst*          inst,
    Int32               productId,
    Int32               stride,
    Int32               height,
    TiledMapType        mapType,
    FrameBufferFormat   format,
    BOOL                interleave,
    DRAMConfig*         pDramCfg
    );

extern RetCode ProductVpuGetBandwidth(
    CodecInst* instance,
    VPUBWData* data
    );


extern RetCode ProductVpuGetDebugInfo(
    CodecInst* instance,
    VPUDebugInfo* info
    );


/************************************************************************/
/* DECODER                                                              */
/************************************************************************/
extern RetCode ProductVpuDecBuildUpOpenParam(
    CodecInst*    instance,
    DecOpenParam* param
    );

extern RetCode ProductVpuDecCheckOpenParam(
    DecOpenParam* param
    );

extern RetCode ProductVpuDecInitSeq(
    CodecInst*  instance
    );

extern RetCode ProductVpuDecFiniSeq(
    CodecInst*  instance
    );

extern RetCode ProductVpuDecSetBitstreamFlag(
    CodecInst*  instance,
    BOOL        running,
    Int32       size
    );

extern RetCode ProductVpuDecSetRdPtr(
    CodecInst*      instance,
    PhysicalAddress rdPtr
    );

extern RetCode ProductVpuDecGetSeqInfo(
    CodecInst*      instance,
    DecInitialInfo* info
    );

extern RetCode ProductVpuDecCheckCapability(
    CodecInst*  instance
    );

extern RetCode ProductVpuDecode(
    CodecInst*  instance,
    DecParam*   option
    );

extern RetCode ProductVpuDecGetResult(
    CodecInst*      instance,
    DecOutputInfo*  result
    );

extern RetCode ProductVpuDecFlush(
    CodecInst*          instance,
    FramebufferIndex*   retIndexes,
    Uint32              size
    );

extern RetCode ProductVpuDecUpdateFrameBuffer(
    CodecInst*   instance,
    FrameBuffer* fbcFb,
    FrameBuffer* linearFb,
    Uint32       mvColIndex,
    Uint32       picWidth,
    Uint32       picHeight
    );

extern RetCode ProductVpuDecClrDispFlag(
    CodecInst* instance,
    Uint32 index
    );

extern RetCode ProductVpuDecSetDispFlag(
    CodecInst* instance,
    Uint32 dispFlag
    );

extern PhysicalAddress ProductVpuDecGetRdPtr(
    CodecInst* instance
    );

/************************************************************************/
/* ENCODER                                                              */
/************************************************************************/
extern RetCode ProductVpuEncUpdateBitstreamBuffer(
    CodecInst* instance
    );

extern RetCode ProductVpuEncGetRdWrPtr(
    CodecInst* instance,
    PhysicalAddress* rdPtr,
    PhysicalAddress* wrPtr
    );

extern RetCode ProductVpuEncBuildUpOpenParam(
    CodecInst*      pCodec,
    EncOpenParam*   param
    );

extern RetCode ProductVpuEncFiniSeq(
    CodecInst*      instance
    );

extern RetCode ProductVpuEncCheckOpenParam(
    EncOpenParam*   param
    );

extern RetCode ProductVpuEncGetHeader(
    CodecInst* instance,
    EncHeaderParam* encHeaderParam
    );

extern RetCode ProductVpuEncSetup(
    CodecInst*      instance
    );

extern RetCode ProductVpuEncode(
    CodecInst*      instance,
    EncParam*       param
    );

extern RetCode ProductVpuEncGetResult(
    CodecInst*      instance,
    EncOutputInfo*  result
    );

extern RetCode ProductVpuEncGiveCommand(
    CodecInst* instance,
    CodecCommand cmd,
    void* param);

extern RetCode ProductVpuEncInitSeq(
    CodecInst*  instance
    );

extern RetCode ProductVpuEncGetSeqInfo(
    CodecInst* instance,
    EncInitialInfo* info
    );

extern RetCode ProductVpuEncChangeParam(
    CodecInst* instance,
    void* param
    );

extern RetCode ProductVpuEncGetSrcBufFlag(
    CodecInst* instance,
    Uint32* data
    );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VPUAPI_PRODUCT_ABSTRACT_H__ */

