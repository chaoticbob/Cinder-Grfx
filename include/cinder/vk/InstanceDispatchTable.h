#pragma once

#include "cinder/vk/vk_config.h"

typedef PFN_vkVoidFunction( VKAPI_PTR *PFN_GetPhysicalDeviceProcAddr )( VkInstance instance, const char *pName );

namespace cinder::vk {

struct InstanceDispatchTable;

void LoadInstanceFunctions(
	PFN_vkGetInstanceProcAddr		   loadFn,
	VkInstance						   instance,
	cinder::vk::InstanceDispatchTable *pTable );

// Borrowed from VkLayerInstanceDispatchTable_:
//   https://github.com/KhronosGroup/Vulkan-Loader/blob/master/loader/generated/vk_layer_dispatch_table.h
//
struct InstanceDispatchTable
{
	// Manually add in GetPhysicalDeviceProcAddr entry
	PFN_GetPhysicalDeviceProcAddr GetPhysicalDeviceProcAddr;

	// ---- Core 1_0 commands
	PFN_vkCreateInstance							   CreateInstance;
	PFN_vkDestroyInstance							   DestroyInstance;
	PFN_vkEnumeratePhysicalDevices					   EnumeratePhysicalDevices;
	PFN_vkGetPhysicalDeviceFeatures					   GetPhysicalDeviceFeatures;
	PFN_vkGetPhysicalDeviceFormatProperties			   GetPhysicalDeviceFormatProperties;
	PFN_vkGetPhysicalDeviceImageFormatProperties	   GetPhysicalDeviceImageFormatProperties;
	PFN_vkGetPhysicalDeviceProperties				   GetPhysicalDeviceProperties;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties	   GetPhysicalDeviceQueueFamilyProperties;
	PFN_vkGetPhysicalDeviceMemoryProperties			   GetPhysicalDeviceMemoryProperties;
	PFN_vkGetInstanceProcAddr						   GetInstanceProcAddr;
	PFN_vkGetDeviceProcAddr							   GetDeviceProcAddr;
	PFN_vkCreateDevice								   CreateDevice;
	PFN_vkDestroyDevice								   DestroyDevice;
	PFN_vkEnumerateInstanceExtensionProperties		   EnumerateInstanceExtensionProperties;
	PFN_vkEnumerateDeviceExtensionProperties		   EnumerateDeviceExtensionProperties;
	PFN_vkEnumerateInstanceLayerProperties			   EnumerateInstanceLayerProperties;
	PFN_vkEnumerateDeviceLayerProperties			   EnumerateDeviceLayerProperties;
	PFN_vkGetPhysicalDeviceSparseImageFormatProperties GetPhysicalDeviceSparseImageFormatProperties;

	// ---- Core 1_1 commands
	PFN_vkEnumerateInstanceVersion						EnumerateInstanceVersion;
	PFN_vkEnumeratePhysicalDeviceGroups					EnumeratePhysicalDeviceGroups;
	PFN_vkGetPhysicalDeviceFeatures2					GetPhysicalDeviceFeatures2;
	PFN_vkGetPhysicalDeviceProperties2					GetPhysicalDeviceProperties2;
	PFN_vkGetPhysicalDeviceFormatProperties2			GetPhysicalDeviceFormatProperties2;
	PFN_vkGetPhysicalDeviceImageFormatProperties2		GetPhysicalDeviceImageFormatProperties2;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties2		GetPhysicalDeviceQueueFamilyProperties2;
	PFN_vkGetPhysicalDeviceMemoryProperties2			GetPhysicalDeviceMemoryProperties2;
	PFN_vkGetPhysicalDeviceSparseImageFormatProperties2 GetPhysicalDeviceSparseImageFormatProperties2;
	PFN_vkGetPhysicalDeviceExternalBufferProperties		GetPhysicalDeviceExternalBufferProperties;
	PFN_vkGetPhysicalDeviceExternalFenceProperties		GetPhysicalDeviceExternalFenceProperties;
	PFN_vkGetPhysicalDeviceExternalSemaphoreProperties	GetPhysicalDeviceExternalSemaphoreProperties;

	// ---- VK_KHR_surface extension commands
	PFN_vkDestroySurfaceKHR						  DestroySurfaceKHR;
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR	  GetPhysicalDeviceSurfaceSupportKHR;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR GetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR	  GetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR GetPhysicalDeviceSurfacePresentModesKHR;

	// ---- VK_KHR_swapchain extension commands
	PFN_vkGetPhysicalDevicePresentRectanglesKHR GetPhysicalDevicePresentRectanglesKHR;

	// ---- VK_KHR_display extension commands
	PFN_vkGetPhysicalDeviceDisplayPropertiesKHR		 GetPhysicalDeviceDisplayPropertiesKHR;
	PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR GetPhysicalDeviceDisplayPlanePropertiesKHR;
	PFN_vkGetDisplayPlaneSupportedDisplaysKHR		 GetDisplayPlaneSupportedDisplaysKHR;
	PFN_vkGetDisplayModePropertiesKHR				 GetDisplayModePropertiesKHR;
	PFN_vkCreateDisplayModeKHR						 CreateDisplayModeKHR;
	PFN_vkGetDisplayPlaneCapabilitiesKHR			 GetDisplayPlaneCapabilitiesKHR;
	PFN_vkCreateDisplayPlaneSurfaceKHR				 CreateDisplayPlaneSurfaceKHR;

	// ---- VK_KHR_xlib_surface extension commands
#ifdef VK_USE_PLATFORM_XLIB_KHR
	PFN_vkCreateXlibSurfaceKHR CreateXlibSurfaceKHR;
#endif // VK_USE_PLATFORM_XLIB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
	PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR GetPhysicalDeviceXlibPresentationSupportKHR;
#endif // VK_USE_PLATFORM_XLIB_KHR

	// ---- VK_KHR_xcb_surface extension commands
#ifdef VK_USE_PLATFORM_XCB_KHR
	PFN_vkCreateXcbSurfaceKHR CreateXcbSurfaceKHR;
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
	PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR GetPhysicalDeviceXcbPresentationSupportKHR;
#endif // VK_USE_PLATFORM_XCB_KHR

	// ---- VK_KHR_wayland_surface extension commands
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
	PFN_vkCreateWaylandSurfaceKHR CreateWaylandSurfaceKHR;
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
	PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR GetPhysicalDeviceWaylandPresentationSupportKHR;
#endif // VK_USE_PLATFORM_WAYLAND_KHR

	// ---- VK_KHR_android_surface extension commands
#ifdef VK_USE_PLATFORM_ANDROID_KHR
	PFN_vkCreateAndroidSurfaceKHR CreateAndroidSurfaceKHR;
#endif // VK_USE_PLATFORM_ANDROID_KHR

	// ---- VK_KHR_win32_surface extension commands
#ifdef VK_USE_PLATFORM_WIN32_KHR
	PFN_vkCreateWin32SurfaceKHR CreateWin32SurfaceKHR;
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
	PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR GetPhysicalDeviceWin32PresentationSupportKHR;
#endif // VK_USE_PLATFORM_WIN32_KHR

	// ---- VK_KHR_video_queue extension commands
#ifdef VK_ENABLE_BETA_EXTENSIONS
	PFN_vkGetPhysicalDeviceVideoCapabilitiesKHR GetPhysicalDeviceVideoCapabilitiesKHR;
#endif // VK_ENABLE_BETA_EXTENSIONS
#ifdef VK_ENABLE_BETA_EXTENSIONS
	PFN_vkGetPhysicalDeviceVideoFormatPropertiesKHR GetPhysicalDeviceVideoFormatPropertiesKHR;
#endif // VK_ENABLE_BETA_EXTENSIONS

	// ---- VK_KHR_get_physical_device_properties2 extension commands
	PFN_vkGetPhysicalDeviceFeatures2KHR					   GetPhysicalDeviceFeatures2KHR;
	PFN_vkGetPhysicalDeviceProperties2KHR				   GetPhysicalDeviceProperties2KHR;
	PFN_vkGetPhysicalDeviceFormatProperties2KHR			   GetPhysicalDeviceFormatProperties2KHR;
	PFN_vkGetPhysicalDeviceImageFormatProperties2KHR	   GetPhysicalDeviceImageFormatProperties2KHR;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR	   GetPhysicalDeviceQueueFamilyProperties2KHR;
	PFN_vkGetPhysicalDeviceMemoryProperties2KHR			   GetPhysicalDeviceMemoryProperties2KHR;
	PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR GetPhysicalDeviceSparseImageFormatProperties2KHR;

	// ---- VK_KHR_device_group_creation extension commands
	PFN_vkEnumeratePhysicalDeviceGroupsKHR EnumeratePhysicalDeviceGroupsKHR;

	// ---- VK_KHR_external_memory_capabilities extension commands
	PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR GetPhysicalDeviceExternalBufferPropertiesKHR;

	// ---- VK_KHR_external_semaphore_capabilities extension commands
	PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR GetPhysicalDeviceExternalSemaphorePropertiesKHR;

	// ---- VK_KHR_external_fence_capabilities extension commands
	PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR GetPhysicalDeviceExternalFencePropertiesKHR;

	// ---- VK_KHR_performance_query extension commands
	PFN_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR;
	PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR			GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR;

	// ---- VK_KHR_get_surface_capabilities2 extension commands
	PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR GetPhysicalDeviceSurfaceCapabilities2KHR;
	PFN_vkGetPhysicalDeviceSurfaceFormats2KHR	   GetPhysicalDeviceSurfaceFormats2KHR;

	// ---- VK_KHR_get_display_properties2 extension commands
	PFN_vkGetPhysicalDeviceDisplayProperties2KHR	  GetPhysicalDeviceDisplayProperties2KHR;
	PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR GetPhysicalDeviceDisplayPlaneProperties2KHR;
	PFN_vkGetDisplayModeProperties2KHR				  GetDisplayModeProperties2KHR;
	PFN_vkGetDisplayPlaneCapabilities2KHR			  GetDisplayPlaneCapabilities2KHR;

	// ---- VK_KHR_fragment_shading_rate extension commands
	PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR GetPhysicalDeviceFragmentShadingRatesKHR;

	// ---- VK_EXT_debug_report extension commands
	PFN_vkCreateDebugReportCallbackEXT	CreateDebugReportCallbackEXT;
	PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallbackEXT;
	PFN_vkDebugReportMessageEXT			DebugReportMessageEXT;

	// ---- VK_GGP_stream_descriptor_surface extension commands
#ifdef VK_USE_PLATFORM_GGP
	PFN_vkCreateStreamDescriptorSurfaceGGP CreateStreamDescriptorSurfaceGGP;
#endif // VK_USE_PLATFORM_GGP

	// ---- VK_NV_external_memory_capabilities extension commands
	PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV GetPhysicalDeviceExternalImageFormatPropertiesNV;

	// ---- VK_NN_vi_surface extension commands
#ifdef VK_USE_PLATFORM_VI_NN
	PFN_vkCreateViSurfaceNN CreateViSurfaceNN;
#endif // VK_USE_PLATFORM_VI_NN

	// ---- VK_EXT_direct_mode_display extension commands
	PFN_vkReleaseDisplayEXT ReleaseDisplayEXT;

	// ---- VK_EXT_acquire_xlib_display extension commands
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
	PFN_vkAcquireXlibDisplayEXT AcquireXlibDisplayEXT;
#endif // VK_USE_PLATFORM_XLIB_XRANDR_EXT
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
	PFN_vkGetRandROutputDisplayEXT GetRandROutputDisplayEXT;
#endif // VK_USE_PLATFORM_XLIB_XRANDR_EXT

	// ---- VK_EXT_display_surface_counter extension commands
	PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT GetPhysicalDeviceSurfaceCapabilities2EXT;

	// ---- VK_MVK_ios_surface extension commands
#ifdef VK_USE_PLATFORM_IOS_MVK
	PFN_vkCreateIOSSurfaceMVK CreateIOSSurfaceMVK;
#endif // VK_USE_PLATFORM_IOS_MVK

	// ---- VK_MVK_macos_surface extension commands
#ifdef VK_USE_PLATFORM_MACOS_MVK
	PFN_vkCreateMacOSSurfaceMVK CreateMacOSSurfaceMVK;
#endif // VK_USE_PLATFORM_MACOS_MVK

	// ---- VK_EXT_debug_utils extension commands
	PFN_vkCreateDebugUtilsMessengerEXT	CreateDebugUtilsMessengerEXT;
	PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT;
	PFN_vkSubmitDebugUtilsMessageEXT	SubmitDebugUtilsMessageEXT;

	// ---- VK_EXT_sample_locations extension commands
	PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT GetPhysicalDeviceMultisamplePropertiesEXT;

	// ---- VK_EXT_calibrated_timestamps extension commands
	PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT GetPhysicalDeviceCalibrateableTimeDomainsEXT;

	// ---- VK_FUCHSIA_imagepipe_surface extension commands
#ifdef VK_USE_PLATFORM_FUCHSIA
	PFN_vkCreateImagePipeSurfaceFUCHSIA CreateImagePipeSurfaceFUCHSIA;
#endif // VK_USE_PLATFORM_FUCHSIA

	// ---- VK_EXT_metal_surface extension commands
#ifdef VK_USE_PLATFORM_METAL_EXT
	PFN_vkCreateMetalSurfaceEXT CreateMetalSurfaceEXT;
#endif // VK_USE_PLATFORM_METAL_EXT

	// ---- VK_EXT_tooling_info extension commands
	PFN_vkGetPhysicalDeviceToolPropertiesEXT GetPhysicalDeviceToolPropertiesEXT;

	// ---- VK_NV_cooperative_matrix extension commands
	PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV GetPhysicalDeviceCooperativeMatrixPropertiesNV;

	// ---- VK_NV_coverage_reduction_mode extension commands
	PFN_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV GetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV;

	// ---- VK_EXT_full_screen_exclusive extension commands
#ifdef VK_USE_PLATFORM_WIN32_KHR
	PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT GetPhysicalDeviceSurfacePresentModes2EXT;
#endif // VK_USE_PLATFORM_WIN32_KHR

	// ---- VK_EXT_headless_surface extension commands
	PFN_vkCreateHeadlessSurfaceEXT CreateHeadlessSurfaceEXT;

	// ---- VK_EXT_acquire_drm_display extension commands
	PFN_vkAcquireDrmDisplayEXT AcquireDrmDisplayEXT;
	PFN_vkGetDrmDisplayEXT	   GetDrmDisplayEXT;

	// ---- VK_NV_acquire_winrt_display extension commands
#ifdef VK_USE_PLATFORM_WIN32_KHR
	PFN_vkAcquireWinrtDisplayNV AcquireWinrtDisplayNV;
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
	PFN_vkGetWinrtDisplayNV GetWinrtDisplayNV;
#endif // VK_USE_PLATFORM_WIN32_KHR

	// ---- VK_EXT_directfb_surface extension commands
#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
	PFN_vkCreateDirectFBSurfaceEXT CreateDirectFBSurfaceEXT;
#endif // VK_USE_PLATFORM_DIRECTFB_EXT
#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
	PFN_vkGetPhysicalDeviceDirectFBPresentationSupportEXT GetPhysicalDeviceDirectFBPresentationSupportEXT;
#endif // VK_USE_PLATFORM_DIRECTFB_EXT

	// ---- VK_QNX_screen_surface extension commands
#ifdef VK_USE_PLATFORM_SCREEN_QNX
	PFN_vkCreateScreenSurfaceQNX CreateScreenSurfaceQNX;
#endif // VK_USE_PLATFORM_SCREEN_QNX
#ifdef VK_USE_PLATFORM_SCREEN_QNX
	PFN_vkGetPhysicalDeviceScreenPresentationSupportQNX GetPhysicalDeviceScreenPresentationSupportQNX;
#endif // VK_USE_PLATFORM_SCREEN_QNX
};

} // namespace cinder::vk
