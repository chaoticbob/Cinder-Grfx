#pragma once

#include "cinder/vk/ChildObject.h"
#include "cinder/DataSource.h"
#include "cinder/ImageIo.h"
#include "cinder/Surface.h"

namespace cinder::vk {

class Texture2d;
using Texture	   = Texture2d;
using TextureRef   = std::shared_ptr<Texture>;
using Texture1dRef = std::shared_ptr<class Texture1d>;
using Texture2dRef = std::shared_ptr<class Texture2d>;
using Texture3dRef = std::shared_ptr<class Texture3d>;

class TextureBase
	: public vk::DeviceChildObject
{
public:
	struct Format
	{
		Format() {}

		void samples( VkSampleCountFlagBits value ) { mSamples = value; }
		void mipmap( bool enable ) { mMipLevels = enable ? CINDER_REMAINING_MIP_LEVELS : 1; }
		void mipLevels( uint32_t value ) { mMipLevels = value; }
		void arrayLayers( uint32_t value ) { mArrayLayers = value; }
		void imageUsage( VkImageUsageFlags value ) { mImageUsageFlags = value; }
		void memoryUsage( MemoryUsage value ) { mMemoryUsage = value; }

		void setSampler( vk::SamplerRef sampler ) { mSampler = sampler; }
		void minFilter( VkFilter value ) { mMinFilter = value; }
		void magFilter( VkFilter value ) { mMagFilter = value; }
		void mipmapMode( VkSamplerMipmapMode value ) { mMipMapMode = value; }
		void addressModeU( VkSamplerAddressMode value ) { mAddressModeU = value; }
		void addressModeV( VkSamplerAddressMode value ) { mAddressModeV = value; }
		void addressModeW( VkSamplerAddressMode value ) { mAddressModeW = value; }
		void maxAnisotropy( uint32_t value ) { mMaxAnisotropy = value; }
		void compareOp( VkCompareOp value ) { mCompareOp = value; }
		void borderColor( VkBorderColor value ) { mBorderColor = value; }
		void unnormalizedCoordinates( bool value ) { mUnnormalizedCoordinates = value; }

		VkSampleCountFlagBits getSamples() const { return mSamples; }
		uint32_t			  getMipLevels() const { return mMipLevels; }
		uint32_t			  getArrayLayers() const { return mArrayLayers; }
		VkImageTiling		  getTiling() const { return mTiling; }
		VkImageUsageFlags	  getImageUsage() const { return mImageUsageFlags; }
		MemoryUsage			  getMemoryUsage() const { return mMemoryUsage; }

		vk::SamplerRef		 getSampler() const { return mSampler; }
		VkFilter			 getMinFilter() const { return mMinFilter; }
		VkFilter			 getMagFilter() const { return mMagFilter; }
		VkSamplerMipmapMode	 getMipMapMode() const { return mMipMapMode; }
		VkSamplerAddressMode getAddressModeU() const { return mAddressModeU; }
		VkSamplerAddressMode getAddressModeV() const { return mAddressModeV; }
		VkSamplerAddressMode getAddressModeW() const { return mAddressModeW; }
		uint32_t			 getMaxAnisotropy() const { return mMaxAnisotropy; }
		VkCompareOp			 getCompareOp() const { return mCompareOp; }
		VkBorderColor		 getBorderColor() const { return mBorderColor; }
		bool				 getUnnormalizedCoordinates() const { return mUnnormalizedCoordinates; }

	protected:
		// Image properties
		VkSampleCountFlagBits mSamples		   = VK_SAMPLE_COUNT_1_BIT;
		uint32_t			  mMipLevels	   = 1;
		uint32_t			  mArrayLayers	   = 1;
		VkImageTiling		  mTiling		   = VK_IMAGE_TILING_OPTIMAL;
		VkImageUsageFlags	  mImageUsageFlags = 0;
		MemoryUsage			  mMemoryUsage	   = MemoryUsage::GPU_ONLY;

		// Sampler properties
		vk::SamplerRef		 mSampler;
		VkFilter			 mMinFilter				  = VK_FILTER_LINEAR;
		VkFilter			 mMagFilter				  = VK_FILTER_LINEAR;
		VkSamplerMipmapMode	 mMipMapMode			  = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		VkSamplerAddressMode mAddressModeU			  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		VkSamplerAddressMode mAddressModeV			  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		VkSamplerAddressMode mAddressModeW			  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		uint32_t			 mMaxAnisotropy			  = 0;
		VkCompareOp			 mCompareOp				  = VK_COMPARE_OP_NEVER;
		VkBorderColor		 mBorderColor			  = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		bool				 mUnnormalizedCoordinates = VK_FALSE;

		friend class Texturebase;
	};

	virtual ~TextureBase();

	uint32_t getWidth() const { return mExtent.width; }
	uint32_t getHeight() const { return mExtent.height; }
	uint32_t getDepth() const { return mExtent.depth; }

	VkFormat				  getImageFormat() const { return mImageFormat; }
	VkImageType				  getImageType() const { return mImageType; }
	const VkComponentMapping &getComponentMapping() const { return mComponentMapping; }

	const vk::Image		*getImage() const { return mImage.get(); }
	const vk::Sampler	  *getSampler() const { return mSampler.get(); }
	const vk::ImageView *getSampledImageView() const { return mSampledImage.get(); }
	const vk::ImageView *getStorageIamgeView() const { return mStorageImage.get(); }
	const vk::ImageView *getOutputTargetView() const { return mOutputTarget.get(); }

	virtual void bind( uint32_t binding = 0 ) = 0;
	void		 unbind( uint32_t binding );

protected:
	TextureBase( vk::DeviceRef device, uint32_t width );
	TextureBase( vk::DeviceRef device, uint32_t width, uint32_t height );
	TextureBase( vk::DeviceRef device, uint32_t width, uint32_t height, uint32_t depth );

	void		 initImage( VkImageCreateFlags createFlags, VkFormat imageFormat, const Format &format );
	void		 initSampler( const Format &format );
	virtual void initViews() = 0;

protected:
	VkExtent3D		   mExtent		= {};
	VkFormat		   mImageFormat = VK_FORMAT_UNDEFINED;
	VkImageType		   mImageType;
	VkComponentMapping mComponentMapping = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
	vk::ImageRef	   mImage;
	vk::SamplerRef	   mSampler;
	bool			   mDisposeSampler = false;

	vk::ImageViewRef mSampledImage;
	vk::ImageViewRef mStorageImage;
	vk::ImageViewRef mOutputTarget; // Color or depth/stencil attachments
};

//! @class Texture2d
//!
//!
class Texture2d
	: public vk::TextureBase
{
public:
	virtual ~Texture2d();

	struct Format : public TextureBase::Format
	{
		// clang-format off
		Format& mipmap( uint32_t numLevels = CINDER_REMAINING_MIP_LEVELS ) { TextureBase::Format::mipmap(numLevels); return *this; }
		Format& arrayLayers( uint32_t numLayers = CINDER_REMAINING_MIP_LEVELS ) { TextureBase::Format::arrayLayers(numLayers); return *this; }
		// clang-format on

	private:
		std::function<void( Texture2d * )> mDeleter;

		friend Texture2d;
	};

	//! Constructs a texture of size(\a width, \a height) and allocates storage.
	static Texture2dRef create( int width, int height, const Format &format = Format(), vk::DeviceRef device = nullptr );
	//! Constructs a texture of size(\a width, \a height). Pixel data is provided by \a data in format \a dataFormat (Ex: \c GL_RGB, \c GL_RGBA). Use \a format.setDataType() to specify a dataType other than \c GL_UNSIGNED_CHAR. Ignores \a format.loadTopDown().
	static Texture2dRef create( const void *data, VkFormat dataFormat, int width, int height, const Format &format = Format(), vk::DeviceRef device = nullptr );
	//! Constructs a Texture based on the contents of \a surface.
	static Texture2dRef create( const Surface8u &surface, const Format &format = Format(), vk::DeviceRef device = nullptr );
	//! Constructs a Texture based on the contents of \a channel. Sets swizzle mask to {R,R,R,1} where supported unless otherwise specified in \a format.
	static Texture2dRef create( const Channel8u &channel, const Format &format = Format(), vk::DeviceRef device = nullptr );
	//! Constructs a Texture based on the contents of \a surface.
	static Texture2dRef create( const Surface16u &surface, const Format &format = Format(), vk::DeviceRef device = nullptr );
	//! Constructs a Texture based on the contents of \a channel. Sets swizzle mask to {R,R,R,1} where supported unless otherwise specified in \a format.
	static Texture2dRef create( const Channel16u &channel, const Format &format = Format(), vk::DeviceRef device = nullptr );
	//! Constructs a Texture based on the contents of \a surface.
	static Texture2dRef create( const Surface32f &surface, const Format &format = Format(), vk::DeviceRef device = nullptr );
	/** \brief Constructs a texture based on the contents of \a channel. A default value of -1 for \a internalFormat chooses an appropriate internal format automatically. **/
	static Texture2dRef create( const Channel32f &channel, const Format &format = Format(), vk::DeviceRef device = nullptr );
	//! Constructs a Texture based on \a imageSource. A default value of -1 for \a internalFormat chooses an appropriate internal format based on the contents of \a imageSource. Uses a Format's intermediate PBO when available, which is resized as necessary.
	static Texture2dRef create( ImageSourceRef imageSource, const Format &format = Format(), vk::DeviceRef device = nullptr );

	// @TODO: Implement these
	/*
	//! Constructs a Texture based on an instance of TextureData
	static Texture2dRef create( const TextureData &data, const Format &format );
	//! Constructs a Texture from an optionally compressed KTX file. Enables mipmapping if KTX file contains mipmaps and Format has not specified \c false for mipmapping. Uses Format's intermediate PBO if supplied; requires it to be large enough to hold all MIP levels and throws if it is not. (http://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/)
	static Texture2dRef createFromKtx( const DataSourceRef &dataSource, const Format &format = Format() );
	//! Constructs a Texture from a DDS file.
	static Texture2dRef createFromDds( const DataSourceRef &dataSource, const Format &format = Format() );
*/

	virtual void bind( uint32_t binding = 0 ) override;

protected:
	Texture2d( vk::DeviceRef device, int width, int height, Format format = Format() );
	Texture2d( vk::DeviceRef device, const void *data, VkFormat dataFormat, int width, int height, Format format = Format() );
	Texture2d( vk::DeviceRef device, const Surface8u &surface, Format format = Format() );
	Texture2d( vk::DeviceRef device, const Surface16u &surface, Format format = Format() );
	Texture2d( vk::DeviceRef device, const Surface32f &surface, Format format = Format() );
	Texture2d( vk::DeviceRef device, const Channel8u &channel, Format format = Format() );
	Texture2d( vk::DeviceRef device, const Channel16u &channel, Format format = Format() );
	Texture2d( vk::DeviceRef device, const Channel32f &channel, Format format = Format() );
	Texture2d( vk::DeviceRef device, const ImageSourceRef &imageSource, Format format = Format() );

	virtual void initViews() override;

private:
	ivec2 mActualSize;	// true texture size in pixels, as opposed to clean bounds
	Area  mCleanBounds; // relative to upper-left origin regardless of top-down
};

//! @class TextureCubeMap
//!
//!
class CI_API TextureCubeMap
	: public vk::TextureBase
{
public:
	struct Format : public vk::TextureBase::Format
	{
		Format() { arrayLayers( 6 ); }

		// clang-format off
		Format& mipmap( uint32_t numLevels = CINDER_REMAINING_MIP_LEVELS ) { TextureBase::Format::mipmap(numLevels); return *this; }
		// clang-format on

	private:
		std::function<void( TextureCubeMap * )> mDeleter;

		friend TextureCubeMap;
	};

	~TextureCubeMap();

	//! Automatically infers Horizontal Cross, Vertical Cross, Row, or Column based on image aspect ratio
	static vk::TextureCubeMapRef create( const ImageSourceRef &imageSource, const Format &format = Format(), vk::DeviceRef device = nullptr );

	virtual void bind( uint32_t binding = 0 ) override;

private:
	template <typename T>
	TextureCubeMap( vk::DeviceRef device, const SurfaceT<T> images[6], Format format );

	template <typename T>
	static TextureCubeMapRef createTextureCubeMapImpl( const ImageSourceRef &imageSource, const Format &format, vk::DeviceRef device );

	virtual void initViews() override;
};

} // namespace cinder::vk
