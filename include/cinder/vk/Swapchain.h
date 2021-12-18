#pragma once

#include "cinder/vk/DeviceChildObject.h"

namespace cinder::vk {

using FenceRef	   = std::shared_ptr<class Fence>;
using ImageRef	   = std::shared_ptr<class Image>;
using SemaphoreRef = std::shared_ptr<class Semaphore>;
using SwapchainRef = std::shared_ptr<class Swapchain>;

class Swapchain
	: public vk::DeviceChildObject
{
public:
	// clang-format off
	class Options
	{
	public:
		Options() {}

		Options& numBuffers(uint32_t value) { mNumBuffers = value; return *this; }
		Options& format(VkFormat value) { mFormat = value; return *this; }
		Options& presentMode(VkPresentModeKHR value) { mPresentMode = value; return *this; }

	private:
		uint32_t			mNumBuffers		= 2;
		VkFormat			mFormat			= VK_FORMAT_B8G8R8A8_UNORM;
		VkPresentModeKHR	mPresentMode	= VK_PRESENT_MODE_FIFO_KHR;

		friend Swapchain;
	};
	// clang-format on

	struct AcquireInfo
	{
		uint32_t	 imageIndex = UINT32_MAX;
		ImageRef	 image;
		SemaphoreRef imageReady;
	};

	struct PresentInfo
	{
		uint32_t	 imageIndex;
		SemaphoreRef presentReady;
	};

	virtual ~Swapchain();

	static SwapchainRef create( app::WindowImplMsw *windowImpl, const Options &options, vk::DeviceRef device );

	VkSurfaceKHR   getSurfaceHandle() const { return mSurfaceHandle; }
	VkSwapchainKHR getSwapchaineHandle() const { return mSwapchainHandle; }

	const VkSurfaceFormatKHR &getSurfaceFormat() const { return mSurfaceFormat; }

	const std::vector<ImageRef> &getImages() const { return mImages; }

	VkResult acquireNextImage( uint64_t timeout, AcquireInfo *pAcquireInfo );

	VkResult present( const PresentInfo *pPresentInfo );

private:
#if defined( CINDER_MSW_DESKTOP )
	Swapchain( vk::DeviceRef device, app::WindowImplMsw *windowImpl, const Options &options );
#elif defined( CINDER_LINUX )
	Swapchain( vk::DeviceRef device, void *nativeWindow );
#endif

	void init( const VkExtent2D &extent, const Options &options );

private:
	struct Sync
	{
		SemaphoreRef imageReadySemaphore;
		FenceRef	 imageReadyFence;
	};

	VkSurfaceKHR		  mSurfaceHandle   = VK_NULL_HANDLE;
	VkSwapchainKHR		  mSwapchainHandle = VK_NULL_HANDLE;
	VkSurfaceFormatKHR	  mSurfaceFormat   = {};
	uint32_t			  mNumBuffers	   = 0;
	VkImageUsageFlags	  mUsageFlags	   = 0;
	std::vector<ImageRef> mImages		   = {};
	std::vector<Sync>	  mSyncs		   = {};
	uint64_t			  mPresentCount	   = 0;
};

} // namespace cinder::vk
