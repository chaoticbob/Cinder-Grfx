#pragma once

#include "cinder/vk/ChildObject.h"

namespace cinder::vk {

//! @class Buffer
//!
//! Index, uniform, and vertex buffers will automatically have
//! VK_BUFFER_USAGE_TRANSFER_DST_BIT added to buffer usage.
//!
class Buffer
	: public vk::DeviceChildObject
{
public:
	struct Usage
	{
		Usage( VkBufferUsageFlags flags = 0 )
			: mFlags( flags ) {}

		// clang-format off
		Usage& transferSrc(bool enable = true) { mBits.transferSrc = enable; return *this; }
		Usage& transferDst(bool enable = true) { mBits.transferDst = enable; return *this; }
		Usage& uniformTexelBuffer(bool enable = true) { mBits.uniformTexelBuffer = enable; return *this; }
		Usage& storageTexelBuffer(bool enable = true) { mBits.storageTexelBuffer = enable; return *this; }
		Usage& uniformBuffer(bool enable = true) { mBits.uniformBuffer = enable; return *this; }
		Usage& storageBuffer(bool enable = true) { mBits.storageBuffer = enable; return *this; }
		Usage& indexBuffer(bool enable = true) { mBits.indexBuffer = enable; return *this; }
		Usage& vertexBuffer(bool enable = true) { mBits.vertexBuffer = enable; return *this; }
		Usage& indirectBuffer(bool enable = true) { mBits.indirectBuffer = enable; return *this; }
		// clang-format on

	private:
		union
		{
			struct
			{
				bool transferSrc		: 1;
				bool transferDst		: 1;
				bool uniformTexelBuffer : 1;
				bool storageTexelBuffer : 1;
				bool uniformBuffer		: 1;
				bool storageBuffer		: 1;
				bool indexBuffer		: 1;
				bool vertexBuffer		: 1;
				bool indirectBuffer		: 1;
			} mBits;
			VkBufferUsageFlags mFlags = 0;
		};

		friend class vk::Buffer;
	};

	struct Options
	{
		Options() {}

		// clang-format off
		//! Persistently maps any non GPU_ONLY buffers, map() returns mapped address, unmap() has no effect
		Options& persisentMap(bool value = true) { mPersistentMap = value; return *this; }
		// clang-format on

	private:
		bool mPersistentMap = false;

		friend class vk::Buffer;
	};

	virtual ~Buffer();

	static vk::BufferRef create( uint64_t size, const vk::Buffer::Usage &usage, vk::MemoryUsage memoryUsage = vk::MemoryUsage::GPU_ONLY, const vk::Buffer::Options &options = vk::Buffer::Options(), vk::DeviceRef device = nullptr );

	static vk::BufferRef create( uint64_t size, const void *pData, const vk::Buffer::Usage &usage, vk::MemoryUsage memoryUsage = vk::MemoryUsage::GPU_ONLY, const vk::Buffer::Options &options = vk::Buffer::Options(), vk::DeviceRef device = nullptr );

	uint64_t getSize() const { return mSize; }

	const vk::Buffer::Usage &getUsage() const { return mUsage; }

	vk::MemoryUsage getMemoryUsage() const { return mMemoryUsage; }

	VkBuffer getBufferHandle() const { return mBufferHandle; }

	bool isMapped() const { return ( mMappedAddress != nullptr ); }

	void map( void **ppMappedAddress );

	void unmap();

	void copyData( uint64_t size, const void *pData );

	void ensureMinimumSize( uint64_t minimumSize );

private:
	Buffer( vk::DeviceRef device, uint64_t size, const vk::Buffer::Usage &usage, vk::MemoryUsage memoryUsage, const vk::Buffer::Options &options = vk::Buffer::Options() );

private:
	uint64_t			mSize			= 0;
	vk::Buffer::Usage	mUsage			= {};
	vk::Buffer::Options mOptions		= {};
	vk::MemoryUsage		mMemoryUsage	= vk::MemoryUsage::UNKNOWN;
	VkBuffer			mBufferHandle	= VK_NULL_HANDLE;
	VmaAllocation		mAllocation		= VK_NULL_HANDLE;
	VmaAllocationInfo	mAllocationinfo = {};
	void			   *mMappedAddress	= nullptr;
};

//! @class MutableBuffer
//!
//!
class MutableBuffer
	: public vk::DeviceChildObject
{
public:
	struct Usage
	{
		Usage( VkBufferUsageFlags flags = 0 )
			: mFlags( flags ) {}

		// clang-format off
		Usage& transferSrc(bool enable = true) { mBits.transferSrc = enable; return *this; }
		Usage& transferDst(bool enable = true) { mBits.transferDst = enable; return *this; }
		Usage& uniformTexelBuffer(bool enable = true) { mBits.uniformTexelBuffer = enable; return *this; }
		Usage& storageTexelBuffer(bool enable = true) { mBits.storageTexelBuffer = enable; return *this; }
		Usage& uniformBuffer(bool enable = true) { mBits.uniformBuffer = enable; return *this; }
		Usage& storageBuffer(bool enable = true) { mBits.storageBuffer = enable; return *this; }
		Usage& indexBuffer(bool enable = true) { mBits.indexBuffer = enable; return *this; }
		Usage& vertexBuffer(bool enable = true) { mBits.vertexBuffer = enable; return *this; }
		Usage& indirectBuffer(bool enable = true) { mBits.indirectBuffer = enable; return *this; }
		// clang-format on

	private:
		union
		{
			struct
			{
				bool transferSrc		: 1;
				bool transferDst		: 1;
				bool uniformTexelBuffer : 1;
				bool storageTexelBuffer : 1;
				bool uniformBuffer		: 1;
				bool storageBuffer		: 1;
				bool indexBuffer		: 1;
				bool vertexBuffer		: 1;
				bool indirectBuffer		: 1;
			} mBits;
			VkBufferUsageFlags mFlags = 0;
		};

		friend class vk::MutableBuffer;
	};

	struct Options
	{
		Options() {}

		// clang-format off
		Options& persisentMap(bool value = true) { mPersistentMap = value; return *this; }
		Options &cpuOnly( bool value = true ) { mCpuOnly = value; return *this; }
		// clang-format on

	private:
		bool mPersistentMap = false;
		bool mCpuOnly		= false; // Sometimes it's useful to put a MutableBuffer to CPU only

		friend class vk::MutableBuffer;
	};

	virtual ~MutableBuffer();

	static vk::MutableBufferRef create( uint64_t size, const vk::MutableBuffer::Usage &usage, const vk::MutableBuffer::Options &options = vk::MutableBuffer::Options(), vk::DeviceRef device = nullptr );

	static vk::MutableBufferRef create( uint64_t size, const void *pData, const vk::MutableBuffer::Usage &usage, const vk::MutableBuffer::Options &options = vk::MutableBuffer::Options(), vk::DeviceRef device = nullptr );

	uint64_t getSize() const { return mSize; }

	const vk::MutableBuffer::Usage &getUsage() const { return mUsage; }

	bool isCpuOnly() const { return mOptions.mCpuOnly; }

	const vk::Buffer *getCpuBuffer() const { return mCpuBuffer.get(); }

	const vk::Buffer *getGpuBuffer() const { return mGpuBuffer.get(); }

	void *getBaseAddress() const { return mMappedAddress; }

	//! PERFORMANCE WARNING: This will copy to both CPU and also GPU (if not CPU only).
	void copyData( uint64_t size, const void *pData );

	void ensureMinimumSize( uint64_t minimumSize );

private:
	MutableBuffer( vk::DeviceRef device, uint64_t size, const vk::MutableBuffer::Usage &usage, const vk::MutableBuffer::Options &options );

private:
	uint64_t				   mSize = 0;
	vk::MutableBuffer::Usage   mUsage;
	vk::MutableBuffer::Options mOptions;
	vk::BufferRef			   mCpuBuffer;
	vk::BufferRef			   mGpuBuffer;
	void					  *mMappedAddress = nullptr;
};

/*
//! @class UniformBuffer
//!
//!
class UniformBuffer
	: public vk::DeviceChildObject
{
public:
	// clang-format off
	template <typename T> struct LookUpVariableType    { static const VariableType type = VariableType::UNKNOWN; };
	template <> struct LookUpVariableType<float>       { static const VariableType type = VariableType::FLOAT_1; };
	template <> struct LookUpVariableType<glm::vec2>   { static const VariableType type = VariableType::FLOAT_2; };
	template <> struct LookUpVariableType<glm::vec3>   { static const VariableType type = VariableType::FLOAT_3; };
	template <> struct LookUpVariableType<glm::vec4>   { static const VariableType type = VariableType::FLOAT_4; };
	template <> struct LookUpVariableType<glm::mat2>   { static const VariableType type = VariableType::FLOAT_2x2; };
	template <> struct LookUpVariableType<glm::mat2x3> { static const VariableType type = VariableType::FLOAT_2x3; };
	template <> struct LookUpVariableType<glm::mat2x4> { static const VariableType type = VariableType::FLOAT_2x4; };
	template <> struct LookUpVariableType<glm::mat3x2> { static const VariableType type = VariableType::FLOAT_3x2; };
	template <> struct LookUpVariableType<glm::mat3x3> { static const VariableType type = VariableType::FLOAT_3x3; };
	template <> struct LookUpVariableType<glm::mat3x4> { static const VariableType type = VariableType::FLOAT_3x4; };
	template <> struct LookUpVariableType<glm::mat4x2> { static const VariableType type = VariableType::FLOAT_4x2; };
	template <> struct LookUpVariableType<glm::mat4x3> { static const VariableType type = VariableType::FLOAT_4x3; };
	template <> struct LookUpVariableType<glm::mat4x4> { static const VariableType type = VariableType::FLOAT_4x4; };
	// clang-format on

	struct Uniform
	{
		vk::DataType dataType;
		uint32_t	 offset;
	};

	struct Layout
	{
		Layout() {}

		template <typename T>
		void add( const std::string &name, uint32_t offset )
		{
			Uniform uniform = {};
			uniform.type	= LookUpVariableType<T>::type;
			uniform.offset	= offset;
			mUniforms[name] = uniform;
		}

		// clang-format off
		Layout &addFloat1( const std::string& name, uint32_t offset ) { add<float>( name, offset ); return *this; }
		Layout &addFloat2( const std::string& name, uint32_t offset ) { add<glm::vec2>( name, offset ); return *this; }
		Layout &addFloat3( const std::string& name, uint32_t offset ) { add<glm::vec3>( name, offset ); return *this; }
		Layout &addFloat4( const std::string& name, uint32_t offset ) { add<glm::vec4>( name, offset ); return *this; }
		Layout &addMatrix2x2( const std::string& name, uint32_t offset ) { add<glm::mat2x2>( name, offset ); return *this; }
		Layout &addMatrix2x3( const std::string& name, uint32_t offset ) { add<glm::mat2x3>( name, offset ); return *this; }
		Layout &addMatrix2x4( const std::string& name, uint32_t offset ) { add<glm::mat2x4>( name, offset ); return *this; }
		Layout &addMatrix3x2( const std::string& name, uint32_t offset ) { add<glm::mat3x2>( name, offset ); return *this; }
		Layout &addMatrix3x3( const std::string& name, uint32_t offset ) { add<glm::mat3x3>( name, offset ); return *this; }
		Layout &addMatrix3x4( const std::string& name, uint32_t offset ) { add<glm::mat3x4>( name, offset ); return *this; }
		Layout &addMatrix4x2( const std::string& name, uint32_t offset ) { add<glm::mat4x2>( name, offset ); return *this; }
		Layout &addMatrix4x3( const std::string& name, uint32_t offset ) { add<glm::mat4x3>( name, offset ); return *this; }
		Layout &addMatrix4x4( const std::string& name, uint32_t offset ) { add<glm::mat4x4>( name, offset ); return *this; }
		// clang-format on

	private:
		std::map<std::string, Uniform> mUniforms;

		friend class UniformBuffer;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////

	~UniformBuffer();

	static vk::UniformBufferRef create( uint32_t size, const Layout &layout, vk::DeviceRef device = nullptr );

	bool exists( const std::string &name ) const;

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
	UniformBuffer( vk::DeviceRef device, uint32_t size, const Layout &layout );

	template <typename T>
	void uniform( const std::string &name, const T &value );

private:
	uint32_t	  mSize = 0;
	Layout		  mLayout;
	vk::BufferRef mCpuBuffer;
	vk::BufferRef mGpuBuffer;
	void *		  mMappedAddress = nullptr;
};
*/

} // namespace cinder::vk
