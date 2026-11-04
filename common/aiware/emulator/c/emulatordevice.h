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

#ifndef AIWARE_EMULATOR__EMULATOR_DEVICE_H
#define AIWARE_EMULATOR__EMULATOR_DEVICE_H

#include "aiware/emulator/aiware-emulator-lib_export.h"
#include "aiware/runtime/c/device.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/// Creates a new emulator instance by using the given configuration.
	///
	/// If a valid configuration is passed to this function, and there is enough memory in the
	/// system, then this function instantiates and initializes a new aiWare Emulator object,
	/// and returns its pointer. The caller have to destroy the new emulator instance by
	/// calling #aiwDeviceClose function when it's no longer needed.
	///
	/// If there is enough resource in the system, multiple emulator instances can be created.
	///
	/// @param[in] config		Defines the configuration of the emulator. Can't be NULL, and
	///							must be a valid device configuration object.
	///
	/// @return		On success the function returns a non-null pointer to the new emulator
	///				instance. In case of any error, the function returns NULL.
	///
	AIWARE_EMULATOR_LIB_EXPORT aiwDevice* aiwCreateEmulator(const aiwDeviceConfigData* config);

#ifdef __cplusplus
}
#endif

#endif //AIWARE_EMULATOR__EMULATOR_DEVICE_H
