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

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "config.h"
#include "main_helper.h"

enum {
    CNMQC_ENV_NONE,
    CNMQC_ENV_GDBSERVER,            /*!<< It executes gdb server in order to debug F/W on the C&M FPGA env. */
    CNMQC_ENV_MAX,
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void InitializeDebugEnv(Uint32 options);
extern void ReleaseDebugEnv(void);
extern void ExecuteDebugger(void);

void ChekcAndPrintDebugInfo(VpuHandle handle, BOOL isEnc, RetCode result);

void PrintDecVpuStatus(
    DecHandle   handle
    );

void PrintEncVpuStatus(
    EncHandle   handle
    );

void PrintMemoryAccessViolationReason(
    Uint32          core_idx,
    void            *outp
    );

#define VCORE_DBG_ADDR(__vCoreIdx)      0x8000+(0x1000*__vCoreIdx) + 0x300
#define VCORE_DBG_DATA(__vCoreIdx)      0x8000+(0x1000*__vCoreIdx) + 0x304
#define VCORE_DBG_READY(__vCoreIdx)     0x8000+(0x1000*__vCoreIdx) + 0x308

void WriteRegVCE(
    Uint32   core_idx,
    Uint32   vce_core_idx,
    Uint32   vce_addr,
    Uint32   udata
    );

Uint32 ReadRegVCE(
    Uint32 core_idx,
    Uint32 vce_core_idx,
    Uint32 vce_addr
    );

extern char dumpTime[200];
#define HEXDUMP_COLS 16
void DisplayHex(void *mem, Uint32 len, const char* name);


RetCode PrintVpuProductInfo(
    Uint32      core_idx,
    VpuAttr*    productInfo
    );


Int32 HandleDecInitSequenceError(
    DecHandle       handle,
    Uint32          productId,
    DecOpenParam*   openParam,
    DecInitialInfo* seqInfo,
    RetCode         apiErrorCode
    );

void HandleDecoderError(
    DecHandle       handle,
    Uint32          frameIdx,
    DecOutputInfo*  outputInfo
    );

void DumpMemory(const char* path, Uint32 coreIdx, PhysicalAddress addr, Uint32 size, EndianMode endian);
void DumpCodeBuffer(const char* path);
void DumpBitstreamBuffer(Uint32 coreIdx, PhysicalAddress addr, Uint32 size, EndianMode endian, const char* prefix);
void DumpColMvBuffers(Uint32 coreIdx, const DecInfo* pDecInfo);

void HandleEncoderError(
    EncHandle       handle,
    Uint32          encPicCnt,
    EncOutputInfo*  outputInfo
    );
Uint32 SetEncoderTimeout(
    int width,
    int height
    );

void print_busy_timeout_status(
    Uint32 coreIdx,
    Uint32 product_code,
    Uint32 pc
    );

void wave5xx_vcpu_status (
    unsigned long coreIdx
    );

void vdi_print_vpu_status(
    unsigned long coreIdx
    );

void wave5xx_bpu_status(
    Uint32 coreIdx
    );

void vdi_print_vcore_status(
    Uint32 coreIdx
    );

void vdi_print_vpu_status_dec(
    unsigned long coreIdx
    );

void vdi_print_vpu_status_enc(
    unsigned long coreIdx
    );



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _DEBUG_H_ */

