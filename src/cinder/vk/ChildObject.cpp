#include "cinder/vk/ChildObject.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Environment.h"

namespace cinder::vk {

////////////////////////////////////////////////////////////////////////////////////////////////////
// DeviceChildObject

DeviceChildObject::DeviceChildObject( DeviceRef device )
	: mDevice( device )
{
}

DeviceChildObject::~DeviceChildObject()
{
}

vk::DeviceRef DeviceChildObject::getDevice() const
{
	return mDevice;
}

VkInstance DeviceChildObject::getInstanceHandle() const
{
	return Environment::get()->getInstanceHandle();
}

VkPhysicalDevice DeviceChildObject::getGpuHandle() const
{
	return getDevice()->getGpuHandle();
}

VkDevice DeviceChildObject::getDeviceHandle() const
{
	return getDevice()->getDeviceHandle();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ContextChildObject

ContextChildObject::ContextChildObject( vk::ContextRef context )
	: mContext( context )
{
}

ContextChildObject::~ContextChildObject()
{
}

vk::ContextRef ContextChildObject::getContext() const
{
	return mContext;
}

} // namespace cinder::vk
