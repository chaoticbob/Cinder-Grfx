#include "cinder/vk/Context.h"
#include "cinder/vk/Command.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Image.h"
#include "cinder/vk/Sync.h"
#include "cinder/vk/Util.h"
#include "cinder/app/RendererVk.h"

namespace cinder::vk {

static Context *sCurrentContext = nullptr;

/////////////////////////////////////////////////////////////////////////////////////////////////
// Context::Options
Context::Options::Options( VkFormat renderTargetFormat, VkFormat depthStencilFormat, uint32_t samples )
	: mRenderTargetFormat( renderTargetFormat ), mDepthFormat( depthStencilFormat ), mSampleCount( toVkSampleCount( samples ) )
{
}

Context::Options &Context::Options::sampleCount( uint32_t value )
{
	mSampleCount = toVkSampleCount( value );
	return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Context

ContextRef Context::create( const Options &options, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return ContextRef( new Context( device, 0, 0, options ) );
}

ContextRef Context::create( uint32_t width, uint32_t height, const Options &options, vk::DeviceRef device )
{
	if ( ( width == 0 ) || ( height == 0 ) ) {
		throw VulkanExc( "invalid dimensions for renderable context" );
		return ContextRef();
	}

	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return ContextRef( new Context( device, width, height, options ) );
}

Context::Context( vk::DeviceRef device, uint32_t width, uint32_t height, const Options &options )
	: vk::DeviceChildObject( device ),
	  mNumFramesInFlight( options.mNumInFlightFrames ),
	  mWidth( width ),
	  mHeight( height ),
	  mDepthFormat( options.mDepthFormat ),
	  mStencilFormat( options.mStencilFormat ),
	  mSampleCount( options.mSampleCount )
{
	mRenderTargetFormats.push_back( options.mRenderTargetFormat );
	std::copy( options.mAdditionalRenderTargets.begin(), options.mAdditionalRenderTargets.end(), std::back_inserter( mRenderTargetFormats ) );

	mCombinedDepthStencil = ( mDepthFormat != VK_FORMAT_UNDEFINED ) && ( mDepthFormat == mStencilFormat );

	// Command pool and command buffers
	std::vector<vk::CommandBufferRef> commandBuffers;
	{
		mCommandPool   = vk::CommandPool::create( device->getQueueFamilyIndices().graphics, vk::CommandPool::Options(), device );
		commandBuffers = mCommandPool->allocateCommandBuffers( mNumFramesInFlight );
	}

	// Frame sync semaphore
	mFrameSyncSemaphore = vk::CountingSemaphore::create( 0, getDevice() );

	// Frames
	mFrames.resize( mNumFramesInFlight );
	for ( uint32_t i = 0; i < mNumFramesInFlight; ++i ) {
		initializeFrame( commandBuffers[i], mFrames[i] );
	}
}

Context::~Context()
{
}

void Context::initializeFrame( vk::CommandBufferRef commandBuffer, Frame &frame )
{
	frame.commandBuffer = commandBuffer;

	uint32_t numRenderTargets = countU32( mRenderTargetFormats );
	frame.renderTargets.resize( numRenderTargets );
	frame.rtvs.resize( numRenderTargets );
	for ( uint32_t i = 0; i < numRenderTargets; ++i ) {
		vk::Image::Usage   usage   = vk::Image::Usage().renderTarget().sampledImage();
		vk::Image::Options options = vk::Image::Options().samples( mSampleCount );
		frame.renderTargets[i]	   = vk::Image::create( mWidth, mHeight, mRenderTargetFormats[i], usage, vk::MemoryUsage::GPU_ONLY, options, getDevice() );

		frame.rtvs[i] = vk::ImageView::create( frame.renderTargets[i], getDevice() );
	}

	if ( mCombinedDepthStencil ) {
		if ( mDepthFormat != VK_FORMAT_UNDEFINED ) {
			vk::Image::Usage   usage   = vk::Image::Usage().depthStencil().sampledImage();
			vk::Image::Options options = vk::Image::Options().samples( mSampleCount );
			frame.depthTarget		   = vk::Image::create( mWidth, mHeight, mDepthFormat, usage, vk::MemoryUsage::GPU_ONLY, options, getDevice() );

			frame.dtv = vk::ImageView::create( frame.depthTarget, getDevice() );
		}
	}
	else {
		if ( mDepthFormat != VK_FORMAT_UNDEFINED ) {
			vk::Image::Usage   usage   = vk::Image::Usage().depthStencil().sampledImage();
			vk::Image::Options options = vk::Image::Options().samples( mSampleCount );
			frame.depthTarget		   = vk::Image::create( mWidth, mHeight, mDepthFormat, usage, vk::MemoryUsage::GPU_ONLY, options, getDevice() );

			frame.dtv = vk::ImageView::create( frame.depthTarget, getDevice() );
		}

		if ( mStencilFormat != VK_FORMAT_UNDEFINED ) {
			vk::Image::Usage   usage   = vk::Image::Usage().depthStencil().sampledImage();
			vk::Image::Options options = vk::Image::Options().samples( mSampleCount );
			frame.stencilTarget		   = vk::Image::create( mWidth, mHeight, mStencilFormat, usage, vk::MemoryUsage::GPU_ONLY, options, getDevice() );

			frame.stv = vk::ImageView::create( frame.stencilTarget, getDevice() );
		}
	}
}

void Context::makeCurrent( const std::vector<SemaphoreInfo> &externalWaits )
{
	sCurrentContext = this;

	// Wait for any external semaphores
	if ( !externalWaits.empty() ) {
		std::vector<VkSemaphore> semaphores;
		std::vector<uint64_t>	 values;
		for ( const auto &wait : externalWaits ) {
			if ( !wait.semaphore->isTimeline() ) {
				throw VulkanExc( "all external waits must be timeline semaphores" );
			}
			semaphores.push_back( wait.semaphore->getSemaphoreHandle() );
			values.push_back( wait.value );
		}

		VkSemaphoreWaitInfo vkswi = { VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
		vkswi.pNext				  = nullptr;
		vkswi.flags				  = 0;
		vkswi.semaphoreCount	  = countU32( semaphores );
		vkswi.pSemaphores		  = dataPtr( semaphores );
		vkswi.pValues			  = dataPtr( values );

		VkResult vkres = CI_VK_DEVICE_FN( WaitSemaphores( getDeviceHandle(), &vkswi, UINT64_MAX ) );
		if ( vkres != VK_SUCCESS ) {
			throw VulkanFnFailedExc( "vkWaitSemaphores", vkres );
		}
	}

	// Wait for any pending work to complete
	waitForCompletion();

	// Get current frame
	Frame &frame = getCurrentFrame();

	// Start command buffer recording if it's not already started
	if ( !frame.commandBuffer->isRecording() ) {
		frame.commandBuffer->begin();
	}
	// Start rendering if it's not already started
	if ( !frame.commandBuffer->isRendering() ) {
		vk::CommandBuffer::RenderingInfo ri = vk::CommandBuffer::RenderingInfo( frame.rtvs, frame.dtv );
		frame.commandBuffer->beginRendering( ri );
	}
}

Context *Context::getCurrentContext()
{
	return sCurrentContext;
}

Context::Frame &Context::getCurrentFrame()
{
	return mFrames[mFrameIndex];
}

const Context::Frame &Context::getCurrentFrame() const
{
	return mFrames[mFrameIndex];
}

void Context::submit( const std::vector<SemaphoreInfo> &waits, const std::vector<SemaphoreInfo> &signals )
{
	Frame &frame = getCurrentFrame();

	// End rendeirng
	if ( frame.commandBuffer->isRendering() ) {
		frame.commandBuffer->endRendering();
	}

	// End command buffer recording
	if ( frame.commandBuffer->isRecording() ) {
		frame.commandBuffer->end();
	}

	frame.frameSignaledValue = mFrameSyncSemaphore->incrementCounter();

	vk::SubmitInfo submitInfo = vk::SubmitInfo()
									.addCommandBuffer( frame.commandBuffer )
									.addSignal( mFrameSyncSemaphore, frame.frameSignaledValue );
	// Waits
	for ( const auto &wait : waits ) {
		submitInfo.addWait( wait.semaphore, wait.value );
	}
	// Signals
	for ( const auto &signal : signals ) {
		submitInfo.addSignal( signal.semaphore, signal.value );
	}

	VkResult vkres = getDevice()->submitGraphics( submitInfo );
	if ( vkres != VK_SUCCESS ) {
		throw vk::VulkanFnFailedExc( "vkQueueSumbit", vkres );
	}

	// Increment frame count
	++mFrameCount;
	// Update frame index
	mPreviousFrameIndex = mFrameIndex;
	mFrameIndex			= static_cast<uint32_t>( mFrameCount % mNumFramesInFlight );
}

void Context::waitForCompletion()
{
	Frame &frame = getCurrentFrame();

	// Avoid unncessary waits
	const uint64_t counterValue = mFrameSyncSemaphore->getCounterValue();
	if ( counterValue < frame.frameSignaledValue ) {
		mFrameSyncSemaphore->wait( frame.frameSignaledValue );
	}
}

void Context::clearColorAttachment( uint32_t index )
{
	VkRect2D rect = getCurrentFrame().renderTargets[index]->getArea();

	const ColorA &	  value		 = mClearState.get().color;
	VkClearColorValue clearValue = { value.r, value.g, value.b, value.a };

	getCommandBuffer()->clearColorAttachment( index, clearValue, rect );
}

void Context::clearDepthStencilAttachment( VkImageAspectFlags aspectMask )
{
}

} // namespace cinder::vk
