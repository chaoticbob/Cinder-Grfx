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
	// NOTE: Seperated depth/stencil is coming just not here yet.
	//
	struct RenderingInfo
	{
		RenderingInfo() {}

		RenderingInfo( const VkRect2D &area )
			: mRenderArea( area ) {}

		RenderingInfo( const vk::ImageViewRef &colorAttachment, const vk::ImageViewRef &depthStenciAttachment = nullptr );

		RenderingInfo( const std::vector<vk::ImageViewRef> &colorAttachments, const vk::ImageViewRef &depthStenciAttachment = nullptr );

		// clang-format off
		RenderingInfo &renderArea(const VkRect2D &rect) { mRenderArea = rect; return *this; }
		RenderingInfo &addColorAttachment( const vk::ImageViewRef &attachment, const vk::ImageViewRef &resolve = nullptr );
		RenderingInfo &addColorAttachment( const vk::ImageViewRef &attachment, const ColorA &clearValue, const vk::ImageViewRef &resolve = nullptr);
		RenderingInfo &setDepthStencilAttachment( const vk::ImageViewRef &attachment, const vk::ImageViewRef &resolve = nullptr );
		RenderingInfo &setDepthStencilAttachment( const vk::ImageViewRef &attachment, float depthClearValue, uint32_t stencilClearValue, const vk::ImageViewRef &resolve = nullptr );
		// clang-format on

	private:
		RenderingInfo( const std::vector<vk::ImageViewRef> &colorAttachments, const vk::ImageViewRef &dDepthAttachment, const vk::ImageViewRef &stencilAttachment );

		// clang-format off
		// Make these public when seperated depth/stencil has landed.
		RenderingInfo &setDepthAttachment( const vk::ImageViewRef &attachment, const vk::ImageViewRef &resolve = nullptr, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL );
		RenderingInfo &setDepthAttachment( const vk::ImageViewRef &attachment, float clearValue, const vk::ImageViewRef &resolve = nullptr, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL );
		RenderingInfo &setStencilAttachment( const vk::ImageViewRef &attachment, const vk::ImageViewRef &pResolve = nullptr, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL );
		RenderingInfo &setStencilAttachment( const vk::ImageViewRef &attachment, uint32_t clearValue, const vk::ImageViewRef &resolve = nullptr, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL );
		// clang-format on

	private:
		VkRect2D								  mRenderArea		 = {};
		std::vector<VkRenderingAttachmentInfoKHR> mColorAttachments	 = {};
		VkRenderingAttachmentInfoKHR			  mDepthAttachment	 = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
		VkRenderingAttachmentInfoKHR			  mStencilAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };

		friend CommandBuffer;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	virtual ~CommandBuffer();

	static CommandBufferRef create( vk::CommandPoolRef pool, VkCommandBufferLevel level, vk::DeviceRef device = vk::DeviceRef() );

	VkCommandBuffer getCommandBufferHandle() const { return mCommandBufferHandle; }

	bool isRecording() const { return mRecording; }
	bool isRendering() const { return mRendering; }

	void begin( VkCommandBufferUsageFlags usageFlags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT );
	void end();

	void bindDescriptorSets(
		VkPipelineBindPoint						 pipelineBindPoint,
		const vk::PipelineLayoutRef &			 pipelineLayout,
		uint32_t								 firstSet,
		const std::vector<vk::DescriptorSetRef> &sets,
		uint32_t								 dynamicOffsetCount = 0,
		const uint32_t *						 pDynamicOffsets	= nullptr );

	void bindPipeline(
		VkPipelineBindPoint	   pipelineBindPoint,
		const vk::PipelineRef &pipeline );

	void beginRendering( const RenderingInfo &ri );
	void endRendering();

	void setScissor( int32_t x, int32_t y, uint32_t width, uint32_t height );
	void setViewport( float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f );

	void clearColorAttachment( uint32_t index, const VkClearColorValue &clearValue, const VkRect2D &rect );
	void clearDepthStencilAttachment( float depthClearValue, uint32_t stencilClearValue, const VkRect2D &rect, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT );
	void clearDepthAttachment( float clearValue, const VkRect2D &rect );
	void clearStencilAttachment( uint32_t clearValue, const VkRect2D &rect );

	void bindIndexBuffer( vk::BufferRef &buffer, uint32_t offset, VkIndexType indexType );
	void bindVertexBuffers( uint32_t firstBinding, const std::vector<vk::BufferRef> &buffers, std::vector<uint64_t> offsets = {} );

	void setCullMode( VkCullModeFlags cullMode );
	void setDepthTestEnable( bool depthTestEnable );
	void setDepthWriteEnable( bool depthWriteEnable );
	void setFrontFace( VkFrontFace frontFace );

	void draw( uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance );
	void drawIndexed( uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance );

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
	CommandBuffer( vk::DeviceRef device, vk::CommandPoolRef pool, VkCommandBufferLevel level );
	CommandBuffer( vk::DeviceRef device, VkCommandBuffer commandBufferHandle );
	friend class CommandPool;

private:
	vk::CommandPoolRef mPool;
	VkCommandBuffer	   mCommandBufferHandle = VK_NULL_HANDLE;
	bool			   mRecording			= false;
	bool			   mRendering			= false;
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
		Options& flags( VkCommandPoolCreateFlags value ) { mFlags = value; return *this; }
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
