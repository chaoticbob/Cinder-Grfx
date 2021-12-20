#pragma once

#include "cinder/vk/vk_config.h"
#include "cinder/vk/DeviceDispatchTable.h"
#include "cinder/vk/HashKeys.h"

#include <mutex>

#define CI_VK_MINIMUM_STAGING_BUFFER_SIZE ( 64 * 1024 * 1024 )
#define CI_VK_DEFAULT_STAGING_BUFFER_SIZE ( 4 * CI_VK_MINIMUM_STAGING_BUFFER_SIZE )

#define CI_VK_DEVICE_FN( FN_WITH_ARGS ) \
	getDevice()->vkfn()->FN_WITH_ARGS

namespace cinder::vk {

class SubmitInfo
{
public:
	SubmitInfo() {}

	SubmitInfo &addCommandBuffer( const vk::CommandBufferRef &commandBuffer );
	SubmitInfo &addWait( const vk::Semaphore *semaphore, uint64_t value = 0 );
	SubmitInfo &addWait( const vk::SemaphoreRef &semaphore, uint64_t value = 0 ) { return addWait( semaphore.get(), value ); }
	SubmitInfo &addSignal( const vk::Semaphore *semaphore, uint64_t value = 0 );
	SubmitInfo &addSignal( const vk::SemaphoreRef &semaphore, uint64_t value = 0 ) { return addSignal( semaphore.get(), value ); }

private:
	std::vector<VkCommandBuffer>	  mCommandBuffers;
	std::vector<VkSemaphore>		  mWaitSemaphores;
	std::vector<uint64_t>			  mWaitValues;
	std::vector<VkPipelineStageFlags> mWaitDstStageMasks;
	std::vector<VkSemaphore>		  mSignalSemaphores;
	std::vector<uint64_t>			  mSignalValues;

	friend class Device;
};

class Device
	: public std::enable_shared_from_this<Device>
{
private:
public:
	class SamplerCache
	{
	public:
		SamplerCache( vk::Device *pDevice );
		~SamplerCache();

		vk::SamplerRef createSampler( const vk::SamplerHashKey &key );

	private:
		vk::Device *					   mDevice = nullptr;
		std::map<uint64_t, vk::SamplerRef> mSamplerMap;
	};

	class Options
	{
	public:
		Options() {}

		// clang-format off
		Options&	addExtension( const std::string& value ) { mExtensions.push_back(value); return *this; }
		Options&	addExtension( const std::vector<std::string>& value ) { std::copy(value.begin(), value.end(), std::back_inserter(mExtensions)); return *this; }
		Options&	enableComputeQueue( bool value = true ) { mEnableComputeQueue = value; return *this; }
		Options&	enableTransferQueue( bool value = true ) { mEnableTransferQueue = value; return *this; }
		Options&	stagingBufferSize( uint32_t value ) { mStagingBufferSize = std::max<uint32_t>(CI_VK_MINIMUM_STAGING_BUFFER_SIZE, value); return *this; }
		// clang-format on

		const std::vector<std::string> &getExtensions() const { return mExtensions; }
		bool							getEnableComputeQueue() const { return mEnableComputeQueue; }
		bool							getEnableTransferQueue() const { return mEnableTransferQueue; }
		uint32_t						getStagingBufferSize() const { return mStagingBufferSize; }

	private:
		std::vector<std::string> mExtensions;
		bool					 mEnableComputeQueue  = false;
		bool					 mEnableTransferQueue = false;
		uint32_t				 mStagingBufferSize	  = CI_VK_DEFAULT_STAGING_BUFFER_SIZE;
	};

	virtual ~Device();

	static DeviceRef create( VkPhysicalDevice gpuHandle, const Options &options );

	const DeviceDispatchTable *vkfn() const { return &mVkFn; }

	VkInstance						  getInstanceHandle() const;
	VkPhysicalDevice				  getGpuHandle() const;
	const VkPhysicalDeviceProperties &getDeviceProperties() const;
	VkDevice						  getDeviceHandle() const;
	const VkPhysicalDeviceFeatures &  getDeviceFeatures() const;
	std::string						  getDeviceName() const;
	const vk::QueueFamilyIndices &	  getQueueFamilyIndices() const;
	VkQueue							  getGraphicsQueueHandle() const;
	VkQueue							  getComputeQueueHandle() const;
	VkQueue							  getTransferQueueHandle() const;
	VmaAllocator					  getAllocatorHandle() const;

	//! Returns max sample count for render targets
	VkSampleCountFlagBits getMaxRenderTargetSampleCount() const;
	//! Returns max sample count for depth targets
	VkSampleCountFlagBits getMaxDepthTargetSampleCount() const;
	//! Returns max sample count for stencil targets
	VkSampleCountFlagBits getMaxStencilTargetSampleCount() const;
	//! Returns max sample count possible for render, depth, and stencil targets
	VkSampleCountFlagBits getMaxOutputSampleCount() const;

	//! Submit work to graphics queue
	VkResult submitGraphics( const VkSubmitInfo *pSubmitInfo, VkFence fence = VK_NULL_HANDLE, bool waitForIdle = false );
	VkResult submitGraphics( const vk::SubmitInfo &submitInfo, VkFence fence = VK_NULL_HANDLE, bool waitForIdle = false );
	//! Submit work to compute queue
	VkResult submitCompute( const VkSubmitInfo *pSubmitInfo, VkFence fence = VK_NULL_HANDLE, bool waitForIdle = false );
	//! Submit work to transfer queue
	VkResult submitTransfer( const VkSubmitInfo *pSubmitInfo, VkFence fence = VK_NULL_HANDLE, bool waitForIdle = false );

	//! Wait for device to idle
	VkResult waitIdle();
	//! Wait for graphics queue to idle
	VkResult waitIdleGraphics();
	//! Wait for compute queue to idle
	VkResult waitIdleCompute();
	//! Wait for transfer queue to idle
	VkResult waitIdleTransfer();

	void transitionImageLayout(
		VkImage				 image,
		VkImageAspectFlags	 aspectMask,
		uint32_t			 baseMipLevel,
		uint32_t			 levelCount,
		uint32_t			 baseArrayLayer,
		uint32_t			 layerCount,
		VkImageLayout		 oldLayout,
		VkImageLayout		 newLayout,
		VkPipelineStageFlags newPipelineStageFlags );

	//! Use this if the copy to buffer is straight forward
	void copyToBuffer(
		uint64_t	size,
		const void *pSrcData,
		vk::Buffer *pDstBuffer );

	//! Use these if copy requires using mapped pointer from staging buffer as storage
	void *beginCopyToBuffer( uint64_t size, vk::Buffer *pDstBuffer );
	void  endCopyToBuffer( uint64_t size, vk::Buffer *pDstBuffer );

	//! Use this if the copy to image is straight forward
	void copyToImage(
		uint32_t	srcWidth,
		uint32_t	srcHeight,
		uint32_t	srcRowBytes,
		const void *pSrcData,
		vk::Image * pDstImage );

	//! Use these if copy requires using mapped pointer from staging buffer as storage
	void *beginCopyToImage( uint32_t srcWidth, uint32_t srcHeight, uint32_t srcRowBytes, vk::Image *pDstImage );
	void  endCopyToImage( uint32_t srcWidth, uint32_t srcHeight, uint32_t srcRowBytes, vk::Image *pDstImage );

	SamplerCache *getSamplerCache() const { return mSamplerCache.get(); }

	// Use these create/destroy for device object tracking and destruction
	VkResult createFence( const VkFenceCreateInfo *pCreateInfo, VkFence *pFence );
	VkResult createSemaphore( const VkSemaphoreCreateInfo *pCreateInfo, VkSemaphore *pSemaphore );
	void	 destroyFence( VkFence fence );
	void	 destroySemaphore( VkSemaphore semaphore );

private:
	Device( VkPhysicalDevice gpuHandle, const Options &options );

	const Device *getDevice() const { return this; }

	void initializeStagingBuffer();

	void internalCopyToBuffer(
		uint64_t	size,
		vk::Buffer *pSrcBuffer,
		vk::Buffer *pDstBuffer );

	void internalCopyToImage(
		uint32_t	srcWidth,
		uint32_t	srcHeight,
		uint32_t	srcRowBytes,
		vk::Buffer *pSrcBuffer,
		vk::Image * pDstImage );

private:
	DeviceDispatchTable		   mVkFn				= {};
	VkPhysicalDevice		   mGpuHandle			= VK_NULL_HANDLE;
	VkPhysicalDeviceProperties mDeviceProperties	= {};
	VkPhysicalDeviceFeatures   mDeviceFeatures		= {};
	VkDevice				   mDeviceHandle		= VK_NULL_HANDLE;
	vk::QueueFamilyIndices	   mQueueFamilyIndices	= {};
	VkQueue					   mGraphicsQueueHandle = VK_NULL_HANDLE;
	VkQueue					   mComputeQueueHandle	= VK_NULL_HANDLE;
	VkQueue					   mTransferQueueHandle = VK_NULL_HANDLE;
	VmaAllocator			   mVmaAllocatorHandle	= VK_NULL_HANDLE;
	vk::Swapchain *			   mSwapchain			= nullptr;
	std::mutex				   mGraphicsQueueMutex;
	std::mutex				   mComputeQueueMutex;
	std::mutex				   mTransferQueueMutex;
	uint32_t				   mStagingBufferSize = CI_VK_DEFAULT_STAGING_BUFFER_SIZE;
	vk::BufferRef			   mStagingBuffer;
	vk::BufferRef			   mOversizedStagingBuffer;

	static const uint32_t NUM_TRANSIENT_OPERATIONS = 2;
	VkCommandPool		  mTransientCommandPool	   = VK_NULL_HANDLE;
	VkCommandBuffer		  mTransitionCommanBuffer  = VK_NULL_HANDLE;
	VkCommandBuffer		  mCopyCommandBuffer	   = VK_NULL_HANDLE;
	std::mutex			  mTransitionMutex;
	std::mutex			  mCopyMutex;

	std::unique_ptr<SamplerCache> mSamplerCache;
	std::vector<VkFence>		  mFenceHandles;
	std::vector<VkSemaphore>	  mSemaphoreHandles;
};

} // namespace cinder::vk
