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
}

Context::~Context()
{
}

void Context::initializeDescriptorSetLayouts()
{
	vk::DescriptorSetLayout::Options options = vk::DescriptorSetLayout::Options();

	uint32_t bindingNumber = 0;
	for ( size_t i = 0; i < CINDER_CONTEXT_MAX_TEXTURE_COUNT; ++i, ++bindingNumber ) {
		options.addCombinedImageSampler( bindingNumber );
	}

	for ( size_t i = 0; i < CINDER_CONTEXT_MAX_UBO_COUNT; ++i, ++bindingNumber ) {
		options.addUniformBuffer( bindingNumber );
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
											  .addCombinedImageSampler( 10 * CINDER_CONTEXT_MAX_TEXTURE_COUNT )
											  .addUniformBuffer( 10 * CINDER_CONTEXT_MAX_UBO_COUNT );
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

std::pair<ivec2, ivec2> Context::getViewport()
{
	return mViewportStack.back();
}

void Context::bindShaderProg( vk::ShaderProgRef prog )
{
	mShaderProgram = prog;

	mGraphicsState.vert = mShaderProgram->getVertexShader();
	mGraphicsState.frag = mShaderProgram->getFragmentShader();
	mGraphicsState.geom = mShaderProgram->getGeometryShader();
	mGraphicsState.tese = mShaderProgram->getTessellationEvalShader();
	mGraphicsState.tesc = mShaderProgram->getTessellationCtrlShader();

	auto block = mShaderProgram->getDefaultUniformBlock();
	if ( block ) {
		mDescriptorState.bindUniformBuffer( block->getBinding(), mShaderProgram->getDefaultUniformBuffer()->getBindableBuffer() );
	}
}

void Context::bindTexture( const vk::TextureBase *texture, uint32_t binding )
{
	binding += CINDER_CONTEXT_BINDING_SHIFT_TEXTURE;
	mDescriptorState.bindCombinedImageSampler( binding, texture->getSampledImageView(), texture->getSampler() );
}

void Context::unbindTexture( uint32_t binding )
{
	binding += CINDER_CONTEXT_BINDING_SHIFT_TEXTURE;
	mDescriptorState.bindCombinedImageSampler( binding, nullptr, nullptr );
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

//////////////////////////////////////////////////////////////////
// Default shader vars

void Context::setDefaultShaderVars()
{
	if ( !mShaderProgram ) {
		return;
	}

	auto pBuffer = mShaderProgram->getDefaultUniformBuffer();

	const auto &uniforms = mShaderProgram->getDefaultUniformBlock()->getUniforms();
	for ( const auto &uniform : uniforms ) {
		switch ( uniform.getUniformSemantic() ) {
			default: break;
			case UNIFORM_MODEL_MATRIX: {
				auto model = vk::getModelMatrix();
				pBuffer->uniform( uniform.getName(), model );
			} break;
			case UNIFORM_MODEL_MATRIX_INVERSE: {
				auto inverseModel = glm::inverse( vk::getModelMatrix() );
				pBuffer->uniform( uniform.getName(), inverseModel );
			} break;
			case UNIFORM_MODEL_MATRIX_INVERSE_TRANSPOSE: {
				auto modelInverseTranspose = vk::calcModelMatrixInverseTranspose();
				pBuffer->uniform( uniform.getName(), modelInverseTranspose );
			} break;
			case UNIFORM_VIEW_MATRIX: {
				auto view = vk::getViewMatrix();
				pBuffer->uniform( uniform.getName(), view );
			} break;
			case UNIFORM_VIEW_MATRIX_INVERSE: {
				auto viewInverse = vk::calcViewMatrixInverse();
				pBuffer->uniform( uniform.getName(), viewInverse );
			} break;
			case UNIFORM_MODEL_VIEW: {
				auto modelView = vk::getModelView();
				pBuffer->uniform( uniform.getName(), modelView );
			} break;
			case UNIFORM_MODEL_VIEW_INVERSE: {
				auto modelViewInverse = glm::inverse( vk::getModelView() );
				pBuffer->uniform( uniform.getName(), modelViewInverse );
			} break;
			case UNIFORM_MODEL_VIEW_INVERSE_TRANSPOSE: {
				auto normalMatrix = vk::calcNormalMatrix();
				pBuffer->uniform( uniform.getName(), normalMatrix );
			} break;
			case UNIFORM_MODEL_VIEW_PROJECTION: {
				auto modelViewProjection = vk::getModelViewProjection();
				pBuffer->uniform( uniform.getName(), modelViewProjection );
			} break;
			case UNIFORM_MODEL_VIEW_PROJECTION_INVERSE: {
				auto modelViewProjectionInverse = glm::inverse( vk::getModelViewProjection() );
				pBuffer->uniform( uniform.getName(), modelViewProjectionInverse );
			} break;
			case UNIFORM_PROJECTION_MATRIX: {
				auto projection = vk::getProjectionMatrix();
				pBuffer->uniform( uniform.getName(), projection );
			} break;
			case UNIFORM_PROJECTION_MATRIX_INVERSE: {
				auto projectionInverse = glm::inverse( vk::getProjectionMatrix() );
				pBuffer->uniform( uniform.getName(), projectionInverse );
			} break;
			case UNIFORM_VIEW_PROJECTION: {
				auto viewProjection = vk::getProjectionMatrix() * vk::getViewMatrix();
				pBuffer->uniform( uniform.getName(), viewProjection );
			} break;
			case UNIFORM_NORMAL_MATRIX: {
				auto normalMatrix = vk::calcNormalMatrix();
				pBuffer->uniform( uniform.getName(), normalMatrix );
			} break;
			case UNIFORM_VIEWPORT_MATRIX: {
				auto viewport = vk::calcViewportMatrix();
				pBuffer->uniform( uniform.getName(), viewport );
			} break;
			case UNIFORM_WINDOW_SIZE: {
				auto windowSize = app::getWindowSize();
				pBuffer->uniform( uniform.getName(), windowSize );
			} break;
			case UNIFORM_ELAPSED_SECONDS: {
				auto elapsed = float( app::getElapsedSeconds() );
				pBuffer->uniform( uniform.getName(), elapsed );
				break;
			}
		}
	}
}

void Context::clearColorAttachment( uint32_t index )
{
	VkRect2D rect = getCurrentFrame().renderTargets[index]->getArea();

	const ColorA	 &value		 = mClearValues.color;
	VkClearColorValue clearValue = { value.r, value.g, value.b, value.a };

	getCommandBuffer()->clearColorAttachment( index, clearValue, rect );
}

void Context::clearDepthStencilAttachment( VkImageAspectFlags aspectMask )
{
	VkRect2D rect = getCurrentFrame().depthStencil->getArea();

	getCommandBuffer()->clearDepthStencilAttachment( mClearValues.depth, mClearValues.stencil, rect, aspectMask );
}

void Context::bindDefaultDescriptorSet()
{
	std::array<VkDescriptorBufferInfo, CINDER_CONTEXT_MAX_UBO_COUNT>	uboBufferInfos;
	std::array<VkDescriptorImageInfo, CINDER_CONTEXT_MAX_TEXTURE_COUNT> textureImageInfos;
	uint32_t															uboCount	 = 0;
	uint32_t															textureCount = 0;

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

	getCommandBuffer()->bindDescriptorSets(
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mDefaultPipelineLayout,
		0,
		{ getCurrentFrame().currentDrawCall->descriptorSet } );
}

void Context::assignVertexAttributeLocations()
{
	if ( !mShaderProgram ) {
		return;
	}

	auto &programVertexAttributes = mShaderProgram->getVertexAttributes();

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
	getCommandBuffer()->bindIndexBuffer( mesh->getIndices(), 0, mesh->getIndexType() );
}

void Context::bindVertexBuffers( const vk::BufferedMeshRef &mesh )
{
	mVertexBuffers = mesh->getVertexBuffers();
	assignVertexAttributeLocations();

	std::vector<vk::BufferRef> buffers;
	for ( auto &it : mVertexBuffers ) {
		buffers.push_back( it.second );
	}

	getCommandBuffer()->bindVertexBuffers( 0, buffers );
}

void Context::bindGraphicsPipeline()
{
	uint64_t hash = vk::Pipeline::calculateHash( &mGraphicsState );

	auto it = mGraphicsPipelines.find( hash );
	if ( it == mGraphicsPipelines.end() ) {
		auto pipeline			 = vk::Pipeline::create( mGraphicsState, getDevice() );
		mGraphicsPipelines[hash] = pipeline;
	}

	auto &pipeline = mGraphicsPipelines[hash];
	getCommandBuffer()->bindPipeline( VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
}

void Context::setDynamicStates( bool force )
{
	if ( mDynamicStates.depthWrite.isDirty() || force ) {
		getCommandBuffer()->setDepthWriteEnable( mDynamicStates.depthWrite.getValue() );
		mDynamicStates.depthWrite.setClean();
	}

	if ( mDynamicStates.depthTest.isDirty() || force ) {
		getCommandBuffer()->setDepthTestEnable( mDynamicStates.depthTest.getValue() );
		mDynamicStates.depthTest.setClean();
	}

	if ( mDynamicStates.frontFace.isDirty() || force ) {
		getCommandBuffer()->setFrontFace( mDynamicStates.frontFace.getValue() );
		mDynamicStates.frontFace.setClean();
	}
}

void Context::draw( int32_t firstVertex, int32_t vertexCount )
{
	setDynamicStates();
	getCommandBuffer()->draw( static_cast<uint32_t>( vertexCount ), 1, static_cast<uint32_t>( firstVertex ), 0 );

	getCurrentFrame().nextDrawCall( mDefaultSetLayout );
}

void Context::drawIndexed( int32_t firstIndex, int32_t indexCount )
{
	setDynamicStates();
	getCommandBuffer()->drawIndexed( static_cast<uint32_t>( indexCount ), 1, static_cast<uint32_t>( firstIndex ), 0, 0 );

	getCurrentFrame().nextDrawCall( mDefaultSetLayout );
}

} // namespace cinder::vk
