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

#ifndef AIWARE_RUNTIME__CONFIGS_H
#define AIWARE_RUNTIME__CONFIGS_H

#include "aiware/common/c/deviceconfigdata.h"
#include "aiware/runtime/c/aiware-runtime-common-lib-c_export.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/// Creates the default Apache5 MP config.
	///
	/// Convenience method, calls #aiwCreateApache5Config with 800 MHz. For further info see
	/// the previously mentioned function.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwDeviceConfigData* aiwCreateApache5ConfigDefault(void);

	/// Creates Apache5 MP2 config and sets its frequency to the given value.
	///
	/// @param[in] frequencyMHz		Frequency of the configuration given in MHz. Must be
	///								greater than 0.
	///
	/// @return		On success a new device config data instance is created by using the given
	///				parameters. The returned instance must be deleted by the caller when it's
	///				no longer used. In case of any error the function returns NULL.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwDeviceConfigData* aiwCreateApache5Config(uint32_t frequencyMHz);

	/// Creates the default Apache6 config.
	///
	/// Convenience method, calls #aiwCreateApache6Config with 1300 MHz. For further info see
	/// the previously mentioned function.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwDeviceConfigData* aiwCreateApache6ConfigDefault(void);

	/// Creates Apache6 config and sets its frequency to the given value.
	///
	/// @param[in] frequencyMHz		Frequency of the configuration given in MHz. Must be
	///								greater than 0.
	///
	/// @return		On success a new device config data instance is created by using the given
	///				parameters. The returned instance must be deleted by the caller when it's
	///				no longer used. In case of any error the function returns NULL.

	// TODO remove this
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwDeviceConfigData* aiwCreateApache6Config(uint32_t frequencyMHz);

	/// Creates the default Apache6 ES config.
	///
	/// Convenience method, calls #aiwCreateApache6ESConfig with 1300 MHz. For further info see
	/// the previously mentioned function.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwDeviceConfigData* aiwCreateApache6ESConfigDefault(void);

	/// Creates Apache6 ES config and sets its frequency to the given value.
	///
	/// @param[in] frequencyMHz		Frequency of the configuration given in MHz. Must be
	///								greater than 0.
	///
	/// @return		On success a new device config data instance is created by using the given
	///				parameters. The returned instance must be deleted by the caller when it's
	///				no longer used. In case of any error the function returns NULL.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwDeviceConfigData* aiwCreateApache6ESConfig(uint32_t frequencyMHz);

#ifdef __cplusplus
}
#endif

#endif //AIWARE_RUNTIME__CONFIGS_H
