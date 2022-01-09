#pragma once

#include "cinder/grfx/grfx_config.h"

// clang-format off
#if !( defined( VK_USE_PLATFORM_WIN32_KHR ) || defined( VK_USE_PLATFORM_XCB_KHR ) || defined( VK_USE_PLATFORM_GGP ) )
	#if defined( CINDER_GGP )
		#define VK_USE_PLATFORM_GGP
	#elif defined( CINDER_LINUX )
		#define VK_USE_PLATFORM_XCB_KHR
	#elif defined( CINDER_MSW )
		#define VK_USE_PLATFORM_WIN32_KHR
	#else
		#error "unsupported Vulkan platform"
	#endif
#endif
// clang-format on

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vk_mem_alloc.h"

#include <memory>
#include <vector>

#define VK_KHR_KHRONOS_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

#if !defined( VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME )
#define VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME "VK_KHR_dynamic_rendering"
#endif

#define CINDER_VK_QUEUE_MASK_ALL	  ( VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT )
#define CINDER_VK_QUEUE_MASK_GRAPHICS ( VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT )
#define CINDER_VK_QUEUE_MASK_COMPUTE  ( VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT )
#define CINDER_VK_QUEUE_MASK_TRANSFER ( VK_QUEUE_TRANSFER_BIT )
#define CINDER_VK_ALL_SUB_RESOURCES	  0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS

#define CINDER_VK_DATA_TYPE_SIGNED_SHIFT	31
#define CINDER_VK_DATA_TYPE_COMPOSITE_SHIFT 20
#define CINDER_VK_DATA_TYPE_ROWS_SHIFT		16
#define CINDER_VK_DATA_TYPE_COLUMNS_SHIFT	12
#define CINDER_VK_DATA_TYPE_SCALAR_SHIFT	8
#define CINDER_VK_DATA_TYPE_WIDTH_SHIFT		0

#define CINDER_VK_MAKE_DATA_TYPE( SIGNED, COMPOSITE, ROWS, COLS, SCALAR, WIDTH )      \
	( ( static_cast<uint32_t>( SIGNED ) << CINDER_VK_DATA_TYPE_SIGNED_SHIFT ) |       \
	  ( static_cast<uint32_t>( COMPOSITE ) << CINDER_VK_DATA_TYPE_COMPOSITE_SHIFT ) | \
	  ( static_cast<uint32_t>( ROWS ) << CINDER_VK_DATA_TYPE_ROWS_SHIFT ) |           \
	  ( static_cast<uint32_t>( COLS ) << CINDER_VK_DATA_TYPE_COLUMNS_SHIFT ) |        \
	  ( static_cast<uint32_t>( SCALAR ) << CINDER_VK_DATA_TYPE_SCALAR_SHIFT ) |       \
	  ( static_cast<uint32_t>( WIDTH ) << CINDER_VK_DATA_TYPE_WIDTH_SHIFT ) ) *       \
		( ( static_cast<uint32_t>( COMPOSITE ) > 0 ) *                                \
		  ( static_cast<uint32_t>( ROWS ) > 0 ) *                                     \
		  ( static_cast<uint32_t>( COLS ) > 0 ) *                                     \
		  ( static_cast<uint32_t>( SCALAR ) > 0 ) *                                   \
		  ( static_cast<uint32_t>( WIDTH ) > 0 ) )

namespace cinder::app {

class RendererVk;

} // namespace cinder::app

namespace cinder::vk {

enum class ScalarType : uint32_t
{
	UNKNOWN = 0,
	BOOL	= 1,
	INT		= 2,
	FLOAT	= 3,
};

enum class CompositeType : uint32_t
{
	UNKNOWN = 0,
	SCALAR	= 1,
	VECTOR	= 2,
	MATRIX	= 3,
};

enum class DataType : uint32_t
{
	UNKNOWN = 0,

	BOOL1 = CINDER_VK_MAKE_DATA_TYPE( 0, CompositeType::SCALAR, 1, 1, ScalarType::BOOL, 32 ),
	BOOL2 = CINDER_VK_MAKE_DATA_TYPE( 0, CompositeType::VECTOR, 2, 1, ScalarType::BOOL, 32 ),
	BOOL3 = CINDER_VK_MAKE_DATA_TYPE( 0, CompositeType::VECTOR, 3, 1, ScalarType::BOOL, 32 ),
	BOOL4 = CINDER_VK_MAKE_DATA_TYPE( 0, CompositeType::VECTOR, 4, 1, ScalarType::BOOL, 32 ),
	BOOL  = BOOL1,

	INT1 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::SCALAR, 1, 1, ScalarType::INT, 32 ),
	INT2 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::VECTOR, 2, 1, ScalarType::INT, 32 ),
	INT3 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::VECTOR, 3, 1, ScalarType::INT, 32 ),
	INT4 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::VECTOR, 4, 1, ScalarType::INT, 32 ),
	INT	 = INT1,

	UINT1 = CINDER_VK_MAKE_DATA_TYPE( 0, CompositeType::SCALAR, 1, 1, ScalarType::INT, 32 ),
	UINT2 = CINDER_VK_MAKE_DATA_TYPE( 0, CompositeType::VECTOR, 2, 1, ScalarType::INT, 32 ),
	UINT3 = CINDER_VK_MAKE_DATA_TYPE( 0, CompositeType::VECTOR, 3, 1, ScalarType::INT, 32 ),
	UINT4 = CINDER_VK_MAKE_DATA_TYPE( 0, CompositeType::VECTOR, 4, 1, ScalarType::INT, 32 ),
	UINT  = UINT1,

	FLOAT1 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::SCALAR, 1, 1, ScalarType::FLOAT, 32 ),
	FLOAT2 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::VECTOR, 2, 1, ScalarType::FLOAT, 32 ),
	FLOAT3 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::VECTOR, 3, 1, ScalarType::FLOAT, 32 ),
	FLOAT4 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::VECTOR, 4, 1, ScalarType::FLOAT, 32 ),
	FLOAT  = FLOAT1,

	FLOAT2x2 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::MATRIX, 2, 2, ScalarType::FLOAT, 32 ),
	FLOAT2x3 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::MATRIX, 2, 3, ScalarType::FLOAT, 32 ),
	FLOAT2x4 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::MATRIX, 2, 4, ScalarType::FLOAT, 32 ),

	FLOAT3x2 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::MATRIX, 3, 2, ScalarType::FLOAT, 32 ),
	FLOAT3x3 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::MATRIX, 3, 3, ScalarType::FLOAT, 32 ),
	FLOAT3x4 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::MATRIX, 3, 4, ScalarType::FLOAT, 32 ),

	FLOAT4x2 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::MATRIX, 4, 2, ScalarType::FLOAT, 32 ),
	FLOAT4x3 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::MATRIX, 4, 3, ScalarType::FLOAT, 32 ),
	FLOAT4x4 = CINDER_VK_MAKE_DATA_TYPE( 1, CompositeType::MATRIX, 4, 4, ScalarType::FLOAT, 32 ),
};

enum class MemoryUsage
{
	UNKNOWN = 0,
	CPU_ONLY,
	CPU_TO_GPU,
	GPU_ONLY,
	GPU_TO_CPU,
};

class Batch;
class Buffer;
class BufferedMesh;
class BufferedRenderPass;
class CommandBuffer;
class CommandPool;
class Context;
class CountingSemaphore;
class DescriptorPool;
class DescriptorSet;
class DescriptorSetLayout;
class Device;
class Fence;
class Framebuffer;
class GlslProg;
class HlslProg;
class Image;
class ImageView;
class MutableBuffer;
class Pipeline;
class PipelineLayout;
class RenderPass;
class Sampler;
class Semaphore;
class ShaderModule;
class ShaderProg;
class Swapchain;
class TextureBase;
class Texture1d;
class Texture2d;
class Texture3d;
class TextureCubeMap;
class UniformBuffer;

using BatchRef				 = std::shared_ptr<Batch>;
using BufferRef				 = std::shared_ptr<Buffer>;
using BufferedMeshRef		 = std::shared_ptr<BufferedMesh>;
using BufferedRenderPassRef	 = std::shared_ptr<BufferedRenderPass>;
using CommandBufferRef		 = std::shared_ptr<CommandBuffer>;
using CommandPoolRef		 = std::shared_ptr<CommandPool>;
using ContextRef			 = std::shared_ptr<Context>;
using CountingSemaphoreRef	 = std::shared_ptr<CountingSemaphore>;
using DescriptorPoolRef		 = std::shared_ptr<DescriptorPool>;
using DescriptorSetRef		 = std::shared_ptr<DescriptorSet>;
using DescriptorSetLayoutRef = std::shared_ptr<DescriptorSetLayout>;
using DeviceRef				 = std::shared_ptr<Device>;
using FenceRef				 = std::shared_ptr<Fence>;
using FramebufferRef		 = std::shared_ptr<Framebuffer>;
using GlslProgRef			 = std::shared_ptr<GlslProg>;
using HlslProgRef			 = std::shared_ptr<HlslProg>;
using ImageRef				 = std::shared_ptr<Image>;
using ImageViewRef			 = std::shared_ptr<ImageView>;
using MutableBufferRef		 = std::shared_ptr<MutableBuffer>;
using PipelineRef			 = std::shared_ptr<Pipeline>;
using PipelineLayoutRef		 = std::shared_ptr<PipelineLayout>;
using RenderPassRef			 = std::shared_ptr<RenderPass>;
using SamplerRef			 = std::shared_ptr<Sampler>;
using SemaphoreRef			 = std::shared_ptr<Semaphore>;
using ShaderModuleRef		 = std::shared_ptr<ShaderModule>;
using ShaderProgRef			 = std::shared_ptr<ShaderProg>;
using SwapchainRef			 = std::shared_ptr<Swapchain>;
using TextureBaseRef		 = std::shared_ptr<TextureBase>;
using Texture1dRef			 = std::shared_ptr<Texture1d>;
using Texture2dRef			 = std::shared_ptr<Texture2d>;
using Texture3dRef			 = std::shared_ptr<Texture3d>;
using TextureCubeMapRef		 = std::shared_ptr<TextureCubeMap>;
using UniformBufferRef		 = std::shared_ptr<UniformBuffer>;

class CI_API VulkanExc : public cinder::Exception
{
public:
	VulkanExc( const std::string &description )
		: Exception( description ) {}
};

class CI_API VulkanExtensionNotFoundExc : public cinder::Exception
{
public:
	VulkanExtensionNotFoundExc( const std::string &extension )
		: Exception( extension ) {}
};

class CI_API VulkanFeatureNotFoundExc : public cinder::Exception
{
public:
	VulkanFeatureNotFoundExc( const std::string &description )
		: Exception( description ) {}
};

class CI_API VulkanFnFailedExc : public cinder::Exception
{
public:
	VulkanFnFailedExc( const std::string &fnName, VkResult vkres )
		: Exception( fnName + std::string( ": " ) + std::to_string( static_cast<int>( vkres ) ) ) {}
};

struct QueueFamilyIndices
{
	uint32_t graphics = VK_QUEUE_FAMILY_IGNORED;
	uint32_t compute  = VK_QUEUE_FAMILY_IGNORED;
	uint32_t transfer = VK_QUEUE_FAMILY_IGNORED;
};

template <typename T>
uint32_t countU32( const std::vector<T> &container )
{
	return static_cast<uint32_t>( container.size() );
}

template <typename T>
T *dataPtr( std::vector<T> &container )
{
	return container.empty() ? nullptr : container.data();
}

template <typename T>
const T *dataPtr( const std::vector<T> &container )
{
	return container.empty() ? nullptr : container.data();
}

inline bool hasExtension(
	const std::string						&name,
	const std::vector<VkExtensionProperties> &foundExtensions )
{
	auto it = std::find_if(
		foundExtensions.begin(), foundExtensions.end(), [name]( const VkExtensionProperties &elem ) -> bool {
			return ( name == elem.extensionName );
		} );
	bool res = ( it != foundExtensions.end() );
	return res;
}

template <typename T, typename U>
void changeFlagBit( T &flags, const U bitmask, bool state )
{
	static_assert( std::is_unsigned<T>::value, "T must be an unsigned integer type" );
	static_assert( std::is_integral<T>::value, "U must be an integer type" );

	// There's definitely a more clever way to do this.
	if ( state ) {
		flags |= static_cast<T>( bitmask );
	}
	else {
		flags &= ~static_cast<T>( bitmask );
	}
}

} // namespace cinder::vk
