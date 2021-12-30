#include "cinder/vk/Texture.h"
#include "cinder/vk//Context.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Image.h"
#include "cinder/vk/wrapper.h"
#include "cinder/app/RendererVk.h"
#include "cinder/ip/Resize.h"

namespace cinder::vk {

/////////////////////////////////////////////////////////////////////////////////
// ImageTargetBuffer

template <typename T>
class ImageTargetTexture : public ImageTarget
{
public:
	static std::shared_ptr<ImageTargetTexture<T>> create( const Texture *texture, ImageIo::ChannelOrder channelOrder, bool isGray, bool hasAlpha, void *intermediateDataStore );

	virtual bool hasAlpha() const { return mHasAlpha; }

	virtual void *getRowPointer( int32_t row );

	void *getData() const { return mDataBaseAddress; }

private:
	ImageTargetTexture( const Texture *texture, ImageIo::ChannelOrder channelOrder, bool isGray, bool hasAlpha, void *intermediateDataStore );

private:
	const vk::Texture *mTexture;
	bool			   mHasAlpha;
	uint8_t			   mPixelInc;
	T *				   mDataBaseAddress;
	int32_t			   mRowInc;
};

template <typename T>
std::shared_ptr<ImageTargetTexture<T>> ImageTargetTexture<T>::create( const Texture *texture, ImageIo::ChannelOrder channelOrder, bool isGray, bool hasAlpha, void *intermediateDataStore )
{
	return std::shared_ptr<ImageTargetTexture<T>>( new ImageTargetTexture<T>( texture, channelOrder, isGray, hasAlpha, intermediateDataStore ) );
}

template <typename T>
ImageTargetTexture<T>::ImageTargetTexture( const Texture *texture, ImageIo::ChannelOrder channelOrder, bool isGray, bool hasAlpha, void *intermediateDataStore )
	: ImageTarget(), mTexture( texture ), mHasAlpha( hasAlpha )
{
	if ( isGray ) {
		mPixelInc = mHasAlpha ? 2 : 1;
	}
	else {
		mPixelInc = mHasAlpha ? 4 : 3;
	}
	mRowInc = mTexture->getWidth() * mPixelInc;

	mDataBaseAddress = reinterpret_cast<T *>( intermediateDataStore );

	if ( std::is_same<T, uint8_t>::value ) {
		setDataType( ImageIo::UINT8 );
	}
	else if ( std::is_same<T, uint16_t>::value ) {
		setDataType( ImageIo::UINT16 );
	}
	else if ( std::is_same<T, half_float>::value ) {
		setDataType( ImageIo::FLOAT16 );
	}
	else if ( std::is_same<T, float>::value ) {
		setDataType( ImageIo::FLOAT32 );
	}

	setChannelOrder( channelOrder );
	setColorModel( isGray ? ImageIo::CM_GRAY : ImageIo::CM_RGB );
}

template <typename T>
void *ImageTargetTexture<T>::getRowPointer( int32_t row )
{
	return mDataBaseAddress + ( row * mRowInc );
}

/////////////////////////////////////////////////////////////////////////////////
// TextureBase

TextureBase::TextureBase( vk::DeviceRef device, uint32_t width )
	: vk::DeviceChildObject( device ),
	  mExtent( { width, 1, 1 } ),
	  mImageType( VK_IMAGE_TYPE_1D )
{
}

TextureBase::TextureBase( vk::DeviceRef device, uint32_t width, uint32_t height )
	: vk::DeviceChildObject( device ),
	  mExtent( { width, height, 1 } ),
	  mImageType( VK_IMAGE_TYPE_2D )
{
}

TextureBase::TextureBase( vk::DeviceRef device, uint32_t width, uint32_t height, uint32_t depth )
	: vk::DeviceChildObject( device ),
	  mExtent( { width, height, depth } ),
	  mImageType( VK_IMAGE_TYPE_3D )
{
}

TextureBase::~TextureBase()
{
}

static uint32_t countMips( uint32_t width )
{
	uint32_t mipLevels = 1;
	while ( width > 1 ) {
		if ( width > 1 ) {
			width >>= 1;
		}
		++mipLevels;
	}
	return mipLevels;
}

static uint32_t countMips( uint32_t width, uint32_t height )
{
	uint32_t mipLevels = 1;
	while ( ( width > 1 ) || ( height > 1 ) ) {
		if ( width > 1 ) {
			width >>= 1;
		}
		if ( height > 1 ) {
			height >>= 1;
		}
		++mipLevels;
	}
	return mipLevels;
}

static uint32_t countMips( uint32_t width, uint32_t height, uint32_t depth )
{
	uint32_t mipLevels = 1;
	while ( ( width > 1 ) || ( height > 1 ) || ( depth > 1 ) ) {
		if ( width > 1 ) {
			width >>= 1;
		}
		if ( height > 1 ) {
			height >>= 1;
		}
		if ( depth > 1 ) {
			depth >>= 1;
		}
		++mipLevels;
	}
	return mipLevels;
}

void TextureBase::initImage( VkFormat imageFormat, const Format &format )
{
	mImageFormat = imageFormat;

	Image::Usage imageUsage = Image::Usage().transferSrc().transferDst().sampledImage();

	uint32_t mipLevels = format.getMipLevels();
	if ( mipLevels > 1 ) {
		uint32_t mipLevelCount = 0;
		switch ( mImageType ) {
			default: break;
			case VK_IMAGE_TYPE_1D: mipLevelCount = countMips( mExtent.width ); break;
			case VK_IMAGE_TYPE_2D: mipLevelCount = countMips( mExtent.width, mExtent.height ); break;
			case VK_IMAGE_TYPE_3D: mipLevelCount = countMips( mExtent.width, mExtent.height, mExtent.depth ); break;
		}
		mipLevels = std::min<uint32_t>( mipLevels, mipLevelCount );
	}

	if ( mipLevels == 0 ) {
		throw VulkanExc( "invalid mip levle count" );
	}

	Image::Options imageOptions = Image::Options();
	imageOptions.samples( format.getSamples() );
	imageOptions.mipLevels( mipLevels );
	imageOptions.arrayLayers( format.getArrayLayers() );
	imageOptions.tiling( format.getTiling() );

	mImage = Image::create( mImageType, mExtent, mImageFormat, imageUsage, format.getMemoryUsage(), imageOptions, getDevice() );
}

void TextureBase::initSampler( const Format &format )
{
	if ( format.getSampler() ) {
		mSampler		= format.getSampler();
		mDisposeSampler = false;
	}
	else {
		vk::SamplerHashKey key		= {};
		key.magFilter				= format.getMagFilter();
		key.minFilter				= format.getMinFilter();
		key.mipmapMode				= format.getMipMapMode();
		key.addressModeU			= format.getAddressModeU();
		key.addressModeV			= format.getAddressModeV();
		key.addressModeW			= format.getAddressModeW();
		key.mipLodBias				= 0;
		key.maxAnisotropy			= format.getMaxAnisotropy();
		key.compareOp				= format.getCompareOp();
		key.minLod					= 0;
		key.maxLod					= static_cast<uint32_t>( VK_LOD_CLAMP_NONE );
		key.borderColor				= format.getBorderColor();
		key.unnormalizedCoordinates = format.getUnnormalizedCoordinates();

		mSampler		= getDevice()->getSamplerCache()->createSampler( key );
		mDisposeSampler = true;
	}
}

void TextureBase::unbind( uint32_t binding )
{
	auto ctx = Context::getCurrentContext();
	ctx->unbindTexture( binding );
}

/////////////////////////////////////////////////////////////////////////////////
// Texture2d

Texture2dRef Texture2d::create( int width, int height, const Format &format, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	if ( format.mDeleter ) {
		return TextureRef( new Texture2d( device, width, height, format ), format.mDeleter );
	}
	else {
		return TextureRef( new Texture2d( device, width, height, format ) );
	}
}

Texture2dRef Texture2d::create( const void *data, VkFormat dataFormat, int width, int height, const Format &format, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	if ( format.mDeleter ) {
		return Texture2dRef( new Texture2d( device, data, dataFormat, width, height, format ), format.mDeleter );
	}
	else {
		return Texture2dRef( new Texture2d( device, data, dataFormat, width, height, format ) );
	}
}

Texture2dRef Texture2d::create( const Surface8u &surface, const Format &format, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	if ( format.mDeleter ) {
		return Texture2dRef( new Texture2d( device, surface, format ), format.mDeleter );
	}
	else {
		return Texture2dRef( new Texture2d( device, surface, format ) );
	}
}

Texture2dRef Texture2d::create( const Channel8u &channel, const Format &format, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	if ( format.mDeleter ) {
		return Texture2dRef( new Texture2d( device, channel, format ), format.mDeleter );
	}
	else {
		return Texture2dRef( new Texture2d( device, channel, format ) );
	}
}

Texture2dRef Texture2d::create( const Surface16u &surface, const Format &format, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	if ( format.mDeleter ) {
		return TextureRef( new Texture( device, surface, format ), format.mDeleter );
	}
	else {
		return TextureRef( new Texture( device, surface, format ) );
	}
}

Texture2dRef Texture2d::create( const Channel16u &channel, const Format &format, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	if ( format.mDeleter ) {
		return Texture2dRef( new Texture2d( device, channel, format ), format.mDeleter );
	}
	else {
		return Texture2dRef( new Texture2d( device, channel, format ) );
	}
}

Texture2dRef Texture2d::create( const Surface32f &surface, const Format &format, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	if ( format.mDeleter ) {
		return Texture2dRef( new Texture2d( device, surface, format ), format.mDeleter );
	}
	else {
		return Texture2dRef( new Texture2d( device, surface, format ) );
	}
}

Texture2dRef Texture2d::create( const Channel32f &channel, const Format &format, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	if ( format.mDeleter ) {
		return Texture2dRef( new Texture2d( device, channel, format ), format.mDeleter );
	}
	else {
		return Texture2dRef( new Texture2d( device, channel, format ) );
	}
}

Texture2dRef Texture2d::create( ImageSourceRef imageSource, const Format &format, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	if ( format.mDeleter ) {
		return Texture2dRef( new Texture2d( device, imageSource, format ), format.mDeleter );
	}
	else {
		return Texture2dRef( new Texture2d( device, imageSource, format ) );
	}
}

Texture2d::Texture2d( vk::DeviceRef device, int width, int height, Format format )
	: TextureBase( device, width, height ),
	  mActualSize( width, height ),
	  mCleanBounds( 0, 0, width, height )
{
}

Texture2d::Texture2d( vk::DeviceRef device, const void *data, VkFormat dataFormat, int width, int height, Format format )
	: TextureBase( device, width, height ),
	  mActualSize( width, height ),
	  mCleanBounds( 0, 0, width, height )
{
	initImage( dataFormat, format );
	initSampler( format );
	initViews();
}

Texture2d::Texture2d( vk::DeviceRef device, const Surface8u &surface, Format format )
	: TextureBase( device, surface.getWidth(), surface.getHeight() ),
	  mActualSize( surface.getSize() ),
	  mCleanBounds( 0, 0, surface.getWidth(), surface.getHeight() )
{
	int		 channelOrder  = surface.getChannelOrder().getCode();
	VkFormat imageFormat   = VK_FORMAT_UNDEFINED;
	bool	 convertToRgba = false;
	// clang-format off
	switch ( channelOrder ) {
		default: break;
		case SurfaceChannelOrder::RGBA: imageFormat = VK_FORMAT_R8G8B8A8_UNORM; break;
		case SurfaceChannelOrder::BGRA: imageFormat = VK_FORMAT_B8G8R8A8_UNORM; break;
		case SurfaceChannelOrder::ARGB: convertToRgba = true; break;
		case SurfaceChannelOrder::ABGR: imageFormat = VK_FORMAT_A8B8G8R8_UNORM_PACK32; break;
		case SurfaceChannelOrder::RGBX: imageFormat = VK_FORMAT_R8G8B8A8_UNORM; break;
		case SurfaceChannelOrder::BGRX: imageFormat = VK_FORMAT_B8G8R8A8_UNORM; break;
		case SurfaceChannelOrder::XRGB: convertToRgba = true;
		case SurfaceChannelOrder::XBGR: imageFormat = VK_FORMAT_A8B8G8R8_UNORM_PACK32; break;
		case SurfaceChannelOrder::RGB : convertToRgba = true; break;
		case SurfaceChannelOrder::BGR : convertToRgba = true; break;
	}
	// clang-format on

	if ( convertToRgba ) {
		imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
	}

	if ( imageFormat == VK_FORMAT_UNDEFINED ) {
		throw VulkanExc( "couldn't find matching Vulkan format for surface" );
	}

	initImage( imageFormat, format );
	initSampler( format );
	initViews();

	const Surface8u *pSrcSurface = &surface;
	Surface8u		 surfaceRgba;

	if ( convertToRgba ) {
		surfaceRgba = Surface8u( surface.getWidth(), surface.getHeight(), true, SurfaceChannelOrder::RGBA );
		surfaceRgba.copyFrom( surface, surface.getBounds(), glm::ivec2( 0, 0 ) );
		pSrcSurface = &surface;
	}

	//getDevice()->copyToImage(
	//	static_cast<uint32_t>( pSrcSurface->getWidth() ),
	//	static_cast<uint32_t>( pSrcSurface->getHeight() ),
	//	static_cast<uint32_t>( pSrcSurface->getRowBytes() ),
	//	pSrcSurface->getData(),
	//	mImage.get() );
}

Texture2d::Texture2d( vk::DeviceRef device, const Surface16u &surface, Format format )
	: TextureBase( device, surface.getWidth(), surface.getHeight() ),
	  mActualSize( surface.getSize() ),
	  mCleanBounds( 0, 0, surface.getWidth(), surface.getHeight() )
{
	int		 channelOrder  = surface.getChannelOrder().getCode();
	VkFormat imageFormat   = VK_FORMAT_UNDEFINED;
	bool	 convertToRgba = false;
	// clang-format off
	switch ( channelOrder ) {
		default: break;
		case SurfaceChannelOrder::RGBA: imageFormat = VK_FORMAT_R16G16B16A16_UNORM; break;
		case SurfaceChannelOrder::BGRA: convertToRgba = true;
		case SurfaceChannelOrder::ARGB: convertToRgba = true;
		case SurfaceChannelOrder::ABGR: convertToRgba = true;
		case SurfaceChannelOrder::RGBX: imageFormat = VK_FORMAT_R16G16B16A16_UNORM; break;
		case SurfaceChannelOrder::BGRX: convertToRgba = true;
		case SurfaceChannelOrder::XRGB: convertToRgba = true;
		case SurfaceChannelOrder::XBGR: convertToRgba = true;
		case SurfaceChannelOrder::RGB : convertToRgba = true;
		case SurfaceChannelOrder::BGR : convertToRgba = true;
	}
	// clang-format on

	if ( convertToRgba ) {
		imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
	}

	if ( imageFormat == VK_FORMAT_UNDEFINED ) {
		throw VulkanExc( "couldn't find matching Vulkan format for surface" );
	}

	initImage( imageFormat, format );
	initSampler( format );
	initViews();

	const Surface16u *pSrcSurface = &surface;
	Surface16u		  surfaceRgba;

	if ( convertToRgba ) {
		surfaceRgba = Surface16u( surface.getWidth(), surface.getHeight(), true, SurfaceChannelOrder::RGBA );
		surfaceRgba.copyFrom( surface, surface.getBounds(), glm::ivec2( 0, 0 ) );
		pSrcSurface = &surface;
	}

	//getDevice()->copyToImage(
	//	static_cast<uint32_t>( pSrcSurface->getWidth() ),
	//	static_cast<uint32_t>( pSrcSurface->getHeight() ),
	//	static_cast<uint32_t>( pSrcSurface->getRowBytes() ),
	//	pSrcSurface->getData(),
	//	mImage.get() );
}

Texture2d::Texture2d( vk::DeviceRef device, const Surface32f &surface, Format format )
	: TextureBase( device, surface.getWidth(), surface.getHeight() ),
	  mActualSize( surface.getSize() ),
	  mCleanBounds( 0, 0, surface.getWidth(), surface.getHeight() )
{
	int		 channelOrder  = surface.getChannelOrder().getCode();
	VkFormat imageFormat   = VK_FORMAT_UNDEFINED;
	bool	 convertToRgba = false;
	// clang-format off
	switch ( channelOrder ) {
		default: break;
		case SurfaceChannelOrder::RGBA: imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT; break;
		case SurfaceChannelOrder::BGRA: convertToRgba = true;
		case SurfaceChannelOrder::ARGB: convertToRgba = true;
		case SurfaceChannelOrder::ABGR: convertToRgba = true;
		case SurfaceChannelOrder::RGBX: imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT; break;
		case SurfaceChannelOrder::BGRX: convertToRgba = true;
		case SurfaceChannelOrder::XRGB: convertToRgba = true;
		case SurfaceChannelOrder::XBGR: convertToRgba = true;
		case SurfaceChannelOrder::RGB : convertToRgba = true;
		case SurfaceChannelOrder::BGR : convertToRgba = true;
	}
	// clang-format on

	if ( convertToRgba ) {
		imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
	}

	if ( imageFormat == VK_FORMAT_UNDEFINED ) {
		throw VulkanExc( "couldn't find matching Vulkan format for surface" );
	}

	initImage( imageFormat, format );
	initSampler( format );
	initViews();

	const Surface32f *pSrcSurface = &surface;
	Surface32f		  surfaceRgba;

	if ( convertToRgba ) {
		surfaceRgba = Surface32f( surface.getWidth(), surface.getHeight(), true, SurfaceChannelOrder::RGBA );
		surfaceRgba.copyFrom( surface, surface.getBounds(), glm::ivec2( 0, 0 ) );
		pSrcSurface = &surface;
	}

	//getDevice()->copyToImage(
	//	static_cast<uint32_t>( pSrcSurface->getWidth() ),
	//	static_cast<uint32_t>( pSrcSurface->getHeight() ),
	//	static_cast<uint32_t>( pSrcSurface->getRowBytes() ),
	//	pSrcSurface->getData(),
	//	mImage.get() );
}

Texture2d::Texture2d( vk::DeviceRef device, const Channel8u &channel, Format format )
	: TextureBase( device, channel.getWidth(), channel.getHeight() ),
	  mActualSize( channel.getSize() ),
	  mCleanBounds( 0, 0, channel.getWidth(), channel.getHeight() )
{
	initImage( VK_FORMAT_R8_UNORM, format );
	initSampler( format );
	initViews();

	//getDevice()->copyToImage(
	//	static_cast<uint32_t>( channel.getWidth() ),
	//	static_cast<uint32_t>( channel.getHeight() ),
	//	static_cast<uint32_t>( channel.getRowBytes() ),
	//	channel.getData(),
	//	mImage.get() );
}

Texture2d::Texture2d( vk::DeviceRef device, const Channel16u &channel, Format format )
	: TextureBase( device, channel.getWidth(), channel.getHeight() ),
	  mActualSize( channel.getSize() ),
	  mCleanBounds( 0, 0, channel.getWidth(), channel.getHeight() )
{
	initImage( VK_FORMAT_R16_UNORM, format );
	initSampler( format );
	initViews();

	//getDevice()->copyToImage(
	//	static_cast<uint32_t>( channel.getWidth() ),
	//	static_cast<uint32_t>( channel.getHeight() ),
	//	static_cast<uint32_t>( channel.getRowBytes() ),
	//	channel.getData(),
	//	mImage.get() );
}

Texture2d::Texture2d( vk::DeviceRef device, const Channel32f &channel, Format format )
	: TextureBase( device, channel.getWidth(), channel.getHeight() ),
	  mActualSize( channel.getSize() ),
	  mCleanBounds( 0, 0, channel.getWidth(), channel.getHeight() )
{
	initImage( VK_FORMAT_R32_SFLOAT, format );
	initSampler( format );
	initViews();

	//getDevice()->copyToImage(
	//	static_cast<uint32_t>( channel.getWidth() ),
	//	static_cast<uint32_t>( channel.getHeight() ),
	//	static_cast<uint32_t>( channel.getRowBytes() ),
	//	channel.getData(),
	//	mImage.get() );
}

template <typename T>
static void copyMipsToImage(
	vk::Device *	   pDevice,
	const ChannelT<T> &mip0,
	vk::Image *		   pDstImage )
{
	// Dims for mip 0
	uint32_t width	   = static_cast<uint32_t>( mip0.getWidth() );
	uint32_t height	   = static_cast<uint32_t>( mip0.getHeight() );
	uint32_t rowBytes  = static_cast<uint32_t>( mip0.getRowBytes() );
	uint32_t increment = mip0.getIncrement();
	// Copy to mip 0
	pDevice->copyToImage( width, height, rowBytes, mip0.getData(), 0, 0, pDstImage );
	// Scale and copy to remaining mips
	const uint32_t numMipLevels = pDstImage->getMipLevels();
	for ( uint32_t mipLevel = 1; mipLevel < numMipLevels; ++mipLevel ) {
		// Calculate dims for current mip
		width >>= 1;
		height >>= 1;
		rowBytes >>= 1;
		// Get staging buffer pointer
		void *pStorage = pDevice->beginCopyToImage( width, height, rowBytes, mipLevel, 0, pDstImage );
		// Scale to current mip from mip 0
		ChannelT<T> mipN = ChannelT<T>( width, height, rowBytes, increment, reinterpret_cast<uint8_t *>( pStorage ) );
		ip::resize( mip0, &mipN, ci::FilterCatmullRom() );
		// Finalize copy
		pDevice->endCopyToImage( width, height, rowBytes, mipLevel, 0, pDstImage );
	}
}

template <typename T>
static void copyMipsToImage(
	vk::Device *	   pDevice,
	const SurfaceT<T> &mip0,
	vk::Image *		   pDstImage )
{
	// Dims for mip 0
	uint32_t width	  = static_cast<uint32_t>( mip0.getWidth() );
	uint32_t height	  = static_cast<uint32_t>( mip0.getHeight() );
	uint32_t rowBytes = static_cast<uint32_t>( mip0.getRowBytes() );
	// Copy to mip 0
	pDevice->copyToImage( width, height, rowBytes, mip0.getData(), 0, 0, pDstImage );
	// Scale and copy to remaining mips
	const uint32_t numMipLevels = pDstImage->getMipLevels();
	for ( uint32_t mipLevel = 1; mipLevel < numMipLevels; ++mipLevel ) {
		// Calculate dims for current mip
		width >>= 1;
		height >>= 1;
		rowBytes >>= 1;
		// Get staging buffer pointer
		void *pStorage = pDevice->beginCopyToImage( width, height, rowBytes, mipLevel, 0, pDstImage );
		// Scale to current mip from mip 0
		Surface8u mipN = Surface8u( reinterpret_cast<uint8_t *>( pStorage ), width, height, rowBytes, mip0.getChannelOrder() );
		ip::resize( mip0, &mipN, ci::FilterCatmullRom() );
		// Finalize copy
		pDevice->endCopyToImage( width, height, rowBytes, mipLevel, 0, pDstImage );
	}
}

Texture2d::Texture2d( vk::DeviceRef device, const ImageSourceRef &imageSource, Format format )
	: TextureBase( device, imageSource->getWidth(), imageSource->getHeight() ),
	  mActualSize( imageSource->getWidth(), imageSource->getHeight() ),
	  mCleanBounds( 0, 0, imageSource->getWidth(), imageSource->getHeight() )
{
	enum ConversionTarget
	{
		CONVERSION_TARGET_NONE,
		CONVERSION_TARGET_RGBA_U8,
		CONVERSION_TARGET_RGBA_U16,
		CONVERSION_TARGET_RGBA_32F,
		CONVERSION_TARGET_RG_32F,
		CONVERSION_TARGET_R_32F,
	};

	ImageIo::ChannelOrder channelOrder	   = imageSource->getChannelOrder();
	ImageIo::ColorModel	  colorModel	   = imageSource->getColorModel();
	ImageIo::DataType	  dataType		   = imageSource->getDataType();
	bool				  isGray		   = ( colorModel == ImageIo::ColorModel::CM_GRAY );
	VkFormat			  imageFormat	   = VK_FORMAT_UNDEFINED;
	ConversionTarget	  conversionTarget = CONVERSION_TARGET_NONE;

	switch ( channelOrder ) {
		default: break;
		case ImageIo::ChannelOrder::RGBA:
		case ImageIo::ChannelOrder::RGBX: {
			switch ( dataType ) {
				default: break;
				case ImageIo::DataType::UINT8: imageFormat = VK_FORMAT_R8G8B8A8_UNORM; break;
				case ImageIo::DataType::UINT16: imageFormat = VK_FORMAT_R16G16B16A16_UNORM; break;
				case ImageIo::DataType::FLOAT32: imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT; break;
				case ImageIo::DataType::FLOAT16: imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT; break;
			}
		} break;

		case ImageIo::ChannelOrder::BGRA:
		case ImageIo::ChannelOrder::BGRX: {
			switch ( dataType ) {
				default: break;
				case ImageIo::DataType::UINT8: imageFormat = VK_FORMAT_B8G8R8A8_UNORM; break;
				case ImageIo::DataType::UINT16: conversionTarget = CONVERSION_TARGET_RGBA_U16; break;
				case ImageIo::DataType::FLOAT32: conversionTarget = CONVERSION_TARGET_RGBA_32F; break;
				case ImageIo::DataType::FLOAT16: conversionTarget = CONVERSION_TARGET_RGBA_32F; break;
			}
		} break;

		case ImageIo::ChannelOrder::ABGR:
		case ImageIo::ChannelOrder::XBGR: {
			switch ( dataType ) {
				default: break;
				case ImageIo::DataType::UINT8: imageFormat = VK_FORMAT_A8B8G8R8_UNORM_PACK32; break;
				case ImageIo::DataType::UINT16: conversionTarget = CONVERSION_TARGET_RGBA_U16; break;
				case ImageIo::DataType::FLOAT32: conversionTarget = CONVERSION_TARGET_RGBA_32F; break;
				case ImageIo::DataType::FLOAT16: conversionTarget = CONVERSION_TARGET_RGBA_32F; break;
			}
		} break;

		case ImageIo::ChannelOrder::ARGB:
		case ImageIo::ChannelOrder::XRGB:
		case ImageIo::ChannelOrder::RGB:
		case ImageIo::ChannelOrder::BGR: {
			switch ( dataType ) {
				default: break;
				case ImageIo::DataType::UINT8: conversionTarget = CONVERSION_TARGET_RGBA_U8; break;
				case ImageIo::DataType::UINT16: conversionTarget = CONVERSION_TARGET_RGBA_U16; break;
				case ImageIo::DataType::FLOAT32: conversionTarget = CONVERSION_TARGET_RGBA_32F; break;
				case ImageIo::DataType::FLOAT16: conversionTarget = CONVERSION_TARGET_RGBA_32F; break;
			}
		} break;

		case ImageIo::ChannelOrder::Y: {
			switch ( dataType ) {
				default: break;
				case ImageIo::DataType::UINT8: imageFormat = VK_FORMAT_R8_UNORM; break;
				case ImageIo::DataType::UINT16: imageFormat = VK_FORMAT_R16_UNORM; break;
				case ImageIo::DataType::FLOAT32: imageFormat = VK_FORMAT_R32_SFLOAT; break;
				case ImageIo::DataType::FLOAT16: imageFormat = VK_FORMAT_R16_SFLOAT; break;
			}
			if ( colorModel == ImageIo::ColorModel::CM_GRAY ) {
				mComponentMapping.r = VK_COMPONENT_SWIZZLE_R;
				mComponentMapping.g = VK_COMPONENT_SWIZZLE_R;
				mComponentMapping.b = VK_COMPONENT_SWIZZLE_R;
				mComponentMapping.a = VK_COMPONENT_SWIZZLE_R;
			}
		} break;

		case ImageIo::ChannelOrder::YA: {
			switch ( dataType ) {
				default: break;
				case ImageIo::DataType::UINT8: imageFormat = VK_FORMAT_R8G8_UNORM; break;
				case ImageIo::DataType::UINT16: imageFormat = VK_FORMAT_R16G16_UNORM; break;
				case ImageIo::DataType::FLOAT32: imageFormat = VK_FORMAT_R32G32_SFLOAT; break;
				case ImageIo::DataType::FLOAT16: imageFormat = VK_FORMAT_R16G16_SFLOAT; break;
			}
		} break;
	}

	initImage( imageFormat, format );
	initSampler( format );
	initViews();

	switch ( conversionTarget ) {
		default: {
			switch ( dataType ) {
				default: {
					throw VulkanExc( "unrecognized data type" );
				} break;

				case ImageIo::DataType::UINT8: {
					if ( isGray ) {
						auto mip0 = Channel8u( imageSource );
						copyMipsToImage<uint8_t>( getDevice().get(), mip0, mImage.get() );
					}
					else {
						auto mip0 = Surface8u( imageSource );
						copyMipsToImage<uint8_t>( getDevice().get(), mip0, mImage.get() );
					}
				} break;
			}
		} break;

		case CONVERSION_TARGET_RGBA_U8: {
		} break;

		case CONVERSION_TARGET_RGBA_U16: {
		} break;

		case CONVERSION_TARGET_RGBA_32F: {
		} break;

		case CONVERSION_TARGET_RG_32F: {
		} break;

		case CONVERSION_TARGET_R_32F: {
		} break;
	}
}

Texture2d::~Texture2d()
{
}

void Texture2d::initViews()
{
	vk::ImageView::Options options = vk::ImageView::Options( mImage.get() ).components( mComponentMapping );

	mSampledImage = vk::ImageView::create( mImage, options, getDevice() );
}

void Texture2d::bind( uint32_t binding )
{
	auto ctx = vk::context();
	ctx->bindTexture( binding, mSampledImage.get(), mSampler.get() );
}

} // namespace cinder::vk
