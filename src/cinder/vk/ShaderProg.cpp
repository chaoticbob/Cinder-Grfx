#include "cinder/vk/ShaderProg.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Environment.h"
#include "cinder/app/AppBase.h"
#include "cinder/app/RendererVk.h"
#include "cinder/Log.h"

#include "glslang/Include/glslang_c_interface.h"
#include "StandAlone/resource_limits_c.h"

namespace cinder::vk {

static std::map<std::string, geom::Attrib> sDefaultAttribNameToSemanticMap = {
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
};

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

const char *ShaderModule::getEntryPoint() const
{
	return mReflection.GetEntryPointName();
}

std::vector<VertexAttribute> ShaderModule::getVertexAttributes() const
{
	std::vector<VertexAttribute> attributes;
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
				// that's inaccessible to the shader.
				if (pVar->location == UINT32_MAX) {
					continue;
				}

				VertexAttribute				 attr = {};
				attr.name						  = pVar->name;
				attr.location					  = pVar->location;
				attr.format						  = static_cast<VkFormat>( pVar->format );
				attr.semantic					  = geom::Attrib::USER_DEFINED;

				auto it = sDefaultAttribNameToSemanticMap.find( attr.name );
				if ( it != sDefaultAttribNameToSemanticMap.end() ) {
					attr.semantic = it->second;
				}

				attributes.push_back(attr);
			}
		}
	}
	return attributes;
}

std::vector<DescriptorBinding> ShaderModule::getDescriptorBindings() const
{
	uint32_t		 count	= 0;
	SpvReflectResult spvres = mReflection.EnumerateDescriptorBindings( &count, nullptr );
	if ( spvres != SPV_REFLECT_RESULT_SUCCESS ) {
		throw VulkanExc( "SPIR-V reflection: enumerate descriptor binding count failed" );
	}

	std::vector<SpvReflectDescriptorBinding *> spvBindings( count );
	spvres = mReflection.EnumerateDescriptorBindings( &count, dataPtr( spvBindings ) );
	if ( spvres != SPV_REFLECT_RESULT_SUCCESS ) {
		throw VulkanExc( "SPIR-V reflection: enumerate descriptor binding failed" );
	}

	std::vector<DescriptorBinding> bindings;
	for ( uint32_t i = 0; i < count; ++i ) {
		const SpvReflectDescriptorBinding *pSpvBinding = spvBindings[i];

		DescriptorBinding binding = {};

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

		bindings.push_back( binding );
	}

	return bindings;
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

	addDescriptorBindings( mVS );
	addDescriptorBindings( mPS );
	addDescriptorBindings( mGS );
	addDescriptorBindings( mHS );
	addDescriptorBindings( mDS );
	addDescriptorBindings( mCS );
}

ShaderProg::~ShaderProg()
{
}

void ShaderProg::addDescriptorBindings( vk::ShaderModuleRef shader )
{
	if ( !shader ) {
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

	loadShader( vertexShader, vertexSource );
	loadShader( fragmentShader, fragmentSource );
	loadShader( geometryShader, geometrySource );
	loadShader( tessEvalShader, tessEvalSource );
	loadShader( tessCtrlShader, tessCtrlSource );

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
	glslang_shader_shift_binding( shader, GLSLANG_RESOURCE_TYPE_SAMPLER, CINDER_CONTEXT_BINDING_SHIFT_SAMPLER );
	glslang_shader_shift_binding( shader, GLSLANG_RESOURCE_TYPE_TEXTURE, CINDER_CONTEXT_BINDING_SHIFT_IMAGE );
	glslang_shader_shift_binding( shader, GLSLANG_RESOURCE_TYPE_IMAGE, CINDER_CONTEXT_BINDING_SHIFT_TEXTURE );
	glslang_shader_shift_binding( shader, GLSLANG_RESOURCE_TYPE_UBO, CINDER_CONTEXT_BINDING_SHIFT_UBO );
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

} // namespace cinder::vk
