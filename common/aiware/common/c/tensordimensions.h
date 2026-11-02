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

#ifndef AIW_COM_C__TENSOR_DIMENSIONS_H
#define AIW_COM_C__TENSOR_DIMENSIONS_H

#include "aiware/common/c/types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef __cplusplus
	typedef
#endif
		struct
#ifdef __cplusplus
		aiwTensorDimensions
#endif
	{
		uint32_t w;
		uint32_t h;
		uint32_t ch;
		uint32_t b;

#ifndef __cplusplus
	} aiwTensorDimensions;
#else
	constexpr aiwTensorDimensions(uint32_t width = 1, uint32_t height = 1, uint32_t channels = 1, uint32_t batches = 1)
		: w(width)
		, h(height)
		, ch(channels)
		, b(batches)
	{
	}

	bool operator==(const aiwTensorDimensions& other) const
	{
		return w == other.w && h == other.h && ch == other.ch && b == other.b;
	}

	bool operator!=(const aiwTensorDimensions& other) const { return !(*this == other); }

	uint32_t mulDimensions() const { return w * h * ch * b; }

	uint32_t getAxis(uint32_t axis) const
	{
		const uint32_t* ptr = getAxisInternal(axis);
		return nullptr != ptr ? *ptr : 0u;
	}

	void setAxis(uint32_t axis, uint32_t value)
	{
		uint32_t* ax = const_cast<uint32_t*>(getAxisInternal(axis));
		if (ax)
			*ax = value;
	}

private:
	const uint32_t* getAxisInternal(uint32_t axis) const
	{
		switch (axis)
		{
		case 1:
			return &ch;
		case 2:
			return &h;
		case 3:
			return &w;
		default:
			return nullptr;
		}
	}
};
#endif

#ifdef __cplusplus
}
#endif

#endif //AIW_COM_C__TENSOR_DIMENSIONS_H
