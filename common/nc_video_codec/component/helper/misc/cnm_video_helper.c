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
#include "wave5_regdefine.h"
// include in the ffmpeg header
typedef struct {
    CodStd      codStd;
    Uint32      mp4Class;
    Uint32      codecId;
    Uint32      fourcc;
} CodStdTab;

#ifndef MKTAG
#define MKTAG(a,b,c,d) (a | (b << 8) | (c << 16) | (d << 24))
#endif

//////////////////////////////////////////////////////////////////////////////
// this is a sample to control VPU sub frame sync in view of Camera Interface.
//////////////////////////////////////////////////////////////////////////////
static void UpdateSubFrameSyncPos(
    EncHandle               handle,
    EncSubFrameSyncState    *subFrameSyncState,
    Uint32                  y
    )
{

    subFrameSyncState->ipuEndOfRow = VPU_ALIGN64(y+1); // Camera interface module should set ipuEndOfRow whenever image is saved in frame buffer

    VPU_EncGiveCommand(handle, ENC_SET_SUB_FRAME_SYNC, subFrameSyncState);

}

BOOL LoadYuvImageByYCbCrLine(
    EncHandle   handle,
    Uint32      coreIdx,
    Uint8*      src,
    size_t      picWidth,
    size_t      picHeight,
    FrameBuffer* fb,
    void        *arg,
    ENC_subFrameSyncCfg *subFrameSyncConfig,
    Uint32      srcFbIndex
    )
{
    Int32                mem_y, nCb;
    Uint32               YUV_y, nY;
    PhysicalAddress      addrY, addrCb, addrCr;
    PhysicalAddress      nextWriteAddrY, nextWriteAddrCb, nextWriteAddrCr;
    size_t               lumaSize, chromaSize=0, chromaStride = 0, chromaWidth=0;
    Uint8                *srcY, *srcCb, *srcCr;
    size_t               stride      = fb->stride;
    EndianMode           endian      = (EndianMode)fb->endian;
    FrameBufferFormat    format      = fb->format;
    BOOL                 interLeave  = fb->cbcrInterleave;
    int                  twice       = 1 << interLeave;
    int                  cnt=0;
    Uint32               YStartPos=0;
    BOOL                 subFrameSyncEn = subFrameSyncConfig->subFrameSyncOn;
    EncSubFrameSyncState *subFrameSyncState = &subFrameSyncConfig->subFrameSyncState;
    BOOL                 remainDataWrite = (subFrameSyncConfig->subFrameSyncSrcWriteMode & REMAIN_SRC_DATA_WRITE) ? TRUE : FALSE;
    Uint32               writeSrcLine = subFrameSyncConfig->subFrameSyncSrcWriteMode & ~REMAIN_SRC_DATA_WRITE;

    if (subFrameSyncEn == TRUE && writeSrcLine == SRC_0LINE_WRITE && remainDataWrite == FALSE)
        return TRUE;

    nY = (Uint32)picHeight;
    UNREFERENCED_PARAMETER(arg);
    switch (format) {
    case FORMAT_420:
        nCb = (Int32)(picHeight / 2);
        chromaSize = picWidth * picHeight / 4;
        chromaStride = stride / 2;
        chromaWidth = picWidth / 2;
        break;
    case FORMAT_224:
        nCb = (Int32)(picHeight / 2);
        chromaSize = picWidth * picHeight / 2;
        chromaStride = stride;
        chromaWidth = picWidth;
        break;
    case FORMAT_422:
        nCb = (Int32)(picHeight / 2);
        chromaSize = picWidth * picHeight / 2;
        chromaStride = stride;
        chromaWidth = picWidth / 2;
        break;
    case FORMAT_444:
        nCb = (Int32)picHeight;
        chromaSize = picWidth * picHeight;
        chromaStride = stride;
        chromaWidth = picWidth;
        break;
    case FORMAT_400:
        nCb = 0;
        chromaSize = picWidth * picHeight / 4;
        chromaStride = stride / 2;
        chromaWidth = picWidth / 2;
        break;
    case FORMAT_YUYV:
    case FORMAT_YVYU:
    case FORMAT_UYVY:
    case FORMAT_VYUY:
    case FORMAT_YUYV_P10_16BIT_MSB:
    case FORMAT_YUYV_P10_16BIT_LSB:
    case FORMAT_YUYV_P10_32BIT_MSB:
    case FORMAT_YUYV_P10_32BIT_LSB:
    case FORMAT_YVYU_P10_16BIT_MSB:
    case FORMAT_YVYU_P10_16BIT_LSB:
    case FORMAT_YVYU_P10_32BIT_MSB:
    case FORMAT_YVYU_P10_32BIT_LSB:
    case FORMAT_UYVY_P10_16BIT_MSB:
    case FORMAT_UYVY_P10_16BIT_LSB:
    case FORMAT_UYVY_P10_32BIT_MSB:
    case FORMAT_UYVY_P10_32BIT_LSB:
    case FORMAT_VYUY_P10_16BIT_MSB:
    case FORMAT_VYUY_P10_16BIT_LSB:
    case FORMAT_VYUY_P10_32BIT_MSB:
    case FORMAT_VYUY_P10_32BIT_LSB:
        nCb = 0;
        break;
    case FORMAT_420_P10_16BIT_LSB:
    case FORMAT_420_P10_16BIT_MSB:
        nCb = (Int32)(picHeight/2);
        chromaSize = picWidth * picHeight/2;
        chromaStride = stride / 2;
        chromaWidth = picWidth;
        picWidth *= 2;
        break;
    case FORMAT_420_P10_32BIT_LSB:
    case FORMAT_420_P10_32BIT_MSB:
        nCb = (Int32)(picHeight/2);
        picWidth = VPU_ALIGN32(picWidth);
        chromaWidth = ((VPU_ALIGN16(picWidth/2*twice)+2)/3*4);
        chromaStride = VPU_ALIGN16(stride/2)*twice;
        if ( interLeave == 1)
            chromaStride = stride;
        chromaSize = chromaWidth * picHeight/2;
        picWidth   = ((VPU_ALIGN16(picWidth)+2)/3)*4;
        break;
    case FORMAT_422_P10_16BIT_LSB:
    case FORMAT_422_P10_16BIT_MSB:
        nCb = (Int32)picHeight/2;
        chromaSize = picWidth * picHeight;
        chromaStride = stride;
        chromaWidth = picWidth;
        picWidth *= 2;
        break;
    case FORMAT_422_P10_32BIT_LSB:
    case FORMAT_422_P10_32BIT_MSB:
        nCb = (Int32)(picHeight/2);
        picWidth = VPU_ALIGN32(picWidth);
        chromaStride = stride;
        chromaWidth = ((VPU_ALIGN16(picWidth/2*twice)+2)/3*4);
        chromaSize = chromaWidth * picHeight;
        picWidth   = ((VPU_ALIGN16(picWidth)+2)/3)*4;
        break;
    default:
        nCb = (Int32)(picHeight / 2);
        chromaSize = picWidth * picHeight / 4;
        chromaStride = stride / 2;
        chromaWidth = picWidth / 2;
        break;
    }

    lumaSize = picWidth * picHeight;

    srcY  = src;
    srcCb = src + lumaSize;
    srcCr = src + lumaSize + chromaSize;

    addrY  = fb->bufY;
    addrCb = fb->bufCb;
    addrCr = fb->bufCr;

    if ( nCb )
        cnt = (Int32)(picHeight / nCb);

    YUV_y = mem_y = YStartPos;

    if ( subFrameSyncEn == TRUE ) {
        subFrameSyncState->ipuCurFrameIndex = (1 << srcFbIndex);
        if (remainDataWrite == FALSE || (writeSrcLine == 0 && remainDataWrite == TRUE) ) {
            subFrameSyncState->ipuEndOfRow = 0;
            subFrameSyncState->ipuNewFrame = !subFrameSyncState->ipuNewFrame;           // indicate the current is not start of the frame;
            VPU_EncGiveCommand(handle , ENC_SET_SUB_FRAME_SYNC, subFrameSyncState);
        }
        else {
                mem_y = writeSrcLine;
                YUV_y = writeSrcLine;
        }
    }

    for (  ; YUV_y < nY; ++mem_y, ++YUV_y) {
        nextWriteAddrY = (PhysicalAddress)(addrY + stride * mem_y);
        vdi_write_memory(coreIdx, nextWriteAddrY, (Uint8 *)(srcY + YUV_y * picWidth), (Int32)picWidth, endian);
        if (format == FORMAT_400) {
            if ( subFrameSyncEn == TRUE && ( (YUV_y-YStartPos+1) > (64-1) && ((YUV_y-YStartPos+1) % 64) == 0) ) {
                UpdateSubFrameSyncPos(handle, subFrameSyncState, YUV_y);
                if (writeSrcLine == YUV_y+1) {
                    return TRUE;
                }
            }
            continue;
        }
        if (format >= FORMAT_YUYV && format <= FORMAT_VYUY_P10_32BIT_LSB) {
            if ( subFrameSyncEn == TRUE && ( (YUV_y-YStartPos+1) > (64-1) && ((YUV_y-YStartPos+1) % 64) == 0) ) {
                UpdateSubFrameSyncPos(handle, subFrameSyncState, YUV_y);
                if (writeSrcLine == YUV_y+1 ) {
                    return TRUE;
                }
            }
            continue;
        }
        if (interLeave == TRUE) {
            if (cnt == 2 && ((YUV_y%2)==0)) {
                nextWriteAddrCb = (PhysicalAddress)(addrCb + stride * mem_y/cnt);
                switch (format) {
                case FORMAT_422:
                case FORMAT_422_P10_16BIT_LSB:
                case FORMAT_422_P10_16BIT_MSB:
                case FORMAT_422_P10_32BIT_LSB:
                case FORMAT_422_P10_32BIT_MSB:
                    vdi_write_memory(coreIdx, nextWriteAddrCb, (Uint8 *)(srcCb+ YUV_y/cnt * picWidth), (Int32)picWidth, endian);
                    vdi_write_memory(coreIdx, (PhysicalAddress)(nextWriteAddrCb + stride * mem_y/cnt), (Uint8 *)(srcCb+ YUV_y/cnt * (picWidth*2)), (Int32)picWidth, endian);
                    break;
                default:
                    vdi_write_memory(coreIdx, nextWriteAddrCb, (Uint8 *)(srcCb+ YUV_y/cnt * picWidth), (Int32)picWidth, endian);
                    break;
                }
            }
        }
        else {
            if (cnt == 2 && ((YUV_y%2)==0)) {
                nextWriteAddrCb = (PhysicalAddress)(addrCb + chromaStride * mem_y/cnt);
                nextWriteAddrCr = (PhysicalAddress)(addrCr + chromaStride * mem_y/cnt);
                switch (format) {
                case FORMAT_422:
                case FORMAT_422_P10_16BIT_LSB:
                case FORMAT_422_P10_16BIT_MSB:
                case FORMAT_422_P10_32BIT_LSB:
                case FORMAT_422_P10_32BIT_MSB:
                    vdi_write_memory(coreIdx, nextWriteAddrCb, (Uint8 *)(srcCb + YUV_y * chromaWidth), (Int32)chromaWidth, endian);
                    vdi_write_memory(coreIdx, nextWriteAddrCr, (Uint8 *)(srcCr + YUV_y * chromaWidth), (Int32)chromaWidth, endian);

                    vdi_write_memory(coreIdx, (PhysicalAddress)(nextWriteAddrCb+chromaStride/2), (Uint8 *)(srcCb + (YUV_y+1) * chromaWidth), (Int32)chromaWidth, endian);
                    vdi_write_memory(coreIdx, (PhysicalAddress)(nextWriteAddrCr+chromaStride/2), (Uint8 *)(srcCr + (YUV_y+1) * chromaWidth), (Int32)chromaWidth, endian);
                    break;
                default:
                    vdi_write_memory(coreIdx, nextWriteAddrCb, (Uint8 *)(srcCb + YUV_y/cnt * chromaWidth), (Int32)chromaWidth, endian);
                    vdi_write_memory(coreIdx, nextWriteAddrCr, (Uint8 *)(srcCr + YUV_y/cnt * chromaWidth), (Int32)chromaWidth, endian);
                    break;
                }
            }
        }
        if ( subFrameSyncEn == TRUE && ( (YUV_y-YStartPos+1) > (64-1) && ((YUV_y-YStartPos+1) % 64) == 0) ) {
            UpdateSubFrameSyncPos(handle, subFrameSyncState, YUV_y);
            if (writeSrcLine == YUV_y+1 ) {
                return TRUE;
            }
        }
    }

    //remain YUV_y line
    if ( subFrameSyncEn == TRUE && ( (YUV_y % 64) != 0) ) {//YUV_y is already added number one at for loop
        UpdateSubFrameSyncPos(handle, subFrameSyncState, YUV_y-1);
    }

    return TRUE;//lint !e438
}

//////////////////// DRAM Read/Write helper Function ////////////////////////////
BOOL LoadYuvImageBurstFormat(
    Uint32      coreIdx,
    Uint8*      src,
    size_t      picWidth,
    size_t      picHeight,
    FrameBuffer* fb,
    BOOL        convertCbcrIntl
    )
{
    Int32               y, nY=0, nCb=0, nCr=0;
    PhysicalAddress     addr;
    size_t              lumaSize, chromaSize=0, chromaStride = 0, chromaWidth=0;
    Uint8*              puc;
    size_t              stride      = fb->stride;
    EndianMode          endian      = (EndianMode)fb->endian;
    FrameBufferFormat   format      = fb->format;
    BOOL                interLeave  = fb->cbcrInterleave;
    int                 twice       = 1 << interLeave;

    switch (format) {
    case FORMAT_420:
        nY = (Int32)picHeight;
        nCb = nCr = (Int32)(picHeight / 2);
        chromaSize = picWidth * picHeight / 4;
        chromaStride = stride / 2;
        chromaWidth = picWidth / 2;
        break;
    case FORMAT_224:
        nY = (Int32)picHeight;
        nCb = nCr = (Int32)(picHeight / 2);
        chromaSize = picWidth * picHeight / 2;
        chromaStride = stride;
        chromaWidth = picWidth;
        break;
    case FORMAT_422:
        nY = (Int32)picHeight;
        nCb = nCr = (Int32)picHeight;
        chromaSize = picWidth * picHeight / 2;
        chromaStride = stride / 2;
        chromaWidth = picWidth / 2;
        break;
    case FORMAT_444:
        nY = (Int32)picHeight;
        nCb = nCr = (Int32)picHeight;
        chromaSize = picWidth * picHeight;
        chromaStride = stride;
        chromaWidth = picWidth;
        break;
    case FORMAT_400:
        nY = (Int32)picHeight;
        nCb = nCr = 0;
        chromaSize = picWidth * picHeight / 4;
        chromaStride = stride / 2;
        chromaWidth = picWidth / 2;
        break;
    case FORMAT_YUYV:
    case FORMAT_YVYU:
    case FORMAT_UYVY:
    case FORMAT_VYUY:
    case FORMAT_YUYV_P10_16BIT_MSB:
    case FORMAT_YUYV_P10_16BIT_LSB:
    case FORMAT_YUYV_P10_32BIT_MSB:
    case FORMAT_YUYV_P10_32BIT_LSB:
    case FORMAT_YVYU_P10_16BIT_MSB:
    case FORMAT_YVYU_P10_16BIT_LSB:
    case FORMAT_YVYU_P10_32BIT_MSB:
    case FORMAT_YVYU_P10_32BIT_LSB:
    case FORMAT_UYVY_P10_16BIT_MSB:
    case FORMAT_UYVY_P10_16BIT_LSB:
    case FORMAT_UYVY_P10_32BIT_MSB:
    case FORMAT_UYVY_P10_32BIT_LSB:
    case FORMAT_VYUY_P10_16BIT_MSB:
    case FORMAT_VYUY_P10_16BIT_LSB:
    case FORMAT_VYUY_P10_32BIT_MSB:
    case FORMAT_VYUY_P10_32BIT_LSB:
        nY = (Int32)picHeight;
        nCb = nCr = 0;
        break;
    case FORMAT_420_P10_16BIT_LSB:
    case FORMAT_420_P10_16BIT_MSB:
        nY = (Int32)picHeight;
        nCb = nCr = (Int32)(picHeight/2);
        chromaSize = picWidth * picHeight/2;
        chromaStride = stride / 2;
        chromaWidth = picWidth;
        picWidth *= 2;
        break;
    case FORMAT_420_P10_32BIT_LSB:
    case FORMAT_420_P10_32BIT_MSB:
        nY = (Int32)picHeight;
        nCb = nCr = (Int32)(picHeight/2);
        picWidth = VPU_ALIGN32(picWidth);
        chromaWidth = ((VPU_ALIGN16(picWidth/2*twice)+2)/3*4);
        chromaStride = VPU_ALIGN16(stride/2)*twice;
        if ( interLeave == 1)
            chromaStride = stride;
        chromaSize = chromaWidth * picHeight/2;
        picWidth   = ((VPU_ALIGN16(picWidth)+2)/3)*4;
        break;
    case FORMAT_422_P10_16BIT_LSB:
    case FORMAT_422_P10_16BIT_MSB:
        nY = (Int32)picHeight;
        nCb = nCr = (Int32)(picHeight/2);
        chromaSize = picWidth * picHeight;
        chromaStride = stride;
        chromaWidth = picWidth;
        picWidth *= 2;
        break;
    case FORMAT_422_P10_32BIT_LSB:
    case FORMAT_422_P10_32BIT_MSB:
        nY = (Int32)picHeight;
        nCb = nCr = (Int32)(picHeight/2);
        picWidth = VPU_ALIGN32(picWidth);
        chromaWidth = ((VPU_ALIGN16(picWidth/2*twice)+2)/3*4);
        chromaStride = VPU_ALIGN16(stride/2)*twice;
        if ( interLeave == 1)
            chromaStride = stride;
        chromaSize = chromaWidth * picHeight/2;
        picWidth   = ((VPU_ALIGN16(picWidth)+2)/3)*4;
        break;
    default:
        nY = (Int32)picHeight;
        nCb = nCr = (Int32)(picHeight / 2);
        chromaSize = picWidth * picHeight / 4;
        chromaStride = stride / 2;
        chromaWidth = picWidth / 2;
        break;
    }

    puc = src;
    addr = fb->bufY;
    lumaSize = picWidth * picHeight;

    if( picWidth == stride) { // for fast write
        vdi_write_memory(coreIdx, addr, (Uint8 *)( puc ), (Int32)lumaSize, endian);

        if( format == FORMAT_400)
            return FALSE;
        if (format >= FORMAT_YUYV && format <= FORMAT_VYUY_P10_32BIT_LSB)
            return FALSE;

        if (interLeave == TRUE) {
            UNREFERENCED_PARAMETER(convertCbcrIntl);
            puc = src + lumaSize;
            addr = fb->bufCb;
            vdi_write_memory(coreIdx, addr, (Uint8 *)puc, (Int32)(chromaSize*2), endian);
        }
        else {
            if ( chromaWidth == chromaStride )
            {
                puc = src + lumaSize;
                addr = fb->bufCb;
                vdi_write_memory(coreIdx, addr, (Uint8 *)puc, (Int32)chromaSize, endian);

                puc = src + lumaSize + chromaSize;
                addr = fb->bufCr;
                vdi_write_memory(coreIdx, addr, (Uint8 *)puc, (Int32)chromaSize, endian);
            }
            else
            {
                puc = src + lumaSize;
                addr = fb->bufCb;
                for (y = 0; y < nCb; ++y) {
                    vdi_write_memory(coreIdx, (PhysicalAddress)(addr + chromaStride * y), (Uint8 *)(puc + y * chromaWidth), (Int32)chromaWidth, endian);
                }

                puc = src + lumaSize + chromaSize;
                addr = fb->bufCr;
                for (y = 0; y < nCr; ++y) {
                    vdi_write_memory(coreIdx, (PhysicalAddress)(addr + chromaStride * y), (Uint8 *)(puc + y * chromaWidth), (Int32)chromaWidth, endian);
                }
            }
        }
    }
    else {
#if defined(CNM_FPGA_HAPS_INTERFACE) || defined(CNM_FPGA_VU440_INTERFACE)
        {
            Uint8   *temp;
            UNREFERENCED_PARAMETER(chromaStride);
            UNREFERENCED_PARAMETER(chromaSize);
            temp = osal_malloc(stride*picHeight);
            if (!temp)
                return FALSE;

            for (y=0;y< nY;y++) {
                osal_memcpy(temp+stride*y, puc+y*picWidth, picWidth);
            }
            vdi_write_memory(coreIdx, addr, (Uint8 *)( temp ), stride*picHeight, endian);

            if (format == FORMAT_400) {
                osal_free(temp);
                return FALSE;
            }
            if (format >= FORMAT_YUYV && format <= FORMAT_VYUY_P10_32BIT_LSB) {
                osal_free(temp);
                return FALSE;
            }
            if (interLeave == TRUE) {
                puc = src + lumaSize;
                addr = fb->bufCb;
                for (y=0;y< nCb;y++) {
                    osal_memcpy(temp+stride*y, puc+y*picWidth, picWidth);
                }
                vdi_write_memory(coreIdx, addr, (Uint8 *)temp, stride*nCb, endian);
            } else {
                puc = src + lumaSize;
                addr = fb->bufCb;
                for (y = 0; y < nCb; ++y) {
                    osal_memcpy(temp+chromaStride*y, puc+y*chromaWidth, chromaWidth);
                }
                vdi_write_memory(coreIdx, addr, (Uint8 *)temp, chromaStride*nCb, endian);

                puc = src + lumaSize + chromaSize;
                addr = fb->bufCr;
                for (y = 0; y < nCr; ++y) {
                    osal_memcpy(temp+chromaStride*y, puc+y*chromaWidth, chromaWidth);
                }
                vdi_write_memory(coreIdx, addr, (Uint8 *)temp, chromaStride*nCr, endian);
            }
            osal_free(temp);
        }
#else
        for (y = 0; y < nY; ++y) {
            vdi_write_memory(coreIdx, (PhysicalAddress)(addr + stride * y), (Uint8 *)(puc + y * picWidth), (Int32)picWidth, endian);
        }

        if (format == FORMAT_400) {
            return FALSE;
        }
        if (format >= FORMAT_YUYV && format <= FORMAT_VYUY_P10_32BIT_LSB) {
            return FALSE;
        }

        if (interLeave == TRUE) {
            UNREFERENCED_PARAMETER(convertCbcrIntl);
            puc = src + lumaSize;
            addr = fb->bufCb;
            for (y = 0; y < nCb; ++y) {
                vdi_write_memory(coreIdx, (PhysicalAddress)(addr + stride * y), (Uint8 *)(puc + y * picWidth), (Int32)picWidth, endian);
            }
        }
        else {
            puc = src + lumaSize;
            addr = fb->bufCb;
            for (y = 0; y < nCb; ++y) {
                vdi_write_memory(coreIdx, (PhysicalAddress)(addr + chromaStride * y), (Uint8 *)(puc + y * chromaWidth), (Int32)chromaWidth, endian);
            }

            puc = src + lumaSize + chromaSize;
            addr = fb->bufCr;
            for (y = 0; y < nCr; ++y) {
                vdi_write_memory(coreIdx, (PhysicalAddress)(addr + chromaStride * y), (Uint8 *)(puc + y * chromaWidth), (Int32)chromaWidth, endian);
            }
        }
#endif /* CNM_FPGA_HAPS_INTERFACE */
    }

    return TRUE;//lint !e438
}

BOOL LoadTiledImageYuvBurst(
    VpuHandle       handle __attribute__((unused)),
    Uint32          coreIdx,
    BYTE*           pYuv,
    size_t          picWidth,
    size_t          picHeight,
    FrameBuffer*    fb,
    TiledMapConfig  mapCfg
    )
{
    BYTE *pSrc;
    size_t              divX, divY;
    PhysicalAddress     pix_addr;
    size_t              rrow, ccol;
    size_t              offsetX,offsetY;
    size_t              stride_c;
    size_t              stride      = fb->stride;
    EndianMode          endian      = (EndianMode)fb->endian;
    FrameBufferFormat   format      = fb->format;
    BOOL                interLeave  = fb->cbcrInterleave;
    Int32               productId;
    Int32               dramBusWidth = 8;

    productId = VPU_GetProductId(coreIdx);
    if (PRODUCT_ID_W_SERIES(productId)) {
        dramBusWidth = 16;
    }

    offsetX = offsetY    = 0;

    divX = format == FORMAT_420 || format == FORMAT_422 ? 2 : 1;
    divY = format == FORMAT_420 || format == FORMAT_224 ? 2 : 1;

    switch (format) {
    case FORMAT_400:
        stride_c = 0;
        break;
    case FORMAT_420:
    case FORMAT_422:
        stride_c = stride / 2;
        break;
    case FORMAT_224:
    case FORMAT_444:
        stride_c = stride;
        break;
    default:
        stride_c = stride / 2;
        break;
    }

    // Y
    pSrc    = pYuv;

    // no opt code
    for (rrow=0; rrow <picHeight; rrow=rrow+1)
    {
        for (ccol=0; ccol<picWidth; ccol=ccol+dramBusWidth)
        {
            pix_addr = GetXY2AXIAddr(&mapCfg, 0/*luma*/, (int)(rrow +offsetY), (int)(ccol + offsetX), (int)stride, fb);
            vdi_write_memory(coreIdx, pix_addr, pSrc+rrow*picWidth+ccol, 8, endian);
        }
    }

    if (format == FORMAT_400) {
        return 1;
    }

    if (interLeave == FALSE) {
        // CB
        pSrc = pYuv + picWidth*picHeight;

        for (rrow=0; rrow <(picHeight/divY) ; rrow=rrow+1) {
            for (ccol=0; ccol<(picWidth/divX); ccol=ccol+dramBusWidth) {
                pix_addr = GetXY2AXIAddr(&mapCfg, 2, (int)(rrow + offsetY), (int)(ccol +offsetX), (int)stride_c, fb);
                vdi_write_memory(coreIdx, pix_addr, pSrc+rrow*picWidth/divX+ccol, 8, endian);
            }
        }
        // CR

        pSrc = pYuv + picWidth*picHeight+ (picWidth/divX)*(picHeight/divY);

        for (rrow=0; rrow <picHeight/divY ; rrow=rrow+1) {
            for (ccol=0; ccol<picWidth/divX; ccol=ccol+dramBusWidth) {
                pix_addr = GetXY2AXIAddr(&mapCfg, 3, (int)(rrow  + offsetY), (int)(ccol +offsetX), (int)stride_c, fb);
                vdi_write_memory(coreIdx, pix_addr, pSrc+rrow*picWidth/divX+ccol, 8, endian);
            }
        }
    }
    else {

        BYTE * pTemp;
        BYTE * srcAddrCb;
        BYTE * srcAddrCr;

        size_t  cbcr_x;

        switch( format) {
        case FORMAT_444 :
            cbcr_x = picWidth*2;
            break;
        case FORMAT_420 :
            cbcr_x = picWidth  ;
            break;
        case FORMAT_422 :
            cbcr_x = picWidth  ;
            break;
        case FORMAT_224 :
            cbcr_x = picWidth*2;
            break;
        default:
            cbcr_x = picWidth  ;
            break;
        }

        stride = stride_c * 2;

        srcAddrCb = pYuv + picWidth*picHeight;
        srcAddrCr = pYuv + picWidth*picHeight + picWidth/divX*picHeight/divY;


        pTemp = (BYTE*)osal_malloc(sizeof(char)*8);
        if (!pTemp) {
            return FALSE;
        }

        for (rrow=0; rrow <picHeight/divY; rrow=rrow+1) {
            for (ccol=0; ccol<cbcr_x ; ccol=ccol+dramBusWidth) {

                pTemp[0  ] = *srcAddrCb++;
                pTemp[0+2] = *srcAddrCb++;
                pTemp[0+4] = *srcAddrCb++;
                pTemp[0+6] = *srcAddrCb++;
                pTemp[0+1] = *srcAddrCr++;
                pTemp[0+3] = *srcAddrCr++;
                pTemp[0+5] = *srcAddrCr++;
                pTemp[0+7] = *srcAddrCr++;

                pix_addr = GetXY2AXIAddr(&mapCfg, 2, (int)(rrow + offsetY) , (int)(ccol + (offsetX*2)), (int)stride, fb);
                vdi_write_memory(coreIdx, pix_addr, (unsigned char *)pTemp, 8, endian);
            }
        }
        osal_free(pTemp);
    }

    return TRUE;
}


static void SwapDword(unsigned char* data, int len)
{
    Uint32  temp;
    Uint32* ptr = (Uint32*)data;
    Int32   i, size = len/sizeof(Uint32);

    for (i=0; i<size; i+=2) {
        temp      = ptr[i];
        ptr[i]   = ptr[i+1];
        ptr[i+1] = temp;
    }
}

static void SwapLword(unsigned char* data, int len)
{
    Uint64  temp;
    Uint64* ptr = (Uint64*)data;
    Int32   i, size = len/sizeof(Uint64);

    for (i=0; i<size; i+=2) {
        temp      = ptr[i];
        ptr[i]   = ptr[i+1];
        ptr[i+1] = temp;
    }
}

static void SwapPixelOrder(
    Uint8*      data
    )
{
    Uint32*     temp;
    Uint32      temp2[4]={0,};
    Int32       i,j;

    for (i=0, j=3 ; i < 16 ; i+=4, j--) {
        temp = (Uint32*)(data+i);
        temp2[j] =  (*temp & 0xffc00000)>>20;
        temp2[j] |= (*temp & 0x003ff000);
        temp2[j] |= (*temp & 0x00000ffc)<<20;
    }

    osal_memcpy(data, temp2, 16);

//for matching with Ref-C
    SwapDword(data, 16);
    SwapLword(data, 16);
}



Uint32 StoreYuvImageBurstLinear(
    Uint32      coreIdx,
    FrameBuffer *fbSrc,
    TiledMapConfig  mapCfg,
    Uint8       *pDst,
    VpuRect     cropRect,
    BOOL        enableCrop,
    const DecInfo* pDecInfo
    )
{
    Uint32          y, x;
    Uint32          pix_addr, div_x, div_y, chroma_stride=0;
    Uint8*          puc;
    Uint8*          rowBufferY, *rowBufferCb, *rowBufferCr;
    Uint32          stride      = fbSrc->stride;
    Uint32          height      = fbSrc->height;
    int             interLeave  = fbSrc->cbcrInterleave;
    BOOL            nv21        = fbSrc->nv21;
    EndianMode      endian      = (EndianMode)fbSrc->endian;
    FrameBufferFormat format    = (FrameBufferFormat)fbSrc->format;
    Uint32          width;
    Uint32          dstWidth=0, dstHeight=0;
    Uint32          offsetX, offsetY;
    Uint32          dstChromaHeight = 0;
    Uint32          dstChromaWidth = 0;
    Uint32          chromaHeight = 0;
    Uint32          bpp=8;
    Uint32          p10_32bit_interleave = 0;
    Int32           productId;
    Int32           dramBusWidth = 8;
    Uint32          totSize = 0;
    BOOL            copyLumaOnly = FALSE;

    //Int32           addr;
    Int32           baseY;
    Int32           baseCb;
    Int32           baseCr;
    Uint8           *pY;
    Uint8           *pCbTemp;
    Uint8           *pCb;
    Uint8           *pCr;
    Uint32          bitKey = 0;

    productId = VPU_GetProductId(coreIdx);
    if (PRODUCT_ID_W_SERIES(productId)) {
        dramBusWidth = 16;
    }
    switch (fbSrc->format) {
    case FORMAT_420:
    case FORMAT_420_P10_16BIT_LSB:
    case FORMAT_420_P10_16BIT_MSB:
    case FORMAT_420_P10_32BIT_LSB:
    case FORMAT_420_P10_32BIT_MSB:
    case FORMAT_422:
    case FORMAT_422_P10_16BIT_LSB:
    case FORMAT_422_P10_16BIT_MSB:
    case FORMAT_422_P10_32BIT_LSB:
    case FORMAT_422_P10_32BIT_MSB:
        div_x = 2;
        break;
    default:
        div_x = 1;
    }

    switch (fbSrc->format) {
    case FORMAT_420:
    case FORMAT_420_P10_16BIT_LSB:
    case FORMAT_420_P10_16BIT_MSB:
    case FORMAT_420_P10_32BIT_LSB:
    case FORMAT_420_P10_32BIT_MSB:
    case FORMAT_224:
        div_y = 2;
        break;
    default:
        div_y = 1;
    }

    //for matching with Ref-C
    width     = (enableCrop == TRUE ? cropRect.right - cropRect.left : stride);
    dstHeight = (enableCrop == TRUE ? cropRect.bottom - cropRect.top : height);
    offsetX   = (enableCrop == TRUE ? cropRect.left : 0);
    offsetY   = (enableCrop == TRUE ? cropRect.top  : 0);

    if (pDecInfo->openParam.bitstreamFormat == STD_VP9) {
        if (pDecInfo->scalerEnable != TRUE) {
            width = VPU_ALIGN8(width);
            height = VPU_ALIGN8(height);
        }
    }

    switch (fbSrc->format) {
    case FORMAT_400:
        copyLumaOnly = TRUE;
        break;
    case FORMAT_YUYV:
    case FORMAT_YVYU:
    case FORMAT_UYVY:
    case FORMAT_VYUY:
        copyLumaOnly    = TRUE;
        dstWidth        = width * 2;
        dstChromaHeight = 0;
        chromaHeight    = 0;
        break;
    case FORMAT_YUYV_P10_16BIT_LSB:
    case FORMAT_YUYV_P10_16BIT_MSB:
    case FORMAT_YVYU_P10_16BIT_LSB:
    case FORMAT_YVYU_P10_16BIT_MSB:
    case FORMAT_UYVY_P10_16BIT_LSB:
    case FORMAT_UYVY_P10_16BIT_MSB:
    case FORMAT_VYUY_P10_16BIT_LSB:
    case FORMAT_VYUY_P10_16BIT_MSB:
        copyLumaOnly    = TRUE;
        dstWidth        = (width * 2)*2;
        dstChromaHeight = 0;
        chromaHeight    = 0;
        break;
    case FORMAT_YUYV_P10_32BIT_LSB:
    case FORMAT_YUYV_P10_32BIT_MSB:
    case FORMAT_YVYU_P10_32BIT_LSB:
    case FORMAT_YVYU_P10_32BIT_MSB:
    case FORMAT_UYVY_P10_32BIT_LSB:
    case FORMAT_UYVY_P10_32BIT_MSB:
    case FORMAT_VYUY_P10_32BIT_LSB:
    case FORMAT_VYUY_P10_32BIT_MSB:
        copyLumaOnly    = TRUE;
        dstWidth        = ((width+2)/3*4)*2;
        dstChromaHeight = 0;
        chromaHeight    = 0;
        break;
    case FORMAT_422_P10_16BIT_LSB:
    case FORMAT_422_P10_16BIT_MSB:
        dstWidth = width * 2;
        bpp = 16;
        dstChromaWidth  = (dstWidth+1) / div_x;
        dstChromaHeight = (dstHeight+1) / div_y;
        chromaHeight    = (height+1) / div_y;
        chroma_stride   = (stride / div_x);
        break;
    case FORMAT_420_P10_16BIT_LSB:
    case FORMAT_420_P10_16BIT_MSB:
        dstWidth = width * 2;
        bpp = 16;
        dstChromaWidth  = (dstWidth+1) / div_x;
        dstChromaHeight = (dstHeight+1) / div_y;
        chromaHeight    = (height+1) / div_y;
        chroma_stride = (stride / div_x);
        break;
    case FORMAT_420_P10_32BIT_LSB:
    case FORMAT_420_P10_32BIT_MSB:
#ifdef DUMP_YUV_WHOLE_DATA
        if (interLeave)
        {
            dstChromaWidth = ((VPU_ALIGN16(width/div_x))*2+11)/12*16;
            dstChromaWidth = VPU_ALIGN16(dstChromaWidth);
            dstChromaHeight = dstHeight / div_y;

            stride = stride;
            chroma_stride = stride;
            dstWidth = (VPU_ALIGN16(width)+11)/12*16;

            interLeave = 0;
        }
        else
        {
            dstChromaWidth = ((VPU_ALIGN16(width/div_x))+11)/12*16;
            dstChromaWidth = VPU_ALIGN16(dstChromaWidth);
            dstChromaHeight = dstHeight / div_y;
            chroma_stride = dstChromaWidth;
            stride    = (VPU_ALIGN16(stride)+11)/12*16;
            dstWidth = (VPU_ALIGN16(dstWidth)+11)/12*16;
        }
        break;
#else
        if (interLeave) {
            dstChromaWidth = ((VPU_ALIGN16(width*2/div_x))+11)/12*16;
            dstChromaWidth = VPU_ALIGN16(dstChromaWidth);
            if(pDecInfo->openParam.bitstreamFormat == STD_VP9) {
                dstChromaWidth = VPU_ALIGN32(dstChromaWidth);
            }
            chroma_stride = stride;

            dstChromaWidth = (width/div_x+2)/3*4;

            dstChromaHeight = dstHeight / div_y;
            chromaHeight    = height / div_y;

            dstWidth = (width+2)/3*4;

            interLeave = 0;
            p10_32bit_interleave = 1;
        }
        else {
            chroma_stride = stride;
            dstChromaWidth = ((width+1)+2)/3*4;

            dstChromaHeight = dstHeight / div_y;
            chromaHeight    = height / div_y;

            dstWidth = (width+2)/3*4;
        }
        break;
#endif
    case FORMAT_422_P10_32BIT_LSB:
    case FORMAT_422_P10_32BIT_MSB:
        if (interLeave) {
            dstChromaWidth = ((VPU_ALIGN16(width*2/div_x))+11)/12*16;
            dstChromaWidth = VPU_ALIGN16(dstChromaWidth);
            if(pDecInfo->openParam.bitstreamFormat == STD_VP9) {
                dstChromaWidth = VPU_ALIGN32(dstChromaWidth);
            }
            chroma_stride = stride;

            dstChromaWidth = (width/div_x+2)/3*4;

            dstChromaHeight = dstHeight / div_y;
            chromaHeight    = height / div_y;
            dstWidth = (width+2)/3*4;

            interLeave = 0;
            p10_32bit_interleave = 1;
        }
        else {
            dstWidth = (width+2)/3*4;
            dstChromaWidth = (((width+1) / div_x)+2)/3*4;
            dstChromaHeight = dstHeight / div_y;
            chromaHeight    = height / div_y;
            chroma_stride = (stride / div_x);
        }
        break;
    default:
        dstWidth = width;
        dstChromaWidth  = (width+1) / div_x;
        dstChromaHeight = (dstHeight+1) / div_y;
        chromaHeight    = (height+1) / div_y;
        chroma_stride   = (stride / div_x);
        break;
    }

    bitKey=1;
    bitKey=~(bitKey);
    if (div_x == 1) {
        dstChromaWidth&=bitKey;
    }
    if (div_y ==1) {
        dstChromaHeight&=bitKey;
        chromaHeight&=bitKey;
    }

    puc         = pDst;

    pY = (Uint8*)osal_malloc(stride * height);
    if ( !pY )
        return 0;
    pCbTemp = (Uint8*)osal_malloc(stride*4 * chromaHeight);
    if ( !pCbTemp )
        return 0;
    pCb = (Uint8*)osal_malloc(stride*4 * chromaHeight);
    if ( !pCb )
        return 0;
    pCr = (Uint8*)osal_malloc(stride*2 * chromaHeight);
    if ( !pCr )
        return 0;
    baseY = fbSrc->bufY;
    baseCb = fbSrc->bufCb;
    baseCr = fbSrc->bufCr;

    vdi_read_memory(coreIdx, fbSrc->bufY, pY, stride * height, endian);

    for (y=0 ; y<dstHeight ; y+=1) {
        pix_addr = GetXY2AXIAddr(&mapCfg, 0, y+offsetY, 0, stride, fbSrc);
        rowBufferY = pY + (pix_addr - baseY);
        // CHECK POINT
        for (x=0; x<stride ; x+=dramBusWidth) {
            if ( fbSrc->format == FORMAT_420_P10_32BIT_MSB )
                SwapPixelOrder(rowBufferY+x);
        }
        // CHECK POINT

        osal_memcpy(puc+y*dstWidth, rowBufferY+offsetX, dstWidth);
        totSize += dstWidth;
    }

    if (copyLumaOnly == TRUE) {
        osal_free(pY);
        osal_free(pCb);
        osal_free(pCr);
        osal_free(pCbTemp);
        return totSize;
    }

    if (interLeave || p10_32bit_interleave) {
        Int32    cbcr_per_2pix=1;

        cbcr_per_2pix = (format==FORMAT_224||format==FORMAT_444) ? 2 : 1;
        vdi_read_memory(coreIdx, fbSrc->bufCb, pCbTemp, stride*cbcr_per_2pix * chromaHeight, endian);
    } else {
        vdi_read_memory(coreIdx, fbSrc->bufCb, pCb, chroma_stride * chromaHeight, endian);
        if ( (fbSrc->format == FORMAT_420_P10_32BIT_LSB || fbSrc->format == FORMAT_420_P10_32BIT_MSB) &&
            p10_32bit_interleave == 1) {
            // Nothing to do
        }
        else {
            vdi_read_memory(coreIdx, fbSrc->bufCr, pCr, chroma_stride * chromaHeight, endian);
        }
    }

    if (interLeave == TRUE || p10_32bit_interleave == TRUE) {
        Uint8*   pTemp;
        Uint8*   dstAddrCb;
        Uint8*   dstAddrCr;
        Uint8*   ptrCb, *ptrCr;
        Int32    cbcr_per_2pix=1, k;
        Uint32*  pTempLeft32, *pTempRight32;
        Uint32   temp_32;

        dstAddrCb = pDst + dstWidth*dstHeight;
        dstAddrCr = dstAddrCb + dstChromaWidth*dstChromaHeight;

        cbcr_per_2pix = (format==FORMAT_224||format==FORMAT_444) ? 2 : 1;

        for ( y = 0 ; y < dstChromaHeight; ++y ) {
            ptrCb = pCb;
            ptrCr = pCr;
            for ( x = 0 ; x < stride*cbcr_per_2pix ; x += dramBusWidth ) {
                pix_addr = GetXY2AXIAddr(&mapCfg, 2, y+(offsetY/div_y), x, stride, fbSrc);
                pTemp = pCbTemp + (pix_addr - baseCb);
                // CHECK POINT
                if ( fbSrc->format == FORMAT_420_P10_32BIT_MSB )
                    SwapPixelOrder(pTemp);
                // CHECK POINT

                if (interLeave == TRUE) {
                    for (k=0; k<dramBusWidth && (x+k) < stride; k+=(2*bpp/8)) {
                        if (bpp == 8) {
                            if (nv21) {
                                *ptrCr++ = pTemp[k];
                                *ptrCb++ = pTemp[k+1];
                            }
                            else {
                                *ptrCb++ = pTemp[k];
                                *ptrCr++ = pTemp[k+1];
                            }
                        }
                        else {
                            if (nv21) {
                                *ptrCr++ = pTemp[k];
                                *ptrCr++ = pTemp[k+1];
                                *ptrCb++ = pTemp[k+2];
                                *ptrCb++ = pTemp[k+3];
                            }
                            else {
                                *ptrCb++ = pTemp[k];
                                *ptrCb++ = pTemp[k+1];
                                *ptrCr++ = pTemp[k+2];
                                *ptrCr++ = pTemp[k+3];
                            }
                        }
                    }
                }
                else {
                    for (k=0; k<dramBusWidth && (x+k) < stride; k+=8) {//(2*bpp/8)) {
                        pTempLeft32 = (Uint32*)&pTemp[k];
                        pTempRight32 = (Uint32*)&pTemp[k+4];

                        if (format==FORMAT_420_P10_32BIT_MSB) {
                            temp_32 = *pTempLeft32 & 0x003ff000;
                            *pTempLeft32 = (*pTempLeft32 & 0xffc00000)
                                | (*pTempLeft32 & 0x00000ffc) << 10
                                | (*pTempRight32 & 0x003ff000) >> 10;
                            *pTempRight32 = (temp_32) << 10
                                | (*pTempRight32 & 0xffc00000) >> 10
                                | (*pTempRight32 & 0x00000ffc);
                        }
                        else if (format==FORMAT_420_P10_32BIT_LSB) {
                            temp_32 = *pTempLeft32 & 0x000ffc00;
                            *pTempLeft32 = (*pTempLeft32 & 0x000003ff)
                                | (*pTempLeft32 & 0x3ff00000) >> 10
                                | (*pTempRight32 & 0x000ffc00) << 10;
                            *pTempRight32 = (temp_32) >> 10
                                | (*pTempRight32 & 0x000003ff) << 10
                                | (*pTempRight32 & 0x3ff00000);
                        }

                        if (nv21) {
                            *ptrCr++ = pTemp[k];
                            *ptrCr++ = pTemp[k+1];
                            *ptrCr++ = pTemp[k+2];
                            *ptrCr++ = pTemp[k+3];
                            *ptrCb++ = pTemp[k+4];
                            *ptrCb++ = pTemp[k+5];
                            *ptrCb++ = pTemp[k+6];
                            *ptrCb++ = pTemp[k+7];
                        }
                        else {
                            *ptrCb++ = pTemp[k];
                            *ptrCb++ = pTemp[k+1];
                            *ptrCb++ = pTemp[k+2];
                            *ptrCb++ = pTemp[k+3];
                            *ptrCr++ = pTemp[k+4];
                            *ptrCr++ = pTemp[k+5];
                            *ptrCr++ = pTemp[k+6];
                            *ptrCr++ = pTemp[k+7];
                        }
                    }
                }
            }
            osal_memcpy(dstAddrCb+y*dstChromaWidth, pCb+offsetX/div_x, dstChromaWidth);
            totSize += dstChromaWidth;
            osal_memcpy(dstAddrCr+y*dstChromaWidth, pCr+offsetX/div_x, dstChromaWidth);
            totSize += dstChromaWidth;
        }
    }
    else {
        puc = pDst + dstWidth*dstHeight;

        for (y = 0 ; y < dstChromaHeight; y += 1) {
            x = 0;
            pix_addr = GetXY2AXIAddr(&mapCfg, 2, y+(offsetY/div_y), x, chroma_stride, fbSrc);
            rowBufferCb = pCb + (pix_addr - baseCb);
            // CHECK POINT
            if ( fbSrc->format == FORMAT_420_P10_32BIT_MSB ) {
                for (x = 0 ; x < chroma_stride; x += dramBusWidth) {
                    SwapPixelOrder(rowBufferCb+x);
                }
            }
            // CHECK POINT
            osal_memcpy(puc + (y*dstChromaWidth), rowBufferCb+offsetX/div_x, dstChromaWidth);
            totSize += dstChromaWidth;
        }

        puc += dstChromaWidth * dstChromaHeight;
        if ( (fbSrc->format == FORMAT_420_P10_32BIT_LSB || fbSrc->format == FORMAT_420_P10_32BIT_MSB) &&
            p10_32bit_interleave == 1)
        {
        }
        else
        {
            for (y = 0 ; y < dstChromaHeight; y += 1) {
                x = 0;
                pix_addr = GetXY2AXIAddr(&mapCfg, 3, y+(offsetY/div_y), x, chroma_stride, fbSrc);
                //vdi_read_memory(coreIdx, pix_addr, rowBufferCr+x, dramBusWidth,  endian);
                rowBufferCr = pCr + (pix_addr - baseCr);
                // CHECK POINT
                if ( fbSrc->format == FORMAT_420_P10_32BIT_MSB ) {
                    for ( x = 0 ; x < chroma_stride; x += dramBusWidth ) {
                        SwapPixelOrder(rowBufferCr+x);
                    }
                }
                osal_memcpy(puc + (y*dstChromaWidth), rowBufferCr+offsetX/div_x, dstChromaWidth);
                totSize += dstChromaWidth;
            }
        }
    }

    osal_free(pY);
    osal_free(pCb);
    osal_free(pCr);
    osal_free(pCbTemp);

    return totSize;
}

Uint32 StoreYuvImageBurstFormat(
    Uint32          coreIdx,
    FrameBuffer*    fbSrc,
    TiledMapConfig  mapCfg,
    Uint8*          pDst,
    VpuRect         cropRect,
    BOOL            enableCrop
    )
{
    Uint32          y, x;
    Uint32          pix_addr, div_x, div_y, chroma_stride;
    Uint8*          puc;
    Uint8*          rowBufferY, *rowBufferCb, *rowBufferCr;
    Uint32          stride      = fbSrc->stride;
    Uint32          height      = fbSrc->height;
    int             interLeave  = fbSrc->cbcrInterleave;
    BOOL            nv21        = fbSrc->nv21;
    EndianMode      endian      = (EndianMode)fbSrc->endian;
    FrameBufferFormat format    = (FrameBufferFormat)fbSrc->format;
    Uint32          width;
    Uint32          dstWidth, dstHeight;
    Uint32          offsetX, offsetY;
    Uint32          dstChromaHeight;
    Uint32          dstChromaWidth;
    Uint32          bpp=8;
    Uint32          p10_32bit_interleave = 0;
    Int32           productId;
    Int32           dramBusWidth = 8;
        Uint32                  totSize = 0;

    productId = VPU_GetProductId(coreIdx);
    if (PRODUCT_ID_W_SERIES(productId)) {
        dramBusWidth = 16;
    }
    switch (fbSrc->format) {
    case FORMAT_420:
    case FORMAT_420_P10_16BIT_LSB:
    case FORMAT_420_P10_16BIT_MSB:
    case FORMAT_420_P10_32BIT_LSB:
    case FORMAT_420_P10_32BIT_MSB:
    case FORMAT_422:
    case FORMAT_422_P10_16BIT_LSB:
    case FORMAT_422_P10_16BIT_MSB:
    case FORMAT_422_P10_32BIT_LSB:
    case FORMAT_422_P10_32BIT_MSB:
        div_x = 2;
        break;
    default:
        div_x = 1;
    }

    switch (fbSrc->format) {
    case FORMAT_420:
    case FORMAT_420_P10_16BIT_LSB:
    case FORMAT_420_P10_16BIT_MSB:
    case FORMAT_420_P10_32BIT_LSB:
    case FORMAT_420_P10_32BIT_MSB:
    case FORMAT_224:
        div_y = 2;
        break;
    default:
        div_y = 1;
    }

    width     = (enableCrop == TRUE ? cropRect.right - cropRect.left : stride);
    dstHeight = (enableCrop == TRUE ? cropRect.bottom - cropRect.top : height);
    offsetX   = (enableCrop == TRUE ? cropRect.left : 0);
    offsetY   = (enableCrop == TRUE ? cropRect.top  : 0);

    switch (fbSrc->format) {
    case FORMAT_420_P10_16BIT_LSB:
    case FORMAT_420_P10_16BIT_MSB:
        dstWidth = width * 2;
        bpp = 16;
        dstChromaWidth  = dstWidth / div_x;
        dstChromaHeight = dstHeight / div_y;
        chroma_stride   = (stride / div_x);
        break;
    case FORMAT_420_P10_32BIT_LSB:
    case FORMAT_420_P10_32BIT_MSB:
#ifdef DUMP_YUV_WHOLE_DATA
        if (interLeave)
        {
            dstChromaWidth = ((VPU_ALIGN16(width/div_x))*2+11)/12*16;
            dstChromaWidth = VPU_ALIGN16(dstChromaWidth);
            dstChromaHeight = dstHeight / div_y;

            stride = stride;
            chroma_stride = stride;
            dstWidth = (VPU_ALIGN16(width)+11)/12*16;

            interLeave = 0;
        }
        else
        {
            dstChromaWidth = ((VPU_ALIGN16(width/div_x))+11)/12*16;
            dstChromaWidth = VPU_ALIGN16(dstChromaWidth);
            dstChromaHeight = dstHeight / div_y;
            chroma_stride = dstChromaWidth;
            stride    = (VPU_ALIGN16(stride)+11)/12*16;
            dstWidth = (VPU_ALIGN16(dstWidth)+11)/12*16;
        }
        break;
#else
        if (interLeave)
        {
            dstChromaWidth = ((VPU_ALIGN16(width*2/div_x))+11)/12*16;
            dstChromaWidth = VPU_ALIGN16(dstChromaWidth);
            chroma_stride = stride;

            dstChromaWidth = (width+2)/3*4;
            dstChromaHeight = dstHeight / div_y;

            dstWidth = (width+2)/3*4;

            interLeave = 0;
            p10_32bit_interleave = 1;
        }
        else
        {
            dstChromaWidth = ((VPU_ALIGN16(width/div_x))+11)/12*16;
            dstChromaWidth = VPU_ALIGN16(dstChromaWidth);
            chroma_stride = dstChromaWidth;

            dstChromaWidth = (width/2+2)/3*4;
            dstChromaHeight = dstHeight / div_y;

            dstWidth = (width+2)/3*4;
        }
        break;
#endif
    case FORMAT_YUYV:
    case FORMAT_YUYV_P10_16BIT_MSB:
    case FORMAT_YUYV_P10_16BIT_LSB:
    case FORMAT_YUYV_P10_32BIT_MSB:
    case FORMAT_YUYV_P10_32BIT_LSB:
    case FORMAT_YVYU:
    case FORMAT_YVYU_P10_16BIT_MSB:
    case FORMAT_YVYU_P10_16BIT_LSB:
    case FORMAT_YVYU_P10_32BIT_MSB:
    case FORMAT_YVYU_P10_32BIT_LSB:
    case FORMAT_UYVY:
    case FORMAT_UYVY_P10_16BIT_MSB:
    case FORMAT_UYVY_P10_16BIT_LSB:
    case FORMAT_UYVY_P10_32BIT_MSB:
    case FORMAT_UYVY_P10_32BIT_LSB:
    case FORMAT_VYUY:
    case FORMAT_VYUY_P10_16BIT_MSB:
    case FORMAT_VYUY_P10_16BIT_LSB:
    case FORMAT_VYUY_P10_32BIT_MSB:
    case FORMAT_VYUY_P10_32BIT_LSB:
        dstWidth        = stride;
        dstChromaWidth  = 0;
        dstChromaHeight = 0;
        chroma_stride   = 0;
        break;
    default:
        dstWidth = width;
        dstChromaWidth  = width / div_x;
        dstChromaHeight = dstHeight / div_y;
        chroma_stride = (stride / div_x);
        break;
    }

    puc         = pDst;
    rowBufferY  = (Uint8*)osal_malloc(stride);
    if ( !rowBufferY )
        return 0;
    rowBufferCb = (Uint8*)osal_malloc(stride*4);
    if ( !rowBufferCb )
        return 0;
    rowBufferCr = (Uint8*)osal_malloc(stride*2);
    if ( !rowBufferCr )
        return 0;

    for ( y=0 ; y<dstHeight ; y+=1 )
    {
        for ( x=0; x<stride ; x+=dramBusWidth )
        {
            pix_addr = GetXY2AXIAddr(&mapCfg, 0, y+offsetY, x, stride, fbSrc);
            vdi_read_memory(coreIdx, pix_addr, rowBufferY+x, dramBusWidth,  endian);
        }
        osal_memcpy(puc+y*dstWidth, rowBufferY+offsetX, dstWidth);
                totSize += dstWidth;
    }

    if (format == FORMAT_400) {
        osal_free(rowBufferY);
        osal_free(rowBufferCb);
        osal_free(rowBufferCr);
        return totSize;
    }

    if (interLeave == TRUE) {
        Uint8    pTemp[16];
        Uint8*   dstAddrCb;
        Uint8*   dstAddrCr;
        Uint8*   ptrCb, *ptrCr;
        Int32    cbcr_per_2pix=1, k;

        dstAddrCb = pDst + dstWidth*dstHeight;
        dstAddrCr = dstAddrCb + dstChromaWidth*dstChromaHeight;

        cbcr_per_2pix = (format==FORMAT_224||format==FORMAT_444) ? 2 : 1;

        for ( y = 0 ; y < dstChromaHeight; ++y ) {
            ptrCb = rowBufferCb;
            ptrCr = rowBufferCr;
            for ( x = 0 ; x < stride*cbcr_per_2pix ; x += dramBusWidth ) {
                pix_addr = GetXY2AXIAddr(&mapCfg, 2, y+(offsetY/div_y), x, stride, fbSrc);
                vdi_read_memory(coreIdx, pix_addr,  pTemp, dramBusWidth,  endian);
                // CHECK POINT
                if ( fbSrc->format == FORMAT_420_P10_32BIT_MSB )
                    SwapPixelOrder(pTemp);
                // CHECK POINT
                for (k=0; k<dramBusWidth && (x+k) < stride; k+=(2*bpp/8)) {
                    if (bpp == 8) {
                        if (nv21) {
                            *ptrCr++ = pTemp[k];
                            *ptrCb++ = pTemp[k+1];
                        }
                        else {
                            *ptrCb++ = pTemp[k];
                            *ptrCr++ = pTemp[k+1];
                        }
                    }
                    else {
                        if (nv21) {
                            *ptrCr++ = pTemp[k];
                            *ptrCr++ = pTemp[k+1];
                            *ptrCb++ = pTemp[k+2];
                            *ptrCb++ = pTemp[k+3];
                        }
                        else {
                            *ptrCb++ = pTemp[k];
                            *ptrCb++ = pTemp[k+1];
                            *ptrCr++ = pTemp[k+2];
                            *ptrCr++ = pTemp[k+3];
                        }
                    }
                }
            }
            osal_memcpy(dstAddrCb+y*dstChromaWidth, rowBufferCb+offsetX/div_x, dstChromaWidth);
                        totSize += dstChromaWidth;
            osal_memcpy(dstAddrCr+y*dstChromaWidth, rowBufferCr+offsetX/div_x, dstChromaWidth);
                        totSize += dstChromaWidth;
        }
    }
    else {
        puc = pDst + dstWidth*dstHeight;

        for (y = 0 ; y < dstChromaHeight; y += 1) {
            for (x = 0 ; x < chroma_stride; x += dramBusWidth) {
                pix_addr = GetXY2AXIAddr(&mapCfg, 2, y+(offsetY/div_y), x, chroma_stride, fbSrc);
                vdi_read_memory(coreIdx, pix_addr, rowBufferCb+x, dramBusWidth,  endian);
            }
            osal_memcpy(puc + (y*dstChromaWidth), rowBufferCb+offsetX/div_x, dstChromaWidth);
                        totSize += dstChromaWidth;
        }

        puc += dstChromaWidth * dstChromaHeight;
        if ( (fbSrc->format == FORMAT_420_P10_32BIT_LSB || fbSrc->format == FORMAT_420_P10_32BIT_MSB) &&
            p10_32bit_interleave == 1)
        {
        }
        else
        {
            for (y = 0 ; y < dstChromaHeight; y += 1) {
                for ( x = 0 ; x < chroma_stride; x += dramBusWidth ) {
                    pix_addr = GetXY2AXIAddr(&mapCfg, 3, y+(offsetY/div_y), x, chroma_stride, fbSrc);
                    vdi_read_memory(coreIdx, pix_addr, rowBufferCr+x, dramBusWidth,  endian);
                }
                osal_memcpy(puc + (y*dstChromaWidth), rowBufferCr+offsetX/div_x, dstChromaWidth);
                                totSize += dstChromaWidth;
            }
        }
    }

    osal_free(rowBufferY);
    osal_free(rowBufferCb);
    osal_free(rowBufferCr);

    return totSize;
}

Uint8* GetYUVFromFrameBuffer(
    DecHandle       decHandle,
    FrameBuffer*    fb,
    VpuRect         rcFrame,
    Uint32*         retWidth,
    Uint32*         retHeight,
    Uint32*         retBpp,
    size_t*         retSize
    )
{
    Uint32          coreIdx = VPU_HANDLE_CORE_INDEX(decHandle);
    size_t          frameSizeY;                                         // the size of luma
    size_t          frameSizeC;                                         // the size of chroma
    size_t          frameSize;                                          // the size of frame
    Uint32          Bpp = 1;                                            //!<< Byte per pixel
    Uint32          picWidth, picHeight;
    Uint8*          pYuv;
    TiledMapConfig  mapCfg;
    const DecInfo* pDecInfo = VPU_HANDLE_TO_DECINFO(decHandle);
    picWidth  = rcFrame.right - rcFrame.left;
    picHeight = rcFrame.bottom - rcFrame.top;
    if (pDecInfo->scalerEnable == TRUE) {
        rcFrame.right = pDecInfo->scaleWidth;
        rcFrame.bottom = pDecInfo->scaleHeight;
    } else {
        if (STD_VP9 == pDecInfo->openParam.bitstreamFormat) {
            rcFrame.right = VPU_ALIGN8(rcFrame.right);
            rcFrame.bottom = VPU_ALIGN8(rcFrame.bottom);
        }
    }

    CalcYuvSize(fb->format, picWidth, fb->height, fb->cbcrInterleave, &frameSizeY, &frameSizeC, &frameSize, NULL, NULL, NULL);

    switch (fb->format) {
    case FORMAT_422_P10_16BIT_MSB:
    case FORMAT_422_P10_16BIT_LSB:
    case FORMAT_420_P10_16BIT_LSB:
    case FORMAT_420_P10_16BIT_MSB:
        Bpp = 2;
        break;
    case FORMAT_420_P10_32BIT_LSB:
    case FORMAT_420_P10_32BIT_MSB:
        picWidth = (picWidth/3)*4 + ((picWidth%3) ? 4 : 0);
        Bpp = 1;
        break;
    case FORMAT_422:
    case FORMAT_422_P10_32BIT_MSB:
    case FORMAT_422_P10_32BIT_LSB:
        break;
    default:
        Bpp = 1;
        break;
    }
    {
        Int32   temp_picWidth;
        Int32   chromaWidth;

        switch (fb->format) {
        case FORMAT_420_P10_32BIT_LSB:
        case FORMAT_420_P10_32BIT_MSB:
            temp_picWidth = VPU_ALIGN32(picWidth);
            chromaWidth = ((VPU_ALIGN16(temp_picWidth / 2*(1 << 1)) + 2) / 3 * 4);
            frameSizeY = (temp_picWidth + 2) / 3 * 4 * picHeight;
            frameSizeC = chromaWidth * picHeight / 2 * 2;
            frameSize = frameSizeY + frameSizeC;
            break;
        default:
            break;
        }
    }

    if ((pYuv=(Uint8*)osal_malloc((Int32)frameSize)) == NULL) {
        VLOG(ERR, "%s:%d Failed to allocate memory\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    VPU_DecGiveCommand(decHandle, GET_TILEDMAP_CONFIG, &mapCfg);

    if (fb->mapType == LINEAR_FRAME_MAP || fb->mapType == COMPRESSED_FRAME_MAP) {
        *retSize = StoreYuvImageBurstLinear(coreIdx, fb, mapCfg, pYuv, rcFrame, TRUE, pDecInfo);
    } else {
        *retSize = StoreYuvImageBurstFormat(coreIdx, fb, mapCfg, pYuv, rcFrame, TRUE);
    }

    *retWidth  = picWidth;
    *retHeight = picHeight;
    *retBpp    = Bpp;

    return pYuv;
}

void PrepareDecoderTest(
    DecHandle decHandle
    )
{
    UNREFERENCED_PARAMETER(decHandle);
}

int ProcessEncodedBitstreamBurst(Uint32 coreIdx, osal_file_t fp, int targetAddr,
    PhysicalAddress bsBufStartAddr, PhysicalAddress bsBufEndAddr,
    int size, int endian, Comparator comparator)
{
    Uint8 * buffer = 0;
    int room = 0;
    int file_wr_size = 0;

    buffer = (Uint8 *)osal_malloc(size);
    if( ( targetAddr + size ) > (int)bsBufEndAddr )
    {
        room = bsBufEndAddr - targetAddr;
        vdi_read_memory(coreIdx, targetAddr, buffer, room,  endian);
        vdi_read_memory(coreIdx, bsBufStartAddr, buffer+room, (size-room), endian);
    }
    else
    {
        vdi_read_memory(coreIdx, targetAddr, buffer, size, endian);
    }

    if ( comparator) {
        if (Comparator_Act(comparator, buffer, size, 0) == FALSE) {
            osal_free(buffer);
            return 0;
        }
    }

    if (fp) {
        file_wr_size = (Int32)osal_fwrite(buffer, sizeof(Uint8), size, fp);
        osal_fflush(fp);
    }

    osal_free( buffer );

    return file_wr_size;
}

Uint32 CalcScaleDown(
    Uint32   origin,
    Uint32   scaledValue
    )
{
    Uint32   minScaleValue;
    Uint32   retVal;

    minScaleValue = ((origin/8)+7)&~7;
    minScaleValue = (minScaleValue < 8) ? 8 : minScaleValue;
    if (origin == 99) {
        retVal = GetRandom(minScaleValue, origin);
        retVal = VPU_ALIGN8(retVal);
    }
    else {
        if (scaledValue == 0) {
            retVal = origin;
        }
        else {
            if ( scaledValue < origin ) {
                retVal = VPU_ALIGN8(scaledValue);
                if (retVal < minScaleValue) {
                    retVal = minScaleValue;
                }
            } else {
                retVal = origin;
            }
        }
    }

    return retVal;
}

