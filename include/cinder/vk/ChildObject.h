#pragma once

#include "cinder/vk/vk_config.h"

namespace cinder::vk {

//! @class DeviceChildObject
//!
//!
class DeviceChildObject
{
public:
	DeviceChildObject( vk::DeviceRef device );
	virtual ~DeviceChildObject();

	vk::DeviceRef getDevice() const;

	VkInstance getInstanceHandle() const;

	VkPhysicalDevice getGpuHandle() const;

	VkDevice getDeviceHandle() const;

private:
	vk::DeviceRef mDevice;
};

//! @class ContextChildObject
//!
//!
class ContextChildObject
{
public:
	ContextChildObject( vk::ContextRef context );
	virtual ~ContextChildObject();

	vk::ContextRef getContext() const;

	uint32_t getNumFramesInFlight() const;

	uint32_t getCurrentFrameIndex() const;

private:
	vk::ContextRef mContext;
};

} // namespace cinder::vk
