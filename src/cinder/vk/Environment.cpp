#include "cinder/vk/Environment.h"
#include "cinder/app/AppBase.h"

namespace cinder::vk {

static std::unique_ptr<Environment> sEnvironment;

Environment::Environment( const std::string &appName, const app::RendererVk::Options &options )
	: mApiVersion( options.getApiVersion() )
{
#if defined( CINDER_MSW )
	mVulkanDLL									= LoadLibraryA( "vulkan-1.dll" );
	PFN_vkCreateInstance	  createInstanceFn	= (PFN_vkCreateInstance)GetProcAddress( mVulkanDLL, "vkCreateInstance" );
	PFN_vkGetInstanceProcAddr getInstanceProcFn = (PFN_vkGetInstanceProcAddr)GetProcAddress( mVulkanDLL, "vkGetInstanceProcAddr" );
#endif

	if ( mApiVersion < VK_API_VERSION_1_1 ) {
		throw VulkanExc( "unsupported Vulkan API version" );
	}

	VkApplicationInfo appInfo  = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pNext			   = nullptr;
	appInfo.pApplicationName   = appName.c_str();
	appInfo.applicationVersion = 0;
	appInfo.pEngineName		   = "Cinder";
	appInfo.engineVersion	   = CINDER_VERSION;
	appInfo.apiVersion		   = mApiVersion;

	std::vector<const char *> layers;
	if ( options.getEnableValidation() ) {
		layers.push_back( VK_KHR_KHRONOS_VALIDATION_LAYER_NAME );
	}

	std::vector<const char *> extensions;
	if ( options.getEnableValidation() ) {
		extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
	}
#if defined( CINDER_MSW )
	extensions.push_back( VK_KHR_SURFACE_EXTENSION_NAME );
	extensions.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
#elif defined( CINDER_LINUX ) && !defined( CINDER_HEADLESS )
	extensions.push_back( VK_KHR_SURFACE_EXTENSION_NAME );
	extensions.push_back( VK_KHR_XCB_SURFACE_EXTENSION_NAME );
#else
#error "platform does not support Vulkan surface"
#endif

	VkInstanceCreateInfo vkci	 = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	vkci.pNext					 = nullptr;
	vkci.flags					 = 0;
	vkci.pApplicationInfo		 = &appInfo;
	vkci.enabledLayerCount		 = countU32( layers );
	vkci.ppEnabledLayerNames	 = dataPtr( layers );
	vkci.enabledExtensionCount	 = countU32( extensions );
	vkci.ppEnabledExtensionNames = dataPtr( extensions );

	VkResult vkres = createInstanceFn( &vkci, nullptr, &mInstanceHandle );
	if ( vkres != VK_SUCCESS ) {
		throw grfx::GraphicsApiExc( "vkCreateInstance failed" );
	}

	LoadInstanceFunctions( getInstanceProcFn, mInstanceHandle, &mVkFn );

	cinder::app::console() << "Vulkan instance created for Vulkan " << VK_API_VERSION_MAJOR( mApiVersion ) << "." << VK_API_VERSION_MINOR( mApiVersion ) << std::endl;
	cinder::app::console() << "Instance layers loaded:" << std::endl;
	for ( auto &ext : layers ) {
		cinder::app::console() << "   " << ext << std::endl;
	}
	cinder::app::console() << "Instance extensions loaded:" << std::endl;
	for ( auto &ext : extensions ) {
		cinder::app::console() << "   " << ext << std::endl;
	}
}

Environment::~Environment()
{
	if ( mInstanceHandle ) {
		vkfn()->DestroyInstance( mInstanceHandle, nullptr );
		mInstanceHandle = VK_NULL_HANDLE;
	}
	mVkFn = {};
}

std::vector<VkPhysicalDeviceProperties> Environment::enumerateGpus()
{
	Environment *pEnvironment = nullptr;
	bool		 disposeEnv	  = true;
	if ( sEnvironment ) {
		pEnvironment = sEnvironment.get();
		disposeEnv	 = false;
	}
	else {
		pEnvironment = new Environment( "Cinder Gpu Enumeration", app::RendererVk::Options() );
		if ( pEnvironment == nullptr ) {
			throw grfx::GraphicsApiExc( "failed to create Vulkan environment for GPU enumeration" );
		}
	}

	uint32_t count = 0;
	VkResult vkres = pEnvironment->vkfn()->EnumeratePhysicalDevices( pEnvironment->getInstanceHandle(), &count, nullptr );
	if ( vkres != VK_SUCCESS ) {
		throw grfx::GraphicsApiExc( "vkEnumeratePhysicalDevices failed" );
	}

	std::vector<VkPhysicalDevice> devices( count );
	vkres = pEnvironment->vkfn()->EnumeratePhysicalDevices( pEnvironment->getInstanceHandle(), &count, devices.data() );
	if ( vkres != VK_SUCCESS ) {
		throw grfx::GraphicsApiExc( "vkEnumeratePhysicalDevices failed" );
	}

	std::vector<VkPhysicalDeviceProperties> properties( count );
	for ( uint32_t i = 0; i < count; ++i ) {
		pEnvironment->vkfn()->GetPhysicalDeviceProperties( devices[i], &properties[i] );
	}

	return properties;
}

void Environment::initialize( const std::string &appName, const app::RendererVk::Options &options )
{
	if ( sEnvironment ) {
		return;
	}

	Environment *pEnvironment = new Environment( appName, options );
	sEnvironment			  = std::unique_ptr<Environment>( pEnvironment );
}

Environment *Environment::get()
{
	return sEnvironment.get();
}

VkPhysicalDevice Environment::getGpuHandle( uint32_t index ) const
{
	uint32_t count = 0;
	VkResult vkres = vkfn()->EnumeratePhysicalDevices( getInstanceHandle(), &count, nullptr );
	if ( vkres != VK_SUCCESS ) {
		throw grfx::GraphicsApiExc( "vkEnumeratePhysicalDevices failed" );
	}

	std::vector<VkPhysicalDevice> gpuHandles( count );
	vkres = vkfn()->EnumeratePhysicalDevices( getInstanceHandle(), &count, gpuHandles.data() );
	if ( vkres != VK_SUCCESS ) {
		throw grfx::GraphicsApiExc( "vkEnumeratePhysicalDevices failed" );
	}

	VkPhysicalDevice gpuHandle = VK_NULL_HANDLE;
	if ( index < count ) {
		gpuHandle = gpuHandles[index];
	}

	return gpuHandle;
}

QueueFamilyIndices Environment::getQueueFamilyIndices( VkPhysicalDevice gpu ) const
{
	uint32_t count = 0;
	vkfn()->GetPhysicalDeviceQueueFamilyProperties( gpu, &count, nullptr );

	std::vector<VkQueueFamilyProperties> propertiesArray( count );
	vkfn()->GetPhysicalDeviceQueueFamilyProperties( gpu, &count, propertiesArray.data() );

	QueueFamilyIndices indices = {};
	if ( count > 0 ) {
		for ( uint32_t i = 0; i < count; ++i ) {
			const auto &properties = propertiesArray[i];
			if ( ( properties.queueFlags & CINDER_VK_QUEUE_MASK_ALL ) == CINDER_VK_QUEUE_MASK_GRAPHICS ) {
				indices.graphics = i;
			}
			else if ( ( properties.queueFlags & CINDER_VK_QUEUE_MASK_ALL ) == CINDER_VK_QUEUE_MASK_COMPUTE ) {
				indices.compute = i;
			}
			else if ( ( properties.queueFlags & CINDER_VK_QUEUE_MASK_ALL ) == CINDER_VK_QUEUE_MASK_TRANSFER ) {
				indices.transfer = i;
			}
		}
	}

	return indices;
}

} // namespace cinder::vk
