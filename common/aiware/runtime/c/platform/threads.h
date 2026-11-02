/*
** Copyright (c) AImotive Kft. 2024
**
** The intellectual and technical concepts and implementations contained herein (including
** data structures, algorithms and essential business logic developed by AImotive Kft.) are
** proprietary to AImotive Kft., and may be covered by patents, and/or copyright law. This
** hardware or software is protected by trade secret, confidential business secret and as a
** general principle must be treated as confidential information.
**
** You may not use this hardware or software without specific prior written permission
** obtained from AImotive Kft.
**
** Access to this hardware or software is hereby forbidden to anyone except for Contracted
** Partners who have prior signed License Agreement, or Confidentiality, Non-Disclosure
** Agreements or any other equivalent Agreements explicitly covering such access and use.
**
** UNLESS OTHERWISE AGREED, THIS HARDWARE OR SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS
** OR IMPLIED WARRANTIES, INCLUDING - BUT NOT LIMITED TO - THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
*/

#ifndef AIWARE_RUNTIME_PRIVATE__THREADS_H
#define AIWARE_RUNTIME_PRIVATE__THREADS_H

#include "aiware/common/c/status.h"
#include "aiware/common/c/types.h"
#include "aiware/runtime/c/aiware-runtime-device-lib-c_export.h"
#include "aiware/runtime/c/platform/defines.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwPlatformThreadCreate(aiwThread* threadId, void (*startRoutine)(void*), void* arg);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT void aiwPlatformThreadJoin(aiwThread* threadId);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwPlatformMutexCreate(aiwMutex* mutex);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT void aiwPlatformMutexDestroy(aiwMutex* mutex);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwPlatformMutexLock(aiwMutex* mutex);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwPlatformMutexTimedLock(aiwMutex* mutex, uint32_t timeoutMsec);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwPlatformMutexUnlock(aiwMutex* mutex);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwPlatformCondCreate(aiwCond* cond);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT void aiwPlatformCondDestroy(aiwCond* cond);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwPlatformCondWaitWithTimeout(aiwCond* cond, aiwMutex* mutex, uint32_t timeoutMsec);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwPlatformCondWait(aiwCond* cond, aiwMutex* mutex);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwPlatformCondSignal(aiwCond* cond);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT void aiwPlatformSleepMs(size_t milliseconds);

#ifdef __cplusplus
}
#endif

#endif // AIWARE_RUNTIME_PRIVATE__THREADS_H
