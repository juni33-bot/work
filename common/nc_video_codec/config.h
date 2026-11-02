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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#if defined(_MSC_VER)
#	include <windows.h>
#	define inline _inline
#elif defined(__GNUC__)
#elif defined(__ARMCC__)
#else
#  error "Unknown compiler."
#endif

#define API_VERSION_MAJOR       5
#define API_VERSION_MINOR       5
#define API_VERSION_PATCH       73
#define API_VERSION             ((API_VERSION_MAJOR<<16) | (API_VERSION_MINOR<<8) | API_VERSION_PATCH)

#if defined(PLATFORM_NON_OS) || defined (ANDROID) || defined(MFHMFT_EXPORTS) || defined(PLATFORM_QNX)
//#define SUPPORT_FFMPEG_DEMUX
#else
#define SUPPORT_FFMPEG_DEMUX
#endif

//------------------------------------------------------------------------------
// COMMON
//------------------------------------------------------------------------------
#if defined(linux) || defined(__linux) || defined(ANDROID) || defined(CNM_FPGA_HAPS_INTERFACE)
#define SUPPORT_MULTI_INST_INTR
#endif
#if defined(linux) || defined(__linux) || defined(ANDROID)
#define SUPPORT_INTERRUPT
#endif

// do not define BIT_CODE_FILE_PATH in case of multiple product support. because wave410 and coda980 has different firmware binary format.
#define CORE_0_BIT_CODE_FILE_PATH   "coda960.out"               // for coda960
#define CORE_1_BIT_CODE_FILE_PATH   "coda980.out"               // for coda980
#define CORE_6_BIT_CODE_FILE_PATH   "/lib/wave_fw/chagall.bin"  // for wave521
#define CORE_7_BIT_CODE_FILE_PATH   "vincent.bin"               // for wave517

//------------------------------------------------------------------------------
// OMX
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// WAVE521C
//------------------------------------------------------------------------------
//#define SUPPORT_SOURCE_RELEASE_INTERRUPT
//#define SUPPORT_READ_BITSTREAM_IN_ENCODER

//#define SUPPORT_SW_UART
// #define SUPPORT_SW_UART_V2	// WAVE511 or WAVE521C
// #define SUPPORT_SW_UART_ON_NONOS
#ifdef SUPPORT_SW_UART_ON_NONOS
    #if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
    #else
    #error "SUPPORT_SW_UART_ON_NONOS define needs (#if defined(SUPPORT_SW_UART_V2) || defined(SUPPORT_SW_UART))"
    #endif
    #undef SUPPORT_MULTI_INST_INTR
    #undef SUPPORT_INTERRUPT
#endif

#endif /* __CONFIG_H__ */