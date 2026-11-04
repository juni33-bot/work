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

#include "config.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include "wave5_regdefine.h"
#include "wave5.h"
#if defined(PLATFORM_LINUX) || defined(PLATFORM_QNX)
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#endif
#include "main_helper.h"
#include "misc/debug.h"
#include "vpuconfig.h"


enum { False, True };

void ExecuteDebugger(void)
{
    VLOG(INFO, "Starting the debugger....\n");
}

void InitializeDebugEnv(Uint32  option)
{
    switch(option) {
    case CNMQC_ENV_GDBSERVER: ExecuteDebugger(); break;
    default: break;
    }
}

void ReleaseDebugEnv(void)
{
}

Int32 checkLineFeedInHelp(
    struct OptionExt *opt
    )
{
    int i;

    for (i=0;i<MAX_GETOPT_OPTIONS;i++) {
        if (opt[i].name==NULL)
            break;
        if (!strstr(opt[i].help, "\n")) {
            VLOG(INFO, "(%s) doesn't have \\n in options struct in main function. please add \\n\n", opt[i].help);
            return FALSE;
        }
    }
    return TRUE;
}

RetCode PrintVpuProductInfo(
    Uint32   coreIdx,
    VpuAttr* productInfo
    )
{
    RetCode ret = RETCODE_SUCCESS;

    if ((ret = VPU_GetProductInfo(coreIdx, productInfo)) != RETCODE_SUCCESS) {
        return ret;
    }

    // VLOG(INFO, "\n==========================\n");
    // VLOG(INFO, "VPU coreNum : [%d]\n", coreIdx);
    // VLOG(INFO, "Firmware : CustomerCode: %04x | version : rev.%d\n", productInfo->customerId, productInfo->fwVersion);
    // VLOG(INFO, "Hardware : %04x\n", productInfo->productId);
    // VLOG(INFO, "API      : %d.%d.%d\n", API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);
    // VLOG(INFO, "==========================\n");
    // if (PRODUCT_ID_W_SERIES(productInfo->productId))
    // {
    //     VLOG(INFO, "productId        : %08x\n", productInfo->productId);
    //     VLOG(INFO, "fwVersion        : %08x(r%d)\n", productInfo->fwVersion, productInfo->fwVersion);
    //     VLOG(INFO, "productName      : %s%4x\n", productInfo->productName, productInfo->productVersion);
    //     if ( verbose == TRUE )
    //     {
    //         Uint32 stdDef0          = productInfo->hwConfigDef0;
    //         Uint32 stdDef1          = productInfo->hwConfigDef1;
    //         Uint32 confFeature      = productInfo->hwConfigFeature;
    //         BOOL supportDownScaler  = FALSE;
    //         BOOL supportAfbce       = FALSE;
    //         char ch_ox[2]           = {'X', 'O'};

    //         VLOG(INFO, "==========================\n");
    //         VLOG(INFO, "stdDef0           : %08x\n", stdDef0);
    //         /* checking ONE AXI BIT FILE */
    //         VLOG(INFO, "MAP CONVERTER REG : %d\n", (stdDef0>>31)&1);
    //         VLOG(INFO, "MAP CONVERTER SIG : %d\n", (stdDef0>>30)&1);
    //         VLOG(INFO, "PVRIC FBC EN      : %d\n", (stdDef0>>27)&1);
    //         VLOG(INFO, "PVRIC FBC ID      : %d\n", (stdDef0>>24)&7);
    //         VLOG(INFO, "SCALER 2ALIGNED   : %d\n", (stdDef0>>23)&1);
    //         VLOG(INFO, "VCORE BACKBONE    : %d\n", (stdDef0>>22)&1);
    //         VLOG(INFO, "STD SWITCH EN     : %d\n", (stdDef0>>21)&1);
    //         VLOG(INFO, "BG_DETECT         : %d\n", (stdDef0>>20)&1);
    //         VLOG(INFO, "3D NR             : %d\n", (stdDef0>>19)&1);
    //         VLOG(INFO, "ONE-PORT AXI      : %d\n", (stdDef0>>18)&1);
    //         VLOG(INFO, "2nd AXI           : %d\n", (stdDef0>>17)&1);
    //         VLOG(INFO, "GDI               : %d\n", !((stdDef0>>16)&1));//no-gdi
    //         VLOG(INFO, "AFBC              : %d\n", (stdDef0>>15)&1);
    //         VLOG(INFO, "AFBC VERSION      : %d\n", (stdDef0>>12)&7);
    //         VLOG(INFO, "FBC               : %d\n", (stdDef0>>11)&1);
    //         VLOG(INFO, "FBC  VERSION      : %d\n", (stdDef0>>8)&7);
    //         VLOG(INFO, "SCALER            : %d\n", (stdDef0>>7)&1);
    //         VLOG(INFO, "SCALER VERSION    : %d\n", (stdDef0>>4)&7);
    //         VLOG(INFO, "BWB               : %d\n", (stdDef0>>2)&1);
    //         VLOG(INFO, "==========================\n");
    //         VLOG(INFO, "stdDef1           : %08x\n", stdDef1);
    //         VLOG(INFO, "REF RINGBUFFER    : %d\n", (stdDef1>>31)&1);
    //         VLOG(INFO, "SKIP VLC EN       : %d\n", (stdDef1>>30)&1);
    //         VLOG(INFO, "CyclePerTick      : %d\n", (stdDef1>>27)&1); //0:32768, 1:256
    //         VLOG(INFO, "MULTI CORE EN     : %d\n", (stdDef1>>26)&1);
    //         VLOG(INFO, "GCU EN            : %d\n", (stdDef1>>25)&1);
    //         VLOG(INFO, "CU REPORT         : %d\n", (stdDef1>>24)&1);
    //         VLOG(INFO, "RDO REPORT        : %d\n", (stdDef1>>23)&1);
    //         VLOG(INFO, "MV HISTOGRAM      : %d\n", (stdDef1>>22)&1);
    //         VLOG(INFO, "INPUT RINGBUFFER  : %d\n", (stdDef1>>21)&1);
    //         VLOG(INFO, "ENT OPT ENABLED   : %d\n", (stdDef1>>20)&1);
    //         VLOG(INFO, "VCORE ID 3        : %d\n", (stdDef1>>19)&1);
    //         VLOG(INFO, "VCORE ID 2        : %d\n", (stdDef1>>18)&1);
    //         VLOG(INFO, "VCORE ID 1        : %d\n", (stdDef1>>17)&1);
    //         VLOG(INFO, "VCORE ID 0        : %d\n", (stdDef1>>16)&1);
    //         VLOG(INFO, "BW OPT            : %d\n", (stdDef1>>15)&1);
    //         VLOG(INFO, "CODEC STD AV1     : %d\n", (stdDef1>>4)&1);
    //         VLOG(INFO, "CODEC STD AVS2    : %d\n", (stdDef1>>3)&1);
    //         VLOG(INFO, "CODEC STD AVC     : %d\n", (stdDef1>>2)&1);
    //         VLOG(INFO, "CODEC STD VP9     : %d\n", (stdDef1>>1)&1);
    //         VLOG(INFO, "CODEC STD HEVC    : %d\n", (stdDef1>>0)&1);

    //         VLOG(INFO, "==========================\n");
    //         VLOG(INFO, "confFeature       : %08x\n", confFeature);
    //         if ( productInfo->hwConfigRev > 167455 ) {//20190321
    //             VLOG(INFO, "AVC  ENC MAIN10   : %d\n", (confFeature>>11)&1);
    //             VLOG(INFO, "AVC  ENC MAIN     : %d\n", (confFeature>>10)&1);
    //             VLOG(INFO, "AVC  DEC MAIN10   : %d\n", (confFeature>>9)&1);
    //             VLOG(INFO, "AVC  DEC MAIN     : %d\n", (confFeature>>8)&1);
    //         }
    //         else {
    //             VLOG(INFO, "AVC  ENC          : %d\n", (confFeature>>9)&1);
    //             VLOG(INFO, "AVC  DEC          : %d\n", (confFeature>>8)&1);
    //         }
    //         VLOG(INFO, "AV1  ENC PROF     : %d\n", (confFeature>>14)&1);
    //         VLOG(INFO, "AV1  DEC HIGH     : %d\n", (confFeature>>13)&1);
    //         VLOG(INFO, "AV1  DEC MAIN     : %d\n", (confFeature>>12)&1);

    //         VLOG(INFO, "AVC  ENC MAIN10   : %d\n", (confFeature>>11)&1);
    //         VLOG(INFO, "AVC  ENC MAIN     : %d\n", (confFeature>>10)&1);
    //         VLOG(INFO, "AVC  DEC MAIN10   : %d\n", (confFeature>>9)&1);
    //         VLOG(INFO, "AVC  DEC MAIN     : %d\n", (confFeature>>8)&1);

    //         VLOG(INFO, "VP9  ENC Profile2 : %d\n", (confFeature>>7)&1);
    //         VLOG(INFO, "VP9  ENC Profile0 : %d\n", (confFeature>>6)&1);
    //         VLOG(INFO, "VP9  DEC Profile2 : %d\n", (confFeature>>5)&1);
    //         VLOG(INFO, "VP9  DEC Profile0 : %d\n", (confFeature>>4)&1);
    //         VLOG(INFO, "HEVC ENC MAIN10   : %d\n", (confFeature>>3)&1);
    //         VLOG(INFO, "HEVC ENC MAIN     : %d\n", (confFeature>>2)&1);
    //         VLOG(INFO, "HEVC DEC MAIN10   : %d\n", (confFeature>>1)&1);
    //         VLOG(INFO, "HEVC DEC MAIN     : %d\n", (confFeature>>0)&1);
    //         VLOG(INFO, "==========================\n");
    //         VLOG(INFO, "configDate        : %d\n", productInfo->hwConfigDate);
    //         VLOG(INFO, "HW version        : r%d\n", productInfo->hwConfigRev);

    //         supportDownScaler = (BOOL)((stdDef0>>7)&1);
    //         supportAfbce      = (BOOL)((stdDef0>>15)&1);

    //         VLOG (INFO, "------------------------------------\n");
    //         VLOG (INFO, "VPU CONF| SCALER | AFBCE  |\n");
    //         VLOG (INFO, "        |   %c    |    %c   |\n", ch_ox[supportDownScaler], ch_ox[supportAfbce]);
    //         VLOG (INFO, "------------------------------------\n");
    //     }
    //     else {
    //         VLOG(INFO, "==========================\n");
    //         VLOG(INFO, "stdDef0          : %08x\n", productInfo->hwConfigDef0);
    //         VLOG(INFO, "stdDef1          : %08x\n", productInfo->hwConfigDef1);
    //         VLOG(INFO, "confFeature      : %08x\n", productInfo->hwConfigFeature);
    //         VLOG(INFO, "configDate       : %08x\n", productInfo->hwConfigDate);
    //         VLOG(INFO, "configRevision   : %08x\n", productInfo->hwConfigRev);
    //         VLOG(INFO, "configType       : %08x\n", productInfo->hwConfigType);
    //         VLOG(INFO, "==========================\n");
    //     }
    // }
    return ret;
}

#define FIO_DBG_IRB_ADDR    0x8074
#define FIO_DBG_IRB_DATA    0x8078
#define FIO_DBG_IRB_STATUS  0x807C
void vdi_irb_write_register(
    unsigned long coreIdx,
    unsigned int  vcore_idx,
    unsigned int  irb_addr,
    unsigned int  irb_data)
{
    vdi_fio_write_register(coreIdx, FIO_DBG_IRB_DATA + 0x1000*vcore_idx, irb_data);
    vdi_fio_write_register(coreIdx, FIO_DBG_IRB_ADDR + 0x1000*vcore_idx, irb_addr);
}

unsigned int vdi_irb_read_register(
    unsigned long coreIdx,
    unsigned int  vcore_idx,
    unsigned int  irb_addr
    )
{
    unsigned int irb_rdata = 0;

    unsigned int irb_rd_cmd = 0;

    irb_rd_cmd = (1<<20)| (1<<16) | irb_addr; // {dbgMode, Read, Addr}
    vdi_fio_write_register(coreIdx, FIO_DBG_IRB_ADDR + 0x1000*vcore_idx, irb_rd_cmd);
    while((vdi_fio_read_register(coreIdx, FIO_DBG_IRB_STATUS + 0x1000*vcore_idx) & 0x1) == 0);

    irb_rdata = vdi_fio_read_register(coreIdx, FIO_DBG_IRB_DATA + 0x1000*vcore_idx);
    return irb_rdata;
}

char dumpTime[200];

/******************************************************************************
* help function                                                               *
******************************************************************************/

void DisplayHex(void *mem, Uint32 len, const char* name)
{
    Uint32   i, j;

    VLOG(INFO, "  addr  : 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F %s \n", name);
    for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++) {
        /* print offset */
        if(i % HEXDUMP_COLS == 0) {
            VLOG(INFO, "0x%06x: ", i);
        }

        /* print hex data */
        if(i < len) {
            VLOG(INFO, "%02x ", 0xFF & ((char*)mem)[i]);
        }
        else /* end of block, just aligning for ASCII dump */ {
            VLOG(INFO, "   ");
        }

        /* print ASCII dump */
        if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1)) {
            for(j = i - (HEXDUMP_COLS - 1); j <= i; j++) {
                if(j >= len) { /* end of block, not really printing */
                    VLOG(INFO, " ");
                }
                else if(isprint(((char*)mem)[j])) { /* printable char */
                    VLOG(INFO, "%c", 0xFF & ((char*)mem)[j]);
                }
                else { /* other char */
                    VLOG(INFO, ".");
                }
            }
            VLOG(INFO, "\n");
        }
    }
}


Uint32 ReadRegVCE(
    Uint32 coreIdx,
    Uint32 vce_core_idx,
    Uint32 vce_addr
    )
{//lint !e18
    int     vcpu_reg_addr;
    int     udata;
    int     vce_core_base = 0x8000 + 0x1000*vce_core_idx;

    SetClockGate(coreIdx, 1);
    vdi_fio_write_register(coreIdx, VCORE_DBG_READY(vce_core_idx), 0);

    vcpu_reg_addr = vce_addr >> 2;

    vdi_fio_write_register(coreIdx, VCORE_DBG_ADDR(vce_core_idx),vcpu_reg_addr + vce_core_base);

    if (vdi_fio_read_register(0, VCORE_DBG_READY(vce_core_idx)) == 1)
        udata= vdi_fio_read_register(0, VCORE_DBG_DATA(vce_core_idx));
    else {
        VLOG(ERR, "failed to read VCE register: %d, 0x%04x\n", vce_core_idx, vce_addr);
        udata = -2;//-1 can be a valid value
    }

    SetClockGate(coreIdx, 0);
    return udata;
}

void WriteRegVCE(
    Uint32   coreIdx,
    Uint32   vce_core_idx,
    Uint32   vce_addr,
    Uint32   udata
    )
{
    int vcpu_reg_addr;

    SetClockGate(coreIdx, 1);

    vdi_fio_write_register(coreIdx, VCORE_DBG_READY(vce_core_idx),0);

    vcpu_reg_addr = vce_addr >> 2;

    vdi_fio_write_register(coreIdx, VCORE_DBG_DATA(vce_core_idx),udata);
    vdi_fio_write_register(coreIdx, VCORE_DBG_ADDR(vce_core_idx),(vcpu_reg_addr) & 0x00007FFF);

    while (vdi_fio_read_register(0, VCORE_DBG_READY(vce_core_idx)) == 0xffffffff) {
        VLOG(ERR, "failed to write VCE register: 0x%04x\n", vce_addr);
    }
    SetClockGate(coreIdx, 0);
}

#define VCE_DEC_CHECK_SUM0         0x110
#define VCE_DEC_CHECK_SUM1         0x114
#define VCE_DEC_CHECK_SUM2         0x118
#define VCE_DEC_CHECK_SUM3         0x11C
#define VCE_DEC_CHECK_SUM4         0x120
#define VCE_DEC_CHECK_SUM5         0x124
#define VCE_DEC_CHECK_SUM6         0x128
#define VCE_DEC_CHECK_SUM7         0x12C
#define VCE_DEC_CHECK_SUM8         0x130
#define VCE_DEC_CHECK_SUM9         0x134
#define VCE_DEC_CHECK_SUM10        0x138
#define VCE_DEC_CHECK_SUM11        0x13C

#define READ_BIT(val,high,low) ((((high)==31) && ((low) == 0)) ?  (val) : (((val)>>(low)) & (((1<< ((high)-(low)+1))-1))))


static void	DisplayVceEncDebugCommon521(int coreIdx, int vcore_idx, int set_mode, int debug0, int debug1, int debug2)
{
    int reg_val;
    VLOG(ERR, "---------------Common Debug INFO-----------------\n");

    WriteRegVCE(coreIdx, vcore_idx, set_mode,0 );

    reg_val = ReadRegVCE(coreIdx, vcore_idx, debug0);
    VLOG(ERR,"\t- subblok_done      :  0x%x\n", READ_BIT(reg_val,30,23));
    VLOG(ERR,"\t- pipe_on[4]        :  0x%x\n", READ_BIT(reg_val,20,20));
    VLOG(ERR,"\t- cur_s2ime         :  0x%x\n", READ_BIT(reg_val,19,16));
    VLOG(ERR,"\t- cur_pipe          :  0x%x\n", READ_BIT(reg_val,15,12));
    VLOG(ERR,"\t- pipe_on[3:0]      :  0x%x\n", READ_BIT(reg_val,11, 8));
    VLOG(ERR,"\t- i_grdma_debug_reg :  0x%x\n", READ_BIT(reg_val, 5, 3));
    VLOG(ERR,"\t- cur_ar_tbl_w_fsm  :  0x%x\n", READ_BIT(reg_val, 2, 0));

    reg_val = ReadRegVCE(coreIdx, vcore_idx, debug1);
    VLOG(ERR,"\t- i_avc_rdo_debug :  0x%x\n", READ_BIT(reg_val,31,31));
    VLOG(ERR,"\t- curbuf_prp      :  0x%x\n", READ_BIT(reg_val,28,25));
    VLOG(ERR,"\t- curbuf_s2       :  0x%x\n", READ_BIT(reg_val,24,21));
    VLOG(ERR,"\t- curbuf_s0       :  0x%x\n", READ_BIT(reg_val,20,17));
    VLOG(ERR,"\t- cur_s2ime_sel   :  0x%x\n", READ_BIT(reg_val,16,16));
    VLOG(ERR,"\t- cur_mvp         :  0x%x\n", READ_BIT(reg_val,15,14));
    VLOG(ERR,"\t- cmd_ready       :  0x%x\n", READ_BIT(reg_val,13,13));
    VLOG(ERR,"\t- rc_ready        :  0x%x\n", READ_BIT(reg_val,12,12));
    VLOG(ERR,"\t- pipe_cmd_cnt    :  0x%x\n", READ_BIT(reg_val,11, 9));
    VLOG(ERR,"\t- subblok_done    :  LF_PARAM 0x%x SFU 0x%x LF 0x%x RDO 0x%x IMD 0x%x FME 0x%x IME 0x%x\n",
        READ_BIT(reg_val, 6, 6), READ_BIT(reg_val, 5, 5), READ_BIT(reg_val, 4, 4), READ_BIT(reg_val, 3, 3),
        READ_BIT(reg_val, 2, 2), READ_BIT(reg_val, 1, 1), READ_BIT(reg_val, 0, 0));

    reg_val = ReadRegVCE(coreIdx, vcore_idx, debug2);
    //VLOG(ERR,"\t- reserved          :  0x%x\n", READ_BIT(reg_val,31, 23));
    VLOG(ERR,"\t- cur_prp_dma_state :  0x%x\n", READ_BIT(reg_val,22, 20));
    VLOG(ERR,"\t- cur_prp_state     :  0x%x\n", READ_BIT(reg_val,19, 18));
    VLOG(ERR,"\t- main_ctu_xpos     :  0x%x\n", READ_BIT(reg_val,17,  9));
    VLOG(ERR,"\t- main_ctu_ypos     :  0x%x(HEVC:*32, AVC:*16)\n", READ_BIT(reg_val, 8,  0));

    reg_val = ReadRegVCE(coreIdx, vcore_idx, 0x0ae8);
    VLOG(ERR,"\t- sub_frame_sync_ypos_valid :  0x%x\n", READ_BIT(reg_val,0,0));
    VLOG(ERR,"\t- sub_frame_sync_ypos       :  0x%x\n", READ_BIT(reg_val,13,1));
}

static void	DisplayVceEncDebugMode(int core_idx, int vcore_idx, int set_mode, int* debug)
{
    int reg_val;
    int i;
    VLOG(ERR,"----------- MODE 2 : ----------\n");

    WriteRegVCE(core_idx,vcore_idx, set_mode, 2);

    reg_val = ReadRegVCE(core_idx, vcore_idx, debug[7]);
    VLOG(ERR,"\t- s2fme_info_full    :  0x%x\n", READ_BIT(reg_val,26,26));
    VLOG(ERR,"\t- ime_cmd_ref_full   :  0x%x\n", READ_BIT(reg_val,25,25));
    VLOG(ERR,"\t- ime_cmd_ctb_full   :  0x%x\n", READ_BIT(reg_val,24,24));
    VLOG(ERR,"\t- ime_load_info_full :  0x%x\n", READ_BIT(reg_val,23,23));
    VLOG(ERR,"\t- mvp_nb_info_full   :  0x%x\n", READ_BIT(reg_val,22,22));
    VLOG(ERR,"\t- ime_final_mv_full  :  0x%x\n", READ_BIT(reg_val,21,21));
    VLOG(ERR,"\t- ime_mv_full        :  0x%x\n", READ_BIT(reg_val,20,20));
    VLOG(ERR,"\t- cur_fme_fsm[3:0]   :  0x%x\n", READ_BIT(reg_val,19,16));
    VLOG(ERR,"\t- cur_s2me_fsm[3:0]  :  0x%x\n", READ_BIT(reg_val,15,12));
    VLOG(ERR,"\t- cur_s2mvp_fsm[3:0] :  0x%x\n", READ_BIT(reg_val,11, 8));
    VLOG(ERR,"\t- cur_ime_fsm[3:0]   :  0x%x\n", READ_BIT(reg_val, 7, 4));
    VLOG(ERR,"\t- cur_sam_fsm[3:0]   :  0x%x\n", READ_BIT(reg_val, 3, 0));

    VLOG(ERR,"----------- MODE 6 : ----------\n");
    WriteRegVCE(core_idx,vcore_idx, set_mode, 6);
    for ( i = 3; i < 10 ; i++ )
    {
        reg_val = ReadRegVCE(core_idx, vcore_idx, debug[i]);
        VLOG(ERR,"\t- mode 6, %08x = %08x\n", debug[i], reg_val);
    }

    VLOG(ERR,"----------- MODE 7 : ----------\n");
    WriteRegVCE(core_idx,vcore_idx, set_mode, 7);
    for ( i = 3; i < 10 ; i++ )
    {
        reg_val = ReadRegVCE(core_idx, vcore_idx, debug[i]);
        VLOG(ERR,"\t- mode 7, %08x = %08x\n", debug[i], reg_val);
    }
}

#define VCE_BUSY                   0xA04
#define VCE_LF_PARAM               0xA6c
#define VCE_BIN_WDMA_CUR_ADDR      0xB1C
#define VCE_BIN_PIC_PARAM          0xB20
#define VCE_BIN_WDMA_BASE          0xB24
#define VCE_BIN_WDMA_END           0xB28
static void	DisplayVceEncReadVCE(int coreIdx, int vcore_idx)
{
    int reg_val;

    VLOG(ERR, "---------------DisplayVceEncReadVCE-----------------\n");
    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_BUSY);
    VLOG(ERR,"\t- VCE_BUSY                 :  0x%x\n", reg_val);
    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_LF_PARAM);
    VLOG(ERR,"\t- VCE_LF_PARAM             :  0x%x\n", reg_val);
    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_BIN_WDMA_CUR_ADDR);
    VLOG(ERR,"\t- VCE_BIN_WDMA_CUR_ADDR    :  0x%x\n", reg_val);
    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_BIN_PIC_PARAM);
    VLOG(ERR,"\t- VCE_BIN_PIC_PARAM        :  0x%x\n", reg_val);
    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_BIN_WDMA_BASE);
    VLOG(ERR,"\t- VCE_BIN_WDMA_BASE        :  0x%x\n", reg_val);
    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_BIN_WDMA_END);
    VLOG(ERR,"\t- VCE_BIN_WDMA_END         :  0x%x\n", reg_val);
}


void PrintWave5xxDecSppStatus(
    Uint32 coreIdx
    )
{
    Uint32  regVal;
    //DECODER SDMA INFO
    VLOG(INFO,"[+] SDMA REG Dump\n");
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5000);
    VLOG(INFO,"C_SDMA_LOAD_CMD      = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5004);
    VLOG(INFO,"C_SDMA_AUTO_MODE     = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5008);
    VLOG(INFO,"C_SDMA_START_ADDR    = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x500C);
    VLOG(INFO,"C_SDMA_END_ADDR      = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5010);
    VLOG(INFO,"C_SDMA_ENDIAN        = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5014);
    VLOG(INFO,"C_SDMA_IRQ_CLEAR     = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5018);
    VLOG(INFO,"C_SDMA_BUSY          = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x501C);
    VLOG(INFO,"C_SDMA_LAST_ADDR     = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5020);
    VLOG(INFO,"C_SDMA_SC_BASE_ADDR  = 0x08%x\n",regVal);
    VLOG(INFO,"[-] SMDA REG Dump\n");

    VLOG(INFO,"[+] SHU REG Dump\n");
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5400);
    VLOG(INFO,"C_SHU_INIT           = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5404);
    VLOG(INFO,"C_SHU_SEEK_NXT_NAL   = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x540C);
    VLOG(INFO,"C_SHU_RD_NAL_ADDR    = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x540C);
    VLOG(INFO,"C_SHU_STATUS         = 0x%08x\n",regVal);

    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5478);
    VLOG(INFO,"C_SHU_REMAIN_BYTE    = 0x%08x\n",regVal);
    VLOG(INFO,"[-] SHU REG Dump\n");

    VLOG(INFO,"[+] GBU REG Dump\n");
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5800);
    VLOG(INFO,"C_GBU_INIT           = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5804);
    VLOG(INFO,"GBU_STATUS           = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5808);
    VLOG(INFO,"GBU_TCNT             = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x580C);
    VLOG(INFO,"GBU_NCNT             = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5478);
    VLOG(INFO,"GBU_REMAIN_BIT       = 0x%08x\n",regVal);
    VLOG(INFO,"[-] GBU REG Dump\n");
}


void PrintWave5xxDecPrescanStatus(
    Uint32 coreIdx
    )
{
    Uint32  regVal;
    //DECODER SDMA INFO
    VLOG(INFO,"[+] PRESCAN REG Dump\n");

    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x61a0);
    VLOG(INFO,"V_PRESCAN_CQ_BS_START_ADDR   = 0x%08x\n",regVal);

    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x61a4);
    VLOG(INFO,"V_PRESCAN_CQ_BS_END_ADDR     = 0x%08x\n",regVal);

    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x61ac);
    VLOG(INFO,"V_PRESCAN_CQ_DEC_CODEC_STD   = 0x%08x\n",regVal);

    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x6200);
    VLOG(INFO,"V_PRESCAN_AVC_SEQ_PARAM      = 0x%08x\n",regVal);

    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x6204);
    VLOG(INFO,"V_PRESCAN_AVC_PIC_PARAM     = 0x%08x\n",regVal);

    VLOG(INFO,"[-] PRESCAN REG Dump\n");
}



void PrintDecVpuStatus(DecHandle handle)
{
    Int32       coreIdx;
    Uint32 product_code = 0;

    if (NULL == handle) {
        VLOG(ERR, "<%s:%d> Handle is NULL\n", __FUNCTION__, __LINE__);
        return;
    }
    coreIdx = handle->coreIdx;

    if (TRUE == PRODUCT_ID_NOT_W_SERIES(handle->productId)) {
        SetClockGate(coreIdx, TRUE);
    }

    vdi_print_vpu_status(coreIdx);

    if (PRODUCT_ID_W_SERIES(handle->productId)) {
        product_code = VpuReadReg(coreIdx, VPU_PRODUCT_CODE_REGISTER);
        if (WAVE517_CODE == product_code || WAVE537_CODE == product_code) {
            vdi_print_vpu_status_dec(coreIdx);
        }
    }

    if (TRUE == PRODUCT_ID_NOT_W_SERIES(handle->productId)) {
        SetClockGate(coreIdx, FALSE);
    }
}

void PrintEncVpuStatus(EncHandle   handle)
{
    Int32       coreIdx;
    coreIdx   = handle->coreIdx;

    SetClockGate(coreIdx, 1);
    vdi_print_vpu_status(coreIdx);
    vdi_print_vpu_status_enc(coreIdx);
    SetClockGate(coreIdx, 0);
}


void HandleEncoderError(
    EncHandle       handle,
    Uint32          encPicCnt __attribute__((unused)),
    EncOutputInfo*  outputInfo __attribute__((unused))
    )
{
/*lint -save -e527 */
    /* Dump VCPU status registers */
    PrintEncVpuStatus(handle);
    /*lint -restore */
}

void DumpMemory(const char* path, Uint32 coreIdx, PhysicalAddress addr, Uint32 size, EndianMode endian)
{
    FILE*   ofp;
    Uint8*  buffer;

    VLOG(INFO,"DUMP MEMORY ADDR(0x%08x) SIZE(%d) FILE(%s)\n", addr, size, path);
    if (NULL == (ofp=(FILE*)osal_fopen(path, "wb"))) {
        VLOG(ERR,"[FAIL]\n");
        return;
    }

    if (NULL == (buffer=(Uint8*)osal_malloc(size))) {
        VLOG(ERR, "<%s:%d> Failed to allocate memory(%d)\n", __FUNCTION__, __LINE__, size);
        return;
    }

    VpuReadMem(coreIdx, addr, buffer, size, endian);
    osal_fwrite(buffer, 1, size, ofp);
    osal_free(buffer);
    osal_fclose(ofp);
}

void DumpCodeBuffer(const char* path)
{
    Uint8*          buffer;
    vpu_buffer_t    vb;
    PhysicalAddress addr;
    osal_file_t     ofp;

    buffer = (Uint8*)osal_malloc(WAVE5_MAX_CODE_BUF_SIZE);
    if ((ofp=osal_fopen(path, "wb")) == NULL) {
        VLOG(ERR,"DUMP CODE AREA into %s FAILED", path);
    }
    else {
        vdi_get_common_memory(0, &vb);

        addr   = vb.phys_addr;
        VpuReadMem(0, addr, buffer, WAVE5_MAX_CODE_BUF_SIZE, VDI_128BIT_LITTLE_ENDIAN);
        osal_fwrite(buffer, 1, WAVE5_MAX_CODE_BUF_SIZE, ofp);
        osal_fclose(ofp);
        VLOG(ERR,"DUMP CODE AREA into %s OK", path);
    }
    osal_free(buffer);
}

void DumpBitstreamBuffer(Uint32 coreIdx, PhysicalAddress addr, Uint32 size, EndianMode endian, const char* prefix)
{
    char    path[1024];
    FILE*   ofp;

    sprintf(path, "./%s_dump_bitstream_buffer.bin", prefix);
    if ((ofp=(FILE*)osal_fopen(path, "wb")) == NULL) {
        VLOG(ERR,"DUMP BITSTREAMBUFFER into %s [FAIL]", path);
    }
    else {
        if (size > 0) {
            Uint8* buffer = (Uint8*)osal_malloc(size);
            if (NULL == buffer) {
                VLOG(ERR, "<%s:%d> Failed to allocate memory(%d)\n", __FUNCTION__, __LINE__, size);
                return;
            }
            VpuReadMem(coreIdx, addr, buffer, size, endian);
            osal_fwrite(buffer, 1, size, ofp);
            osal_free(buffer);
        }
        else {
            VLOG(ERR,">> NO BITSTREAM BUFFER\n");
        }
        osal_fclose(ofp);
        VLOG(INFO,"DUMP BITSTREAMBUFFER into %s [OK]", path);
    }
}

void DumpColMvBuffers(Uint32 coreIdx, const DecInfo* pDecInfo)
{
    char    path[MAX_FILE_PATH];
    FILE*   ofp;
    Uint32  idx = 0;
    Uint32  mvCnt = pDecInfo->numFbsForDecoding;
    Uint32  mvSize = pDecInfo->vbMV[0].size;
    Uint8*  buffer;

    buffer = (Uint8*)osal_malloc(mvSize);
    osal_memset(buffer, 0x00, mvSize);

    if (NULL == buffer) {
        VLOG(ERR,"%s:%d MEM Alloc Failure", __FUNCTION__, __LINE__);
        return;
    }

    for (idx = 0; idx < mvCnt; idx++) {
        if (0 < mvSize) {
            sprintf(path, "Dump_CovMV_buffer_%d.bin", idx);

            if ((ofp=(FILE*)osal_fopen(path, "wb")) == NULL) {
                VLOG(ERR,"DUMP MVCOL BUFFER into %s [FAIL]", path);
                return ;
            }
            VpuReadMem(coreIdx, pDecInfo->vbMV[idx].phys_addr, buffer, mvSize, VDI_128BIT_LITTLE_ENDIAN);
            osal_fwrite(buffer, 1, mvSize, ofp);
            osal_fclose(ofp);
        }
    }
    osal_free(buffer);
}



void HandleDecoderError(
    DecHandle       handle,
    Uint32          frameIdx __attribute__((unused)),
    DecOutputInfo*  outputInfo
    )
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(outputInfo);
}

void PrintMemoryAccessViolationReason(
    Uint32          coreIdx,
    void            *outp
    )
{
    UNREFERENCED_PARAMETER(coreIdx);
    UNREFERENCED_PARAMETER(outp);
}

/**
* \brief           Handle error cases depending on product
* \return  -1      SEQUENCE ERROR
*/
Int32 HandleDecInitSequenceError(DecHandle handle, Uint32 productId, DecOpenParam* openParam, DecInitialInfo* seqInfo, RetCode apiErrorCode)
{
    if (apiErrorCode == RETCODE_MEMORY_ACCESS_VIOLATION) {
        PrintMemoryAccessViolationReason(handle->coreIdx, NULL);
        return -1;
    }

    if (PRODUCT_ID_W_SERIES(productId)) {
        if (seqInfo->seqInitErrReason == HEVC_ETCERR_INIT_SEQ_SPS_NOT_FOUND) {
            return -2;
        } else {
            if (seqInfo->seqInitErrReason == HEVC_SPECERR_OVER_PICTURE_WIDTH_SIZE) {
                VLOG(ERR, "Not supported picture width: MAX_SIZE(8192): %d\n", seqInfo->picWidth);
            }
            if (seqInfo->seqInitErrReason == HEVC_SPECERR_OVER_PICTURE_HEIGHT_SIZE) {
                VLOG(ERR, "Not supported picture height: MAX_SIZE(8192): %d\n", seqInfo->picHeight);
            }
            if (seqInfo->seqInitErrReason == HEVC_SPECERR_OVER_CHROMA_FORMAT) {
                VLOG(ERR, "Not supported chroma idc: %d\n", seqInfo->chromaFormatIDC);
            }
            if (seqInfo->seqInitErrReason == HEVC_SPECERR_OVER_BIT_DEPTH) {
                VLOG(ERR, "Not supported Luma or Chroma bitdepth: L(%d), C(%d)\n", seqInfo->lumaBitdepth, seqInfo->chromaBitdepth);
            }
            if (seqInfo->warnInfo == HEVC_SPECWARN_OVER_PROFILE) {
                VLOG(INFO, "SPEC over profile: %d\n", seqInfo->profile);
            }
            if (seqInfo->warnInfo == HEVC_ETCWARN_INIT_SEQ_VCL_NOT_FOUND) {
                VLOG(INFO, "VCL Not found : RD_PTR(0x%08x), WR_PTR(0x%08x)\n", seqInfo->rdPtr, seqInfo->wrPtr);
            }
            return -1;
        }
    }
    else {
        if (openParam->bitstreamMode == BS_MODE_PIC_END && (seqInfo->seqInitErrReason&(1UL<<31))) {
            VLOG(ERR, "SEQUENCE HEADER NOT FOUND\n");
            return -1;
        }
        else {
            return -1;
        }
    }
}

void print_busy_timeout_status(Uint32 coreIdx, Uint32 product_code, Uint32 pc)
{
    if (PRODUCT_CODE_W_SERIES(product_code)) {
        vdi_print_vpu_status(coreIdx);
        vdi_print_vpu_status_enc(coreIdx);
    } else {
        Uint32 idx;
        for (idx=0; idx<20; idx++) {
            VLOG(ERR, "[VDI] vdi_wait_vpu_busy timeout, PC=0x%lx\n", vdi_read_register(coreIdx, pc));
            {
                Uint32 vcpu_reg[31]= {0,};
                int i;
                // --------- VCPU register Dump
                VLOG(INFO, "[+] VCPU REG Dump\n");
                for (i = 0; i < 25; i++)
                {
                    vdi_write_register(coreIdx, 0x14, (1 << 9) | (i & 0xff));
                    vcpu_reg[i] = vdi_read_register(coreIdx, 0x1c);

                    if (i < 16)
                    {
                        VLOG(INFO, "0x%08x\t", vcpu_reg[i]);
                        if ((i % 4) == 3)
                            VLOG(INFO, "\n");
                    }
                    else
                    {
                        switch (i)
                        {
                        case 16:
                            VLOG(INFO, "CR0: 0x%08x\t", vcpu_reg[i]);
                            break;
                        case 17:
                            VLOG(INFO, "CR1: 0x%08x\n", vcpu_reg[i]);
                            break;
                        case 18:
                            VLOG(INFO, "ML:  0x%08x\t", vcpu_reg[i]);
                            break;
                        case 19:
                            VLOG(INFO, "MH:  0x%08x\n", vcpu_reg[i]);
                            break;
                        case 21:
                            VLOG(INFO, "LR:  0x%08x\n", vcpu_reg[i]);
                            break;
                        case 22:
                            VLOG(INFO, "PC:  0x%08x\n", vcpu_reg[i]);
                            break;
                        case 23:
                            VLOG(INFO, "SR:  0x%08x\n", vcpu_reg[i]);
                            break;
                        case 24:
                            VLOG(INFO, "SSP: 0x%08x\n", vcpu_reg[i]);
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }
    }
}

void wave5xx_vcore_status(
    Uint32 coreIdx
    )
{
    Uint32 i;
    Uint32 temp;

    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    VLOG(INFO,"------                            VCORE BPU STATUS                        -----\n");
    VLOG(INFO,"-------------------------------------------------------------------------------\n");

    VLOG(INFO,"[+] BPU REG Dump\n");
    for (i=0;i < 20; i++) {
        temp = vdi_fio_read_register(coreIdx, (W5_REG_BASE + 0x8000 + 0x18));
        VLOG(INFO,"BITPC = 0x%08x\n", temp);
    }

/*
    VLOG(INFO, "r0 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x0) );
    VLOG(INFO, "r1 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x1) );
    VLOG(INFO, "r2 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x2) );
    VLOG(INFO, "r3 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x3) );

    VLOG(INFO, "r4 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x4) );
    VLOG(INFO, "r5 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x5) );
    VLOG(INFO, "r6 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x6) );
    VLOG(INFO, "r7 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x7) );

    VLOG(INFO, "stack0 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x10) );
    VLOG(INFO, "stack1 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x11) );
    VLOG(INFO, "stack2 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x12) );
    VLOG(INFO, "stack3 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x13) );

    VLOG(INFO, "stack4 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x14) );
    VLOG(INFO, "stack5 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x15) );
    VLOG(INFO, "stack6 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x16) );
    VLOG(INFO, "stack7 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x17) );
*/

    VLOG(INFO,"[+] BPU Debug message REG Dump\n");
    VLOG(INFO,"[MSG_0:0x%08x], [MSG_1:0x%08x],[MSG_2:0x%08x],[MSG_3:0x%08x],[MSG_4:0x%08x],[MSG_5:0x%08x] \n",
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1A8),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1AC),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1B0),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1B4),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1B8),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1BC));

    VLOG(INFO,"[-] BPU Debug message REG Dump\n");
    VLOG(INFO,"[+] BPU interface REG Dump\n");
    for(i = 0x8000; i < 0x80FC; i += 16) {
        VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i)),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12)));
    }
    VLOG(INFO,"[-] BPU interfrace REG Dump\n");



    VLOG(INFO,"[+] MIB REG Dump\n");
    temp  = vdi_irb_read_register(coreIdx, 0, 0x110);
    VLOG(INFO,"MIB_EXTADDR        : 0x%08x , External base address \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x111);
    VLOG(INFO,"MIB_INT_ADDR       : 0x%08x , Internal base address (MIBMEM) \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x112);
    VLOG(INFO,"MIB_DATA_CNT       : 0x%08x , Length (8-byte unit) \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x113);
    VLOG(INFO,"MIB_COMMAND        : 0x%08x , COMMAND[load, save] \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x114);
    VLOG(INFO,"MIB_BUSY           : 0x%08x , Busy status \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x116);
    VLOG(INFO,"MIB_WREQ           : 0x%08x , Write response done \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x117);
    VLOG(INFO,"MIB_BUSID          : 0x%08x , GDI bus ID for core \n", temp);
    VLOG(INFO,"[-] MIB REG Dump\n");

    VLOG(INFO,"[+] RDMA REG Dump\n");
    temp  = vdi_irb_read_register(coreIdx, 0, 0x120);
    VLOG(INFO,"RDMA_WR_SEL          : 0x%08x , [0] : selection flag for writing register, 0 - for GBIN0, 1- for GBIN1 \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x121);
    VLOG(INFO,"RDMA_RD_SEL          : 0x%08x , [0] : selection flag for reading register,  \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x122);
    VLOG(INFO,"RDMA_INIT            : 0x%08x , (WO) 1 - init RDMA, (RO) 1 - init_busy during RDMA initialize  \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x123);
    VLOG(INFO,"RDMA_LOAD_CMD        : 0x%08x , [0] auto_mode,[1] manual_mode  \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x125);
    VLOG(INFO,"RDMA_BASE_ADDR       : 0x%08x , Base address after init, should be 16byte align \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x126);
    VLOG(INFO,"RDMA_END_ADDR        : 0x%08x , RDMA end address, if current >= rdma end addr, empty intterupt is occrured \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x127);
    VLOG(INFO,"RDMA_ENDIAN          : 0x%08x , ENDIAN setting for RDMA \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x128);
    VLOG(INFO,"RDMA_CUR_ADDR        : 0x%08x , RDMA current addr, after loading, current addr is increased with load Bytes \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x129);;
    VLOG(INFO,"RDMA_STATUS          : 0x%08x , [0] if 1, RMDA busy [30:28] load command count [31] if 1, bin_rmda_empty \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x12A);;
    VLOG(INFO,"RDMA_DBG_INFO        : 0x%08x , RDMA debug info \n", temp);
    VLOG(INFO,"[+] RDMA REG Dump\n");
    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    VLOG(INFO,"------                            VCORE STATUS                              -----\n");
    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    // --------- VCE register Dump
    VLOG(INFO,"[+] VCE REG Dump Core0\n");
    for (i=0x000; i<0x1fc; i+=16) {
        VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", i,
            ReadRegVCE(coreIdx, 0, (i+0x00)),
            ReadRegVCE(coreIdx, 0, (i+0x04)),
            ReadRegVCE(coreIdx, 0, (i+0x08)),
            ReadRegVCE(coreIdx, 0, (i+0x0c)));
    }
    VLOG(INFO,"[-] VCE REG Dump\n");
}


void wave5xx_PP_status(
    Uint32 coreIdx
    )
{
    Uint32 temp;

    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    VLOG(INFO,"------                            PP STATUS                               -----\n");
    VLOG(INFO,"-------------------------------------------------------------------------------\n");

    VLOG(INFO,"[+] PP COMMAND REG Dump\n");
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x0);
    VLOG(INFO,"PP_CMD               : 0x%08x , [0] pic_init_c, [1] soft_reset_c \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x4);
    VLOG(INFO,"PP_CTB_SIZE          : 0x%08x , 0:CTB16, 1:CTB32, 2:CTB64, 3:CTB128 \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x48);
    VLOG(INFO,"PP_LF_CRTL           : 0x%08x , [0] bwb_chroma_out_disable \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x4C);
    VLOG(INFO,"PP_BWB_OUT_FORMAT    : 0x%08x , \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x50);
    VLOG(INFO,"PP_BWB_STRIDE        : 0x%08x , [15: 0] bwb_chr_stride [31:16] bwb_lum_stride \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x54);
    VLOG(INFO,"PP_BWB_SIZE          : 0x%08x , [15: 0] bwb_pic_height [31:16] bwb_pic_width \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x80);
    VLOG(INFO,"PP_STATUS            : 0x%08x \n ", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x84);
    VLOG(INFO,"PP_BWB_DEBUG0        : 0x%08x \n ", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x88);
    VLOG(INFO,"PP_BWB_DEBUG1        : 0x%08x \n ", temp);
    VLOG(INFO,"[-] PP COMMAND REG Dump\n");

    VLOG(INFO,"[+] PP SCALER REG Dump\n");
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x14);
    VLOG(INFO,"PP_SCL_PARAM         : 0x%08x , 0:scaler enable \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x18);
    VLOG(INFO,"PP_SCL_IN_DISP_SIZE  : 0x%08x , [15: 0] scl_in_disp_pic_height, [31:16] scl_in_disp_pic_width \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x1C);
    VLOG(INFO,"PP_SCL_IN_SIZE       : 0x%08x , [15: 0] scl_in_pic_height, [31:16] scl_in_disp_pic_width \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x20);
    VLOG(INFO,"PP_SCL_OUT_SIZE      : 0x%08x , [15: 0] scl_out_pic_height, [31:16] scl_out_pic_width \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x20);
    VLOG(INFO,"PP_SCL_OUT_SIZE      : 0x%08x , [15: 0] scl_out_pic_height, [31:16] scl_out_pic_width \n", temp);
    VLOG(INFO,"[-] PP SCALER REG Dump\n");

    VLOG(INFO,"[+] PP IFBC REG Dump\n");
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x58);
    VLOG(INFO,"IFBC_NO_MORE_REQ     : 0x%08x , [0] ifbc_no_more_req [31] soft_reset_ready  \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x5C);
    VLOG(INFO,"IFBC_PICSIZE         : 0x%08x , [15: 0] ifbc_pic_height [31:16]ifbc_pic_width \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x60);
    VLOG(INFO,"IFBC_BYTE_FORMAT     : 0x%08x , [1:0] ifbc_byte_format \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x64);
    VLOG(INFO,"IFBC_RUN_MODE        : 0x%08x , [0] ifbc_enable [1] pp_out_disable \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x68);
    VLOG(INFO,"IFBC_Y_BASE          : 0x%08x , [25:0] base_ifbc_y \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x6C);
    VLOG(INFO,"IFBC_UV_BASE         : 0x%08x , [25:0] base_ifbc_uv \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x70);
    VLOG(INFO,"IFBC_TILE_INFO       : 0x%08x \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x74);
    VLOG(INFO,"IFBC_PIC_WIDTH_MOD96 : 0x%08x \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x78);
    VLOG(INFO,"IFBC_TILE_STRIDE     : 0x%08x \n", temp);
    VLOG(INFO,"[-] PP IFBC REG Dump\n");
}


void wave5xx_mismatch_vcore_status(
    Uint32 coreIdx
    )
{
    Uint32 i;
    Uint32 temp;

    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    VLOG(INFO,"------                            VCORE XXX STATUS                        -----\n");
    VLOG(INFO,"-------------------------------------------------------------------------------\n");

    VLOG(INFO,"[+] BPU REG Dump\n");
    for (i=0;i < 20; i++) {
        temp = vdi_fio_read_register(coreIdx, (W5_REG_BASE + 0x8000 + 0x18));
        VLOG(INFO,"BITPC = 0x%08x\n", temp);
    }
    VLOG(INFO,"[-] VCE REG Dump\n");
}

void wave5xx_bpu_status(
    Uint32 coreIdx
    )
{
    Uint32 i;
    Uint32 temp;

    VLOG(INFO,"[+] BPU REG Dump\n");
    for (i=0;i < 20; i++) {
        temp = vdi_fio_read_register(coreIdx, (W5_REG_BASE + 0x8000 + 0x18));
        VLOG(ERR,"BITPC = 0x%08x\n", temp);
    }
    for (i=0; i < 44; i+=4) {
        //0x40 ~ 0x68, DEBUG_BPU_INFO_0 ~ DEBUG_BPU_INFO_A
        temp = vdi_fio_read_register(coreIdx, (W5_REG_BASE + 0x8000 + 0x40 + i));
        VLOG(ERR,"DEBUG_BPU_INFO_%x = 0x%08x\n", (i/4), temp);
    }

    temp = vdi_fio_read_register(coreIdx, (W5_REG_BASE + 0x8000 + 0x30));
    VLOG(ERR,"BIT_BUSY Core0=0x%08x \n", temp);

    for (i=0; i < 8; i += 4 ) {
        temp = vdi_fio_read_register(coreIdx, (W5_REG_BASE + 0x8000 + 0x80 + i));
        VLOG(ERR,"stack[%d] Core0=0x%08x\n", temp);
    }



    for(i = 0x8000; i < 0x81FC; i += 16) {
        VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i)),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12)));
    }

    VLOG(INFO,"[-] BPU REG Dump\n");

    VLOG(INFO,"[+] MIB REG Dump\n");
    for (i=0x110 ; i < 0x118 ; i++) {
        temp  = vdi_irb_read_register(coreIdx, 0, i);
        VLOG(ERR,"MIB 0x%08x Core0=0x%08x\n", i, temp);
    }
    VLOG(INFO,"[-] MIB REG Dump\n");


    VLOG(INFO,"[-] BPU MSG REG Dump\n");

    VLOG(INFO,"[MSG_0:0x%08x], [MSG_1:0x%08x],[MSG_2:0x%08x],[MSG_3:0x%08x],[MSG_4:0x%08x],[MSG_5:0x%08x] \n",
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1A8),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1AC),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1B0),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1B4),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1B8),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1BC));

    VLOG(INFO,"[-] BPU MSG REG Dump\n");

}


void vdi_print_vpu_status_enc(unsigned long coreIdx)
{
    int       vce_enc_debug[12] = {0, };
    int       set_mode;
    int       vcore_num, vcore_idx;
    int i;

    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    VLOG(INFO,"------                           Encoder only                                                         -----\n");
    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    VLOG(INFO,"BS_OPT: 0x%08x\n", VpuReadReg(coreIdx, W5_BS_OPTION));

    VLOG(INFO,"[+] VCPU DMA Dump\n");
    for(i = 0x2000; i < 0x2018; i += 16) {
        VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i)),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12)));
    }
    VLOG(INFO,"[-] VCPU DMA Dump\n");

    VLOG(INFO,"[+] VCPU HOST REG Dump\n");
    for(i = 0x3000; i < 0x30fc; i += 16) {
        VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i)),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12)));
    }
    VLOG(INFO,"[-] VCPU HOST REG Dump\n");

    VLOG(INFO,"[+] VCPU ENT ENC REG Dump\n");
    for(i = 0x6800; i < 0x7000; i += 16) {
        VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i)),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12)));
    }
    VLOG(INFO,"[-] VCPU ENT ENC REG Dump\n");

    VLOG(INFO,"[+] VCPU HOST MEM Dump\n");
    for(i = 0x7000; i < 0x70fc; i += 16) {
        VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i)),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12)));
    }
    VLOG(INFO,"[-] VCPU SPP Dump\n");

    VLOG(INFO,"vce run flag = %d\n", VpuReadReg(coreIdx, 0x1E8));


    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    VLOG(INFO,"------                            VCE DUMP(ENC)                           -----\n");
    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    vce_enc_debug[0] = 0x0ba0;//MODE SEL //parameter VCE_ENC_DEBUG0            = 9'h1A0;
    vce_enc_debug[1] = 0x0ba4;
    vce_enc_debug[2] = 0x0ba8;
    vce_enc_debug[3] = 0x0bac;
    vce_enc_debug[4] = 0x0bb0;
    vce_enc_debug[5] = 0x0bb4;
    vce_enc_debug[6] = 0x0bb8;
    vce_enc_debug[7] = 0x0bbc;
    vce_enc_debug[8] = 0x0bc0;
    vce_enc_debug[9] = 0x0bc4;
    set_mode              = 0x0ba0;
    vcore_num            = 1;


    for (vcore_idx = 0; vcore_idx < vcore_num ; vcore_idx++) {
        VLOG(INFO,"==========================================\n");
        VLOG(INFO,"[+] VCE REG Dump VCORE_IDX : %d\n",vcore_idx);
        VLOG(INFO,"==========================================\n");
        DisplayVceEncReadVCE             ((Int32)coreIdx, vcore_idx);
        DisplayVceEncDebugCommon521      ((Int32)coreIdx, vcore_idx, set_mode, vce_enc_debug[0], vce_enc_debug[1], vce_enc_debug[2]);
        DisplayVceEncDebugMode           ((Int32)coreIdx, vcore_idx, set_mode, vce_enc_debug);
    }
}

void vdi_print_vpu_status_dec(unsigned long coreIdx)
{
        Uint32 i;

        VLOG(INFO,"-------------------------------------------------------------------------------\n");
        VLOG(INFO,"------                           Decoder only                             -----\n");
        VLOG(INFO,"-------------------------------------------------------------------------------\n");

        /// -- VCPU ENTROPY PERI DECODE Common
        VLOG(INFO,"[+] VCPU ENT DEC REG Dump\n");
        for(i = 0x6000; i < 0x6800; i += 16) {
            VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
                vdi_fio_read_register(coreIdx, (W5_REG_BASE + i)),
                vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4 )),
                vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8 )),
                vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12)));
        }
        VLOG(INFO,"[-] VCPU ENT DEC REG Dump\n");
}


void wave5xx_vcpu_status (unsigned long coreIdx)
{
#ifdef MCTS_RC_TEST
    return;
#endif //MCTS_RC_TEST

    Uint32 vcpu_reg[31]= {0,};
    Uint32 i;

    Uint32 que_status;
    Uint32 stage_1;
    Uint32 stage_2;
    Uint32 stage_3;
    Uint32 instance_done;

    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    VLOG(INFO,"------                            Scheduler STATUS                        -----\n");
    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    {
        que_status = vdi_read_register(coreIdx,  0x1e0);
        VLOG(INFO, "COMMAND_QUE_CNT %d  : REPORT_QUE_CNT : %d \n", que_status >> 16, que_status & 0xffff);
        stage_1 = vdi_read_register(coreIdx, 0x1f0);
        VLOG(INFO, "PARSING_INSTANCE     : 0x%08x \n", stage_1);
        stage_2 = vdi_read_register(coreIdx, 0x1f4);
        VLOG(INFO, "DECODING_INSTANCE     : 0x%08x \n", stage_2);
        stage_3 = vdi_read_register(coreIdx, 0x1f8);
        VLOG(INFO, "ENCODING_INSTANCE     : 0x%08x \n", stage_3);
        instance_done = vdi_read_register(coreIdx, 0x1fC);
        VLOG(INFO, "QUEUED_COMMAND_DONE : 0x%08x \n", instance_done);
    }

    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    VLOG(INFO,"------                            VCPU CORE STATUS                        -----\n");
    VLOG(INFO,"-------------------------------------------------------------------------------\n");
   // --------- VCPU register Dump
    VLOG(INFO,"[+] VCPU REG Dump\n");
    for (i = 0; i < 25; i++) {
        VpuWriteReg (coreIdx, 0x14, (1<<9) | (i & 0xff));
        vcpu_reg[i] = VpuReadReg (coreIdx, 0x1c);

        if (i < 16) {
            VLOG(INFO,"0x%08x\t",  vcpu_reg[i]);
            if ((i % 4) == 3) VLOG(INFO,"\n");
        }
        else {
            switch (i) {
            case 16: VLOG(INFO,"CR0: 0x%08x\t", vcpu_reg[i]); break;
            case 17: VLOG(INFO,"CR1: 0x%08x\n", vcpu_reg[i]); break;
            case 18: VLOG(INFO,"ML:  0x%08x\t", vcpu_reg[i]); break;
            case 19: VLOG(INFO,"MH:  0x%08x\n", vcpu_reg[i]); break;
            case 21: VLOG(INFO,"LR:  0x%08x\n", vcpu_reg[i]); break;
            case 22: VLOG(INFO,"PC:  0x%08x\n", vcpu_reg[i]); break;
            case 23: VLOG(INFO,"SR:  0x%08x\n", vcpu_reg[i]); break;
            case 24: VLOG(INFO,"SSP: 0x%08x\n", vcpu_reg[i]); break;
            default: break;
            }
        }
    }

    for ( i = 0 ; i < 20 ; i++) {
        VLOG(INFO, "LR=0x%x, PC=0x%x\n", vdi_read_register(coreIdx, W5_VCPU_CUR_LR), vdi_read_register(coreIdx, W5_VCPU_CUR_PC));
    }
    VLOG(INFO, "VCPU_BUSY_STATUS    : 0x%08x , VPU command re-enterance check \n", vdi_read_register(coreIdx, 0x70) );
    VLOG(INFO, "VCPU_HALT_STATUS    : 0x%08x , [3:0] VPU bus transaction [4] halt status \n", vdi_read_register(coreIdx, 0x74) );
    VLOG(INFO,"[-] VCPU CORE REG Dump\n");
    VLOG(INFO, "TEMP VPU_SUB_FRM_SYNC_IF          : 0x%08x , VPU command re-enterance check \n", vdi_read_register(coreIdx, 0x300) );
    VLOG(INFO, "TEMP VPU_SUB_FRM_SYNC             : 0x%08x , VPU command re-enterance check \n", vdi_read_register(coreIdx, 0x304) );
    VLOG(INFO, "TEMP VPU_SUB_FRM_SYNC_IDX         : 0x%08x , VPU command re-enterance check \n", vdi_read_register(coreIdx, 0x308) );
    VLOG(INFO, "TEMP VPU_SUB_FRM_SYNC_CLR         : 0x%08x , VPU command re-enterance check \n", vdi_read_register(coreIdx, 0x30C) );
    VLOG(INFO, "TEMP VPU_SUB_FRM_SYNC_CNT         : 0x%08x , VPU command re-enterance check \n", vdi_read_register(coreIdx, 0x310) );
    VLOG(INFO, "TEMP VPU_SUB_FRM_SYNC_ENC_FIDX    : 0x%08x , VPU command re-enterance check \n", vdi_read_register(coreIdx, 0x314) );

    VLOG(INFO,"[-] VCPU REG Dump\n");
    /// -- VCPU PERI DECODE Common
    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    VLOG(INFO,"------                            VCPU PERI(SPP)                          -----\n");
    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    PrintWave5xxDecSppStatus((Uint32)coreIdx);
    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    VLOG(INFO,"------                            VCPU PERI(PRESCAN)                      -----\n");
    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    PrintWave5xxDecPrescanStatus((Uint32)coreIdx);
}

void vdi_print_vpu_status(unsigned long coreIdx)
{
    Uint32 productCode;

    productCode = vdi_read_register(coreIdx, VPU_PRODUCT_CODE_REGISTER);

    if (PRODUCT_CODE_W_SERIES(productCode)) {
        wave5xx_vcpu_status(coreIdx);
        wave5xx_vcore_status((Uint32)coreIdx);
        VLOG(INFO,"-------------------------------------------------------------------------------\n");
    }
    else if (PRODUCT_CODE_NOT_W_SERIES(productCode)) {
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", productCode);
    }
}

void ChekcAndPrintDebugInfo(VpuHandle handle, BOOL bEncoder, RetCode result)
{
    RetCode cmd_ret;
    VPUDebugInfo debugInfo;
    VLOG(INFO, "+%s core=%d, inst=%d, bEncoder=%d, result=%d\n", __FUNCTION__, handle->coreIdx, handle->instIndex, bEncoder, result);

    vdi_log(handle->coreIdx, handle->instIndex, 0, 1);
    if (bEncoder == TRUE)
        cmd_ret = VPU_EncGiveCommand(handle, GET_DEBUG_INFORM, &debugInfo);
    else
        cmd_ret = VPU_DecGiveCommand(handle, GET_DEBUG_INFORM, &debugInfo);
    if (cmd_ret != RETCODE_SUCCESS) {
        VLOG(ERR, "-%s GET_DEBUG_INFOM command fail \n", __FUNCTION__);
        return;
    }

    VLOG(TRACE, "++ interrupt flags \n");
    VLOG(TRACE, "     pend_intrpt_idc = 0x%x \n", (debugInfo.regs[0x0c]>>16)&0xffff);
    VLOG(TRACE, "     multi_int_reason = 0x%x \n", debugInfo.regs[0x0c]&0xffff);
    VLOG(TRACE, "     last interrupt reason = 0x%x \n", debugInfo.regs[0x0d]);
    VLOG(TRACE, "-- interrupt flags \n\n");

    VLOG(TRACE, "++ available core flags \n");
    VLOG(TRACE, "     STAGE_SEEK      core_avail_idc = %d \n", ((debugInfo.regs[0x0e] >> 0)&0x0f));
    VLOG(TRACE, "     STAGE_PARSING   core_avail_idc = %d \n", ((debugInfo.regs[0x0e] >> 4)&0x0f));
    VLOG(TRACE, "     STAGE_DEC/ENC   core_avail_idc = %d \n", ((debugInfo.regs[0x0e] >> 8)&0x0f));
    VLOG(TRACE, "     STAGE_PACKAGING core_avail_idc = %d \n", ((debugInfo.regs[0x0e] >> 12)&0x0f));
    VLOG(TRACE, "-- avaiable core flags \n\n");

    VLOG(TRACE, "++ the number of the allocated queue and commands in report queue\n");
    VLOG(TRACE, "     inst0={rq_cnt=%d, que_cnt=%d} inst1={rq_cnt=%d, que_cnt=%d} inst2={rq_cnt=%d, que_cnt=%d} inst3={rq_cnt=%d, que_cnt=%d}\n",
    ((((debugInfo.regs[0x06] >> 0)&0xff)>>4) & 0xf), ((((debugInfo.regs[0x06] >> 0)&0xff)>>0) & 0xf),
    ((((debugInfo.regs[0x06] >> 8)&0xff)>>4) & 0xf), ((((debugInfo.regs[0x06] >> 8)&0xff)>>0) & 0xf),
    ((((debugInfo.regs[0x06] >> 16)&0xff)>>4) & 0xf), ((((debugInfo.regs[0x06] >> 16)&0xff)>>0) & 0xf),
    ((((debugInfo.regs[0x06] >> 24)&0xff)>>4) & 0xf), ((((debugInfo.regs[0x06] >> 24)&0xff)>>0) & 0xf));
    VLOG(TRACE, "     inst4={rq_cnt=%d, que_cnt=%d} inst5={rq_cnt=%d, que_cnt=%d} inst6={rq_cnt=%d, que_cnt=%d} inst7={rq_cnt=%d, que_cnt=%d}\n",
    ((((debugInfo.regs[0x07] >> 0)&0xff)>>4) & 0xf), ((((debugInfo.regs[0x07] >> 0)&0xff)>>0) & 0xf),
    ((((debugInfo.regs[0x07] >> 8)&0xff)>>4) & 0xf), ((((debugInfo.regs[0x07] >> 8)&0xff)>>0) & 0xf),
    ((((debugInfo.regs[0x07] >> 16)&0xff)>>4) & 0xf), ((((debugInfo.regs[0x07] >> 16)&0xff)>>0) & 0xf),
    ((((debugInfo.regs[0x07] >> 24)&0xff)>>4) & 0xf), ((((debugInfo.regs[0x07] >> 24)&0xff)>>0) & 0xf));
    VLOG(TRACE, "     inst7={rq_cnt=%d, que_cnt=%d} inst9={rq_cnt=%d, que_cnt=%d} inst10={rq_cnt=%d, que_cnt=%d} inst11={rq_cnt=%d, que_cnt=%d}\n",
    ((((debugInfo.regs[0x08] >> 0)&0xff)>>4) & 0xf), ((((debugInfo.regs[0x08] >> 0)&0xff)>>0) & 0xf),
    ((((debugInfo.regs[0x08] >> 8)&0xff)>>4) & 0xf), ((((debugInfo.regs[0x08] >> 8)&0xff)>>0) & 0xf),
    ((((debugInfo.regs[0x08] >> 16)&0xff)>>4) & 0xf), ((((debugInfo.regs[0x08] >> 16)&0xff)>>0) & 0xf),
    ((((debugInfo.regs[0x08] >> 24)&0xff)>>4) & 0xf), ((((debugInfo.regs[0x08] >> 24)&0xff)>>0) & 0xf));
    VLOG(TRACE, "     inst12={rq_cnt=%d, que_cnt=%d} inst13={rq_cnt=%d, que_cnt=%d} inst14={rq_cnt=%d, que_cnt=%d} inst15={rq_cnt=%d, que_cnt=%d}\n",
    ((((debugInfo.regs[0x09] >> 0)&0xff)>>4) & 0xf), ((((debugInfo.regs[0x09] >> 0)&0xff)>>0) & 0xf),
    ((((debugInfo.regs[0x09] >> 8)&0xff)>>4) & 0xf), ((((debugInfo.regs[0x09] >> 8)&0xff)>>0) & 0xf),
    ((((debugInfo.regs[0x09] >> 16)&0xff)>>4) & 0xf), ((((debugInfo.regs[0x09] >> 16)&0xff)>>0) & 0xf),
    ((((debugInfo.regs[0x09] >> 24)&0xff)>>4) & 0xf), ((((debugInfo.regs[0x09] >> 24)&0xff)>>0) & 0xf));
    VLOG(TRACE, "-- the number of the allocated queue and commands in report queue\n\n");


    VLOG(TRACE, "++ status of instance handle\n");
    VLOG(TRACE, "     inst0 status=%d, inst1 status=%d, inst2 status=%d, inst3 status=%d \n",
    ((debugInfo.regs[0x0A] >> 0)&0xf), ((debugInfo.regs[0x0A] >> 4)&0xf), ((debugInfo.regs[0x0A] >> 8)&0xf), ((debugInfo.regs[0x0A] >> 12)&0xf));
    VLOG(TRACE, "     inst4 status=%d, inst5 status=%d, inst6 status=%d, inst7 status=%d \n",
    ((debugInfo.regs[0x0A] >> 16)&0xf), ((debugInfo.regs[0x0A] >> 20)&0xf), ((debugInfo.regs[0x0A] >> 24)&0xf), ((debugInfo.regs[0x0A] >> 28)&0xf));

    VLOG(TRACE, "     inst8 status=%d, inst9 status=%d, inst10 status=%d, inst11 status=%d \n",
    ((debugInfo.regs[0x0B] >> 0)&0xf), ((debugInfo.regs[0x0B] >> 4)&0xf), ((debugInfo.regs[0x0B] >> 8)&0xf), ((debugInfo.regs[0x0B] >> 12)&0xf));

    VLOG(TRACE, "     inst12 status=%d, inst13 status=%d, inst14 status=%d, inst15 status=%d \n",
    ((debugInfo.regs[0x0B] >> 16)&0xf), ((debugInfo.regs[0x0B] >> 20)&0xf), ((debugInfo.regs[0x0B] >> 24)&0xf), ((debugInfo.regs[0x0B] >> 28)&0xf));
    VLOG(TRACE, "-- status of instance handle\n\n");

    VLOG(TRACE, "++ active command in each stage\n");
    VLOG(TRACE, "     STAGE_SEEK, CMD_INST_ID = %d \n", (debugInfo.regs[0x10]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_SEEK, CMD_QUE_CNT = %d \n", (debugInfo.regs[0x10]>>0)&0xffff);
    VLOG(TRACE, "     STAGE_SEEK, CMD_IDX = %d \n", (debugInfo.regs[0x11]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_SEEK, CMD_BUF_IDX = %d \n", (debugInfo.regs[0x11]>>0)&0xffff);
    VLOG(TRACE, "     STAGE_SEEK, CMD_QUEUE_STATUS = %d \n", (debugInfo.regs[0x12]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_SEEK, CMD_STATUS = %d \n", (debugInfo.regs[0x12]>>0)&0xffff);
    VLOG(TRACE, "     STAGE_SEEK, CMD_CMD = %d \n", (debugInfo.regs[0x13]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_SEEK, CMD_USE_TASKBUF = %d \n", (debugInfo.regs[0x13]>>0)&0xffff);
    VLOG(TRACE, "     STAGE_SEEK, CMD_SCHED_STATUS = %d \n", (debugInfo.regs[0x14]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_SEEK, CMD_MAX_QUEUE_DEPTH = %d \n", (debugInfo.regs[0x14]>>0)&0xffff);


    VLOG(TRACE, "     STAGE_PARSING, CMD_INST_ID = %d \n", (debugInfo.regs[0x15]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_PARSING, CMD_QUE_CNT = %d \n", (debugInfo.regs[0x15]>>0)&0xffff);
    VLOG(TRACE, "     STAGE_PARSING, CMD_IDX = %d \n", (debugInfo.regs[0x16]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_PARSING, CMD_BUF_IDX = %d \n", (debugInfo.regs[0x16]>>0)&0xffff);
    VLOG(TRACE, "     STAGE_PARSING, CMD_QUEUE_STATUS = %d \n", (debugInfo.regs[0x17]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_PARSING, CMD_STATUS = %d \n", (debugInfo.regs[0x17]>>0)&0xffff);
    VLOG(TRACE, "     STAGE_PARSING, CMD_CMD = %d \n", (debugInfo.regs[0x18]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_PARSING, CMD_USE_TASKBUF = %d \n", (debugInfo.regs[0x18]>>0)&0xffff);
    VLOG(TRACE, "     STAGE_PARSING, CMD_SCHED_STATUS = %d \n", (debugInfo.regs[0x19]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_PARSING, CMD_MAX_QUEUE_DEPTH = %d \n", (debugInfo.regs[0x19]>>0)&0xffff);


    VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_INST_ID = %d \n", (debugInfo.regs[0x1a]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_QUE_CNT = %d \n", (debugInfo.regs[0x1a]>>0)&0xffff);
    VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_IDX = %d \n", (debugInfo.regs[0x1b]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_BUF_IDX = %d \n", (debugInfo.regs[0x1b]>>0)&0xffff);
    VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_QUEUE_STATUS = %d \n", (debugInfo.regs[0x1c]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_STATUS = %d \n", (debugInfo.regs[0x1c]>>0)&0xffff);
    VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_CMD = %d \n", (debugInfo.regs[0x1d]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_USE_TASKBUF = %d \n", (debugInfo.regs[0x1d]>>0)&0xffff);
    VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_SCHED_STATUS = %d \n", (debugInfo.regs[0x1e]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_MAX_QUEUE_DEPTH = %d \n", (debugInfo.regs[0x1e]>>0)&0xffff);

    VLOG(TRACE, "     STAGE_PACKAGING, CMD_INST_ID = %d \n", (debugInfo.regs[0x1f]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_PACKAGING, CMD_QUE_CNT = %d \n", (debugInfo.regs[0x1f]>>0)&0xffff);
    VLOG(TRACE, "     STAGE_PACKAGING, CMD_IDX = %d \n", (debugInfo.regs[0x20]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_PACKAGING, CMD_BUF_IDX = %d \n", (debugInfo.regs[0x20]>>0)&0xffff);
    VLOG(TRACE, "     STAGE_PACKAGING, CMD_QUEUE_STATUS = %d \n", (debugInfo.regs[0x21]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_PACKAGING, CMD_STATUS = %d \n", (debugInfo.regs[0x21]>>0)&0xffff);
    VLOG(TRACE, "     STAGE_PACKAGING, CMD_CMD = %d \n", (debugInfo.regs[0x22]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_PACKAGING, CMD_USE_TASKBUF = %d \n", (debugInfo.regs[0x22]>>0)&0xffff);
    VLOG(TRACE, "     STAGE_PACKAGING, CMD_SCHED_STATUS = %d \n", (debugInfo.regs[0x23]>>16)&0xffff);
    VLOG(TRACE, "     STAGE_PACKAGING, CMD_MAX_QUEUE_DEPTH = %d \n", (debugInfo.regs[0x23]>>0)&0xffff);
    VLOG(TRACE, "-- active command in each stage\n\n");


    VLOG(TRACE, "++ queued commands in each stage\n");
    VLOG(TRACE, "     STAGE_SEEK      inst0->cmd_cnt=%d, inst1->cmt_cmd=%d, inst2->cmd_cnt=%d, inst3=>cmd_cnt=%d inst4=>cmd_cnt=%d, inst5=>cmd_cnt=%d, inst6=>cmd_cnt=%d, inst7=>cmd_cnt=%d \n",
    ((debugInfo.regs[0x30] >> 0) & 0xf), ((debugInfo.regs[0x30] >> 4) & 0xf),
    ((debugInfo.regs[0x30] >> 8) & 0xf), ((debugInfo.regs[0x30] >> 12) & 0xf),
    ((debugInfo.regs[0x30] >> 16) & 0xf), ((debugInfo.regs[0x30] >> 20) & 0xf),
    ((debugInfo.regs[0x30] >> 24) & 0xf), ((debugInfo.regs[0x30] >> 28) & 0xf));
    VLOG(TRACE, "     STAGE_SEEK      inst8->cmd_cnt=%d, inst9->cmt_cmd=%d, inst10->cmd_cnt=%d, inst11=>cmd_cnt=%d inst12=>cmd_cnt=%d, inst13=>cmd_cnt=%d, inst14=>cmd_cnt=%d, inst15=>cmd_cnt=%d \n",
    ((debugInfo.regs[0x31] >> 0) & 0xf), ((debugInfo.regs[0x31] >> 4) & 0xf),
    ((debugInfo.regs[0x31] >> 8) & 0xf), ((debugInfo.regs[0x31] >> 12) & 0xf),
    ((debugInfo.regs[0x31] >> 16) & 0xf), ((debugInfo.regs[0x31] >> 20) & 0xf),
    ((debugInfo.regs[0x31] >> 24) & 0xf), ((debugInfo.regs[0x31] >> 28) & 0xf));
    VLOG(TRACE, "     STAGE_PARSING      inst0->cmd_cnt=%d, inst1->cmt_cmd=%d, inst2->cmd_cnt=%d, inst3=>cmd_cnt=%d inst4=>cmd_cnt=%d, inst5=>cmd_cnt=%d, inst6=>cmd_cnt=%d, inst7=>cmd_cnt=%d \n",
    ((debugInfo.regs[0x32] >> 0) & 0xf), ((debugInfo.regs[0x32] >> 4) & 0xf),
    ((debugInfo.regs[0x32] >> 8) & 0xf), ((debugInfo.regs[0x32] >> 12) & 0xf),
    ((debugInfo.regs[0x32] >> 16) & 0xf), ((debugInfo.regs[0x32] >> 20) & 0xf),
    ((debugInfo.regs[0x32] >> 24) & 0xf), ((debugInfo.regs[0x32] >> 28) & 0xf));
    VLOG(TRACE, "     STAGE_PARSING      inst8->cmd_cnt=%d, inst9->cmt_cmd=%d, inst10->cmd_cnt=%d, inst11=>cmd_cnt=%d inst12=>cmd_cnt=%d, inst13=>cmd_cnt=%d, inst14=>cmd_cnt=%d, inst15=>cmd_cnt=%d \n",
    ((debugInfo.regs[0x33] >> 0) & 0xf), ((debugInfo.regs[0x33] >> 4) & 0xf),
    ((debugInfo.regs[0x33] >> 8) & 0xf), ((debugInfo.regs[0x33] >> 12) & 0xf),
    ((debugInfo.regs[0x33] >> 16) & 0xf), ((debugInfo.regs[0x33] >> 20) & 0xf),
    ((debugInfo.regs[0x33] >> 24) & 0xf), ((debugInfo.regs[0x33] >> 28) & 0xf));
    VLOG(TRACE, "     STAGE_DEC/ENCODING      inst0->cmd_cnt=%d, inst1->cmt_cmd=%d, inst2->cmd_cnt=%d, inst3=>cmd_cnt=%d inst4=>cmd_cnt=%d, inst5=>cmd_cnt=%d, inst6=>cmd_cnt=%d, inst7=>cmd_cnt=%d \n",
    ((debugInfo.regs[0x34] >> 0) & 0xf), ((debugInfo.regs[0x34] >> 4) & 0xf),
    ((debugInfo.regs[0x34] >> 8) & 0xf), ((debugInfo.regs[0x34] >> 12) & 0xf),
    ((debugInfo.regs[0x34] >> 16) & 0xf), ((debugInfo.regs[0x34] >> 20) & 0xf),
    ((debugInfo.regs[0x34] >> 24) & 0xf), ((debugInfo.regs[0x34] >> 28) & 0xf));
    VLOG(TRACE, "     STAGE_DEC/ENCODING      inst8->cmd_cnt=%d, inst9->cmt_cmd=%d, inst10->cmd_cnt=%d, inst11=>cmd_cnt=%d inst12=>cmd_cnt=%d, inst13=>cmd_cnt=%d, inst14=>cmd_cnt=%d, inst15=>cmd_cnt=%d \n",
    ((debugInfo.regs[0x35] >> 0) & 0xf), ((debugInfo.regs[0x35] >> 4) & 0xf),
    ((debugInfo.regs[0x35] >> 8) & 0xf), ((debugInfo.regs[0x35] >> 12) & 0xf),
    ((debugInfo.regs[0x35] >> 16) & 0xf), ((debugInfo.regs[0x35] >> 20) & 0xf),
    ((debugInfo.regs[0x35] >> 24) & 0xf), ((debugInfo.regs[0x35] >> 28) & 0xf));
    VLOG(TRACE, "     STAGE_PACKAGING      inst0->cmd_cnt=%d, inst1->cmt_cmd=%d, inst2->cmd_cnt=%d, inst3=>cmd_cnt=%d inst4=>cmd_cnt=%d, inst5=>cmd_cnt=%d, inst6=>cmd_cnt=%d, inst7=>cmd_cnt=%d \n",
    ((debugInfo.regs[0x36] >> 0) & 0xf), ((debugInfo.regs[0x36] >> 4) & 0xf),
    ((debugInfo.regs[0x36] >> 8) & 0xf), ((debugInfo.regs[0x36] >> 12) & 0xf),
    ((debugInfo.regs[0x36] >> 16) & 0xf), ((debugInfo.regs[0x36] >> 20) & 0xf),
    ((debugInfo.regs[0x36] >> 24) & 0xf), ((debugInfo.regs[0x36] >> 28) & 0xf));
    VLOG(TRACE, "     STAGE_PACKAGING      inst8->cmd_cnt=%d, inst9->cmt_cmd=%d, inst10->cmd_cnt=%d, inst11=>cmd_cnt=%d inst12=>cmd_cnt=%d, inst13=>cmd_cnt=%d, inst14=>cmd_cnt=%d, inst15=>cmd_cnt=%d \n",
    ((debugInfo.regs[0x37] >> 0) & 0xf), ((debugInfo.regs[0x37] >> 4) & 0xf),
    ((debugInfo.regs[0x37] >> 8) & 0xf), ((debugInfo.regs[0x37] >> 12) & 0xf),
    ((debugInfo.regs[0x37] >> 16) & 0xf), ((debugInfo.regs[0x37] >> 20) & 0xf),
    ((debugInfo.regs[0x37] >> 24) & 0xf), ((debugInfo.regs[0x37] >> 28) & 0xf));
    VLOG(TRACE, "-- queued commands in each stage\n\n");
    VLOG(INFO, "-%s core=%d, inst=%d, bEncoder=%d\n", __FUNCTION__, handle->coreIdx, handle->instIndex, bEncoder);

}






long CalcFileSize(const char *path)
{
    long fileSize = 0;
    FILE* fp = NULL;

    fp = (FILE*)osal_fopen(path, "r");
    if (NULL != fp) {
        osal_fseek(fp, 0, SEEK_END);
        fileSize = osal_ftell(fp);
        //VLOG(INFO, "file size : %d\n", fileSize);
        osal_fclose(fp);
    }
    return fileSize;
}

int ExistOfFile(const char* path)
{
    int resOf = -1;
    if(NULL != path) {
        resOf = access(path, 0);
    }
    return resOf;
}



