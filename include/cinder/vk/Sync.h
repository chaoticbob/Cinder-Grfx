#pragma once

#include "cinder/vk/DeviceChildObject.h"

#include <mutex>

namespace cinder::vk {

//! @class Fence
//!
//!
class Fence
	: public vk::DeviceChildObject
{
public:
	virtual ~Fence();

	VkFence getFenceHandle() const { return mFenceHandle; }

	static FenceRef create( bool signaled, vk::DeviceRef device = vk::DeviceRef() );

	VkResult waitAndReset( uint64_t timeout = UINT64_MAX );

private:
	Fence( vk::DeviceRef device, bool signaled );

private:
	VkFence mFenceHandle = VK_NULL_HANDLE;
};

//! @class Semaphore
//!
//!
class Semaphore
	: public vk::DeviceChildObject
{
public:
	virtual ~Semaphore();

	//! Creates a binary semaphore
	static SemaphoreRef create( vk::DeviceRef device = vk::DeviceRef() );
	//! Creates a timeline semaphore
	static SemaphoreRef create( uint64_t initialValue, vk::DeviceRef device = vk::DeviceRef() );

	VkSemaphoreType getSemaphoreType() const { return mSemaphoreType; }

	bool isBinary() const { return ( getSemaphoreType() == VK_SEMAPHORE_TYPE_BINARY ); }

	bool isTimeline() const { return ( getSemaphoreType() == VK_SEMAPHORE_TYPE_TIMELINE ); }

	VkSemaphore getSemaphoreHandle() const { return mSemaphoreHandle; }

	void signal( uint64_t value );
	void wait( uint64_t value, uint64_t timeout = UINT64_MAX );

	uint64_t getCounterValue() const;

protected:
	Semaphore( vk::DeviceRef device, VkSemaphoreType semaphoreType, uint64_t initialValue );

private:
	VkSemaphoreType mSemaphoreType	 = VK_SEMAPHORE_TYPE_TIMELINE;
	VkSemaphore		mSemaphoreHandle = VK_NULL_HANDLE;
};

//! class CountingSemaphore
//!
//!
class CountingSemaphore
	: public vk::Semaphore
{
public:
	virtual ~CountingSemaphore();

	static CountingSemaphoreRef create( uint64_t initialValue = 0, vk::DeviceRef device = vk::DeviceRef() );

	uint64_t incrementCounter();

private:
	CountingSemaphore( vk::DeviceRef device, uint64_t initialValue );

private:
	std::mutex mMutex;
	uint64_t   mCounterValue = 0;
};

} // namespace cinder::vk
