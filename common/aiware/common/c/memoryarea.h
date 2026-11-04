#ifndef AIWARE_COMMON_C__MEMORY_AREA_H
#define AIWARE_COMMON_C__MEMORY_AREA_H

#include "aiware/common/c/types.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/// Flags which can tell what kind of data type can be stored in a particular.
	/// memory area.
	enum aiwMemoryAreaDataType
	{
		/// The memory area can contain tensor data.
		AIW_MADT_DATA = 0x1,

		/// The memory area can contain weights.
		AIW_MADT_WEIGHTS = 0x2,

		/// The memory area can contain command queues.
		AIW_MADT_CMDQ = 0x4,

		/// All type combined together.
		AIW_MADT_ALL = (uint8_t)AIW_MADT_DATA | (uint8_t)AIW_MADT_WEIGHTS | (uint8_t)AIW_MADT_CMDQ
	};

	/// Contains basic information about a memory area accessible by aiWare.
	struct aiwMemoryAreaImpl
	{
		/// Tells what kinds of data can be stored in this memory area.
		/// If this member is 0 it means the memory area is unused.
		uint8_t dataType;

		/// Start address of the memory area in bytes.
		uint64_t startAddress;

		/// End address of the memory area in bytes.
		uint64_t endAddress;

		// The following members are currently not set and not used by
		// the runtime, but due to compatibility and future proofing
		// reasons left here in this struct. Thy are also excluded from
		// strict DeviceConfigData comparison.

		/// Tells whether the memory area is external (i.e. DDR) or internal
		uint8_t external;

		/// Tells whether the memory area is mapped into the hosts memory
		/// space or it can be accessed just by DMA.
		uint8_t mapped;

		/// Contains the bandwidth of the memory area.
		aiw_f32_t bandwidth;

		/// Contains the latency of the memory area.
		aiw_f32_t latency;
	};

#ifdef __cplusplus
}
#endif

#endif //AIWARE_COMMON_C__MEMORY_AREA_H
