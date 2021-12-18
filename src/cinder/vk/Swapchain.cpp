#include "cinder/vk/Swapchain.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Environment.h"
#include "cinder/vk/Image.h"
#include "cinder/vk/Sync.h"
#include "cinder/vk/Util.h"
#include "cinder/app/RendererVk.h"

#if defined( CINDER_MSW_DESKTOP )
#include "cinder/app/msw/AppImplMsw.h"
#endif

namespace cinder::vk {

SwapchainRef Swapchain::create( app::WindowImplMsw *windowImpl, const Options &options, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return SwapchainRef( new Swapchain( device, windowImpl, options ) );
}

#if defined( CINDER_MSW_DESKTOP )
Swapchain::Swapchain( vk::DeviceRef device, app::WindowImplMsw *windowImpl, const Options &options )
	: vk::DeviceChildObject( device ),
	  mNumBuffers( options.mNumBuffers )
{
	VkWin32SurfaceCreateInfoKHR vkci = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	vkci.pNext						 = nullptr;
	vkci.flags						 = 0;
	vkci.hinstance					 = ::GetModuleHandle( nullptr );
	vkci.hwnd						 = windowImpl->getHwnd();

	//VkResult vkres = vkCreateWin32SurfaceKHR( getInstanceHandle(), &vkci, nullptr, &mSurfaceHandle );
	VkResult vkres = vk::Environment::get()->vkfn()->CreateWin32SurfaceKHR( getInstanceHandle(), &vkci, nullptr, &mSurfaceHandle );
	if ( vkres != VK_SUCCESS ) {
		throw grfx::GraphicsApiExc( "vkCreateWin32SurfaceKHR failed" );
	}

	VkExtent2D extent = {
		static_cast<uint32_t>( windowImpl->getSize().x ),
		static_cast<uint32_t>( windowImpl->getSize().y ) };

	init( extent, options );
}
#elif defined( CINDER_LINUX )
Swapchain::Swapchain( vk::DeviceRef device, void *nativeWindow )
	: vk::DeviceChildObject( device ),
	  mNumBuffers( options.mNumBuffers )
{
}
#endif

Swapchain::~Swapchain()
{
	if ( mSwapchainHandle != VK_NULL_HANDLE ) {
		CI_VK_DEVICE_FN( DestroySwapchainKHR(
			getDeviceHandle(),
			mSwapchainHandle,
			nullptr ) );
		mSwapchainHandle = VK_NULL_HANDLE;
	}

	if ( mSurfaceHandle != VK_NULL_HANDLE ) {
		CI_VK_INSTANCE_FN( DestroySurfaceKHR(
			getInstanceHandle(),
			mSurfaceHandle,
			nullptr ) );
		mSurfaceHandle = VK_NULL_HANDLE;
	}
}

void Swapchain::init( const VkExtent2D &extent, const Options &options )
{
	VkFormat				 format				= options.mFormat;
	VkSurfaceCapabilitiesKHR surfaceCaps		= {};
	QueueFamilyIndices		 queueFamilyIndices = getDevice()->getQueueFamilyIndices();
	VkBool32				 supportsPresent	= VK_FALSE;

	// For now - always present on graphics queue
	VkResult vkres = CI_VK_INSTANCE_FN( GetPhysicalDeviceSurfaceSupportKHR(
		getGpuHandle(),
		queueFamilyIndices.graphics,
		mSurfaceHandle,
		&supportsPresent ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkGetPhysicalDeviceSurfaceSupportKHR", vkres );
	}
	if ( supportsPresent != VK_TRUE ) {
		throw VulkanFeatureNotFoundExc( "surface does not support present" );
	}

	vkres = CI_VK_INSTANCE_FN( GetPhysicalDeviceSurfaceCapabilitiesKHR(
		getGpuHandle(),
		mSurfaceHandle,
		&surfaceCaps ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkGetPhysicalDeviceSurfaceCapabilitiesKHR", vkres );
	}
	mUsageFlags = surfaceCaps.supportedUsageFlags;

	uint32_t count = 0;
	vkres		   = CI_VK_INSTANCE_FN( GetPhysicalDeviceSurfaceFormatsKHR(
		 getGpuHandle(),
		 mSurfaceHandle,
		 &count,
		 nullptr ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkGetPhysicalDeviceSurfaceFormatsKHR", vkres );
	}

	std::vector<VkSurfaceFormatKHR> surfaceFormats( count );
	vkres = CI_VK_INSTANCE_FN( GetPhysicalDeviceSurfaceFormatsKHR(
		getGpuHandle(),
		mSurfaceHandle,
		&count,
		dataPtr( surfaceFormats ) ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkGetPhysicalDeviceSurfaceFormatsKHR", vkres );
	}

	auto it = std::find_if( surfaceFormats.begin(), surfaceFormats.end(), [format]( const VkSurfaceFormatKHR &elem ) {
		return ( elem.format == format );
	} );
	if ( it == surfaceFormats.end() ) {
		throw VulkanFeatureNotFoundExc( "requested swapchain format not supported" );
	}
	mSurfaceFormat = *it;

	VkImageUsageFlags usageFlags = surfaceCaps.supportedUsageFlags;

	VkSwapchainCreateInfoKHR vkci = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	vkci.pNext					  = nullptr;
	vkci.flags					  = 0;
	vkci.surface				  = mSurfaceHandle;
	vkci.minImageCount			  = mNumBuffers;
	vkci.imageFormat			  = mSurfaceFormat.format;
	vkci.imageColorSpace		  = mSurfaceFormat.colorSpace;
	vkci.imageExtent			  = extent;
	vkci.imageArrayLayers		  = 1;
	vkci.imageUsage				  = mUsageFlags;
	vkci.imageSharingMode		  = VK_SHARING_MODE_EXCLUSIVE;
	vkci.queueFamilyIndexCount	  = 0;
	vkci.pQueueFamilyIndices	  = nullptr;
	vkci.preTransform			  = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	vkci.compositeAlpha			  = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	vkci.presentMode			  = options.mPresentMode;
	vkci.clipped				  = VK_TRUE;
	vkci.oldSwapchain			  = VK_NULL_HANDLE;

	vkres = CI_VK_DEVICE_FN( CreateSwapchainKHR(
		getDeviceHandle(),
		&vkci,
		nullptr,
		&mSwapchainHandle ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkCreateSwapchainKHR", vkres );
	}

	// Get swapchain images
	{
		count = 0;
		vkres = CI_VK_DEVICE_FN( GetSwapchainImagesKHR( getDeviceHandle(), mSwapchainHandle, &count, nullptr ) );
		if ( vkres != VK_SUCCESS ) {
			throw VulkanFnFailedExc( "vkGetSwapchainImagesKHR", vkres );
		}

		std::vector<VkImage> images( count );
		//vkres = vkGetSwapchainImagesKHR( getDeviceHandle(), mSwapchainHandle, &count, dataPtr( images ) );
		vkres = CI_VK_DEVICE_FN( GetSwapchainImagesKHR( getDeviceHandle(), mSwapchainHandle, &count, dataPtr( images ) ) );
		if ( vkres != VK_SUCCESS ) {
			throw VulkanFnFailedExc( "vkGetSwapchainImagesKHR", vkres );
		}

		for ( uint32_t i = 0; i < count; ++i ) {
			VkImage imageHandle = images[i];

			// Transition from UNDEFINED to PRESENT
			getDevice()->transitionImageLayout(
				imageHandle,
				vk::determineAspectMask( mSurfaceFormat.format ),
				0,
				VK_REMAINING_MIP_LEVELS,
				0,
				VK_REMAINING_ARRAY_LAYERS,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				vk::guessPipelineStageFromImageLayout( getDevice(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ) );

			// Create wrapped image
			ImageRef image = Image::create(
				imageHandle,
				VK_IMAGE_TYPE_2D,
				{ extent.width, extent.height, 1 },
				mSurfaceFormat.format,
				Image::Usage( mUsageFlags ),
				Image::Options(),
				getDevice() );
			mImages.push_back( image );

			// Create sync objects
			Sync sync				 = {};
			sync.imageReadySemaphore = Semaphore::create( VK_SEMAPHORE_TYPE_BINARY, getDevice() );
			sync.imageReadyFence	 = Fence::create( false, getDevice() );
			mSyncs.push_back( sync );
		}
	}
}

// Do not throw in this function since the return value
// can indicate if a swapchain is out of date or suboptimal
// and requires recreation.
//
VkResult Swapchain::acquireNextImage( uint64_t timeout, AcquireInfo *pAcquireInfo )
{
	uint32_t	 frameIndex			   = mPresentCount % mNumBuffers;
	SemaphoreRef imageReadySemaphore   = mSyncs[frameIndex].imageReadySemaphore;
	VkFence		 imageReadyFenceHandle = mSyncs[frameIndex].imageReadyFence->getFenceHandle();

	VkResult vkres = CI_VK_DEVICE_FN( AcquireNextImageKHR(
		getDeviceHandle(),
		mSwapchainHandle,
		timeout,
		imageReadySemaphore->getSemaphoreHandle(),
		imageReadyFenceHandle,
		&pAcquireInfo->imageIndex ) );
	if ( vkres != VK_SUCCESS ) {
		return vkres;
	}

	vkres = CI_VK_DEVICE_FN( WaitForFences( getDeviceHandle(), 1, &imageReadyFenceHandle, VK_TRUE, timeout ) );
	if ( vkres != VK_SUCCESS ) {
		return vkres;
	}

	vkres = CI_VK_DEVICE_FN( ResetFences( getDeviceHandle(), 1, &imageReadyFenceHandle ) );
	if ( vkres != VK_SUCCESS ) {
		return vkres;
	}

	pAcquireInfo->image		 = mImages[pAcquireInfo->imageIndex];
	pAcquireInfo->imageReady = imageReadySemaphore;

	return VK_SUCCESS;
}

// Do not throw in this function since the return value
// can indicate if a swapchain is out of date or suboptimal
// and requires recreation.
//
VkResult Swapchain::present( const PresentInfo *pPresentInfo )
{
	VkSemaphore semaphoreHandle = pPresentInfo->presentReady ? pPresentInfo->presentReady->getSemaphoreHandle() : VK_NULL_HANDLE;

	VkPresentInfoKHR presentInfo   = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.pNext			   = nullptr;
	presentInfo.waitSemaphoreCount = ( semaphoreHandle != VK_NULL_HANDLE ) ? 1 : 0;
	presentInfo.pWaitSemaphores	   = &semaphoreHandle;
	presentInfo.swapchainCount	   = 1;
	presentInfo.pSwapchains		   = &mSwapchainHandle;
	presentInfo.pImageIndices	   = &pPresentInfo->imageIndex;
	presentInfo.pResults		   = nullptr;

	VkResult vkres = CI_VK_DEVICE_FN( QueuePresentKHR(
		getDevice()->getGraphicsQueueHandle(),
		&presentInfo ) );
	if ( vkres != VK_SUCCESS ) {
		return vkres;
	}

	++mPresentCount;

	return VK_SUCCESS;
}

} // namespace cinder::vk
