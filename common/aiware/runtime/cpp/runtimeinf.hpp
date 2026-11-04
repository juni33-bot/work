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

/*
* This interface is th base for every system which is able to execute a neural network
* workload with aiWare arithmetic.
*/
#ifndef AIWARE_RUNTIME__RUNTIME_INF_HPP
#define AIWARE_RUNTIME__RUNTIME_INF_HPP

#include "aiware/common/c/status.h"
#include "aiware/common/c/tensordimensions.h"
#include "aiware/runtime/c/selftest.h"
#include "aiware/runtime/cpp/aiware-runtime-inf-lib-cpp_export.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace aiware
{
namespace runtime
{
namespace inf
{

class Buffer;
class Program;
class Status;

/// An interface for any kind of device which is capable of performing neural network
/// workloads with aiWare arithmetic.
///
/// Each device has a set of programs associated with itself. These programs represent
/// the workloads, and the device provides methods to query and manage them. The device
/// is also responsible for destroying these programs.
class AIWARE_RUNTIME_INF_LIB_CPP_EXPORT Device
{
public:
	virtual ~Device();

	Device(const Device&) = delete;
	Device& operator=(const Device&) = delete;
	Device(Device&&) = delete;
	Device& operator=(Device&&) = delete;

	/// Returns the number of Programs associated with this device instance.
	virtual uint32_t programCount() const = 0;

	/// Returns the program identified by the given index, or nullptr if the index is out of range.
	virtual const Program* program(uint32_t index) const = 0;

	/// Returns the program identified by the given index, or nullptr if the index is out of range.
	virtual Program* program(uint32_t index) = 0;

	/// Deletes the given program.
	///
	/// The default implementation doesn't do anything and returns false. The derived classes
	/// may override this behaviour with something meaningful. These implementation should take
	/// care of only such programs can be deleted by a device which are associated with it.
	virtual bool deleteProgram(Program* program);

	/// Returns if the device supports tensor export or not.
	///
	/// The default implementation doesn't do anything and returns false. The derived classes
	/// may override this behaviour with something meaningful.
	virtual bool tensorExportSupported() const;

	/// Sets if the selftest should be executed
	/// before a program or not.
	/// Returns true if the operation was successful.
	virtual bool setSelftestExecutionPolicy(aiwSelftestExecutionPolicy policy);

	/// Returns the selftest execution policy.
	/// If the operation was successful, then the policy
	/// will be stored in the policy parameter.
	/// Returns true if the operation was successful.
	virtual bool selftestExecutionPolicy(aiwSelftestExecutionPolicy& policy) const;

	/// Manually executes the selftest program.
	/// Return value is the same as the execute() method.
	virtual Status executeSelftest();

protected:
	Device();
};

/// Internal helper for implementing RAII style raw buffer pointer management.
///
/// #BufferPtr will call the #acquire method during its construction, which will
/// get a raw pointer. This pointer will be released in the destructor by calling
/// this class' #release method.
///
/// If the buffer is pinned, then the #acquire method shall return the same pointer
/// each time it's called.
class AIWARE_RUNTIME_INF_LIB_CPP_EXPORT BufferOperations
{
public:
	virtual ~BufferOperations();

	BufferOperations(const BufferOperations&) = delete;
	BufferOperations(BufferOperations&&) = delete;

	BufferOperations& operator=(const BufferOperations&) = delete;
	BufferOperations& operator=(BufferOperations&&) = delete;

	/// Returns the size of the buffer in bytes.
	virtual uint64_t size() = 0;

	/// Gets the raw pointer to a buffer which can be used until
	/// the #release method is called (unless it's a pinned buffer).
	virtual void* acquire() = 0;

	/// Releases the acquired pointer which shouldn't be used anymore
	/// (unless it's a pinned buffer).
	virtual void release() = 0;

	/// If returns true, then #acquire method shall return the same pointer
	/// every time, and this pointer can be cached. Otherwise the #acquire
	/// may return different pointers, and they can be used only until #release
	/// is called.
	virtual bool pinned() const = 0;

protected:
	BufferOperations();
};

/// Wrapper class for a buffers memory location.
///
/// Don't store the raw pointer this object can be casted to unless the
/// parent buffer is a pinned one.
///
/// The const version of this object has only const conversion operators,
/// while the non-const version has both.
///
/// The class has conversion operator for multiple raw pointer types, but
/// it doesn't change the basic data type of the buffer, which can be
/// queried from the #Buffer class.
template<bool Const>
class AIWARE_RUNTIME_INF_LIB_CPP_EXPORT BufferPtr final
{
public:
	BufferPtr() = delete;
	BufferPtr(const BufferPtr&) = delete;
	BufferPtr(BufferPtr&& other) noexcept;

	BufferPtr& operator=(const BufferPtr&) = delete;
	BufferPtr& operator=(BufferPtr&&) noexcept;

	~BufferPtr();

	operator const void*() const;

	operator const uint8_t*() const;

	operator const int8_t*() const;

	operator const char*() const;

	operator const float*() const;

	template<bool B = Const, typename = typename std::enable_if<!B, void*>::type>
	operator void*()
	{
		return _ptr;
	}

	template<bool B = Const, typename = typename std::enable_if<!B, uint8_t*>::type>
	operator uint8_t*()
	{
		return static_cast<uint8_t*>(_ptr);
	}

	template<bool B = Const, typename = typename std::enable_if<!B, int8_t*>::type>
	operator int8_t*()
	{
		return static_cast<int8_t*>(_ptr);
	}

	template<bool B = Const, typename = typename std::enable_if<!B, char*>::type>
	operator char*()
	{
		return static_cast<char*>(_ptr);
	}

	template<bool B = Const, typename = typename std::enable_if<!B, float*>::type>
	operator float*()
	{
		return static_cast<float*>(_ptr);
	}

private:
	BufferPtr(BufferOperations& ops);

	friend class Buffer;

private:
	BufferOperations* _ops = nullptr;
	void* _ptr = nullptr;
};

/// Represents a data buffer associated with a tensor.
///
/// Through such an object the content of the tensor can be manipulated
/// directly. To do so, the layout of th buffer's data (the ordering) must
/// be taken into account. This ordering can be queried by the #ordering
/// method.
///
/// However aiWare like devices usually use int8 arithmetic, these buffer
/// can represent float data too if that's how the implementation defines.
/// In this case take care of using the #size method, which always returns
/// the size of the buffer in bytes instead of the element count.
///
/// The actual memory behind the buffer can be get by the #ptr functions
/// (const and non-const). Instead of returning actual raw pointers these
/// functions return wrapper objects, which can be casted to pointers. The
/// reason for this is these buffers may not pinned to a specific memory
/// location but changes time to time. These wrappers manage the acquiring
/// and releasing of these raw buffer in a RAII style. Thus the raw pointers
/// coming from these wrappers shouldn't be cached or stored, unless the
/// buffer is pinned (which means the buffer location doesn't change), so
/// it can be safely stored and used.
class AIWARE_RUNTIME_INF_LIB_CPP_EXPORT Buffer
{
public:
	using ConstPtr = BufferPtr<true>;
	using Ptr = BufferPtr<false>;

	enum class Ordering : std::uint8_t
	{
		TileBase,
		CellRowBased,
		RowMajor
	};

public:
	virtual ~Buffer();

	Buffer(const Buffer&) = delete;
	Buffer& operator=(const Buffer&) = delete;
	Buffer(Buffer&&) = delete;
	Buffer& operator=(Buffer&&) = delete;

	/// Returns the size of the buffer in bytes.
	virtual uint64_t size() const = 0;

	/// Returns the ordering of the underlying buffer.
	virtual Ordering ordering() const = 0;

	/// Returns whether the buffer is pinned (fixed location) or not.
	bool pinned() const;

	/// Returns a const pointer to the underlying raw buffer.
	ConstPtr ptr() const;

	/// Returns a non-const pointer to the underlying raw buffer.
	Ptr ptr();

protected:
	Buffer();

	virtual const BufferOperations& bufferOperations() const = 0;
	virtual BufferOperations& bufferOperations() = 0;
};

/// Represents an external memory descriptor for an aiWare memory tensor.
///
/// This object is used to store the export or import descriptors ofthe physical memory of
/// a given tensor. The descriptor can be used by other accelerators to access the memory area.
///
/// The descriptor is hardware platform and operating system specific.
class AIWARE_RUNTIME_INF_LIB_CPP_EXPORT ExternalMemoryDescriptor final
{
public:
	ExternalMemoryDescriptor();

	ExternalMemoryDescriptor(const ExternalMemoryDescriptor&);
	ExternalMemoryDescriptor(ExternalMemoryDescriptor&&) noexcept;

	ExternalMemoryDescriptor& operator=(const ExternalMemoryDescriptor&);
	ExternalMemoryDescriptor& operator=(ExternalMemoryDescriptor&&) noexcept;

	/// Returns the status of the descriptor, true if it's valid, false otherwise.
	operator bool() const;
	/// @brief Returns the descriptor to be used by other accelerators.
	/// @return The platform-dependent descriptor object.
	void* descriptor() const;

private:
	ExternalMemoryDescriptor(bool status, void* descriptor = nullptr);

	friend class Tensor;

private:
	bool _status = false;
	void* _descriptor = nullptr;
};

/// Represents a tensor of the neural network.
///
/// Tensor interface has 3 group of member functions: the first group
/// is the property getter functions. Note that some of these functions
/// may not implemented by the derived classes.
///
/// The second group of method provide an easy way to get or set the
/// tensor's data. These methods take an int8/uint8 or float buffer in
/// NCHW order and copy the data from it into the internal buffer or
/// vice versa, regardless of the internal data representation or layout.
/// All subclass must properly implement these methods.
///
/// The subclasses also shall provide access to these internal buffers,
/// where the data is actually stored. The third group of methods serve
/// this purpose.
class AIWARE_RUNTIME_INF_LIB_CPP_EXPORT Tensor
{
public:
	enum class DataType : std::uint8_t
	{
		Int8,
		Float32
	};

public:
	virtual ~Tensor();

	Tensor(const Tensor&) = delete;
	Tensor(Tensor&&) = delete;
	Tensor& operator=(const Tensor&) = delete;
	Tensor& operator=(Tensor&&) = delete;

	/// Returns the id of the buffer if implemented, otherwise returns 0.
	virtual uint32_t id() const;

	/// Returns the name of the buffer if implemented, otherwise returns
	/// an empty string.
	virtual const std::string& name() const;

	/// Returns the dimension of tensor if implemented, otherwise returns
	/// an invalid dimension object (all dimensions are 0).
	virtual const aiwTensorDimensions& dim() const;

	/// Returns the original dimension of the tensor if implemented,
	/// otherwise returns an empty vector.
	///
	/// The original dimension is the one can be found in the original
	/// network descriptor file (i.e. NNEF graph file), not affected by
	/// any transformations.
	virtual std::vector<uint32_t> originalDim() const;

	/// Returns the data type of the buffer if implemented, otherwise
	/// returns DataType::Int8.
	virtual DataType dataType() const;

	/// Returns the sign of the tensor, or false if it's not implemented
	/// in the subclasses. It also may return dummy value if the tensor's
	/// data type is float.
	virtual bool sign() const;

	/// Returns the exponent of the tensor, or 0 if it's not implemented
	/// in the subclasses. It also may return dummy value if the tensor's
	/// data type is float.
	virtual int8_t exponent() const;

	/// Returns the size of the elements in the tensor.
	///
	/// Always returns a positive number, otherwise some error happened.
	/// Note that it usually doesn't return the same number as #Buffer::size.
	virtual uint32_t nchwSize() const = 0;

	/// Sets the tensor's data from the passed NCHW ordered buffer, and
	/// returns true if it was successful.
	///
	/// The passed buffer can't be null and must be at least #nchwSize element
	/// large.
	///
	/// If the tensor's data type is float, then the function should do
	/// anything, just return with false.
	virtual bool setDataNCHW(const uint8_t* data) = 0;

	/// Sets the tensor's data from the passed NCHW ordered buffer, and
	/// returns true if it was successful.
	///
	/// The passed buffer can't be null and must be at least #nchwSize element
	/// large.
	///
	/// This function always shall be properly implemented. If the tensor's
	/// data type is int8, then he values will be quantized, otherwise simple
	/// copy will be performed.
	virtual bool setDataNCHW(const float* data) = 0;

	/// Same as int8 variant of #setDataNCHW except if fills the passed buffer
	/// with the tensor's data.
	virtual bool getDataNCHW(uint8_t* data) const = 0;

	/// Same as float variant of #setDataNCHW except if fills the passed buffer
	/// with the tensor's data.
	virtual bool getDataNCHW(float* data) const = 0;

	/// Returns the buffer to the underlying raw data.
	virtual const Buffer& rawBuffer() const = 0;

	/// Returns the buffer to the underlying raw data.
	virtual Buffer& rawBuffer() = 0;

	/// Exports the tensor's memory descriptor.
	virtual ExternalMemoryDescriptor exportMemory();

	/// Releases the memory descriptor, optional.
	virtual bool releaseExportedMemory(ExternalMemoryDescriptor& descriptor);

protected:
	Tensor();

	ExternalMemoryDescriptor createExtMemDescriptor(bool status, void* desc);

private:
	static const std::string _sDummyName;
	static const aiwTensorDimensions _sDummyDim;
};

/// Represents a return status code of an operation where the status can differ from success or error.
/// The status code can be used to determine the result of an operation but is also compatible with
/// the "classical" SUCCESS/ERROR status codes.
class AIWARE_RUNTIME_INF_LIB_CPP_EXPORT Status final
{
public:
	Status();
	Status(aiw_status s);
	Status(bool s);

	Status(const Status&);
	Status(Status&&) noexcept;

	Status& operator=(const Status&);
	Status& operator=(Status&&) noexcept;

	aiw_status statusCode() const;

	bool success() const;
	bool error() const;
	bool timeout() const;

	operator bool() const;

private:
	aiw_status _s;
};

/// Represents and executable neural network workload associated
/// with a device.
class AIWARE_RUNTIME_INF_LIB_CPP_EXPORT Program
{
public:
	virtual ~Program();

	Program(const Program&) = delete;
	Program(Program&&) = delete;
	Program& operator=(const Program&) = delete;
	Program& operator=(Program&&) = delete;

	/// Number of the input tensor, shall return a positive number,
	/// otherwise some error happened.
	virtual uint32_t inputTensorCount() const = 0;

	/// Returns the input tensor identified by the given index, or nullptr
	/// if index is out of range.
	virtual const Tensor* inputTensor(uint32_t index) const = 0;

	/// Returns the input tensor identified by the given index, or nullptr
	/// if index is out of range.
	virtual Tensor* inputTensor(uint32_t) = 0;

	/// Number of the output tensor, shall return a positive number,
	/// otherwise some error happened.
	virtual uint32_t outputTensorCount() const = 0;

	/// Returns the output tensor identified by the given index, or nullptr
	/// if index is out of range.
	virtual const Tensor* outputTensor(uint32_t index) const = 0;

	/// Returns the output tensor identified by the given index, or nullptr
	/// if index is out of range.
	virtual Tensor* outputTensor(uint32_t) = 0;

	/// Executes the workload, and returns the status.
	virtual Status execute() = 0;

	/// Starts the execution of the workload without blocking, and returns the status.
	virtual Status executeAsync();

	/// Waits for the execution of the workload to finish, and returns the status.
	virtual Status await();

	/// Checks if program supports asynchronous execution.
	virtual bool asyncExecutionSupported() const;

	/// Synchronizes the input tensors data between the host and the
	/// device (from the host to the device) if necessary.
	virtual Status uploadInputs() = 0;

	/// Synchronizes the output tensors data between the host and the
	/// device (from the device to the host) if necessary.
	virtual Status downloadOutputs() = 0;

	/// Returns whether setting memory transfer timeout is supported.
	virtual bool memoryTransferTimeoutSupported() const;

	/// Returns whether setting execution timeout is supported.
	virtual bool executionTimeoutSupported() const;

	/// Sets the memory transfer (DMA) timeout. Timeout 0 means no timeout.
	/// Returns true if the operation was successful.
	/// If memory transfer timeout isn't supported, then it always return false.
	virtual bool setMemoryTransferTimeout(uint32_t transferTimeoutMs);

	/// Sets the program execution timeout. Timeout 0 means no timeout.
	/// Returns true if the operation was successful.
	/// If execution timeout isn't supported, then it always return false.
	virtual bool setExecutionTimeout(uint32_t executionTimeoutMs);

	/// Gets the memory transfer (DMA) timeout.
	/// Returns true if the operation was successful.
	/// If memory transfer timeout isn't supported, then it always return false.
	virtual bool getMemoryTransferTimeout(uint32_t& transferTimeoutMs) const;

	/// Gets the program execution timeout.
	/// Returns true if the operation was successful.
	/// If execution timeout isn't supported, then it always return false.
	virtual bool getExecutionTimeout(uint32_t& executionTimeoutMs) const;

	/// Fine-grained selftest control.
	/// If the selftest is enabled, then it will be executed
	/// according to the device policy. If it's disabled, then
	/// it won't be executed, regardless the policy - useful
	/// for performance reasons when multiple networks are executed for a
	/// single frame. By default the selftest is enabled.
	/// Returns true if the operation was successful.

	virtual bool enableSelftest(bool enabled);

protected:
	Program();
};

} // namespace inf
} // namespace runtime
} // namespace aiware

#endif //AIWARE_RUNTIME__RUNTIME_INF_HPP
