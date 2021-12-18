#pragma once

#include "cinder/vk/DeviceChildObject.h"

namespace cinder::vk {

//! @class CommandBuffer
//!
//!
class CommandBuffer
	: public vk::DeviceChildObject
{
public:
	//
	// Usage:
	//   - Combined depth/stencil uses depth resources and null out stencil resources.
	//   - Setting combined depth/stencil will invalidate stencil member variables.
	//   - Setting stencil after setting combined depth/stencil disables combine depth/stencil.
	//
	struct RenderingInfo
	{
		RenderingInfo() {}

		RenderingInfo( const VkRect2D &area )
			: mRenderArea( area ) {}

		RenderingInfo( const vk::ImageView *pColorAttachment, const vk::ImageView *pDepthStencilAttachment );

		RenderingInfo( const std::vector<const vk::ImageView *> colorAttachments, const vk::ImageView *pDepthStencilAttachment );

		RenderingInfo( const std::vector<const vk::ImageView *> colorAttachments, const vk::ImageView *pDepthAttachment, const vk::ImageView *pStencilAttachment );

		// clang-format off
		RenderingInfo& renderArea(const VkRect2D& rect) { mRenderArea = rect; return *this; }
		RenderingInfo& addColorAttachment( const vk::ImageView* pAttachment, const vk::ImageView* pResolve = nullptr );
		RenderingInfo& addColorAttachment( const vk::ImageView* pAttachment, const ColorA& clearValue, const vk::ImageView* pResolve = nullptr);
		RenderingInfo& setDepthAttachment( const vk::ImageView* pAttachment, const vk::ImageView* pResolve = nullptr, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL );
		RenderingInfo& setDepthAttachment( const vk::ImageView* pAttachment, float clearValue, const vk::ImageView* pResolve = nullptr, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL );
		RenderingInfo& setStencilAttachment( const vk::ImageView* pAttachment, const vk::ImageView* pResolve = nullptr, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL );
		RenderingInfo& setStencilAttachment( const vk::ImageView* pAttachment, uint32_t clearValue, const vk::ImageView* pResolve = nullptr, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL );
		RenderingInfo& setDepthStencilAttachment( const vk::ImageView* pAttachment, const vk::ImageView* pResolve = nullptr );
		RenderingInfo& setDepthStencilAttachment( const vk::ImageView* pAttachment, float depthClearValue, uint32_t stencilClearValue, const vk::ImageView* pResolve = nullptr );
		// clang-format on

	private:
		VkRect2D								  mRenderArea			= {};
		std::vector<VkRenderingAttachmentInfoKHR> mColorAttachments		= {};
		VkRenderingAttachmentInfoKHR			  mDepthAttachment		= { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
		VkRenderingAttachmentInfoKHR			  mStencilAttachment	= { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
		bool									  mCombinedDepthStencil = false;

		friend CommandBuffer;
	};

	virtual ~CommandBuffer();

	static CommandBufferRef create( VkCommandPool poolHandle, VkCommandBufferLevel level, vk::DeviceRef device = vk::DeviceRef() );

	VkCommandBuffer getCommandBufferHandle() const { return mCommandBufferHandle; }

	bool isRecording() const { return mRecording; }
	bool isRenderingActive() const { return mRenderingActive; }

	void begin( VkCommandBufferUsageFlags usageFlags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT );
	void end();

	void beginRendering( const RenderingInfo &ri );
	void endRendering();

	void clearColorAttachment( uint32_t index, const VkClearColorValue &clearValue, const VkRect2D &rect );
	void clearDepthStencilAttachment( float depthClearValue, uint32_t stencilClearValue, const VkRect2D &rect, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT );
	void clearDepthAttachment( float clearValue, const VkRect2D &rect );
	void clearStencilAttachment( uint32_t clearValue, const VkRect2D &rect );

	void transitionImageLayout(
		VkImage				 image,
		VkImageAspectFlags	 aspectMask,
		uint32_t			 baseMipLevel,
		uint32_t			 levelCount,
		uint32_t			 baseArrayLayer,
		uint32_t			 layerCount,
		VkImageLayout		 oldLayout,
		VkImageLayout		 newLayout,
		VkPipelineStageFlags newPipelineStageFlags );

	void transitionImageLayout(
		ImageRef			 image,
		VkImageLayout		 oldLayout,
		VkImageLayout		 newLayout,
		VkPipelineStageFlags newPipelineStageFlags );

private:
	CommandBuffer( vk::DeviceRef device, VkCommandPool poolHandle, VkCommandBufferLevel level, VkCommandBuffer commandBufferHandle, bool disposeCommandBuffer );
	friend class CommandPool;

private:
	VkCommandPool	mPoolHandle			  = VK_NULL_HANDLE;
	VkCommandBuffer mCommandBufferHandle  = VK_NULL_HANDLE;
	bool			mDisposeCommandBuffer = false;
	bool			mRecording			  = false;
	bool			mRenderingActive	  = false;
};

//! @class CommandPool
//!
//!
class CommandPool
	: public vk::DeviceChildObject
{
public:
	struct Options
	{
		Options() {}

		// clang-format off
		Options& flags(VkCommandPoolCreateFlags value) { mFlags = value; return *this; }
		// clang-format on

	private:
		VkCommandPoolCreateFlags mFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	};

	virtual ~CommandPool();

	static CommandPoolRef create( uint32_t queueFamilyIndex, const Options &options = Options(), vk::DeviceRef device = vk::DeviceRef() );

	uint32_t getQueueFamilyIndex() const { return mQueueFamilyIndex; }

	VkCommandPool getCommandPoolHandle() const { return mCommandPoolHandle; }

	std::vector<vk::CommandBufferRef> allocateCommandBuffers( uint32_t count, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY );

private:
	CommandPool( vk::DeviceRef device, uint32_t queueFamilyIndex, const Options &options );

private:
	uint32_t	  mQueueFamilyIndex	 = VK_QUEUE_FAMILY_IGNORED;
	VkCommandPool mCommandPoolHandle = VK_NULL_HANDLE;
};

} // namespace cinder::vk
