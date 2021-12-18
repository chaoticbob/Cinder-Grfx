#include "cinder/vk/Context.h"
#include "cinder/vk/Command.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Image.h"
#include "cinder/vk/Sync.h"
#include "cinder/vk/Util.h"
#include "cinder/app/RendererVk.h"

namespace cinder::vk {

Context *sCurrentContext = nullptr;

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
		frame.renderTargets[i]	   = vk::Image::create( mWidth, mHeight, mDepthFormat, usage, vk::MemoryUsage::GPU_ONLY, options, getDevice() );

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

void Context::makeCurrent()
{
	sCurrentContext = this;

	// Wait for any pending work to complete
	waitForCompletion();

	// Get current frame
	Frame &frame = getCurrentFrame();

	// Start command buffer recording if it's not already started
	if ( !frame.commandBuffer->isRecording() ) {
		frame.commandBuffer->begin();
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

void Context::submit( const SemaphoreList &waits, const SemaphoreList &signals )
{
	Frame &frame = getCurrentFrame();

	// End command buffer recording
	if ( frame.commandBuffer->isRecording() ) {
		frame.commandBuffer->end();
	}

	frame.frameSignaledValue = mFrameSyncSemaphore->incrementCounter();

	VkCommandBuffer commandBuffer = frame.commandBuffer->getCommandBufferHandle();

	std::vector<VkSemaphore>		  waitSemaphores;
	std::vector<uint64_t>			  waitValues;
	std::vector<VkPipelineStageFlags> waitDstStageMasks;
	for ( const auto &elem : waits.mSemaphores ) {
		waitSemaphores.push_back( elem.semaphore->getSemaphoreHandle() );
		waitValues.push_back( elem.value );
		waitDstStageMasks.push_back( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
	}

	std::vector<VkSemaphore> signalSemaphores;
	std::vector<uint64_t>	 signalValues;
	signalSemaphores.push_back( mFrameSyncSemaphore->getSemaphoreHandle() );
	signalValues.push_back( frame.frameSignaledValue );
	for ( const auto &elem : signals.mSemaphores ) {
		signalSemaphores.push_back( elem.semaphore->getSemaphoreHandle() );
		signalValues.push_back( elem.value );
	}

	VkTimelineSemaphoreSubmitInfo tsSubmitInfo = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
	tsSubmitInfo.pNext						   = nullptr;
	tsSubmitInfo.waitSemaphoreValueCount	   = countU32( waitValues );
	tsSubmitInfo.pWaitSemaphoreValues		   = dataPtr( waitValues );
	tsSubmitInfo.signalSemaphoreValueCount	   = countU32( signalValues );
	tsSubmitInfo.pSignalSemaphoreValues		   = dataPtr( signalValues );

	VkSubmitInfo submitInfo			= { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.pNext				= &tsSubmitInfo;
	submitInfo.waitSemaphoreCount	= countU32( waitSemaphores );
	submitInfo.pWaitSemaphores		= dataPtr( waitSemaphores );
	submitInfo.pWaitDstStageMask	= dataPtr( waitDstStageMasks );
	submitInfo.commandBufferCount	= 1;
	submitInfo.pCommandBuffers		= &commandBuffer;
	submitInfo.signalSemaphoreCount = countU32( signalSemaphores );
	submitInfo.pSignalSemaphores	= dataPtr( signalSemaphores );

	VkResult vkres = getDevice()->submitGraphics( &submitInfo );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkQueueSumbit", vkres );
	}

	// Increment frame count
	++mFrameCount;
	// Update frame index
	mFrameIndex = static_cast<uint32_t>( mFrameCount % mNumFramesInFlight );
}

void Context::waitForCompletion()
{
	Frame &frame = getCurrentFrame();

	mFrameSyncSemaphore->wait( frame.frameSignaledValue );
}

} // namespace cinder::vk
