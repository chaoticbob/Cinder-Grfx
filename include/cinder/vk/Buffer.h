#pragma once

#include "cinder/vk/DeviceChildObject.h"

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
		Usage( VkBufferUsageFlags usage = 0 )
			: mUsage( usage ) {}

		// clang-format off
		Usage& transferSrc(bool enable = true) { mUsage |= enable ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0; return *this; }
		Usage& transferDst(bool enable = true) { mUsage |= enable ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0; return *this; }
		Usage& uniformTexelBuffer(bool enable = true) { mUsage |= enable ? VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT : 0; return *this; }
		Usage& storageTexelBuffer(bool enable = true) { mUsage |= enable ? VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT : 0; return *this; }
		Usage& uniformBuffer(bool enable = true) { mUsage |= enable ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 0; return *this; }
		Usage& storageBuffer(bool enable = true) { mUsage |= enable ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0; return *this; }
		Usage& indexBuffer(bool enable = true) { mUsage |= enable ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0; return *this; }
		Usage& vertexBuffer(bool enable = true) { mUsage |= enable ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0; return *this; }
		Usage& indirectBuffer(bool enable = true) { mUsage |= enable ? VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT : 0; return *this; }
		// clang-format on

	private:
		VkBufferUsageFlags mUsage = 0;

		friend class Buffer;
	};

	virtual ~Buffer();

	static vk::BufferRef create( uint64_t size, const vk::Buffer::Usage &bufferUsage, vk::MemoryUsage memoryUsage = vk::MemoryUsage::GPU_ONLY, vk::DeviceRef device = vk::DeviceRef() );

	static vk::BufferRef create( uint64_t size, const void *pData, const vk::Buffer::Usage &bufferUsage, vk::MemoryUsage memoryUsage = vk::MemoryUsage::GPU_ONLY, vk::DeviceRef device = vk::DeviceRef() );

	uint64_t getSize() const { return mSize; }

	const Buffer::Usage &getBufferUsage() const { return mBufferUsage; }

	MemoryUsage getMemoryUsage() const { return mMemoryUsage; }

	VkBuffer getBufferHandle() const { return mBufferHandle; }

	void map( void **ppMappedAddress );

	void unmap();

	void copyData( uint64_t size, const void *pData );

private:
	Buffer( vk::DeviceRef device, uint64_t size, const vk::Buffer::Usage &bufferUsage, vk::MemoryUsage memoryUsage );

private:
	uint64_t		  mSize;
	Buffer::Usage	  mBufferUsage;
	MemoryUsage		  mMemoryUsage;
	VkBuffer		  mBufferHandle	  = VK_NULL_HANDLE;
	VmaAllocation	  mAllocation	  = VK_NULL_HANDLE;
	VmaAllocationInfo mAllocationinfo = {};
	void *			  mMappedAddress  = nullptr;
};

} // namespace cinder::vk
