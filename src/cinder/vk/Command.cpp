#include "cinder/vk/Command.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Image.h"
#include "cinder/app/RendererVk.h"

namespace cinder::vk {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CommandBuffer::RenderingInfo

CommandBuffer::RenderingInfo::RenderingInfo( const vk::ImageView *pColorAttachment, const vk::ImageView *pDepthStencilAttachment )
{
	if ( pColorAttachment != nullptr ) {
		renderArea( pColorAttachment->getImageArea() );
	}
	else if ( pDepthStencilAttachment != nullptr ) {
		renderArea( pDepthStencilAttachment->getImageArea() );
	}
}

CommandBuffer::RenderingInfo::RenderingInfo( const std::vector<const vk::ImageView *> colorAttachments, const vk::ImageView *pDepthStencilAttachment )
{
	if ( !colorAttachments.empty() ) {
		renderArea( colorAttachments[0]->getImageArea() );
	}
	else if ( pDepthStencilAttachment != nullptr ) {
		renderArea( pDepthStencilAttachment->getImageArea() );
	}

	for ( const auto &attachment : colorAttachments ) {
		addColorAttachment( attachment );
	}

	if ( pDepthStencilAttachment != nullptr ) {
		setDepthStencilAttachment( pDepthStencilAttachment );
	}
}

CommandBuffer::RenderingInfo::RenderingInfo( const std::vector<const vk::ImageView *> colorAttachments, const vk::ImageView *pDepthAttachment, const vk::ImageView *pStencilAttachment )
{
	if ( !colorAttachments.empty() ) {
		renderArea( colorAttachments[0]->getImageArea() );
	}
	else if ( pDepthAttachment != nullptr ) {
		renderArea( pDepthAttachment->getImageArea() );
	}
	else if ( pStencilAttachment != nullptr ) {
		renderArea( pStencilAttachment->getImageArea() );
	}

	for ( const auto &attachment : colorAttachments ) {
		addColorAttachment( attachment );
	}

	if ( pDepthAttachment != nullptr ) {
		setDepthAttachment( pDepthAttachment );
	}

	if ( pStencilAttachment != nullptr ) {
		setStencilAttachment( pDepthAttachment );
	}
}

CommandBuffer::RenderingInfo &CommandBuffer::RenderingInfo::addColorAttachment( const vk::ImageView *pAttachment, const vk::ImageView *pResolve )
{
	VkRenderingAttachmentInfoKHR vkai = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
	vkai.pNext						  = nullptr;
	vkai.imageView					  = pAttachment->getImageViewHandle();
	vkai.imageLayout				  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	vkai.resolveMode				  = ( pResolve != nullptr ) ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
	vkai.resolveImageView			  = ( pResolve != nullptr ) ? pResolve->getImageViewHandle() : VK_NULL_HANDLE;
	vkai.resolveImageLayout			  = ( pResolve != nullptr ) ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
	vkai.loadOp						  = VK_ATTACHMENT_LOAD_OP_LOAD;
	vkai.storeOp					  = VK_ATTACHMENT_STORE_OP_STORE;
	vkai.clearValue.color.float32[0]  = 0;
	vkai.clearValue.color.float32[1]  = 0;
	vkai.clearValue.color.float32[2]  = 0;
	vkai.clearValue.color.float32[3]  = 0;

	mColorAttachments.push_back( vkai );

	return *this;
}

CommandBuffer::RenderingInfo &CommandBuffer::RenderingInfo::addColorAttachment( const vk::ImageView *pAttachment, const ColorA &clearValue, const vk::ImageView *pResolve )
{
	VkRenderingAttachmentInfoKHR vkai = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
	vkai.pNext						  = nullptr;
	vkai.imageView					  = pAttachment->getImageViewHandle();
	vkai.imageLayout				  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	vkai.resolveMode				  = ( pResolve != nullptr ) ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
	vkai.resolveImageView			  = ( pResolve != nullptr ) ? pResolve->getImageViewHandle() : VK_NULL_HANDLE;
	vkai.resolveImageLayout			  = ( pResolve != nullptr ) ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
	vkai.loadOp						  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	vkai.storeOp					  = VK_ATTACHMENT_STORE_OP_STORE;
	vkai.clearValue.color.float32[0]  = clearValue.r;
	vkai.clearValue.color.float32[1]  = clearValue.g;
	vkai.clearValue.color.float32[2]  = clearValue.b;
	vkai.clearValue.color.float32[3]  = clearValue.a;

	mColorAttachments.push_back( vkai );

	return *this;
}

CommandBuffer::RenderingInfo &CommandBuffer::RenderingInfo::setDepthAttachment( const vk::ImageView *pAttachment, const vk::ImageView *pResolve, VkImageLayout imageLayout )
{
	mDepthAttachment.pNext						   = nullptr;
	mDepthAttachment.imageView					   = pAttachment->getImageViewHandle();
	mDepthAttachment.imageLayout				   = imageLayout;
	mDepthAttachment.resolveMode				   = ( pResolve != nullptr ) ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
	mDepthAttachment.resolveImageView			   = ( pResolve != nullptr ) ? pResolve->getImageViewHandle() : VK_NULL_HANDLE;
	mDepthAttachment.resolveImageLayout			   = ( pResolve != nullptr ) ? imageLayout : VK_IMAGE_LAYOUT_UNDEFINED;
	mDepthAttachment.loadOp						   = VK_ATTACHMENT_LOAD_OP_LOAD;
	mDepthAttachment.storeOp					   = VK_ATTACHMENT_STORE_OP_STORE;
	mDepthAttachment.clearValue.depthStencil.depth = CINDER_DEFAULT_DEPTH;

	return *this;
}

CommandBuffer::RenderingInfo &CommandBuffer::RenderingInfo::setDepthAttachment( const vk::ImageView *pAttachment, float clearValue, const vk::ImageView *pResolve, VkImageLayout imageLayout )
{
	mDepthAttachment.pNext						   = nullptr;
	mDepthAttachment.imageView					   = pAttachment->getImageViewHandle();
	mDepthAttachment.imageLayout				   = imageLayout;
	mDepthAttachment.resolveMode				   = ( pResolve != nullptr ) ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
	mDepthAttachment.resolveImageView			   = ( pResolve != nullptr ) ? pResolve->getImageViewHandle() : VK_NULL_HANDLE;
	mDepthAttachment.resolveImageLayout			   = ( pResolve != nullptr ) ? imageLayout : VK_IMAGE_LAYOUT_UNDEFINED;
	mDepthAttachment.loadOp						   = VK_ATTACHMENT_LOAD_OP_CLEAR;
	mDepthAttachment.storeOp					   = VK_ATTACHMENT_STORE_OP_STORE;
	mDepthAttachment.clearValue.depthStencil.depth = CINDER_DEFAULT_DEPTH;

	return *this;
}

CommandBuffer::RenderingInfo &CommandBuffer::RenderingInfo::setStencilAttachment( const vk::ImageView *pAttachment, const vk::ImageView *pResolve, VkImageLayout imageLayout )
{
	mDepthAttachment.pNext							 = nullptr;
	mDepthAttachment.imageView						 = pAttachment->getImageViewHandle();
	mDepthAttachment.imageLayout					 = imageLayout;
	mDepthAttachment.resolveMode					 = ( pResolve != nullptr ) ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
	mDepthAttachment.resolveImageView				 = ( pResolve != nullptr ) ? pResolve->getImageViewHandle() : VK_NULL_HANDLE;
	mDepthAttachment.resolveImageLayout				 = ( pResolve != nullptr ) ? imageLayout : VK_IMAGE_LAYOUT_UNDEFINED;
	mDepthAttachment.loadOp							 = VK_ATTACHMENT_LOAD_OP_LOAD;
	mDepthAttachment.storeOp						 = VK_ATTACHMENT_STORE_OP_STORE;
	mDepthAttachment.clearValue.depthStencil.stencil = CINDER_DEFAULT_STENCIL;

	mCombinedDepthStencil = false;

	return *this;
}

CommandBuffer::RenderingInfo &CommandBuffer::RenderingInfo::setStencilAttachment( const vk::ImageView *pAttachment, uint32_t clearValue, const vk::ImageView *pResolve, VkImageLayout imageLayout )
{
	mDepthAttachment.pNext							 = nullptr;
	mDepthAttachment.imageView						 = pAttachment->getImageViewHandle();
	mDepthAttachment.imageLayout					 = imageLayout;
	mDepthAttachment.resolveMode					 = ( pResolve != nullptr ) ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
	mDepthAttachment.resolveImageView				 = ( pResolve != nullptr ) ? pResolve->getImageViewHandle() : VK_NULL_HANDLE;
	mDepthAttachment.resolveImageLayout				 = ( pResolve != nullptr ) ? imageLayout : VK_IMAGE_LAYOUT_UNDEFINED;
	mDepthAttachment.loadOp							 = VK_ATTACHMENT_LOAD_OP_CLEAR;
	mDepthAttachment.storeOp						 = VK_ATTACHMENT_STORE_OP_STORE;
	mDepthAttachment.clearValue.depthStencil.stencil = CINDER_DEFAULT_STENCIL;

	mCombinedDepthStencil = false;

	return *this;
}

CommandBuffer::RenderingInfo &CommandBuffer::RenderingInfo::setDepthStencilAttachment( const vk::ImageView *pAttachment, const vk::ImageView *pResolve )
{
	setDepthAttachment( pAttachment, pResolve, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
	mStencilAttachment	  = {};
	mCombinedDepthStencil = true;
	return *this;
}

CommandBuffer::RenderingInfo &CommandBuffer::RenderingInfo::setDepthStencilAttachment( const vk::ImageView *pAttachment, float depthClearValue, uint32_t stencilClearValue, const vk::ImageView *pResolve )
{
	setDepthAttachment( pAttachment, depthClearValue, pResolve, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
	mStencilAttachment	  = {};
	mCombinedDepthStencil = true;
	return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CommandBuffer

CommandBufferRef CommandBuffer::create( VkCommandPool poolHandle, VkCommandBufferLevel level, vk::DeviceRef device )
{
	return CommandBufferRef();
}

CommandBuffer::CommandBuffer( vk::DeviceRef device, VkCommandPool poolHandle, VkCommandBufferLevel level, VkCommandBuffer commandBufferHandle, bool disposeCommandBuffer )
	: vk::DeviceChildObject( device ),
	  mPoolHandle( poolHandle ),
	  mCommandBufferHandle( commandBufferHandle ),
	  mDisposeCommandBuffer( disposeCommandBuffer )
{
	if ( mCommandBufferHandle == VK_NULL_HANDLE ) {
		VkCommandBufferAllocateInfo vkai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		vkai.pNext						 = nullptr;
		vkai.commandPool				 = mPoolHandle;
		vkai.level						 = level;
		vkai.commandBufferCount			 = 1;

		VkResult vkres = CI_VK_DEVICE_FN( AllocateCommandBuffers( getDeviceHandle(), &vkai, &mCommandBufferHandle ) );
		if ( vkres != VK_SUCCESS ) {
			throw VulkanFnFailedExc( "vkAllocateCommandBuffers", vkres );
		}

		mDisposeCommandBuffer = true;
	}
}

CommandBuffer::~CommandBuffer()
{
	if ( ( mCommandBufferHandle != VK_NULL_HANDLE ) && mDisposeCommandBuffer ) {
		CI_VK_DEVICE_FN( FreeCommandBuffers( getDeviceHandle(), mPoolHandle, 1, &mCommandBufferHandle ) );
		mCommandBufferHandle  = VK_NULL_HANDLE;
		mDisposeCommandBuffer = false;
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

void CommandBuffer::beginRendering( const RenderingInfo &ri )
{
	if ( mRenderingActive ) {
		endRendering();
	}

	VkRenderingInfoKHR vkri	  = { VK_STRUCTURE_TYPE_RENDERING_INFO_KHR };
	vkri.pNext				  = nullptr;
	vkri.flags				  = 0;
	vkri.renderArea			  = ri.mRenderArea;
	vkri.layerCount			  = 1;
	vkri.viewMask			  = 0;
	vkri.colorAttachmentCount = countU32( ri.mColorAttachments );
	vkri.pColorAttachments	  = dataPtr( ri.mColorAttachments );
	vkri.pDepthAttachment	  = ( ri.mDepthAttachment.imageView != VK_NULL_HANDLE ) ? &ri.mDepthAttachment : nullptr;
	vkri.pStencilAttachment	  = ( ri.mStencilAttachment.imageView != VK_NULL_HANDLE ) ? &ri.mStencilAttachment : nullptr;

	CI_VK_DEVICE_FN( CmdBeginRenderingKHR( getCommandBufferHandle(), &vkri ) );

	mRenderingActive = true;
}

void CommandBuffer::endRendering()
{
	CI_VK_DEVICE_FN( CmdEndRenderingKHR( getCommandBufferHandle() ) );
	mRenderingActive = false;
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
			commandBuffers.push_back( vk::CommandBufferRef( new vk::CommandBuffer( getDevice(), mCommandPoolHandle, level, commandBufferHandles[i], true ) ) );
		}
	}

	return commandBuffers;
}

} // namespace cinder::vk
