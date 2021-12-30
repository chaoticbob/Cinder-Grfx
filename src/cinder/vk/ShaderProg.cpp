#include "cinder/vk/ShaderProg.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Environment.h"
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

namespace cinder::vk {

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

static void loadShaderSource( DataSourceRef dataSource, std::string &sourceTarget )
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
	: vk::DeviceChildObject( device ),
	  mSpirv( pSpirvCode, pSpirvCode + spirvSize )
{
	// Refelection
	{
		mReflection = spv_reflect::ShaderModule( mSpirv.size(), mSpirv.data() );
		if ( mReflection.GetResult() != SPV_REFLECT_RESULT_SUCCESS ) {
			throw VulkanExc( "SPIR-V reflection failed" );
		}

		// clang-format off
		switch (mReflection.GetShaderStage()) {
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

		parseDescriptorBindings();
		parseUniformBlocks();
	}

	// Creat shader module
	VkShaderModuleCreateInfo vkci = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	vkci.pNext					  = nullptr;
	vkci.flags					  = 0;
	vkci.codeSize				  = mSpirv.size();
	vkci.pCode					  = reinterpret_cast<const uint32_t *>( dataPtr( mSpirv ) );

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

void ShaderModule::parseDescriptorBindings()
{
	uint32_t		 count	= 0;
	SpvReflectResult spvres = mReflection.EnumerateDescriptorBindings( &count, nullptr );
	if ( spvres != SPV_REFLECT_RESULT_SUCCESS ) {
		throw VulkanExc( "SPIR-V reflection: enumerate descriptor binding count failed" );
	}

	mSpirvBindings.resize( count );
	spvres = mReflection.EnumerateDescriptorBindings( &count, dataPtr( mSpirvBindings ) );
	if ( spvres != SPV_REFLECT_RESULT_SUCCESS ) {
		throw VulkanExc( "SPIR-V reflection: enumerate descriptor binding failed" );
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

void ShaderModule::parseUniformBlocks()
{
	bool isGlsl = ( mReflection.GetShaderModule().source_language == SpvSourceLanguageGLSL );
	bool isHlsl = ( mReflection.GetShaderModule().source_language == SpvSourceLanguageHLSL );

	for ( size_t i = 0; i < mSpirvBindings.size(); ++i ) {
		const SpvReflectDescriptorBinding *pSpvBinding = mSpirvBindings[i];
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

const char *ShaderModule::getEntryPoint() const
{
	return mReflection.GetEntryPointName();
}

std::vector<vk::VertexAttribute> ShaderModule::getVertexAttributes() const
{
	std::vector<vk::VertexAttribute> attributes;
	if ( mShaderStage == VK_SHADER_STAGE_VERTEX_BIT ) {
		uint32_t count	= 0;
		auto	 spvRes = mReflection.EnumerateInputVariables( &count, nullptr );

		std::vector<SpvReflectInterfaceVariable *> inputVars;
		if ( ( spvRes == SPV_REFLECT_RESULT_SUCCESS ) && ( count > 0 ) ) {
			inputVars.resize( count );
			spvRes = mReflection.EnumerateInputVariables( &count, dataPtr( inputVars ) );
		}

		if ( ( spvRes == SPV_REFLECT_RESULT_SUCCESS ) && !inputVars.empty() ) {
			for ( size_t i = 0; i < inputVars.size(); ++i ) {
				SpvReflectInterfaceVariable *pVar = inputVars[i];

				// Skip invalid locations - there are usually system input variables
				// that's inaccessible to the shader anyway.
				if ( pVar->location == UINT32_MAX ) {
					continue;
				}

				std::string	 name	  = pVar->name;
				uint32_t	 location = pVar->location;
				VkFormat	 format	  = static_cast<VkFormat>( pVar->format );
				geom::Attrib semantic = geom::Attrib::USER_DEFINED;

				auto it = sDefaultAttribNameToSemanticMap.find( name );
				if ( it != sDefaultAttribNameToSemanticMap.end() ) {
					semantic = it->second;
				}

				vk::VertexAttribute attr = vk::VertexAttribute( name, location, format, semantic );
				attributes.push_back( attr );
			}
		}
	}
	return attributes;
}

std::vector<vk::DescriptorBinding> ShaderModule::getDescriptorBindings() const
{
	std::vector<DescriptorBinding> bindings;
	for ( size_t i = 0; i < mSpirvBindings.size(); ++i ) {
		const SpvReflectDescriptorBinding *pSpvBinding = mSpirvBindings[i];

		vk::DescriptorBinding binding = {};

		// clang-format off
		switch ( pSpvBinding->descriptor_type ) {
			default: {
				throw VulkanExc( "SPIR-V reflection: unrecognized descriptor type" );
			} break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER                    : binding.type = VK_DESCRIPTOR_TYPE_SAMPLER                   ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER     : binding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER    ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE              : binding.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE             ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE              : binding.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE             ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER       : binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER      ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER       : binding.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER      ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER             : binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER            ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER             : binding.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER            ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC     : binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC    ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC     : binding.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC    ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT           : binding.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT          ; break;
			case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR : binding.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR; break;
		}
		// clang-format on

		binding.binding = pSpvBinding->binding;
		binding.set		= pSpvBinding->set;
		binding.name	= pSpvBinding->name;

		// Set binding name to block name if name is empty and is default uniform  block
		if ( ( binding.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ) && binding.name.empty() ) {
			std::string blockName = spvReflectBlockVariableTypeName( &pSpvBinding->block );
			if ( blockName == CI_VK_DEFAULT_UNIFORM_BLOCK_NAME ) {
				binding.name = blockName;
			}
		}

		bindings.push_back( binding );
	}

	return bindings;
}

std::vector<const vk::UniformBlock *> ShaderModule::getUniformBlocks() const
{
	std::vector<const vk::UniformBlock *> blocks;
	for ( const auto &block : mUniformBlocks ) {
		blocks.push_back( block.get() );
	}
	return blocks;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ShaderProg

vk::ShaderProgRef ShaderProg::create( vk::DeviceRef device, const Format &format )
{
	return ShaderProgRef( new ShaderProg( device, format ) );
}

ShaderProgRef ShaderProg::create(
	vk::ShaderModuleRef vertexShader,
	vk::ShaderModuleRef fragmentShader,
	vk::ShaderModuleRef geometryShader,
	vk::ShaderModuleRef tessEvalShader,
	vk::ShaderModuleRef tessCtrlShader )
{
	Format format = Format().vertex( vertexShader );
	if ( fragmentShader ) {
		format.fragment( fragmentShader );
	}
	if ( geometryShader ) {
		format.geometry( geometryShader );
	}
	if ( tessEvalShader ) {
		format.tessellationEval( tessEvalShader );
	}
	if ( tessCtrlShader ) {
		format.tessellationCtrl( tessCtrlShader );
	}

	return ShaderProg::create( vertexShader->getDevice(), format );
}

ShaderProgRef ShaderProg::create(
	vk::DeviceRef device,
	DataSourceRef vertexDataSource,
	DataSourceRef fragmentDataSource,
	DataSourceRef geometryDataSource,
	DataSourceRef tessEvalDataSource,
	DataSourceRef tessCtrlDataSource )
{
	Format format = Format().vertex( vk::ShaderModule::create( vertexDataSource, device ) );
	if ( fragmentDataSource ) {
		format.fragment( vk::ShaderModule::create( fragmentDataSource, device ) );
	}
	if ( geometryDataSource ) {
		format.geometry( vk::ShaderModule::create( geometryDataSource, device ) );
	}
	if ( tessEvalDataSource ) {
		format.tessellationEval( vk::ShaderModule::create( tessEvalDataSource, device ) );
	}
	if ( tessCtrlDataSource ) {
		format.tessellationCtrl( vk::ShaderModule::create( tessCtrlDataSource, device ) );
	}

	return ShaderProg::create( device, format );
}

ShaderProg::ShaderProg( vk::DeviceRef device, const Format &format )
	: mVS( format.mVertexShader ),
	  mPS( format.mPixelShader ),
	  mGS( format.mGeometryShader ),
	  mHS( format.mHullShader ),
	  mDS( format.mDomainShader ),
	  mCS( format.mComputeShader )
{
	if ( !mVS && format.mVertexDataSource ) {
		mVS = vk::ShaderModule::create( format.mVertexDataSource, device );
	}
	if ( !mPS && format.mPixelDataSource ) {
		mPS = vk::ShaderModule::create( format.mPixelDataSource, device );
	}
	if ( !mGS && format.mGeometryDataSource ) {
		mGS = vk::ShaderModule::create( format.mGeometryDataSource, device );
	}
	if ( !mHS && format.mHullDataSource ) {
		mHS = vk::ShaderModule::create( format.mHullDataSource, device );
	}
	if ( !mDS && format.mDomainDataSource ) {
		mDS = vk::ShaderModule::create( format.mDomainDataSource, device );
	}
	if ( !mCS && format.mComputeDataSource ) {
		mCS = vk::ShaderModule::create( format.mComputeDataSource, device );
	}

	if ( mVS ) {
		mVertexAttributes = mVS->getVertexAttributes();
	}

	parseDscriptorBindings( mVS.get() );
	parseDscriptorBindings( mPS.get() );
	parseDscriptorBindings( mGS.get() );
	parseDscriptorBindings( mHS.get() );
	parseDscriptorBindings( mDS.get() );
	parseDscriptorBindings( mCS.get() );

	parseUniformBlocks( mVS.get() );
	parseUniformBlocks( mPS.get() );
	parseUniformBlocks( mGS.get() );
	parseUniformBlocks( mHS.get() );
	parseUniformBlocks( mDS.get() );
	parseUniformBlocks( mCS.get() );
}

ShaderProg::~ShaderProg()
{
}

void ShaderProg::parseDscriptorBindings( const vk::ShaderModule *shader )
{
	if ( shader == nullptr ) {
		return;
	}

	auto bindings = shader->getDescriptorBindings();

	for ( const auto &binding : bindings ) {
		// Get reference to bindings vector
		auto &bindings = mDescriptorSetBindings[binding.set];
		// Look binding using binding number
		auto it = std::find_if(
			bindings.begin(),
			bindings.end(),
			[binding]( const DescriptorBinding &elem ) -> bool {
				bool isMatchBinding = ( elem.binding == binding.binding );
				return isMatchBinding;
			} );
		// Add if the binding if not match is found
		if ( it == bindings.end() ) {
			bindings.push_back( binding );
		}
		// Types must match if binding exists
		else {
			if ( binding.type != it->type ) {
				std::stringstream ss;
				ss << "descriptor type mismatch where binding=" << binding.binding << ", set=" << binding.set;
				throw VulkanExc( ss.str() );
			}
		}
	}
}

void ShaderProg::parseUniformBlocks( const vk::ShaderModule *shader )
{
	if ( shader == nullptr ) {
		return;
	}

	auto blocks = shader->getUniformBlocks();
	for ( const auto &block : blocks ) {
		if ( ( block->getName() == CI_VK_DEFAULT_UNIFORM_BLOCK_NAME ) || ( block->getName() == CI_VK_HLSL_GLOBALS_NAME ) ) {
			// Add default uniform blcok if we don't have one...
			if ( mDefaultUniformBlock == nullptr ) {
				auto defaultUniformBlock = std::make_unique<UniformBlock>( *block );
				mDefaultUniformBlock	 = defaultUniformBlock.get();

				mUniformBlocks.push_back( std::move( defaultUniformBlock ) );
			}
			// ...otherwise validate that default uniform blocks have the same size and uniforms.
			else {
				auto it = std::find_if(
					mUniformBlocks.begin(),
					mUniformBlocks.end(),
					[]( const std::unique_ptr<UniformBlock> &elem ) -> bool {
						bool res = ( elem->getName() == CI_VK_DEFAULT_UNIFORM_BLOCK_NAME );
						return res;
					} );

				if ( it != mUniformBlocks.end() ) {
					if ( block->getSize() != ( *it )->getSize() ) {
						std::stringstream ss;
						ss << "default uniform blocks at binding=" << block->getBinding() << ", set=" << block->getSet();
						ss << " and binding=" << ( *it )->getBinding() << ", set=" << ( *it )->getSet();
						ss << " do not have the same size";
						throw VulkanExc( ss.str() );
					}

					if ( block->getUniforms() != ( *it )->getUniforms() ) {
						std::stringstream ss;
						ss << "default uniform blocks at binding=" << block->getBinding() << ", set=" << block->getSet();
						ss << " and binding=" << ( *it )->getBinding() << ", set=" << ( *it )->getSet();
						ss << " do not have the same uniforms";
						throw VulkanExc( ss.str() );
					}
				}
			}
		}
		else {
			// Uniform blocks with the same binding/set must have same size and uniforms.
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
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GlslProg

GlslProgRef GlslProg::create(
	DataSourceRef vertexShader,
	DataSourceRef fragmentShader,
	DataSourceRef geometryShader,
	DataSourceRef tessEvalShader,
	DataSourceRef tessCtrlShader )
{
	vk::DeviceRef device = app::RendererVk::getCurrentRenderer()->getDevice();
	return create( device, vertexShader, fragmentShader, geometryShader, tessEvalShader, tessCtrlShader );
}

GlslProgRef GlslProg::create(
	vk::DeviceRef device,
	DataSourceRef vertexShader,
	DataSourceRef fragmentShader,
	DataSourceRef geometryShader,
	DataSourceRef tessEvalShader,
	DataSourceRef tessCtrlShader )
{
	std::string vertexSource;
	std::string fragmentSource;
	std::string geometrySource;
	std::string tessEvalSource;
	std::string tessCtrlSource;

	loadShaderSource( vertexShader, vertexSource );
	loadShaderSource( fragmentShader, fragmentSource );
	loadShaderSource( geometryShader, geometrySource );
	loadShaderSource( tessEvalShader, tessEvalSource );
	loadShaderSource( tessCtrlShader, tessCtrlSource );

	return create( device, vertexSource, fragmentSource, geometrySource, tessEvalSource, tessCtrlSource );
}

GlslProgRef GlslProg::create(
	const std::string &vertexShader,
	const std::string &fragmentShader,
	const std::string &geometryShader,
	const std::string &tessEvalShader,
	const std::string &tessCtrlShader )
{
	vk::DeviceRef device = app::RendererVk::getCurrentRenderer()->getDevice();
	return create( device, vertexShader, fragmentShader, geometryShader, tessEvalShader, tessCtrlShader );
}

GlslProgRef GlslProg::create(
	vk::DeviceRef	   device,
	const std::string &vertexShader,
	const std::string &fragmentShader,
	const std::string &geometryShader,
	const std::string &tessEvalShader,
	const std::string &tessCtrlShader )
{
	vk::ShaderModuleRef vs = compileShader( device, vertexShader, VK_SHADER_STAGE_VERTEX_BIT );
	vk::ShaderModuleRef ps = compileShader( device, fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT );
	vk::ShaderModuleRef gs = compileShader( device, geometryShader, VK_SHADER_STAGE_GEOMETRY_BIT );
	vk::ShaderModuleRef ds = compileShader( device, tessEvalShader, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT );
	vk::ShaderModuleRef hs = compileShader( device, tessCtrlShader, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT );

	vk::ShaderProg::Format format = ShaderProg::Format();
	format.vertex( vs );
	format.fragment( ps );
	format.geometry( gs );
	format.tessellationEval( ds );
	format.tessellationCtrl( hs );

	return GlslProgRef( new GlslProg( device, format ) );
}

GlslProg::GlslProg( vk::DeviceRef device, const ShaderProg::Format &format )
	: vk::ShaderProg( device, format )
{
}

GlslProg::~GlslProg()
{
}

/*
void GlslProg::loadShader( DataSourceRef dataSource, std::string &sourceTarget )
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
*/

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

vk::ShaderModuleRef GlslProg::compileShader( vk::DeviceRef device, const std::string &source, VkShaderStageFlagBits stage )
{
	if ( source.empty() ) {
		return vk::ShaderModuleRef();
	}

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
	input.code								= source.c_str();
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
	const char * pSpirvCode	 = reinterpret_cast<const char *>( glslang_program_SPIRV_get_ptr( program ) );

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

	vk::ShaderModuleRef shaderModule = vk::ShaderModule::create( sizeInBytes, pSpirvCode, device );

	glslang_program_delete( program );

	return shaderModule;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HlslProg

HlslProgRef HlslProg::create(
	DataSourceRef vs,
	DataSourceRef ps,
	DataSourceRef gs,
	DataSourceRef ds,
	DataSourceRef hs )
{
	vk::DeviceRef device = app::RendererVk::getCurrentRenderer()->getDevice();
	return create( device, vs, ps, gs, ds, hs );
}

HlslProgRef HlslProg::create(
	vk::DeviceRef device,
	DataSourceRef vs,
	DataSourceRef ps,
	DataSourceRef gs,
	DataSourceRef ds,
	DataSourceRef hs )
{
	std::string vsSource;
	std::string psSource;
	std::string gsSource;
	std::string dsSource;
	std::string hsSource;

	loadShaderSource( vs, vsSource );
	loadShaderSource( ps, psSource );
	loadShaderSource( gs, gsSource );
	loadShaderSource( ds, dsSource );
	loadShaderSource( hs, hsSource );

	return create( device, vsSource, psSource, gsSource, dsSource, hsSource );
}

HlslProgRef HlslProg::create(
	const std::string &vsSource,
	const std::string &psSource,
	const std::string &gsSource,
	const std::string &dsSource,
	const std::string &hsSource )
{
	vk::DeviceRef device = app::RendererVk::getCurrentRenderer()->getDevice();
	return create( device, vsSource, psSource, gsSource, dsSource, hsSource );
}

HlslProgRef HlslProg::create(
	vk::DeviceRef	   device,
	const std::string &vsSource,
	const std::string &psSource,
	const std::string &gsSource,
	const std::string &dsSource,
	const std::string &hsSource )
{
	vk::ShaderModuleRef vs = compileShader( device, vsSource, VK_SHADER_STAGE_VERTEX_BIT );
	vk::ShaderModuleRef ps = compileShader( device, psSource, VK_SHADER_STAGE_FRAGMENT_BIT );
	vk::ShaderModuleRef gs = compileShader( device, gsSource, VK_SHADER_STAGE_GEOMETRY_BIT );
	vk::ShaderModuleRef ds = compileShader( device, dsSource, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT );
	vk::ShaderModuleRef hs = compileShader( device, hsSource, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT );

	vk::ShaderProg::Format format = ShaderProg::Format();
	format.vertex( vs );
	format.fragment( ps );
	format.geometry( gs );
	format.tessellationEval( ds );
	format.tessellationCtrl( hs );

	return HlslProgRef( new HlslProg( device, format ) );
}

HlslProg::HlslProg( vk::DeviceRef device, const ShaderProg::Format &format )
	: vk::ShaderProg( device, format )
{
}

HlslProg::~HlslProg()
{
}

vk::ShaderModuleRef HlslProg::compileShader(
	vk::DeviceRef			  device,
	const std::string &		  sourceText,
	VkShaderStageFlagBits	  shaderStage,
	const std::string &		  entryPointName,
	vk::HlslProg::ShaderModel shaderModel )
{
	if ( sourceText.empty() ) {
		return vk::ShaderModuleRef();
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
		(LPVOID)sourceText.c_str(),
		static_cast<UINT32>( sourceText.length() ),
		CP_ACP,
		&source );
	if ( FAILED( hr ) ) {
		throw VulkanExc( "hlsl compile error: create source blob failed" );
	}

	// clang-format off
	std::vector<LPCWSTR> arguments = {
		(LPCWSTR)kSpirvArg.c_str(),
		(LPCWSTR)kSpirvReflectArg.c_str(),
		L"-fvk-t-shift", (LPCWSTR)kShiftTArg.c_str(), L"0",
		L"-fvk-b-shift", (LPCWSTR)kShiftBArg.c_str(), L"0",
		L"-fvk-s-shift", (LPCWSTR)kShiftSArg.c_str(), L"0",
		L"-fvk-u-shift", (LPCWSTR)kShiftUArg.c_str(), L"0",
	};
	// clang-format on

	std::vector<std::pair<std::u16string, std::u16string>> defineNameValues;
	//for ( XgiU32 i = 0; i < numDefines; ++i ) {
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
	//}

	std::vector<DxcDefine> defines;
	//for ( auto &elem : defineNameValues ) {
	//	DxcDefine define = {};
	//	define.Name		 = elem.first.c_str();
	//	define.Value	 = elem.second.empty() ? nullptr : elem.second.c_str();
	//	defines.push_back( define );
	//}

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

	vk::ShaderModuleRef shaderModule = vk::ShaderModule::create( sizeInBytes, pSpirvCode, device );

	return shaderModule;
}

} // namespace cinder::vk
