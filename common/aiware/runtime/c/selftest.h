/*
** Copyright (c) AImotive Kft. 2024
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

#ifndef AIWARE_RUNTIME_COMMON_C__SELFTEST_H
#define AIWARE_RUNTIME_COMMON_C__SELFTEST_H

#include "aiware/runtime/c/aiware-runtime-common-lib-c_export.h"
#include "aiware/runtime/c/device.h"
#include "aiware/runtime/c/program.h"
#include "aiware/runtime/c/types.h"

#ifdef __cplusplus
extern "C"
{
#endif
	/// @brief The selftest execution policy
	///
	/// The selftest execution policy defines how the selftest program should be executed.
	///
	/// AIW_SEP_NO_EXEC (default): The selftest program is not loaded, never executed.
	/// AIW_SEP_MANUAL: The selftest program is loaded, but not executed automatically, #aiwSelftestExecute must be called by user.
	/// AIW_AIW_SEP_BEFORE_PROGRAM: The selftest program is loaded at the call of #aiwSetSelftestExecutionPolicy and executed before every program execution (except selftest program).
	///
	/// Selftest program is loaded when #aiwSetSelftestExecutionPolicy is called with AIW_SEP_NO_EXEC!=policy first time.
	typedef enum
	{
		AIW_SEP_NO_EXEC = -1,
		AIW_SEP_MANUAL = 0,
		AIW_SEP_BEFORE_PROGRAM,
	} aiwSelftestExecutionPolicy;

	/// Run the selftest
	/// @param[in] device The device to run the selftest on
	/// @return AIW_SUCCESS on success, AIW_ERROR otherwise
	///
	/// This function immediately runs the selftest program on the device. The selftest program
	/// should be already loaded by calling #aiwSetSelftestExecutionPolicy with AIW_SEP_NO_EXEC!=policy.
	///

	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status aiwSelftestExecute(aiwDevice* device);

	/// Get the selftest execution policy
	/// @param[in] device The device to get the selftest execution policy from
	/// @param[out] policy The policy to use for selftest execution
	/// @return AIW_SUCCESS on success, AIW_ERROR otherwise
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status
	aiwGetSelftestExecutionPolicy(aiwDevice* device, aiwSelftestExecutionPolicy* policy);

	/// Set the selftest execution policy
	/// @param[in] device The device to set the selftest execution policy on
	/// @param[in] policy The policy to use for selftest execution
	/// @return AIW_SUCCESS on success, AIW_ERROR otherwise
	///
	/// This function sets the selftest execution policy for the device. On the first call with
	/// AIW_SEP_NO_EXEC!=policy, the selftest program for the device is found and loaded.
	/// The selftest program is freed when the device is closed by #aiwDeviceClose.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status
	aiwSetSelftestExecutionPolicy(aiwDevice* device, aiwSelftestExecutionPolicy policy);

#ifdef __cplusplus
}
#endif

#endif //AIWARE_RUNTIME_COMMON_C__SELFTEST_H
