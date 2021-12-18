#include "cinder/vk/DeviceChildObject.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Environment.h"

namespace cinder::vk {

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

} // namespace cinder::vk
