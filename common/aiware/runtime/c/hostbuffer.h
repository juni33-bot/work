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

#ifndef AIWARE_RUNTIME_DEVICE_COMMON_C__HOSTBUFFER_H
#define AIWARE_RUNTIME_DEVICE_COMMON_C__HOSTBUFFER_H
#include <stdlib.h>
#ifdef __cplusplus
extern "C"
{
#endif

	enum HostBufferType
	{
		HBT_INVALID = 0,
		HBT_CONTIGUOUS, // Physically contiguous memory, e.g. hugepage or CMA backed
		HBT_DEVICE, // Device memory mapped into CPU memory space
		HBT_USER, // Userspace memory (e.g. malloc())
		HBT_FD, // DMABUF backed file descriptor
	};

	struct ContiguousMemoryBuffer
	{
		void* virtBase; // virtual memory pointer for CPU access
		void* physBase; // physical address mapped to DMA controller required for DMA transfers
	};

	struct DeviceMemoryBuffer
	{
		void* virtBase; // virtual memory pointer for CPU access, memory mapped
	};

	struct DeviceUserspaceBuffer
	{
		void* virtBase; // virtual memory pointer for CPU access, userspace buffer
		int handle; // driver handle
		size_t offset; // offset to the address imported by the driver
	};

	struct DeviceDMABufBuffer
	{
		int dmaFd; // file descriptor for Linux dmabuf
	};

	struct aiwHostBuffer
	{
		enum HostBufferType type;
		union
		{
			struct ContiguousMemoryBuffer contig;
			struct DeviceMemoryBuffer device;
			struct DeviceUserspaceBuffer user;
			struct DeviceDMABufBuffer dmabuf;
		} d;
		size_t length; // length of the buffer
	};

#define AIW_HOST_BUFFER_REQUIRES_DMA(buffer)                                                                           \
	(((buffer)->type == HBT_CONTIGUOUS) || ((buffer)->type == HBT_USER) || ((buffer)->type == HBT_FD))

#ifdef __cplusplus
}
#endif

#endif
