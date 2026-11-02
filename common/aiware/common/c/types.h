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

#ifndef AIWARE_COMMON_C__TYPES_H
#define AIWARE_COMMON_C__TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
	/// 8-bit unsigned integer number type - deprecated and will be removed: use uint8_t instead
	typedef uint8_t aiw_u8_t;

	/// 16-bit unsigned integer number type - deprecated and will be removed: use uint16_t instead
	typedef uint16_t aiw_u16_t;

	/// 32-bit unsigned integer number type - deprecated and will be removed: use uint32_t instead
	typedef uint32_t aiw_u32_t;

	/// 64-bit unsigned integer number type - deprecated and will be removed: use uint64_t instead
	typedef uint64_t aiw_u64_t;

	/// 8-bit signed integer number type - deprecated and will be removed: use int8_t instead
	typedef int8_t aiw_i8_t;

	/// 16-bit signed integer number type - deprecated and will be removed: use int16_t instead
	typedef int16_t aiw_i16_t;

	/// 32-bit signed integer number type - deprecated and will be removed: use int32_t instead
	typedef int32_t aiw_i32_t;

	/// 64-bit signed integer number type - deprecated and will be removed: use int64_t instead
	typedef int64_t aiw_i64_t;

	/// Character type
	typedef char aiw_char_t;

	/// 32-bit floating point number
	typedef float aiw_f32_t;

	/// Represents a binary instance.
	typedef struct aiwBinaryImpl aiwBinary;

	/// Contains information of a tensor.
	typedef struct aiwTensorInfoImpl aiwTensorInfo;

	/// Contains all information of an aiWare device.
	typedef struct aiwDeviceConfigDataImpl aiwDeviceConfigData;

	/// Contains basic information about a memory area accessible by aiWare.
	typedef struct aiwMemoryAreaImpl aiwMemoryArea;

#ifdef __cplusplus
}
#endif

#endif //AIWARE_COMMON_C__TYPES_H
