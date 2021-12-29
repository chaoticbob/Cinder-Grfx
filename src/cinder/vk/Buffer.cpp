#include "cinder/vk/Buffer.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Util.h"
#include "cinder/app/RendererVk.h"

#include <sstream>

namespace cinder::vk {

////////////////////////////////////////////////////////////////////////////////////////////////////
// Buffer

vk::BufferRef Buffer::create( uint64_t size, const vk::Buffer::Usage &bufferUsage, vk::MemoryUsage memoryUsage, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return BufferRef( new Buffer( device, size, bufferUsage, memoryUsage ) );
}

vk::BufferRef Buffer::create( uint64_t size, const void *pData, const vk::Buffer::Usage &bufferUsage, vk::MemoryUsage memoryUsage, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	BufferRef buffer = BufferRef( new Buffer( device, size, bufferUsage, memoryUsage ) );
	if ( buffer ) {
		buffer->copyData( size, pData );
	}
	return buffer;
}

Buffer::Buffer( vk::DeviceRef device, uint64_t size, const vk::Buffer::Usage &bufferUsage, vk::MemoryUsage memoryUsage )
	: vk::DeviceChildObject( device ),
	  mSize( size ),
	  mBufferUsage( bufferUsage ),
	  mMemoryUsage( memoryUsage )
{
	VkBufferUsageFlags usage			= mBufferUsage.mUsage;
	bool			   isIndexBuffer	= ( usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
	bool			   isUniformBuffer	= ( usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT );
	bool			   isVertexBuffer	= ( usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
	bool			   needsTransferDst = isIndexBuffer || isUniformBuffer || isVertexBuffer;
	if ( needsTransferDst ) {
		usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

	VkBufferCreateInfo vkci	   = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	vkci.pNext				   = nullptr;
	vkci.flags				   = 0;
	vkci.size				   = static_cast<VkDeviceSize>( size );
	vkci.usage				   = usage;
	vkci.sharingMode		   = VK_SHARING_MODE_EXCLUSIVE;
	vkci.queueFamilyIndexCount = 0;
	vkci.pQueueFamilyIndices   = nullptr;

	VkResult vkres = CI_VK_DEVICE_FN( CreateBuffer( getDeviceHandle(), &vkci, nullptr, &mBufferHandle ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkCreateBuffer", vkres );
	}

	VmaMemoryUsage vmaUsage = toVmaMemoryUsage( memoryUsage );
	if ( vmaUsage == VMA_MEMORY_USAGE_UNKNOWN ) {
		throw VulkanExc( "unsupported memory type for buffer" );
	}

	VmaAllocationCreateFlags createFlags = 0;
	if ( ( vmaUsage == VMA_MEMORY_USAGE_CPU_ONLY ) || ( vmaUsage == VMA_MEMORY_USAGE_CPU_TO_GPU ) ) {
		createFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}

	VmaAllocationCreateInfo vmaci = {};
	vmaci.flags					  = createFlags;
	vmaci.usage					  = vmaUsage;
	vmaci.requiredFlags			  = 0;
	vmaci.preferredFlags		  = 0;
	vmaci.memoryTypeBits		  = 0;
	vmaci.pool					  = VK_NULL_HANDLE;
	vmaci.pUserData				  = nullptr;

	vkres = vmaAllocateMemoryForBuffer(
		getDevice()->getAllocatorHandle(),
		mBufferHandle,
		&vmaci,
		&mAllocation,
		&mAllocationinfo );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vmaAllocateMemoryForBuffer", vkres );
	}

	vkres = vmaBindBufferMemory(
		getDevice()->getAllocatorHandle(),
		mAllocation,
		mBufferHandle );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vmaBindBufferMemory", vkres );
	}
}

Buffer::~Buffer()
{
	if ( mAllocation != VK_NULL_HANDLE ) {
		vmaFreeMemory( getDevice()->getAllocatorHandle(), mAllocation );
		mAllocation = VK_NULL_HANDLE;
	}

	if ( mBufferHandle != VK_NULL_HANDLE ) {
		CI_VK_DEVICE_FN( DestroyBuffer( getDeviceHandle(), mBufferHandle, nullptr ) );
		mBufferHandle = VK_NULL_HANDLE;
	}
}

void Buffer::map( void **ppMappedAddress )
{
	if ( mMemoryUsage == vk::MemoryUsage::GPU_ONLY ) {
		throw VulkanExc( "GPU ONLY buffers cannot be mapped or unmapped" );
	}

	if ( ppMappedAddress == nullptr ) {
		throw VulkanExc( "unexpected null argument: ppMappedAddress" );
	}

	if ( mMappedAddress != nullptr ) {
		throw VulkanExc( "buffer already mapped" );
	}

	VkResult vkres = vmaMapMemory(
		getDevice()->getAllocatorHandle(),
		mAllocation,
		&mMappedAddress );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vmaMapMemory", vkres );
	}

	*ppMappedAddress = mMappedAddress;
}

void Buffer::unmap()
{
	if ( mMemoryUsage == vk::MemoryUsage::GPU_ONLY ) {
		throw VulkanExc( "GPU ONLY buffers cannot be mapped or unmapped" );
	}

	if ( mMappedAddress == nullptr ) {
		return;
	}

	vmaUnmapMemory( getDevice()->getAllocatorHandle(), mAllocation );
	mMappedAddress = nullptr;
}

void Buffer::copyData( uint64_t size, const void *pData )
{
	getDevice()->copyToBuffer( size, pData, this );
}

void Buffer::ensureMinimumSize( uint64_t minimumSize )
{
	if ( minimumSize > mSize ) {
		throw VulkanExc( "implement me" );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// DynamicBuffer

DynamicBuffer::DynamicBuffer( vk::DeviceRef device, uint64_t size, const vk::Buffer::Usage &bufferUsage )
	: vk::DeviceChildObject( device )
{
	vk::Buffer::Usage cpuUsage = vk::Buffer::Usage().uniformBuffer().transferSrc();
	mCpuBuffer				   = vk::Buffer::create( size, cpuUsage, vk::MemoryUsage::CPU_TO_GPU, device );

	vk::Buffer::Usage gpuUsage = vk::Buffer::Usage().uniformBuffer().transferDst();
	mGpuBuffer				   = vk::Buffer::create( size, cpuUsage, vk::MemoryUsage::GPU_ONLY, device );

	mCpuBuffer->map( &mMappedAddress );
}

DynamicBuffer::~DynamicBuffer()
{
	if ( mMappedAddress != nullptr ) {
		mCpuBuffer->unmap();
		mMappedAddress = nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// UniformBuffer

/*
* 
vk::UniformBufferRef UniformBuffer::create( uint32_t size, const Layout &layout, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return UniformBufferRef( new UniformBuffer( device, size, layout ) );
}

UniformBuffer::UniformBuffer( vk::DeviceRef device, uint32_t size, const Layout &layout )
	: vk::DeviceChildObject( device ),
	  mSize( size ),
	  mLayout( layout )
{
	const uint32_t maxSize = getDevice()->getDeviceLimits().maxUniformBufferRange;
	if ( mSize > maxSize ) {
		std::stringstream ss;
		ss << "uniform buffer size exceeds device limit of " << maxSize;
		throw VulkanExc( ss.str() );
	}

	vk::Buffer::Usage cpuUsage = vk::Buffer::Usage().uniformBuffer().transferSrc();
	mCpuBuffer				   = vk::Buffer::create( size, cpuUsage, vk::MemoryUsage::CPU_TO_GPU, device );

	vk::Buffer::Usage gpuUsage = vk::Buffer::Usage().uniformBuffer().transferDst();
	mGpuBuffer				   = vk::Buffer::create( size, cpuUsage, vk::MemoryUsage::GPU_ONLY, device );

	mCpuBuffer->map( &mMappedAddress );
}

UniformBuffer::~UniformBuffer()
{
	if ( mMappedAddress != nullptr ) {
		mCpuBuffer->unmap();
		mMappedAddress = nullptr;
	}
}

bool UniformBuffer::exists( const std::string &name ) const
{
	auto it	   = mLayout.mUniforms.find( name );
	bool found = ( it != mLayout.mUniforms.end() );
	return found;
}

template <typename T>
void UniformBuffer::uniform( const std::string &name, const T &value )
{
	auto it = mLayout.mUniforms.find( name );
	if ( it == mLayout.mUniforms.end() ) {
		std::stringstream ss;
		ss << "couldn't find uniform variable: " << name;
		throw VulkanExc( ss.str() );
	}

	uint32_t valueSize	 = static_cast<uint32_t>( sizeof( value ) );
	uint32_t startOffset = it->second.offset;
	uint32_t endOffset	 = startOffset + valueSize;
	if ( endOffset > mSize ) {
		std::stringstream ss;
		ss << "end offset (" << endOffset << ") exceeds buffer size (" << mSize << ")";
		throw VulkanExc( ss.str() );
	}

	const char *pSrc = reinterpret_cast<const char *>( &value );
	char *		pDst = static_cast<char *>( mMappedAddress ) + startOffset;
	memcpy( pDst, pSrc, valueSize );
}

void UniformBuffer::uniform( const std::string &name, float value )
{
	uniform<float>( name, value );
}

void UniformBuffer::uniform( const std::string &name, const glm::vec2 &value )
{
	uniform<glm::vec2>( name, value );
}

void UniformBuffer::uniform( const std::string &name, const glm::vec3 &value )
{
	uniform<glm::vec3>( name, value );
}

void UniformBuffer::uniform( const std::string &name, const glm::vec4 &value )
{
	uniform<glm::vec4>( name, value );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat2x2 &value )
{
	uniform<glm::mat2x2>( name, value );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat2x3 &value )
{
	uniform<glm::mat2x3>( name, value );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat2x4 &value )
{
	uniform<glm::mat2x4>( name, value );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat3x2 &value )
{
	uniform<glm::mat3x2>( name, value );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat3x3 &value )
{
	uniform<glm::mat3x3>( name, value );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat3x4 &value )
{
	uniform<glm::mat3x4>( name, value );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat4x2 &value )
{
	uniform<glm::mat4x2>( name, value );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat4x3 &value )
{
	uniform<glm::mat4x3>( name, value );
}

void UniformBuffer::uniform( const std::string &name, const glm::mat4x4 &value )
{
	uniform<glm::mat4x4>( name, value );
}

*/

} // namespace cinder::vk
