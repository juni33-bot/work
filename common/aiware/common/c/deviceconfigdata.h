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

#ifndef AIW_COM_C__DEVICE_CONFIG_DATA_H
#define AIW_COM_C__DEVICE_CONFIG_DATA_H

#include "aiware/common/c/aiware-common-lib-c_export.h"
#include "aiware/common/c/memoryarea.h"
#include "aiware/common/c/status.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

	/// Creates a new instance of aiwDeviceConfigData and initializes it with the content of
	/// the passed file.
	///
	/// @param[in] path		The path of the fill which will be used to initialize the new
	///						instance. Can't be NULL. The file must exist and must contain valid
	///						device config data.
	///
	/// @return		If the new instance can be allocated and can be initialized with the content
	///				of the passed file, the function returns the pointer of the new instance.
	///				The returned object must be destroyed by the caller by using the method
	///				#aiwReleaseDeviceConfigData function. If any error happens, the function
	///				returns NULL and error callback is called.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiwDeviceConfigData* aiwLoadDeviceConfigData(const char* path);

	/// Creates a new instance of aiwDeviceConfigData and initializes it with the content of
	/// the passed file.
	///
	/// @param[in] file		Non-null, opened, read-capable file whose content will be
	///						deserialized.
	///
	/// @return		If the new instance can be allocated and can be initialized with the content
	///				of the passed file, the function returns the pointer of the new instance.
	///				The returned object must be destroyed by the caller by using the method
	///				#aiwReleaseDeviceConfigData function. If any error happens, the function
	///				returns NULL and error callback is called.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiwDeviceConfigData* aiwLoadDeviceConfigData2(FILE* file);

	/// Creates a new instance of aiwDeviceConfigData and initializes it with the content of the
	/// passed buffer.
	///
	/// The buffer should contain valid serialized device config data, otherwise the function
	/// fails.
	///
	/// @param[in] buffer			Pointer to the buffer which contains serialized device config
	///								data. Can't be NULL.
	/// @param[in] bufferSize		Size of the previous buffer in bytes.
	///
	/// @return		If the new instance can be allocated and can be initialized with the content
	///				of the passed buffer, the function returns the pointer of the new instance.
	///				The returned object must be destroyed by the caller by using the method
	///				#aiwReleaseDeviceConfigData function. If any error happens, the function
	///				returns NULL and the error callback will be called.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiwDeviceConfigData* aiwDeserializeDeviceConfigData(
		const uint8_t* buffer,
		uint32_t bufferSize);

	/// Saves device config data into file.
	///
	/// @param[in] dcd		Device config data instance to save, can't be NULL.
	/// @param[in] path		Path of the target file where the device config data will be saved.
	///						Can't be NULL. If the target file doesn't exist, it will be be
	///						created. If it's exist, it will be overwritten.
	///
	/// @return		On success the function return AIW_SUCCESS, otherwise AIW_ERROR and the
	///				error callback will be called.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiw_status aiwSaveDeviceConfigData(const aiwDeviceConfigData* dcd, const char* path);

	/// Saves device config data into file.
	///
	/// @param[in] dcd		Device config data instance to save, can't be NULL.
	/// @param[in] file		Opened, write-capable file where the device config data will be
	///						saved.
	///
	/// @return		On success the function return AIW_SUCCESS, otherwise AIW_ERROR and the
	///				error callback will be called.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiw_status aiwSaveDeviceConfigData2(const aiwDeviceConfigData* dcd, FILE* file);

	/// Returns the minimum size of a buffer which can contain the serialization of the passed
	/// device config data.
	///
	/// @param[in] dcd		Device config data instance to serialize, can't be NULL
	///
	/// @return		On success, the required buffer size will be return, otherwise 0 and the
	///				error callback will be called.
	///
	AIWARE_COMMON_LIB_C_EXPORT uint32_t aiwGetDeviceConfigDataSize(const aiwDeviceConfigData* dcd);

	/// Serializes a device config data instance into the passed buffer.
	///
	/// The buffer should be at least as large as the number returned by #aiwGetDeviceConfigDataSerializedBufferSize.
	/// If the buffer is smaller than that number, the function fails. If the buffer is larger,
	/// then the back of it will be untouched.
	///
	/// @param[in] dcd				Device config data to be serialized. Can't be NULL.
	/// @param[out] buffer			Target buffer where the serialized data will be placed. The
	///								length of the buffer is defined by \p bufferSize parameter.
	///								Can't be NULL.
	/// @param[in] bufferSize		Size of the buffer in bytes. The number must be at least as
	///								large as the value returned by #aiwGetDeviceConfigDataSerializedBufferSize
	///								function.
	///
	/// @return		On success the function returns AIW_SUCCESS. If the input device config or
	///				the buffer is invalid, or the buffer's size isn't large enough, the function
	///				fails, returns AIW_ERROR status and error callback will be called.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiw_status
	aiwSerializeDeviceConfigData(const aiwDeviceConfigData* dcd, uint8_t* buffer, uint32_t bufferSize);

	/// Copies the source device config data into the target.
	///
	/// @param[in] srcDcd	Source device config data instance, can't be NULL.
	/// @param[in] dstDcd	Destination device config data instance, can't be NULL.
	///
	/// @return		On success the function returns AIW_SUCCESS. If any parameter is NULL, then
	///				the function returns AIW_ERROR status.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiw_status
	aiwCopyDeviceConfigData(const aiwDeviceConfigData* srcDcd, aiwDeviceConfigData* dstDcd);

	/// Creates a new device config instance and copies the content of the parameter into it.
	///
	/// @param[in] dcd		Source device config data that will be copied.
	///
	/// @return		On success the function returns the new device config data instance whose
	///				content is the same as the parameter. The caller is responsible to release
	///				the returned object. If any error happens the function will return NULL and
	///				the error callback will be called.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiwDeviceConfigData* aiwCloneDeviceConfigData(const aiwDeviceConfigData* dcd);

	/// Returns the number of LAMs can be found in the passed device config data.
	///
	/// @param[in] dcd		Device config data, can't be NULL.
	///
	/// @return		If a valid device config data instance is passed to the function, then it
	///				returns a positive number. The returned object must be destroyed by the
	///				caller by using the method #aiwReleaseDeviceConfigData function. In case of
	///				any error, the function returns 0.
	///
	AIWARE_COMMON_LIB_C_EXPORT uint32_t aiwGetNumberOfLAMs(const aiwDeviceConfigData* dcd);

	/// Returns the number of memory areas can be found in the passed device config data.
	///
	/// @param[in] dcd		Device config data, can't be NULL.
	///
	/// @return		If a valid device config data instance is passed to the function, then it
	///				returns a positive number. In case of any error, the function returns 0.
	///
	AIWARE_COMMON_LIB_C_EXPORT int32_t aiwGetNumberOfMemoryAreas(const aiwDeviceConfigData* dcd);

	/// Returns the memory area of the passed device config data identified by the passed index.
	///
	/// @param[in] dcd		Device config data, can't be NULL.
	/// @param[in] index	Zero based index of the requested memory area. The number must be
	///						less than the value returned by #aiwGetNumberOfMemoryAreas function.
	///
	/// @return		If the passed device config data and index is valid, then the function
	///				returns a reference to the requested memory area object. The ownership of
	///				the returned object remains at the device config data object. If any
	///				parameter is invalid, then the function returns NULL.
	///
	AIWARE_COMMON_LIB_C_EXPORT const aiwMemoryArea* aiwGetMemoryArea(const aiwDeviceConfigData* dcd, uint32_t index);

	/// Destroys a device config data instance.
	///
	/// The function must be used when a device config data instance isn't used anymore, and
	/// the object is created by #aiwLoadDeviceConfigData, #aiwDeserializeDeviceConfigData and
	/// #aiwCreateAndCopyDeviceConfigData functions.
	///
	/// @param[in] dcd		Device config data to be destroyed, can't be NULL.
	///
	/// @return		On success, the function return AIW_SUCCESS, otherwise AIW_ERROR and the
	///				error callback will be called.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiw_status aiwReleaseDeviceConfigData(aiwDeviceConfigData* dcd);

	/// Compares 2 instances of device config data.
	///
	/// The 2 instances are equal if both of them are valid and contains exactly the same
	/// configuration.
	///
	/// @param[in] dcd1		First device config data instance.
	/// @param[in] dcd2		Second device config data instance.
	///
	/// @return		If both instances are valid and they are equal, then the function returns 1
	///				otherwise 0.
	///
	AIWARE_COMMON_LIB_C_EXPORT uint32_t
	aiwDeviceConfigDataCmpStrict(const aiwDeviceConfigData* dcd1, const aiwDeviceConfigData* dcd2);

	/// Compares the main attributes of 2 device config data instances.
	///
	/// The 2 instances are equal if both of them are valid and contains the same configuration
	/// except some memory attributes.
	///
	/// @param[in] dcd1		First device config data instance.
	/// @param[in] dcd2		Second device config data instance.
	///
	/// @return		If both instances are valid and they are equal, then the function returns 1
	///				otherwise 0.
	///
	AIWARE_COMMON_LIB_C_EXPORT uint32_t
	aiwDeviceConfigDataCmpRelaxed(const aiwDeviceConfigData* dcd1, const aiwDeviceConfigData* dcd2);

#ifdef __cplusplus
}
#endif

#endif //AIW_COM_C__DEVICE_CONFIG_DATA_H
