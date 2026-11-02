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

#ifndef AIWARE_RUNTIME_COMMON__PROGRAM_HPP
#define AIWARE_RUNTIME_COMMON__PROGRAM_HPP

#include "aiware/common/cpp/binary.hpp"
#include "aiware/runtime/cpp/aiware-runtime-common-lib-cpp_export.h"
#include "aiware/runtime/cpp/runtimeinf.hpp"

#include <memory>
#include <vector>

namespace aiware
{
namespace runtime
{

class Device;
class ProgramSet;

using Tensor = inf::Tensor;
class Buffer;
class TensorImpl;

class AIWARE_RUNTIME_COMMON_LIB_CPP_EXPORT Program : public inf::Program
{
public:
	Program() = delete;
	~Program() override;

	uint32_t inputTensorCount() const override;
	const Tensor* inputTensor(uint32_t index) const override;
	Tensor* inputTensor(uint32_t) override;

	uint32_t outputTensorCount() const override;
	const Tensor* outputTensor(uint32_t index) const override;
	Tensor* outputTensor(uint32_t) override;

	inf::Status execute() override;

	inf::Status executeAsync() override;
	inf::Status await() override;
	bool asyncExecutionSupported() const override;

	inf::Status uploadInputs() override;
	inf::Status downloadOutputs() override;

	bool memoryTransferTimeoutSupported() const override;
	bool executionTimeoutSupported() const override;

	bool setMemoryTransferTimeout(uint32_t transferTimeoutMs) override;
	bool setExecutionTimeout(uint32_t executionTimeoutMs) override;

	bool getMemoryTransferTimeout(uint32_t& transferTimeoutMs) const override;
	bool getExecutionTimeout(uint32_t& executionTimeoutMs) const override;

	bool enableSelftest(bool enabled) override;

	Device& device() const;

	ProgramSet& programSet() const;

	const aiware::common::Binary& binary() const;

	uint32_t executionTimeMsec() const;

	uint64_t executionTimeClockCycles() const;

	inf::Buffer::Ordering rawBufferOrdering() const;

protected:
	Program(Device& device, ProgramSet& programSet, const aiware::common::Binary& binary, void* impl);

protected:
	Device& _device;
	ProgramSet& _programSet;
	const aiware::common::Binary& _binary;
	void* _impl = nullptr;
	std::vector<std::unique_ptr<TensorImpl>> _inputTensors;
	std::vector<std::unique_ptr<TensorImpl>> _outputTensors;
};

} // namespace runtime
} // namespace aiware

#endif //AIWARE_RUNTIME_COMMON__PROGRAM_HPP
