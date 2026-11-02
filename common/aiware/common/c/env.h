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

#ifndef AIWARE_COMMON_C__ENV_H
#define AIWARE_COMMON_C__ENV_H

#include "aiware/common/c/aiware-common-lib-c_export.h"
#include "aiware/common/c/status.h"
#include "aiware/common/c/types.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/// Initializes the aiWare software environment.
	///
	/// It must be called before any other function is called from the API.
	///
	/// The function sets the default allocator and deallocator, which is platform dependent.
	/// On Linux and Windows it's malloc and free, on other platform some platform specific
	/// allocation method.
	///
	/// The function also sets the default error callback, which is by default fprintf if it's
	/// available.
	///
	/// Finally the function also sets the logger function, which is printf if it's available.
	///
	/// @return		On success, the function returns 0.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiw_status aiwInitEnvironment(void);

	/// Destroys the aiWare software environment.
	///
	/// Must be called last in the application.
	///
	/// @return		On success the function returns 0.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiw_status aiwDestroyEnvironment(void);

	/// Signature of the memory allocator method.
	///
	/// The function take 2 parameters: the amount of memory that should be allocated and a
	/// pointer to custom data, that the function can use anything that it want for.
	/// If the allocation request is feasible, the function should return a non-null pointer
	/// to that memory buffer. If any error happens, the function should return NULL.
	///
	typedef void* (*aiwAllocatorFun)(uint64_t size, void* ctx);

	/// Signature of the memory deallocator method.
	///
	/// The function is used to dealloc dynamically allocated memory allocated by the allocator
	/// function. The function's first parameter is pointer to a previously allocated. It's
	/// guarantied that no other pointer will be passed to this function.
	///
	/// The second parameter is a pointer to custom data, that the function can use anything for.
	///
	/// If the first parameter is NULL, the function should not do anything.
	///
	typedef void (*aiwDeallocatorFun)(void* ptr, void* ctx);

	/// Sets the dynamic memory allocator and deallocator function which will manage all kind of
	/// dynamic memory allocation.
	///
	/// For further information see #aiwAllocatorFun and #aiwDeallocatorFun.
	///
	/// @param[in] allocator			Pointer to the allocator function. Can't be NULL.
	/// @param[in] deallocator			Pointer to the deallocator function. Can't be NULL.
	/// @param[in] allocatorData		Pointer to custom data which will be passed to the
	///									allocator and deallocator as second parameter. Can
	///									be NULL. The ownership of the passed data remains at
	///									the caller.
	/// @return		On success, the function return 0, otherwise a positive value. If the
	///				allocator or deallocator parameters is NULL, then no changes will be made,
	///				and a non-zero error status will be returned.
	///
	AIWARE_COMMON_LIB_C_EXPORT aiw_status
	aiwSetAllocator(aiwAllocatorFun allocator, aiwDeallocatorFun deallocator, void* ctx);

	/// Return the total amount of dynamically allocated memory in bytes.
	///
	/// @return		The amount of dynamically allocated memory in bytes.
	///
	AIWARE_COMMON_LIB_C_EXPORT uint64_t aiwGetTotalAllocatedMemorySize(void);

	/// Signature of the error callback function.
	///
	/// This kind of callback is called when some error happens within the aiWare SDK. Most
	/// of the functions returns an error code or something similar simply value. The more
	/// precise reason of the error can be got by this error callback.
	///
	/// The function's first parameter is a number specific to the module where the error
	/// happened.
	///
	/// The second parameter is more specific code about what happened.
	///
	/// The third parameter is an error message which can be used as the user wants.
	/// The ownership of the passed string remains at the caller.
	///
	/// The fourth parameter is custom data for callback, can be set by the
	/// #aiwSetErrorCallback function.
	///
	typedef void (*aiwErrorCallbackFun)(uint32_t moduleCode, uint32_t errorCode, const char* message, void* ctx);

	/// Sets the error callback function.
	///
	/// For further information see #aiwErrorCallbackFun.
	///
	/// @param[in] errorCallback		Pointer to the new error callback function. If null,
	///									then the callback won't be called, and callback custom
	///									data will be ignored.
	/// @param[in] errorCallbackData	Custom data for the error callback. Can be NULL. The
	///									ownership of the passed object remains at the caller.
	///
	AIWARE_COMMON_LIB_C_EXPORT void aiwSetErrorCallback(aiwErrorCallbackFun errorCallback, void* ctx);

#ifdef __cplusplus
}
#endif

#endif //AIWARE_COMMON_C__ENV_H
