#pragma once

#include "cinder/vk/ChildObject.h"

namespace cinder::vk {

class DescriptorPool;

//! @class DescriptorSetLayout
//!
//! Yes, it's legal to create an empty VkDescriptorSetLayout.
//!
class DescriptorSetLayout
	: public vk::DeviceChildObject
{
public:
	static const VkShaderStageFlags DEFAULT_STAGE_FLAGS = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	struct Options
	{
		Options() {}

		//
		// For all graphics stages, use VK_SHADER_STAGE_ALL_GRAPHICS for stageFlags.
		//
		// clang-format off
		Options& flags( VkDescriptorSetLayoutCreateFlags flags ) { mFlags = flags; return *this; }
		Options& pushDescriptor(bool value  = true ) { vk::changeFlagBit(mFlags, VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR, value); return *this; }
		Options& updateAfterBind(bool value = true ) { vk::changeFlagBit(mFlags, VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT, value); return *this; }
		Options& addSampler( uint32_t binding, uint32_t count = 1, VkShaderStageFlags stageFlags = DEFAULT_STAGE_FLAGS ) { mBindings.push_back( { binding, VK_DESCRIPTOR_TYPE_SAMPLER, count, stageFlags } ); return *this; }
		Options& addCombinedImageSampler( uint32_t binding, uint32_t count = 1, VkShaderStageFlags stageFlags = DEFAULT_STAGE_FLAGS ) { mBindings.push_back( { binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count, stageFlags } ); return *this; }
		Options& addSampledImage( uint32_t binding, uint32_t count = 1, VkShaderStageFlags stageFlags = DEFAULT_STAGE_FLAGS ) { mBindings.push_back( { binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, count, stageFlags } ); return *this; }
		Options& addStorageImage( uint32_t binding, uint32_t count = 1, VkShaderStageFlags stageFlags = DEFAULT_STAGE_FLAGS ) { mBindings.push_back( { binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, count, stageFlags } ); return *this; }
		Options& addUniformBuffer( uint32_t binding, uint32_t count = 1, VkShaderStageFlags stageFlags = DEFAULT_STAGE_FLAGS ) { mBindings.push_back( { binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count, stageFlags } ); return *this; }
		Options& addStorageBuffer( uint32_t binding, uint32_t count = 1, VkShaderStageFlags stageFlags = DEFAULT_STAGE_FLAGS ) { mBindings.push_back( { binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count, stageFlags } ); return *this; }
		Options& addUniformBufferDynamic( uint32_t binding, uint32_t count = 1, VkShaderStageFlags stageFlags = DEFAULT_STAGE_FLAGS ) { mBindings.push_back( { binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, count, stageFlags } ); return *this; }
		Options& addStorageBufferDynamic( uint32_t binding, uint32_t count = 1, VkShaderStageFlags stageFlags = DEFAULT_STAGE_FLAGS ) { mBindings.push_back( { binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, count, stageFlags } ); return *this; }
		// clang-format on

	private:
		VkDescriptorSetLayoutCreateFlags		  mFlags = 0;
		std::vector<VkDescriptorSetLayoutBinding> mBindings;

		friend class DescriptorSetLayout;
	};

	virtual ~DescriptorSetLayout();

	static DescriptorSetLayoutRef create( const Options &options = Options(), vk::DeviceRef device = vk::DeviceRef() );

	VkDescriptorSetLayout getDescriptorSetLayoutHandle() const { return mDescriptorSetLayoutHandle; }

private:
	DescriptorSetLayout( vk::DeviceRef device, const Options &options );

private:
	VkDescriptorSetLayout mDescriptorSetLayoutHandle = VK_NULL_HANDLE;
};

//! @class DescriptorSet
//!
//! Yes, it's legal to allocate a VkDescriptorSet using an empty VkDescriptorsetLayout.
//!
class DescriptorSet
	: public vk::DeviceChildObject
{
public:
	virtual ~DescriptorSet();

	static vk::DescriptorSetRef create( vk::DescriptorPoolRef pool, const vk::DescriptorSetLayoutRef& layout );

	VkDescriptorSet getDescriptorSetHandle() const { return mDescriptorSetHandle; }

private:
	DescriptorSet( vk::DescriptorPoolRef pool, const vk::DescriptorSetLayoutRef& layout );
	friend class DescriptorPool;

private:
	vk::DescriptorPoolRef mPool;
	VkDescriptorSet		  mDescriptorSetHandle = VK_NULL_HANDLE;
};

//! @class DescriptorPool
//!
//!
class DescriptorPool
	: public vk::DeviceChildObject
{
public:
	struct Options
	{
		Options() {}

		// clang-format off
		Options& flags( VkDescriptorPoolCreateFlags value ) { mFlags = value; return *this; }
		Options& updateAfterBind(bool value = true) { vk::changeFlagBit(mFlags, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT, value); return *this; } 
		Options& maxSets( uint32_t value ) { mMaxSets = value; return *this; }
		Options& addSampler( uint32_t descriptorCount ) { mPoolSizes.push_back( { VK_DESCRIPTOR_TYPE_SAMPLER, descriptorCount } ); return *this; }
		Options& addCombinedImageSampler( uint32_t descriptorCount ) { mPoolSizes.push_back( { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorCount } ); return *this; }
		Options& addSampledImage( uint32_t descriptorCount ) { mPoolSizes.push_back( { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, descriptorCount } ); return *this; }
		Options& addStorageImage( uint32_t descriptorCount ) { mPoolSizes.push_back( { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, descriptorCount } ); return *this; }
		Options& addUniformBuffer( uint32_t descriptorCount ) { mPoolSizes.push_back( { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorCount } ); return *this; }
		Options& addStorageBuffer( uint32_t descriptorCount ) { mPoolSizes.push_back( { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptorCount } ); return *this; }
		Options& addUniformBufferDynamic( uint32_t descriptorCount ) { mPoolSizes.push_back( {  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, descriptorCount } ); return *this; }
		Options& addStorageBufferDynamic( uint32_t descriptorCount ) { mPoolSizes.push_back( {  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, descriptorCount } ); return *this; }
		// clang-format on

	private:
		uint32_t						  mMaxSets = 0; // If 0 then sum of all pool sizes is used
		VkDescriptorPoolCreateFlags		  mFlags   = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		std::vector<VkDescriptorPoolSize> mPoolSizes;

		friend class DescriptorPool;
	};

	virtual ~DescriptorPool();

	static DescriptorPoolRef create( const Options &options = Options(), vk::DeviceRef device = vk::DeviceRef() );

	VkDescriptorPool getDescriptorPoolHandle() const { return mDescriptorPoolHandle; }

private:
	DescriptorPool( vk::DeviceRef device, const Options &options );

private:
	VkDescriptorPool mDescriptorPoolHandle = VK_NULL_HANDLE;
};

} // namespace cinder::vk
