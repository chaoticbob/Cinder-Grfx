#include "cinder/vk/Image.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Util.h"
#include "cinder/app/RendererVk.h"

namespace cinder::vk {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Image

ImageRef Image::create(
	VkImageType			imageType,
	const VkExtent3D	 &extent,
	VkFormat			format,
	const Image::Usage &imageUsage,
	MemoryUsage			memoryUsage,
	const Options	  &options,
	vk::DeviceRef		device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return ImageRef( new Image( device, VK_NULL_HANDLE, imageType, extent, format, imageUsage, memoryUsage, options ) );
}

ImageRef Image::create(
	VkImage				imageHandle,
	VkImageType			imageType,
	const VkExtent3D	 &extent,
	VkFormat			format,
	const Image::Usage &imageUsage,
	const Options	  &options,
	vk::DeviceRef		device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return ImageRef( new Image( device, imageHandle, imageType, extent, format, imageUsage, MemoryUsage::UNKNOWN, options ) );
}

Image::Image(
	vk::DeviceRef		device,
	VkImage				imageHandle,
	VkImageType			imageType,
	const VkExtent3D	 &extent,
	VkFormat			format,
	const Image::Usage &imageUsage,
	MemoryUsage			memoryUsage,
	const Options	  &options )
	: vk::DeviceChildObject( device ),
	  mImageHandle( imageHandle ),
	  mDisposeImage( imageHandle == VK_NULL_HANDLE ),
	  mCreateFlags( options.mCreateFlags ),
	  mImageType( imageType ),
	  mFormat( format ),
	  mExtent( extent ),
	  mArea( ( imageType == VK_IMAGE_TYPE_2D ) ? VkRect2D{ { 0, 0 }, { extent.width, extent.height } } : VkRect2D{} ),
	  mMipLevels( options.mMipLevels ),
	  mArrayLayers( options.mArrayLayers ),
	  mSamples( options.mSamples ),
	  mTiling( options.mTiling ),
	  mImageUsage( imageUsage ),
	  mMemoryUsage( memoryUsage ),
	  mAspectMask( determineAspectMask( format ) )
{
	mPixelStride = formatSize( mFormat );
	if ( imageType != VK_IMAGE_TYPE_1D ) {
		mRowStride = mExtent.width * mPixelStride;
	}
	if ( imageType == VK_IMAGE_TYPE_3D ) {
		mSliceStride = mExtent.width * mRowStride;
	}

	if ( mCreateFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT ) {
		mViewType = VK_IMAGE_VIEW_TYPE_CUBE;
	}
	else {
		switch ( mImageType ) {
			default: break;
			case VK_IMAGE_TYPE_1D: mViewType = VK_IMAGE_VIEW_TYPE_1D; break;
			case VK_IMAGE_TYPE_2D: mViewType = VK_IMAGE_VIEW_TYPE_2D; break;
			case VK_IMAGE_TYPE_3D: mViewType = VK_IMAGE_VIEW_TYPE_3D; break;
		}
	}

	if ( mImageHandle == VK_NULL_HANDLE ) {
		VkImageCreateInfo vkci	   = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		vkci.pNext				   = nullptr;
		vkci.flags				   = mCreateFlags;
		vkci.imageType			   = mImageType;
		vkci.format				   = mFormat;
		vkci.extent				   = extent;
		vkci.mipLevels			   = mMipLevels;
		vkci.arrayLayers		   = mArrayLayers;
		vkci.samples			   = mSamples;
		vkci.tiling				   = mTiling;
		vkci.usage				   = mImageUsage.mUsage;
		vkci.sharingMode		   = VK_SHARING_MODE_EXCLUSIVE;
		vkci.queueFamilyIndexCount = 0;
		vkci.pQueueFamilyIndices   = nullptr;
		vkci.initialLayout		   = VK_IMAGE_LAYOUT_UNDEFINED;

		VkResult vkres = CI_VK_DEVICE_FN( CreateImage(
			getDeviceHandle(),
			&vkci,
			nullptr,
			&mImageHandle ) );
		if ( vkres != VK_SUCCESS ) {
			throw VulkanFnFailedExc( "vkCreateImage", vkres );
		}

		VmaMemoryUsage vmaUsage = toVmaMemoryUsage( memoryUsage );
		if ( vmaUsage == VMA_MEMORY_USAGE_UNKNOWN ) {
			throw VulkanExc( "unsupported memory type for image" );
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

		vkres = vmaAllocateMemoryForImage(
			getDevice()->getAllocatorHandle(),
			mImageHandle,
			&vmaci,
			&mAllocation,
			&mAllocationinfo );
		if ( vkres != VK_SUCCESS ) {
			throw VulkanFnFailedExc( "vmaAllocateMemoryForImage", vkres );
		}

		vkres = vmaBindImageMemory(
			getDevice()->getAllocatorHandle(),
			mAllocation,
			mImageHandle );
		if ( vkres != VK_SUCCESS ) {
			throw VulkanFnFailedExc( "vmaBindImageMemory", vkres );
		}
	}
}

Image::~Image()
{
	if ( mAllocation != VK_NULL_HANDLE ) {
		vmaFreeMemory( getDevice()->getAllocatorHandle(), mAllocation );
		mAllocation = VK_NULL_HANDLE;
	}

	if ( ( mImageHandle != VK_NULL_HANDLE ) && mDisposeImage ) {
		CI_VK_DEVICE_FN( DestroyImage( getDeviceHandle(), mImageHandle, nullptr ) );
		mImageHandle = VK_NULL_HANDLE;
	}
}

void Image::map( void **ppMappedAddress )
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

void Image::unmap()
{
	if ( mMappedAddress == nullptr ) {
		return;
	}

	vmaUnmapMemory( getDevice()->getAllocatorHandle(), mAllocation );
	mMappedAddress = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageView

ImageViewRef ImageView::create( vk::ImageRef image, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	ImageView::Options options = ImageView::Options( image.get() );

	return ImageViewRef( new ImageView( device, image, options ) );
}

ImageViewRef ImageView::create( ImageRef image, const Options &options, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return ImageViewRef( new ImageView( device, image, options ) );
}

ImageViewRef ImageView::create( VkImage imageHandle, VkSampleCountFlagBits samples, const Options &options, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return ImageViewRef( new ImageView( device, imageHandle, samples, options ) );
}

ImageViewRef ImageView::create( VkImageView imageViewHandle, VkSampleCountFlagBits samples, const Options &options, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return ImageViewRef( new ImageView( device, imageViewHandle, samples, options ) );
}

ImageView::ImageView( vk::DeviceRef device, ImageRef image, const Options &options )
	: vk::DeviceChildObject( device ),
	  mDisposeImageViewHandle( true ),
	  mImage( image ),
	  mSamples( image->getSamples() )
{
	VkImageViewType viewType = image->getViewType();
	if ( options.mViewType != grfx::invalidValue<VkImageType>() ) {
		viewType = options.mViewType;
	}

	mFormat = image->getFormat();
	if ( options.mFormat != VK_FORMAT_UNDEFINED ) {
		mFormat = options.mFormat;
	}

	mAspectMask = image->getAspectMask();
	if ( options.mAspectMask != 0 ) {
		mAspectMask = options.mAspectMask;
	}

	VkImageViewCreateInfo vkci			 = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	vkci.pNext							 = nullptr;
	vkci.flags							 = 0;
	vkci.image							 = mImage->getImageHandle();
	vkci.viewType						 = viewType;
	vkci.format							 = mFormat;
	vkci.components						 = options.mComponents;
	vkci.subresourceRange.aspectMask	 = mAspectMask;
	vkci.subresourceRange.baseMipLevel	 = options.mBaseMipLevel;
	vkci.subresourceRange.levelCount	 = options.mLevelCount;
	vkci.subresourceRange.baseArrayLayer = options.mBaseArrayLayer;
	vkci.subresourceRange.layerCount	 = options.mLayerCount;

	VkResult vkres = CI_VK_DEVICE_FN( CreateImageView(
		getDeviceHandle(),
		&vkci,
		nullptr,
		&mImageViewHandle ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkCreateImageView", vkres );
	}
}

ImageView::ImageView( vk::DeviceRef device, VkImage imageHandle, VkSampleCountFlagBits samples, const Options &options )
	: vk::DeviceChildObject( device ),
	  mDisposeImageViewHandle( true ),
	  mFormat( options.mFormat ),
	  mSamples( samples ),
	  mAspectMask( options.mAspectMask )
{
	if ( mAspectMask == 0 ) {
		mAspectMask = determineAspectMask( mFormat );
	}

	VkImageViewCreateInfo vkci			 = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	vkci.pNext							 = nullptr;
	vkci.flags							 = 0;
	vkci.image							 = imageHandle;
	vkci.viewType						 = options.mViewType;
	vkci.format							 = mFormat;
	vkci.components						 = options.mComponents;
	vkci.subresourceRange.aspectMask	 = options.mAspectMask;
	vkci.subresourceRange.baseMipLevel	 = options.mBaseMipLevel;
	vkci.subresourceRange.levelCount	 = options.mLevelCount;
	vkci.subresourceRange.baseArrayLayer = options.mBaseArrayLayer;
	vkci.subresourceRange.layerCount	 = options.mLayerCount;

	VkResult vkres = CI_VK_DEVICE_FN( CreateImageView(
		getDeviceHandle(),
		&vkci,
		nullptr,
		&mImageViewHandle ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkCreateImageView", vkres );
	}
}

ImageView::ImageView( vk::DeviceRef device, VkImageView imageViewHandle, VkSampleCountFlagBits samples, const Options &options )
	: vk::DeviceChildObject( device ),
	  mImageViewHandle( imageViewHandle ),
	  mDisposeImageViewHandle( false ),
	  mFormat( options.mFormat ),
	  mSamples( samples )
{
}

ImageView::~ImageView()
{
	if ( ( mImageViewHandle != VK_NULL_HANDLE ) && mDisposeImageViewHandle ) {
		CI_VK_DEVICE_FN( DestroyImageView( getDeviceHandle(), mImageViewHandle, nullptr ) );
		mImageViewHandle		= VK_NULL_HANDLE;
		mDisposeImageViewHandle = false;
	}
}

} // namespace cinder::vk
