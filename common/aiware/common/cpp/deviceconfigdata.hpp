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

#ifndef AIWARE_COMMON_CPP__DEVICE_CONFIG_DATA_HPP
#define AIWARE_COMMON_CPP__DEVICE_CONFIG_DATA_HPP

#include "aiware/common/c/deviceconfigdata.h"
#include "aiware/common/cpp/aiware-common-lib-cpp_export.h"

#include <memory>
#include <stdio.h>
#include <vector>

namespace aiware
{
namespace common
{

/// Contains all information of an aiWare device.
///
/// This class can be queried from a device, can be saved and later can be loaded. In the
/// last case if any error happened, and invalid instance is returned. The functions return
/// dummy or error values if they are called on an invalid instance.
///
class AIWARE_COMMON_LIB_CPP_EXPORT DeviceConfigData
{
public:
	using Ptr = std::unique_ptr<DeviceConfigData>;

	/// Creates a new instance of device config data by loading a serialized device config
	/// data stored in a file.
	///
	/// @param[in] path		Path of the file to load. Can't be null and must point to an
	///						existing file with a serialized device config data content.
	///
	/// @return		If the file exists and valid, it will be parsed, and a new device
	///				config data instance will be returned. Otherwise and invalid object
	///				will be returned.
	///
	static DeviceConfigData::Ptr load(const char* path);

	/// Creates a new instance of device config data by loading a serialized device config
	/// data stored in a file.
	///
	/// @param[in] file		Opened file whose content will be used to create the new device
	///						config instance. Can't be NULL, and must be read-capable.
	///
	/// @return		If the file exists and valid, it will be parsed, and a new device
	///				config data instance will be returned. Otherwise and invalid object
	///				will be returned.
	///
	static DeviceConfigData::Ptr load(FILE* file);

	/// Creates a new instance of device config data by deserializing data of the passed
	/// buffer.
	///
	/// @param[in] buffer	Pointer to serialized data buffer. Can't be null.
	/// @param[in] bufferSize	Size of the data buffer. Can't be 0.
	///
	/// @return		If the buffer contains valid data it will be parsed and a new instance
	///				will be returned. Otherwise and invalid object will be returned.
	///
	static DeviceConfigData::Ptr load(const uint8_t* buffer, uint32_t bufferSize);

public:
	DeviceConfigData(const DeviceConfigData& other) = delete;
	DeviceConfigData(DeviceConfigData&& other) = delete;
	DeviceConfigData& operator=(DeviceConfigData&& other) noexcept = delete;

	DeviceConfigData& operator=(const DeviceConfigData& other);

	/// Destroys the instance.
	virtual ~DeviceConfigData();

	/// Creates a new instance by cloning the current one.
	Ptr clone() const;

	/// Same as #strictCompare.
	bool operator==(const DeviceConfigData& other) const;

	/// Compares the current instance to another one in the strict way and returns true
	/// if they are different.
	bool operator!=(const DeviceConfigData& other) const;

	/// Compares the current instance to another one in the strict way and returns true
	/// if they are equal.
	bool strictCompare(const DeviceConfigData& other) const;

	/// Compares the current instance to another one in the loose way and returns true if
	/// they are equal.
	bool relaxedCompare(const DeviceConfigData& other) const;

	/// Returns the number of LAMs can be found in the current configuration.
	/// If the object is invalid it returns 0.
	uint32_t lamCount() const;

	/// Returns the number of memory areas can be found in the current configuration.
	/// If the object is invalid it returns 0.
	uint32_t memoryAreaCount() const;

	/// Returns the Nth memory area descriptor object can be found in the current
	/// configuration.
	///
	/// @param[in] index	Index of the requested memory area. Must be larger than the
	///						value returned by #memoryAreaCount().
	///
	/// @return		If the object and the passed index is valid, then returns a weak
	///				pointer to the proper memory area object.
	///
	const aiwMemoryArea* memoryArea(uint32_t index) const;

	/// Serializes the current instance into a file.
	///
	/// The object can be saved only if it's valid.
	///
	/// @param[in] path		Path of the destination file. Can't be null.
	///
	/// @return		Returns the status of the serialization.
	///
	bool save(const char* path) const;

	/// Serializes the current instance into a file.
	///
	/// The object can be saved only if it's valid.
	///
	/// @param[in] file		Pointer to the opened file, where the config will be saved.
	///						Can't be saved.
	///
	/// @return		Returns the status of the serialization.
	///
	bool save(FILE* file) const;

	/// Serializes the current instance into a buffer.
	///
	/// The object can be saved only if it's valid.
	///
	/// @return		Returns a non-empty vector if the serialization was successful,
	///				otherwise it's returns an empty one.
	///
	std::vector<uint8_t> save() const;

protected:
	/// Constructs an object by using the passed raw data.
	///
	/// @param[in] data		Pointer to the raw data. If null, the object will be invalid.
	/// @param[in] owned	Tells whether the new instance takes the ownership of the passed
	///						object or doesn't.
	DeviceConfigData(void* data, bool owned);

protected:
	/// Pointer to the raw data. If null, the instance is invalid.
	void* _data = nullptr;

	/// Tells whether the instance owns the raw data or doesn't.
	bool _owned = false;
};

} // namespace common
} // namespace aiware

#endif //AIWARE_COMMON_CPP__DEVICE_CONFIG_DATA_HPP
