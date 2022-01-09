#include "cinder/vk/ShaderProg.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Environment.h"
#include "cinder/vk/UniformBuffer.h"
#include "cinder/app/AppBase.h"
#include "cinder/app/RendererVk.h"
#include "cinder/Log.h"
#include "cinder/Unicode.h"

#include "glslang/Include/glslang_c_interface.h"
#include "StandAlone/resource_limits_c.h"

#include "dxc/dxcapi.h"

#if defined( CINDER_MSW )
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

#include <regex>

namespace cinder::vk {

const uint32_t SPIRV_STARTING_WORD_INDEX = 5;
const uint32_t SPIRV_WORD_SIZE			 = sizeof( uint32_t );
const uint32_t SPIRV_MINIMUM_FILE_SIZE	 = SPIRV_STARTING_WORD_INDEX * SPIRV_WORD_SIZE;

static const std::string CI_VK_DEFAULT_UNIFORM_BLOCK_NAME = "gl_DefaultUniformBlock";
static const std::string CI_VK_HLSL_GLOBALS_NAME		  = "$Globals";

static std::map<std::string, UniformSemantic> sDefaultUniformNameToSemanticMap = {
	{ "ciModelMatrix", UNIFORM_MODEL_MATRIX },
	{ "ciModelMatrixInverse", UNIFORM_MODEL_MATRIX_INVERSE },
	{ "ciModelMatrixInverseTranspose", UNIFORM_MODEL_MATRIX_INVERSE_TRANSPOSE },
	{ "ciViewMatrix", UNIFORM_VIEW_MATRIX },
	{ "ciViewMatrixInverse", UNIFORM_VIEW_MATRIX_INVERSE },
	{ "ciModelView", UNIFORM_MODEL_VIEW },
	{ "ciModelViewInverse", UNIFORM_MODEL_VIEW_INVERSE },
	{ "ciModelViewInverseTranspose", UNIFORM_MODEL_VIEW_INVERSE_TRANSPOSE },
	{ "ciModelViewProjection", UNIFORM_MODEL_VIEW_PROJECTION },
	{ "ciModelViewProjectionInverse", UNIFORM_MODEL_VIEW_PROJECTION_INVERSE },
	{ "ciProjectionMatrix", UNIFORM_PROJECTION_MATRIX },
	{ "ciProjectionMatrixInverse", UNIFORM_PROJECTION_MATRIX_INVERSE },
	{ "ciViewProjection", UNIFORM_VIEW_PROJECTION },
	{ "ciNormalMatrix", UNIFORM_NORMAL_MATRIX },
	{ "ciViewportMatrix", UNIFORM_VIEWPORT_MATRIX },
	{ "ciWindowSize", UNIFORM_WINDOW_SIZE },
	{ "ciElapsedSeconds", UNIFORM_ELAPSED_SECONDS },
};

static std::map<std::string, geom::Attrib> sDefaultAttribNameToSemanticMap = {
	// GLSL
	{ "ciPosition", geom::Attrib::POSITION },
	{ "ciNormal", geom::Attrib::NORMAL },
	{ "ciTangent", geom::Attrib::TANGENT },
	{ "ciBitangent", geom::Attrib::BITANGENT },
	{ "ciTexCoord0", geom::Attrib::TEX_COORD_0 },
	{ "ciTexCoord1", geom::Attrib::TEX_COORD_1 },
	{ "ciTexCoord2", geom::Attrib::TEX_COORD_2 },
	{ "ciTexCoord3", geom::Attrib::TEX_COORD_3 },
	{ "ciColor", geom::Attrib::COLOR },
	{ "ciBoneIndex", geom::Attrib::BONE_INDEX },
	{ "ciBoneWeight", geom::Attrib::BONE_WEIGHT },
	// HLSL
	{ "in.var.POSTIION", geom::Attrib::POSITION },
	{ "in.var.NORMAL", geom::Attrib::NORMAL },
	{ "in.var.TANGENT", geom::Attrib::TANGENT },
	{ "in.var.BITANGENT", geom::Attrib::BITANGENT },
	{ "in.var.BINORMAL", geom::Attrib::BITANGENT },
	{ "in.var.TEXCOORD", geom::Attrib::TEX_COORD_0 },
	{ "in.var.TEXCOORD0", geom::Attrib::TEX_COORD_0 },
	{ "in.var.TEXCOORD01", geom::Attrib::TEX_COORD_1 },
	{ "in.var.TEXCOORD02", geom::Attrib::TEX_COORD_2 },
	{ "in.var.TEXCOORD03", geom::Attrib::TEX_COORD_3 },
	{ "in.var.COLOR", geom::Attrib::COLOR },
	{ "in.var.BONEINDEX", geom::Attrib::BONE_INDEX },
	{ "in.var.BONEWEIGHT", geom::Attrib::BONE_WEIGHT },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions

static void loadShaderText( const DataSourceRef &dataSource, std::string &sourceTarget )
{
	if ( !dataSource ) {
		return;
	}

	ci::BufferRef buffer;
	if ( dataSource->isFilePath() ) {
		buffer = loadFile( dataSource->getFilePath() )->getBuffer();
	}
	else if ( dataSource->isUrl() ) {
		buffer = loadUrl( dataSource->getUrl() )->getBuffer();
	}
	else {
		buffer = dataSource->getBuffer();
	}

	const char *start = static_cast<const char *>( buffer->getData() );
	const char *end	  = start + buffer->getSize();
	sourceTarget	  = std::string( start, end );
}

static void loadShaderSpirv( const DataSourceRef &dataSource, std::vector<char> &spirvTarget )
{
	if ( !dataSource ) {
		return;
	}

	ci::BufferRef buffer;
	if ( dataSource->isFilePath() ) {
		buffer = loadFile( dataSource->getFilePath() )->getBuffer();
	}
	else if ( dataSource->isUrl() ) {
		buffer = loadUrl( dataSource->getUrl() )->getBuffer();
	}
	else {
		buffer = dataSource->getBuffer();
	}

	const char *start = static_cast<const char *>( buffer->getData() );
	const char *end	  = start + buffer->getSize();
	spirvTarget		  = std::vector<char>( start, end );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ShaderModule

vk::ShaderModuleRef ShaderModule::create( DataSourceRef dataSource, vk::DeviceRef device )
{
	if ( dataSource->isFilePath() || dataSource->isUrl() ) {
		throw VulkanExc( "File path and URL data sources not supported" );
	}

	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	auto buffer = dataSource->getBuffer();
	return ShaderModuleRef( new ShaderModule( device, buffer->getSize(), static_cast<const char *>( buffer->getData() ) ) );
}

vk::ShaderModuleRef ShaderModule::create( size_t sizeInBytes, const char *pSpirvCode, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return ShaderModuleRef( new ShaderModule( device, sizeInBytes, pSpirvCode ) );
}

vk::ShaderModuleRef ShaderModule::create( const std::vector<char> &spirv, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return ShaderModuleRef( new ShaderModule( device, countU32( spirv ), dataPtr( spirv ) ) );
}

ShaderModule::ShaderModule( vk::DeviceRef device, size_t spirvSize, const char *pSpirvCode )
	: vk::DeviceChildObject( device )
{
	if ( ( spirvSize < SPIRV_MINIMUM_FILE_SIZE ) || ( pSpirvCode == nullptr ) ) {
		throw VulkanExc( "invalid SPIR-V data" );
	}

	const uint32_t sig = *( reinterpret_cast<const uint32_t *>( pSpirvCode ) );
	if ( sig != SpvMagicNumber ) {
		throw VulkanExc( "invalid SPIR-V magic number" );
	}

	// Reflection
	{
		auto reflection = spv_reflect::ShaderModule( spirvSize, pSpirvCode );
		if ( reflection.GetResult() != SPV_REFLECT_RESULT_SUCCESS ) {
			throw VulkanExc( "SPIR-V reflection failed" );
		}

		// Shader stage
		// clang-format off
		switch (reflection.GetShaderStage()) {
			default: {
				throw VulkanExc( "SPIR-V reflection: unrecognized shader stage" );
			} break;

			case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT                  :	mShaderStage = VK_SHADER_STAGE_VERTEX_BIT                 ; break;
			case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT    :	mShaderStage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT   ; break;
			case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT :	mShaderStage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT; break;
			case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT                :	mShaderStage = VK_SHADER_STAGE_GEOMETRY_BIT               ; break;
			case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT                :	mShaderStage = VK_SHADER_STAGE_FRAGMENT_BIT               ; break;
			case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT                 :	mShaderStage = VK_SHADER_STAGE_COMPUTE_BIT                ; break;
			case SPV_REFLECT_SHADER_STAGE_TASK_BIT_NV                 :	mShaderStage = VK_SHADER_STAGE_TASK_BIT_NV                ; break;
			case SPV_REFLECT_SHADER_STAGE_MESH_BIT_NV                 :	mShaderStage = VK_SHADER_STAGE_MESH_BIT_NV                ; break;
			case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR              :	mShaderStage = VK_SHADER_STAGE_RAYGEN_BIT_KHR             ; break;
			case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR             :	mShaderStage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR            ; break;
			case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR         :	mShaderStage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR        ; break;
			case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR                :	mShaderStage = VK_SHADER_STAGE_MISS_BIT_KHR               ; break;
			case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR        :	mShaderStage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR       ; break;
			case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR            :	mShaderStage = VK_SHADER_STAGE_CALLABLE_BIT_KHR           ; break;
		}
		// clang-format on

		// Entry point
		mEntryPoint = reflection.GetEntryPointName();

		// Source langauge
		mSourceLanguage = reflection.GetShaderModule().source_language;

		// SPIR-v bindings
		std::vector<SpvReflectDescriptorBinding *> spirvBindings;
		{
			uint32_t		 count	= 0;
			SpvReflectResult spvres = reflection.EnumerateDescriptorBindings( &count, nullptr );
			if ( spvres != SPV_REFLECT_RESULT_SUCCESS ) {
				throw VulkanExc( "SPIR-V reflection: enumerate descriptor binding count failed" );
			}

			spirvBindings.resize( count );
			spvres = reflection.EnumerateDescriptorBindings( &count, dataPtr( spirvBindings ) );
			if ( spvres != SPV_REFLECT_RESULT_SUCCESS ) {
				throw VulkanExc( "SPIR-V reflection: enumerate descriptor binding failed" );
			}
		}

		parseInterfaceVariables( reflection );
		parseDescriptorBindings( spirvBindings );
		parseUniformBlocks( spirvBindings );
	}

	// Creat shader module
	VkShaderModuleCreateInfo vkci = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	vkci.pNext					  = nullptr;
	vkci.flags					  = 0;
	vkci.codeSize				  = spirvSize;
	vkci.pCode					  = reinterpret_cast<const uint32_t *>( pSpirvCode );

	VkResult vkres = CI_VK_DEVICE_FN( CreateShaderModule(
		getDeviceHandle(),
		&vkci,
		nullptr,
		&mShaderModuleHandle ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkCreateShaderModule", vkres );
	}
}

ShaderModule::~ShaderModule()
{
	if ( mShaderModuleHandle != VK_NULL_HANDLE ) {
		CI_VK_DEVICE_FN( DestroyShaderModule(
			getDeviceHandle(),
			mShaderModuleHandle,
			nullptr ) );
		mShaderModuleHandle = VK_NULL_HANDLE;
	}
}

void ShaderModule::parseInterfaceVariables( const spv_reflect::ShaderModule &reflection )
{
	// Input variables
	{
		uint32_t		 count	= 0;
		SpvReflectResult spvres = reflection.EnumerateInputVariables( &count, nullptr );
		if ( spvres != SPV_REFLECT_RESULT_SUCCESS ) {
			throw VulkanExc( "SPIR-V reflection: enumerate input variables count failed" );
		}

		std::vector<SpvReflectInterfaceVariable *> spirvVars( count );
		spvres = reflection.EnumerateInputVariables( &count, dataPtr( spirvVars ) );
		if ( spvres != SPV_REFLECT_RESULT_SUCCESS ) {
			throw VulkanExc( "SPIR-V reflection: enumerate input variables failed" );
		}

		for ( size_t i = 0; i < spirvVars.size(); ++i ) {
			const SpvReflectInterfaceVariable *pSpvVar = spirvVars[i];

			// Skip invalid locations - there are usually system input variables
			// that's inaccessible to the shader anyway.
			if ( pSpvVar->location == UINT32_MAX ) {
				continue;
			}

			std::string	 name	  = pSpvVar->name;
			uint32_t	 location = pSpvVar->location;
			VkFormat	 format	  = static_cast<VkFormat>( pSpvVar->format );
			geom::Attrib semantic = geom::Attrib::USER_DEFINED;

			auto it = sDefaultAttribNameToSemanticMap.find( name );
			if ( it != sDefaultAttribNameToSemanticMap.end() ) {
				semantic = it->second;
			}

			mInputVariables.emplace_back( name, location, format, semantic );
		}
	}

	// Output variables
	{
		uint32_t		 count	= 0;
		SpvReflectResult spvres = reflection.EnumerateOutputVariables( &count, nullptr );
		if ( spvres != SPV_REFLECT_RESULT_SUCCESS ) {
			throw VulkanExc( "SPIR-V reflection: enumerate input variables count failed" );
		}

		std::vector<SpvReflectInterfaceVariable *> spirvVars( count );
		spvres = reflection.EnumerateOutputVariables( &count, dataPtr( spirvVars ) );
		if ( spvres != SPV_REFLECT_RESULT_SUCCESS ) {
			throw VulkanExc( "SPIR-V reflection: enumerate input variables failed" );
		}

		for ( size_t i = 0; i < spirvVars.size(); ++i ) {
			const SpvReflectInterfaceVariable *pSpvVar = spirvVars[i];

			// Skip invalid locations - there are usually system input variables
			// that's inaccessible to the shader anyway.
			if ( pSpvVar->location == UINT32_MAX ) {
				continue;
			}

			std::string	 name	  = pSpvVar->name;
			uint32_t	 location = pSpvVar->location;
			VkFormat	 format	  = static_cast<VkFormat>( pSpvVar->format );
			geom::Attrib semantic = geom::Attrib::USER_DEFINED;

			auto it = sDefaultAttribNameToSemanticMap.find( name );
			if ( it != sDefaultAttribNameToSemanticMap.end() ) {
				semantic = it->second;
			}

			mOutputVariables.emplace_back( name, location, format, semantic );
		}
	}
}

void ShaderModule::parseDescriptorBindings( const std::vector<SpvReflectDescriptorBinding *> &spirvBindings )
{
	for ( size_t i = 0; i < spirvBindings.size(); ++i ) {
		const SpvReflectDescriptorBinding *pSpvBinding = spirvBindings[i];

		VkDescriptorType descriptorType = static_cast<VkDescriptorType>( ~0 );

		// clang-format off
		switch ( pSpvBinding->descriptor_type ) {
			default: {
				throw VulkanExc( "SPIR-V reflection: unrecognized descriptor type" );
			} break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER                    : descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER                   ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER     : descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER    ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE              : descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE             ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE              : descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE             ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER       : descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER      ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER       : descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER      ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER             : descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER            ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER             : descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER            ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC     : descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC    ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC     : descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC    ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT           : descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT          ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR : descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR; break;
		}
		// clang-format on

		// Binding name
		std::string name = pSpvBinding->name;

		// Set binding name to block name if name is empty and is default uniform  block
		if ( ( descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ) && name.empty() ) {
			std::string blockName = spvReflectBlockVariableTypeName( &pSpvBinding->block );
			if ( blockName == CI_VK_DEFAULT_UNIFORM_BLOCK_NAME ) {
				name = blockName;
			}
		}

		mDescriptorBindings.emplace_back( name, descriptorType, pSpvBinding->binding, pSpvBinding->set );
	}
}

static vk::DataType dtermineDataType( const SpvReflectTypeDescription *pDesc )
{
	uint32_t sign  = 0;
	uint32_t width = 32;
	uint32_t rows  = 1;
	uint32_t cols  = 1;

	vk::CompositeType composite = vk::CompositeType::SCALAR;
	if ( pDesc->op == SpvOpTypeVector ) {
		composite = vk::CompositeType::VECTOR;
		rows	  = pDesc->traits.numeric.vector.component_count;
	}
	else if ( pDesc->op == SpvOpTypeMatrix ) {
		composite = vk::CompositeType::MATRIX;
		rows	  = pDesc->traits.numeric.matrix.row_count;
		cols	  = pDesc->traits.numeric.matrix.column_count;
	}

	vk::ScalarType scalar = vk::ScalarType::UNKNOWN;
	if ( pDesc->type_flags & SPV_REFLECT_TYPE_FLAG_BOOL ) {
		scalar = vk::ScalarType::BOOL;
	}
	else if ( pDesc->type_flags & SPV_REFLECT_TYPE_FLAG_INT ) {
		scalar = vk::ScalarType::INT;
	}
	else if ( pDesc->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT ) {
		sign   = 1;
		scalar = vk::ScalarType::FLOAT;
	}

	uint32_t typeValue = CINDER_VK_MAKE_DATA_TYPE( sign, composite, rows, cols, scalar, width );

	vk::DataType dataType = static_cast<vk::DataType>( typeValue );
	// Validate just in case macro generated unknown type
	switch ( dataType ) {
		default:
			dataType = vk::DataType::UNKNOWN;
			break;
		case vk::DataType::BOOL1:
		case vk::DataType::BOOL2:
		case vk::DataType::BOOL3:
		case vk::DataType::BOOL4:
		case vk::DataType::INT1:
		case vk::DataType::INT2:
		case vk::DataType::INT3:
		case vk::DataType::INT4:
		case vk::DataType::UINT1:
		case vk::DataType::UINT2:
		case vk::DataType::UINT3:
		case vk::DataType::UINT4:
		case vk::DataType::FLOAT1:
		case vk::DataType::FLOAT2:
		case vk::DataType::FLOAT3:
		case vk::DataType::FLOAT4:
		case vk::DataType::FLOAT2x2:
		case vk::DataType::FLOAT2x3:
		case vk::DataType::FLOAT2x4:
		case vk::DataType::FLOAT3x2:
		case vk::DataType::FLOAT3x3:
		case vk::DataType::FLOAT3x4:
		case vk::DataType::FLOAT4x2:
		case vk::DataType::FLOAT4x3:
		case vk::DataType::FLOAT4x4:
			break;
	}

	return dataType;
}

static void parseSpirvUniformBlock( std::string prefix, const SpvReflectBlockVariable &spirvBlock, std::vector<Uniform> &uniforms )
{
	for ( uint32_t i = 0; i < spirvBlock.member_count; ++i ) {
		const SpvReflectBlockVariable &member = spirvBlock.members[i];

		if ( member.type_description->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT ) {
		}
		else {
			std::stringstream name;
			if ( ( prefix != CI_VK_DEFAULT_UNIFORM_BLOCK_NAME ) && ( prefix != CI_VK_HLSL_GLOBALS_NAME ) ) {
				name << prefix << ".";
			}
			name << member.name;

			vk::DataType dataType = dtermineDataType( member.type_description );
			if ( dataType == vk::DataType::UNKNOWN ) {
				throw VulkanExc( "failed to determine data type for uniform variable" );
			}

			vk::UniformSemantic uniformSemantic = vk::UniformSemantic::UNIFORM_USER_DEFINED;
			auto				it				= sDefaultUniformNameToSemanticMap.find( name.str() );
			if ( it != sDefaultUniformNameToSemanticMap.end() ) {
				uniformSemantic = it->second;
			}

			uint32_t offset = member.absolute_offset;

			uint32_t arraySize	 = 1;
			uint32_t arrayStride = 0;
			if ( member.type_description->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY ) {
				for ( uint32_t j = 0; j < member.array.dims_count; ++j ) {
					uint32_t dim = member.array.dims[j];
					arraySize *= dim;
				}

				arrayStride = member.array.stride;
			}

			uniforms.emplace_back( name.str(), dataType, uniformSemantic, offset, arraySize );
		}
	}
}

void ShaderModule::parseUniformBlocks( const std::vector<SpvReflectDescriptorBinding *> &spirvBindings )
{
	bool isGlsl = ( mSourceLanguage == SpvSourceLanguageGLSL );
	bool isHlsl = ( mSourceLanguage == SpvSourceLanguageHLSL );

	for ( size_t i = 0; i < spirvBindings.size(); ++i ) {
		const SpvReflectDescriptorBinding *pSpvBinding = spirvBindings[i];
		if ( pSpvBinding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER ) {
			continue;
		}

		std::string name = pSpvBinding->block.name;
		if ( name.empty() ) {
			if ( isGlsl ) {
				std::string typeName = spvReflectBlockVariableTypeName( &pSpvBinding->block );
				if ( typeName == CI_VK_DEFAULT_UNIFORM_BLOCK_NAME ) {
					name = typeName;
				}
			}
			else {
				throw VulkanExc( "unexpected unnamed uniform block" );
			}
		}

		std::vector<Uniform> uniforms;
		parseSpirvUniformBlock( name, pSpvBinding->block, uniforms );

		auto uniformBlock = std::make_unique<UniformBlock>( name, pSpvBinding->block.size, pSpvBinding->binding, pSpvBinding->set, uniforms );

		bool isGlslDefaultUniformBlock = isGlsl && ( name == CI_VK_DEFAULT_UNIFORM_BLOCK_NAME );
		bool isHlslGlobals			   = isHlsl && ( name == CI_VK_HLSL_GLOBALS_NAME );
		if ( isGlslDefaultUniformBlock || isHlslGlobals ) {
			mDefaultUniformBlock = uniformBlock.get();
		}

		mUniformBlocks.push_back( std::move( uniformBlock ) );
	}
}

const std::vector<vk::InterfaceVariable> &ShaderModule::getVertexAttributes() const
{
	return ( mShaderStage == VK_SHADER_STAGE_VERTEX_BIT ) ? mInputVariables : mNullVariables;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ShaderProg

ShaderProgRef ShaderProg::create(
	vk::ContextRef		  context,
	vk::ShaderModuleRef	  vertOrCompModule,
	VkShaderStageFlagBits shaderStage )
{
	if ( ( shaderStage != VK_SHADER_STAGE_VERTEX_BIT ) || ( shaderStage != VK_SHADER_STAGE_COMPUTE_BIT ) ) {
		throw VulkanExc( "invalid shader stage" );
	}

	if ( shaderStage == VK_SHADER_STAGE_VERTEX_BIT ) {
		return vk::ShaderProgRef( new vk::ShaderProg(
			context,
			vertOrCompModule,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr ) );
	}

	return vk::ShaderProgRef( new vk::ShaderProg(
		context,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		vertOrCompModule ) );
}

ShaderProgRef ShaderProg::create(
	vk::ContextRef		context,
	vk::ShaderModuleRef vertModule,
	vk::ShaderModuleRef fragModule,
	vk::ShaderModuleRef geomModule,
	vk::ShaderModuleRef teseModule,
	vk::ShaderModuleRef tescModule )
{
	return vk::ShaderProgRef( new vk::ShaderProg(
		context,
		vertModule,
		fragModule,
		geomModule,
		teseModule,
		tescModule,
		nullptr ) );
}

vk::ShaderProgRef ShaderProg::create(
	vk::ContextRef		  context,
	const DataSourceRef	&vertOrCompSpirvDataSource,
	VkShaderStageFlagBits shaderStage )
{
	std::vector<char> spirv;
	loadShaderSpirv( vertOrCompSpirvDataSource, spirv );

	return vk::ShaderProg::create( context, vk::SpirvBytecode{ spirv.size(), spirv.data() }, shaderStage );
}

vk::ShaderProgRef ShaderProg::create(
	vk::ContextRef		 context,
	const DataSourceRef &vertSpirvDataSource,
	const DataSourceRef &fragSpirvDataSource,
	const DataSourceRef &geomSpirvDataSource,
	const DataSourceRef &teseSpirvDataSource,
	const DataSourceRef &tescSpirvDataSource )
{
	std::vector<char> vertSpirv;
	std::vector<char> fragSpirv;
	std::vector<char> geomSpirv;
	std::vector<char> teseSpirv;
	std::vector<char> tescSpirv;
	loadShaderSpirv( vertSpirvDataSource, vertSpirv );
	loadShaderSpirv( fragSpirvDataSource, fragSpirv );
	loadShaderSpirv( geomSpirvDataSource, geomSpirv );
	loadShaderSpirv( teseSpirvDataSource, teseSpirv );
	loadShaderSpirv( tescSpirvDataSource, tescSpirv );

	return vk::ShaderProg::create(
		context,
		vk::SpirvBytecode{ vertSpirv.size(), vertSpirv.data() },
		vk::SpirvBytecode{ fragSpirv.size(), fragSpirv.data() },
		vk::SpirvBytecode{ geomSpirv.size(), geomSpirv.data() },
		vk::SpirvBytecode{ teseSpirv.size(), teseSpirv.data() },
		vk::SpirvBytecode{ tescSpirv.size(), tescSpirv.data() } );
}

vk::ShaderProgRef ShaderProg::create(
	vk::ContextRef			 context,
	const vk::SpirvBytecode &vsOrCsSpirv,
	VkShaderStageFlagBits	 shaderStage )
{
	auto spirvStore = std::vector<char>( vsOrCsSpirv.begin(), vsOrCsSpirv.end() );
	return vk::ShaderProgRef( new vk::ShaderProg( context, std::move( spirvStore ), shaderStage ) );
}

vk::ShaderProgRef ShaderProg::create(
	vk::ContextRef			 context,
	const vk::SpirvBytecode &vsSpirv,
	const vk::SpirvBytecode &psSpirv,
	const vk::SpirvBytecode &gsSpirv,
	const vk::SpirvBytecode &dsSpirv,
	const vk::SpirvBytecode &hsSpirv )
{
	auto vsSpirvStore = std::vector<char>( vsSpirv.begin(), vsSpirv.end() );
	auto psSpirvStore = std::vector<char>( psSpirv.begin(), psSpirv.end() );
	auto gsSpirvStore = std::vector<char>( gsSpirv.begin(), gsSpirv.end() );
	auto dsSpirvStore = std::vector<char>( dsSpirv.begin(), dsSpirv.end() );
	auto hsSpirvStore = std::vector<char>( hsSpirv.begin(), hsSpirv.end() );

	return vk::ShaderProgRef( new vk::ShaderProg(
		context,
		std::move( vsSpirvStore ),
		std::move( psSpirvStore ),
		std::move( gsSpirvStore ),
		std::move( dsSpirvStore ),
		std::move( hsSpirvStore ) ) );
}

ShaderProg::ShaderProg(
	vk::ContextRef		context,
	vk::ShaderModuleRef vs,
	vk::ShaderModuleRef ps,
	vk::ShaderModuleRef gs,
	vk::ShaderModuleRef ds,
	vk::ShaderModuleRef hs,
	vk::ShaderModuleRef cs )
	: vk::ContextChildObject( context ),
	  mVs( vs ),
	  mPs( ps ),
	  mGs( gs ),
	  mDs( ds ),
	  mHs( hs ),
	  mCs( cs )
{
	parseModules();
}

ShaderProg::ShaderProg(
	vk::ContextRef		  context,
	std::vector<char>	  &&vsOrCsSpirv,
	VkShaderStageFlagBits shaderStage )
	: vk::ContextChildObject( context )
{
}

static std::vector<SpvReflectInterfaceVariable *> getInputVariables(
	spv_reflect::ShaderModule &reflection )
{
	uint32_t		 count	= 0;
	SpvReflectResult spvres = reflection.EnumerateInputVariables( &count, nullptr );
	if ( spvres != SPV_REFLECT_RESULT_SUCCESS ) {
		throw VulkanExc( "interface variable alignment failed: couldn't enumerate input variable count" );
	}

	std::vector<SpvReflectInterfaceVariable *> vars( count );
	spvres = reflection.EnumerateInputVariables( &count, dataPtr( vars ) );
	if ( spvres != SPV_REFLECT_RESULT_SUCCESS ) {
		throw VulkanExc( "interface variable alignment failed: couldn't enumerate input variables" );
	}

	return vars;
}

static std::vector<SpvReflectInterfaceVariable *> getOutputVariables(
	spv_reflect::ShaderModule &reflection )
{
	uint32_t		 count	= 0;
	SpvReflectResult spvres = reflection.EnumerateOutputVariables( &count, nullptr );
	if ( spvres != SPV_REFLECT_RESULT_SUCCESS ) {
		throw VulkanExc( "interface variable alignment failed: couldn't enumerate output variable count" );
	}

	std::vector<SpvReflectInterfaceVariable *> vars( count );
	spvres = reflection.EnumerateOutputVariables( &count, dataPtr( vars ) );
	if ( spvres != SPV_REFLECT_RESULT_SUCCESS ) {
		throw VulkanExc( "interface variable alignment failed: couldn't enumerate output variables" );
	}

	return vars;
}

const uint32_t kSpirvWordSize		   = sizeof( uint32_t );
const uint32_t kSpirvBoundOffset	   = 3;
const uint32_t kSprivStartingWordIndex = 5;

static void removeRelaxedPrecision(
	std::vector<char>								*spirv,
	spv_reflect::ShaderModule						  &reflection,
	const std::vector<SpvReflectInterfaceVariable *> &targetVars )
{
	uint32_t		 *pWords		  = reinterpret_cast<uint32_t *>( spirv->data() );
	const uint32_t totalWordCount = static_cast<uint32_t>( spirv->size() / kSpirvWordSize );

	// Get a list of <op word index, op word count> for instructions to remove
	std::vector<std::pair<uint32_t, uint32_t>> removeInfo;

	uint32_t wordIndex = kSprivStartingWordIndex;
	while ( wordIndex < totalWordCount ) {
		const uint32_t word		 = pWords[wordIndex];
		const SpvOp	   op		 = (SpvOp)( word & SpvOpCodeMask );
		const uint32_t wordCount = ( word >> SpvWordCountShift ) & SpvOpCodeMask;

		if ( ( op == SpvOpDecorate ) && ( wordCount >= 3 ) ) {
			const uint32_t		id		   = pWords[wordIndex + 1];
			const SpvDecoration decoration = static_cast<SpvDecoration>( pWords[wordIndex + 2] );
			if ( decoration == SpvDecorationRelaxedPrecision ) {
				for ( size_t i = 0; i < targetVars.size(); ++i ) {
					if ( targetVars[i]->spirv_id == id ) {
						removeInfo.push_back( std::make_pair( wordIndex, wordCount ) );
					}
				}
			}
		}

		wordIndex += wordCount;
	}

	// Remove in backwards order
	const size_t n = removeInfo.size();
	for ( size_t i = 0; i < n; ++i ) {
		const size_t j	   = n - i - 1;
		auto		 &info  = removeInfo[j];
		auto		 first = spirv->begin() + ( info.first * kSpirvWordSize );
		auto		 last  = first + ( info.second * kSpirvWordSize );
		spirv->erase( first, last );
	}
}

static void alignInterfaceVariables(
	std::vector<char> *outStage,
	std::vector<char> *inStage )
{
	spv_reflect::ShaderModule outReflection = spv_reflect::ShaderModule( outStage->size(), outStage->data() );
	spv_reflect::ShaderModule inReflection	= spv_reflect::ShaderModule( inStage->size(), inStage->data() );

	if ( outReflection.GetResult() != SPV_REFLECT_RESULT_SUCCESS ) {
		throw VulkanExc( "interface variable alignment failed: reflection failed for out stage" );
	}

	if ( inReflection.GetResult() != SPV_REFLECT_RESULT_SUCCESS ) {
		throw VulkanExc( "interface variable alignment failed: reflection failed for out stage" );
	}

	if ( outReflection.GetShaderModule().source_language != inReflection.GetShaderModule().source_language ) {
		throw VulkanExc( "interface variable alignment failed: in and out stage smust have same source language" );
	}

	auto outVars = getOutputVariables( outReflection );
	auto inVars	 = getInputVariables( inReflection );

	bool changed = false;
	for ( size_t i = 0; i < inVars.size(); ++i ) {
		auto &inVar = inVars[i];
		// Skip built-in variables
		if ( inVar->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN ) {
			continue;
		}

		std::string inSemantic = ( inVar->semantic != nullptr ) ? inVar->semantic : "";
		if ( !inSemantic.empty() ) {
			// Use semantic to match up input/output variables
		}
		else {
			// Use name to match up input/output variables
			std::string inName = ( inVar->name != nullptr ) ? inVar->name : "";
			if ( inName.empty() ) {
				throw VulkanExc( "interface variable alignment failed: input variable must have name" );
			}
			for ( size_t j = 0; j < outVars.size(); ++j ) {
				auto		 &outVar	= outVars[j];
				std::string outName = ( outVar->name != nullptr ) ? outVar->name : "";
				// Skip if name doesn't match
				if ( inName != outName ) {
					continue;
				}
				// Change input location if it doesn't match output location
				if ( inVar->location != outVar->location ) {
					inReflection.ChangeInputVariableLocation( inVar, outVar->location );
					changed = true;
				}
				// Check for precion differences
				if ( outVar->decoration_flags & SPV_REFLECT_DECORATION_RELAXED_PRECISION ) {
				}
			}
		}
	}

	if ( changed ) {
		const size_t sizeInBytes = inReflection.GetCodeSize();
		const char  *pCode		 = reinterpret_cast<const char *>( inReflection.GetCode() );
		*inStage				 = std::vector<char>( pCode, pCode + sizeInBytes );
	}

	// Remove RelaxedPrecision decoration for output variables in output stage
	std::vector<SpvReflectInterfaceVariable *> relaxedVars;
	for ( size_t i = 0; i < outVars.size(); ++i ) {
		if ( outVars[i]->decoration_flags & SPV_REFLECT_DECORATION_RELAXED_PRECISION ) {
			relaxedVars.push_back( outVars[i] );
		}
	}
	if ( !relaxedVars.empty() ) {
		removeRelaxedPrecision( outStage, outReflection, relaxedVars );
	}

	// Remove RelaxedPrecision decoration for input variables in input stage
	relaxedVars.clear();
	for ( size_t i = 0; i < inVars.size(); ++i ) {
		if ( inVars[i]->decoration_flags & SPV_REFLECT_DECORATION_RELAXED_PRECISION ) {
			relaxedVars.push_back( inVars[i] );
		}
	}
	if ( !relaxedVars.empty() ) {
		removeRelaxedPrecision( inStage, inReflection, relaxedVars );
	}
}

ShaderProg::ShaderProg(
	vk::ContextRef		context,
	std::vector<char> &&vsSpirv,
	std::vector<char> &&psSpirv,
	std::vector<char> &&gsSpirv,
	std::vector<char> &&dsSpirv,
	std::vector<char> &&hsSpirv )
	: vk::ContextChildObject( context )
{
	std::vector<std::vector<char> *> stages;
	if ( !vsSpirv.empty() ) {
		stages.push_back( &vsSpirv );
	}
	if ( !hsSpirv.empty() ) {
		stages.push_back( &hsSpirv );
	}
	if ( !dsSpirv.empty() ) {
		stages.push_back( &dsSpirv );
	}
	if ( !gsSpirv.empty() ) {
		stages.push_back( &gsSpirv );
	}
	if ( !psSpirv.empty() ) {
		stages.push_back( &psSpirv );
	}

	const size_t numStages = stages.size();
	if ( numStages < 2 ) {
		throw VulkanExc( "invalid number of stages" );
	}

	for ( size_t i = 1; i < numStages; ++i ) {
		std::vector<char> *outStage = stages[i - 1];
		std::vector<char> *inStage	= stages[i];
		alignInterfaceVariables( outStage, inStage );
	}

	if ( !vsSpirv.empty() ) {
		mVs = vk::ShaderModule::create( vsSpirv, context->getDevice() );
	}
	if ( !hsSpirv.empty() ) {
		mHs = vk::ShaderModule::create( hsSpirv, context->getDevice() );
	}
	if ( !dsSpirv.empty() ) {
		mDs = vk::ShaderModule::create( dsSpirv, context->getDevice() );
	}
	if ( !gsSpirv.empty() ) {
		mGs = vk::ShaderModule::create( gsSpirv, context->getDevice() );
	}
	if ( !psSpirv.empty() ) {
		mPs = vk::ShaderModule::create( psSpirv, context->getDevice() );
	}

	parseModules();
}

ShaderProg::~ShaderProg()
{
}

void ShaderProg::parseModules()
{
	parseDscriptorBindings( mVs.get() );
	parseDscriptorBindings( mPs.get() );
	parseDscriptorBindings( mGs.get() );
	parseDscriptorBindings( mDs.get() );
	parseDscriptorBindings( mHs.get() );
	parseDscriptorBindings( mCs.get() );

	mUniformBlocks.clear();
	mDefaultUniformBlock = nullptr;
	parseUniformBlocks( mVs.get() );
	parseUniformBlocks( mPs.get() );
	parseUniformBlocks( mGs.get() );
	parseUniformBlocks( mDs.get() );
	parseUniformBlocks( mHs.get() );
	parseUniformBlocks( mCs.get() );

	for ( auto &block : mUniformBlocks ) {
		auto					  &name	   = block->getName();
		vk::UniformBuffer::Options options = vk::UniformBuffer::Options().cpuOnly();
		vk::UniformBufferRef	   buffer  = vk::UniformBuffer::create( block, options, getContext() );
		mUniformBuffers[name]			   = buffer;

		if ( ( mDefaultUniformBuffer == nullptr ) && ( name == CI_VK_DEFAULT_UNIFORM_BLOCK_NAME ) || ( name == CI_VK_HLSL_GLOBALS_NAME ) ) {
			mDefaultUniformBuffer = buffer.get();
		}

		auto &uniforms = block->getUniforms();
		for ( auto &uniform : uniforms ) {
			auto &uniformName				 = uniform.getName();
			mUniforNameToBuffer[uniformName] = buffer;
		}
	}
}

void ShaderProg::parseDscriptorBindings( const vk::ShaderModule *shader )
{
	mSetBindings.clear();
	mDescriptorBindings.clear();

	if ( shader == nullptr ) {
		return;
	}

	auto descriptorBindings = shader->getDescriptorBindings();

	for ( const auto &binding : descriptorBindings ) {
		// Get reference to bindings vector
		const uint32_t setNumber = binding.getSet();
		auto			 &bindings	 = mSetBindings[setNumber];
		// Look binding using binding number
		auto		   it		 = std::find_if(
			 bindings.begin(),
			 bindings.end(),
			 [binding]( const vk::DescriptorBinding &elem ) -> bool {
								 bool isMatchBinding = ( elem.getBinding() == binding.getBinding() );
								 return isMatchBinding;
			 } );
		// Add if the binding if not match is found
		if ( it == bindings.end() ) {
			bindings.push_back( binding );
		}
		// Types must match if binding exists
		else {
			if ( binding.getType() != it->getType() ) {
				std::stringstream ss;
				ss << "descriptor type mismatch where binding=" << binding.getBinding() << ", set=" << binding.getSet();
				throw VulkanExc( ss.str() );
			}
		}
	}

	for ( auto setIt : mSetBindings ) {
		for ( auto &binding : setIt.second ) {
			// Unnamed bindings will be a problem later for uniform() calls
			if ( binding.getName().empty() ) {
				std::stringstream ss;
				ss << "unnamed binding: " << binding.getSet() << "." << binding.getBinding();
				throw VulkanExc( ss.str() );
			}
			// Check for duplicate names
			auto it = mDescriptorBindings.find( binding.getName() );
			if ( it != mDescriptorBindings.end() ) {
				std::stringstream ss;
				ss << "duplicate binding name: " << binding.getSet() << "." << binding.getBinding();
				ss << " and " << ( *it ).second->getSet() << "." << ( *it ).second->getBinding();
				throw VulkanExc( ss.str() );
			}
			// Add binding
			mDescriptorBindings[binding.getName()] = &binding;
		}
	}
}

void ShaderProg::parseUniformBlocks( const vk::ShaderModule *shader )
{
	if ( shader == nullptr ) {
		return;
	}

	auto &blocks = shader->getUniformBlocks();
	for ( const auto &block : blocks ) {
		if ( ( block->getName() == CI_VK_DEFAULT_UNIFORM_BLOCK_NAME ) || ( block->getName() == CI_VK_HLSL_GLOBALS_NAME ) ) {
			// Add default uniform block if we don't have one...
			if ( mDefaultUniformBlock == nullptr ) {
				mDefaultUniformBlock = block.get();

				mUniformBlocks.push_back( std::move( block ) );
			}
			// ...otherwise validate that default uniform blocks have the set, binding, size, and uniforms
			else {
				auto it = std::find_if(
					mUniformBlocks.begin(),
					mUniformBlocks.end(),
					[]( const vk::UniformBlockRef &elem ) -> bool {
						bool res = ( elem->getName() == CI_VK_DEFAULT_UNIFORM_BLOCK_NAME ) || ( elem->getName() == CI_VK_HLSL_GLOBALS_NAME );
						return res;
					} );

				if ( it != mUniformBlocks.end() ) {
					if ( block->getSet() != ( *it )->getSet() ) {
						throw VulkanExc( "default uniform buffers in all shader stages must have same set" );
					}

					if ( block->getBinding() != ( *it )->getBinding() ) {
						throw VulkanExc( "default uniform buffers in all shader stages must have same binding" );
					}

					if ( block->getSize() != ( *it )->getSize() ) {
						throw VulkanExc( "default uniform buffers in all shader stages must have same size" );
					}

					if ( block->getUniforms() != ( *it )->getUniforms() ) {
						throw VulkanExc( "default uniform buffers in all shader stages must have same uniforms" );
					}
				}
			}
		}
		else {
			// Uniform blocks with the same binding/set must have same size, uniforms, and name.
			// If the uniform block names aren't the same, uniform look up will have undefined
			// behavior.
			//
			for ( const auto &existingBlock : mUniformBlocks ) {
				bool isSameBinding = ( block->getBinding() == existingBlock->getBinding() );
				bool isSameSet	   = ( block->getSet() == existingBlock->getSet() );
				if ( !( isSameBinding && isSameSet ) ) {
					continue;
				}

				if ( block->getSize() != existingBlock->getSize() ) {
					std::stringstream ss;
					ss << "uniform blocks at binding=" << block->getBinding() << ", set=" << block->getSet() << " do not have the same size";
					throw VulkanExc( ss.str() );
				}

				if ( block->getUniforms() != existingBlock->getUniforms() ) {
					std::stringstream ss;
					ss << "uniform blocks at binding=" << block->getBinding() << ", set=" << block->getSet() << " do not have the same uniforms";
					throw VulkanExc( ss.str() );
				}

				if ( block->getName() != existingBlock->getName() ) {
					std::stringstream ss;
					ss << "uniform blocks at binding=" << block->getBinding() << ", set=" << block->getSet() << " do not have the same name";
					throw VulkanExc( ss.str() );
				}
			}
		}
	}
}

const std::vector<vk::InterfaceVariable> &ShaderProg::getVertexAttributes() const
{
	return mVs ? mVs->getVertexAttributes() : mNullVariables;
}

// void ShaderProg::uniform( const std::string &name, int data ) const
//{
//	// Check bindings first
//	bool descriptorFound = false;
//	{
//		auto it = mDescriptorBindings.find( name );
//		if ( it != mDescriptorBindings.end() ) {
//			const uint32_t binding = static_cast<uint32_t>( data + CINDER_CONTEXT_BINDING_SHIFT_TEXTURE );
//
//			bool isSameType	   = ( it->second->getType() != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER );
//			bool isSameBinding = ( binding != it->second->getBinding() );
//			descriptorFound	   = ( isSameType && isSameBinding );
//
//			if ( !descriptorFound ) {
//				if ( !isSameType ) {
//					CI_LOG_E( name << " is not a texture resource" );
//				}
//				else {
//					if ( !isSameBinding ) {
//						CI_LOG_E( "binding mismatch for " << name << ", expected " << it->second->getBinding() << " got " << data );
//					}
//				}
//			}
//		}
//	}
//
//	// Check
//	if ( !descriptorFound ) {
//	}
// }

template <typename T>
void ShaderProg::setUniform( const std::string &name, const T &value )
{
	auto it = mUniforNameToBuffer.find( name );
	if ( it == mUniforNameToBuffer.end() ) {
		return;
	}
	it->second->uniform( name, value );
}

void ShaderProg::uniform( const std::string &name, bool value )
{
	setUniform<bool>( name, value );
}

void ShaderProg::uniform( const std::string &name, int32_t value )
{
	setUniform<bool>( name, value );
}

void ShaderProg::uniform( const std::string &name, uint32_t value )
{
	setUniform<bool>( name, value );
}

void ShaderProg::uniform( const std::string &name, float value )
{
	setUniform<bool>( name, value );
}

void ShaderProg::uniform( const std::string &name, const glm::vec2 &value )
{
	setUniform<glm::vec2>( name, value );
}

void ShaderProg::uniform( const std::string &name, const glm::vec3 &value )
{
	setUniform<glm::vec3>( name, value );
}

void ShaderProg::uniform( const std::string &name, const glm::vec4 &value )
{
	setUniform<glm::vec4>( name, value );
}

void ShaderProg::uniform( const std::string &name, const glm::mat2x2 &value )
{
	setUniform<glm::mat2x2>( name, value );
}

void ShaderProg::uniform( const std::string &name, const glm::mat2x3 &value )
{
	setUniform<glm::mat2x3>( name, value );
}

void ShaderProg::uniform( const std::string &name, const glm::mat2x4 &value )
{
	setUniform<glm::mat2x4>( name, value );
}

void ShaderProg::uniform( const std::string &name, const glm::mat3x2 &value )
{
	setUniform<glm::mat3x2>( name, value );
}

void ShaderProg::uniform( const std::string &name, const glm::mat3x3 &value )
{
	setUniform<glm::mat3x3>( name, value );
}

void ShaderProg::uniform( const std::string &name, const glm::mat3x4 &value )
{
	setUniform<glm::mat3x4>( name, value );
}

void ShaderProg::uniform( const std::string &name, const glm::mat4x2 &value )
{
	setUniform<glm::mat4x2>( name, value );
}

void ShaderProg::uniform( const std::string &name, const glm::mat4x3 &value )
{
	setUniform<glm::mat4x3>( name, value );
}

void ShaderProg::uniform( const std::string &name, const glm::mat4x4 &value )
{
	setUniform<glm::mat4x4>( name, value );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GlslProg

vk::GlslProgRef GlslProg::create(
	const DataSourceRef &vertTextDataSource,
	const DataSourceRef &fragTextDataSource,
	const DataSourceRef &geomTextDataSource,
	const DataSourceRef &teseTextDataSource,
	const DataSourceRef &tescTextDataSource )
{
	vk::ContextRef context = app::RendererVk::getCurrentRenderer()->getContext();
	return vk::GlslProg::create( context, vertTextDataSource, fragTextDataSource, geomTextDataSource, teseTextDataSource, tescTextDataSource );
}

vk::GlslProgRef GlslProg::create(
	vk::ContextRef		 context,
	const DataSourceRef &vertTextDataSource,
	const DataSourceRef &fragTextDataSource,
	const DataSourceRef &geomTextDataSource,
	const DataSourceRef &teseTextDataSource,
	const DataSourceRef &tescTextDataSource )
{
	std::string vertText;
	std::string fragText;
	std::string geomText;
	std::string teseText;
	std::string tescText;

	loadShaderText( vertTextDataSource, vertText );
	loadShaderText( fragTextDataSource, fragText );
	loadShaderText( geomTextDataSource, geomText );
	loadShaderText( teseTextDataSource, teseText );
	loadShaderText( tescTextDataSource, tescText );

	return vk::GlslProg::create( context, vertText, fragText, geomText, teseText, tescText );
}

vk::GlslProgRef GlslProg::create(
	const std::string &vertText,
	const std::string &fragText,
	const std::string &geomText,
	const std::string &teseText,
	const std::string &tescText )
{
	vk::ContextRef context = app::RendererVk::getCurrentRenderer()->getContext();
	return vk::GlslProg::create( context, vertText, fragText, geomText, teseText, tescText );
}

vk::GlslProgRef GlslProg::create(
	vk::ContextRef	   context,
	const std::string &vertText,
	const std::string &fragText,
	const std::string &geomText,
	const std::string &teseText,
	const std::string &tescText )
{
	auto vsSpirv = compileShader( context->getDevice(), vertText, VK_SHADER_STAGE_VERTEX_BIT );
	auto psSpirv = compileShader( context->getDevice(), fragText, VK_SHADER_STAGE_FRAGMENT_BIT );
	auto gsSpirv = compileShader( context->getDevice(), geomText, VK_SHADER_STAGE_GEOMETRY_BIT );
	auto dsSpirv = compileShader( context->getDevice(), teseText, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT );
	auto hsSpirv = compileShader( context->getDevice(), tescText, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT );

	return vk::GlslProgRef( new GlslProg(
		context,
		std::move( vsSpirv ),
		std::move( psSpirv ),
		std::move( gsSpirv ),
		std::move( dsSpirv ),
		std::move( hsSpirv ) ) );
}

GlslProg::GlslProg(
	vk::ContextRef		  context,
	std::vector<char>	  &&vsOrCsSpirv,
	VkShaderStageFlagBits shaderStage )
	: vk::ShaderProg( context, std::forward<std::vector<char>>( vsOrCsSpirv ), shaderStage )
{
}

GlslProg::GlslProg(
	vk::ContextRef		context,
	std::vector<char> &&vsSpirv,
	std::vector<char> &&psSpirv,
	std::vector<char> &&gsSpirv,
	std::vector<char> &&dsSpirv,
	std::vector<char> &&hsSpirv )
	: vk::ShaderProg(
		  context,
		  std::forward<std::vector<char>>( vsSpirv ),
		  std::forward<std::vector<char>>( psSpirv ),
		  std::forward<std::vector<char>>( gsSpirv ),
		  std::forward<std::vector<char>>( dsSpirv ),
		  std::forward<std::vector<char>>( hsSpirv ) )
{
}

GlslProg::~GlslProg()
{
}

class Glslang
{
public:
	~Glslang()
	{
		glslang_finalize_process();
	}

	static int initialize()
	{
		if ( !sInstance ) {
			int res = glslang_initialize_process();
			if ( res == 0 ) {
				return res;
			}
			sInstance = std::make_unique<Glslang>();
		}
		return 1;
	}

private:
	static std::unique_ptr<Glslang> sInstance;
};

std::unique_ptr<Glslang> Glslang::sInstance;

static void replaceVersion( std::string &text )
{
	std::regex expr( "#version\\s+\\d+(\\s*es)?" );
	text = std::regex_replace( text, expr, "#version 460" );
}

std::vector<char> GlslProg::compileShader(
	vk::DeviceRef		  device,
	std::string			  text,
	VkShaderStageFlagBits stage )
{
	if ( text.empty() ) {
		return std::vector<char>();
	}

	replaceVersion( text );

	const glslang_stage_t			kInvalidStage		 = static_cast<glslang_stage_t>( ~0 );
	glslang_target_client_version_t kInvalidClientVerson = static_cast<glslang_target_client_version_t>( ~0 );

	glslang_stage_t glslangStage = kInvalidStage;
	// clang-format off
	switch ( stage ) {
		default:break;
		case VK_SHADER_STAGE_VERTEX_BIT					: glslangStage = GLSLANG_STAGE_VERTEX; break;
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT	: glslangStage = GLSLANG_STAGE_TESSCONTROL; break;
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: glslangStage = GLSLANG_STAGE_TESSEVALUATION; break;
		case VK_SHADER_STAGE_GEOMETRY_BIT				: glslangStage = GLSLANG_STAGE_GEOMETRY; break;
		case VK_SHADER_STAGE_FRAGMENT_BIT				: glslangStage = GLSLANG_STAGE_FRAGMENT; break;
		case VK_SHADER_STAGE_COMPUTE_BIT				: glslangStage = GLSLANG_STAGE_COMPUTE; break;
		case VK_SHADER_STAGE_RAYGEN_BIT_KHR				: glslangStage = GLSLANG_STAGE_RAYGEN_NV; break;
		case VK_SHADER_STAGE_ANY_HIT_BIT_KHR			: glslangStage = GLSLANG_STAGE_ANYHIT_NV; break;
		case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR		: glslangStage = GLSLANG_STAGE_CLOSESTHIT_NV; break;
		case VK_SHADER_STAGE_MISS_BIT_KHR				: glslangStage = GLSLANG_STAGE_MISS_NV; break;
		case VK_SHADER_STAGE_INTERSECTION_BIT_KHR		: glslangStage = GLSLANG_STAGE_INTERSECT_NV; break;
		case VK_SHADER_STAGE_CALLABLE_BIT_KHR			: glslangStage = GLSLANG_STAGE_CALLABLE_NV; break;
		case VK_SHADER_STAGE_TASK_BIT_NV				: glslangStage = GLSLANG_STAGE_TASK_NV; break;
		case VK_SHADER_STAGE_MESH_BIT_NV				: glslangStage = GLSLANG_STAGE_MESH_NV; break;
	}
	// clang-format on
	if ( glslangStage == kInvalidStage ) {
		throw VulkanExc( "unrecognized Vulkan shader stage" );
	}

	glslang_target_client_version_t clientVersion = kInvalidClientVerson;
	// clang-format off
	const uint32_t apiVersion = vk::Environment::get()->getApiVersion();
	switch (apiVersion) {
		default: break;
		case VK_API_VERSION_1_1: clientVersion = GLSLANG_TARGET_VULKAN_1_1; break;
		case VK_API_VERSION_1_2: clientVersion = GLSLANG_TARGET_VULKAN_1_2; break;
	}
	// clang-format on

	glslang_input_t input					= {};
	input.language							= GLSLANG_SOURCE_GLSL;
	input.stage								= glslangStage;
	input.client							= GLSLANG_CLIENT_VULKAN;
	input.client_version					= clientVersion;
	input.target_language					= GLSLANG_TARGET_SPV;
	input.target_language_version			= GLSLANG_TARGET_SPV_1_3;
	input.code								= text.c_str();
	input.default_version					= 100;
	input.default_profile					= GLSLANG_NO_PROFILE;
	input.force_default_version_and_profile = false;
	input.forward_compatible				= false;
	input.messages							= GLSLANG_MSG_DEFAULT_BIT;
	input.resource							= glslang_default_resource();

	int res = Glslang::initialize();
	if ( res == 0 ) {
		throw VulkanExc( "GLSL compiler initialization failed" );
	}

	glslang_shader_t *shader = glslang_shader_create( &input );

	// Shift bindings
	glslang_shader_shift_binding( shader, GLSLANG_RESOURCE_TYPE_TEXTURE, CINDER_CONTEXT_BINDING_SHIFT_TEXTURE );
	glslang_shader_shift_binding( shader, GLSLANG_RESOURCE_TYPE_UBO, CINDER_CONTEXT_BINDING_SHIFT_UBO );
	glslang_shader_shift_binding( shader, GLSLANG_RESOURCE_TYPE_IMAGE, CINDER_CONTEXT_BINDING_SHIFT_IMAGE );
	glslang_shader_shift_binding( shader, GLSLANG_RESOURCE_TYPE_SAMPLER, CINDER_CONTEXT_BINDING_SHIFT_SAMPLER );
	glslang_shader_shift_binding( shader, GLSLANG_RESOURCE_TYPE_SSBO, CINDER_CONTEXT_BINDING_SHIFT_SSBO );
	glslang_shader_shift_binding( shader, GLSLANG_RESOURCE_TYPE_UAV, CINDER_CONTEXT_BINDING_SHIFT_UAV );

	// Options
	int shaderOptions = GLSLANG_SHADER_AUTO_MAP_BINDINGS | GLSLANG_SHADER_AUTO_MAP_LOCATIONS | GLSLANG_SHADER_VULKAN_RULES_RELAXED;
	glslang_shader_set_options( shader, shaderOptions );

	// Preprocess
	if ( !glslang_shader_preprocess( shader, &input ) ) {
		const char *infoLog = glslang_shader_get_info_log( shader );
		if ( infoLog != nullptr ) {
			CI_LOG_E( "GLSL preprocess failed (info): " << infoLog );
		}

		const char *debugLog = glslang_shader_get_info_debug_log( shader );
		if ( debugLog != nullptr ) {
			CI_LOG_E( "GLSL preprocess failed (info debug): " << debugLog );
		}

		throw VulkanExc( "GLSL shader preprocess failed" );
	}

	// Compile
	if ( !glslang_shader_parse( shader, &input ) ) {
		const char *infoLog = glslang_shader_get_info_log( shader );
		if ( infoLog != nullptr ) {
			CI_LOG_E( "GLSL compile failed (info): " << infoLog );
		}

		const char *debugLog = glslang_shader_get_info_debug_log( shader );
		if ( debugLog != nullptr ) {
			CI_LOG_E( "GLSL compile failed (info debug): " << debugLog );
		}

		throw VulkanExc( "GLSL shader parse failed" );
	}

	glslang_program_t *program = glslang_program_create();
	glslang_program_add_shader( program, shader );

	// Link
	if ( !glslang_program_link( program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT ) ) {
		const char *infoLog = glslang_program_get_info_log( program );
		if ( infoLog != nullptr ) {
			CI_LOG_E( "GLSL link failed (info): " << infoLog );
		}

		const char *debugLog = glslang_program_get_info_debug_log( program );
		if ( debugLog != nullptr ) {
			CI_LOG_E( "GLSL link failed (info debug): " << debugLog );
		}

		throw VulkanExc( "GLSL program link failed" );
	}

	// Map IO
	if ( !glslang_program_map_io( program ) ) {
		throw VulkanExc( "GLSL program map IO failed" );
	}

	glslang_program_SPIRV_generate( program, input.stage );

	const char *spirvMsg = glslang_program_SPIRV_get_messages( program );
	if ( spirvMsg != nullptr ) {
		CI_LOG_I( "GLSL to SPIR-V: " << spirvMsg );
	}
	glslang_shader_delete( shader );

	const size_t sizeInBytes = glslang_program_SPIRV_get_size( program ) * sizeof( uint32_t );
	const char  *pSpirvCode	 = reinterpret_cast<const char *>( glslang_program_SPIRV_get_ptr( program ) );

	///*
	std::stringstream ss;
	ss << std::setw( 16 ) << std::setfill( '0' ) << std::hex << reinterpret_cast<uintptr_t>( pSpirvCode ) << ".spv";
	std::string fileName = ss.str();

	std::ofstream os( fileName.c_str(), std::ios::binary );
	if ( os.is_open() ) {
		os.write( pSpirvCode, sizeInBytes );
		os.close();

		cinder::app::console() << "Wrote: " << fileName << std::endl;
	}
	//*/

	// vk::ShaderModuleRef shaderModule = vk::ShaderModule::create( sizeInBytes, pSpirvCode, device );
	auto spirv = std::vector<char>( pSpirvCode, pSpirvCode + sizeInBytes );

	glslang_program_delete( program );

	return spirv;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HlslProg

HlslProgRef HlslProg::create(
	const DataSourceRef &vsTextDataSource,
	const DataSourceRef &psTextDataSource,
	const DataSourceRef &gsTextDataSource,
	const DataSourceRef &dsTextDataSource,
	const DataSourceRef &hsTextDataSource )
{
	vk::ContextRef context = app::RendererVk::getCurrentRenderer()->getContext();
	return vk::HlslProg::create( context, vsTextDataSource, psTextDataSource, gsTextDataSource, dsTextDataSource, hsTextDataSource );
}

HlslProgRef HlslProg::create(
	vk::ContextRef		 context,
	const DataSourceRef &vsTextDataSource,
	const DataSourceRef &psTextDataSource,
	const DataSourceRef &gsTextDataSource,
	const DataSourceRef &dsTextDataSource,
	const DataSourceRef &hsTextDataSource )
{
	std::string vsText;
	std::string psText;
	std::string gsText;
	std::string dsText;
	std::string hsText;

	loadShaderText( vsTextDataSource, vsText );
	loadShaderText( psTextDataSource, psText );
	loadShaderText( gsTextDataSource, gsText );
	loadShaderText( dsTextDataSource, dsText );
	loadShaderText( hsTextDataSource, hsText );

	return vk::HlslProg::create( context, vsText, psText, gsText, dsText, hsText );
}

vk::HlslProgRef HlslProg::create(
	const std::string &vsText,
	const std::string &psText,
	const std::string &gsText,
	const std::string &dsText,
	const std::string &hsText )
{
	vk::ContextRef context = app::RendererVk::getCurrentRenderer()->getContext();
	return vk::HlslProg::create( context, vsText, psText, gsText, dsText, hsText );
}

vk::HlslProgRef HlslProg::create(
	vk::ContextRef	   context,
	const std::string &vsText,
	const std::string &psText,
	const std::string &gsText,
	const std::string &dsText,
	const std::string &hsText )
{
	auto vsSpirv = compileShader( context->getDevice(), vsText, VK_SHADER_STAGE_VERTEX_BIT );
	auto psSpirv = compileShader( context->getDevice(), psText, VK_SHADER_STAGE_FRAGMENT_BIT );
	auto gsSpirv = compileShader( context->getDevice(), gsText, VK_SHADER_STAGE_GEOMETRY_BIT );
	auto dsSpirv = compileShader( context->getDevice(), dsText, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT );
	auto hsSpirv = compileShader( context->getDevice(), hsText, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT );

	return vk::HlslProgRef( new vk::HlslProg(
		context,
		std::move( vsSpirv ),
		std::move( psSpirv ),
		std::move( gsSpirv ),
		std::move( dsSpirv ),
		std::move( hsSpirv ) ) );
}

HlslProg::HlslProg(
	vk::ContextRef		  context,
	std::vector<char>	  &&vsOrCsSpirv,
	VkShaderStageFlagBits shaderStage )
	: vk::ShaderProg( context, std::forward<std::vector<char>>( vsOrCsSpirv ), shaderStage )
{
}

HlslProg::HlslProg(
	vk::ContextRef		context,
	std::vector<char> &&vsSpirv,
	std::vector<char> &&psSpirv,
	std::vector<char> &&gsSpirv,
	std::vector<char> &&dsSpirv,
	std::vector<char> &&hsSpirv )
	: vk::ShaderProg(
		  context,
		  std::forward<std::vector<char>>( vsSpirv ),
		  std::forward<std::vector<char>>( psSpirv ),
		  std::forward<std::vector<char>>( gsSpirv ),
		  std::forward<std::vector<char>>( dsSpirv ),
		  std::forward<std::vector<char>>( hsSpirv ) )
{
}

HlslProg::~HlslProg()
{
}

std::vector<char> HlslProg::compileShader(
	vk::DeviceRef			  device,
	const std::string		  &text,
	VkShaderStageFlagBits	  shaderStage,
	const std::string		  &entryPointName,
	vk::HlslProg::ShaderModel shaderModel )
{
	if ( text.empty() ) {
		return std::vector<char>();
	}

	const std::u16string kSpirvArg		  = toUtf16( "-spirv" );
	const std::u16string kSpirvReflectArg = toUtf16( "-fspv-reflect" );
	const std::u16string kShiftTArg		  = toUtf16( std::to_string( CINDER_CONTEXT_BINDING_SHIFT_TEXTURE ) );
	const std::u16string kShiftBArg		  = toUtf16( std::to_string( CINDER_CONTEXT_BINDING_SHIFT_UBO ) );
	const std::u16string kShiftSArg		  = toUtf16( std::to_string( CINDER_CONTEXT_BINDING_SHIFT_TEXTURE ) );
	const std::u16string kShiftUArg		  = toUtf16( std::to_string( CINDER_CONTEXT_BINDING_SHIFT_UAV ) );
	const std::u16string kSourceName	  = toUtf16( "DxcCompileHLSL" );

	std::string profileUtf8;
	std::string defaultEntryPoint;
	// clang-format off
	switch ( shaderStage ) {
		default: {
			throw VulkanExc("unknown shader stage");
		} break;

		case VK_SHADER_STAGE_VERTEX_BIT                  : profileUtf8 = "vs_6_0"; defaultEntryPoint = "vsmain"; break;
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT    : profileUtf8 = "hs_6_0"; defaultEntryPoint = "hsmain"; break;
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT : profileUtf8 = "ds_6_0"; defaultEntryPoint = "dsmain"; break;
		case VK_SHADER_STAGE_GEOMETRY_BIT                : profileUtf8 = "gs_6_0"; defaultEntryPoint = "gsmain"; break;
		case VK_SHADER_STAGE_FRAGMENT_BIT                : profileUtf8 = "ps_6_0"; defaultEntryPoint = "psmain"; break;
		case VK_SHADER_STAGE_COMPUTE_BIT                 : profileUtf8 = "cs_6_0"; defaultEntryPoint = "csmain"; break;
	}
	// clang-format on

	switch ( shaderModel ) {
		default: {
			throw VulkanExc( "unknown shader model" );
		} break;

		case ShaderModel::SM_6_0: profileUtf8[5] = '0'; break;
		case ShaderModel::SM_6_1: profileUtf8[5] = '1'; break;
		case ShaderModel::SM_6_2: profileUtf8[5] = '2'; break;
		case ShaderModel::SM_6_3: profileUtf8[5] = '3'; break;
		case ShaderModel::SM_6_4: profileUtf8[5] = '4'; break;
		case ShaderModel::SM_6_5: profileUtf8[5] = '5'; break;
		case ShaderModel::SM_6_6: profileUtf8[5] = '6'; break;
	}

	std::u16string profile	  = toUtf16( profileUtf8 );
	std::u16string entryPoint = toUtf16( entryPointName.empty() ? defaultEntryPoint : entryPointName );

	ComPtr<IDxcLibrary> library;
	HRESULT				hr = DxcCreateInstance( CLSID_DxcLibrary, __uuidof( IDxcLibrary ), (void **)&library );
	if ( FAILED( hr ) ) {
		throw VulkanExc( "hlsl compile error: create library failed" );
	}

	ComPtr<IDxcBlobEncoding> source;
	hr = library->CreateBlobWithEncodingFromPinned(
		(LPVOID)text.c_str(),
		static_cast<UINT32>( text.length() ),
		CP_ACP,
		&source );
	if ( FAILED( hr ) ) {
		throw VulkanExc( "hlsl compile error: create source blob failed" );
	}

	// clang-format off
	std::vector<LPCWSTR> arguments = {
		(LPCWSTR)kSpirvArg.c_str(),
		(LPCWSTR)kSpirvReflectArg.c_str(),
		L"-fvk-auto-shift-bindings",
		L"-fvk-t-shift", (LPCWSTR)kShiftTArg.c_str(), L"0",
		L"-fvk-b-shift", (LPCWSTR)kShiftBArg.c_str(), L"0",
		L"-fvk-s-shift", (LPCWSTR)kShiftSArg.c_str(), L"0",
		L"-fvk-u-shift", (LPCWSTR)kShiftUArg.c_str(), L"0",
	};
	// clang-format on

	std::vector<std::pair<std::u16string, std::u16string>> defineNameValues;
	// for ( XgiU32 i = 0; i < numDefines; ++i ) {
	//	xgi::String				 s		= pDefines[i];
	//	xgi::Vector<xgi::String> tokens = xgi::SplitString( s, xgi::String( "=" ) );
	//	xgi::WString			 key;
	//	xgi::WString			 value;
	//	if ( tokens.size() == 2 ) {
	//		key	  = StringToWString( tokens[0] );
	//		value = StringToWString( tokens[1] );
	//	}
	//	else if ( tokens.size() == 1 ) {
	//		key = StringToWString( tokens[0] );
	//	}
	//	defineNameValues.emplace_back( std::make_pair( key, value ) );
	// }

	std::vector<DxcDefine> defines;
	// for ( auto &elem : defineNameValues ) {
	//	DxcDefine define = {};
	//	define.Name		 = elem.first.c_str();
	//	define.Value	 = elem.second.empty() ? nullptr : elem.second.c_str();
	//	defines.push_back( define );
	// }

	ComPtr<IDxcCompiler> compiler;
	hr = DxcCreateInstance( CLSID_DxcCompiler, __uuidof( IDxcCompiler ), (void **)&compiler );
	if ( FAILED( hr ) ) {
		throw VulkanExc( "hlsl compile error: create compiler failed" );
	}

	ComPtr<IDxcOperationResult> operationResult;
	hr = compiler->Compile(
		source.Get(),
		(LPCWSTR)kSourceName.c_str(),
		(LPCWSTR)entryPoint.c_str(),
		(LPCWSTR)profile.c_str(),
		arguments.data(),
		static_cast<UINT32>( arguments.size() ),
		defines.empty() ? nullptr : defines.data(),
		static_cast<UINT32>( defines.size() ),
		nullptr,
		&operationResult );
	if ( FAILED( hr ) ) {
		throw VulkanExc( "hlsl compile error: source compile failed" );
	}

	HRESULT status = S_OK;
	hr			   = operationResult->GetStatus( &status );
	if ( FAILED( hr ) || FAILED( status ) ) {
		std::string				 errorMsg;
		ComPtr<IDxcBlobEncoding> errors;
		hr = operationResult->GetErrorBuffer( &errors );
		if ( SUCCEEDED( hr ) ) {
			BOOL   known	= FALSE;
			UINT32 codePage = 0;

			hr = errors->GetEncoding( &known, &codePage );
			if ( SUCCEEDED( hr ) ) {
				SIZE_T n = errors->GetBufferSize();
				if ( n > 0 ) {
					errorMsg = std::string( (const char *)errors->GetBufferPointer(), n );
				}
			}
		}
		std::stringstream ss;
		ss << "hlsl compile error: " << errorMsg;
		throw VulkanExc( ss.str() );
	}

	ComPtr<IDxcBlob> result;
	hr = operationResult->GetResult( &result );
	if ( FAILED( hr ) ) {
		throw VulkanExc( "hlsl compile error: failed to get compile result" );
	}

	SIZE_T sizeInBytes = result->GetBufferSize();
	if ( sizeInBytes == 0 ) {
		throw VulkanExc( "hlsl compile error: invalid result size" );
	}

	const char *pSpirvCode = (const char *)result->GetBufferPointer();

	///*
	std::stringstream ss;
	ss << std::setw( 16 ) << std::setfill( '0' ) << std::hex << reinterpret_cast<uintptr_t>( pSpirvCode ) << ".spv";
	std::string fileName = ss.str();

	std::ofstream os( fileName.c_str(), std::ios::binary );
	if ( os.is_open() ) {
		os.write( pSpirvCode, sizeInBytes );
		os.close();

		cinder::app::console() << "Wrote: " << fileName << std::endl;
	}
	//*/

	auto spirv = std::vector<char>( pSpirvCode, pSpirvCode + sizeInBytes );

	return spirv;
}

} // namespace cinder::vk
