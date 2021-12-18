#include "cinder/vk/Sync.h"
#include "cinder/vk/Device.h"
#include "cinder/app/RendererVk.h"

namespace cinder::vk {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fence

FenceRef Fence::create( bool signaled, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return FenceRef( new Fence( device, signaled ) );
}

Fence::Fence( vk::DeviceRef device, bool signaled )
	: vk::DeviceChildObject( device )
{
	VkFenceCreateInfo vkci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	vkci.pNext			   = nullptr;
	vkci.flags			   = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	VkResult vkres = getDevice()->createFence( &vkci, &mFenceHandle );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkCreateFence", vkres );
	}
}

Fence::~Fence()
{
	if ( mFenceHandle != VK_NULL_HANDLE ) {
		getDevice()->destroyFence( mFenceHandle );
		mFenceHandle = VK_NULL_HANDLE;
	}
}

VkResult Fence::waitAndReset( uint64_t timeout )
{
	VkResult vkres = CI_VK_DEVICE_FN( WaitForFences( getDeviceHandle(), 1, &mFenceHandle, VK_TRUE, timeout ) );
	if ( vkres != VK_SUCCESS ) {
		return vkres;
	}

	vkres = CI_VK_DEVICE_FN( ResetFences( getDeviceHandle(), 1, &mFenceHandle ) );
	if ( vkres != VK_SUCCESS ) {
		return vkres;
	}
	return VK_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semaphore

SemaphoreRef Semaphore::create( vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return SemaphoreRef( new Semaphore( device, VK_SEMAPHORE_TYPE_BINARY, 0 ) );
}

SemaphoreRef Semaphore::create( uint64_t initialValue, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return SemaphoreRef( new Semaphore( device, VK_SEMAPHORE_TYPE_TIMELINE, initialValue ) );
}

Semaphore::Semaphore( vk::DeviceRef device, VkSemaphoreType semaphoreType, uint64_t initialValue )
	: vk::DeviceChildObject( device ),
	  mSemaphoreType( semaphoreType )
{
	VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
	semaphoreTypeCreateInfo.pNext					  = nullptr;
	semaphoreTypeCreateInfo.semaphoreType			  = semaphoreType;
	semaphoreTypeCreateInfo.initialValue			  = initialValue;

	VkSemaphoreCreateInfo vkci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	vkci.pNext				   = ( mSemaphoreType == VK_SEMAPHORE_TYPE_TIMELINE ) ? &semaphoreTypeCreateInfo : nullptr;
	vkci.flags				   = 0;

	VkResult vkres = getDevice()->createSemaphore( &vkci, &mSemaphoreHandle );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkCreateSemaphore", vkres );
	}
}

Semaphore::~Semaphore()
{
	if ( mSemaphoreHandle != VK_NULL_HANDLE ) {
		getDevice()->destroySemaphore( mSemaphoreHandle );
		mSemaphoreHandle = VK_NULL_HANDLE;
	}
}

void Semaphore::signal( uint64_t value )
{
	if ( mSemaphoreType == VK_SEMAPHORE_TYPE_BINARY ) {
		return;
	}

	VkSemaphoreSignalInfo info = { VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO };
	info.pNext				   = nullptr;
	info.semaphore			   = mSemaphoreHandle;
	info.value				   = value;

	VkResult vkres = CI_VK_DEVICE_FN( SignalSemaphore( getDeviceHandle(), &info ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkSignalSemaphore", vkres );
	}
}

void Semaphore::wait( uint64_t value, uint64_t timeout )
{
	if ( mSemaphoreType == VK_SEMAPHORE_TYPE_BINARY ) {
		return;
	}

	VkSemaphoreWaitInfo info = { VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
	info.pNext				 = nullptr;
	info.flags				 = 0;
	info.semaphoreCount		 = 1;
	info.pSemaphores		 = &mSemaphoreHandle;
	info.pValues			 = &value;

	VkResult vkres = CI_VK_DEVICE_FN( WaitSemaphores( getDeviceHandle(), &info, timeout ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkWaitSemaphores", vkres );
	}
}

uint64_t Semaphore::getCounterValue() const
{
	if ( mSemaphoreType == VK_SEMAPHORE_TYPE_BINARY ) {
		return 0;
	}

	uint64_t value = CINDER_INVALID_SEMAPHORE_VALUE;
	VkResult vkres = CI_VK_DEVICE_FN( GetSemaphoreCounterValue( getDeviceHandle(), mSemaphoreHandle, &value ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkGetSemaphoreCounterValue", vkres );
	}
	return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CountingSemaphore

CountingSemaphoreRef CountingSemaphore::create( uint64_t initialValue, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return CountingSemaphoreRef( new CountingSemaphore( device, initialValue ) );
}

CountingSemaphore::CountingSemaphore( vk::DeviceRef device, uint64_t initialValue )
	: vk::Semaphore( device, VK_SEMAPHORE_TYPE_TIMELINE, initialValue ),
	  mCounterValue( initialValue )
{
}

CountingSemaphore::~CountingSemaphore()
{
}

uint64_t CountingSemaphore::incrementCounter()
{
	std::lock_guard<std::mutex> lock( mMutex );
	++mCounterValue;
	return mCounterValue;
}

} // namespace cinder::vk
