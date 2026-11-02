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

#ifndef AIWARE_COMMON_C__BINARY_H
#define AIWARE_COMMON_C__BINARY_H

#include "aiware/common/c/aiware-common-lib-c_export.h"
#include "aiware/common/c/deviceconfigdata.h"
#include "aiware/common/c/status.h"
#include "aiware/common/c/tensordimensions.h"
#include "aiware/common/c/types.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

/// Maximum length of a tensor name in binary
#define AIWARE_TENSOR_NAME_MAX_LEN (uint32_t)64

/// Maximum length of a tensor dimension in binary
#define AIWARE_DIMENSION_VECTOR_MAX_LEN (uint32_t)20

	/// Contains information of a tensor.
	struct aiwTensorInfoImpl
	{
		/// Id of the tensor in the original neural network.
		uint32_t id;

		/// Name of the tensor. NULL terminated string.
		aiw_char_t name[AIWARE_TENSOR_NAME_MAX_LEN];

		/// Dimensions of the tensor.
		aiwTensorDimensions dim;

		/// Original tensor dimension specified in the NNEF graph
		uint32_t originalDimCount;
		uint32_t originalDim[AIWARE_DIMENSION_VECTOR_MAX_LEN];

		/// Sign of the tensor. 0 - unsigned, 1 - signed, other values are not used.
		uint8_t sign;

		/// Exponent of the tensor.
		int8_t exponent;
	};

	/// Creates a new binary by loading its content from a file defined by a path.
	///
	/// @param[in] path		Path of the file which contains a serialized binary. Can't be NULL,
	///						and must point to an existing file.
	///
	/// @return		On success the function returns a new binary instance. The caller is
	///				responsible for destroying the returned object. If the passed path is NULL,
	///				or the content of the file is invalid, the function returns NULL.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiwBinary* aiwBinaryLoadFromPath(const char* path);

	/// Creates a new binary by loading its content from a file.
	///
	/// @param[in] file		Pointer to an opened file which contains the serialized binary.
	///						Can't be NULL, and the file must be opened for read.
	///
	/// @return		On success the function returns a new binary instance. The caller is
	///				responsible for destroying the returned object. If the passed file is NULL,
	///				or its content is invalid, the function returns NULL.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiwBinary* aiwBinaryLoadFromFile(FILE* file);

	/// Creates a new binary by loading its content from a memory buffer.
	///
	/// @param[in] buffer			Pointer to the buffer which contains the serialized binary.
	///								Can't be NULL.
	/// @param[in] bufferSize		Size of the buffer in bytes. Can't be 0.
	///
	/// @return		On success the function returns a new binary instance. The caller is
	///				responsible for destroying the returned object. If the passed buffer or its
	///				content is invalid the function returns NULL.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiwBinary* aiwDeserializeBinary(const uint8_t* buffer, uint32_t bufferSize);

	/// Serializes the passed binary into a file.
	///
	/// The function has some options that can affect the serialization process:
	/// - Compression: some internal buffers can be serialized in compressed form, if the
	///				   compressed buffer size is smaller than the original one's.
	///
	/// @param[in] binary			The binary to save. Can't be NULL and must be a valid
	///								binary object.
	/// @param[in] path				Path of the destination file. Can't be NULL. If the file
	///								doesn't exist, it will be created, if it exists, it will be
	///								overwritten.
	/// @param[in] compress			If 0 the binary will be written in an uncompressed way, if
	///								1, internal buffers will be compressed.
	///
	/// @return		Returns AIW_SUCCESS on success, otherwise AIW_ERROR.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiw_status aiwSaveBinary(const aiwBinary* binary, const char* path, uint8_t compress);

	/// Serializes the passed binary into a file.
	///
	/// The function has some options that can affect the serialization process:
	/// - Compression: some internal buffers can be serialized in compressed form, if the
	///				   compressed buffer size is smaller than the original one's.
	///
	/// @param[in] binary			The binary to save. Can't be NULL and must be a valid
	///								binary object.
	/// @param[in] file				An opened file where the serialized data will be saved into.
	///								Can't be NULL, and the file must be opened for writing.
	/// @param[in] compress			If 0 the binary will be written in an uncompressed way, if
	///								1, internal buffers will be compressed.
	///
	/// @return		Returns AIW_SUCCESS on success, otherwise AIW_ERROR.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiw_status aiwSaveBinary2(const aiwBinary* binary, FILE* file, uint8_t compress);

	/// Returns the size of the buffer which can store the passed buffer after serialization.
	///
	/// @param[in] binary		The binary to serialize. Must be a valid binary object.
	///
	/// @return		On success the function returns a positive integer, which is the size of the
	///				serialization buffer. In case of any error the function returns 0.
	///
	AIWARE_COMMON_LIB_C_EXPORT uint32_t aiwGetBinarySize(const aiwBinary* binary);

	/// Serializes a binary into a memory buffer.
	///
	/// The required size of the buffer can be determined by calling the #aiwGetBinarySerializedBufferSize
	/// method. However the previous function returns the maximum size of the serialized binary.
	/// Compression and omitting weights can reduce the size of the serialized data, in this
	/// case back of the buffer won't be used. The function returns the exact size of the
	/// serialized data.
	///
	/// The function has some options that can affect the serialization process:
	/// - Compression: some internal buffers can be serialized in compressed form, if the
	///				   compressed buffer size is smaller than the original one's.
	///
	/// @param[in] binary			The binary to serialize. Must be a valid binary object.
	/// @param[out] buffer			The destination buffer. Can't be NULL.
	/// @param[in] bufferSize		Size of the previous buffer in bytes.
	/// @param[in] compress			If 0 the binary will be written in an uncompressed way, if
	///								1, internal buffers will be compressed.
	///
	/// @return		If the serialization was successful, the function returns the number of
	///				bytes that were actually used from the whole buffer. (The number is always
	///				less than or equal to \p bufferSize.) In case of any error, the
	///				function returns 0.
	///
	AIWARE_COMMON_LIB_C_EXPORT uint32_t
	aiwSerializeBinary(const aiwBinary* binary, uint8_t* buffer, uint32_t bufferSize, uint8_t compress);

	/// Returns the device config data which the current binary was built for.
	///
	/// @return		Returns a const ref to the binary's device config data object. If the
	///				passed object is null or invalid, then returns NULL.
	///
	AIWARE_COMMON_LIB_C_EXPORT const aiwDeviceConfigData* aiwBinaryGetDeviceConfigData(const aiwBinary* binary);

	/// Returns the name of a binary.
	///
	/// @return		Returns a pointer to a NULL-terminated string. If the passed object is NULL
	///				or invalid, the function returns NULL.
	///
	AIWARE_COMMON_LIB_C_EXPORT const aiw_char_t* aiwGetBinaryName(const aiwBinary* binary);

	/// Returns the number of the binary's input tensors.
	///
	/// @return		Returns the number of input tensors. If the passed object is NULL
	///				or invalid, the function returns 0.
	///
	AIWARE_COMMON_LIB_C_EXPORT uint32_t aiwBinaryGetInputTensorCount(const aiwBinary* binary);

	/// Returns info of the Nth input tensor of the passed binary.
	///
	/// @param[in] inputTensorIndex		Index of the input tensor, must be less than the value
	///									returned by #aiwBinaryGetInputTensorCount.
	///
	/// @return		If the passed binary object and the input tensor index is valid, then the
	///				function returns a pointer to the tensor info structure. The ownership of
	///				the returned object remains at the binary, it must not be deleted. If any
	///				parameter of the function is invalid the function return NULL.
	///
	AIWARE_COMMON_LIB_C_EXPORT const aiwTensorInfo* aiwBinaryGetInputTensorInfo(
		const aiwBinary* binary,
		uint32_t inputTensorIndex);

	/// Returns the number of the binary's output tensors.
	///
	/// @return		Returns the number of output tensors. If the passed object is NULL
	///				or invalid, the function returns 0.
	///
	AIWARE_COMMON_LIB_C_EXPORT uint32_t aiwBinaryGetOutputTensorCount(const aiwBinary* binary);

	/// Returns info of the Nth output tensor of the passed binary.
	///
	/// @param[in] outputTensorIndex		Index of the input tensor, must be less than the
	///										value returned by #aiwBinaryGetInputTensorCount.
	///
	/// @return		If the passed binary object and the output tensor index is valid, then the
	///				function returns a pointer to the tensor info structure. The ownership of
	///				the returned object remains at the binary, it must not be deleted. If any
	///				parameter of the function is invalid the function return NULL.
	///
	AIWARE_COMMON_LIB_C_EXPORT const aiwTensorInfo* aiwBinaryGetOutputTensorInfo(
		const aiwBinary* binary,
		uint32_t outputTensorIndex);

	/// Destroys a binary.
	///
	/// The binary must be created by the #aiwBinaryLoadFromPath, #aiwBinaryLoadFromFile or #aiwDeserializeBinary
	/// function.
	///
	/// @param[in] binary		The binary to destroy. Can't be NULL.
	///
	/// @return		The function returns AIW_SUCCESS on success, otherwise AIW_ERROR.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiw_status aiwReleaseBinary(aiwBinary* binary);

#ifdef __cplusplus
}
#endif

#endif //AIWARE_COMMON_C__BINARY_H
