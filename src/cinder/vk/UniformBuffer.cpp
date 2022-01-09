#include "cinder/vk/UniformBuffer.h"
#include "cinder/vk/Context.h"
#include "cinder/app/RendererVk.h"

namespace cinder::vk {

vk::UniformBufferRef UniformBuffer::create( uint32_t size, const vk::UniformBuffer::Options &options, vk::ContextRef context )
{
	if ( !context ) {
		context = app::RendererVk::getCurrentRenderer()->getContext();
	}

	return vk::UniformBufferRef( new vk::UniformBuffer( context, size, options ) );
}

vk::UniformBufferRef UniformBuffer::create( vk::UniformBlockRef uniformBlock, const vk::UniformBuffer::Options &options, vk::ContextRef context )
{
	if ( !context ) {
		context = app::RendererVk::getCurrentRenderer()->getContext();
	}

	return vk::UniformBufferRef( new vk::UniformBuffer( context, uniformBlock, options ) );
}

UniformBuffer::UniformBuffer( vk::ContextRef context, uint32_t size, const vk::UniformBuffer::Options &options )
	: vk::ContextChildObject( context ),
	  mContentMode( options.mContentMode )
{
	initFrames( size );
}

UniformBuffer::UniformBuffer( vk::ContextRef context, vk::UniformBlockRef uniformBlock, const vk::UniformBuffer::Options &options )
	: vk::ContextChildObject( context ),
	  mUniformBlock( uniformBlock ),
	  mContentMode( options.mContentMode )
{
	uint32_t size = uniformBlock->getSize();
	initFrames( size );
}

void UniformBuffer::initFrames( uint32_t size )
{
	uint32_t numFrames = ( mContentMode == vk::ContentMode::DYNAMIC ) ? getContext()->getNumFramesInFlight() : 1;
	for ( uint32_t i = 0; i < numFrames; ++i ) {
		Frame frame = {};

		vk::MutableBuffer::Usage   usage   = vk::MutableBuffer::Usage().uniformBuffer();
		vk::MutableBuffer::Options options = vk::MutableBuffer::Options().persisentMap().cpuOnly();
		frame.buffer					   = vk::MutableBuffer::create( size, usage, options, getContext()->getDevice() );

		mFrames.push_back( frame );
	}
}

vk::UniformBuffer::Frame *UniformBuffer::getCurrentFrame()
{
	uint32_t index = ( mContentMode == vk::ContentMode::DYNAMIC ) ? getContext()->getFrameIndex() : 0;
	auto	 frame = &mFrames[index];
	return frame;
}

const vk::UniformBuffer::Frame *UniformBuffer::getCurrentFrame() const
{
	uint32_t index = ( mContentMode == vk::ContentMode::DYNAMIC ) ? getContext()->getFrameIndex() : 0;
	auto	 frame = &mFrames[index];
	return frame;
}

const vk::Buffer *UniformBuffer::getBindableBuffer() const
{
	auto			  frame	  = getCurrentFrame();
	const vk::Buffer *pBuffer = frame->buffer->isCpuOnly() ? frame->buffer->getCpuBuffer() : frame->buffer->getGpuBuffer();
	return pBuffer;
}

template <typename T>
void UniformBuffer::uniform( const std::string &name, const T &value, size_t size, size_t dims, size_t stride )
{
	if ( !mUniformBlock ) {
		return;
	}

	auto uniform = mUniformBlock->getUniform( name );
	if ( uniform == nullptr ) {
		return;
	}

	void	 *baseAddress = getCurrentFrame()->buffer->getBaseAddress();
	uint32_t offset		 = uniform->getOffset();

	const char *src = reinterpret_cast<const char *>( &value );
	char		 *dst = static_cast<char *>( baseAddress ) + offset;

	for ( size_t i = 0; i < dims; ++i ) {
		memcpy( dst, src, size );
		dst += stride;
		src += size;
	}
}

void UniformBuffer::uniform( const std::string &name, bool value )
{
	uniform<uint32_t>( name, static_cast<uint32_t>( value ), sizeof( uint32_t ), 1, sizeof( uint32_t ) );
}

void UniformBuffer::uniform( const std::string &name, int32_t value )
{
	uniform<int32_t>( name, value, sizeof( int32_t ), 1, sizeof( int32_t ) );
}

void UniformBuffer::uniform( const std::string &name, uint32_t value )
{
	uniform<uint32_t>( name, value, sizeof( uint32_t ), 1, sizeof( uint32_t ) );
}

void UniformBuffer::uniform( const std::string &name, float value )
{
	uniform<float>( name, value, sizeof( float ), 1, sizeof( float ) );
}

void UniformBuffer::uniform( const std::string &name, const glm::vec2 &value )
{
	uniform<glm::vec2>( name, value, sizeof( vec2 ), 1, sizeof( vec2 ) );
}

void UniformBuffer::uniform( const std::string &name, const glm::vec3 &value )
{
	uniform<glm::vec3>( name, value, sizeof( vec3 ), 1, sizeof( vec3 ) );
}

void UniformBuffer::uniform( const std::string &name, const glm::vec4 &value )
{
	uniform<glm::vec4>( name, value, sizeof( vec4 ), 1, sizeof( vec4 ) );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat2x2 &value )
{
	uniform<glm::mat2x2>( name, value, sizeof( vec2 ), 2, 16 );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat2x3 &value )
{
	uniform<glm::mat2x3>( name, value, sizeof( vec3 ), 2, 16 );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat2x4 &value )
{
	uniform<glm::mat2x4>( name, value, sizeof( vec3 ), 2, 16 );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat3x2 &value )
{
	uniform<glm::mat3x2>( name, value, sizeof( vec2 ), 3, 16 );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat3x3 &value )
{
	uniform<glm::mat3x3>( name, value, sizeof( vec3 ), 3, 16 );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat3x4 &value )
{
	uniform<glm::mat3x4>( name, value, sizeof( vec4 ), 3, 16 );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat4x2 &value )
{
	uniform<glm::mat4x2>( name, value, sizeof( vec2 ), 4, 16 );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat4x3 &value )
{
	uniform<glm::mat4x3>( name, value, sizeof( vec3 ), 4, 16 );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat4x4 &value )
{
	uniform<glm::mat4x4>( name, value, sizeof( vec4 ), 4, 16 );
}

} // namespace cinder::vk
