#pragma once

#include "cinder/vk/Buffer.h"

namespace cinder::vk {

class UniformBuffer
	: public vk::DynamicBuffer
{
public:
	virtual ~UniformBuffer() {}

private:

};

} // namespace cinder::vk
