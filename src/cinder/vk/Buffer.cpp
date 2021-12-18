#include "cinder/vk/Buffer.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Util.h"
#include "cinder/app/RendererVk.h"

namespace cinder::vk {

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

} // namespace cinder::vk
