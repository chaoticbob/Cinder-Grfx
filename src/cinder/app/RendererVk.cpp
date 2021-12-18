#include "cinder/app/RendererVk.h"
#include "cinder/app/AppBase.h"
#include "cinder/vk/Command.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Environment.h"
#include "cinder/vk/Image.h"
#include "cinder/vk/RenderPass.h"
#include "cinder/vk/Swapchain.h"
#include "cinder/vk/Sync.h"
#include "cinder/vk/Util.h"

#if defined( CINDER_MSW_DESKTOP )
#include "cinder/app/msw/AppImplMsw.h"
#endif

namespace cinder::app {

static RendererVk *		 sMainRenderer				  = nullptr;
thread_local RendererVk *RendererVk::sCurrentRenderer = nullptr;

struct RendererVk::Frame
{
	//vk::FenceRef			  frameWorkComplete;
	//VkCommandPool			  commandPool;
	//vk::CommandBufferRef	  contextCommandBuffer;
	//vk::CommandBufferRef	  presentCommandBuffer;
	//vk::ImageRef			  contextRenderTarget;
	//vk::ImageRef			  contextDepthStencil;
	//vk::BufferedRenderPassRef contextRenderPass;
	//vk::SemaphoreRef		  contextWorkComplete;
	//vk::SemaphoreRef		  presentReady;

	//vk::ImageRef swapchainImage;
};

RendererVk::RendererVk( const RendererVk &renderer )
	: mOptions( renderer.mOptions ),
	  mDevice( renderer.mDevice )
{
}

RendererVk::RendererVk( const Options &options )
	: mOptions( options )
{
}

RendererVk::~RendererVk()
{
}

RendererVk *RendererVk::getCurrentRenderer()
{
	return sCurrentRenderer;
}

vk::DeviceRef RendererVk::getDevice() const
{
	return mDevice;
}

vk::SwapchainRef RendererVk::getSwapchain() const
{
	return mSwapchain;
}

RendererRef RendererVk::clone() const
{
	return RendererVkRef( new RendererVk( *this ) );
}

void RendererVk::setupDevice( const std::string &appName, RendererRef sharedRenderer )
{
	// Safe to call this multiple times
	vk::Environment::initialize( appName, mOptions );

	// Shared Vulkan renderers will share a logcal Vulkan device
	if ( sharedRenderer ) {
		RendererVkRef sharedRendererVk = std::dynamic_pointer_cast<RendererVk>( sharedRenderer );
		if ( !sharedRendererVk ) {
			throw ExcRendererAllocation( "sharedRenderer is not a RendererVk object." );
		}
		this->mDevice = sharedRendererVk->mDevice;
	}
	else {
		// Create a device using the GPU index
		VkPhysicalDevice gpuHandle = vk::Environment::get()->getGpuHandle( mOptions.getGpuIndex() );
		if ( gpuHandle == VK_NULL_HANDLE ) {
			throw grfx::GraphicsApiExc( "failed to find GPU using index" );
		}

		vk::Device::Options options = vk::Device::Options()
										  .addExtension( mOptions.getDeviceExtensions() )
										  .enableComputeQueue( mOptions.getEnableComputeQueue() )
										  .enableTransferQueue( mOptions.getEnableTransferQueue() );
		mDevice = vk::Device::create( gpuHandle, options );

		// Readjust MSAA samples so app won't fail because they all use 16
		{
			//uint32_t msaa = mOptions.getMsaa();
			//msaa		  = ( msaa == 0 ) ? 1 : msaa;
			//msaa		  = ( msaa > 2 ) ? grfx::roundUpPow2( msaa ) : msaa;
			//
			//VkSampleCountFlagBits msaaSamples = static_cast<VkSampleCountFlagBits>( msaa );

			uint32_t			  msaa		  = mOptions.getMsaa();
			VkSampleCountFlagBits msaaSamples = vk::toVkSampleCount( msaa );
			VkSampleCountFlagBits maxSamples  = mDevice->getMaxOutputSampleCount();
			if ( msaaSamples > maxSamples ) {
				app::console() << "Readjusting MSAA from " << msaa << " to " << maxSamples << " to meet device requirements" << std::endl;
				msaa = static_cast<uint32_t>( maxSamples );
				mOptions.msaa( msaa );
			}
		}
	}
}

void RendererVk::setupFrames( uint32_t windowWidth, uint32_t windowHeight )
{
	VkFormat			  swapchainFormat = mSwapchain->getSurfaceFormat().format;
	VkSampleCountFlagBits samples		  = static_cast<VkSampleCountFlagBits>( mOptions.getMsaa() );

	/*
	uint32_t numFrames = getOptions().getNumFramesInFlight();
	for ( uint32_t i = 0; i < numFrames; ++i ) {
		Frame frame = {};

		// Work complete fence
		frame.frameWorkComplete = vk::Fence::create( true, getDevice() );

		// Command pool and command buffers
		{
			VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
			poolCreateInfo.pNext				   = nullptr;
			poolCreateInfo.flags				   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			poolCreateInfo.queueFamilyIndex		   = mDevice->getQueueFamilyIndices().graphics;

			VkResult vkres = mDevice->vkfn()->CreateCommandPool(
				mDevice->getDeviceHandle(),
				&poolCreateInfo,
				nullptr,
				&frame.commandPool );
			if ( vkres != VK_SUCCESS ) {
				throw vk::VulkanFnFailedExc( "vkCreateCommandPool", vkres );
			}

			frame.contextCommandBuffer = vk::CommandBuffer::create( frame.commandPool, mDevice );
			frame.presentCommandBuffer = vk::CommandBuffer::create( frame.commandPool, mDevice );
		}

		// Render target
		{
			vk::Image::Usage imageUsage = vk::Image::Usage().sampledImage().renderTarget();

			vk::Image::Options imageOptions = vk::Image::Options().samples( samples );

			frame.contextRenderTarget = vk::Image::create(
				VK_IMAGE_TYPE_2D,
				{ windowWidth, windowHeight, 1 },
				swapchainFormat,
				imageUsage,
				vk::MemoryUsage::GPU_ONLY,
				imageOptions,
				mDevice );

			mDevice->transitionImageLayout(
				frame.contextRenderTarget->getImageHandle(),
				frame.contextRenderTarget->getAspectMask(),
				CINDER_VK_ALL_SUB_RESOURCES,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				vk::guessPipelineStageFromImageLayout( mDevice, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ) );
		}

		// Depth stencil
		{
			vk::Image::Usage imageUsage = vk::Image::Usage().sampledImage().depthStencil();

			vk::Image::Options imageOptions = vk::Image::Options();

			frame.contextDepthStencil = vk::Image::create(
				VK_IMAGE_TYPE_2D,
				{ windowWidth, windowHeight, 1 },
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				imageUsage,
				vk::MemoryUsage::GPU_ONLY,
				imageOptions,
				mDevice );

			mDevice->transitionImageLayout(
				frame.contextDepthStencil->getImageHandle(),
				frame.contextDepthStencil->getAspectMask(),
				CINDER_VK_ALL_SUB_RESOURCES,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				vk::guessPipelineStageFromImageLayout( mDevice, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ) );
		}

		// Render pass
		{
			vk::BufferedRenderPass::ImageDesc desc = vk::BufferedRenderPass::ImageDesc()
														 .addRenderTarget( frame.contextRenderTarget )
														 .setDepthStencil( frame.contextDepthStencil );
			frame.contextRenderPass = vk::BufferedRenderPass::create(
				windowWidth,
				windowHeight,
				desc,
				mDevice );
		}

		// Semaphores
		frame.contextWorkComplete = vk::Semaphore::create( VK_SEMAPHORE_TYPE_BINARY, getDevice() );
		frame.presentReady		  = vk::Semaphore::create( VK_SEMAPHORE_TYPE_BINARY, getDevice() );

		mFrames.push_back( frame );
	}
	*/
}

#if defined( CINDER_MSW_DESKTOP )
void RendererVk::setup( WindowImplMsw *windowImpl, RendererRef sharedRenderer )
{
	setupDevice( windowImpl->getTitle(), sharedRenderer );

	// Setup swapchain for window
	{
		vk::Swapchain::Options options = vk::Swapchain::Options();

		mSwapchain = vk::Swapchain::create( windowImpl, options, getDevice() );
	}

	// Create context
	{
		vk::Context::Options options = vk::Context::Options();

		mContext = vk::Context::create(
			static_cast<uint32_t>( windowImpl->getSize().x ),
			static_cast<uint32_t>( windowImpl->getSize().y ),
			options,
			mDevice );
	}

	// Setup frames
	setupFrames( windowImpl->getSize().x, windowImpl->getSize().y );
}

#elif defined( CINDER_LINUX )
#if defined( CINDER_HEADLESS )
void RendererVk::setup( ci::ivec2 renderSize, RendererRef sharedRenderer )
{
}
#else
void RendererVk::setup( void *nativeWindow, RendererRef sharedRenderer )
{
}
#endif
#endif

void RendererVk::kill()
{
	mDevice->waitIdle();
	mSwapchain.reset();
	mDevice.reset();
}

const RendererVk::Options &RendererVk::getOptions() const
{
	return mOptions;
}

void RendererVk::startDraw()
{
	if ( mStartDrawFn ) {
		mStartDrawFn( this );
	}
	else {
		this->makeCurrentContext();
	}

	/*
	uint32_t frameIndex = mNumPreseents % mOptions.getNumFramesInFlight();
	mCurrentFrame		= &mFrames[frameIndex];

	VkResult vkres = mCurrentFrame->frameWorkComplete->waitAndReset();
	if ( vkres < VK_SUCCESS ) {
		throw vk::VulkanFnFailedExc( "wait and reset fence", vkres );
	}
	*/
}

void RendererVk::finishDraw()
{
	if ( mFinishDrawFn ) {
		mFinishDrawFn( this );
	}
	else {
		this->swapBuffers();
	}
}

void RendererVk::defaultResize()
{
}

void RendererVk::makeCurrentContext( bool force )
{
	(void)force;
	sCurrentRenderer = this;

	mContext->makeCurrent();
}

void RendererVk::swapBuffers()
{
	mContext->submit();

	/*
	// Process context commands
	{
		mCurrentFrame->contextCommandBuffer->begin();
		{
			mCurrentFrame->contextCommandBuffer->beginRenderPass( mCurrentFrame->contextRenderPass );

			const VkRect2D &renderArea = mCurrentFrame->contextRenderPass->getRenderArea();
			mCurrentFrame->contextCommandBuffer->clearRenderTarget( 0, { 1, 0, 0, 1 }, renderArea );
			mCurrentFrame->contextCommandBuffer->clearDepthStencil( { 1.0f, 0xFF }, renderArea );

			mCurrentFrame->contextCommandBuffer->endRenderPass();
		}
		mCurrentFrame->contextCommandBuffer->end();

		VkCommandBuffer commandBufferHandle	  = mCurrentFrame->contextCommandBuffer->getCommandBufferHandle();
		VkSemaphore		signalSemaphoreHandle = mCurrentFrame->contextWorkComplete->getSemaphoreHandle();

		VkSubmitInfo submitInfo			= { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.pNext				= nullptr;
		submitInfo.waitSemaphoreCount	= 0;
		submitInfo.pWaitSemaphores		= nullptr; //&waitSemaphoreHandle;
		submitInfo.pWaitDstStageMask	= nullptr; //&waitDstStageMask;
		submitInfo.commandBufferCount	= 1;
		submitInfo.pCommandBuffers		= &commandBufferHandle;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores	= &signalSemaphoreHandle;

		VkResult vkres = getDevice()->submitGraphics( &submitInfo );
		if ( vkres != VK_SUCCESS ) {
			throw vk::VulkanExc( "context work submit failed" );
		}
	}

	// Acquire next swapchain image
	vk::Swapchain::AcquireInfo acquireInfo = {};

	VkResult vkres = getSwapchain()->acquireNextImage( UINT64_MAX, &acquireInfo );
	if ( vkres != VK_SUCCESS ) {
		// An error actually happened
		if ( vkres < VK_SUCCESS ) {
			throw vk::VulkanFnFailedExc( "acquire next image wrapper", vkres );
		}
	}

	// Copy context render target to swapchain
	{
		mCurrentFrame->presentCommandBuffer->begin();
		{
			VkImageAspectFlags srcAspectMask = mCurrentFrame->contextRenderTarget->getAspectMask();
			VkImageAspectFlags dstAspectMask = acquireInfo.image->getAspectMask();

			if ( mOptions.getMsaa() > 1 ) {
				VkImageResolve region = {};
				region.srcSubresource = { srcAspectMask, 0, 0, 1 };
				region.srcOffset	  = {};
				region.dstSubresource = { dstAspectMask, 0, 0, 1 };
				region.dstOffset	  = {};
				region.extent		  = mCurrentFrame->contextRenderTarget->getExtent();

				mDevice->vkfn()->CmdResolveImage(
					mCurrentFrame->presentCommandBuffer->getCommandBufferHandle(),
					mCurrentFrame->contextRenderTarget->getImageHandle(),
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					acquireInfo.image->getImageHandle(),
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&region );
			}
			else {
				VkImageCopy region	  = {};
				region.srcSubresource = { srcAspectMask, 0, 0, 1 };
				region.srcOffset	  = {};
				region.dstSubresource = { dstAspectMask, 0, 0, 1 };
				region.dstOffset	  = {};
				region.extent		  = mCurrentFrame->contextRenderTarget->getExtent();

				mDevice->vkfn()->CmdCopyImage(
					mCurrentFrame->presentCommandBuffer->getCommandBufferHandle(),
					mCurrentFrame->contextRenderTarget->getImageHandle(),
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					acquireInfo.image->getImageHandle(),
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&region );
			}
		}
		mCurrentFrame->presentCommandBuffer->end();

		VkSemaphore			 waitSemaphoreHandles[2] = { acquireInfo.imageReady->getSemaphoreHandle(), mCurrentFrame->contextWorkComplete->getSemaphoreHandle() };
		VkPipelineStageFlags waitDstStageMasks[2]	 = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
		VkCommandBuffer		 commandBufferHandle	 = mCurrentFrame->presentCommandBuffer->getCommandBufferHandle();
		VkSemaphore			 signalSemaphoreHandle	 = mCurrentFrame->presentReady->getSemaphoreHandle();

		VkSubmitInfo submitInfo			= { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.pNext				= nullptr;
		submitInfo.waitSemaphoreCount	= 2;
		submitInfo.pWaitSemaphores		= waitSemaphoreHandles;
		submitInfo.pWaitDstStageMask	= waitDstStageMasks;
		submitInfo.commandBufferCount	= 1;
		submitInfo.pCommandBuffers		= &commandBufferHandle;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores	= &signalSemaphoreHandle;

		vkres = getDevice()->submitGraphics( &submitInfo, mCurrentFrame->frameWorkComplete->getFenceHandle() );
		if ( vkres != VK_SUCCESS ) {
			throw vk::VulkanExc( "present work submit failed" );
		}
	}

	vk::Swapchain::PresentInfo presentInfo = {};
	presentInfo.imageIndex				   = acquireInfo.imageIndex;
	presentInfo.presentReady			   = mCurrentFrame->presentReady;

	vkres = getSwapchain()->present( &presentInfo );
	if ( vkres != VK_SUCCESS ) {
		// An error actually happened
		if ( vkres < VK_SUCCESS ) {
			throw vk::VulkanFnFailedExc( "swapchain present wrapper", vkres );
		}
	}

	// Increment present counter
	++mNumPreseents;

	// No work on this frame after this
	mCurrentFrame = nullptr;
*/
}

Surface8u RendererVk::copyWindowSurface( const Area &area, int32_t windowHeightPixels )
{
	return Surface8u();
}

} // namespace cinder::app
