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

#ifndef AIWARE_RUNTIME_COMMON_C__DEVICE_H
#define AIWARE_RUNTIME_COMMON_C__DEVICE_H

#include "aiware/common/c/deviceconfigdata.h"
#include "aiware/runtime/c/aiware-runtime-common-lib-c_export.h"
#include "aiware/runtime/c/rawbufferordering.h"
#include "aiware/runtime/c/types.h"

#include <stdbool.h>
#ifdef __cplusplus
extern "C"
{
#endif

	/// Opens a device.
	///
	/// Allocates all resources used by the device. The device can't be used after this
	/// function called.
	///
	/// @param[in] deviceIndex		The device index that should be opened.
	///
	/// @return		NULL on error, a pointer to an aiwDevice otherwise.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwDevice* aiwDeviceOpen(uint32_t deviceIndex);

	/// Returns the number of devices found in the system.
	///
	/// @return   The number of devices accessible for the runtime.
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT uint32_t aiwDeviceCount(void);

	/// Gets the configuration of a device
	///
	/// The ownership of the returned object remains at the device, the caller don't have to
	/// release it.
	///
	/// @param[in] device		The device whose config the user interested in. Can't be NULL.
	///
	/// @return		If the passed device pointer is NULL, then returns NULL. Otherwise the
	///				function returns the device config data object that belongs to the device.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT const aiwDeviceConfigData* aiwDeviceGetConfig(aiwDevice* device);

	/// Closes the passed device.
	///
	/// Releases all resource allocated by the device. The device can't be used after this
	/// function called.
	///
	/// @param[in] device		The device that should be closed. Can't be NULL.
	///
	/// @return		If NULL pointer is passed or the device couldn't be closed successfully,
	///				the function returns AIW_ERROR, otherwise AIW_SUCCESS.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status aiwDeviceClose(aiwDevice* device);

	/// Free all programs (device and host memory) currently associated with the device.
	/// This function is also called on device destruction.
	///
	/// @param[in] device		The device whose programs should be freed.
	///
	/// @return		If the passed device is NULL or invalid or an error happens internally.
	/// the function returns AIW_ERROR.
	///
	/// @note This function does not need to be called explicitly if aiwDeviceClose is called,
	/// as it is called internally. It is provided for convenience in case the user wants to
	/// free the programs without closing the device to avoid the overhead of reopening the device.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status aiwDeviceFreePrograms(aiwDevice* device);

	/// Returns the number of program sets associated to the device
	///
	/// @return		If the passed device is NULL or invalid, the function returns -1. Otherwise
	///				the function return a non-negative value.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT int32_t aiwDeviceProgramSetCount(const aiwDevice* device);

	/// Returns the Nth program set.
	///
	/// @param[in] programSetIndex		Index of the requested program set, must be less than
	///									the value returned by #aiwDeviceProgramSetCount (if it
	///									was positive).
	///
	/// @return		If the passed device and the index is valid, then the function returns a
	///				pointer to the requested program set. THe ownership of the returned object
	///				remains at the device, it must not be deleted. If any of the parameters is
	///				invalid, then the function returns NULL.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwProgramSet* aiwDeviceGetProgramSet(
		aiwDevice* device,
		uint32_t programSetIndex);

	/// Deletes the Nth program set.
	///
	/// @param[in] programSetIndex		Index of the requested program set, must be less than
	///									the value returned by #aiwDeviceProgramSetCount (if it
	///									was positive).
	/// @return		If the passed device and the index is valid, then the function destroys
	///				the program set identified by the index, and returns AIW_SUCCESS. If any
	///				parameter is invalid, then no program set will be deleted and the function
	///				returns AIW_ERROR.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status
	aiwDeviceDeleteProgramSet(aiwDevice* device, uint32_t programSetIndex);

	/// Returns the ordering type of all raw tensor buffers of all tensors that belong to the passed
	/// device.
	///
	/// The programs and their tensors also have a function to query this property, but all of them
	/// returns the same ordering type like the associated device does.
	///
	/// @return		Returns AIW_RTBO_INVALID if the passed device pointer is null or invalid. Otherwise
	///				returns one of the valid ids of #aiwRawTensorBufferOrdering enum.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwRawTensorBufferOrdering aiwDeviceRawTensorBufferOrdering(aiwDevice* device);

	/// Returns if the device supports tensor export.
	///
	/// @return		Returns AIW_ERROR if the passed device pointer is null or invalid or the device
	///				doesn't support tensor export. Otherwise returns AIW_SUCCESS.
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status aiwDeviceHasTensorExportSupport(aiwDevice* device);

#ifdef __cplusplus
}
#endif

#endif //AIWARE_RUNTIME_COMMON_C__DEVICE_H
