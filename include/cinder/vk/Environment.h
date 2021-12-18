#pragma once

#include "cinder/vk/vk_config.h"
#include "cinder/vk/InstanceDispatchTable.h"
#include "cinder/app/RendererVk.h"

#define CI_VK_INSTANCE_FN( FN_WITH_ARGS ) \
	ci::vk::Environment::get()->vkfn()->FN_WITH_ARGS

namespace cinder::vk {

class Environment
{
private:
	Environment( const std::string &appName, const app::RendererVk::Options &options );

public:
	~Environment();

	static void			initialize( const std::string &appName, const app::RendererVk::Options &options );
	static Environment *get();

	static std::vector<VkPhysicalDeviceProperties> enumerateGpus();

	const InstanceDispatchTable *vkfn() const { return &mVkFn; }

	uint32_t getApiVersion() const { return mApiVersion; }

	VkInstance getInstanceHandle() const { return mInstanceHandle; }

	VkPhysicalDevice getGpuHandle( uint32_t index ) const;

	QueueFamilyIndices getQueueFamilyIndices( VkPhysicalDevice gpu ) const;

private:
#if defined( CINDER_MSW )
	HMODULE mVulkanDLL = nullptr;
#endif
	InstanceDispatchTable mVkFn			  = {};
	uint32_t			  mApiVersion	  = 0;
	VkInstance			  mInstanceHandle = VK_NULL_HANDLE;
};

} // namespace cinder::vk
