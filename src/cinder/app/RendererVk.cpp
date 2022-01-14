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
#include "cinder/vk/wrapper.h"

#if defined( CINDER_MSW_DESKTOP )
#include "cinder/app/msw/AppImplMsw.h"
#endif

namespace cinder::app {

static RendererVk *sMainRenderer = nullptr;

thread_local RendererVk *RendererVk::sCurrentRenderer = nullptr;

struct RendererVk::Frame
{
	vk::CommandBufferRef commandBuffer;
	vk::SemaphoreRef	 presentReady;
	uint64_t			 signaledValue = 0;
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

vk::ContextRef RendererVk::getContext() const
{
	return mContext;
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
	const uint32_t numFrames = getOptions().getNumFramesInFlight();

	mFrames.resize( numFrames );

	for ( uint32_t i = 0; i < numFrames; ++i ) {
		Frame &frame		= mFrames[i];
		frame.commandBuffer = vk::CommandBuffer::create( mCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, mDevice );
		frame.presentReady	= vk::Semaphore::create( mDevice );
	}
}

#if defined( CINDER_MSW_DESKTOP )
void RendererVk::setup( WindowImplMsw *windowImpl, RendererRef sharedRenderer )
{
	mWindowImpl = windowImpl;

	setupDevice( windowImpl->getTitle(), sharedRenderer );

	// Setup swapchain for window
	{
		vk::Swapchain::Options options = vk::Swapchain::Options();

		mSwapchain = vk::Swapchain::create( windowImpl, options, getDevice() );
	}

	// Create context
	{
		vk::Context::Options options = vk::Context::Options()
										   .setRenderTargets( { mSwapchain->getSurfaceFormat().format } )
										   .sampleCount( mOptions.getMsaa() );

		mContext = vk::Context::create(
			static_cast<uint32_t>( windowImpl->getSize().x ),
			static_cast<uint32_t>( windowImpl->getSize().y ),
			options,
			mDevice );
	}

	// Frame sync
	mFrameSync = vk::CountingSemaphore::create( 0, mDevice );

	// Command pool
	mCommandPool = vk::CommandPool::create( mDevice->getQueueFamilyIndices().graphics, vk::CommandPool::Options(), mDevice );

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
#if defined(CINDER_MSW)
	::RECT clientRect;
	::GetClientRect( mWindowImpl->getHwnd(), &clientRect );
	int widthPx = clientRect.right - clientRect.left;
	int heightPx = clientRect.bottom - clientRect.top;

	ivec2 sizePt = mWindowImpl->getSize();

	vk::viewport( 0, 0, widthPx, heightPx );
	vk::setMatricesWindow( sizePt.x, sizePt.y );
#endif
}

void RendererVk::makeCurrentContext( bool force )
{
	(void)force;
	sCurrentRenderer = this;

	// Frame data
	const uint32_t contextFrameIndex = mContext->getFrameIndex();
	Frame		  &frame			 = mFrames[contextFrameIndex];

	// Waits for frame's work to complete (i.e. present work)
	std::vector<vk::Context::SemaphoreInfo> waits;

	// Avoid unnecessary waits
	uint64_t counterValue = mFrameSync->getCounterValue();
	if ( counterValue < frame.signaledValue ) {
		waits.push_back( { mFrameSync.get(), frame.signaledValue } );
	}

	mContext->makeCurrent( waits );
}

void RendererVk::swapBuffers()
{
	// Value to signal when context's work is complete
	uint64_t contextWorkCompleteValue = mFrameSync->incrementCounter();

	// Get current context data before submit
	uint32_t	 contextFrameIndex = mContext->getFrameIndex();
	vk::ImageRef contextImage	   = mContext->getRenderTargetView( 0 )->getImage();

	// Submit context frame's work - this will increment frame index
	std::vector<vk::Context::SemaphoreInfo> waits;
	std::vector<vk::Context::SemaphoreInfo> signals = { { mFrameSync.get(), contextWorkCompleteValue } };
	mContext->submit( waits, signals );

	// Acquire next swapchain image
	vk::Swapchain::AcquireInfo acquireInfo = {};

	VkResult vkres = getSwapchain()->acquireNextImage( UINT64_MAX, &acquireInfo );
	if ( vkres != VK_SUCCESS ) {
		// An error actually happened
		if ( vkres < VK_SUCCESS ) {
			throw vk::VulkanFnFailedExc( "acquire next image wrapper", vkres );
		}
		else {
			// @TODO: recreate swapchain
		}
	}

	// Current renderer frame
	Frame			  &frame		   = mFrames[contextFrameIndex];
	const vk::ImageRef &swapchainImage = acquireInfo.image;

	// Build commnad buffer to copy context render target to swapchain
	frame.commandBuffer->begin();
	{
		// Resolve if needed othewise just copy
		if ( mOptions.getMsaa() > 1 ) {
			VkImageResolve region = {};
			region.srcSubresource = { contextImage->getAspectMask(), 0, 0, 1 };
			region.srcOffset	  = {};
			region.dstSubresource = { swapchainImage->getAspectMask(), 0, 0, 1 };
			region.dstOffset	  = {};
			region.extent		  = swapchainImage->getExtent();

			CI_VK_DEVICE_FN( CmdResolveImage(
				frame.commandBuffer->getCommandBufferHandle(),
				contextImage->getImageHandle(),
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				swapchainImage->getImageHandle(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&region ) );
		}
		else {
			VkImageCopy region	  = {};
			region.srcSubresource = { contextImage->getAspectMask(), 0, 0, 1 };
			region.srcOffset	  = {};
			region.dstSubresource = { swapchainImage->getAspectMask(), 0, 0, 1 };
			region.dstOffset	  = {};
			region.extent		  = swapchainImage->getExtent();

			CI_VK_DEVICE_FN( CmdCopyImage(
				frame.commandBuffer->getCommandBufferHandle(),
				contextImage->getImageHandle(),
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				swapchainImage->getImageHandle(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&region ) );
		}
	}
	frame.commandBuffer->end();

	frame.signaledValue = mFrameSync->incrementCounter();

	// Submit command buffer
	vk::SubmitInfo submitInfo = vk::SubmitInfo()
									.addCommandBuffer( frame.commandBuffer )
									.addWait( acquireInfo.imageReady )
									.addWait( mFrameSync, contextWorkCompleteValue )
									.addSignal( frame.presentReady )
									.addSignal( mFrameSync, frame.signaledValue );
	vkres = mDevice->submitGraphics( submitInfo );
	if ( vkres != VK_SUCCESS ) {
		throw vk::VulkanFnFailedExc( "vkQueueSumbit", vkres );
	}

	// Queue present
	vk::Swapchain::PresentInfo presentInfo = {};
	presentInfo.imageIndex				   = acquireInfo.imageIndex;
	presentInfo.presentReady			   = frame.presentReady;

	vkres = getSwapchain()->present( &presentInfo );
	if ( vkres != VK_SUCCESS ) {
		// An error actually happened
		if ( vkres < VK_SUCCESS ) {
			throw vk::VulkanFnFailedExc( "swapchain present wrapper", vkres );
		}
		else {
			// @TODO: recreate swapchain
		}
	}
}

Surface8u RendererVk::copyWindowSurface( const Area &area, int32_t windowHeightPixels )
{
	return Surface8u();
}

} // namespace cinder::app
