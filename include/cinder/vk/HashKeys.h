#pragma once

#include "cinder/vk/vk_config.h"

namespace cinder::vk {

struct SamplerHashKey
{
	VkFilter			 magFilter				 = VK_FILTER_NEAREST;
	VkFilter			 minFilter				 = VK_FILTER_NEAREST;
	VkSamplerMipmapMode	 mipmapMode				 = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	VkSamplerAddressMode addressModeU			 = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkSamplerAddressMode addressModeV			 = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkSamplerAddressMode addressModeW			 = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	int32_t				 mipLodBias				 = 0;
	uint32_t			 maxAnisotropy			 = 0;					// 0 = disabled, [1, maxSamplerAnisotropy (usually 16)] = enabled
	VkCompareOp			 compareOp				 = VK_COMPARE_OP_NEVER; // VK_COMPARE_OP_NEVER = disabled, everything else = enabled
	uint32_t			 minLod					 = 0;
	uint32_t			 maxLod					 = 0;
	VkBorderColor		 borderColor			 = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	VkBool32			 unnormalizedCoordinates = VK_FALSE;
};

} // namespace cinder::vk
