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

#ifndef AIWARE_RUNTIME_DEVICE_COMMON_C__RUNTIMEDEVICE_H
#define AIWARE_RUNTIME_DEVICE_COMMON_C__RUNTIMEDEVICE_H
#include "aiware/common/c/memoryarea.h"
#include "aiware/common/c/status.h"
#include "aiware/common/c/types.h"
#include "aiware/runtime/c/aiware-runtime-device-lib-c_export.h"
#include "aiware/runtime/c/hostbuffer.h"

#include <stdbool.h>
#ifdef __cplusplus
extern "C"
{
#endif
	struct aiwRuntimeDevice;
	enum aiwRuntimeDeviceLockType
	{
		LOCK_HARDWARE = 0, // exclusive hardware access is required
		LOCK_MEMORY = 1, // exclusive memory access is required
		LOCK_PROGRAM = 2 // exclusive command queue manipulation is required
	};

	enum aiwRuntimeInterruptSource
	{
		CMDQ_FINISHED = 0,
		CHECKPOINT = 1,
		BLOCK_FINISHED = 2,
		INTERNAL_ERROR = 3,
		ILLEGAL_CONTROL = 4,
		ACC_OVERFLOW = 5,
		MAX_INTERRUPTS = 6
	};

	static const uint32_t AIW_INTERRUPT_MASK = (1u << MAX_INTERRUPTS) - 1u;

	enum aiwRuntimeExtendedRegisterType
	{
		/* aiWare IP register area: 0-15 reserved */
		REG_AIWARE = 0, /* Standard register set from HSI */
		REG_DRL = 1, /* Data rate limiter*/
		REG_AIWARE_LAST = 15,
		/* SoC specific register area */
		REG_SOC_FIRST = REG_AIWARE_LAST + 1,
		REG_SOC_LAST = REG_SOC_FIRST + 15,
		/* Board specific register area */
		REG_BOARD_FIRST = REG_SOC_LAST + 1,
		REG_BOARD_LAST = REG_BOARD_FIRST + 15,
		/* User specific register area */
		REG_USER_FIRST = REG_BOARD_LAST + 1
	};

	struct aiwRuntimeDeviceMemoryBank
	{
		uintptr_t physAddr; /* Device physical address for the memory bank */
		size_t length; /* Size of memory bank */
		uint32_t type; /* What can be put into the memory bank, combination of aiwRuntimeDeviceMemoryBankType */

#ifdef __cplusplus
		aiwRuntimeDeviceMemoryBank(uintptr_t physAddr0, size_t length0, uint32_t type0)
			: physAddr(physAddr0)
			, length(length0)
			, type(type0)
		{
		}
#endif
	};

	struct aiwDeviceBuffer
	{
		uintptr_t physAddr; /* Device physical address for the memory area */
		size_t length; /* Size of memory area */

#ifdef __cplusplus
		aiwDeviceBuffer(uintptr_t physAddr0, size_t length0) : physAddr(physAddr0), length(length0) {}
#endif
	};

	enum aiwRuntimeDeviceFeatures
	{
		/* Memory access features */
		FEATURE_CONTIG_DMA = 0x1,
		FEATURE_USER_DMA = 0x2,
		FEATURE_DMABUF_DMA = 0x4,
		FEATURE_DIRECT_MAPPING = 0x8,
		FEATURE_EXTERNAL_EXPORT = 0x10,
		/* RESERVED*/

		/* Interrupt handling features */
		FEATURE_EVENTFD_IRQ = 0x20,
		/* RESERVED */
		/* Device locking for user */
		FEATURE_USER_LOCK = 0x100,
		/* Additional features */
		FEATURE_EXTENDED = 0x1000,
		FEATURE_DRL = 0x1000, /* Data Rate Limiter */
	};

#define AIW_RUNTIME_DEVICE_DEFAULT_IMPLEMENTATION (NULL)
	struct aiwRuntimeDeviceFunctions
	{
		/* Lifetime management */
		aiw_status (*release)(struct aiwRuntimeDevice* device);
		/* Device information */
		aiw_status (*getDeviceFeatures)(struct aiwRuntimeDevice* device, uint32_t* featuresResult);
		aiw_status (*getMemoryBankCount)(struct aiwRuntimeDevice* device, uint8_t* count);
		aiw_status (*getMemoryBankInformation)(
			struct aiwRuntimeDevice* device,
			uint8_t index,
			struct aiwRuntimeDeviceMemoryBank* bankResult);

		/* Clock handling */
		aiw_status (*getClockFrequency)(struct aiwRuntimeDevice* device, uint64_t* freq);
		aiw_status (*setClockFrequency)(struct aiwRuntimeDevice* device, uint64_t freq);

		/* Reset handling */
		aiw_status (*resetDevice)(struct aiwRuntimeDevice* device);

		/* Register access */
		aiw_status (*writeRegister)(struct aiwRuntimeDevice* device, uint64_t reg, uint64_t value);
		aiw_status (*readRegister)(struct aiwRuntimeDevice* device, uint64_t reg, uint64_t* value);

		aiw_status (*writeRegisters)(
			struct aiwRuntimeDevice* device,
			uint64_t regStart,
			const void* value,
			size_t regSize,
			size_t regCount);
		aiw_status (*readRegisters)(
			struct aiwRuntimeDevice* device,
			uint64_t regStart,
			void* value,
			size_t regSize,
			size_t regCount);

		/* Extended register access - DRL, etc. */
		aiw_status (*writeExtendedRegister)(
			struct aiwRuntimeDevice* device,
			enum aiwRuntimeExtendedRegisterType type,
			uint64_t reg,
			uint64_t value);
		aiw_status (*readExtendedRegister)(
			struct aiwRuntimeDevice* device,
			enum aiwRuntimeExtendedRegisterType type,
			uint64_t reg,
			uint64_t* value);

		/* Interrupt handling */
		aiw_status (*pollInterrupts)(
			struct aiwRuntimeDevice* device,
			uint64_t* mask,
			uint32_t timeoutMs); /* read the status for every interrupt line aiWare has */
		aiw_status (*readInterruptCount)(
			struct aiwRuntimeDevice* device,
			enum aiwRuntimeInterruptSource irq,
			uint64_t* count); /* read the interrupt count for an interrupt line since the last read */
		aiw_status (
			*waitInterrupts)(struct aiwRuntimeDevice* device, uint64_t* mask); /* block until interrupt happens */

		/* Support for OS-dependent interrupt waiting, advanced API */
		aiw_status (*getInterruptEventSource)(
			struct aiwRuntimeDevice* device,
			enum aiwRuntimeInterruptSource irq,
			void* sourceData,
			size_t* sourceDataSize);
		aiw_status (*handleInterruptEventSource)(
			struct aiwRuntimeDevice* device,
			enum aiwRuntimeInterruptSource irq,
			void* sourceData,
			size_t* sourceDataSize);

		/* Memory handling */
		aiw_status (*createContigBuffer)(struct aiwRuntimeDevice* device, struct aiwHostBuffer* buffer);
		aiw_status (*destroyContigBuffer)(struct aiwRuntimeDevice* device, struct aiwHostBuffer* buffer);

		// Buffer preparation: before first use a host buffer is needed to be prepared, and after last use it is needed to be released

		aiw_status (*prepareBuffer)(struct aiwRuntimeDevice* device, struct aiwHostBuffer* buffer);
		aiw_status (*unprepareBuffer)(struct aiwRuntimeDevice* device, struct aiwHostBuffer* buffer);

		// DMA operation
		aiw_status (*uploadBuffer)(
			struct aiwRuntimeDevice* device,
			struct aiwHostBuffer* source,
			size_t sourceOffset,
			struct aiwDeviceBuffer* destination,
			size_t destinationOffset,
			size_t length,
			uint32_t timeoutMs);
		aiw_status (*downloadBuffer)(
			struct aiwRuntimeDevice* device,
			struct aiwDeviceBuffer* source,
			size_t sourceOffset,
			struct aiwHostBuffer* destination,
			size_t destinationOffset,
			size_t length,
			uint32_t timeoutMs);

		// Device locking
		aiw_status (
			*lockDevice)(struct aiwRuntimeDevice* device, enum aiwRuntimeDeviceLockType type, uint32_t timeoutMs);
		aiw_status (*unlockDevice)(struct aiwRuntimeDevice* device, enum aiwRuntimeDeviceLockType type);

		// Device information
		aiw_status (*description)(struct aiwRuntimeDevice* device, char* retStr, size_t maxLength);

		// User locking
		aiw_status (*userLock)(struct aiwRuntimeDevice* device);
		aiw_status (*userUnlock)(struct aiwRuntimeDevice* device);
		aiw_status (*userLocked)(struct aiwRuntimeDevice* device);

		// Export physical memory to shared buffer
		aiw_status (*exportMemory)(
			struct aiwRuntimeDevice* device,
			uint8_t memoryBankIndex,
			uintptr_t memoryOffset,
			size_t length,
			void** externalMemory);
		aiw_status (*exportRelease)(struct aiwRuntimeDevice* device, void* externalMemory);

		aiw_status (
			*getDriverVersion)(struct aiwRuntimeDevice* device, uint32_t* major, uint32_t* minor, uint32_t* patch);
	};

	struct aiwRuntimeDevice
	{
		const struct aiwRuntimeDeviceFunctions* operations;
		uint32_t defaultMemoryTransferTimeoutMs; // Initial suggested timeout for memory transfer.
			// Emulator and remote can set this to a large enough value
			// to have a sane default.
		uint32_t defaultCommandQueueExecutionTimeoutMs; // Initial suggested timeout for command queue execution.
			// Emulator and remote can set this to a large enough value
			// to have a sane default.

		uint64_t quirks; // Device specific quirks which needed to be handled in runtime. It is a mask of aiwQuirkMask

		void* priv; // Private data for runtime device, platform specific
	};

	/* These functions should be provided by the runtime device port */
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT uint32_t aiwRuntimeDeviceCount(void);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDeviceOpen(struct aiwRuntimeDevice* device, uint32_t deviceIndex);

	/* These convenience functions are exported by this library */
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT void* aiwHostBufferGetVirt(struct aiwHostBuffer* hostBuffer);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceRelease(struct aiwRuntimeDevice* device);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDeviceGetDeviceFeatures(struct aiwRuntimeDevice* device, uint32_t* featuresResult);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDeviceGetMemoryBankCount(struct aiwRuntimeDevice* device, uint8_t* count);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceGetMemoryBankInformation(
		struct aiwRuntimeDevice* device,
		uint8_t index,
		struct aiwRuntimeDeviceMemoryBank* bankResult);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDeviceGetClockFrequency(struct aiwRuntimeDevice* device, uint64_t* freq);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDeviceSetClockFrequency(struct aiwRuntimeDevice* device, uint64_t freq);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceResetDevice(struct aiwRuntimeDevice* device);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDeviceWriteRegister(struct aiwRuntimeDevice* device, uint64_t reg, uint64_t value);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDeviceReadRegister(struct aiwRuntimeDevice* device, uint64_t reg, uint64_t* value);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceWriteRegisters(
		struct aiwRuntimeDevice* device,
		uint64_t regStart,
		const void* value,
		size_t regSize,
		size_t regCount);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceReadRegisters(
		struct aiwRuntimeDevice* device,
		uint64_t regStart,
		void* value,
		size_t regSize,
		size_t regCount);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceWriteExtendedRegister(
		struct aiwRuntimeDevice* device,
		enum aiwRuntimeExtendedRegisterType type,
		uint64_t reg,
		uint64_t value);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceReadExtendedRegister(
		struct aiwRuntimeDevice* device,
		enum aiwRuntimeExtendedRegisterType type,
		uint64_t reg,
		uint64_t* value);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDevicePollInterrupts(
		struct aiwRuntimeDevice* device,
		uint64_t* mask,
		uint32_t timeoutMs); /* read the status for every interrupt line aiWare has */
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceReadInterruptCount(
		struct aiwRuntimeDevice* device,
		uint8_t irq,
		uint64_t* count); /* read the interrupt count for an interrupt line since the last read */
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDeviceWaitInterrupts(struct aiwRuntimeDevice* device, uint64_t* mask); /* block until interrupt happens */
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceGetInterruptEventSource(
		struct aiwRuntimeDevice* device,
		uint8_t irq,
		void* sourceData,
		size_t* sourceDataSize);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceHandleInterruptEventSource(
		struct aiwRuntimeDevice* device,
		uint8_t irq,
		void* sourceData,
		size_t* sourceDataSize);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDeviceCreateContigBuffer(struct aiwRuntimeDevice* device, struct aiwHostBuffer* buffer);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDeviceDestroyContigBuffer(struct aiwRuntimeDevice* device, struct aiwHostBuffer* buffer);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDevicePrepareBuffer(struct aiwRuntimeDevice* device, struct aiwHostBuffer* buffer);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDeviceUnprepareBuffer(struct aiwRuntimeDevice* device, struct aiwHostBuffer* buffer);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceUploadBuffer(
		struct aiwRuntimeDevice* device,
		struct aiwHostBuffer* source,
		size_t sourceOffset,
		struct aiwDeviceBuffer* destination,
		size_t destinationOffset,
		size_t length,
		uint32_t timeoutMs);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceDownloadBuffer(
		struct aiwRuntimeDevice* device,
		struct aiwDeviceBuffer* source,
		size_t sourceOffset,
		struct aiwHostBuffer* destination,
		size_t destinationOffset,
		size_t length,
		uint32_t timeoutMs);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDeviceLockDevice(struct aiwRuntimeDevice* device, enum aiwRuntimeDeviceLockType type, uint32_t timeoutMs);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDeviceUnlockDevice(struct aiwRuntimeDevice* device, enum aiwRuntimeDeviceLockType type);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDeviceDescription(struct aiwRuntimeDevice* device, char* retStr, size_t maxLength);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceUserLock(struct aiwRuntimeDevice* device);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceUserUnlock(struct aiwRuntimeDevice* device);
	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceUserLocked(struct aiwRuntimeDevice* device);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceExportMemory(
		struct aiwRuntimeDevice* device,
		uint8_t memoryBankIndex,
		uintptr_t memoryOffset,
		size_t length,
		void** externalMemory);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status
	aiwRuntimeDeviceExportRelease(struct aiwRuntimeDevice* device, void* externalMemory);

	AIWARE_RUNTIME_DEVICE_LIB_C_EXPORT aiw_status aiwRuntimeDeviceGetDriverVersion(
		struct aiwRuntimeDevice* device,
		uint32_t* major,
		uint32_t* minor,
		uint32_t* patch);

#ifdef __cplusplus
}
#endif

#endif
