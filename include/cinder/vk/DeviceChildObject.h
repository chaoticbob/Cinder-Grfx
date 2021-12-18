#pragma once

#include "cinder/vk/vk_config.h"

namespace cinder::vk {

class DeviceChildObject
{
public:
	DeviceChildObject( DeviceRef device );
	virtual ~DeviceChildObject();

	vk::DeviceRef getDevice() const;

	VkInstance getInstanceHandle() const;

	VkPhysicalDevice getGpuHandle() const;

	VkDevice getDeviceHandle() const;

private:
	DeviceRef mDevice;
};

} // namespace cinder::vk
