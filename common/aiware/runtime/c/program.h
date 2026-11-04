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

#ifndef AIWARE_RUNTIME_COMMON_C__PROGRAM_H
#define AIWARE_RUNTIME_COMMON_C__PROGRAM_H

#include "aiware/common/c/binary.h"
#include "aiware/runtime/c/aiware-runtime-common-lib-c_export.h"
#include "aiware/runtime/c/device.h"
#include "aiware/runtime/c/types.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/// Returns info of the passed tensor.
	///
	/// @param[in] tensor		Tensor to query. Can't be NULL.
	///
	/// @return		When a valid tensor instance is passed the function returns its info. The
	///				returned object must not be released. If the passed tensor is NULL or
	///				invalid, the function returns NULL.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT const aiwTensorInfo* aiwTensorGetInfo(const aiwTensor* tensor);

	/// Returns the size in bytes of the raw buffer that belongs to the tensor.
	///
	/// For further info please see #aiwTensorRawBufferPointer.
	///
	/// @param[in] tensor		Tensor to query. Can't be NULL.
	///
	/// @return		When a valid object tensor instance is passed the function returns the size,
	///				which is a positive integer. Otherwise the function returns 0.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT uint64_t aiwTensorRawBufferSize(const aiwTensor* tensor);

	/// Returns the ordering of the raw buffer that belongs to the passed tensor.
	///
	/// Returns the same value as the program and device the tensor belongs to.
	///
	/// @return		Returns AIW_RTBO_INVALID if the passed tensor is null or invalid. Otherwise returns
	///				one of the valid items from the #aiwRawTensorBufferOrdering enum.
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwRawTensorBufferOrdering aiwTensorRawBufferOrdering(const aiwTensor* tensor);

	/// Acquires and returns the pointer to the raw buffer that belongs to the tensor.
	///
	/// Through the returned pointer the caller can get direct access to data that is assigned
	/// to a specific tensor. The data is stored in the host memory in special, aiWare related ordering.
	/// The ordering can be queried by #aiwTensowRawBufferOrdering function.
	///
	/// To give direct access to internal buffers, internal structures must be locked. After
	/// the buffer's content was accessed, it must be released by calling
	/// #aiwTensorAcquireRawBufferPointer, which releases the internal locks. No buffer can
	/// be locked during the execution of the program whom the tensor belongs to. It's also not
	/// sure the function will return the same pointer for the same tensor if it's called twice.
	/// So the returned pointer should not stored in the application.
	///
	/// Each tensor has a counterpart in the device's memory. When one modifies the data of
	/// the tensor via this pointer, it only affects the copy being in the host's memory. To
	/// modify the pair of the tensor in the device's memory the tensor must be uploaded into
	/// it by calling #aiwProgramUploadInputs. Similarly, the result of a program's execution doesn't
	/// immediately changes the data in the host memory, it has to be downloaded first.
	///
	/// @param[in] tensor		Tensor to query. Can't be NULL.
	///
	/// @return		When a valid object tensor instance is passed the function returns a non-NULL
	///				pointer that points to the raw buffer. Otherwise then function returns NULL.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT uint8_t* aiwTensorAcquireRawBufferPointer(aiwTensor* tensor);

	/// Releases a previously acquired internal buffer pointer.
	///
	/// For further info please see #aiwTensorAcquireRawdBufferPointer
	///
	/// @return		If the function gets a valid tensor object, and it has been acquired, then
	///				the function releases it and returns AIW_SUCCESS. In any other case the
	///				function return AIW_ERROR.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status aiwTensorReleaseRawBufferPointer(aiwTensor* tensor);

	/// Returns the tensor's export object.
	///
	/// The export object contains the tensor's underlying memory area in an structure that can be used by other
	/// hardware accelerators or libraries.
	/// This routine is platform dependent. The ownership of the returned object remains at the tensor,
	/// which is managed by the program. If needed, the export object may be released manually by calling #aiwTensorExportRelease.
	///
	/// @param[in] tensor		Tensor to query. Can't be NULL.
	/// @param[out] tensorExport	Pointer to hold the export object of the tensor.
	///
	/// @return		When a valid tensor instance is passed the function returns the export object in tensorExport and AIW_SUCCESS
	///				Otherwise the function returns AIW_ERROR.

	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status
	aiwTensorExport(aiwTensor* tensor, aiwExternalMemoryDescriptor** tensorExport);

	/// Releases the tensor's export object.
	///
	/// For further info please see #aiwTensorExport.
	///
	/// @param[in] tensor		Tensor used to create the export.
	/// @param[in] tensorExport		Export object to release.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status
	aiwTensorExportRelease(aiwTensor* tensor, aiwExternalMemoryDescriptor* tensorExport);

	/// Returns the NCHW size of the passed tensor.
	///
	/// The returned value is equal to the multiplication of all dimensions of the tensor.
	///
	/// @param[in] tensor		Tensor to query. Can't be NULL.
	///
	/// @return		When a valid tensor instance is passed the function returns the size, which
	///				is a positive integer. If the passed tensor is invalid, the function returns
	///				0.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT uint32_t aiwTensorSizeNCHW(const aiwTensor* tensor);

	/// Sets the tensor's data by using uint8 NCHW ordered data.
	///
	/// The passed buffer's size must be at least as large as the size returned by
	/// #aiwTensorSizeNCHW. The input data in NCHW representation will be reordered to the
	/// proper internal format. The reordering performed by the host's CPU, so it can be an overhead in
	/// the execution.
	///
	/// @param[in] tensor		Tensor whose data will be modified. Can't be NULL.
	/// @param[in] data			Source data buffer. Can't be NULL and the buffer size must be
	///							at least #aiwTensorSizeNCHW bytes large.
	///
	/// @return		On success it returns AIW_SUCCESS, otherwise return AIW_ERROR.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status
	aiwTensorSetDataNCHWUInt8(const aiwTensor* tensor, const uint8_t* data);

	/// Sets the tensor's data by using float NCHW ordered data.
	///
	/// The number of elements of the passed buffer must be at least as large as the size
	/// returned by #aiwTensorSizeNCHW. The input data in NCHW representation will be quantized
	/// and reordered to the proper internal format. The reordering performed by the host's CPU, so it
	/// can be an overhead in the execution.
	///
	/// @param[in] tensor		Tensor whose data will be modified. Can't be NULL.
	/// @param[in] data			Source data buffer. Can't be NULL and the buffer size must be
	///							at least #aiwTensorSizeNCHW bytes large.
	///
	/// @return		On success it returns AIW_SUCCESS, otherwise return AIW_ERROR.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status
	aiwTensorSetDataNCHWFloat(const aiwTensor* tensor, const aiw_f32_t* data);

	/// Gets the tensor's data and writes it into the passed buffer in NCHW order.
	///
	/// For further information please see #aiwTensorSetDataNCHWUInt8.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status aiwTensorGetDataNCHWUInt8(const aiwTensor* tensor, uint8_t* data);

	/// Gets the tensor's data, dequantize it to float and writes it into the passed buffer in
	/// NCHW order.
	///
	/// For further information please see #aiwTensorSetDataNCHWFloat.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status aiwTensorGetDataNCHWFloat(const aiwTensor* tensor, aiw_f32_t* data);

	/// Returns the device associated to the passed program.
	///
	/// The program will be executed on this device.
	///
	/// @return		If the passed program is valid, then returns a non-null pointer to its
	///				devices. Otherwise the function returns NULL.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwDevice* aiwProgramGetDevice(const aiwProgram* program);

	/// Returns the program set where the program belongs to.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwProgramSet* aiwProgramGetProgramSet(const aiwProgram* program);

	/// Returns the binary that was used to build to the program.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT const aiwBinary* aiwProgramGetBinary(const aiwProgram* program);

	/// Returns the number of input tensors belongs to the passed program.
	///
	/// @return		If the passed pointer is not null and a valid program instance, then the
	///				function returns a positive integer. In case of any error, the function
	///				returns NULL.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT uint32_t aiwProgramGetInputTensorCount(const aiwProgram* program);

	/// Returns the Nth input tensor of the program.
	///
	/// @param[in] inputTensorIndex		Index of the input tensor, must be less than the value
	///									returned by #aiwProgramGetInputTensorCount.
	///
	/// @return		If the passed program and the inputTensorIndex are both valid, then the
	///				function returns a non-null pointer to the tensor object. The ownership of
	///				the returned object remains at the program, doesn't need deleting. If any
	///				parameter of the function is invalid, the function returns NULL.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT const aiwTensor* aiwProgramGetInputTensorConst(
		const aiwProgram* program,
		uint32_t inputTensorIndex);

	/// Returns the Nth input tensor of the program, non-const version.
	///
	/// For further info please check #aiwProgramGetInputTensorConst.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwTensor* aiwProgramGetInputTensor(
		const aiwProgram* program,
		uint32_t inputTensorIndex);

	/// Returns the number of output tensors belongs to the passed program.
	///
	/// @return		If the passed pointer is not null and a valid program instance, then the
	///				function returns a positive integer. In case of any error, the function
	///				returns NULL.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT uint32_t aiwProgramGetOutputTensorCount(const aiwProgram* program);

	/// Returns the Nth output tensor of the program.
	///
	/// @param[in] outputTensorIndex		Index of the input tensor, must be less than the
	///										value returned by #aiwProgramGetOutputTensorCount.
	///
	/// @return		If the passed program and the outputTensorIndex are both valid, then the
	///				function returns a non-null pointer to the tensor object. The ownership of
	///				the returned object remains at the program, doesn't need deleting. If any
	///				parameter of the function is invalid, the function returns NULL.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT const aiwTensor* aiwProgramGetOutputTensorConst(
		const aiwProgram* program,
		uint32_t outputTensorIndex);

	/// Returns the Nth output tensor of the program, non-const version.
	///
	/// For further info please check #aiwProgramGetOutputTensorConst.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiwTensor* aiwProgramGetOutputTensor(
		const aiwProgram* program,
		uint32_t outputTensorIndex);

	/// Executes the passed program.
	///
	/// Starts the execution on the associated device, and waits until the program finishes or
	/// the execution timeout set by #aiwProgramSetExecutionTimeout is reached.
	///
	/// @param[in] program		Program to execute, must be a valid program object.
	///
	/// @return		If the passed program is valid, and could be executed successfully, then
	///				the function returns AIW_SUCCESS. If the execution timeout is reached, the
	///				function returns AIW_TIMEOUT. Otherwise the function returns with an error,
	///				and the error callback will be called with the detailed reasons of fail.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status aiwProgramExecute(aiwProgram* program);

	/// Starts the execution of the passed program.
	///
	/// If no program is executing, the function returns immediately, and the program will be executed in the background.
	/// #aiwProgramAwait should be used to wait for the completion of the program and finish the execution.
	/// Only one program can be executed at a time, if the device already executing a program, the function returns
	/// immediately with AIW_TIMEOUT.
	///
	/// @param[in] program		Program to start, must be a valid program object.
	///
	/// @return		If the passed program is valid, and could be started successfully, then
	///				the function returns AIW_SUCCESS. If the device is already executing a program,
	///				the function returns AIW_TIMEOUT. Otherwise the function returns with an error,
	///				and the error callback will be called with the detailed reasons of fail.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status aiwProgramExecuteAsync(aiwProgram* program);

	/// Waits for the completion of the passed program.
	///
	/// The function blocks until the program finishes its execution. Timeouts set by
	/// aiwProgramSetExecutionTimeout() are respected, but time measurement starts from the call of
	/// aiwProgramExecuteAwait().
	///
	/// @param[in] program		Program to wait for, must be a valid program object.
	///
	/// @return		If the passed program is valid, and could be waited successfully, then
	///				the function returns AIW_SUCCESS. Otherwise the function returns AIW_ERROR,
	///				and the error callback will be called with the detailed reasons of fail.
	///
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status aiwProgramAwait(aiwProgram* program);

	/// Returns the time of the program's last execution in milliseconds.
	///
	/// @return		If the passed program is NULL or invalid, the function returns 0. It also
	///				returns 0 when the last execution of the program was unsuccessful. Otherwise
	///				returns a positive integer.
	///				On asynchronous execution, the function returns the time elapsed between the call
	///				of aiwProgramExecuteAsync() and the return of aiwProgramAwait().
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT uint32_t aiwProgramExecutionTimeMsec(const aiwProgram* program);

	/// Returns the time of the program's last execution in clock cycles.
	///
	/// Clock cycles interpreted in the frequency of the program's associated device.
	///
	/// @return		If the passed program is NULL or invalid, the function returns 0. It also
	///				returns 0 when the last execution of the program was unsuccessful. Otherwise
	///				returns a positive integer.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT uint64_t aiwProgramExecutionTimeClockCycles(const aiwProgram* program);

	/// Uploads the content of internal input tensor buffers into the device's memory.
	/// Timeouts set by aiwProgramSetMemoryTransferTimeout() are respected, the function returns
	/// with AIW_TIMEOUT if the upload operation couldn't be finished in time. On error, the return
	/// value is AIW_ERROR, and the error callback will be called with the detailed reasons of fail.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status aiwProgramUploadInputs(aiwProgram* program);

	/// Updates the internal output tensor buffers by downloading their content from the
	/// device's memory.
	/// Timeouts set by aiwProgramSetMemoryTransferTimeout() are respected, the function returns
	/// with AIW_TIMEOUT if the download operation couldn't be finished in time. On error, the return
	/// value is AIW_ERROR, and the error callback will be called with the detailed reasons of fail.
	///
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status aiwProgramDownloadOutputs(aiwProgram* program);

	/// @brief Set memory transfer timeout for the program.
	/// @param program
	/// @param transferTimeoutMs Timeout for data transfer (DMA) operations in milliseconds. If 0, no wait happens.
	/// @return
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status
	aiwProgramSetMemoryTransferTimeout(aiwProgram* program, uint32_t transferTimeoutMs);

	/// @brief Set execution timeout for the program.
	/// @param program
	/// @param executionTimeoutMs Timeout for the execution of the program in milliseconds. If 0, no wait happens.
	/// @return
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status
	aiwProgramSetExecutionTimeout(aiwProgram* program, uint32_t executionTimeoutMs);

	/// @brief Get memory transfer timeout for the program.
	/// @param program
	/// @param transferTimeoutMs Timeout for data transfer (DMA) operations in milliseconds.
	/// @return
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status
	aiwProgramGetMemoryTransferTimeout(const aiwProgram* program, uint32_t* transferTimeoutMs);

	/// @brief Get execution timeout for the program.
	/// @param program
	/// @param executionTimeoutMs Timeout for the execution of the program in milliseconds.
	/// @return
	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status
	aiwProgramGetExecutionTimeout(const aiwProgram* program, uint32_t* executionTimeoutMs);

	/// @brief Sets if selftest should be enabled for the program when selftest execution policy is AIW_SEP_BEFORE_PROGRAM

	/// When device selftest policy is automatic (AIW_SEP_BEFORE_PROGRAM), the selftest is executed before the program.
	/// If there are multiple different neural networks executed in a row, the selftest can be disabled to save time
	/// by setting this to false, if the safety requirements of the use case allow it.
	///
	/// @param program
	/// @param enable
	/// @return

	AIWARE_RUNTIME_COMMON_LIB_C_EXPORT aiw_status aiwProgramEnableSelftest(aiwProgram* program, bool enable);

#ifdef __cplusplus
}
#endif

#endif //AIWARE_RUNTIME_COMMON_C__PROGRAM_H
