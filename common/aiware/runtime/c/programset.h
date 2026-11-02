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

#ifndef AIWARE_RUNTIME_COMMON_C__PROGRAM_SET_H
#define AIWARE_RUNTIME_COMMON_C__PROGRAM_SET_H

#include "aiware/common/c/binary.h"
#include "aiware/runtime/c/aiware-runtime-common-lib-c_export.h"
#include "aiware/runtime/c/device.h"
#include "aiware/runtime/c/types.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/// Returns the device associated to the passed program set.
	///
	/// @return		If the passed pointer refers to a valid program set instance, the function
	///				returns a non-null pointer. In case of any error, the function return NULL.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwDevice* aiwProgramSetGetDevice(const aiwProgramSet* programSet);

	/// Returns the number of programs in a program set.
	///
	/// @return		Returns a positive integer number if the programSet object is valid,
	///				otherwise 0.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT uint32_t aiwProgramSetGetProgramCount(const aiwProgramSet* programSet);

	/// Returns the Nth program of the program set.
	///
	/// @param[in] programIndex		Index of the requested program. Must be less than the value
	///								returned by #aiwProgramSetGetProgramCount.
	///
	/// @return		If both parameters are valid, then function returns the pointer of the
	///				requested program instance. The ownership of the returned object remains at
	///				the program set, the caller doesn't have to destroy it. If any parameter is
	///				invalid, the function returns NULL.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwProgram* aiwProgramSetGetProgram(
		const aiwProgramSet* programSet,
		uint32_t programIndex);

	/// Destroys the passed program set.
	///
	/// All program sets belong to a device, and is destroyed when the associated device is
	/// destroyed. However a program set can be destroyed before that by using this function.
	/// The call of this function will unregister the program set from its device, so it can
	/// modify the indices of other program sets that belong to the same device (if there is
	/// any).
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status aiwReleaseProgramSet(aiwProgramSet* programSet);

	/// Creates a simple program set from a binary for a device.
	///
	/// The passed binary instance will be copied, an one program instance will be built from it
	/// for the passed device. The binary must be built for the device. The new built program
	/// set will be registered to the device and it will be the owner, so the caller doesn't
	/// have to release it.
	///
	/// @return		If the passed device and binary is valid and if they are compatible, then
	///				the function returns the pointer of the set's only program.
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwProgram* aiwBuildSimpleProgramByCopy(
		aiwDevice* device,
		const aiwBinary* binary);

	/// Creates a simple program set from a binary for a device.
	///
	/// The same as #aiwBuildSimpleProgramByCopy, except it takes the ownership of the binary
	/// object: the caller won't be responsible for destroying it. Even if the function fails,
	/// the passed binary object will be released.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwProgram* aiwBuildSimpleProgramByMove(aiwDevice* device, aiwBinary* binary);

	/// Creates a program set builder for the device.
	///
	/// The program set builder can create multiple programs from binaries and pack them into
	/// a single program set.
	///
	/// @return		If the passed device is valid, the function returns a new builder object.
	///				The caller is responsible for destroying the created object. In case of any
	///				error the function returns NULL.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwProgramSetBuilder* aiwCreateProgramSetBuilder(aiwDevice* device);

	/// Adds programs to the program set by using the passed binary.
	///
	/// Creates \p instances number of programs by using the passed binary, which will be
	/// copied during the construction. The new programs will be added to programs set created
	/// by the passed builder.
	///
	/// @param[in] instances		Number of new program instances, can't be 0.
	///
	/// @return		If all parameter is valid the program returns AIW_SUCCESS, otherwise
	///				AIW_ERROR.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status
	aiwProgramSetBuilderAddBinaryCopy(aiwProgramSetBuilder* builder, const aiwBinary* binary, uint8_t instances);

	/// Adds programs to the program set by using the passed binary.
	///
	/// The same as #aiwProgramSetBuilderAddBinaryCopy except it takes the ownership of the binary
	/// object: the caller won't be responsible for destroying it.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status
	aiwProgramSetBuilderAddBinaryMove(aiwProgramSetBuilder* builder, aiwBinary* binary, uint8_t instances);

	/// Finishes building the new program set.
	///
	/// If the builder is valid, and at least one program has been added to the set, then the
	/// builder creates the program set, and registers it to the device which was associated to the
	/// builder at its creation. The device will be the owner of the returned program set. The
	/// builder object can't be used anymore, and should be destroyed by calling
	/// #aiwReleaseProgramSetBuilder.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwProgramSet* aiwProgramSetBuilderFinish(aiwProgramSetBuilder* builder);

	/// Destroys a program set builder instance.
	///
	/// If the building process hasn't been completed yet, the function will destroy every
	///	unfinished objects.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status aiwReleaseProgramSetBuilder(aiwProgramSetBuilder* builder);

#ifdef __cplusplus
}
#endif

#endif //AIWARE_RUNTIME_COMMON_C__PROGRAM_SET_H
