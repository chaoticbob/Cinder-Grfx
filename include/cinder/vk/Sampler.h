#pragma once

#include "cinder/vk/DeviceChildObject.h"

namespace cinder::vk {

class Sampler
	: public vk::DeviceChildObject
{
public:
	struct Options
	{
		Options() {}

		// clang-format off
		Options &flags( VkSamplerCreateFlags value ) { mCreateFlags = value; return *this;  }
		Options &magFilter( VkFilter value ) { mMagFilter = value; return *this;  }
		Options &minFilter( VkFilter value ) { mMinFilter = value; return *this;  }
		Options &mipmapMode( VkSamplerMipmapMode value ) { mMipmapMode = value; return *this;  }
		Options &addressModeU( VkSamplerAddressMode value ) { mAddressModeU = value; return *this;  }
		Options &addressModeV( VkSamplerAddressMode value ) { mAddressModeV = value; return *this;  }
		Options &addressModeW( VkSamplerAddressMode value ) { mAddressModeW = value; return *this;  }
		Options &mipLodBias( float value ) { mMipLodBias = value; return *this;  }
		//! Anisotropic filtering enabled if value >= 1 and <= maxSamplerAnisotropy. Any value < 1 disables anisotropic filtering.
		Options &maxAnisotropy( float value ) { mMaxAnisotropy = value; return *this;  }
		//! VK_COMPARE_OP_NEVER = disabled, everything else = enabled
		Options &compareOp( VkCompareOp value ) { mCompareOp = value; return *this;  }
		Options &minLod( float value ) { mMinLod = value; return *this;  }
		Options &maxLod( float value ) { mMaxLod = value; return *this;  }
		Options &borderColor( VkBorderColor value ) { mBorderColor = value; return *this;  }
		Options &unnormalizedCoordinates( VkBool32 value ) { mUnnormalizedCoordinates = value; return *this;  }
		// clang-format on

		VkSamplerCreateFlags getCreateFlags() const { return mCreateFlags; }
		VkFilter			 getMagFilter() const { return mMagFilter; }
		VkFilter			 getMinFilter() const { return mMinFilter; }
		VkSamplerMipmapMode	 getMipmapMode() const { return mMipmapMode; }
		VkSamplerAddressMode getAddressModeU() const { return mAddressModeU; }
		VkSamplerAddressMode getAddressModeV() const { return mAddressModeV; }
		VkSamplerAddressMode getAddressModeW() const { return mAddressModeW; }
		float				 getMipLodBias() const { return mMipLodBias; }
		float				 getMaxAnisotropy() const { return mMaxAnisotropy; }
		VkCompareOp			 getCompareOp() const { return mCompareOp; }
		float				 getMinLod() const { return mMinLod; }
		float				 getMaxLod() const { return mMaxLod; }
		VkBorderColor		 getBorderColor() const { return mBorderColor; }
		VkBool32			 getUnnormalizedCoordinates() const { return mUnnormalizedCoordinates; }

	private:
		VkSamplerCreateFlags mCreateFlags			  = 0;
		VkFilter			 mMagFilter				  = VK_FILTER_NEAREST;
		VkFilter			 mMinFilter				  = VK_FILTER_NEAREST;
		VkSamplerMipmapMode	 mMipmapMode			  = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		VkSamplerAddressMode mAddressModeU			  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		VkSamplerAddressMode mAddressModeV			  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		VkSamplerAddressMode mAddressModeW			  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		float				 mMipLodBias			  = 0.0f;
		float				 mMaxAnisotropy			  = 0.0f;
		VkCompareOp			 mCompareOp				  = VK_COMPARE_OP_NEVER;
		float				 mMinLod				  = 0.0f;
		float				 mMaxLod				  = VK_LOD_CLAMP_NONE;
		VkBorderColor		 mBorderColor			  = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		VkBool32			 mUnnormalizedCoordinates = VK_FALSE;
	};

	virtual ~Sampler();

	static vk::SamplerRef create( const Options &options = Options(), vk::DeviceRef device = vk::DeviceRef() );

private:
	Sampler( vk::DeviceRef device, const Options &options );

private:
	VkSampler mSamplerHandle = VK_NULL_HANDLE;
};

} // namespace cinder::vk
