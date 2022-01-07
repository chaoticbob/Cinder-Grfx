#include "cinder/vk/Command.h"
#include "cinder/vk/Buffer.h"
#include "cinder/vk/Descriptor.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Image.h"
#include "cinder/vk/Pipeline.h"
#include "cinder/app/RendererVk.h"

namespace cinder::vk {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CommandBuffer::RenderingInfo

CommandBuffer::RenderingInfo::RenderingInfo( const vk::ImageViewRef &colorAttachment, const vk::ImageViewRef &depthStenciAttachment )
{
	if ( colorAttachment ) {
		renderArea( colorAttachment->getImageArea() );
	}
	else if ( depthStenciAttachment != nullptr ) {
		renderArea( depthStenciAttachment->getImageArea() );
	}
}

CommandBuffer::RenderingInfo::RenderingInfo( const std::vector<vk::ImageViewRef> &colorAttachments, const vk::ImageViewRef &depthStenciAttachment )
{
	if ( !colorAttachments.empty() ) {
		renderArea( colorAttachments[0]->getImageArea() );
	}
	else if ( depthStenciAttachment ) {
		renderArea( depthStenciAttachment->getImageArea() );
	}

	for ( const auto &attachment : colorAttachments ) {
		addColorAttachment( attachment );
	}

	if ( depthStenciAttachment ) {
		setDepthStencilAttachment( depthStenciAttachment );
	}
}

CommandBuffer::RenderingInfo::RenderingInfo( const std::vector<vk::ImageViewRef> &colorAttachments, const vk::ImageViewRef &depthAttachment, const vk::ImageViewRef &stencilAttachment )
{
	if ( !colorAttachments.empty() ) {
		renderArea( colorAttachments[0]->getImageArea() );
	}
	else if ( depthAttachment ) {
		renderArea( depthAttachment->getImageArea() );
	}
	else if ( stencilAttachment ) {
		renderArea( stencilAttachment->getImageArea() );
	}

	for ( const auto &attachment : colorAttachments ) {
		addColorAttachment( attachment );
	}

	if ( depthAttachment ) {
		setDepthAttachment( depthAttachment );
	}

	if ( stencilAttachment ) {
		setStencilAttachment( stencilAttachment );
	}
}

CommandBuffer::RenderingInfo &CommandBuffer::RenderingInfo::addColorAttachment( const vk::ImageViewRef &attachment, const vk::ImageViewRef &resolve )
{
	VkRenderingAttachmentInfoKHR vkai = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
	vkai.pNext						  = nullptr;
	vkai.imageView					  = attachment->getImageViewHandle();
	vkai.imageLayout				  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	vkai.resolveMode				  = resolve != nullptr ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
	vkai.resolveImageView			  = resolve != nullptr ? resolve->getImageViewHandle() : VK_NULL_HANDLE;
	vkai.resolveImageLayout			  = resolve != nullptr ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
	vkai.loadOp						  = VK_ATTACHMENT_LOAD_OP_LOAD;
	vkai.storeOp					  = VK_ATTACHMENT_STORE_OP_STORE;
	vkai.clearValue.color.float32[0]  = 0;
	vkai.clearValue.color.float32[1]  = 0;
	vkai.clearValue.color.float32[2]  = 0;
	vkai.clearValue.color.float32[3]  = 0;

	mColorAttachments.push_back( vkai );

	return *this;
}

CommandBuffer::RenderingInfo &CommandBuffer::RenderingInfo::addColorAttachment( const vk::ImageViewRef &attachment, const ColorA &clearValue, const vk::ImageViewRef &resolve )
{
	VkRenderingAttachmentInfoKHR vkai = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
	vkai.pNext						  = nullptr;
	vkai.imageView					  = attachment->getImageViewHandle();
	vkai.imageLayout				  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	vkai.resolveMode				  = resolve ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
	vkai.resolveImageView			  = resolve ? resolve->getImageViewHandle() : VK_NULL_HANDLE;
	vkai.resolveImageLayout			  = resolve ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
	vkai.loadOp						  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	vkai.storeOp					  = VK_ATTACHMENT_STORE_OP_STORE;
	vkai.clearValue.color.float32[0]  = clearValue.r;
	vkai.clearValue.color.float32[1]  = clearValue.g;
	vkai.clearValue.color.float32[2]  = clearValue.b;
	vkai.clearValue.color.float32[3]  = clearValue.a;

	mColorAttachments.push_back( vkai );

	return *this;
}

CommandBuffer::RenderingInfo &CommandBuffer::RenderingInfo::setDepthAttachment( const vk::ImageViewRef &attachment, const vk::ImageViewRef &resolve, VkImageLayout imageLayout )
{
	mDepthAttachment.pNext						   = nullptr;
	mDepthAttachment.imageView					   = attachment->getImageViewHandle();
	mDepthAttachment.imageLayout				   = imageLayout;
	mDepthAttachment.resolveMode				   = resolve ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
	mDepthAttachment.resolveImageView			   = resolve ? resolve->getImageViewHandle() : VK_NULL_HANDLE;
	mDepthAttachment.resolveImageLayout			   = resolve ? imageLayout : VK_IMAGE_LAYOUT_UNDEFINED;
	mDepthAttachment.loadOp						   = VK_ATTACHMENT_LOAD_OP_LOAD;
	mDepthAttachment.storeOp					   = VK_ATTACHMENT_STORE_OP_STORE;
	mDepthAttachment.clearValue.depthStencil.depth = CINDER_DEFAULT_DEPTH;

	return *this;
}

CommandBuffer::RenderingInfo &CommandBuffer::RenderingInfo::setDepthAttachment( const vk::ImageViewRef &attachment, float clearValue, const vk::ImageViewRef &resolve, VkImageLayout imageLayout )
{
	mDepthAttachment.pNext						   = nullptr;
	mDepthAttachment.imageView					   = attachment->getImageViewHandle();
	mDepthAttachment.imageLayout				   = imageLayout;
	mDepthAttachment.resolveMode				   = resolve ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
	mDepthAttachment.resolveImageView			   = resolve ? resolve->getImageViewHandle() : VK_NULL_HANDLE;
	mDepthAttachment.resolveImageLayout			   = resolve ? imageLayout : VK_IMAGE_LAYOUT_UNDEFINED;
	mDepthAttachment.loadOp						   = VK_ATTACHMENT_LOAD_OP_CLEAR;
	mDepthAttachment.storeOp					   = VK_ATTACHMENT_STORE_OP_STORE;
	mDepthAttachment.clearValue.depthStencil.depth = clearValue;

	return *this;
}

CommandBuffer::RenderingInfo &CommandBuffer::RenderingInfo::setStencilAttachment( const vk::ImageViewRef &attachment, const vk::ImageViewRef &resolve, VkImageLayout imageLayout )
{
	mStencilAttachment.pNext						   = nullptr;
	mStencilAttachment.imageView					   = attachment->getImageViewHandle();
	mStencilAttachment.imageLayout					   = imageLayout;
	mStencilAttachment.resolveMode					   = resolve ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
	mStencilAttachment.resolveImageView				   = resolve ? resolve->getImageViewHandle() : VK_NULL_HANDLE;
	mStencilAttachment.resolveImageLayout			   = resolve ? imageLayout : VK_IMAGE_LAYOUT_UNDEFINED;
	mStencilAttachment.loadOp						   = VK_ATTACHMENT_LOAD_OP_LOAD;
	mStencilAttachment.storeOp						   = VK_ATTACHMENT_STORE_OP_STORE;
	mStencilAttachment.clearValue.depthStencil.stencil = CINDER_DEFAULT_STENCIL;

	return *this;
}

CommandBuffer::RenderingInfo &CommandBuffer::RenderingInfo::setStencilAttachment( const vk::ImageViewRef &attachment, uint32_t clearValue, const vk::ImageViewRef &resolve, VkImageLayout imageLayout )
{
	mStencilAttachment.pNext						   = nullptr;
	mStencilAttachment.imageView					   = attachment->getImageViewHandle();
	mStencilAttachment.imageLayout					   = imageLayout;
	mStencilAttachment.resolveMode					   = resolve ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
	mStencilAttachment.resolveImageView				   = resolve ? resolve->getImageViewHandle() : VK_NULL_HANDLE;
	mStencilAttachment.resolveImageLayout			   = resolve ? imageLayout : VK_IMAGE_LAYOUT_UNDEFINED;
	mStencilAttachment.loadOp						   = VK_ATTACHMENT_LOAD_OP_CLEAR;
	mStencilAttachment.storeOp						   = VK_ATTACHMENT_STORE_OP_STORE;
	mStencilAttachment.clearValue.depthStencil.stencil = clearValue;

	return *this;
}

CommandBuffer::RenderingInfo &CommandBuffer::RenderingInfo::setDepthStencilAttachment( const vk::ImageViewRef &attachment, const vk::ImageViewRef &resolve )
{
	setDepthAttachment( attachment, resolve, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
	setStencilAttachment( attachment, resolve, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
	return *this;
}

CommandBuffer::RenderingInfo &CommandBuffer::RenderingInfo::setDepthStencilAttachment( const vk::ImageViewRef &attachment, float depthClearValue, uint32_t stencilClearValue, const vk::ImageViewRef &resolve )
{
	setDepthAttachment( attachment, depthClearValue, resolve, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
	setStencilAttachment( attachment, stencilClearValue, resolve, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
	return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CommandBuffer

CommandBufferRef CommandBuffer::create( vk::CommandPoolRef pool, VkCommandBufferLevel level, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return CommandBufferRef( new CommandBuffer( device, pool, level ) );
}

CommandBuffer::CommandBuffer( vk::DeviceRef device, vk::CommandPoolRef pool, VkCommandBufferLevel level )
	: vk::DeviceChildObject( device ),
	  mPool( pool )
{
	if ( mCommandBufferHandle == VK_NULL_HANDLE ) {
		VkCommandBufferAllocateInfo vkai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		vkai.pNext						 = nullptr;
		vkai.commandPool				 = mPool->getCommandPoolHandle();
		vkai.level						 = level;
		vkai.commandBufferCount			 = 1;

		VkResult vkres = CI_VK_DEVICE_FN( AllocateCommandBuffers( getDeviceHandle(), &vkai, &mCommandBufferHandle ) );
		if ( vkres != VK_SUCCESS ) {
			throw VulkanFnFailedExc( "vkAllocateCommandBuffers", vkres );
		}
	}
}

CommandBuffer::CommandBuffer( vk::DeviceRef device, VkCommandBuffer commandBufferHandle )
	: vk::DeviceChildObject( device ),
	  mCommandBufferHandle( commandBufferHandle )
{
}

CommandBuffer::~CommandBuffer()
{
	if ( ( mCommandBufferHandle != VK_NULL_HANDLE ) && mPool ) {
		CI_VK_DEVICE_FN( FreeCommandBuffers( getDeviceHandle(), mPool->getCommandPoolHandle(), 1, &mCommandBufferHandle ) );
		mCommandBufferHandle = VK_NULL_HANDLE;
	}
}

void CommandBuffer::begin( VkCommandBufferUsageFlags usageFlags )
{
	VkCommandBufferBeginInfo vkbi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkbi.pNext					  = nullptr;
	vkbi.flags					  = usageFlags;
	vkbi.pInheritanceInfo		  = nullptr;

	VkResult vkres = CI_VK_DEVICE_FN( BeginCommandBuffer( getCommandBufferHandle(), &vkbi ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkBeginCommandBuffer", vkres );
	}

	mRecording = true;
}

void CommandBuffer::end()
{
	VkResult vkres = CI_VK_DEVICE_FN( EndCommandBuffer( getCommandBufferHandle() ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkEndCommandBuffer", vkres );
	}

	mRecording = false;
}

void CommandBuffer::bindDescriptorSets(
	VkPipelineBindPoint						 pipelineBindPoint,
	const vk::PipelineLayoutRef				&pipelineLayout,
	uint32_t								 firstSet,
	const std::vector<vk::DescriptorSetRef> &sets,
	uint32_t								 dynamicOffsetCount,
	const uint32_t						  *pDynamicOffsets )
{
	std::vector<VkDescriptorSet> handles;
	for ( auto &set : sets ) {
		handles.push_back( set->getDescriptorSetHandle() );
	}

	CI_VK_DEVICE_FN( CmdBindDescriptorSets(
		getCommandBufferHandle(),
		pipelineBindPoint,
		pipelineLayout->getPipelineLayoutHandle(),
		firstSet,
		countU32( handles ),
		dataPtr( handles ),
		0,
		nullptr ) );
}

void CommandBuffer::bindPipeline(
	VkPipelineBindPoint	   pipelineBindPoint,
	const vk::PipelineRef &pipeline )
{
	CI_VK_DEVICE_FN( CmdBindPipeline(
		getCommandBufferHandle(),
		pipelineBindPoint,
		pipeline->getPipelineHandle() ) );
}

void CommandBuffer::beginRendering( const RenderingInfo &renderingInfo )
{
	if ( mRendering ) {
		endRendering();
	}

	VkRenderingInfoKHR vkri	  = { VK_STRUCTURE_TYPE_RENDERING_INFO_KHR };
	vkri.pNext				  = nullptr;
	vkri.flags				  = 0;
	vkri.renderArea			  = renderingInfo.mRenderArea;
	vkri.layerCount			  = 1;
	vkri.viewMask			  = 0;
	vkri.colorAttachmentCount = countU32( renderingInfo.mColorAttachments );
	vkri.pColorAttachments	  = dataPtr( renderingInfo.mColorAttachments );
	vkri.pDepthAttachment	  = ( renderingInfo.mDepthAttachment.imageView != VK_NULL_HANDLE ) ? &renderingInfo.mDepthAttachment : nullptr;
	vkri.pStencilAttachment	  = ( renderingInfo.mStencilAttachment.imageView != VK_NULL_HANDLE ) ? &renderingInfo.mStencilAttachment : nullptr;

	CI_VK_DEVICE_FN( CmdBeginRenderingKHR( getCommandBufferHandle(), &vkri ) );

	mRendering = true;
}

void CommandBuffer::endRendering()
{
	CI_VK_DEVICE_FN( CmdEndRenderingKHR( getCommandBufferHandle() ) );
	mRendering = false;
}

void CommandBuffer::setScissor( int32_t x, int32_t y, uint32_t width, uint32_t height )
{
	VkRect2D rect = { { x, y }, { width, height } };
	CI_VK_DEVICE_FN( CmdSetScissor( getCommandBufferHandle(), 0, 1, &rect ) );
}

void CommandBuffer::setViewport( float x, float y, float width, float height, float minDepth, float maxDepth )
{
	VkViewport viewport = {};
	//*
	viewport.x			= x;
	viewport.y			= height;
	viewport.width		= width;
	viewport.height		= -height;
	viewport.minDepth	= minDepth;
	viewport.maxDepth	= maxDepth;
	/*/
		viewport.x			= x;
		viewport.y			= y;
		viewport.width		= width;
		viewport.height		= height;
		viewport.minDepth	= minDepth;
		viewport.maxDepth	= maxDepth;
	//*/
	CI_VK_DEVICE_FN( CmdSetViewport( getCommandBufferHandle(), 0, 1, &viewport ) );
}

void CommandBuffer::clearColorAttachment( uint32_t index, const VkClearColorValue &clearValue, const VkRect2D &rect )
{
	VkClearAttachment attachment = {};
	attachment.aspectMask		 = VK_IMAGE_ASPECT_COLOR_BIT;
	attachment.colorAttachment	 = index;
	attachment.clearValue.color	 = clearValue;

	VkClearRect clearRect	 = {};
	clearRect.rect			 = rect;
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount	 = 1;

	CI_VK_DEVICE_FN( CmdClearAttachments( getCommandBufferHandle(), 1, &attachment, 1, &clearRect ) );
}

void CommandBuffer::clearDepthStencilAttachment( float depthClearValue, uint32_t stencilClearValue, const VkRect2D &rect, VkImageAspectFlags aspectMask )
{
	aspectMask = ( aspectMask & ( VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT ) );

	VkClearAttachment attachment			   = {};
	attachment.aspectMask					   = aspectMask;
	attachment.colorAttachment				   = {};
	attachment.clearValue.depthStencil.depth   = depthClearValue;
	attachment.clearValue.depthStencil.stencil = stencilClearValue;

	VkClearRect clearRect	 = {};
	clearRect.rect			 = rect;
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount	 = 1;

	CI_VK_DEVICE_FN( CmdClearAttachments( getCommandBufferHandle(), 1, &attachment, 1, &clearRect ) );
}

void CommandBuffer::clearDepthAttachment( float clearValue, const VkRect2D &rect )
{
	clearDepthStencilAttachment( clearValue, CINDER_DEFAULT_STENCIL, rect, VK_IMAGE_ASPECT_DEPTH_BIT );
}

void CommandBuffer::clearStencilAttachment( uint32_t clearValue, const VkRect2D &rect )
{
	clearDepthStencilAttachment( CINDER_DEFAULT_STENCIL, clearValue, rect, VK_IMAGE_ASPECT_STENCIL_BIT );
}

void CommandBuffer::bindIndexBuffer( vk::BufferRef &buffer, uint32_t offset, VkIndexType indexType )
{
	CI_VK_DEVICE_FN( CmdBindIndexBuffer(
		getCommandBufferHandle(),
		buffer->getBufferHandle(),
		offset,
		indexType ) );
}

void CommandBuffer::bindVertexBuffers( uint32_t firstBinding, const std::vector<vk::BufferRef> &buffers, std::vector<uint64_t> offsets )
{
	bool insertOffsets = offsets.empty() ? true : false;

	std::vector<VkBuffer> handles;
	for ( auto &buffer : buffers ) {
		handles.push_back( buffer->getBufferHandle() );
		if ( insertOffsets ) {
			offsets.push_back( 0 );
		}
	}

	CI_VK_DEVICE_FN( CmdBindVertexBuffers(
		getCommandBufferHandle(),
		firstBinding,
		countU32( handles ),
		dataPtr( handles ),
		dataPtr( offsets ) ) );
}

void CommandBuffer::setCullMode( VkCullModeFlags cullMode )
{
	CI_VK_DEVICE_FN( CmdSetCullModeEXT( getCommandBufferHandle(), cullMode ) );
}

void CommandBuffer::setDepthTestEnable( bool depthTestEnable )
{
	CI_VK_DEVICE_FN( CmdSetDepthTestEnableEXT( getCommandBufferHandle(), depthTestEnable ) );
}

void CommandBuffer::setDepthWriteEnable( bool depthWriteEnable )
{
	CI_VK_DEVICE_FN( CmdSetDepthWriteEnableEXT( getCommandBufferHandle(), depthWriteEnable ) );
}

void CommandBuffer::setFrontFace( VkFrontFace frontFace )
{
	CI_VK_DEVICE_FN( CmdSetFrontFaceEXT( getCommandBufferHandle(), frontFace ) );
}

void CommandBuffer::draw( uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance )
{
	CI_VK_DEVICE_FN( CmdDraw( getCommandBufferHandle(), vertexCount, instanceCount, firstVertex, firstInstance ) );
}

void CommandBuffer::drawIndexed( uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance )
{
	CI_VK_DEVICE_FN( CmdDrawIndexed( getCommandBufferHandle(), indexCount, instanceCount, firstIndex, vertexOffset, firstInstance ) );
}

void CommandBuffer::transitionImageLayout(
	VkImage				 image,
	VkImageAspectFlags	 aspectMask,
	uint32_t			 baseMipLevel,
	uint32_t			 levelCount,
	uint32_t			 baseArrayLayer,
	uint32_t			 layerCount,
	VkImageLayout		 oldLayout,
	VkImageLayout		 newLayout,
	VkPipelineStageFlags newPipelineStageFlags )
{
}

void CommandBuffer::transitionImageLayout(
	ImageRef			 image,
	VkImageLayout		 oldLayout,
	VkImageLayout		 newLayout,
	VkPipelineStageFlags newPipelineStageFlags )
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CommandPool

CommandPoolRef CommandPool::create( uint32_t queueFamilyIndex, const Options &options, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return CommandPoolRef( new CommandPool( device, queueFamilyIndex, options ) );
}

CommandPool::CommandPool( vk::DeviceRef device, uint32_t queueFamilyIndex, const Options &options )
	: vk::DeviceChildObject( device ),
	  mQueueFamilyIndex( queueFamilyIndex )
{
	VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	poolCreateInfo.pNext				   = nullptr;
	poolCreateInfo.flags				   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolCreateInfo.queueFamilyIndex		   = mQueueFamilyIndex;

	VkResult vkres = CI_VK_DEVICE_FN( CreateCommandPool( getDeviceHandle(), &poolCreateInfo, nullptr, &mCommandPoolHandle ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkCreateCommandPool", vkres );
	}
}

CommandPool::~CommandPool()
{
	if ( mCommandPoolHandle != VK_NULL_HANDLE ) {
		CI_VK_DEVICE_FN( DestroyCommandPool( getDeviceHandle(), mCommandPoolHandle, nullptr ) );
		mCommandPoolHandle = VK_NULL_HANDLE;
	}
}

std::vector<vk::CommandBufferRef> CommandPool::allocateCommandBuffers( uint32_t count, VkCommandBufferLevel level )
{
	std::vector<vk::CommandBufferRef> commandBuffers;

	if ( count > 0 ) {
		VkCommandBufferAllocateInfo vkai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		vkai.pNext						 = nullptr;
		vkai.commandPool				 = mCommandPoolHandle;
		vkai.level						 = level;
		vkai.commandBufferCount			 = count;

		std::vector<VkCommandBuffer> commandBufferHandles( count );

		VkResult vkres = CI_VK_DEVICE_FN( AllocateCommandBuffers( getDeviceHandle(), &vkai, dataPtr( commandBufferHandles ) ) );
		if ( vkres != VK_SUCCESS ) {
			throw VulkanFnFailedExc( "vkAllocateCommandBuffers", vkres );
		}

		for ( uint32_t i = 0; i < count; ++i ) {
			commandBuffers.push_back( vk::CommandBufferRef( new vk::CommandBuffer( getDevice(), commandBufferHandles[i] ) ) );
		}
	}

	return commandBuffers;
}

} // namespace cinder::vk
