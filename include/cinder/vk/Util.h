#pragma once

#include "cinder/vk/vk_config.h"

namespace cinder::vk {

class Device;

VmaMemoryUsage toVmaMemoryUsage( MemoryUsage value );

//! Returns a VkSampleCountFlagBits value based on samples
VkSampleCountFlagBits toVkSampleCount( uint32_t samples );

//! Returns size of format
uint32_t formatSize( VkFormat format );

// Determines the image aspect mask for format
VkImageAspectFlags determineAspectMask( VkFormat format );

//! Guesses a pipeline stage for the layout
VkPipelineStageFlags guessPipelineStageFromImageLayout( vk::DeviceRef device, VkImageLayout layout, bool isSource = false );

//! Records a image layout transition using a pipeline barreir to specified command buffer
VkResult cmdTransitionImageLayout(
	PFN_vkCmdPipelineBarrier fnCmdPipelineBarrier,
	VkCommandBuffer			 commandBuffer,
	VkImage					 image,
	VkImageAspectFlags		 aspectMask,
	uint32_t				 baseMipLevel,
	uint32_t				 levelCount,
	uint32_t				 baseArrayLayer,
	uint32_t				 layerCount,
	VkImageLayout			 oldLayout,
	VkImageLayout			 newLayout,
	VkPipelineStageFlags	 newPipelineStageFlags );

} // namespace cinder::vk
