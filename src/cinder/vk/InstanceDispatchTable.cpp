#include "cinder/vk/InstanceDispatchTable.h"

namespace cinder::vk {

void LoadInstanceFunctions(
	PFN_vkGetInstanceProcAddr		   loadFn,
	VkInstance						   instance,
	cinder::vk::InstanceDispatchTable *pTable )
{
	// ---- Core 1_0 commands
	pTable->CreateInstance								 = (PFN_vkCreateInstance)loadFn( instance, "vkCreateInstance" );
	pTable->DestroyInstance								 = (PFN_vkDestroyInstance)loadFn( instance, "vkDestroyInstance" );
	pTable->EnumeratePhysicalDevices					 = (PFN_vkEnumeratePhysicalDevices)loadFn( instance, "vkEnumeratePhysicalDevices" );
	pTable->GetPhysicalDeviceFeatures					 = (PFN_vkGetPhysicalDeviceFeatures)loadFn( instance, "vkGetPhysicalDeviceFeatures" );
	pTable->GetPhysicalDeviceFormatProperties			 = (PFN_vkGetPhysicalDeviceFormatProperties)loadFn( instance, "vkGetPhysicalDeviceFormatProperties" );
	pTable->GetPhysicalDeviceImageFormatProperties		 = (PFN_vkGetPhysicalDeviceImageFormatProperties)loadFn( instance, "vkGetPhysicalDeviceImageFormatProperties" );
	pTable->GetPhysicalDeviceProperties					 = (PFN_vkGetPhysicalDeviceProperties)loadFn( instance, "vkGetPhysicalDeviceProperties" );
	pTable->GetPhysicalDeviceQueueFamilyProperties		 = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)loadFn( instance, "vkGetPhysicalDeviceQueueFamilyProperties" );
	pTable->GetPhysicalDeviceMemoryProperties			 = (PFN_vkGetPhysicalDeviceMemoryProperties)loadFn( instance, "vkGetPhysicalDeviceMemoryProperties" );
	pTable->GetInstanceProcAddr							 = (PFN_vkGetInstanceProcAddr)loadFn( instance, "vkGetInstanceProcAddr" );
	pTable->GetDeviceProcAddr							 = (PFN_vkGetDeviceProcAddr)loadFn( instance, "vkGetDeviceProcAddr" );
	pTable->CreateDevice								 = (PFN_vkCreateDevice)loadFn( instance, "vkCreateDevice" );
	pTable->DestroyDevice								 = (PFN_vkDestroyDevice)loadFn( instance, "vkDestroyDevice" );
	pTable->EnumerateInstanceExtensionProperties		 = (PFN_vkEnumerateInstanceExtensionProperties)loadFn( instance, "vkEnumerateInstanceExtensionProperties" );
	pTable->EnumerateDeviceExtensionProperties			 = (PFN_vkEnumerateDeviceExtensionProperties)loadFn( instance, "vkEnumerateDeviceExtensionProperties" );
	pTable->EnumerateInstanceLayerProperties			 = (PFN_vkEnumerateInstanceLayerProperties)loadFn( instance, "vkEnumerateInstanceLayerProperties" );
	pTable->EnumerateDeviceLayerProperties				 = (PFN_vkEnumerateDeviceLayerProperties)loadFn( instance, "vkEnumerateDeviceLayerProperties" );
	pTable->GetPhysicalDeviceSparseImageFormatProperties = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties)loadFn( instance, "vkGetPhysicalDeviceSparseImageFormatProperties" );

	// ---- Core 1_1 commands
	pTable->EnumerateInstanceVersion					  = (PFN_vkEnumerateInstanceVersion)loadFn( instance, "vkEnumerateInstanceVersion" );
	pTable->EnumeratePhysicalDeviceGroups				  = (PFN_vkEnumeratePhysicalDeviceGroups)loadFn( instance, "vkEnumeratePhysicalDeviceGroups" );
	pTable->GetPhysicalDeviceFeatures2					  = (PFN_vkGetPhysicalDeviceFeatures2)loadFn( instance, "vkGetPhysicalDeviceFeatures2" );
	pTable->GetPhysicalDeviceProperties2				  = (PFN_vkGetPhysicalDeviceProperties2)loadFn( instance, "vkGetPhysicalDeviceProperties2" );
	pTable->GetPhysicalDeviceFormatProperties2			  = (PFN_vkGetPhysicalDeviceFormatProperties2)loadFn( instance, "vkGetPhysicalDeviceFormatProperties2" );
	pTable->GetPhysicalDeviceImageFormatProperties2		  = (PFN_vkGetPhysicalDeviceImageFormatProperties2)loadFn( instance, "vkGetPhysicalDeviceImageFormatProperties2" );
	pTable->GetPhysicalDeviceQueueFamilyProperties2		  = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2)loadFn( instance, "vkGetPhysicalDeviceQueueFamilyProperties2" );
	pTable->GetPhysicalDeviceMemoryProperties2			  = (PFN_vkGetPhysicalDeviceMemoryProperties2)loadFn( instance, "vkGetPhysicalDeviceMemoryProperties2" );
	pTable->GetPhysicalDeviceSparseImageFormatProperties2 = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties2)loadFn( instance, "vkGetPhysicalDeviceSparseImageFormatProperties2" );
	pTable->GetPhysicalDeviceExternalBufferProperties	  = (PFN_vkGetPhysicalDeviceExternalBufferProperties)loadFn( instance, "vkGetPhysicalDeviceExternalBufferProperties" );
	pTable->GetPhysicalDeviceExternalFenceProperties	  = (PFN_vkGetPhysicalDeviceExternalFenceProperties)loadFn( instance, "vkGetPhysicalDeviceExternalFenceProperties" );
	pTable->GetPhysicalDeviceExternalSemaphoreProperties  = (PFN_vkGetPhysicalDeviceExternalSemaphoreProperties)loadFn( instance, "vkGetPhysicalDeviceExternalSemaphoreProperties" );

	// ---- VK_KHR_surface extension commands
	pTable->DestroySurfaceKHR						= (PFN_vkDestroySurfaceKHR)loadFn( instance, "vkDestroySurfaceKHR" );
	pTable->GetPhysicalDeviceSurfaceSupportKHR		= (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)loadFn( instance, "vkGetPhysicalDeviceSurfaceSupportKHR" );
	pTable->GetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)loadFn( instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR" );
	pTable->GetPhysicalDeviceSurfaceFormatsKHR		= (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)loadFn( instance, "vkGetPhysicalDeviceSurfaceFormatsKHR" );
	pTable->GetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)loadFn( instance, "vkGetPhysicalDeviceSurfacePresentModesKHR" );

	// ---- VK_KHR_swapchain extension commands
	pTable->GetPhysicalDevicePresentRectanglesKHR = (PFN_vkGetPhysicalDevicePresentRectanglesKHR)loadFn( instance, "vkGetPhysicalDevicePresentRectanglesKHR" );

	// ---- VK_KHR_display extension commands
	pTable->GetPhysicalDeviceDisplayPropertiesKHR	   = (PFN_vkGetPhysicalDeviceDisplayPropertiesKHR)loadFn( instance, "vkGetPhysicalDeviceDisplayPropertiesKHR" );
	pTable->GetPhysicalDeviceDisplayPlanePropertiesKHR = (PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR)loadFn( instance, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR" );
	pTable->GetDisplayPlaneSupportedDisplaysKHR		   = (PFN_vkGetDisplayPlaneSupportedDisplaysKHR)loadFn( instance, "vkGetDisplayPlaneSupportedDisplaysKHR" );
	pTable->GetDisplayModePropertiesKHR				   = (PFN_vkGetDisplayModePropertiesKHR)loadFn( instance, "vkGetDisplayModePropertiesKHR" );
	pTable->CreateDisplayModeKHR					   = (PFN_vkCreateDisplayModeKHR)loadFn( instance, "vkCreateDisplayModeKHR" );
	pTable->GetDisplayPlaneCapabilitiesKHR			   = (PFN_vkGetDisplayPlaneCapabilitiesKHR)loadFn( instance, "vkGetDisplayPlaneCapabilitiesKHR" );
	pTable->CreateDisplayPlaneSurfaceKHR			   = (PFN_vkCreateDisplayPlaneSurfaceKHR)loadFn( instance, "vkCreateDisplayPlaneSurfaceKHR" );

	// ---- VK_KHR_xlib_surface extension commands
#ifdef VK_USE_PLATFORM_XLIB_KHR
	pTable->CreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)loadFn( instance, "vkCreateXlibSurfaceKHR" );
#endif // VK_USE_PLATFORM_XLIB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
	pTable->GetPhysicalDeviceXlibPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)loadFn( instance, "vkGetPhysicalDeviceXlibPresentationSupportKHR" );
#endif // VK_USE_PLATFORM_XLIB_KHR

	// ---- VK_KHR_xcb_surface extension commands
#ifdef VK_USE_PLATFORM_XCB_KHR
	pTable->CreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)loadFn( instance, "vkCreateXcbSurfaceKHR" );
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
	pTable->GetPhysicalDeviceXcbPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR)loadFn( instance, "vkGetPhysicalDeviceXcbPresentationSupportKHR" );
#endif // VK_USE_PLATFORM_XCB_KHR

	// ---- VK_KHR_wayland_surface extension commands
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
	pTable->CreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)loadFn( instance, "vkCreateWaylandSurfaceKHR" );
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
	pTable->GetPhysicalDeviceWaylandPresentationSupportKHR = (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR)loadFn( instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR" );
#endif // VK_USE_PLATFORM_WAYLAND_KHR

	// ---- VK_KHR_android_surface extension commands
#ifdef VK_USE_PLATFORM_ANDROID_KHR
	pTable->CreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR)loadFn( instance, "vkCreateAndroidSurfaceKHR" );
#endif // VK_USE_PLATFORM_ANDROID_KHR

	// ---- VK_KHR_win32_surface extension commands
#ifdef VK_USE_PLATFORM_WIN32_KHR
	pTable->CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)loadFn( instance, "vkCreateWin32SurfaceKHR" );
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
	pTable->GetPhysicalDeviceWin32PresentationSupportKHR = (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR)loadFn( instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR" );
#endif // VK_USE_PLATFORM_WIN32_KHR

	// ---- VK_KHR_video_queue extension commands
#ifdef VK_ENABLE_BETA_EXTENSIONS
	pTable->GetPhysicalDeviceVideoCapabilitiesKHR = (PFN_vkGetPhysicalDeviceVideoCapabilitiesKHR)loadFn( instance, "vkGetPhysicalDeviceVideoCapabilitiesKHR" );
#endif // VK_ENABLE_BETA_EXTENSIONS
#ifdef VK_ENABLE_BETA_EXTENSIONS
	pTable->GetPhysicalDeviceVideoFormatPropertiesKHR = (PFN_vkGetPhysicalDeviceVideoFormatPropertiesKHR)loadFn( instance, "vkGetPhysicalDeviceVideoFormatPropertiesKHR" );
#endif // VK_ENABLE_BETA_EXTENSIONS

	// ---- VK_KHR_get_physical_device_properties2 extension commands
	pTable->GetPhysicalDeviceFeatures2KHR					 = (PFN_vkGetPhysicalDeviceFeatures2KHR)loadFn( instance, "vkGetPhysicalDeviceFeatures2KHR" );
	pTable->GetPhysicalDeviceProperties2KHR					 = (PFN_vkGetPhysicalDeviceProperties2KHR)loadFn( instance, "vkGetPhysicalDeviceProperties2KHR" );
	pTable->GetPhysicalDeviceFormatProperties2KHR			 = (PFN_vkGetPhysicalDeviceFormatProperties2KHR)loadFn( instance, "vkGetPhysicalDeviceFormatProperties2KHR" );
	pTable->GetPhysicalDeviceImageFormatProperties2KHR		 = (PFN_vkGetPhysicalDeviceImageFormatProperties2KHR)loadFn( instance, "vkGetPhysicalDeviceImageFormatProperties2KHR" );
	pTable->GetPhysicalDeviceQueueFamilyProperties2KHR		 = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR)loadFn( instance, "vkGetPhysicalDeviceQueueFamilyProperties2KHR" );
	pTable->GetPhysicalDeviceMemoryProperties2KHR			 = (PFN_vkGetPhysicalDeviceMemoryProperties2KHR)loadFn( instance, "vkGetPhysicalDeviceMemoryProperties2KHR" );
	pTable->GetPhysicalDeviceSparseImageFormatProperties2KHR = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR)loadFn( instance, "vkGetPhysicalDeviceSparseImageFormatProperties2KHR" );

	// ---- VK_KHR_device_group_creation extension commands
	pTable->EnumeratePhysicalDeviceGroupsKHR = (PFN_vkEnumeratePhysicalDeviceGroupsKHR)loadFn( instance, "vkEnumeratePhysicalDeviceGroupsKHR" );

	// ---- VK_KHR_external_memory_capabilities extension commands
	pTable->GetPhysicalDeviceExternalBufferPropertiesKHR = (PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR)loadFn( instance, "vkGetPhysicalDeviceExternalBufferPropertiesKHR" );

	// ---- VK_KHR_external_semaphore_capabilities extension commands
	pTable->GetPhysicalDeviceExternalSemaphorePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR)loadFn( instance, "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR" );

	// ---- VK_KHR_external_fence_capabilities extension commands
	pTable->GetPhysicalDeviceExternalFencePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR)loadFn( instance, "vkGetPhysicalDeviceExternalFencePropertiesKHR" );

	// ---- VK_KHR_performance_query extension commands
	pTable->EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR = (PFN_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR)loadFn( instance, "vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR" );
	pTable->GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR		  = (PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR)loadFn( instance, "vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR" );

	// ---- VK_KHR_get_surface_capabilities2 extension commands
	pTable->GetPhysicalDeviceSurfaceCapabilities2KHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR)loadFn( instance, "vkGetPhysicalDeviceSurfaceCapabilities2KHR" );
	pTable->GetPhysicalDeviceSurfaceFormats2KHR		 = (PFN_vkGetPhysicalDeviceSurfaceFormats2KHR)loadFn( instance, "vkGetPhysicalDeviceSurfaceFormats2KHR" );

	// ---- VK_KHR_get_display_properties2 extension commands
	pTable->GetPhysicalDeviceDisplayProperties2KHR		= (PFN_vkGetPhysicalDeviceDisplayProperties2KHR)loadFn( instance, "vkGetPhysicalDeviceDisplayProperties2KHR" );
	pTable->GetPhysicalDeviceDisplayPlaneProperties2KHR = (PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR)loadFn( instance, "vkGetPhysicalDeviceDisplayPlaneProperties2KHR" );
	pTable->GetDisplayModeProperties2KHR				= (PFN_vkGetDisplayModeProperties2KHR)loadFn( instance, "vkGetDisplayModeProperties2KHR" );
	pTable->GetDisplayPlaneCapabilities2KHR				= (PFN_vkGetDisplayPlaneCapabilities2KHR)loadFn( instance, "vkGetDisplayPlaneCapabilities2KHR" );

	// ---- VK_KHR_fragment_shading_rate extension commands
	pTable->GetPhysicalDeviceFragmentShadingRatesKHR = (PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR)loadFn( instance, "vkGetPhysicalDeviceFragmentShadingRatesKHR" );

	// ---- VK_EXT_debug_report extension commands
	pTable->CreateDebugReportCallbackEXT  = (PFN_vkCreateDebugReportCallbackEXT)loadFn( instance, "vkCreateDebugReportCallbackEXT" );
	pTable->DestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)loadFn( instance, "vkDestroyDebugReportCallbackEXT" );
	pTable->DebugReportMessageEXT		  = (PFN_vkDebugReportMessageEXT)loadFn( instance, "vkDebugReportMessageEXT" );

	// ---- VK_GGP_stream_descriptor_surface extension commands
#ifdef VK_USE_PLATFORM_GGP
	pTable->CreateStreamDescriptorSurfaceGGP = (PFN_vkCreateStreamDescriptorSurfaceGGP)loadFn( instance, "vkCreateStreamDescriptorSurfaceGGP" );
#endif // VK_USE_PLATFORM_GGP

	// ---- VK_NV_external_memory_capabilities extension commands
	pTable->GetPhysicalDeviceExternalImageFormatPropertiesNV = (PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV)loadFn( instance, "vkGetPhysicalDeviceExternalImageFormatPropertiesNV" );

	// ---- VK_NN_vi_surface extension commands
#ifdef VK_USE_PLATFORM_VI_NN
	pTable->CreateViSurfaceNN = (PFN_vkCreateViSurfaceNN)loadFn( instance, "vkCreateViSurfaceNN" );
#endif // VK_USE_PLATFORM_VI_NN

	// ---- VK_EXT_direct_mode_display extension commands
	pTable->ReleaseDisplayEXT = (PFN_vkReleaseDisplayEXT)loadFn( instance, "vkReleaseDisplayEXT" );

	// ---- VK_EXT_acquire_xlib_display extension commands
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
	pTable->AcquireXlibDisplayEXT = (PFN_vkAcquireXlibDisplayEXT)loadFn( instance, "vkAcquireXlibDisplayEXT" );
#endif // VK_USE_PLATFORM_XLIB_XRANDR_EXT
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
	pTable->GetRandROutputDisplayEXT = (PFN_vkGetRandROutputDisplayEXT)loadFn( instance, "vkGetRandROutputDisplayEXT" );
#endif // VK_USE_PLATFORM_XLIB_XRANDR_EXT

	// ---- VK_EXT_display_surface_counter extension commands
	pTable->GetPhysicalDeviceSurfaceCapabilities2EXT = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT)loadFn( instance, "vkGetPhysicalDeviceSurfaceCapabilities2EXT" );

	// ---- VK_MVK_ios_surface extension commands
#ifdef VK_USE_PLATFORM_IOS_MVK
	pTable->CreateIOSSurfaceMVK = (PFN_vkCreateIOSSurfaceMVK)loadFn( instance, "vkCreateIOSSurfaceMVK" );
#endif // VK_USE_PLATFORM_IOS_MVK

	// ---- VK_MVK_macos_surface extension commands
#ifdef VK_USE_PLATFORM_MACOS_MVK
	pTable->CreateMacOSSurfaceMVK = (PFN_vkCreateMacOSSurfaceMVK)loadFn( instance, "vkCreateMacOSSurfaceMVK" );
#endif // VK_USE_PLATFORM_MACOS_MVK

	// ---- VK_EXT_debug_utils extension commands
	pTable->CreateDebugUtilsMessengerEXT  = (PFN_vkCreateDebugUtilsMessengerEXT)loadFn( instance, "vkCreateDebugUtilsMessengerEXT" );
	pTable->DestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)loadFn( instance, "vkDestroyDebugUtilsMessengerEXT" );
	pTable->SubmitDebugUtilsMessageEXT	  = (PFN_vkSubmitDebugUtilsMessageEXT)loadFn( instance, "vkSubmitDebugUtilsMessageEXT" );

	// ---- VK_EXT_sample_locations extension commands
	pTable->GetPhysicalDeviceMultisamplePropertiesEXT = (PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT)loadFn( instance, "vkGetPhysicalDeviceMultisamplePropertiesEXT" );

	// ---- VK_EXT_calibrated_timestamps extension commands
	pTable->GetPhysicalDeviceCalibrateableTimeDomainsEXT = (PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)loadFn( instance, "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT" );

	// ---- VK_FUCHSIA_imagepipe_surface extension commands
#ifdef VK_USE_PLATFORM_FUCHSIA
	pTable->CreateImagePipeSurfaceFUCHSIA = (PFN_vkCreateImagePipeSurfaceFUCHSIA)loadFn( instance, "vkCreateImagePipeSurfaceFUCHSIA" );
#endif // VK_USE_PLATFORM_FUCHSIA

	// ---- VK_EXT_metal_surface extension commands
#ifdef VK_USE_PLATFORM_METAL_EXT
	pTable->CreateMetalSurfaceEXT = (PFN_vkCreateMetalSurfaceEXT)loadFn( instance, "vkCreateMetalSurfaceEXT" );
#endif // VK_USE_PLATFORM_METAL_EXT

	// ---- VK_EXT_tooling_info extension commands
	pTable->GetPhysicalDeviceToolPropertiesEXT = (PFN_vkGetPhysicalDeviceToolPropertiesEXT)loadFn( instance, "vkGetPhysicalDeviceToolPropertiesEXT" );

	// ---- VK_NV_cooperative_matrix extension commands
	pTable->GetPhysicalDeviceCooperativeMatrixPropertiesNV = (PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV)loadFn( instance, "vkGetPhysicalDeviceCooperativeMatrixPropertiesNV" );

	// ---- VK_NV_coverage_reduction_mode extension commands
	pTable->GetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV = (PFN_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV)loadFn( instance, "vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV" );

	// ---- VK_EXT_full_screen_exclusive extension commands
#ifdef VK_USE_PLATFORM_WIN32_KHR
	pTable->GetPhysicalDeviceSurfacePresentModes2EXT = (PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT)loadFn( instance, "vkGetPhysicalDeviceSurfacePresentModes2EXT" );
#endif // VK_USE_PLATFORM_WIN32_KHR

	// ---- VK_EXT_headless_surface extension commands
	pTable->CreateHeadlessSurfaceEXT = (PFN_vkCreateHeadlessSurfaceEXT)loadFn( instance, "vkCreateHeadlessSurfaceEXT" );

	// ---- VK_EXT_acquire_drm_display extension commands
	pTable->AcquireDrmDisplayEXT = (PFN_vkAcquireDrmDisplayEXT)loadFn( instance, "vkAcquireDrmDisplayEXT" );
	pTable->GetDrmDisplayEXT	 = (PFN_vkGetDrmDisplayEXT)loadFn( instance, "vkGetDrmDisplayEXT" );

	// ---- VK_NV_acquire_winrt_display extension commands
#ifdef VK_USE_PLATFORM_WIN32_KHR
	pTable->AcquireWinrtDisplayNV = (PFN_vkAcquireWinrtDisplayNV)loadFn( instance, "vkAcquireWinrtDisplayNV" );
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
	pTable->GetWinrtDisplayNV = (PFN_vkGetWinrtDisplayNV)loadFn( instance, "vkGetWinrtDisplayNV" );
#endif // VK_USE_PLATFORM_WIN32_KHR

	// ---- VK_EXT_directfb_surface extension commands
#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
	pTable->CreateDirectFBSurfaceEXT = (PFN_vkCreateDirectFBSurfaceEXT)loadFn( instance, "vkCreateDirectFBSurfaceEXT" );
#endif // VK_USE_PLATFORM_DIRECTFB_EXT
#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
	pTable->GetPhysicalDeviceDirectFBPresentationSupportEXT = (PFN_vkGetPhysicalDeviceDirectFBPresentationSupportEXT)loadFn( instance, "vkGetPhysicalDeviceDirectFBPresentationSupportEXT" );
#endif // VK_USE_PLATFORM_DIRECTFB_EXT

	// ---- VK_QNX_screen_surface extension commands
#ifdef VK_USE_PLATFORM_SCREEN_QNX
	pTable->CreateScreenSurfaceQNX = (PFN_vkCreateScreenSurfaceQNX)loadFn( instance, "vkCreateScreenSurfaceQNX" );
#endif // VK_USE_PLATFORM_SCREEN_QNX
#ifdef VK_USE_PLATFORM_SCREEN_QNX
	pTable->GetPhysicalDeviceScreenPresentationSupportQNX = (PFN_vkGetPhysicalDeviceScreenPresentationSupportQNX)loadFn( instance, "vkGetPhysicalDeviceScreenPresentationSupportQNX" );
#endif // VK_USE_PLATFORM_SCREEN_QNX
}

} // namespace cinder::vk
