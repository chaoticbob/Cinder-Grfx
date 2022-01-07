#pragma once

#include "cinder/vk/vk_config.h"
#include "cinder/GeomIo.h"

namespace cinder::vk {

class Device;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Objects

struct CI_API SpirvBytecode
{
	size_t		sizeInBytes;
	const void *code;

	const char *begin() const
	{
		return static_cast<const char *>( code );
	}

	const char *end() const
	{
		return begin() + sizeInBytes;
	}
};

struct CI_API InterfaceVariable
{
	InterfaceVariable() {}

	InterfaceVariable( const std::string &name, uint32_t location, VkFormat format, geom::Attrib semantic )
		: mName( name ), mLocation( location ), mFormat( format ), mSemantic( semantic ) {}

	//! Returns a const reference of the name as defined in the Vertex Shader.
	const std::string &getName() const { return mName; }
	//! Returns the Vertex Shader generated or user defined location of this attribute.
	uint32_t		   getLocation() const { return mLocation; }
	//! Returns the GLenum representation of the type of this attribute (for example, \c GL_FLOAT_VEC3)
	VkFormat		   getFormat() const { return mFormat; }
	//! Returns the defined geom::Attrib semantic.
	geom::Attrib	   getSemantic() const { return mSemantic; }

private:
	std::string	 mName;
	uint32_t	 mLocation = UINT32_MAX;
	VkFormat	 mFormat   = VK_FORMAT_UNDEFINED;
	geom::Attrib mSemantic = geom::Attrib::USER_DEFINED;
	std::string	 mHlslSemantic;
};

struct DescriptorBinding
{
	DescriptorBinding() {}

	DescriptorBinding( const std::string &name, VkDescriptorType type, uint32_t binding, uint32_t set )
		: mName( name ), mType( type ), mBinding( binding ), mSet( set ) {}

	const std::string &getName() const { return mName; }
	VkDescriptorType   getType() const { return mType; }
	uint32_t		   getBinding() const { return mBinding; }
	uint32_t		   getSet() const { return mSet; }

private:
	std::string		 mName;
	VkDescriptorType mType;
	uint32_t		 mBinding;
	uint32_t		 mSet;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions

VmaMemoryUsage toVmaMemoryUsage( vk::MemoryUsage value );

//! Returns a VkSampleCountFlagBits value based on samples
VkSampleCountFlagBits toVkSampleCount( uint32_t samples );

//! Returns VkPrimitiveTopology for geom::Primitive
VkPrimitiveTopology toVkPrimitive( geom::Primitive value );

//! Returns VkFormat for attribInfo
VkFormat toVkFormat( const geom::AttribInfo &attribInfo );

//! Returns size in bytes of format
uint32_t formatSize( VkFormat format );

//! Returns the number of components of format
uint32_t formatComponentCount( VkFormat format );

// Determines the image aspect mask for format
VkImageAspectFlags determineAspectMask( VkFormat format );

//! Guesses a pipeline stage for the layout
VkPipelineStageFlags guessPipelineStageFromImageLayout( vk::DeviceRef device, VkImageLayout layout, bool isSource = false );

//! Records a image layout transition using a pipeline barreir to specified command buffer
VkResult cmdTransitionImageLayout(
	PFN_vkCmdPipelineBarrier fnCmdPipelineBarrier,
	VkCommandBuffer			 commandBuffer,
	VkImage					 image,
	VkImageAspectFlags		 aspectMask,
	uint32_t				 baseMipLevel,
	uint32_t				 levelCount,
	uint32_t				 baseArrayLayer,
	uint32_t				 layerCount,
	VkImageLayout			 oldLayout,
	VkImageLayout			 newLayout,
	VkPipelineStageFlags	 newPipelineStageFlags );

} // namespace cinder::vk
