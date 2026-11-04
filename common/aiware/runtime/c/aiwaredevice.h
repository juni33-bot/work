/*
** Copyright (c) AImotive Kft. 2020
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

#ifndef AIWARE_RUNTIME__AIWARE_DEVICE_H
#define AIWARE_RUNTIME__AIWARE_DEVICE_H

#include "aiware/common/c/status.h"
#include "aiware/common/c/types.h"
#include "aiware/runtime/c/aiware-runtime-device-lib-c_export.h"
#include "aiware/runtime/c/runtimedevice.h"

#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif

	/// Returns the number of aiWare devices can be found in the system.
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT uint32_t aiwRuntimeDeviceCount(void);

	/// Returns the aiwWare device identified by the passedIndex.
	///
	/// Opens and initializes an aiWare device. If a device identified by a specific index
	/// is already opened, then it can't be opened again. The caller must close the device
	/// by calling the #aiwDeviceClose function when it's no longer needed.
	///
	/// @param[in] deviceIndex      Index of the device to open. The value must be less
	///                             the value returned by #aiwDeviceCount.
	///
	/// @return     If the index is valid and the device could be opened successfully, then
	///             the function returns a non-null pointer to the device. In case of any
	///             error the function return NULL.
	///
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDeviceOpen(struct aiwRuntimeDevice* device, uint32_t deviceIndex);

#ifdef __cplusplus
}
#endif

#endif // AIWARE_RUNTIME__AIWARE_DEVICE_H
