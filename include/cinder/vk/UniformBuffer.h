#pragma once

#include "cinder/vk/Buffer.h"
#include "cinder/vk/UniformBlock.h"

namespace cinder::vk {

enum class ContentMode : uint32_t
{
	STATIC	= 1, // All frames in the context uses same content
	DYNAMIC = 2, // Content is updatable for every frame in the context
};

class UniformBuffer
	: public vk::ContextChildObject
{
public:
	struct Options
	{
		Options() {}

		// clang-format off
		Options& contentMode( vk::ContentMode value ) { mContentMode = value; return *this; }
		Options& cpuOnly(bool value = true) { mCpuOnly = value; return *this; }
		// clang-format on

	private:
		vk::ContentMode mContentMode = vk::ContentMode::DYNAMIC;
		bool			mCpuOnly	 = false;

		friend vk::UniformBuffer;
	};

	static vk::UniformBufferRef create( uint32_t size, const vk::UniformBuffer::Options &options = vk::UniformBuffer::Options(), vk::ContextRef context = nullptr );
	static vk::UniformBufferRef create( vk::UniformBlockRef uniformBlock, const vk::UniformBuffer::Options &options = vk::UniformBuffer::Options(), vk::ContextRef context = nullptr );

	virtual ~UniformBuffer() {}

	const vk::UniformBlock *getUniformBlock() const { return mUniformBlock.get(); }

	vk::ContentMode getContentMode() const { return mContentMode; }

	const vk::Buffer *getBindableBuffer() const;

	void uniform( const std::string &name, bool value );
	void uniform( const std::string &name, int32_t value );
	void uniform( const std::string &name, uint32_t value );
	void uniform( const std::string &name, float value );

	void uniform( const std::string &name, const glm::vec2 &value );
	void uniform( const std::string &name, const glm::vec3 &value );
	void uniform( const std::string &name, const glm::vec4 &value );

	void uniform( const std::string &name, const glm::mat2x2 &value );
	void uniform( const std::string &name, const glm::mat2x3 &value );
	void uniform( const std::string &name, const glm::mat2x4 &value );

	void uniform( const std::string &name, const glm::mat3x2 &value );
	void uniform( const std::string &name, const glm::mat3x3 &value );
	void uniform( const std::string &name, const glm::mat3x4 &value );

	void uniform( const std::string &name, const glm::mat4x2 &value );
	void uniform( const std::string &name, const glm::mat4x3 &value );
	void uniform( const std::string &name, const glm::mat4x4 &value );

private:
	UniformBuffer( vk::ContextRef context, uint32_t size, const vk::UniformBuffer::Options &options );
	UniformBuffer( vk::ContextRef context, vk::UniformBlockRef uniformBlock, const vk::UniformBuffer::Options &options );

	void initFrames( uint32_t size );

	struct Frame;
	vk::UniformBuffer::Frame		 *getCurrentFrame();
	const vk::UniformBuffer::Frame *getCurrentFrame() const;

	//! @fn uniform<T>
	//!
	//! @param name   Name of uniform
	//! @param value  Uniform's value of type T
	//! @param size   Size of vector or scalar
	//! @param dims   Number of dimensions to copy (matrix only)
	//! @param stfdie Stride of dimension (matrix only)
	//!
	template <typename T>
	void uniform( const std::string &name, const T &value, size_t size, size_t dims, size_t stride );

private:
	struct Frame
	{
		vk::MutableBufferRef buffer;
	};

	vk::UniformBlockRef mUniformBlock;
	vk::ContentMode		mContentMode;
	std::vector<Frame>	mFrames;
};

} // namespace cinder::vk
