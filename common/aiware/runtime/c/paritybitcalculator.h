#ifndef AIWARE_RUNTIME_COMMON_C__PARITYBIT_CALCULATOR_H
#define AIWARE_RUNTIME_COMMON_C__PARITYBIT_CALCULATOR_H
#include "aiware/common/c/types.h"
#include "aiware/runtime/c/aiware-runtime-common-lib-c_export.h"
#include "aiware/runtime/c/types.h"

#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif

	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT uint16_t aiwCalculateParityBits(const uint8_t* command, size_t size);
#ifdef __cplusplus
}
#endif

#endif
