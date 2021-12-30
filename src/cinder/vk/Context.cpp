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
#include "cinder/vk/UniformBlock.h"
#include "cinder/vk/Util.h"
#include "cinder/vk/wrapper.h"
#include "cinder/app/App.h"
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
// Context::DescriptorState

Context::DescriptorState::DescriptorState()
{
}

void Context::DescriptorState::bindUniformBuffer( uint32_t bindingNumber, vk::Buffer *buffer )
{
	//Descriptor &descriptor		 = mSets[setNumber][bindingNumber];
	//descriptor.type				 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	//descriptor.bufferInfo.buffer = buffer;

	mDescriptors.insert_or_assign( bindingNumber, Descriptor( bindingNumber, buffer ) );
}

void Context::DescriptorState::bindCombinedImageSampler( uint32_t bindingNumber, vk::ImageView *imageView, vk::Sampler *sampler )
{
	//Descriptor &descriptor		   = mSets[setNumber][bindingNumber];
	//descriptor.type				   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//descriptor.imageInfo.imageView = imageView;
	//descriptor.imageInfo.sampler   = sampler;

	mDescriptors.insert_or_assign( bindingNumber, Descriptor( bindingNumber, imageView, sampler ) );
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

	initializeDescriptorSetLayouts();
	initializePipelineLayout();

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

	// Descriptor stuff
	{
		vk::DescriptorPool::Options options = vk::DescriptorPool::Options()
												  .addCombinedImageSampler( CINDER_CONTEXT_MAX_TEXTURE_COUNT )
												  .addUniformBuffer( CINDER_CONTEXT_MAX_UBO_COUNT );
		frame.descriptorPool = vk::DescriptorPool::create( options, getDevice() );

		frame.descriptorSet = vk::DescriptorSet::create( frame.descriptorPool, mDefaultSetLayout );
	}

	// Default uniform buffer
	{
		vk::Buffer::Usage usage = vk::Buffer::Usage().uniformBuffer();

		frame.defaultUniformBuffer = vk::Buffer::create( 1024, usage, vk::MemoryUsage::CPU_ONLY, getDevice() );
	}

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

		frame.commandBuffer->setViewport( 0, 0, static_cast<float>( mWidth ), static_cast<float>( mHeight ) );
		frame.commandBuffer->setScissor( 0, 0, mWidth, mHeight );
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

std::pair<ivec2, ivec2> Context::getViewport()
{
	//if( mViewportStack.empty() ) {
	//	GLint params[4];
	//	glGetIntegerv( GL_VIEWPORT, params );
	//	// push twice in anticipation of later pop
	//	mViewportStack.push_back( std::pair<ivec2, ivec2>( ivec2( params[0], params[1] ), ivec2( params[2], params[3] ) ) );
	//	mViewportStack.push_back( std::pair<ivec2, ivec2>( ivec2( params[0], params[1] ), ivec2( params[2], params[3] ) ) );
	//}

	return mViewportStack.back();
}

void Context::bindShaderProg( vk::ShaderProgRef prog )
{
	mShaderProgram = prog;

	auto block = mShaderProgram->getDefaultUniformBlock();
	if ( block ) {
		mDescriptorState.bindUniformBuffer( block->getBinding(), getCurrentFrame().defaultUniformBuffer.get() );
	}
}

void Context::bindTexture( uint32_t binding, vk::ImageView *imageView, vk::Sampler *sampler )
{
	binding += CINDER_CONTEXT_BINDING_SHIFT_TEXTURE;
	mDescriptorState.bindCombinedImageSampler( binding, imageView, sampler );
}

void Context::unbindTexture( uint32_t binding )
{
	binding += CINDER_CONTEXT_BINDING_SHIFT_TEXTURE;
	mDescriptorState.bindCombinedImageSampler( binding, nullptr, nullptr );
}

void Context::setDefaultShaderVars()
{
	if ( !mShaderProgram ) {
		return;
	}

	vk::Buffer *pBuffer = getCurrentFrame().defaultUniformBuffer.get();

	char *pMappedAddress = nullptr;
	pBuffer->map( reinterpret_cast<void **>( &pMappedAddress ) );

	const auto &uniforms = mShaderProgram->getDefaultUniformBlock()->getUniforms();
	for ( const auto &uniform : uniforms ) {
		const size_t offset = uniform.getOffset();
		char *		 dst	= pMappedAddress + offset;

		switch ( uniform.getUniformSemantic() ) {
			default: break;
			case UNIFORM_MODEL_MATRIX: {
				auto model = vk::getModelMatrix();
				memcpy( dst, &model, sizeof( model ) );
			} break;
			case UNIFORM_MODEL_MATRIX_INVERSE: {
				auto inverseModel = glm::inverse( vk::getModelMatrix() );
				memcpy( dst, &inverseModel, sizeof( inverseModel ) );
			} break;
			case UNIFORM_MODEL_MATRIX_INVERSE_TRANSPOSE: {
				auto modelInverseTranspose = vk::calcModelMatrixInverseTranspose();
				memcpy( dst, &modelInverseTranspose, sizeof( modelInverseTranspose ) );
			} break;
			case UNIFORM_VIEW_MATRIX: {
				auto view = vk::getViewMatrix();
				memcpy( dst, &view, sizeof( view ) );
			} break;
			case UNIFORM_VIEW_MATRIX_INVERSE: {
				auto viewInverse = vk::calcViewMatrixInverse();
				memcpy( dst, &viewInverse, sizeof( viewInverse ) );
			} break;
			case UNIFORM_MODEL_VIEW: {
				auto modelView = vk::getModelView();
				memcpy( dst, &modelView, sizeof( modelView ) );
			} break;
			case UNIFORM_MODEL_VIEW_INVERSE: {
				auto modelViewInverse = glm::inverse( vk::getModelView() );
				memcpy( dst, &modelViewInverse, sizeof( modelViewInverse ) );
			} break;
			case UNIFORM_MODEL_VIEW_INVERSE_TRANSPOSE: {
				auto normalMatrix = vk::calcNormalMatrix();
				memcpy( dst, &normalMatrix, sizeof( normalMatrix ) );
			} break;
			case UNIFORM_MODEL_VIEW_PROJECTION: {
				auto modelViewProjection = vk::getModelViewProjection();
				memcpy( dst, &modelViewProjection, sizeof( modelViewProjection ) );
			} break;
			case UNIFORM_MODEL_VIEW_PROJECTION_INVERSE: {
				auto modelViewProjectionInverse = glm::inverse( vk::getModelViewProjection() );
				memcpy( dst, &modelViewProjectionInverse, sizeof( modelViewProjectionInverse ) );
			} break;
			case UNIFORM_PROJECTION_MATRIX: {
				auto projection = vk::getProjectionMatrix();
				memcpy( dst, &projection, sizeof( projection ) );
			} break;
			case UNIFORM_PROJECTION_MATRIX_INVERSE: {
				auto projectionInverse = glm::inverse( vk::getProjectionMatrix() );
				memcpy( dst, &projectionInverse, sizeof( projectionInverse ) );
			} break;
			case UNIFORM_VIEW_PROJECTION: {
				auto viewProjection = vk::getProjectionMatrix() * vk::getViewMatrix();
				memcpy( dst, &viewProjection, sizeof( viewProjection ) );
			} break;
			case UNIFORM_NORMAL_MATRIX: {
				auto		normalMatrix = vk::calcNormalMatrix();
				const char *src			 = reinterpret_cast<const char *>( &normalMatrix );
				memcpy( dst + 0 * 16, src + 0 * 12, 12 );
				memcpy( dst + 1 * 16, src + 1 * 12, 12 );
				memcpy( dst + 2 * 16, src + 2 * 12, 12 );
				//memcpy( dst, &normalMatrix, sizeof( normalMatrix ) );
			} break;
			case UNIFORM_VIEWPORT_MATRIX: {
				auto viewport = vk::calcViewportMatrix();
				memcpy( dst, &viewport, sizeof( viewport ) );
			} break;
			case UNIFORM_WINDOW_SIZE: {
				auto windowSize = app::getWindowSize();
				memcpy( dst, &windowSize, sizeof( windowSize ) );
			} break;
			case UNIFORM_ELAPSED_SECONDS: {
				auto elapsed = float( app::getElapsedSeconds() );
				memcpy( dst, &elapsed, sizeof( elapsed ) );
				break;
			}
		}
	}

	pBuffer->unmap();
}

void Context::clearColorAttachment( uint32_t index )
{
	VkRect2D rect = getCurrentFrame().renderTargets[index]->getArea();

	const ColorA &	  value		 = mClearValues.color;
	VkClearColorValue clearValue = { value.r, value.g, value.b, value.a };

	getCommandBuffer()->clearColorAttachment( index, clearValue, rect );
}

void Context::clearDepthStencilAttachment( VkImageAspectFlags aspectMask )
{
	VkRect2D rect = getCurrentFrame().depthTarget->getArea();

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
		write.dstSet			   = getCurrentFrame().descriptorSet->getDescriptorSetHandle();
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
		{ getCurrentFrame().descriptorSet } );
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
			auto &		vertexAttrib = mGraphicsState.ia.attributes[vertexAttribCount];

			geom::Attrib semantic = attrib.getAttrib();
			// Find semantic in program vertex attributes
			auto it = std::find_if(
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

			//VkFormat format = it->getFormat();
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
	if ( !mGraphicsPipeline ) {
		mGraphicsState.ia.topology			   = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		mGraphicsState.rs.rasterizationSamples = mSampleCount;
		mGraphicsState.ds.depthCompareOp	   = VK_COMPARE_OP_LESS_OR_EQUAL;
		mGraphicsState.om.renderTargetCount	   = 1;
		mGraphicsState.om.renderTargets[0]	   = mRenderTargetFormats[0];
		mGraphicsState.om.depthStencil		   = mDepthFormat;
		mGraphicsState.cb.attachmentCount	   = 1;
		mGraphicsState.cb.attachments[0]	   = {};

		vk::Pipeline::Options options = vk::Pipeline::Options( mGraphicsState );

		mGraphicsPipeline = vk::Pipeline::create(
			mShaderProgram,
			mDefaultPipelineLayout,
			options,
			getDevice() );
	}

	getCommandBuffer()->bindPipeline( VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline );
}

void Context::draw( int32_t firstVertex, int32_t vertexCount )
{
	getCommandBuffer()->draw( static_cast<uint32_t>( vertexCount ), 1, static_cast<uint32_t>( firstVertex ), 0 );
}

void Context::drawIndexed( int32_t firstIndex, int32_t indexCount )
{
	getCommandBuffer()->drawIndexed( static_cast<uint32_t>( indexCount ), 1, static_cast<uint32_t>( firstIndex ), 0, 0 );
}

} // namespace cinder::vk
