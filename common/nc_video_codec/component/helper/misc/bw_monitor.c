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

#include "vpuapifunc.h"
#include "main_helper.h"
#include "bw_monitor.h"
#include "debug.h"

#include <string.h>

#define MBYTE                               (1024*1024)

static char picTypeString[10][4] = {"I", "P", "B", "", "", "IDR", "", "", "", ""};//picType enum in vpuapi.h

typedef void*   BWData;

typedef struct _tag_bw_data_struct {
    Uint32     vcpu_bw_rd;
    Uint32     vcpu_bw_wr;
    Uint32     sdma_bw_rd;
    Uint32     sdma_bw_wr;
    Uint32     vcpu_dma_bw_wr;
    Uint32     vcpu_dma_bw_rd;
    Uint32     vcpu_nbdma_bw_wr;
    Uint32     vcpu_nbdma_bw_rd;
    Uint32     prescan_wr;
    Uint32     prescan_rd;

    Uint32     arid_pri_total;
    Uint32     awid_pri_total;
    Uint32     arid_sec_total;
    Uint32     awid_sec_total;
    Uint32     arid_mclk_total;
    Uint32     awid_mclk_total;
    Uint32     total_write;
    Uint32     total_read;
    Uint32     total_bandwidth;
    Uint32     subblock_bw_pri_rd[16];
    Uint32     subblock_bw_pri_wr[16];
    Uint32     subblock_bw_sec_rd[16];
    Uint32     subblock_bw_sec_wr[16];
    Uint32     subblock_bw_mclk_rd[16];
    Uint32     subblock_bw_mclk_wr[16];
} BWGdiData;

typedef struct _tag_BWMonitorops {
    BWCtx   (*allocate)(Uint32);
    void    (*release)(BWCtx);
    void    (*reset)(BWCtx);
    BWData  (*get_data)(BWCtx, Uint32 numCores);
    void    (*print)(BWCtx, BWData, Uint32);
} BWMonitorOps_t;

#define BW_CONTEXT_COMMON_VARS            \
    BWMonitorOps_t* ops;                  \
    Uint32          coreIndex;            \
    Uint32          instanceIndex;        \
    Uint32          productId;            \
    CodecInst*      instance;             \
    Uint32          numFrames;            \
    Uint64          totalPriRead;         \
    Uint64          totalPriWrite;        \
    Uint64          totalSecRead;         \
    Uint64          totalSecWrite;        \
    Uint64          totalProcRead;        \
    Uint64          totalProcWrite;       \
    Uint64          total;                \
    BOOL            enableReportPerFrame; \
    Uint32          bwMode;               \
    osal_file_t     fpBWTotal;            \
    BWData*         data;                 \
    char strLogDir[256];

typedef struct _tag_bw_common_context_struct {
    BW_CONTEXT_COMMON_VARS
} BWCommonCtx;


/************************************************************************/
/* DUMMY                                                                */
/************************************************************************/
typedef struct _tag_dummy_bw_context_struct {
    BW_CONTEXT_COMMON_VARS
} dummy_bw_ctx_t;

static BWCtx
dummy_bw_monitor_allocate(
    Uint32     coreIndex
    )
{
    dummy_bw_ctx_t* context = (dummy_bw_ctx_t*)osal_malloc(sizeof(dummy_bw_ctx_t));

    UNREFERENCED_PARAMETER(coreIndex);

    return (BWCtx)context;
}

static void
dummy_bw_monitor_release(
    BWCtx    ctx
    )
{
    UNREFERENCED_PARAMETER(ctx);

    return;
}

static void
dummy_bw_monitor_reset(
    BWCtx    ctx
    )
{
    UNREFERENCED_PARAMETER(ctx);

    return;
}

static BWData
dummy_bw_monitor_get_data(
    BWCtx    ctx __attribute__((unused)),
    Uint32   numCores __attribute__((unused))
    )
{
    BWGdiData* data;

    data = (BWGdiData*)osal_malloc(sizeof(BWGdiData));

    osal_memset((void*)data, 0x00, sizeof(BWGdiData));

    return (BWData)data;
}

static void
    dummy_bw_monitor_print(
    BWCtx   context __attribute__((unused)),
    BWData  data __attribute__((unused)),
    Uint32  picType __attribute__((unused))
    )
{
    return ;
}

static BWMonitorOps_t s_dummy_ops = {
    dummy_bw_monitor_allocate,
    dummy_bw_monitor_release,
    dummy_bw_monitor_reset,
    dummy_bw_monitor_get_data,
    dummy_bw_monitor_print
};


/************************************************************************/
/* WAVE5 BACKBONE INTERFACE                                             */
/************************************************************************/
#define BWB_BACKBONE_DATA_SIZE                   (17) // total + number of burst length sizes (0~15)

typedef struct _tag_backbone_bw_context_struct {
    BW_CONTEXT_COMMON_VARS

    Uint32 arid_prp_total;
    Uint32 awid_prp_total;
    Uint32 arid_fbd_y_total;
    Uint32 awid_fbc_y_total;
    Uint32 arid_fbd_c_total;
    Uint32 awid_fbc_c_total;
    Uint32 arid_pri_total;
    Uint32 awid_pri_total;
    Uint32 arid_sec_total;
    Uint32 awid_sec_total;
    Uint32 arid_proc_total;
    Uint32 awid_proc_total;
    Uint32 awid_bwb_total;
} BackBoneBwCtx;

typedef struct _tag_backbone_bw_data_struct {
    Uint32 arid_prp[BWB_BACKBONE_DATA_SIZE];
    Uint32 awid_prp[BWB_BACKBONE_DATA_SIZE];
    Uint32 arid_fbd_y[BWB_BACKBONE_DATA_SIZE];
    Uint32 awid_fbc_y[BWB_BACKBONE_DATA_SIZE];
    Uint32 arid_fbd_c[BWB_BACKBONE_DATA_SIZE];
    Uint32 awid_fbc_c[BWB_BACKBONE_DATA_SIZE];
    Uint32 arid_pri[BWB_BACKBONE_DATA_SIZE];
    Uint32 awid_pri[BWB_BACKBONE_DATA_SIZE];
    Uint32 arid_sec[BWB_BACKBONE_DATA_SIZE];
    Uint32 awid_sec[BWB_BACKBONE_DATA_SIZE];
    Uint32 arid_proc[BWB_BACKBONE_DATA_SIZE];
    Uint32 awid_proc[BWB_BACKBONE_DATA_SIZE];
    Uint32 awid_bwb[BWB_BACKBONE_DATA_SIZE];
} BWBackboneData;

static BWCtx
backbone_bw_monitor_allocate(
    Uint32   coreIndex __attribute__((unused))
    )
{
    BackBoneBwCtx* context;

    context = (BackBoneBwCtx*)osal_malloc(sizeof(BackBoneBwCtx));
    osal_memset((void*)context, 0x00, sizeof(BackBoneBwCtx));

    return context;
}

static void
backbone_bw_monitor_release(
    BWCtx    context
    )
{
    BackBoneBwCtx*  ctx = (BackBoneBwCtx*)context;
    Uint64          avgPriRead, avgPriWrite;
    Uint64          avgSecRead, avgSecWrite;
    Uint32          avgFBCWrite;
    Uint32          avgFBDRead;
    Uint64          avgProcRead, avgProcWrite;
    Uint64          avgPRPRead, avgPRPWrite;
    Uint64          avgBWBWrite;
    Uint64          avgWrite;
    Uint64          avgRead;
    Uint64          avg;

    if (ctx == NULL)
        return;

    if (0 == ctx->numFrames) {
        VLOG(ERR, "%s:%d divisor must be a integer(not zero)", __FUNCTION__, __LINE__);
        return;
    }

    avgPriRead   = ctx->arid_pri_total / ctx->numFrames;
    avgPriWrite  = ctx->awid_pri_total / ctx->numFrames;
    avgSecRead   = ctx->arid_sec_total / ctx->numFrames;
    avgSecWrite  = ctx->awid_sec_total / ctx->numFrames;
    avgProcRead  = ctx->arid_proc_total / ctx->numFrames;
    avgProcWrite = ctx->awid_proc_total / ctx->numFrames;
    avgFBCWrite  = (ctx->awid_fbc_y_total+ctx->awid_fbc_c_total) / ctx->numFrames;
    avgFBDRead   = (ctx->arid_fbd_y_total+ctx->arid_fbd_c_total) / ctx->numFrames;
    avgPRPRead   = ctx->arid_prp_total / ctx->numFrames;
    avgPRPWrite  = ctx->awid_prp_total / ctx->numFrames;
    avgBWBWrite  = ctx->awid_bwb_total / ctx->numFrames;
    avgWrite     = (ctx->awid_pri_total+ctx->awid_sec_total+ctx->awid_proc_total+ctx->awid_fbc_y_total+ctx->awid_fbc_c_total+ctx->awid_prp_total+ctx->awid_bwb_total) / ctx->numFrames;
    avgRead      = (ctx->arid_pri_total+ctx->arid_sec_total+ctx->arid_proc_total+ctx->arid_fbd_y_total+ctx->arid_fbd_c_total+ctx->arid_prp_total) / ctx->numFrames;
    avg          = ctx->total / ctx->numFrames;

    if (!ctx->bwMode) {
        osal_fprintf(ctx->fpBWTotal, "------------------------------------------------------------------------------------  -----------------------------------------------------------------  -----------\n");

        osal_fprintf(ctx->fpBWTotal, "AVER.   %10d %10d %10d %10d %10d %10d %10d  %10d %10d %10d %10d %10d %10d  %10d\n",
            avgPriWrite, avgSecWrite, avgFBCWrite, avgPRPWrite, avgProcWrite, avgBWBWrite, avgWrite,
            avgPriRead,  avgSecRead,  avgFBDRead,  avgPRPRead,  avgProcRead,               avgRead,
            avg);
    }
    osal_free(context);
}

static void
backbone_bw_monitor_reset(
    BWCtx    context
    )
{
    BackBoneBwCtx* ctx = (BackBoneBwCtx*)context;

    if (ctx == NULL)
        return;
}

static void
backbone_bw_monitor_print(
    BWCtx   context,
    BWData  data,
    Uint32  picType
    )
{
    BWBackboneData* bdata = (BWBackboneData*)data;
    BWCommonCtx*    common = (BWCommonCtx*)context;
    Uint32          total_wr_bw;
    Uint32          total_rd_bw;
    Uint32          idx = 0;
    Uint32          loopCnt = (common->bwMode == 0) ? 1 : BWB_BACKBONE_DATA_SIZE;

    for (idx =0; idx < loopCnt; idx++) {
        total_wr_bw = bdata->awid_pri[idx] + bdata->awid_sec[idx] + bdata->awid_fbc_y[idx] + bdata->awid_fbc_c[idx] + bdata->awid_prp[idx] + bdata->awid_proc[idx] + bdata->awid_bwb[idx];
        total_rd_bw = bdata->arid_pri[idx] + bdata->arid_sec[idx] + bdata->arid_fbd_y[idx] + bdata->arid_fbd_c[idx] + bdata->arid_prp[idx] + bdata->arid_proc[idx];
        if (0 == idx) {
            if (common->numFrames == 1) {
                osal_fprintf(common->fpBWTotal, "  No.                                    WRITE(B)                                                            READ(B)                                        TOTAL\n");
                osal_fprintf(common->fpBWTotal, "------------------------------------------------------------------------------------  -----------------------------------------------------------------  -----------\n");
                osal_fprintf(common->fpBWTotal, "              PRI        SEC        FBC        PRP       PROC        BWB       TOTAL        PRI       SEC        FBD         PRP       PROC      TOTAL\n");
                if ( common->bwMode == 0 )
                    osal_fprintf(common->fpBWTotal, "------------------------------------------------------------------------------------  -----------------------------------------------------------------  -----------\n");
            }
            if ( common->bwMode != 0 )
                osal_fprintf(common->fpBWTotal, "====================================================================================  =================================================================  ===========\n");
            osal_fprintf(common->fpBWTotal, "%5d %s %10d %10d %10d %10d %10d %10d %10d  %10d %10d %10d %10d %10d %10d  %10d\n", common->numFrames-1, picTypeString[picType],
                bdata->awid_pri[idx], bdata->awid_sec[idx], (bdata->awid_fbc_y[idx]+bdata->awid_fbc_c[idx]), bdata->awid_prp[idx], bdata->awid_proc[idx], bdata->awid_bwb[idx], total_wr_bw,
                bdata->arid_pri[idx], bdata->arid_sec[idx], (bdata->arid_fbd_y[idx]+bdata->arid_fbd_c[idx]), bdata->arid_prp[idx], bdata->arid_proc[idx],                  total_rd_bw,
                (total_wr_bw + total_rd_bw));

        } else {
            if (1 == idx) {
                osal_fprintf(common->fpBWTotal, "------------------------------------------------------------------------------------  -----------------------------------------------------------------  -----------\n");
            }
            osal_fprintf(common->fpBWTotal, "%5d %s %10d %10d %10d %10d %10d %10d %10d  %10d %10d %10d %10d %10d %10d  %10d\n", idx-1, picTypeString[picType],
                bdata->awid_pri[idx], bdata->awid_sec[idx], (bdata->awid_fbc_y[idx]+bdata->awid_fbc_c[idx]), bdata->awid_prp[idx], bdata->awid_proc[idx], bdata->awid_bwb[idx], total_wr_bw,
                bdata->arid_pri[idx], bdata->arid_sec[idx], (bdata->arid_fbd_y[idx]+bdata->arid_fbd_c[idx]), bdata->arid_prp[idx], bdata->arid_proc[idx],                  total_rd_bw,
                (total_wr_bw + total_rd_bw));
        }
        osal_fflush(common->fpBWTotal);
    }
    if ( common->bwMode != 0 )
        osal_fprintf(common->fpBWTotal, "------------------------------------------------------------------------------------  -----------------------------------------------------------------  -----------\n");
}

static BWData
backbone_bw_monitor_get_data(
    BWCtx    context,
    Uint32   numCores __attribute__((unused))
    )
{
    BackBoneBwCtx*  ctx   = (BackBoneBwCtx*)context;
    BWBackboneData* idata = NULL;
    VPUBWData       bwdata;

    memset(&bwdata, 0, sizeof(VPUBWData));

    RetCode         ret = RETCODE_FAILURE;
    Uint32          prevTotal;
    Uint32          idx = 0;
    Uint32          loopCnt = (bwdata.bwMode == 0) ? 1 : BWB_BACKBONE_DATA_SIZE;

    idata = (BWBackboneData*)osal_malloc(sizeof(BWBackboneData));

    for (idx = 0; idx < loopCnt; idx++) {
        if (0 < idx) {
            bwdata.bwMode = 1;
            bwdata.burstLengthIdx = idx - 1;
        }
        if (ctx->instance->isDecoder == TRUE) {
            ret = VPU_DecGiveCommand(ctx->instance, GET_BANDWIDTH_REPORT, (void*)&bwdata);
        }
        else {
            ret = VPU_EncGiveCommand(ctx->instance, GET_BANDWIDTH_REPORT, (void*)&bwdata);
        }
        if (ret != RETCODE_SUCCESS) {
            if (NULL != idata) {
                osal_free(idata);
            }
            VLOG(ERR, "%s:%d Failed to VPU_EncGiveCommand(ENC_GET_BW_REPORT), ret(%d)\n", __FUNCTION__, __LINE__, ret);
            return NULL;
        }

        idata->arid_prp[idx]   = bwdata.prpBwRead;
        idata->awid_prp[idx]   = bwdata.prpBwWrite;
        idata->arid_fbd_y[idx] = bwdata.fbdYRead;
        idata->awid_fbc_y[idx] = bwdata.fbcYWrite;
        idata->arid_fbd_c[idx] = bwdata.fbdCRead;
        idata->awid_fbc_c[idx] = bwdata.fbcCWrite;
        idata->arid_pri[idx]   = bwdata.priBwRead;
        idata->awid_pri[idx]   = bwdata.priBwWrite;
        idata->arid_sec[idx]   = bwdata.secBwRead;
        idata->awid_sec[idx]   = bwdata.secBwWrite;
        idata->arid_proc[idx]  = bwdata.procBwRead;
        idata->awid_proc[idx]  = bwdata.procBwWrite;
        idata->awid_bwb[idx]   = bwdata.bwbBwWrite;

        if (0 == idx) {
            prevTotal   = (Uint32)ctx->total;
            ctx->total += idata->arid_prp[idx];
            ctx->total += idata->awid_prp[idx];
            ctx->total += idata->arid_fbd_y[idx];
            ctx->total += idata->awid_fbc_y[idx];
            ctx->total += idata->arid_fbd_c[idx];
            ctx->total += idata->awid_fbc_c[idx];
            ctx->total += idata->arid_pri[idx];
            ctx->total += idata->awid_pri[idx];
            ctx->total += idata->arid_sec[idx];
            ctx->total += idata->awid_sec[idx];
            ctx->total += idata->arid_proc[idx];
            ctx->total += idata->awid_proc[idx];
            ctx->total += idata->awid_bwb[idx];

            if (prevTotal == ctx->total) {
                // VPU didn't decode a frame.
                return NULL;
            }
            ctx->arid_prp_total   += idata->arid_prp[idx]  ;
            ctx->awid_prp_total   += idata->awid_prp[idx]  ;
            ctx->arid_fbd_y_total += idata->arid_fbd_y[idx];
            ctx->arid_fbd_c_total += idata->arid_fbd_c[idx];
            ctx->awid_fbc_y_total += idata->awid_fbc_y[idx];
            ctx->awid_fbc_c_total += idata->awid_fbc_c[idx];
            ctx->arid_pri_total   += idata->arid_pri[idx]  ;
            ctx->awid_pri_total   += idata->awid_pri[idx]  ;
            ctx->arid_sec_total   += idata->arid_sec[idx]  ;
            ctx->awid_sec_total   += idata->awid_sec[idx]  ;
            ctx->arid_proc_total  += idata->arid_proc[idx] ;
            ctx->awid_proc_total  += idata->awid_proc[idx] ;
            ctx->awid_bwb_total   += idata->awid_bwb[idx]  ;
        }
    }

    return (BWData)idata;
}

static BWMonitorOps_t s_wave_backbone_ops = {
    backbone_bw_monitor_allocate,
    backbone_bw_monitor_release,
    backbone_bw_monitor_reset,
    backbone_bw_monitor_get_data,
    backbone_bw_monitor_print
};

/************************************************************************/
/*                                                                      */
/************************************************************************/
BWCtx
BWMonitorSetup(
    CodecInst*  instance,
    BOOL        perFrame,
    Uint32      bwMode,
    char*       strLogDir
    )
{
    Uint32          coreIndex;
    Uint32          productId;
    Uint32          instIndex;
    BWCommonCtx*    common;
    BWMonitorOps_t* bwOps;
    osal_file_t     fp = NULL;
    char            path[256];

    coreIndex = instance->coreIdx;
    productId = instance->productId;
    instIndex = instance->instIndex;

    switch (productId) {
    case PRODUCT_ID_521:
        bwOps = &s_wave_backbone_ops;
        break;
    default:
        bwOps = &s_dummy_ops;
        break;
    }

    if (strLogDir) {
        sprintf(path, "./report/bw/%s/", strLogDir);
        MkDir(path);
        sprintf(path, "./report/bw/%s/report_bandwidth_%d_%d.txt", strLogDir, coreIndex, instIndex);
    }
    else {
        sprintf(path, "./report/bw/report_bandwidth_%d_%d.txt", coreIndex, instIndex);
        MkDir("./report/bw/");
    }

    if ((fp=osal_fopen(path, "w")) == NULL) {
        VLOG(ERR, "Failed to open %s\n", path);
    }

    common = (BWCommonCtx*)bwOps->allocate(coreIndex);
    common->ops             = bwOps;
    common->coreIndex       = coreIndex;
    common->instanceIndex   = instIndex;
    common->productId       = instance->productId;
    common->instance        = instance;
    common->fpBWTotal       = fp;
    common->enableReportPerFrame = perFrame;
    common->bwMode          = bwMode;
    if (strLogDir) {
        sprintf(common->strLogDir, "%s", strLogDir);
    }
    else {
        osal_memset(common->strLogDir, 0x00, sizeof(common->strLogDir)*sizeof(char));
    }

    return common;
}

void
BWMonitorReset(
    BWCtx    context
    )
{
    BWCommonCtx* common = (BWCommonCtx*)context;

    if (common == NULL)
        return;

    common->ops->reset(context);
}

void
BWMonitorRelease(
    BWCtx    context
    )
{
    BWCommonCtx* common = (BWCommonCtx*)context;

    if (common == NULL)
        return;

    common->ops->release(context);

    if (common->fpBWTotal)
        osal_fclose(common->fpBWTotal);
}

void
BWMonitorUpdate(
    BWCtx       context,
    Uint32      numCores
    )
{
    BWCommonCtx* common = (BWCommonCtx*)context;

    if (common == NULL || common->fpBWTotal == NULL) {
        return;
    }

    if (((common->data=(BWData*)(common->ops->get_data(context, numCores)))) == NULL) {
        return;
    }

    common->numFrames++;
}

void
    BWMonitorUpdatePrint(
    BWCtx       context,
    Uint32      picType
    )
{
    BWCommonCtx* common = (BWCommonCtx*)context;

    if (common == NULL || common->fpBWTotal == NULL ) {
        return;
    }
    if (common->data) {
        common->ops->print(context, common->data, picType);
        osal_free(common->data);
        common->data = NULL;
    }

    osal_free(common->data);
}

