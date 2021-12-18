#include "cinder/vk/Sampler.h"
#include "cinder/vk/Device.h"
#include "cinder/app/RendererVk.h"

namespace cinder::vk {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sampler

vk::SamplerRef Sampler::create( const Options &options, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return vk::SamplerRef( new Sampler( device, options ) );
}

Sampler::Sampler( vk::DeviceRef device, const Options &options )
	: vk::DeviceChildObject( device )
{
	VkSamplerCreateInfo vkci	 = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	vkci.pNext					 = nullptr;
	vkci.flags					 = options.getCreateFlags();
	vkci.magFilter				 = options.getMagFilter();
	vkci.minFilter				 = options.getMinFilter();
	vkci.mipmapMode				 = options.getMipmapMode();
	vkci.addressModeU			 = options.getAddressModeU();
	vkci.addressModeV			 = options.getAddressModeV();
	vkci.addressModeW			 = options.getAddressModeW();
	vkci.mipLodBias				 = options.getMipLodBias();
	vkci.anisotropyEnable		 = options.getMaxAnisotropy() >= 1.0f ? VK_TRUE : VK_FALSE;
	vkci.maxAnisotropy			 = options.getMaxAnisotropy();
	vkci.compareEnable			 = options.getCompareOp() >= VK_COMPARE_OP_NEVER;
	vkci.compareOp				 = options.getCompareOp();
	vkci.minLod					 = options.getMinLod();
	vkci.maxLod					 = options.getMaxLod();
	vkci.borderColor			 = options.getBorderColor();
	vkci.unnormalizedCoordinates = options.getUnnormalizedCoordinates();

	VkResult vkres = CI_VK_DEVICE_FN( CreateSampler( getDeviceHandle(), &vkci, nullptr, &mSamplerHandle ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkCreateSampler", vkres );
	}
}

Sampler::~Sampler()
{
	if ( mSamplerHandle != VK_NULL_HANDLE ) {
		CI_VK_DEVICE_FN( DestroySampler( getDeviceHandle(), mSamplerHandle, nullptr ) );
		mSamplerHandle = VK_NULL_HANDLE;
	}
}

} // namespace cinder::vk
