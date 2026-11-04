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

#ifndef AIWARE_COMMON_CPP__BINARY_HPP
#define AIWARE_COMMON_CPP__BINARY_HPP

#include "aiware/common/c/tensordimensions.h"
#include "aiware/common/cpp/aiware-common-lib-cpp_export.h"
#include "aiware/common/cpp/deviceconfigdata.hpp"

#include <memory>
#include <stdio.h>
#include <string>
#include <vector>

namespace aiware
{
namespace common
{

class AIWARE_COMMON_LIB_CPP_EXPORT TensorInfo
{
public:
	TensorInfo() = default;

	TensorInfo(const TensorInfo&) = default;

	TensorInfo(TensorInfo&&) = default;

	TensorInfo& operator=(const TensorInfo&) = default;

	TensorInfo& operator=(TensorInfo&&) = default;

	bool isValid() const;

	uint32_t id() const;

	std::string name() const;

	const aiwTensorDimensions& dim() const;

	std::vector<uint32_t> originalDim() const;

	bool sign() const;

	int8_t exponent() const;

	bool operator==(const TensorInfo& other) const;
	bool operator!=(const TensorInfo& other) const;

protected:
	TensorInfo(void* ti);

private:
	friend class Binary;
	friend class SubBinary;

protected:
	void* _ti = nullptr;
};

class AIWARE_COMMON_LIB_CPP_EXPORT Binary
{
public:
	using Ptr = std::unique_ptr<Binary>;

public:
	static Binary::Ptr load(const char* path);

	static Binary::Ptr load(FILE* file);

	static Binary::Ptr load(const uint8_t* buffer, uint32_t bufferSize);

public:
	Binary(const Binary&) = delete;
	Binary(Binary&&) = delete;
	Binary& operator=(const Binary&) = delete;
	Binary& operator=(Binary&&) = delete;

	virtual ~Binary();

	Ptr clone() const;

	const DeviceConfigData& deviceConfigData() const;

	const char* name() const;

	uint32_t inputTensorCount() const;

	TensorInfo inputTensor(uint32_t index) const;

	uint32_t outputTensorCount() const;

	TensorInfo outputTensor(uint32_t index) const;

	uint32_t nnuVariantCount() const;

	uint32_t nnuVariant(uint32_t index) const;

	bool isNNUNumberSupported(uint32_t nnuNumber) const;

	bool save(const char* path, bool compress = true) const;

	bool save(FILE* file, bool compress = true) const;

	std::vector<uint8_t> save(bool compress = true) const;

protected:
	Binary(void* data, bool owned);

protected:
	void* _data = nullptr;
	bool _owned = false;
	DeviceConfigData::Ptr _config;
};

} // namespace common
} // namespace aiware

#endif //AIWARE_COMMON_CPP__BINARY_HPP
