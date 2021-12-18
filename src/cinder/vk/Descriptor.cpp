#include "cinder/vk/Descriptor.h"
#include "cinder/vk/Device.h"
#include "cinder/app/RendererVk.h"

namespace cinder::vk {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DescriptorSetLayout

DescriptorSetLayoutRef DescriptorSetLayout::create( const Options &options, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return DescriptorSetLayoutRef( new DescriptorSetLayout( device, options ) );
}

DescriptorSetLayout::DescriptorSetLayout( vk::DeviceRef device, const Options &options )
	: vk::DeviceChildObject( device )
{
	VkDescriptorSetLayoutCreateInfo vkci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	vkci.pNext							 = nullptr;
	vkci.flags							 = options.mFlags;
	vkci.bindingCount					 = countU32( options.mBindings );
	vkci.pBindings						 = dataPtr( options.mBindings );

	VkResult vkres = CI_VK_DEVICE_FN( CreateDescriptorSetLayout(
		getDeviceHandle(),
		&vkci,
		nullptr,
		&mDescriptorSetLayoutHandle ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkCreateDescriptorSetLayout", vkres );
	}
}

DescriptorSetLayout::~DescriptorSetLayout()
{
	if ( mDescriptorSetLayoutHandle != VK_NULL_HANDLE ) {
		CI_VK_DEVICE_FN( DestroyDescriptorSetLayout(
			getDeviceHandle(),
			mDescriptorSetLayoutHandle,
			nullptr ) );
		mDescriptorSetLayoutHandle = VK_NULL_HANDLE;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DescriptorSet

DescriptorSet::DescriptorSet( vk::DeviceRef device, VkDescriptorSet handle )
	: vk::DeviceChildObject( device ),
	  mDescriptorSetHandle( handle )
{
}

DescriptorSet::~DescriptorSet()
{
	mDescriptorSetHandle = VK_NULL_HANDLE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DescriptorPool

DescriptorPoolRef DescriptorPool::create( const Options &options, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return DescriptorPoolRef( new DescriptorPool( device, options ) );
}

DescriptorPool::DescriptorPool( vk::DeviceRef device, const Options &options )
	: vk::DeviceChildObject( device )
{
	uint32_t maxSets = options.mMaxSets;
	if ( maxSets == 0 ) {
		for ( size_t i = 0; i < options.mPoolSizes.size(); ++i ) {
			maxSets += options.mPoolSizes[i].descriptorCount;
		}
	}

	VkDescriptorPoolCreateInfo vkci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	vkci.pNext						= nullptr;
	vkci.flags						= options.mFlags;
	vkci.maxSets					= maxSets;
	vkci.poolSizeCount				= countU32( options.mPoolSizes );
	vkci.pPoolSizes					= dataPtr( options.mPoolSizes );

	VkResult vkres = CI_VK_DEVICE_FN( CreateDescriptorPool(
		getDeviceHandle(),
		&vkci,
		nullptr,
		&mDescriptorPoolHandle ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkCreateDescriptorPool", vkres );
	}
}

DescriptorPool::~DescriptorPool()
{
	if ( mDescriptorPoolHandle != VK_NULL_HANDLE ) {
		CI_VK_DEVICE_FN( DestroyDescriptorPool(
			getDeviceHandle(),
			mDescriptorPoolHandle,
			nullptr ) );
		mDescriptorPoolHandle = VK_NULL_HANDLE;
	}
}

} // namespace cinder::vk
