#include "cinder/vk/Device.h"
#include "cinder/vk/Buffer.h"
#include "cinder/vk/Command.h"
#include "cinder/vk/Environment.h"
#include "cinder/vk/Image.h"
#include "cinder/vk/Sampler.h"
#include "cinder/vk/Sync.h"
#include "cinder/vk/Util.h"
#include "cinder/app/AppBase.h"

#define VK_NO_PROTOTYPES
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "xxh3.h"

#include <algorithm>

namespace cinder::vk {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SubmitInfo

SubmitInfo &SubmitInfo::addCommandBuffer( const vk::CommandBufferRef &commandBuffer )
{
	mCommandBuffers.push_back( commandBuffer->getCommandBufferHandle() );
	return *this;
}

SubmitInfo &SubmitInfo::addWait( const vk::Semaphore *semaphore, uint64_t value )
{
	mWaitSemaphores.push_back( semaphore->getSemaphoreHandle() );
	mWaitValues.push_back( value );
	mWaitDstStageMasks.push_back( VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
	return *this;
}

SubmitInfo &SubmitInfo::addSignal( const vk::Semaphore *semaphore, uint64_t value )
{
	mSignalSemaphores.push_back( semaphore->getSemaphoreHandle() );
	mSignalValues.push_back( value );
	return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device::SamplerCache

Device::SamplerCache::SamplerCache( vk::Device *pDevice )
	: mDevice( pDevice )
{
}

Device::SamplerCache::~SamplerCache()
{
}

vk::SamplerRef Device::SamplerCache::createSampler( const vk::SamplerHashKey &key )
{
	// Do not trust the input desc is properly zero'd out.
	vk::SamplerHashKey szKey;

	// Initialize to zero
	size_t size = sizeof( szKey );
	memset( &szKey, 0, size );

	szKey.magFilter				  = key.magFilter;
	szKey.minFilter				  = key.minFilter;
	szKey.mipmapMode			  = key.mipmapMode;
	szKey.addressModeU			  = key.addressModeU;
	szKey.addressModeV			  = key.addressModeV;
	szKey.addressModeW			  = key.addressModeW;
	szKey.mipLodBias			  = key.mipLodBias;
	szKey.maxAnisotropy			  = key.maxAnisotropy;
	szKey.compareOp				  = key.compareOp;
	szKey.minLod				  = key.minLod;
	szKey.maxLod				  = key.maxLod;
	szKey.borderColor			  = key.borderColor;
	szKey.unnormalizedCoordinates = key.unnormalizedCoordinates;

	uint64_t hash = static_cast<uint64_t>( XXH64( &szKey, size, 0x4cffabac5e25a3ac ) );
	auto	 it	  = mSamplerMap.find( hash );
	if ( it != mSamplerMap.end() ) {
		return it->second;
	}

	vk::Sampler::Options options = vk::Sampler::Options();
	options.magFilter( szKey.magFilter );
	options.minFilter( szKey.minFilter );
	options.mipmapMode( szKey.mipmapMode );
	options.addressModeU( szKey.addressModeU );
	options.addressModeV( szKey.addressModeV );
	options.addressModeW( szKey.addressModeW );
	options.mipLodBias( static_cast<float>( szKey.mipLodBias ) );
	options.maxAnisotropy( static_cast<float>( szKey.maxAnisotropy ) );
	options.compareOp( szKey.compareOp );
	options.minLod( static_cast<float>( szKey.minLod ) );
	options.maxLod( static_cast<float>( szKey.maxLod ) );
	options.borderColor( szKey.borderColor );
	options.unnormalizedCoordinates( szKey.unnormalizedCoordinates );

	return vk::Sampler::create( options, mDevice->shared_from_this() );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device

struct ExtensionFeatures
{
	void *											 pNextStart			   = nullptr;
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT	 descriptorIndexing	   = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT };
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT	 extendedDynamciState  = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT };
	VkPhysicalDeviceExtendedDynamicState2FeaturesEXT extendedDynamciState2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT };
	VkPhysicalDeviceExternalMemoryHostPropertiesEXT	 externalMemoryHost	   = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT };
	VkPhysicalDeviceDynamicRenderingFeaturesKHR		 dynamicRendering	   = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR };
	VkPhysicalDeviceTimelineSemaphoreFeaturesKHR	 timelineSemaphore	   = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR };

	ExtensionFeatures( uint32_t apiVersion = VK_API_VERSION_1_1 )
	{
		if ( apiVersion == VK_API_VERSION_1_1 ) {
			pNextStart					= &descriptorIndexing;
			descriptorIndexing.pNext	= &extendedDynamciState;
			extendedDynamciState.pNext	= &extendedDynamciState2;
			extendedDynamciState2.pNext = &externalMemoryHost;
			externalMemoryHost.pNext	= &dynamicRendering;
			dynamicRendering.pNext		= &timelineSemaphore;
			timelineSemaphore.pNext		= nullptr;
		}
		else if ( apiVersion == VK_API_VERSION_1_2 ) {
			pNextStart					= &descriptorIndexing;
			descriptorIndexing.pNext	= &extendedDynamciState;
			extendedDynamciState.pNext	= &extendedDynamciState2;
			extendedDynamciState2.pNext = &externalMemoryHost;
			externalMemoryHost.pNext	= &dynamicRendering;
			dynamicRendering.pNext		= nullptr;
		}
	}
};

static void configureExtensions(
	uint32_t				   apiVersion,
	VkPhysicalDevice		   gpuHandle,
	const Device::Options &	   options,
	std::vector<const char *> &extensions )
{
	// Required extensions for Vulkan 1.1
	if ( apiVersion == VK_API_VERSION_1_1 ) {
		extensions.push_back( VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME );
		extensions.push_back( VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME ); // No pNext chain
		extensions.push_back( VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME );
	}
	// Required extensions for Vulkan 1.2
	else if ( apiVersion == VK_API_VERSION_1_2 ) {
	}

	// Extensions require regardless of Vulkan API version
	{
		extensions.push_back( VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME );		// No participation in VkDeviceCreateInfo::pNext chain
		extensions.push_back( VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME ); // No participation in VkDeviceCreateInfo::pNext chain
		extensions.push_back( VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME );
		extensions.push_back( VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME );
		extensions.push_back( VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME ); // No participation in VkDeviceCreateInfo::pNext chain
		extensions.push_back( VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME );
		extensions.push_back( VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME );
	}

	// Enumerate extensions
	std::vector<VkExtensionProperties> foundExtensions;
	{
		uint32_t count = 0;
		VkResult vkres = CI_VK_INSTANCE_FN( EnumerateDeviceExtensionProperties( gpuHandle, nullptr, &count, nullptr ) );
		if ( vkres != VK_SUCCESS ) {
			throw grfx::GraphicsApiExc( "vkEnumerateDeviceExtensionProperties get count failed" );
		}

		foundExtensions.resize( count );

		vkres = CI_VK_INSTANCE_FN( EnumerateDeviceExtensionProperties( gpuHandle, nullptr, &count, vk::dataPtr( foundExtensions ) ) );
		if ( vkres != VK_SUCCESS ) {
			throw grfx::GraphicsApiExc( "vkEnumerateDeviceExtensionProperties get properties failed" );
		}
	}

	// Make sure all required extensions are present
	for ( const auto &name : extensions ) {
		if ( !vk::hasExtension( name, foundExtensions ) ) {
			throw VulkanExtensionNotFoundExc( name );
		}
	}
}

#define CHECK_VK_FEATURE( FOUND, FEATURE )                      \
	FOUND.FEATURE;                                              \
	if ( FOUND.FEATURE != VK_TRUE ) {                           \
		throw cinder::vk::VulkanFeatureNotFoundExc( #FEATURE ); \
	}

DeviceRef Device::create( VkPhysicalDevice gpuHandle, const Options &options )
{
	return DeviceRef( new Device( gpuHandle, options ) );
}

Device::Device( VkPhysicalDevice gpuHandle, const Options &options )
	: mGpuHandle( gpuHandle ),
	  mStagingBufferSize( options.getStagingBufferSize() )
{
	// Device properties
	{
		VkPhysicalDeviceProperties2 properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };

		CI_VK_INSTANCE_FN( GetPhysicalDeviceProperties2( this->mGpuHandle, &properties ) );
		memcpy( &mDeviceProperties, &properties.properties, sizeof( mDeviceProperties ) );
	}

	// Features
	VkPhysicalDeviceFeatures foundFeatures			= {};
	ExtensionFeatures		 foundExtensionFeatures = ExtensionFeatures( Environment::get()->getApiVersion() );
	{
		VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
		features.pNext					   = &foundExtensionFeatures.descriptorIndexing;

		CI_VK_INSTANCE_FN( GetPhysicalDeviceFeatures2( this->mGpuHandle, &features ) );
		memcpy( &foundFeatures, &features.features, sizeof( foundFeatures ) );
	}

	mQueueFamilyIndices = vk::Environment::get()->getQueueFamilyIndices( this->mGpuHandle );

	const float queuePriority = 1.0f;

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	// Graphics
	{
		VkDeviceQueueCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		createInfo.pNext				   = nullptr;
		createInfo.flags				   = 0;
		createInfo.queueFamilyIndex		   = mQueueFamilyIndices.graphics;
		createInfo.queueCount			   = 1;
		createInfo.pQueuePriorities		   = &queuePriority;
		queueCreateInfos.push_back( createInfo );
	}
	// Compute
	if ( options.getEnableComputeQueue() ) {
		VkDeviceQueueCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		createInfo.pNext				   = nullptr;
		createInfo.flags				   = 0;
		createInfo.queueFamilyIndex		   = mQueueFamilyIndices.compute;
		createInfo.queueCount			   = 1;
		createInfo.pQueuePriorities		   = &queuePriority;
		queueCreateInfos.push_back( createInfo );
	}
	// Transfer
	if ( options.getEnableTransferQueue() ) {
		VkDeviceQueueCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		createInfo.pNext				   = nullptr;
		createInfo.flags				   = 0;
		createInfo.queueFamilyIndex		   = mQueueFamilyIndices.transfer;
		createInfo.queueCount			   = 1;
		createInfo.pQueuePriorities		   = &queuePriority;
		queueCreateInfos.push_back( createInfo );
	}

	std::vector<const char *> extensions;
#if !defined( CINDER_HEADLESS )
	extensions.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
#endif
	configureExtensions( Environment::get()->getApiVersion(), mGpuHandle, options, extensions );

	// Minimum feature requirements
	mDeviceFeatures							  = {};
	mDeviceFeatures.fullDrawIndexUint32		  = CHECK_VK_FEATURE( foundFeatures, fullDrawIndexUint32 );
	mDeviceFeatures.imageCubeArray			  = CHECK_VK_FEATURE( foundFeatures, imageCubeArray );
	mDeviceFeatures.geometryShader			  = CHECK_VK_FEATURE( foundFeatures, geometryShader );
	mDeviceFeatures.tessellationShader		  = CHECK_VK_FEATURE( foundFeatures, tessellationShader );
	mDeviceFeatures.dualSrcBlend			  = CHECK_VK_FEATURE( foundFeatures, dualSrcBlend );
	mDeviceFeatures.logicOp					  = CHECK_VK_FEATURE( foundFeatures, logicOp );
	mDeviceFeatures.multiDrawIndirect		  = CHECK_VK_FEATURE( foundFeatures, multiDrawIndirect );
	mDeviceFeatures.drawIndirectFirstInstance = CHECK_VK_FEATURE( foundFeatures, drawIndirectFirstInstance );
	mDeviceFeatures.depthClamp				  = CHECK_VK_FEATURE( foundFeatures, depthClamp );
	mDeviceFeatures.depthBiasClamp			  = CHECK_VK_FEATURE( foundFeatures, depthBiasClamp );
	mDeviceFeatures.fillModeNonSolid		  = CHECK_VK_FEATURE( foundFeatures, fillModeNonSolid );
	mDeviceFeatures.depthBounds				  = CHECK_VK_FEATURE( foundFeatures, depthBounds );
	mDeviceFeatures.wideLines				  = CHECK_VK_FEATURE( foundFeatures, wideLines );
	mDeviceFeatures.largePoints				  = CHECK_VK_FEATURE( foundFeatures, largePoints );
	mDeviceFeatures.alphaToOne				  = CHECK_VK_FEATURE( foundFeatures, alphaToOne );
	mDeviceFeatures.pipelineStatisticsQuery	  = CHECK_VK_FEATURE( foundFeatures, pipelineStatisticsQuery );

	// Extensions features - use found values for now
	const ExtensionFeatures &extensionFeatures = foundExtensionFeatures;

	VkDeviceCreateInfo vkci		 = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	vkci.pNext					 = extensionFeatures.pNextStart;
	vkci.flags					 = 0;
	vkci.queueCreateInfoCount	 = vk::countU32( queueCreateInfos );
	vkci.pQueueCreateInfos		 = vk::dataPtr( queueCreateInfos );
	vkci.enabledLayerCount		 = 0;
	vkci.ppEnabledLayerNames	 = nullptr;
	vkci.enabledExtensionCount	 = vk::countU32( extensions );
	vkci.ppEnabledExtensionNames = vk::dataPtr( extensions );
	vkci.pEnabledFeatures		 = &mDeviceFeatures;

	VkResult vkres = CI_VK_INSTANCE_FN( CreateDevice( this->mGpuHandle, &vkci, nullptr, &mDeviceHandle ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkCreateDevice", vkres );
	}

	LoadDeviceFunctions( vk::Environment::get()->vkfn()->GetDeviceProcAddr, mDeviceHandle, &mVkFn );
	mVkFn.patchPromotedFunctions();

	CI_VK_DEVICE_FN( GetDeviceQueue( mDeviceHandle, mQueueFamilyIndices.graphics, 0, &mGraphicsQueueHandle ) );
	mComputeQueueHandle	 = mGraphicsQueueHandle;
	mTransferQueueHandle = mGraphicsQueueHandle;
	if ( options.getEnableComputeQueue() ) {
		CI_VK_DEVICE_FN( GetDeviceQueue( mDeviceHandle, mQueueFamilyIndices.compute, 0, &mComputeQueueHandle ) );
	}
	if ( options.getEnableTransferQueue() ) {
		CI_VK_DEVICE_FN( GetDeviceQueue( mDeviceHandle, mQueueFamilyIndices.transfer, 0, &mTransferQueueHandle ) );
	}

	// VMA
	{
		VmaVulkanFunctions vulkanFunctions					= {};
		vulkanFunctions.vkGetInstanceProcAddr				= vk::Environment::get()->vkfn()->GetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr					= vk::Environment::get()->vkfn()->GetDeviceProcAddr;
		vulkanFunctions.vkGetPhysicalDeviceProperties		= vk::Environment::get()->vkfn()->GetPhysicalDeviceProperties;
		vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vk::Environment::get()->vkfn()->GetPhysicalDeviceMemoryProperties;
		vulkanFunctions.vkAllocateMemory					= mVkFn.AllocateMemory;
		vulkanFunctions.vkFreeMemory						= mVkFn.FreeMemory;
		vulkanFunctions.vkMapMemory							= mVkFn.MapMemory;
		vulkanFunctions.vkUnmapMemory						= mVkFn.UnmapMemory;
		vulkanFunctions.vkFlushMappedMemoryRanges			= mVkFn.FlushMappedMemoryRanges;
		vulkanFunctions.vkFlushMappedMemoryRanges			= mVkFn.FlushMappedMemoryRanges;
		vulkanFunctions.vkInvalidateMappedMemoryRanges		= mVkFn.InvalidateMappedMemoryRanges;
		vulkanFunctions.vkBindBufferMemory					= mVkFn.BindBufferMemory;
		vulkanFunctions.vkBindImageMemory					= mVkFn.BindImageMemory;
		vulkanFunctions.vkGetBufferMemoryRequirements		= mVkFn.GetBufferMemoryRequirements;
		vulkanFunctions.vkGetImageMemoryRequirements		= mVkFn.GetImageMemoryRequirements;
		vulkanFunctions.vkCreateBuffer						= mVkFn.CreateBuffer;
		vulkanFunctions.vkDestroyBuffer						= mVkFn.DestroyBuffer;
		vulkanFunctions.vkCreateImage						= mVkFn.CreateImage;
		vulkanFunctions.vkDestroyImage						= mVkFn.DestroyImage;
		vulkanFunctions.vkCmdCopyBuffer						= mVkFn.CmdCopyBuffer;
#if VMA_DEDICATED_ALLOCATION || VMA_VULKAN_VERSION >= 1001000
		vulkanFunctions.vkGetBufferMemoryRequirements2KHR = mVkFn.GetBufferMemoryRequirements2KHR;
		vulkanFunctions.vkGetImageMemoryRequirements2KHR  = mVkFn.GetImageMemoryRequirements2KHR;
#endif
#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
		vulkanFunctions.vkBindBufferMemory2KHR = mVkFn.BindBufferMemory2KHR;
		vulkanFunctions.vkBindImageMemory2KHR  = mVkFn.BindImageMemory2KHR;
#endif
#if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
		vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vk::Environment::get()->vkfn()->GetPhysicalDeviceMemoryProperties2KHR;
#endif

		VmaAllocatorCreateInfo vmaCreateInfo = {};
		vmaCreateInfo.physicalDevice		 = this->mGpuHandle;
		vmaCreateInfo.device				 = this->mDeviceHandle;
		vmaCreateInfo.instance				 = vk::Environment::get()->getInstanceHandle();
		vmaCreateInfo.pVulkanFunctions		 = &vulkanFunctions;

		vkres = vmaCreateAllocator( &vmaCreateInfo, &mVmaAllocatorHandle );
		if ( vkres != VK_SUCCESS ) {
			throw grfx::GraphicsApiExc( "vmaCreateAllocator faield" );
		}
	}

	// Create a command pool and command buffer for transient operations
	{
		VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		poolCreateInfo.pNext				   = nullptr;
		poolCreateInfo.flags				   = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolCreateInfo.queueFamilyIndex		   = mQueueFamilyIndices.graphics;

		vkres = CI_VK_DEVICE_FN( CreateCommandPool( mDeviceHandle, &poolCreateInfo, nullptr, &mTransientCommandPool ) );
		if ( vkres != VK_SUCCESS ) {
			throw VulkanFnFailedExc( "vkCreateCommandPool", vkres );
		}

		VkCommandBufferAllocateInfo vkai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		vkai.pNext						 = nullptr;
		vkai.commandPool				 = mTransientCommandPool;
		vkai.level						 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		vkai.commandBufferCount			 = NUM_TRANSIENT_OPERATIONS;

		VkCommandBuffer commandBuffers[NUM_TRANSIENT_OPERATIONS] = {};

		vkres = CI_VK_DEVICE_FN( AllocateCommandBuffers( mDeviceHandle, &vkai, commandBuffers ) );
		if ( vkres != VK_SUCCESS ) {
			throw VulkanFnFailedExc( "vkAllocateCommandBuffers", vkres );
		}

		mTransitionCommanBuffer = commandBuffers[0];
		mCopyCommandBuffer		= commandBuffers[1];
	}

	// Sampler cache
	mSamplerCache = std::make_unique<SamplerCache>( this );
	if ( !mSamplerCache ) {
		throw VulkanExc( "create sampler cache failed" );
	}

	cinder::app::console() << "Vulkan device created using " << mDeviceProperties.deviceName << std::endl;
	cinder::app::console() << "Device extensions loaded:" << std::endl;
	for ( auto &ext : extensions ) {
		cinder::app::console() << "   " << ext << std::endl;
	}
}

template <typename T>
void destroyAllHandles(
	VkDevice		deviceHandle,
	std::vector<T> &handles,
	void ( *destroyFn )( VkDevice, T, const VkAllocationCallbacks * ) )
{
	for ( size_t i = 0; i < handles.size(); ++i ) {
		destroyFn( deviceHandle, handles[i], nullptr );
	}
	handles.clear();
}

Device::~Device()
{
	if ( mDeviceHandle != VK_NULL_HANDLE ) {
		CI_VK_DEVICE_FN( DeviceWaitIdle( mDeviceHandle ) );
	}

	if ( mTransitionCommanBuffer != VK_NULL_HANDLE ) {
		VkCommandBuffer commandBuffers[NUM_TRANSIENT_OPERATIONS] = { mTransitionCommanBuffer, mCopyCommandBuffer };

		CI_VK_DEVICE_FN( FreeCommandBuffers( mDeviceHandle, mTransientCommandPool, NUM_TRANSIENT_OPERATIONS, commandBuffers ) );
		mTransitionCommanBuffer = VK_NULL_HANDLE;
		mCopyCommandBuffer		= VK_NULL_HANDLE;
	}

	if ( mTransientCommandPool != VK_NULL_HANDLE ) {
		CI_VK_DEVICE_FN( DestroyCommandPool( mDeviceHandle, mTransientCommandPool, nullptr ) );
		mTransientCommandPool = VK_NULL_HANDLE;
	}

	destroyAllHandles( mDeviceHandle, mFenceHandles, mVkFn.DestroyFence );
	destroyAllHandles( mDeviceHandle, mSemaphoreHandles, mVkFn.DestroySemaphore );

	if ( mVmaAllocatorHandle != VK_NULL_HANDLE ) {
		vmaDestroyAllocator( mVmaAllocatorHandle );
		mVmaAllocatorHandle = VK_NULL_HANDLE;
	}

	if ( mDeviceHandle != VK_NULL_HANDLE ) {
		CI_VK_INSTANCE_FN( DestroyDevice( mDeviceHandle, nullptr ) );
		mDeviceHandle = VK_NULL_HANDLE;
	}
}

VkInstance Device::getInstanceHandle() const
{
	return vk::Environment::get()->getInstanceHandle();
}

VkPhysicalDevice Device::getGpuHandle() const
{
	return mGpuHandle;
}

const VkPhysicalDeviceProperties &Device::getDeviceProperties() const
{
	return mDeviceProperties;
}

VkDevice Device::getDeviceHandle() const
{
	return mDeviceHandle;
}

const VkPhysicalDeviceFeatures &Device::getDeviceFeatures() const
{
	return mDeviceFeatures;
}

std::string Device::getDeviceName() const
{
	return mDeviceProperties.deviceName;
}

const vk::QueueFamilyIndices &Device::getQueueFamilyIndices() const
{
	return mQueueFamilyIndices;
}

VkQueue Device::getGraphicsQueueHandle() const
{
	return mGraphicsQueueHandle;
}

VkQueue Device::getComputeQueueHandle() const
{
	return mComputeQueueHandle;
}

VkQueue Device::getTransferQueueHandle() const
{
	return mTransferQueueHandle;
}

VmaAllocator Device::getAllocatorHandle() const
{
	return mVmaAllocatorHandle;
}

VkSampleCountFlagBits Device::getMaxRenderTargetSampleCount() const
{
	uint32_t			  counts  = getDeviceProperties().limits.framebufferColorSampleCounts;
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
	if ( counts & VK_SAMPLE_COUNT_2_BIT ) {
		samples = VK_SAMPLE_COUNT_2_BIT;
	}
	if ( counts & VK_SAMPLE_COUNT_4_BIT ) {
		samples = VK_SAMPLE_COUNT_4_BIT;
	}
	if ( counts & VK_SAMPLE_COUNT_8_BIT ) {
		samples = VK_SAMPLE_COUNT_8_BIT;
	}
	if ( counts & VK_SAMPLE_COUNT_16_BIT ) {
		samples = VK_SAMPLE_COUNT_16_BIT;
	}
	if ( counts & VK_SAMPLE_COUNT_32_BIT ) {
		samples = VK_SAMPLE_COUNT_32_BIT;
	}
	if ( counts & VK_SAMPLE_COUNT_64_BIT ) {
		samples = VK_SAMPLE_COUNT_64_BIT;
	}
	return samples;
}

VkSampleCountFlagBits Device::getMaxDepthTargetSampleCount() const
{
	uint32_t			  counts  = getDeviceProperties().limits.framebufferDepthSampleCounts;
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
	if ( counts & VK_SAMPLE_COUNT_2_BIT ) {
		samples = VK_SAMPLE_COUNT_2_BIT;
	}
	if ( counts & VK_SAMPLE_COUNT_4_BIT ) {
		samples = VK_SAMPLE_COUNT_4_BIT;
	}
	if ( counts & VK_SAMPLE_COUNT_8_BIT ) {
		samples = VK_SAMPLE_COUNT_8_BIT;
	}
	if ( counts & VK_SAMPLE_COUNT_16_BIT ) {
		samples = VK_SAMPLE_COUNT_16_BIT;
	}
	if ( counts & VK_SAMPLE_COUNT_32_BIT ) {
		samples = VK_SAMPLE_COUNT_32_BIT;
	}
	if ( counts & VK_SAMPLE_COUNT_64_BIT ) {
		samples = VK_SAMPLE_COUNT_64_BIT;
	}
	return samples;
}

VkSampleCountFlagBits Device::getMaxStencilTargetSampleCount() const
{
	uint32_t			  counts  = getDeviceProperties().limits.framebufferStencilSampleCounts;
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
	if ( counts & VK_SAMPLE_COUNT_2_BIT ) {
		samples = VK_SAMPLE_COUNT_2_BIT;
	}
	if ( counts & VK_SAMPLE_COUNT_4_BIT ) {
		samples = VK_SAMPLE_COUNT_4_BIT;
	}
	if ( counts & VK_SAMPLE_COUNT_8_BIT ) {
		samples = VK_SAMPLE_COUNT_8_BIT;
	}
	if ( counts & VK_SAMPLE_COUNT_16_BIT ) {
		samples = VK_SAMPLE_COUNT_16_BIT;
	}
	if ( counts & VK_SAMPLE_COUNT_32_BIT ) {
		samples = VK_SAMPLE_COUNT_32_BIT;
	}
	if ( counts & VK_SAMPLE_COUNT_64_BIT ) {
		samples = VK_SAMPLE_COUNT_64_BIT;
	}
	return samples;
}

VkSampleCountFlagBits Device::getMaxOutputSampleCount() const
{
	VkSampleCountFlagBits rt = getMaxRenderTargetSampleCount();
	VkSampleCountFlagBits dt = getMaxDepthTargetSampleCount();
	VkSampleCountFlagBits st = getMaxStencilTargetSampleCount();
	return std::min( rt, std::min( dt, st ) );
}

VkResult Device::submitGraphics( const VkSubmitInfo *pSubmitInfo, VkFence fence, bool waitForIdle )
{
	std::lock_guard<std::mutex> lock( mGraphicsQueueMutex );

	VkResult vkres = CI_VK_DEVICE_FN( QueueSubmit( mGraphicsQueueHandle, 1, pSubmitInfo, fence ) );
	if ( vkres != VK_SUCCESS ) {
		return vkres;
	}

	if ( waitForIdle ) {
		vkres = CI_VK_DEVICE_FN( QueueWaitIdle( mGraphicsQueueHandle ) );
		if ( vkres != VK_SUCCESS ) {
			return vkres;
		}
	}

	return VK_SUCCESS;
}

VkResult Device::submitGraphics( const vk::SubmitInfo &submitInfo, VkFence fence, bool waitForIdle )
{
	VkTimelineSemaphoreSubmitInfo vktssi = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
	vktssi.pNext						 = nullptr;
	vktssi.waitSemaphoreValueCount		 = countU32( submitInfo.mWaitValues );
	vktssi.pWaitSemaphoreValues			 = dataPtr( submitInfo.mWaitValues );
	vktssi.signalSemaphoreValueCount	 = countU32( submitInfo.mSignalValues );
	vktssi.pSignalSemaphoreValues		 = dataPtr( submitInfo.mSignalValues );

	VkSubmitInfo vksi		  = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	vksi.pNext				  = &vktssi;
	vksi.waitSemaphoreCount	  = countU32( submitInfo.mWaitSemaphores );
	vksi.pWaitSemaphores	  = dataPtr( submitInfo.mWaitSemaphores );
	vksi.pWaitDstStageMask	  = dataPtr( submitInfo.mWaitDstStageMasks );
	vksi.commandBufferCount	  = countU32( submitInfo.mCommandBuffers );
	vksi.pCommandBuffers	  = dataPtr( submitInfo.mCommandBuffers );
	vksi.signalSemaphoreCount = countU32( submitInfo.mSignalSemaphores );
	vksi.pSignalSemaphores	  = dataPtr( submitInfo.mSignalSemaphores );

	VkResult vkres = this->submitGraphics( &vksi, fence, waitForIdle );
	return vkres;
}

VkResult Device::submitCompute( const VkSubmitInfo *pSubmitInfo, VkFence fence, bool waitForIdle )
{
	if ( mComputeQueueHandle == mGraphicsQueueHandle ) {
		return submitGraphics( pSubmitInfo, fence, waitForIdle );
	}

	std::lock_guard<std::mutex> lock( mComputeQueueMutex );

	VkResult vkres = CI_VK_DEVICE_FN( QueueSubmit( mComputeQueueHandle, 1, pSubmitInfo, fence ) );
	if ( vkres != VK_SUCCESS ) {
		return vkres;
	}

	if ( waitForIdle ) {
		vkres = CI_VK_DEVICE_FN( QueueWaitIdle( mComputeQueueHandle ) );
		if ( vkres != VK_SUCCESS ) {
			return vkres;
		}
	}

	return VK_SUCCESS;
}

VkResult Device::submitTransfer( const VkSubmitInfo *pSubmitInfo, VkFence fence, bool waitForIdle )
{
	if ( mTransferQueueHandle == mGraphicsQueueHandle ) {
		return submitGraphics( pSubmitInfo, fence, waitForIdle );
	}
	else if ( mTransferQueueHandle == mComputeQueueHandle ) {
		return submitCompute( pSubmitInfo, fence, waitForIdle );
	}

	std::lock_guard<std::mutex> lock( mTransferQueueMutex );

	VkResult vkres = CI_VK_DEVICE_FN( QueueSubmit( mTransferQueueHandle, 1, pSubmitInfo, fence ) );
	if ( vkres != VK_SUCCESS ) {
		return vkres;
	}

	if ( waitForIdle ) {
		vkres = CI_VK_DEVICE_FN( QueueWaitIdle( mTransferQueueHandle ) );
		if ( vkres != VK_SUCCESS ) {
			return vkres;
		}
	}

	return VK_SUCCESS;
}

VkResult Device::waitIdle()
{
	VkResult vkres = CI_VK_DEVICE_FN( DeviceWaitIdle( getDeviceHandle() ) );
	return vkres;
}

VkResult Device::waitIdleGraphics()
{
	std::lock_guard lock( mGraphicsQueueMutex );

	VkResult vkres = CI_VK_DEVICE_FN( QueueWaitIdle( mGraphicsQueueHandle ) );
	return vkres;
}

VkResult Device::waitIdleCompute()
{
	if ( mComputeQueueHandle == mGraphicsQueueHandle ) {
		return waitIdleGraphics();
	}

	std::lock_guard lock( mComputeQueueMutex );

	VkResult vkres = CI_VK_DEVICE_FN( QueueWaitIdle( mComputeQueueHandle ) );
	return vkres;
}

VkResult Device::waitIdleTransfer()
{
	if ( mTransferQueueHandle == mGraphicsQueueHandle ) {
		return waitIdleGraphics();
	}
	else if ( mTransferQueueHandle == mComputeQueueHandle ) {
		return waitIdleCompute();
	}

	std::lock_guard lock( mTransferQueueMutex );

	VkResult vkres = CI_VK_DEVICE_FN( QueueWaitIdle( mTransferQueueHandle ) );
	return vkres;
}

void Device::transitionImageLayout(
	VkImage				 image,
	VkImageAspectFlags	 aspectMask,
	uint32_t			 baseMipLevel,
	uint32_t			 levelCount,
	uint32_t			 baseArrayLayer,
	uint32_t			 layerCount,
	VkImageLayout		 oldLayout,
	VkImageLayout		 newLayout,
	VkPipelineStageFlags newPipelineStage )
{
	std::lock_guard<std::mutex> lock( mTransitionMutex );

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags					   = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo		   = nullptr;

	VkResult vkres = CI_VK_DEVICE_FN( BeginCommandBuffer( mTransitionCommanBuffer, &beginInfo ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkBeginCommandBuffer", vkres );
	}

	vk::cmdTransitionImageLayout(
		vkfn()->CmdPipelineBarrier,
		mTransitionCommanBuffer,
		image,
		aspectMask,
		baseMipLevel,
		levelCount,
		baseArrayLayer,
		layerCount,
		oldLayout,
		newLayout,
		newPipelineStage );

	vkres = CI_VK_DEVICE_FN( EndCommandBuffer( mTransitionCommanBuffer ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkEndCommandBuffer", vkres );
	}

	VkSubmitInfo submitInfo			= { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.pNext				= nullptr;
	submitInfo.waitSemaphoreCount	= 0;
	submitInfo.pWaitSemaphores		= nullptr;
	submitInfo.pWaitDstStageMask	= nullptr;
	submitInfo.commandBufferCount	= 1;
	submitInfo.pCommandBuffers		= &mTransitionCommanBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores	= nullptr;

	vkres = submitGraphics( &submitInfo, VK_NULL_HANDLE, true );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkQueueSubmit", vkres );
	}
}

void Device::initializeStagingBuffer()
{
	if ( !mStagingBuffer ) {
		mStagingBuffer = vk::Buffer::create(
			mStagingBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			MemoryUsage::CPU_TO_GPU,
			shared_from_this() );
	}
}

void Device::internalCopyToBuffer(
	uint64_t	size,
	vk::Buffer *pSrcBuffer,
	vk::Buffer *pDstBuffer )
{
	size = std::min<uint64_t>( size, std::min<uint64_t>( pSrcBuffer->getSize(), pDstBuffer->getSize() ) );

	// Begin command buffer
	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags					   = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo		   = nullptr;

	VkResult vkres = CI_VK_DEVICE_FN( BeginCommandBuffer( mCopyCommandBuffer, &beginInfo ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkBeginCommandBuffer", vkres );
	}

	// Copy command
	VkBufferCopy region = {};
	region.srcOffset	= 0;
	region.dstOffset	= 0;
	region.size			= static_cast<VkDeviceSize>( size );

	vkfn()->CmdCopyBuffer(
		mCopyCommandBuffer,
		pSrcBuffer->getBufferHandle(),
		pDstBuffer->getBufferHandle(),
		1,
		&region );

	// End command buffer
	vkres = CI_VK_DEVICE_FN( EndCommandBuffer( mCopyCommandBuffer ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkEndCommandBuffer", vkres );
	}

	VkSubmitInfo submitInfo			= { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.pNext				= nullptr;
	submitInfo.waitSemaphoreCount	= 0;
	submitInfo.pWaitSemaphores		= nullptr;
	submitInfo.pWaitDstStageMask	= nullptr;
	submitInfo.commandBufferCount	= 1;
	submitInfo.pCommandBuffers		= &mCopyCommandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores	= nullptr;

	vkres = submitGraphics( &submitInfo, VK_NULL_HANDLE, true );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkQueueSubmit", vkres );
	}
}

void Device::copyToBuffer(
	uint64_t	size,
	const void *pSrcData,
	vk::Buffer *pDstBuffer )
{
	std::lock_guard<std::mutex> lock( mCopyMutex );
	initializeStagingBuffer();

	const uint64_t copySize = std::min<uint64_t>( size, pDstBuffer->getSize() );

	// Figure out which staging buffer to use
	vk::BufferRef stagingBuffer = mStagingBuffer;
	if ( copySize > mStagingBufferSize ) {
		stagingBuffer = vk::Buffer::create( copySize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, MemoryUsage::CPU_TO_GPU, shared_from_this() );
	}

	// Map staging buffer
	void *pMappedAddress = nullptr;
	stagingBuffer->map( &pMappedAddress );

	// Copy source data to staging buffer
	memcpy( pMappedAddress, pSrcData, copySize );

	// Do the copy
	internalCopyToBuffer( copySize, stagingBuffer.get(), pDstBuffer );

	// Unmap staging buffer
	stagingBuffer->unmap();
}

void *Device::beginCopyToBuffer( uint64_t size, vk::Buffer *pDstBuffer )
{
	mCopyMutex.lock();
	initializeStagingBuffer();

	if ( size > mStagingBufferSize ) {
		mOversizedStagingBuffer = vk::Buffer::create( size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, MemoryUsage::CPU_TO_GPU, shared_from_this() );
	}

	vk::BufferRef stagingBuffer = mOversizedStagingBuffer ? mOversizedStagingBuffer : mStagingBuffer;

	void *pMappedAddress = nullptr;
	stagingBuffer->map( &pMappedAddress );

	return pMappedAddress;
}

void Device::endCopyToBuffer( uint64_t size, vk::Buffer *pDstBuffer )
{
	vk::BufferRef stagingBuffer = mOversizedStagingBuffer ? mOversizedStagingBuffer : mStagingBuffer;

	internalCopyToBuffer( size, stagingBuffer.get(), pDstBuffer );

	stagingBuffer->unmap();

	if ( mOversizedStagingBuffer ) {
		mOversizedStagingBuffer.reset();
	}

	mCopyMutex.unlock();
}

void Device::internalCopyToImage(
	uint32_t	srcWidth,
	uint32_t	srcHeight,
	uint32_t	srcRowBytes,
	vk::Buffer *pSrcBuffer,
	vk::Image * pDstImage )
{
	const uint32_t	   dstWidth	   = pDstImage->getExtent().width;
	const uint32_t	   dstHeight   = pDstImage->getExtent().height;
	const uint32_t	   dstRowBytes = pDstImage->getRowStride();
	VkImageAspectFlags aspectMask  = pDstImage->getAspectMask();

	// Begin command buffer
	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags					   = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo		   = nullptr;

	VkResult vkres = CI_VK_DEVICE_FN( BeginCommandBuffer( mCopyCommandBuffer, &beginInfo ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkBeginCommandBuffer", vkres );
	}

	// Transition image layout to VK_IMAGE_LAYOUT_TRANSFER_DST
	vk::cmdTransitionImageLayout(
		vkfn()->CmdPipelineBarrier,
		mTransitionCommanBuffer,
		pDstImage->getImageHandle(),
		aspectMask,
		0,
		1,
		0,
		1,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT );

	// Copy command
	VkBufferImageCopy region			   = {};
	region.bufferOffset					   = 0;
	region.bufferRowLength				   = srcWidth;
	region.bufferImageHeight			   = srcHeight;
	region.imageSubresource.aspectMask	   = aspectMask;
	region.imageSubresource.mipLevel	   = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount	   = 1;
	region.imageOffset					   = { 0, 0, 0 };
	region.imageExtent					   = { dstWidth, dstHeight, 1 };

	vkfn()->CmdCopyBufferToImage(
		mCopyCommandBuffer,
		pSrcBuffer->getBufferHandle(),
		pDstImage->getImageHandle(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region );

	// Transition image layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	vk::cmdTransitionImageLayout(
		vkfn()->CmdPipelineBarrier,
		mTransitionCommanBuffer,
		pDstImage->getImageHandle(),
		aspectMask,
		0,
		1,
		0,
		1,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT );

	// End command buffer
	vkres = CI_VK_DEVICE_FN( EndCommandBuffer( mCopyCommandBuffer ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkEndCommandBuffer", vkres );
	}

	VkSubmitInfo submitInfo			= { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.pNext				= nullptr;
	submitInfo.waitSemaphoreCount	= 0;
	submitInfo.pWaitSemaphores		= nullptr;
	submitInfo.pWaitDstStageMask	= nullptr;
	submitInfo.commandBufferCount	= 1;
	submitInfo.pCommandBuffers		= &mCopyCommandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores	= nullptr;

	vkres = submitGraphics( &submitInfo, VK_NULL_HANDLE, true );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkQueueSubmit", vkres );
	}
}

void Device::copyToImage(
	uint32_t	srcWidth,
	uint32_t	srcHeight,
	uint32_t	srcRowBytes,
	const void *pSrcData,
	vk::Image * pDstImage )
{
	std::lock_guard<std::mutex> lock( mCopyMutex );
	initializeStagingBuffer();

	uint32_t dstWidth	 = pDstImage->getExtent().width;
	uint32_t dstHeight	 = pDstImage->getExtent().height;
	uint32_t dstRowBytes = pDstImage->getRowStride();

	bool isWidthSame	= ( srcWidth == dstWidth );
	bool isHeightSame	= ( srcHeight == dstHeight );
	bool isRowBytesSame = ( srcRowBytes == dstRowBytes );
	if ( !( isWidthSame && isHeightSame && isRowBytesSame ) ) {
		throw VulkanExc( "dimension or row stride does not match for surface and image" );
	}

	const uint64_t srcDataSize = srcHeight * srcRowBytes;

	// Figure out which staging buffer to use
	vk::BufferRef stagingBuffer = mStagingBuffer;
	if ( srcDataSize > mStagingBufferSize ) {
		stagingBuffer = vk::Buffer::create( srcDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, MemoryUsage::CPU_TO_GPU, shared_from_this() );
	}

	// Map staging buffer
	void *pMappedAddress = nullptr;
	stagingBuffer->map( &pMappedAddress );

	// Copy source data to staging buffer
	memcpy( pMappedAddress, pSrcData, srcDataSize );

	// Do the copy
	internalCopyToImage( srcWidth, srcHeight, srcRowBytes, stagingBuffer.get(), pDstImage );

	// Unmap staging buffer
	stagingBuffer->unmap();
}

void *Device::beginCopyToImage( uint32_t srcWidth, uint32_t srcHeight, uint32_t srcRowBytes, vk::Image *pDstImage )
{
	uint32_t dstWidth	  = pDstImage->getExtent().width;
	uint32_t dstHeight	  = pDstImage->getExtent().height;
	uint32_t dstRowStride = pDstImage->getRowStride();

	bool isWidthSame	= ( srcWidth == dstWidth );
	bool isHeightSame	= ( srcHeight == dstHeight );
	bool isRowBytesSame = ( srcRowBytes == dstRowStride );
	if ( !( isWidthSame && isHeightSame && isRowBytesSame ) ) {
		throw VulkanExc( "dimension or row stride does not match for surface and image" );
	}

	mCopyMutex.lock();
	initializeStagingBuffer();

	const uint64_t srcDataSize = srcHeight * srcRowBytes;
	if ( srcDataSize > mStagingBufferSize ) {
		mOversizedStagingBuffer = vk::Buffer::create( srcDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, MemoryUsage::CPU_TO_GPU, shared_from_this() );
	}

	vk::BufferRef stagingBuffer = mOversizedStagingBuffer ? mOversizedStagingBuffer : mStagingBuffer;

	void *pMappedAddress = nullptr;
	stagingBuffer->map( &pMappedAddress );

	return pMappedAddress;
}

void Device::endCopyToImage( uint32_t srcWidth, uint32_t srcHeight, uint32_t srcRowBytes, vk::Image *pDstImage )
{
	vk::BufferRef stagingBuffer = mOversizedStagingBuffer ? mOversizedStagingBuffer : mStagingBuffer;

	internalCopyToImage( srcWidth, srcHeight, srcRowBytes, stagingBuffer.get(), pDstImage );

	stagingBuffer->unmap();

	if ( mOversizedStagingBuffer ) {
		mOversizedStagingBuffer.reset();
	}

	mCopyMutex.unlock();
}

VkResult Device::createFence( const VkFenceCreateInfo *pCreateInfo, VkFence *pFence )
{
	VkResult vkres = CI_VK_DEVICE_FN( CreateFence( getDeviceHandle(), pCreateInfo, nullptr, pFence ) );
	if ( vkres != VK_SUCCESS ) {
		return vkres;
	}
	mFenceHandles.push_back( *pFence );
	return VK_SUCCESS;
}

VkResult Device::createSemaphore( const VkSemaphoreCreateInfo *pCreateInfo, VkSemaphore *pSemaphore )
{
	VkResult vkres = CI_VK_DEVICE_FN( CreateSemaphore( getDeviceHandle(), pCreateInfo, nullptr, pSemaphore ) );
	if ( vkres != VK_SUCCESS ) {
		return vkres;
	}
	mSemaphoreHandles.push_back( *pSemaphore );
	return VK_SUCCESS;
}

template <typename T>
void eraseRemoveHandle( T handle, std::vector<T> &container )
{
	container.erase(
		std::remove( container.begin(), container.end(), handle ),
		container.end() );
}

void Device::destroyFence( VkFence fence )
{
	auto it = std::find( mFenceHandles.begin(), mFenceHandles.end(), fence );
	if ( it != mFenceHandles.end() ) {
		CI_VK_DEVICE_FN( DestroyFence( getDeviceHandle(), fence, nullptr ) );
		eraseRemoveHandle( fence, mFenceHandles );
	}
}

void Device::destroySemaphore( VkSemaphore semaphore )
{
	auto it = std::find( mSemaphoreHandles.begin(), mSemaphoreHandles.end(), semaphore );
	if ( it != mSemaphoreHandles.end() ) {
		CI_VK_DEVICE_FN( DestroySemaphore( getDeviceHandle(), semaphore, nullptr ) );
		eraseRemoveHandle( semaphore, mSemaphoreHandles );
	}
}

} // namespace cinder::vk
