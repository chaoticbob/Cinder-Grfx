#pragma

#include "cinder/vk/DeviceChildObject.h"

namespace cinder::vk {

using ImageRef = std::shared_ptr<class Image>;

//! @class Image
//!
//!
class Image
	: public vk::DeviceChildObject
{
public:
	//! @struct Usage
	//!
	//!
	struct Usage
	{
		Usage( VkImageUsageFlags usage = 0 )
			: mUsage( usage ) {}

		// clang-format off
		Usage& transferSrc(bool value = true)  { mUsage |= value ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0; return *this; }
		Usage& transferDst(bool value = true)  { mUsage |= value ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0; return *this; }
		Usage& sampledImage(bool value = true) { mUsage |= value ? VK_IMAGE_USAGE_SAMPLED_BIT : 0; return *this; }
		Usage& storageImage(bool value = true) { mUsage |= value ? VK_IMAGE_USAGE_STORAGE_BIT : 0; return *this; }
		Usage& renderTarget(bool value = true) { mUsage |= value ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0; return *this; }
		Usage& depthStencil(bool value = true) { mUsage |= value ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 0; return *this; }
		// clang-format on

	private:
		VkImageUsageFlags mUsage = 0;

		friend class Image;
	};

	//! @struct Options
	//!
	//!
	struct Options
	{
		Options() {}

		// clang-format off
		Options& createFlags(VkImageCreateFlags flags ) { mCreateFlags = flags; return *this; }
		Options& samples(VkSampleCountFlagBits value) { mSamples = value; return *this; }
		Options& mipLevels(uint32_t value) { mMipLevels = value; return *this; }
		Options& arrayLayers(uint32_t value) { mArrayLayers = value; return *this; }
		Options& tiling(VkImageTiling value) { mTiling = value; return *this; }
		// clang-format on

	private:
		VkImageCreateFlags	  mCreateFlags = 0;
		VkSampleCountFlagBits mSamples	   = VK_SAMPLE_COUNT_1_BIT;
		uint32_t			  mMipLevels   = 1;
		uint32_t			  mArrayLayers = 1;
		VkImageTiling		  mTiling	   = VK_IMAGE_TILING_OPTIMAL;

		friend class Image;
	};

	virtual ~Image();

	//! Create an image with commited memory allocation
	static ImageRef create(
		VkImageType			imageType,
		const VkExtent3D	 &extent,
		VkFormat			format,
		const Image::Usage &imageUsage,
		MemoryUsage			memoryUsage,
		const Options	  &options = Options(),
		vk::DeviceRef		device	= vk::DeviceRef() );

	//! Create an image using external image handle
	static ImageRef create(
		VkImage				imageHandle,
		VkImageType			imageType,
		const VkExtent3D	 &extent,
		VkFormat			format,
		const Image::Usage &imageUsage,
		const Options	  &options = Options(),
		vk::DeviceRef		device	= vk::DeviceRef() );

	//! Create 1D image with committed memory allocation
	static ImageRef create(
		uint32_t			width,
		VkFormat			format,
		const Image::Usage &imageUsage,
		MemoryUsage			memoryUsage,
		const Options	  &options = Options(),
		vk::DeviceRef		device	= vk::DeviceRef() )
	{
		return Image::create( VK_IMAGE_TYPE_1D, { width, 1, 1 }, format, imageUsage, memoryUsage, options, device );
	}

	//! Create 2D image with committed memory allocation
	static ImageRef create(
		uint32_t			width,
		uint32_t			height,
		VkFormat			format,
		const Image::Usage &imageUsage,
		MemoryUsage			memoryUsage,
		const Options	  &options = Options(),
		vk::DeviceRef		device	= vk::DeviceRef() )
	{
		return Image::create( VK_IMAGE_TYPE_2D, { width, height, 1 }, format, imageUsage, memoryUsage, options, device );
	}

	//! Create 3D image with committed memory allocation
	static ImageRef create(
		uint32_t			width,
		uint32_t			height,
		uint32_t			depth,
		VkFormat			format,
		const Image::Usage &imageUsage,
		MemoryUsage			memoryUsage,
		const Options	  &options = Options(),
		vk::DeviceRef		device	= vk::DeviceRef() )
	{
		return Image::create( VK_IMAGE_TYPE_3D, { width, height, depth }, format, imageUsage, memoryUsage, options, device );
	}

	VkImage getImageHandle() const { return mImageHandle; }

	VkImageCreateFlags getCreateFlags() const { return mCreateFlags; }

	VkImageType getImageType() const { return mImageType; }

	VkImageViewType getViewType() const { return mViewType; }

	const VkExtent3D &getExtent() const { return mExtent; }

	const VkRect2D &getArea() const { return mArea; }

	VkFormat getFormat() const { return mFormat; }

	VkSampleCountFlagBits getSamples() const { return mSamples; }

	uint32_t getMipLevels() const { return mMipLevels; }

	uint32_t getArrayLayers() const { return mArrayLayers; }

	VkImageAspectFlags getAspectMask() const { return mAspectMask; }

	void map( void **ppMappedAddress );

	void unmap();

	uint32_t getPixelStride() const { return mPixelStride; }

	uint32_t getRowStride() const { return mRowStride; }

	uint32_t getSliceStride() const { return mSliceStride; }

	bool getCubeMap() const { return mCubeMap; }

private:
	Image(
		vk::DeviceRef		device,
		VkImage				imageHandle,
		VkImageType			imageType,
		const VkExtent3D	 &extent,
		VkFormat			format,
		const Image::Usage &imageUsage,
		MemoryUsage			memoryUsage,
		const Options	  &options );

private:
	VkImage				  mImageHandle	  = VK_NULL_HANDLE;
	bool				  mDisposeImage	  = false;
	VkImageCreateFlags	  mCreateFlags	  = 0;
	VkImageType			  mImageType	  = grfx::invalidValue<VkImageType>();
	VkImageViewType		  mViewType		  = grfx::invalidValue<VkImageViewType>();
	VkFormat			  mFormat		  = VK_FORMAT_UNDEFINED;
	VkExtent3D			  mExtent		  = {};
	VkRect2D			  mArea			  = {};
	uint32_t			  mMipLevels	  = 0;
	uint32_t			  mArrayLayers	  = 0;
	VkSampleCountFlagBits mSamples		  = VK_SAMPLE_COUNT_1_BIT;
	VkImageTiling		  mTiling		  = VK_IMAGE_TILING_OPTIMAL;
	Image::Usage		  mImageUsage	  = Image::Usage();
	MemoryUsage			  mMemoryUsage	  = MemoryUsage::GPU_ONLY;
	VmaAllocation		  mAllocation	  = VK_NULL_HANDLE;
	VmaAllocationInfo	  mAllocationinfo = {};
	void				 *mMappedAddress  = nullptr;
	VkImageAspectFlags	  mAspectMask	  = 0;
	uint32_t			  mPixelStride	  = 0;
	uint32_t			  mRowStride	  = 0;
	uint32_t			  mSliceStride	  = 0;
	bool				  mCubeMap		  = false;
};

using ImageViewRef = std::shared_ptr<class ImageView>;

//! @class ImageView
//!
//!
class ImageView
	: public vk::DeviceChildObject
{
public:
	class Options
	{
	public:
		Options() {}

		Options( const vk::Image *pImage )
		{
			viewType( pImage->getViewType() );
			format( pImage->getFormat() );
			aspectMask( pImage->getAspectMask() );
			mipLevels( 0, pImage->getMipLevels() );
			arrayLayers( 0, pImage->getArrayLayers() );
		}

		// clang-format off
		Options& viewType( VkImageViewType value ) { mViewType = value; return *this; }
		Options& format( VkFormat value ) { mFormat = value; return *this; }
		Options& components( const VkComponentMapping& value ) { mComponents = value; return *this; }
		Options& aspectMask( VkImageAspectFlags value ) { mAspectMask = value; return *this; }
		Options& mipLevels( uint32_t baseLevel, uint32_t levelCount = VK_REMAINING_MIP_LEVELS ) { mBaseMipLevel = baseLevel; mLevelCount = levelCount; return *this; }
		Options& arrayLayers( uint32_t baseLayer, uint32_t layerCount = VK_REMAINING_MIP_LEVELS ) { mBaseArrayLayer = baseLayer; mLayerCount = layerCount; return *this; }
		// clang-format on

	private:
		VkImageViewType	   mViewType	   = grfx::invalidValue<VkImageViewType>();
		VkFormat		   mFormat		   = VK_FORMAT_UNDEFINED;
		VkComponentMapping mComponents	   = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		VkImageAspectFlags mAspectMask	   = 0;
		uint32_t		   mBaseMipLevel   = 0;
		uint32_t		   mLevelCount	   = VK_REMAINING_MIP_LEVELS;
		uint32_t		   mBaseArrayLayer = 0;
		uint32_t		   mLayerCount	   = VK_REMAINING_ARRAY_LAYERS;

		friend class ImageView;
	};

	virtual ~ImageView();

	//! Creates an image view with options derived from image
	static ImageViewRef create( vk::ImageRef image, vk::DeviceRef device = vk::DeviceRef() );
	//! Creates an image view using image with options required
	static ImageViewRef create( ImageRef image, const Options &options, vk::DeviceRef device = vk::DeviceRef() );
	//! Create an image view from external image handle with options  required
	static ImageViewRef create( VkImage imageHandle, VkSampleCountFlagBits samples, const Options &options, vk::DeviceRef device = vk::DeviceRef() );
	//! Create an image view from external image view handle with options  required
	static ImageViewRef create( VkImageView imageViewHandle, VkSampleCountFlagBits samples, const Options &options, vk::DeviceRef device = vk::DeviceRef() );

	VkImageView getImageViewHandle() const { return mImageViewHandle; }

	VkFormat			  getFormat() const { return mFormat; }
	VkSampleCountFlagBits getSamples() const { return mSamples; }
	VkImageAspectFlags	  getAspectMask() const { return mAspectMask; }

	ImageRef getImage() const { return mImage; }

	const VkExtent3D &getImageExtent() const { return mImage->getExtent(); }

	const VkRect2D &getImageArea() const { return mImage->getArea(); }

private:
	ImageView( vk::DeviceRef device, ImageRef image, const Options &options );
	ImageView( vk::DeviceRef device, VkImage imageHandle, VkSampleCountFlagBits samples, const Options &options );
	ImageView( vk::DeviceRef device, VkImageView imageViewHandle, VkSampleCountFlagBits samples, const Options &options );

private:
	VkImageView			  mImageViewHandle		  = VK_NULL_HANDLE;
	bool				  mDisposeImageViewHandle = false;
	VkFormat			  mFormat				  = VK_FORMAT_UNDEFINED;
	VkSampleCountFlagBits mSamples				  = VK_SAMPLE_COUNT_1_BIT; // Needed by render pass
	VkImageAspectFlags	  mAspectMask			  = 0;
	ImageRef			  mImage;
};

} // namespace cinder::vk
