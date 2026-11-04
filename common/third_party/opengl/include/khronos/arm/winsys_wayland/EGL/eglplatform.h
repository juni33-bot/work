/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2011-2014, 2017-2018, 2020-2022 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file
 * Platform related definitions
 */

#ifndef _EGLPLATFORM_H_
#define _EGLPLATFORM_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#if defined(EGL_WAYLAND) && EGL_WAYLAND
#ifndef WL_EGL_PLATFORM
#define WL_EGL_PLATFORM 1
#endif
#include <khronos/original/EGL/eglplatform.h>
#else
#error "Unsupported platform"
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _EGLPLATFORM_H_ */
