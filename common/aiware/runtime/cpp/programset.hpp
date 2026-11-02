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

#ifndef AIWARE_RUNTIME_COMMON__PROGRAM_SET_HPP
#define AIWARE_RUNTIME_COMMON__PROGRAM_SET_HPP

#include "aiware/runtime/cpp/aiware-runtime-common-lib-cpp_export.h"
#include "aiware/runtime/cpp/program.hpp"

#include <memory>
#include <vector>

namespace aiware
{
namespace common
{
class Binary;
}
} // namespace aiware

namespace aiware
{
namespace runtime
{

class Device;

/// Represents a program set object.
class AIWARE_RUNTIME_COMMON_LIB_CPP_EXPORT ProgramSet final
{
public:
	ProgramSet() = delete;
	ProgramSet(const ProgramSet&) = delete;
	ProgramSet(ProgramSet&&) = delete;

	ProgramSet& operator=(const ProgramSet&) = delete;
	ProgramSet& operator=(ProgramSet&&) = delete;

	~ProgramSet();

	/// Returns whether the instance is a valid program set object or not.
	bool isValid() const;

	/// Returns the device where the current program set is associated to.
	///
	/// @return		If the program set is valid, the function returns a non-null pointer
	///				to the associated device, otherwise nullptr.
	///
	Device& device() const;

	/// Returns the number of binaries that belong to the program set.
	uint32_t binaryCount() const;

	/// Returns the Nth binary of the program set.
	///
	/// @param[in] index	Index of the requested binary, must be less than the value
	///						returned by #binaryCount.
	///
	/// @return		If the passed index is valid, then the function returns the a pointer
	///				to the requested binary object. The ownership of the returned object
	///				remains at the program set, it must not be deleted. If the passed index
	///				or the program set itself is invalid, nullptr will be returned.
	///
	const aiware::common::Binary* binary(uint32_t index) const;

	/// Returns the number of programs in the program set.
	uint32_t programCount() const;

	/// Returns the Nth program of the set.
	///
	/// @param[in] index	Index of the requested program set, must be less than the value
	///						returned by #programCount.
	///
	/// @return		If the index is valid, a pointer will be returned to the requested
	///				program. The ownership of the returned object remains at the program
	///				set, it must not be deleted. If the passed index or the program set
	///				object itself is invalid, the function returns nullptr.
	///
	const Program* program(uint32_t index) const;

	/// Returns the Nth program of the set, non-const version.
	Program* program(uint32_t);

private:
	ProgramSet(Device& device, void* impl);

	friend class Device;

	void* internal() const;

private:
	void* _impl = nullptr;
	Device& _device;
	std::vector<std::unique_ptr<aiware::common::Binary>> _binaries;
	std::vector<std::unique_ptr<Program>> _programs;
};

/// Represents a program set builder.
///
/// This object can be used to build programs from binaries for a specific device. Every
/// builder object can be used only once: after the #finish method is called, the object
/// can't be used anymore.
///
class AIWARE_RUNTIME_COMMON_LIB_CPP_EXPORT ProgramSetBuilder final
{
public:
	/// Creates a simple program set from a binary for a device.
	///
	/// The passed binary instance will be copied, an one program instance will be built
	/// from it for the passed device. The binary must be built for the device. The new
	/// built program set will be registered to the device and it will be the owner, so the
	/// caller doesn't have to release it.
	///
	/// @return		If the passed device and binary is valid and if they are compatible,
	///				then the function returns the pointer of the new program set. Otherwise
	///				the function returns null.
	///
	static Program* buildSimpleProgram(Device& device, const aiware::common::Binary& binary);

	/// Creates a simple program set from a binary for a device.
	///
	/// The same as #buildSimpleProgramSet, except it takes the ownership of the binary
	/// object: the caller won't be responsible for destroying it. Even if the function
	/// fails, the passed binary object will be released.
	///
	static Program* buildSimpleProgram(Device& device, aiware::common::Binary::Ptr&& binary);

public:
	ProgramSetBuilder() = delete;

	/// Creates a program set builder for the device.
	ProgramSetBuilder(Device& device);

	ProgramSetBuilder(const ProgramSetBuilder&) = delete;
	ProgramSetBuilder(ProgramSetBuilder&& other) noexcept;

	ProgramSetBuilder& operator=(const ProgramSetBuilder&) = delete;
	ProgramSetBuilder& operator=(ProgramSetBuilder&& other) noexcept;

	~ProgramSetBuilder();

	/// Adds programs to the program set by using the passed binary.
	///
	/// Creates #instances number of programs by using the passed binary, which will be
	/// copied during the construction. The new programs will be added to programs set
	/// created by the passed builder.
	///
	/// @param[in] instances		Number of new program instances, can't be 0.
	///
	/// @return		If all parameter is valid the program returns true, otherwise false.
	///
	bool addBinary(const aiware::common::Binary& binary, uint32_t instances = 1);

	/// Adds programs to the program set by using the passed binary.
	///
	/// The same as #addBinary except it takes the ownership of the binary object: the
	/// caller won't be responsible for destroying it.
	///
	bool addBinary(aiware::common::Binary::Ptr&& binary, uint32_t instances = 1);

	/// Finishes building the new program set.
	///
	/// If at least one program has been added to the set, then the builder creates the
	/// program set, and registers it to the device which was associated to the builder at
	/// its creation. The device will be the owner of the returned program set. The
	/// builder object can't be used anymore.
	///
	ProgramSet* finish();

private:
	Device* _device = nullptr;
	void* _impl = nullptr;
};

} // namespace runtime
} // namespace aiware

#endif //AIWARE_RUNTIME_COMMON__PROGRAM_SET_HPP
