#include "cinder/vk/Context.h"
#include "cinder/vk/Buffer.h"
#include "cinder/vk/Command.h"
#include "cinder/vk/Descriptor.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Image.h"
#include "cinder/vk/Mesh.h"
#include "cinder/vk/Pipeline.h"
#include "cinder/vk/Sampler.h"
#include "cinder/vk/ShaderProg.h"
#include "cinder/vk/Sync.h"
#include "cinder/vk/Texture.h"
#include "cinder/vk/UniformBuffer.h"
#include "cinder/vk/Util.h"
#include "cinder/vk/wrapper.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererVk.h"
#include "cinder/Log.h"

namespace cinder::vk {

static Context *sCurrentContext = nullptr;

/////////////////////////////////////////////////////////////////////////////////////////////////
// Context::Frame

void Context::Frame::resetDrawCalls()
{
	uint32_t n = countU32( drawCalls );
	for ( uint32_t i = 0; i < n; ++i ) {
		auto &elem	= drawCalls[i];
		elem->inUse = false;
	}
}

void Context::Frame::nextDrawCall( const vk::DescriptorSetLayoutRef &defaultSetLayout )
{
	currentDrawCall = nullptr;

	uint32_t n = countU32( drawCalls );
	for ( uint32_t i = 0; i < n; ++i ) {
		auto &elem = drawCalls[i];
		if ( elem->inUse == false ) {
			currentDrawCall		   = elem.get();
			currentDrawCall->inUse = true;
			break;
		}
	}

	if ( currentDrawCall == nullptr ) {
		auto drawCall = std::make_unique<DrawCall>();

		drawCall->descriptorSet = vk::DescriptorSet::create( descriptorPool, defaultSetLayout );

		currentDrawCall		   = drawCall.get();
		currentDrawCall->inUse = true;

		drawCalls.push_back( std::move( drawCall ) );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Context::Options

Context::Options::Options( VkFormat renderTargetFormat, VkFormat depthStencilFormat, uint32_t samples )
	: mRenderTargetFormats( { renderTargetFormat } ),
	  mDepthStencilFormat( depthStencilFormat ),
	  mSampleCount( toVkSampleCount( samples ) )
{
}

Context::Options &Context::Options::sampleCount( uint32_t value )
{
	mSampleCount = toVkSampleCount( value );
	return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Context::DescriptorState

Context::DescriptorState::DescriptorState()
{
}

void Context::DescriptorState::bindUniformBuffer( uint32_t bindingNumber, const vk::Buffer *buffer )
{
	mDescriptors.insert_or_assign( bindingNumber, Descriptor( bindingNumber, buffer ) );
}

void Context::DescriptorState::bindCombinedImageSampler( uint32_t bindingNumber, const vk::ImageView *imageView, const vk::Sampler *sampler )
{
	bool bind = true;
	auto it	  = mDescriptors.find( bindingNumber );
	if ( it != mDescriptors.end() ) {
		auto &descriptor = it->second;
		bind			 = ( descriptor.imageInfo.imageView != imageView ) || ( descriptor.imageInfo.sampler != sampler );
	}

	if ( bind ) {
		mDescriptors.insert_or_assign( bindingNumber, Descriptor( bindingNumber, imageView, sampler ) );
	}
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
	  mRenderTargetFormats( options.mRenderTargetFormats ),
	  mDepthStencilFormat( options.mDepthStencilFormat ),
	  mSampleCount( options.mSampleCount )
{
	initializeDescriptorSetLayouts();
	initializePipelineLayout();

	// Set default graphics state values
	vk::Pipeline::setDefaults( &mGraphicsState );

	// Set context's initial values for graphics state
	{
		mGraphicsState.pipelineLayout = mDefaultPipelineLayout.get();

		// IA
		mGraphicsState.ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		// RS
		mGraphicsState.rs.rasterizationSamples = options.mSampleCount;

		// DS
		mGraphicsState.ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		uint32_t renderTargetCount = countU32( mRenderTargetFormats );
		renderTargetCount		   = std::min<uint32_t>( renderTargetCount, CINDER_MAX_RENDER_TARGETS );

		// CB
		mGraphicsState.cb.attachmentCount = renderTargetCount;
		for ( uint32_t i = 0; i < CINDER_MAX_RENDER_TARGETS; ++i ) {
			mGraphicsState.cb.attachments[i].colorBlendOp = VK_BLEND_OP_ADD;
			mGraphicsState.cb.attachments[i].alphaBlendOp = VK_BLEND_OP_ADD;
		}

		// OM
		mGraphicsState.om.renderTargetCount = renderTargetCount;
		for ( uint32_t i = 0; i < renderTargetCount; ++i ) {
			mGraphicsState.om.renderTargets[i] = mRenderTargetFormats[i];
		}
		mGraphicsState.om.depthStencil = mDepthStencilFormat;
	}

	// Hash graphics pipeline state
	mCurrentGraphicsPipelineHash = vk::Pipeline::calculateHash( &mGraphicsState );

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

	mViewportStack.push_back( std::pair<ivec2, ivec2>( ivec2( 0, 0 ), ivec2( width, height ) ) );
	mScissorStack.push_back( std::pair<ivec2, ivec2>( ivec2( 0, 0 ), ivec2( width, height ) ) );

	mModelMatrixStack.push_back( mat4() );
	mViewMatrixStack.push_back( mat4() );
	mProjectionMatrixStack.push_back( mat4() );
	mGlslProgStack.push_back( nullptr );
}

Context::~Context()
{
}

void Context::initializeDescriptorSetLayouts()
{
	vk::DescriptorSetLayout::Options options = vk::DescriptorSetLayout::Options();

	for ( uint32_t i = 0; i < CINDER_CONTEXT_PER_STAGE_TEXTURE_COUNT; ++i ) {
		uint32_t vs = CINDER_CONTEXT_VS_BINDING_SHIFT_TEXTURE + i;
		uint32_t ps = CINDER_CONTEXT_PS_BINDING_SHIFT_TEXTURE + i;
		uint32_t hs = CINDER_CONTEXT_HS_BINDING_SHIFT_TEXTURE + i;
		uint32_t ds = CINDER_CONTEXT_DS_BINDING_SHIFT_TEXTURE + i;
		uint32_t gs = CINDER_CONTEXT_GS_BINDING_SHIFT_TEXTURE + i;
		options.addCombinedImageSampler( vs );
		options.addCombinedImageSampler( ps );
		options.addCombinedImageSampler( hs );
		options.addCombinedImageSampler( ds );
		options.addCombinedImageSampler( gs );
	}

	for ( uint32_t i = 0; i < CINDER_CONTEXT_PER_STAGE_UBO_COUNT; ++i ) {
		uint32_t vs = CINDER_CONTEXT_VS_BINDING_SHIFT_UBO + i;
		uint32_t ps = CINDER_CONTEXT_PS_BINDING_SHIFT_UBO + i;
		uint32_t hs = CINDER_CONTEXT_HS_BINDING_SHIFT_UBO + i;
		uint32_t ds = CINDER_CONTEXT_DS_BINDING_SHIFT_UBO + i;
		uint32_t gs = CINDER_CONTEXT_GS_BINDING_SHIFT_UBO + i;
		options.addUniformBuffer( vs );
		options.addUniformBuffer( ps );
		options.addUniformBuffer( hs );
		options.addUniformBuffer( ds );
		options.addUniformBuffer( gs );
	}

	mDefaultSetLayout = vk::DescriptorSetLayout::create( options, getDevice() );
}

void Context::initializePipelineLayout()
{
	vk::PipelineLayout::Options options = vk::PipelineLayout::Options().addSetLayout( mDefaultSetLayout );

	mDefaultPipelineLayout = vk::PipelineLayout::create( options, getDevice() );
}

void Context::initializeFrame( vk::CommandBufferRef commandBuffer, Frame &frame )
{
	frame.commandBuffer = commandBuffer;

	vk::DescriptorPool::Options options = vk::DescriptorPool::Options()
											  .addCombinedImageSampler( 10 * CINDER_CONTEXT_PER_STAGE_TEXTURE_COUNT )
											  .addUniformBuffer( 10 * CINDER_CONTEXT_PER_STAGE_UBO_COUNT );
	frame.descriptorPool = vk::DescriptorPool::create( options, getDevice() );

	uint32_t renderTargetCount = countU32( mRenderTargetFormats );
	frame.renderTargets.resize( renderTargetCount );
	frame.rtvs.resize( renderTargetCount );
	for ( uint32_t i = 0; i < renderTargetCount; ++i ) {
		vk::Image::Usage   usage   = vk::Image::Usage().renderTarget().sampledImage();
		vk::Image::Options options = vk::Image::Options().samples( mSampleCount );
		frame.renderTargets[i]	   = vk::Image::create( mWidth, mHeight, mRenderTargetFormats[i], usage, vk::MemoryUsage::GPU_ONLY, options, getDevice() );

		frame.rtvs[i] = vk::ImageView::create( frame.renderTargets[i], getDevice() );
	}

	if ( mDepthStencilFormat != VK_FORMAT_UNDEFINED ) {
		vk::Image::Usage   usage   = vk::Image::Usage().depthStencil().sampledImage();
		vk::Image::Options options = vk::Image::Options().samples( mSampleCount );
		frame.depthStencil		   = vk::Image::create( mWidth, mHeight, mDepthStencilFormat, usage, vk::MemoryUsage::GPU_ONLY, options, getDevice() );

		frame.dsv = vk::ImageView::create( frame.depthStencil, getDevice() );
	}
}

void Context::registerChild( vk::ContextChildObject *child )
{
	auto it = std::find( mChildren.begin(), mChildren.end(), child );
	if ( it == mChildren.end() ) {
		mChildren.push_back( child );
	}
}

void Context::unregisterChild( vk::ContextChildObject *child )
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// Templated stack management routines

template <typename T>
bool Context::pushStackState( std::vector<T> &stack, T value )
{
	bool needsToBeSet = true;
	if ( ( !stack.empty() ) && ( stack.back() == value ) ) {
		needsToBeSet = false;
	}
	stack.push_back( value );
	return needsToBeSet;
}

template <typename T>
bool Context::popStackState( std::vector<T> &stack )
{
	if ( !stack.empty() ) {
		T prevValue = stack.back();
		stack.pop_back();
		if ( !stack.empty() ) {
			return stack.back() != prevValue;
		}
		else {
			return true;
		}
	}
	else {
		return true;
	}
}

template <typename T>
bool Context::setStackState( std::vector<T> &stack, T value )
{
	bool needsToBeSet = true;
	if ( ( !stack.empty() ) && ( stack.back() == value ) ) {
		needsToBeSet = false;
	}
	else if ( stack.empty() ) {
		stack.push_back( value );
	}
	else {
		stack.back() = value;
	}
	return needsToBeSet;
}

template <typename T>
bool Context::getStackState( std::vector<T> &stack, T *result )
{
	if ( stack.empty() ) {
		return false;
	}
	else {
		*result = stack.back();
		return true;
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

	// Reset draw calls
	frame.resetDrawCalls();
	frame.nextDrawCall( mDefaultSetLayout );

	// Start command buffer recording if it's not already started
	if ( !frame.commandBuffer->isRecording() ) {
		frame.commandBuffer->begin();

		frame.commandBuffer->setViewport( 0, 0, static_cast<float>( mWidth ), static_cast<float>( mHeight ) );
		frame.commandBuffer->setScissor( 0, 0, mWidth, mHeight );

		setDynamicStates( true );
	}
	// Start rendering if it's not already started
	if ( !frame.commandBuffer->isRendering() ) {
		vk::CommandBuffer::RenderingInfo ri = vk::CommandBuffer::RenderingInfo( frame.rtvs, frame.dsv );
		frame.commandBuffer->beginRendering( ri );
	}

	if ( mFrameCount > 0 ) {
		for ( auto &child : mChildren ) {
			child->flightSync( mFrameIndex, mPreviousFrameIndex );
		}
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

vk::StockShaderManager *Context::getStockShaderManager()
{
	if ( !mStockShaderManager ) {
		mStockShaderManager = std::make_unique<vk::StockShaderManager>( shared_from_this() );
	}
	return mStockShaderManager.get();
}

//////////////////////////////////////////////////////////////////
// Viewport
void Context::viewport( const std::pair<ivec2, ivec2> &viewport )
{
	if ( setStackState( mViewportStack, viewport ) ) {
		getCurrentCommandBuffer()->setViewport(
			static_cast<float>( viewport.first.x ),
			static_cast<float>( viewport.first.y ),
			static_cast<float>( viewport.second.x ),
			static_cast<float>( viewport.second.y ),
			0.0f,
			1.0f );
	}
}

void Context::pushViewport( const std::pair<ivec2, ivec2> &viewport )
{
	if ( pushStackState( mViewportStack, viewport ) ) {
		getCurrentCommandBuffer()->setViewport(
			static_cast<float>( viewport.first.x ),
			static_cast<float>( viewport.first.y ),
			static_cast<float>( viewport.second.x ),
			static_cast<float>( viewport.second.y ),
			0.0f,
			1.0f );
	}
}

void Context::pushViewport()
{
	mViewportStack.push_back( getViewport() );
}

void Context::popViewport( bool forceRestore )
{
	if ( mViewportStack.empty() ) {
		CI_LOG_E( "Viewport stack underflow" );
	}
	else if ( popStackState( mViewportStack ) || forceRestore ) {
		auto viewport = getViewport();
		getCurrentCommandBuffer()->setViewport(
			static_cast<float>( viewport.first.x ),
			static_cast<float>( viewport.first.y ),
			static_cast<float>( viewport.second.x ),
			static_cast<float>( viewport.second.y ),
			0.0f,
			1.0f );
	}
}

std::pair<ivec2, ivec2> Context::getViewport()
{
	if ( mViewportStack.empty() ) {
		// GLint params[4];
		// glGetIntegerv( GL_VIEWPORT, params );
		//// push twice in anticipation of later pop
		// mViewportStack.push_back( std::pair<ivec2, ivec2>( ivec2( params[0], params[1] ), ivec2( params[2], params[3] ) ) );
		// mViewportStack.push_back( std::pair<ivec2, ivec2>( ivec2( params[0], params[1] ), ivec2( params[2], params[3] ) ) );

		// push twice in anticipation of later pop
		mViewportStack.push_back( std::pair<ivec2, ivec2>( ivec2( 0, 0 ), ivec2( mWidth, mHeight ) ) );
		mViewportStack.push_back( std::pair<ivec2, ivec2>( ivec2( 0, 0 ), ivec2( mWidth, mHeight ) ) );
	}

	return mViewportStack.back();
}

//////////////////////////////////////////////////////////////////
// Scissor Test
void Context::setScissor( const std::pair<ivec2, ivec2> &scissor )
{
	if ( setStackState( mScissorStack, scissor ) ) {
		getCurrentCommandBuffer()->setScissor(
			scissor.first.x,
			scissor.first.y,
			static_cast<uint32_t>( scissor.second.x ),
			static_cast<uint32_t>( scissor.second.y ) );
	}
}

void Context::pushScissor( const std::pair<ivec2, ivec2> &scissor )
{
	if ( pushStackState( mScissorStack, scissor ) ) {
		getCurrentCommandBuffer()->setScissor(
			scissor.first.x,
			scissor.first.y,
			static_cast<uint32_t>( scissor.second.x ),
			static_cast<uint32_t>( scissor.second.y ) );
	}
}

void Context::pushScissor()
{
	mScissorStack.push_back( getScissor() );
}

void Context::popScissor( bool forceRestore )
{
	if ( mScissorStack.empty() ) {
		CI_LOG_E( "Scissor stack underflow" );
	}
	else if ( popStackState( mScissorStack ) || forceRestore ) {
		auto scissor = getScissor();
		getCurrentCommandBuffer()->setScissor(
			scissor.first.x,
			scissor.first.y,
			static_cast<uint32_t>( scissor.second.x ),
			static_cast<uint32_t>( scissor.second.y ) );
	}
}

std::pair<ivec2, ivec2> Context::getScissor()
{
	if ( mScissorStack.empty() ) {
		// GLint params[4];
		// glGetIntegerv( GL_SCISSOR_BOX, params );
		//// push twice in anticipation of later pop
		// mScissorStack.push_back( std::pair<ivec2, ivec2>( ivec2( params[0], params[1] ), ivec2( params[2], params[3] ) ) );
		// mScissorStack.push_back( std::pair<ivec2, ivec2>( ivec2( params[0], params[1] ), ivec2( params[2], params[3] ) ) );

		// push twice in anticipation of later pop
		mScissorStack.push_back( std::pair<ivec2, ivec2>( ivec2( 0, 0 ), ivec2( mWidth, mHeight ) ) );
		mScissorStack.push_back( std::pair<ivec2, ivec2>( ivec2( 0, 0 ), ivec2( mWidth, mHeight ) ) );
	}

	return mScissorStack.back();
}

//////////////////////////////////////////////////////////////////
// Shader

void Context::bindShaderProg( const vk::ShaderProg *prog )
{
	mShaderProg = prog;

	mGraphicsState.vert = ( mShaderProg != nullptr ) ? mShaderProg->getVertexShader() : nullptr;
	mGraphicsState.frag = ( mShaderProg != nullptr ) ? mShaderProg->getFragmentShader() : nullptr;
	mGraphicsState.geom = ( mShaderProg != nullptr ) ? mShaderProg->getGeometryShader() : nullptr;
	mGraphicsState.tese = ( mShaderProg != nullptr ) ? mShaderProg->getTessellationEvalShader() : nullptr;
	mGraphicsState.tesc = ( mShaderProg != nullptr ) ? mShaderProg->getTessellationCtrlShader() : nullptr;

	// auto block = mShaderProgram->getDefaultUniformBlock();
	// if ( block ) {
	//	mDescriptorState.bindUniformBuffer( block->getBinding(), mShaderProgram->getDefaultUniformBuffer()->getBindableBuffer() );
	// }

	if ( mShaderProg != nullptr ) {
		auto &buffers = mShaderProg->getDefaultUniformBuffers();
		for ( auto &buffer : buffers ) {
			auto block = buffer->getUniformBlock();
			mDescriptorState.bindUniformBuffer( block->getBinding(), buffer->getBindableBuffer() );
		}
	}
}

void Context::bindGlslProg( const vk::GlslProg *prog )
{
	if ( mGlslProgStack.empty() || ( mGlslProgStack.back() != prog ) ) {
		if ( !mGlslProgStack.empty() ) {
			mGlslProgStack.back() = prog;
		}
		bindShaderProg( prog );
	}
}

void Context::pushGlslProg( const vk::GlslProg *prog )
{
	const GlslProg *prevGlsl = getGlslProg();

	mGlslProgStack.push_back( prog );
	if ( prog != prevGlsl ) {
		bindShaderProg( prog );
	}
}

void Context::pushGlslProg()
{
	mGlslProgStack.push_back( getGlslProg() );
}

void Context::popGlslProg( bool forceRestore )
{
	const GlslProg *prevGlsl = getGlslProg();

	if ( !mGlslProgStack.empty() ) {
		mGlslProgStack.pop_back();
		if ( !mGlslProgStack.empty() ) {
			if ( forceRestore || ( prevGlsl != mGlslProgStack.back() ) ) {
				bindShaderProg( mGlslProgStack.back() );
			}
		}
		else {
			CI_LOG_E( "Empty GlslProg stack" );
		}
	}
	else {
		CI_LOG_E( "GlslProg stack underflow" );
	}
}

const GlslProg *Context::getGlslProg()
{
	if ( mGlslProgStack.empty() ) {
		mGlslProgStack.push_back( nullptr );
	}

	return mGlslProgStack.back();
}

//////////////////////////////////////////////////////////////////
// Texture

void Context::bindTexture( const vk::TextureBase *texture, uint32_t binding )
{
	// binding += CINDER_CONTEXT_BINDING_SHIFT_TEXTURE;
	mDescriptorState.bindCombinedImageSampler( binding + CINDER_CONTEXT_VS_BINDING_SHIFT_TEXTURE, texture->getSampledImageView(), texture->getSampler() );
	mDescriptorState.bindCombinedImageSampler( binding + CINDER_CONTEXT_PS_BINDING_SHIFT_TEXTURE, texture->getSampledImageView(), texture->getSampler() );
}

void Context::unbindTexture( uint32_t binding )
{
	// binding += CINDER_CONTEXT_BINDING_SHIFT_TEXTURE;
	mDescriptorState.bindCombinedImageSampler( binding + CINDER_CONTEXT_VS_BINDING_SHIFT_TEXTURE, nullptr, nullptr );
	mDescriptorState.bindCombinedImageSampler( binding + CINDER_CONTEXT_PS_BINDING_SHIFT_TEXTURE, nullptr, nullptr );
}

void Context::initTextureBindingStack( uint32_t binding )
{
	if ( mTextureBindingStack.find( binding ) == mTextureBindingStack.end() ) {
		mTextureBindingStack[binding].push_back( nullptr );
		mTextureBindingStack[binding].push_back( nullptr );
	}
}

void Context::pushTextureBinding( const vk::TextureBase *texture )
{
	uint32_t binding = getActiveTexture();
	pushTextureBinding( texture, binding );

	// mTextureBindingStack[binding].push_back( std::make_pair( texture->getSampledImageView(), texture->getSampler() ) );
	//  if( mTextureBindingStack.find( binding ) == mTextureBindingStack.end() ) {
	//	mTextureBindingStack[binding].push_back( std::make_pair(texture->getSampledImageView(), texture->getSampler()));
	//
	//	//GLenum targetBinding = Texture::getBindingConstantForTarget( target );
	//	//GLint queriedInt = -1;
	//	//if( targetBinding > 0 ) {
	//	//	ScopedActiveTexture actScp( textureUnit );
	//	//	glGetIntegerv( targetBinding, &queriedInt );
	//	//}
	//	//mTextureBindingStack[textureUnit][target].push_back( queriedInt );
	//  }
	//  else if( mTextureBindingStack[textureUnit].find( texture ) == mTextureBindingStack[textureUnit].end() ) {
	//	mTextureBindingStack[textureUnit][target] = std::vector<GLint>();
	//	GLenum targetBinding = Texture::getBindingConstantForTarget( target );
	//	GLint queriedInt = -1;
	//	if( targetBinding > 0 ) {
	//		ScopedActiveTexture actScp( textureUnit );
	//		glGetIntegerv( targetBinding, &queriedInt );
	//	}
	//	mTextureBindingStack[textureUnit][target].push_back( queriedInt );
	//  }
	//
	//  mTextureBindingStack[textureUnit][target].push_back( mTextureBindingStack[textureUnit][target].back() );
}

void Context::pushTextureBinding( const vk::TextureBase *texture, uint32_t binding )
{
	initTextureBindingStack( binding );

	auto &stack = mTextureBindingStack[binding];
	pushStackState( stack, texture );

	bindTexture( texture, binding );

	// if ( mTextureBindingStack.find( binding ) == mTextureBindingStack.end() ) {
	//	auto nullEntry = std::make_pair<const vk::ImageView *, const vk::Sampler *>( nullptr, nullptr );
	//	mTextureBindingStack[binding].push_back( nullEntry );
	// }
	// else {
	//
	// }

	// pushTextureBinding( target, textureUnit );
	// bindTexture( target, textureId, textureUnit );
}

void Context::popTextureBinding( uint32_t binding, bool forceRestore )
{
	if ( mTextureBindingStack.find( binding ) == mTextureBindingStack.end() ) {
		CI_LOG_E( "Popping unencountered texture binding target:" << binding );
	}

	initTextureBindingStack( binding );

	auto &stack = mTextureBindingStack[binding];
	popStackState( stack );

	if ( forceRestore ) {
		auto texture = getTextureBinding( binding );
		if ( texture ) {
			bindTexture( texture, binding );
		}
		else {
			unbindTexture( binding );
		}
	}

	/*
		if ( mTextureBindingStack.find( binding ) == mTextureBindingStack.end() ) {
			mTextureBindingStack[textureUnit] = std::map<GLenum, std::vector<GLint>>();
			CI_LOG_E( "Popping unencountered texture binding target:" << gl::constantToString( target ) );
		}

		auto cached = mTextureBindingStack[textureUnit].find( target );
		if ( ( cached != mTextureBindingStack[textureUnit].end() ) && ( !cached->second.empty() ) ) {
			GLint prevValue = cached->second.back();
			cached->second.pop_back();
			if ( !cached->second.empty() ) {
				if ( forceRestore || ( cached->second.back() != prevValue ) ) {
					ScopedActiveTexture actScp( textureUnit );
					glBindTexture( target, cached->second.back() );
				}
			}
		}
	*/
}

const vk::TextureBase *Context::getTextureBinding( uint32_t binding )
{
	initTextureBindingStack( binding );
	const vk::TextureBase *result = nullptr;
	auto				  &stack  = mTextureBindingStack[binding];
	getStackState( stack, &result );
	return result;

	/*
		if ( mTextureBindingStack.find( textureUnit ) == mTextureBindingStack.end() )
			mTextureBindingStack[textureUnit] = std::map<GLenum, std::vector<GLint>>();

		auto cachedIt = mTextureBindingStack[textureUnit].find( target );
		if ( ( cachedIt == mTextureBindingStack[textureUnit].end() ) || ( cachedIt->second.empty() ) || ( cachedIt->second.back() == -1 ) ) {
			GLint  queriedInt	 = 0;
			GLenum targetBinding = Texture::getBindingConstantForTarget( target );
			if ( targetBinding > 0 ) {
				ScopedActiveTexture actScp( textureUnit );
				glGetIntegerv( targetBinding, &queriedInt );
			}
			else
				return 0; // warning?

			if ( mTextureBindingStack[textureUnit][target].empty() ) { // bad - empty stack; push twice to allow for the pop later and not lead to an empty stack
				mTextureBindingStack[textureUnit][target] = vector<GLint>();
				mTextureBindingStack[textureUnit][target].push_back( queriedInt );
				mTextureBindingStack[textureUnit][target].push_back( queriedInt );
			}
			else
				mTextureBindingStack[textureUnit][target].back() = queriedInt;
			return (GLuint)queriedInt;
		}
		else
			return (GLuint)cachedIt->second.back();
	*/
}

//////////////////////////////////////////////////////////////////
// ActiveTexture

void Context::setActiveTexture( uint32_t binding )
{
	if ( setStackState<uint32_t>( mActiveTextureStack, binding ) ) {
		// Nothing for now
	}
}

void Context::pushActiveTexture( uint32_t binding )
{
	if ( pushStackState<uint32_t>( mActiveTextureStack, binding ) ) {
		// Nothing for now
	}
}

void Context::pushActiveTexture()
{
	pushStackState<uint32_t>( mActiveTextureStack, getActiveTexture() );
}

void Context::popActiveTexture( bool forceRestore )
{
	if ( mActiveTextureStack.empty() ) {
		CI_LOG_E( "Active texture stack underflow" );
	}
	else if ( popStackState<uint32_t>( mActiveTextureStack ) || forceRestore ) {
		// Nothing for now
	}
}

uint32_t Context::getActiveTexture()
{
	if ( mActiveTextureStack.empty() ) {
		mActiveTextureStack.push_back( 0 );
		mActiveTextureStack.push_back( 0 );
	}

	return mActiveTextureStack.back();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// BlendFunc

void Context::enableBlend( bool enable, uint32_t attachmentIndex )
{
	mGraphicsState.cb.attachments[attachmentIndex].blendEnable = enable;
}

void Context::blendFunc( VkBlendFactor sfactor, VkBlendFactor dfactor, uint32_t attachmentIndex )
{
	blendFuncSeparate( sfactor, dfactor, sfactor, dfactor, attachmentIndex );
}

void Context::blendFuncSeparate( VkBlendFactor srcRGB, VkBlendFactor dstRGB, VkBlendFactor srcAlpha, VkBlendFactor dstAlpha, uint32_t attachmentIndex )
{
	bool needsChange = setStackState<VkBlendFactor>( mBlendSrcRgbStack[attachmentIndex], srcRGB );
	needsChange		 = setStackState<VkBlendFactor>( mBlendDstRgbStack[attachmentIndex], dstRGB ) || needsChange;
	needsChange		 = setStackState<VkBlendFactor>( mBlendSrcAlphaStack[attachmentIndex], srcAlpha ) || needsChange;
	needsChange		 = setStackState<VkBlendFactor>( mBlendDstAlphaStack[attachmentIndex], dstAlpha ) || needsChange;
	if ( needsChange ) {
		mGraphicsState.cb.attachments[attachmentIndex].srcColorBlendFactor = srcRGB;
		mGraphicsState.cb.attachments[attachmentIndex].dstColorBlendFactor = dstRGB;
		mGraphicsState.cb.attachments[attachmentIndex].srcAlphaBlendFactor = srcAlpha;
		mGraphicsState.cb.attachments[attachmentIndex].dstAlphaBlendFactor = dstAlpha;
	}
}

void Context::pushBlendFuncSeparate( VkBlendFactor srcRGB, VkBlendFactor dstRGB, VkBlendFactor srcAlpha, VkBlendFactor dstAlpha, uint32_t attachmentIndex )
{
	bool needsChange = pushStackState<VkBlendFactor>( mBlendSrcRgbStack[attachmentIndex], srcRGB );
	needsChange		 = pushStackState<VkBlendFactor>( mBlendDstRgbStack[attachmentIndex], dstRGB ) || needsChange;
	needsChange		 = pushStackState<VkBlendFactor>( mBlendSrcAlphaStack[attachmentIndex], srcAlpha ) || needsChange;
	needsChange		 = pushStackState<VkBlendFactor>( mBlendDstAlphaStack[attachmentIndex], dstAlpha ) || needsChange;
	if ( needsChange ) {
		mGraphicsState.cb.attachments[attachmentIndex].srcColorBlendFactor = srcRGB;
		mGraphicsState.cb.attachments[attachmentIndex].dstColorBlendFactor = dstRGB;
		mGraphicsState.cb.attachments[attachmentIndex].srcAlphaBlendFactor = srcAlpha;
		mGraphicsState.cb.attachments[attachmentIndex].dstAlphaBlendFactor = dstAlpha;
	}
}

void Context::pushBlendFuncSeparate( uint32_t attachmentIndex )
{
	VkBlendFactor resultSrcRGB, resultDstRGB, resultSrcAlpha, resultDstAlpha;
	getBlendFuncSeparate( &resultSrcRGB, &resultDstRGB, &resultSrcAlpha, &resultDstAlpha );

	mBlendSrcRgbStack[attachmentIndex].push_back( resultSrcRGB );
	mBlendDstRgbStack[attachmentIndex].push_back( resultDstRGB );
	mBlendSrcAlphaStack[attachmentIndex].push_back( resultSrcAlpha );
	mBlendDstAlphaStack[attachmentIndex].push_back( resultDstAlpha );
}

void Context::popBlendFuncSeparate( bool forceRestore, uint32_t attachmentIndex )
{
	bool needsChange = popStackState<VkBlendFactor>( mBlendSrcRgbStack[attachmentIndex] );
	needsChange		 = popStackState<VkBlendFactor>( mBlendDstRgbStack[attachmentIndex] ) || needsChange;
	needsChange		 = popStackState<VkBlendFactor>( mBlendSrcAlphaStack[attachmentIndex] ) || needsChange;
	needsChange		 = popStackState<VkBlendFactor>( mBlendDstAlphaStack[attachmentIndex] ) || needsChange;
	needsChange		 = forceRestore || needsChange;
	if ( needsChange && ( !mBlendSrcRgbStack[attachmentIndex].empty() ) && ( !mBlendSrcAlphaStack[attachmentIndex].empty() ) && ( !mBlendDstRgbStack[attachmentIndex].empty() ) && ( !mBlendDstAlphaStack[attachmentIndex].empty() ) ) {
		mGraphicsState.cb.attachments[attachmentIndex].srcColorBlendFactor = mBlendSrcRgbStack[attachmentIndex].back();
		mGraphicsState.cb.attachments[attachmentIndex].dstColorBlendFactor = mBlendDstRgbStack[attachmentIndex].back();
		mGraphicsState.cb.attachments[attachmentIndex].srcAlphaBlendFactor = mBlendSrcAlphaStack[attachmentIndex].back();
		mGraphicsState.cb.attachments[attachmentIndex].dstAlphaBlendFactor = mBlendDstAlphaStack[attachmentIndex].back();
	}
}

void Context::getBlendFuncSeparate( VkBlendFactor *resultSrcRGB, VkBlendFactor *resultDstRGB, VkBlendFactor *resultSrcAlpha, VkBlendFactor *resultDstAlpha, uint32_t attachmentIndex )
{
	// push twice on empty to accommodate inevitable push later
	if ( mBlendSrcRgbStack[attachmentIndex].empty() ) {
		mBlendSrcRgbStack[attachmentIndex].push_back( VK_BLEND_FACTOR_ONE );
		mBlendSrcRgbStack[attachmentIndex].push_back( VK_BLEND_FACTOR_ONE );
	}
	if ( mBlendDstRgbStack[attachmentIndex].empty() ) {
		mBlendDstRgbStack[attachmentIndex].push_back( VK_BLEND_FACTOR_ZERO );
		mBlendDstRgbStack[attachmentIndex].push_back( VK_BLEND_FACTOR_ZERO );
	}
	if ( mBlendSrcAlphaStack[attachmentIndex].empty() ) {
		mBlendSrcAlphaStack[attachmentIndex].push_back( VK_BLEND_FACTOR_ONE );
		mBlendSrcAlphaStack[attachmentIndex].push_back( VK_BLEND_FACTOR_ONE );
	}
	if ( mBlendDstAlphaStack[attachmentIndex].empty() ) {
		mBlendDstAlphaStack[attachmentIndex].push_back( VK_BLEND_FACTOR_ZERO );
		mBlendDstAlphaStack[attachmentIndex].push_back( VK_BLEND_FACTOR_ZERO );
	}

	*resultSrcRGB	= mBlendSrcRgbStack[attachmentIndex].back();
	*resultDstRGB	= mBlendDstRgbStack[attachmentIndex].back();
	*resultSrcAlpha = mBlendSrcAlphaStack[attachmentIndex].back();
	*resultDstAlpha = mBlendDstAlphaStack[attachmentIndex].back();
}

//////////////////////////////////////////////////////////////////
// Default shader vars

void Context::setDefaultShaderVars()
{
	if ( !mShaderProg ) {
		return;
	}

	auto &buffers = mShaderProg->getDefaultUniformBuffers();

	for ( auto &buffer : buffers ) {
		const auto &uniforms = buffer->getUniformBlock()->getUniforms();
		for ( const auto &uniform : uniforms ) {
			switch ( uniform.getUniformSemantic() ) {
				default: break;
				case UNIFORM_MODEL_MATRIX: {
					auto model = vk::getModelMatrix();
					buffer->uniform( uniform.getName(), model );
				} break;
				case UNIFORM_MODEL_MATRIX_INVERSE: {
					auto inverseModel = glm::inverse( vk::getModelMatrix() );
					buffer->uniform( uniform.getName(), inverseModel );
				} break;
				case UNIFORM_MODEL_MATRIX_INVERSE_TRANSPOSE: {
					auto modelInverseTranspose = vk::calcModelMatrixInverseTranspose();
					buffer->uniform( uniform.getName(), modelInverseTranspose );
				} break;
				case UNIFORM_VIEW_MATRIX: {
					auto view = vk::getViewMatrix();
					buffer->uniform( uniform.getName(), view );
				} break;
				case UNIFORM_VIEW_MATRIX_INVERSE: {
					auto viewInverse = vk::calcViewMatrixInverse();
					buffer->uniform( uniform.getName(), viewInverse );
				} break;
				case UNIFORM_MODEL_VIEW: {
					auto modelView = vk::getModelView();
					buffer->uniform( uniform.getName(), modelView );
				} break;
				case UNIFORM_MODEL_VIEW_INVERSE: {
					auto modelViewInverse = glm::inverse( vk::getModelView() );
					buffer->uniform( uniform.getName(), modelViewInverse );
				} break;
				case UNIFORM_MODEL_VIEW_INVERSE_TRANSPOSE: {
					auto normalMatrix = vk::calcNormalMatrix();
					buffer->uniform( uniform.getName(), normalMatrix );
				} break;
				case UNIFORM_MODEL_VIEW_PROJECTION: {
					auto modelViewProjection = vk::getModelViewProjection();
					buffer->uniform( uniform.getName(), modelViewProjection );
				} break;
				case UNIFORM_MODEL_VIEW_PROJECTION_INVERSE: {
					auto modelViewProjectionInverse = glm::inverse( vk::getModelViewProjection() );
					buffer->uniform( uniform.getName(), modelViewProjectionInverse );
				} break;
				case UNIFORM_PROJECTION_MATRIX: {
					auto projection = vk::getProjectionMatrix();
					buffer->uniform( uniform.getName(), projection );
				} break;
				case UNIFORM_PROJECTION_MATRIX_INVERSE: {
					auto projectionInverse = glm::inverse( vk::getProjectionMatrix() );
					buffer->uniform( uniform.getName(), projectionInverse );
				} break;
				case UNIFORM_VIEW_PROJECTION: {
					auto viewProjection = vk::getProjectionMatrix() * vk::getViewMatrix();
					buffer->uniform( uniform.getName(), viewProjection );
				} break;
				case UNIFORM_NORMAL_MATRIX: {
					auto normalMatrix = vk::calcNormalMatrix();
					buffer->uniform( uniform.getName(), normalMatrix );
				} break;
				case UNIFORM_VIEWPORT_MATRIX: {
					auto viewport = vk::calcViewportMatrix();
					buffer->uniform( uniform.getName(), viewport );
				} break;
				case UNIFORM_WINDOW_SIZE: {
					auto windowSize = app::getWindowSize();
					buffer->uniform( uniform.getName(), windowSize );
				} break;
				case UNIFORM_ELAPSED_SECONDS: {
					auto elapsed = float( app::getElapsedSeconds() );
					buffer->uniform( uniform.getName(), elapsed );
					break;
				}
			}
		}
	}
}

void Context::clearColorAttachment( uint32_t index )
{
	VkRect2D rect = getCurrentFrame().renderTargets[index]->getArea();

	const ColorA	 &value		 = mClearValues.color;
	VkClearColorValue clearValue = { value.r, value.g, value.b, value.a };

	getCurrentCommandBuffer()->clearColorAttachment( index, clearValue, rect );
}

void Context::clearDepthStencilAttachment( VkImageAspectFlags aspectMask )
{
	VkRect2D rect = getCurrentFrame().depthStencil->getArea();

	getCurrentCommandBuffer()->clearDepthStencilAttachment( mClearValues.depth, mClearValues.stencil, rect, aspectMask );
}

void Context::bindDefaultDescriptorSet()
{
	std::array<VkDescriptorBufferInfo, CINDER_CONTEXT_STAGE_COUNT * CINDER_CONTEXT_PER_STAGE_UBO_COUNT>	   uboBufferInfos;
	std::array<VkDescriptorImageInfo, CINDER_CONTEXT_STAGE_COUNT * CINDER_CONTEXT_PER_STAGE_TEXTURE_COUNT> textureImageInfos;
	uint32_t																							   uboCount		= 0;
	uint32_t																							   textureCount = 0;

	std::vector<VkWriteDescriptorSet> writes;
	for ( const auto &it : mDescriptorState.mDescriptors ) {
		auto &descriptor = it.second;

		VkWriteDescriptorSet write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.dstSet			   = getCurrentFrame().currentDrawCall->descriptorSet->getDescriptorSetHandle();
		write.dstBinding		   = descriptor.bindingNumber;
		write.dstArrayElement	   = 0;
		write.descriptorCount	   = 1;
		write.descriptorType	   = descriptor.type;

		switch ( descriptor.type ) {
			default: break;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
				VkDescriptorBufferInfo *pInfo = &uboBufferInfos[uboCount];
				pInfo->buffer				  = descriptor.bufferInfo.buffer->getBufferHandle();
				pInfo->offset				  = 0;
				pInfo->range				  = VK_WHOLE_SIZE;

				write.pBufferInfo = pInfo;
				writes.push_back( write );

				++uboCount;
			} break;
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
				VkDescriptorImageInfo *pInfo = &textureImageInfos[textureCount];
				pInfo->sampler				 = descriptor.imageInfo.sampler->getSamplerHandle();
				pInfo->imageView			 = descriptor.imageInfo.imageView->getImageViewHandle();
				pInfo->imageLayout			 = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				write.pImageInfo = pInfo;
				writes.push_back( write );

				++textureCount;
			} break;
		}
	}

	if ( !writes.empty() ) {
		CI_VK_DEVICE_FN( UpdateDescriptorSets(
			getDeviceHandle(),
			countU32( writes ),
			dataPtr( writes ),
			0,
			nullptr ) );
	}

	getCurrentCommandBuffer()->bindDescriptorSets(
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mDefaultPipelineLayout,
		0,
		{ getCurrentFrame().currentDrawCall->descriptorSet } );
}

void Context::assignVertexAttributeLocations()
{
	if ( !mShaderProg ) {
		return;
	}

	auto &programVertexAttributes = mShaderProg->getVertexAttributes();

	const uint32_t numBuffers		 = countU32( mVertexBuffers );
	uint32_t	   vertexAttribCount = 0;
	for ( uint32_t i = 0; i < numBuffers; ++i ) {
		const auto &attribs = mVertexBuffers[i].first.getAttribs();

		const uint32_t attribCount = countU32( attribs );
		for ( uint32_t j = 0; j < attribCount; ++j, ++vertexAttribCount ) {
			const auto &attrib		 = attribs[j];
			auto		 &vertexAttrib = mGraphicsState.ia.attributes[vertexAttribCount];

			geom::Attrib semantic = attrib.getAttrib();
			// Find semantic in program vertex attributes
			auto		 it		  = std::find_if(
				  programVertexAttributes.begin(),
				  programVertexAttributes.end(),
				  [semantic]( const vk::InterfaceVariable &elem ) -> bool {
								  bool isSame = ( elem.getSemantic() == semantic );
								  return isSame;
				  } );
			// Skip if shader doesn't use this attribute
			if ( it == programVertexAttributes.end() ) {
				continue;
			}

			const uint32_t location = it->getLocation();

			// VkFormat format = it->getFormat();
			VkFormat format = toVkFormat( attrib );
			vertexAttrib.format( format );
			vertexAttrib.location( location );
			vertexAttrib.offset( static_cast<uint32_t>( attrib.getOffset() ) );
			vertexAttrib.binding( i );
			vertexAttrib.inputRate( VK_VERTEX_INPUT_RATE_VERTEX );
		}
	}
	mGraphicsState.ia.attributeCount = vertexAttribCount;
}

void Context::bindIndexBuffers( const vk::BufferedMeshRef &mesh )
{
	getCurrentCommandBuffer()->bindIndexBuffer( mesh->getIndices(), 0, mesh->getIndexType() );
}

void Context::bindVertexBuffers( const vk::BufferedMeshRef &mesh )
{
	mVertexBuffers = mesh->getVertexBuffers();
	assignVertexAttributeLocations();

	std::vector<vk::BufferRef> buffers;
	for ( auto &it : mVertexBuffers ) {
		buffers.push_back( it.second );
	}

	getCurrentCommandBuffer()->bindVertexBuffers( 0, buffers );
}

void Context::bindGraphicsPipeline( const vk::PipelineLayout *pipelineLayout )
{
	if ( pipelineLayout != nullptr ) {
		mGraphicsState.pipelineLayout = pipelineLayout;
	}
	else {
		mGraphicsState.pipelineLayout = mDefaultPipelineLayout.get();
	}

	uint64_t hash = vk::Pipeline::calculateHash( &mGraphicsState );

	auto it = mGraphicsPipelines.find( hash );
	if ( it == mGraphicsPipelines.end() ) {
		auto pipeline			 = vk::Pipeline::create( mGraphicsState, getDevice() );
		mGraphicsPipelines[hash] = pipeline;
	}

	auto &pipeline = mGraphicsPipelines[hash];
	getCurrentCommandBuffer()->bindPipeline( VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
}

void Context::setDynamicStates( bool force )
{
	if ( mDynamicStates.depthWrite.isDirty() || force ) {
		getCurrentCommandBuffer()->setDepthWriteEnable( mDynamicStates.depthWrite.getValue() );
		mDynamicStates.depthWrite.setClean();
	}

	if ( mDynamicStates.depthTest.isDirty() || force ) {
		getCurrentCommandBuffer()->setDepthTestEnable( mDynamicStates.depthTest.getValue() );
		mDynamicStates.depthTest.setClean();
	}

	if ( mDynamicStates.frontFace.isDirty() || force ) {
		getCurrentCommandBuffer()->setFrontFace( mDynamicStates.frontFace.getValue() );
		mDynamicStates.frontFace.setClean();
	}
}

void Context::draw( int32_t firstVertex, int32_t vertexCount )
{
	setDynamicStates();
	getCurrentCommandBuffer()->draw( static_cast<uint32_t>( vertexCount ), 1, static_cast<uint32_t>( firstVertex ), 0 );

	getCurrentFrame().nextDrawCall( mDefaultSetLayout );
}

void Context::drawIndexed( int32_t firstIndex, int32_t indexCount )
{
	setDynamicStates();
	getCurrentCommandBuffer()->drawIndexed( static_cast<uint32_t>( indexCount ), 1, static_cast<uint32_t>( firstIndex ), 0, 0 );

	getCurrentFrame().nextDrawCall( mDefaultSetLayout );
}

} // namespace cinder::vk
