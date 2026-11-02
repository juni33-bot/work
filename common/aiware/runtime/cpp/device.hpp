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

#ifndef AIWARE_RUNTIME_COMMON__DEVICE_HPP
#define AIWARE_RUNTIME_COMMON__DEVICE_HPP

#include "aiware/common/cpp/deviceconfigdata.hpp"
#include "aiware/runtime/cpp/aiware-runtime-common-lib-cpp_export.h"
#include "aiware/runtime/cpp/runtimeinf.hpp"

#include <memory>

namespace aiware
{
namespace runtime
{

class ProgramSet;

/// Represents and aiWare device
class AIWARE_RUNTIME_COMMON_LIB_CPP_EXPORT Device : public inf::Device
{
public:
	using Ptr = std::unique_ptr<Device>;

public:
	/// Destructor
	~Device() override;

	/// Returns the device's configuration.
	const aiware::common::DeviceConfigData& deviceConfigData() const;

	uint32_t programCount() const override;
	const inf::Program* program(uint32_t index) const override;
	inf::Program* program(uint32_t index) override;

	/// Returns the number of program sets associated with the current device.
	uint32_t programSetCount() const;

	/// Returns the Nth program set.
	///
	/// @param[in] index		Index of the requested device, must be less than the value
	///							returned by #programSetCount.
	///
	/// @return		If the device and the passed index is valid, the functions returns a
	///				non-null pointer to the requested program set. The ownership of the
	///				returned object remains at the device. In case of an invalid device or
	///				index the function returns nullptr.
	///
	const ProgramSet* programSet(uint32_t index) const;

	/// Returns the Nth program set, non-const version.
	ProgramSet* programSet(uint32_t index);

	/// Deletes the Nth program set.
	///
	/// @return		If the index is invalid, the function returns false, otherwise it
	///				destroys the requested program set and returns true.
	///
	bool deleteProgramSet(uint32_t index);

	/// Deletes the passed program set if it belongs to the current device.
	///
	/// @param[in] programSet		Program set to delete. The program set must belong to
	///								the current device.
	///
	/// @return		If the passed programSet belongs to the device, then the function
	///				destroys it and returns true, otherwise returns false.
	bool deleteProgramSet(ProgramSet& programSet);

	inf::Buffer::Ordering rawBufferOrdering() const;

	/// Returns if the device supports tensor export or not.
	///
	/// @return		Returns true if the device supports tensor export, otherwise false.
	bool tensorExportSupported() const override;

	bool setSelftestExecutionPolicy(aiwSelftestExecutionPolicy policy) override;

	bool selftestExecutionPolicy(aiwSelftestExecutionPolicy& policy) const override;

	inf::Status executeSelftest() override;

protected:
	Device(void* impl, std::unique_ptr<aiware::common::DeviceConfigData> dcd);

	void updateProgramSets();

	const inf::Program* getProgram(uint32_t index) const;

private:
	friend void updateProgramSets(Device& device);

	using ProgramSetPtr = std::unique_ptr<ProgramSet>;
	using ProgramSetPtrVec = std::vector<ProgramSetPtr>;

protected:
	void* _impl = nullptr;
	ProgramSetPtrVec* _programSets = nullptr;
	bool _owned = false;
	std::unique_ptr<aiware::common::DeviceConfigData> _dcd;
};

} // namespace runtime
} // namespace aiware

#endif //AIWARE_RUNTIME_COMMON__DEVICE_HPP
