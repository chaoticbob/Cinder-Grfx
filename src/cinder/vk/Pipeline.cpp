#include "cinder/vk/Pipeline.h"
#include "cinder/vk/Descriptor.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/ShaderProg.h"
#include "cinder/vk/Util.h"
#include "cinder/app/RendererVk.h"

#include "xxh3.h"

namespace cinder::vk {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PipelineLayout

PipelineLayout::Options &PipelineLayout::Options::addSetLayout( const vk::DescriptorSetLayoutRef &layout )
{
	addSetLayout( layout->getDescriptorSetLayoutHandle() );
	return *this;
}

PipelineLayoutRef PipelineLayout::create( const Options &options, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return PipelineLayoutRef( new PipelineLayout( device, options ) );
}

PipelineLayout::PipelineLayout( vk::DeviceRef device, const Options &options )
	: vk::DeviceChildObject( device )
{
	VkPipelineLayoutCreateInfo vkci = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	vkci.pNext						= nullptr;
	vkci.flags						= 0;
	vkci.setLayoutCount				= countU32( options.mSetLayouts );
	vkci.pSetLayouts				= dataPtr( options.mSetLayouts );
	vkci.pushConstantRangeCount		= countU32( options.mPushConstantRanges );
	vkci.pPushConstantRanges		= dataPtr( options.mPushConstantRanges );

	// VkResult vkres = vkCreatePipelineLayout(
	VkResult vkres = CI_VK_DEVICE_FN( CreatePipelineLayout(
		getDeviceHandle(),
		&vkci,
		nullptr,
		&mPipelineLayoutHandle ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkCreatePipelineLayout", vkres );
	}
}

PipelineLayout::~PipelineLayout()
{
	if ( mPipelineLayoutHandle != VK_NULL_HANDLE ) {
		CI_VK_DEVICE_FN( DestroyPipelineLayout(
			getDeviceHandle(),
			mPipelineLayoutHandle,
			nullptr ) );
		mPipelineLayoutHandle = VK_NULL_HANDLE;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::Options

// Pipeline::Options::Options( const GraphicsPipelineState &gs )
//{
//	memcpy( &mGraphicsState, &gs, sizeof( gs ) );
// }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline

/*
Pipeline::Options &Pipeline::Options::attributeFormat( uint32_t index, VkFormat format )
{
	if ( index >= CINDER_MAX_VERTEX_INPUTS ) {
		throw VulkanExc( "attribute index exceeds CINDER_MAX_VERTEX_INPUTS" );
	}
	mVertexAttributes[index].format( format );
	return *this;
}

Pipeline::Options &Pipeline::Options::attributeLocation( uint32_t index, uint32_t location )
{
	if ( index >= CINDER_MAX_VERTEX_INPUTS ) {
		throw VulkanExc( "attribute index exceeds CINDER_MAX_VERTEX_INPUTS" );
	}
	mVertexAttributes[index].location( location );
	return *this;
}

Pipeline::Options &Pipeline::Options::attributeBinding( uint32_t index, uint32_t binding )
{
	if ( index >= CINDER_MAX_VERTEX_INPUTS ) {
		throw VulkanExc( "attribute index exceeds CINDER_MAX_VERTEX_INPUTS" );
	}
	mVertexAttributes[index].binding( binding );
	return *this;
}

Pipeline::Options &Pipeline::Options::attribute( uint32_t index, const Attribute &attr )
{
	if ( index >= CINDER_MAX_VERTEX_INPUTS ) {
		throw VulkanExc( "attribute index exceeds CINDER_MAX_VERTEX_INPUTS" );
	}
	mVertexAttributes[index] = attr;
	return *this;
}

Pipeline::Options &Pipeline::Options::addRenderTarget( VkFormat format )
{
	if ( mOutputState.renderTargetCount < CINDER_MAX_RENDER_TARGETS ) {
		mOutputState.renderTargets[mOutputState.renderTargetCount] = format;
		++mOutputState.renderTargetCount;
	}
	return *this;
}
*/

void Pipeline::setDefaults( vk::Pipeline::GraphicsPipelineCreateInfo *createInfo )
{
	memset( createInfo, 0, sizeof( *createInfo ) );

	// Input Assembly
	createInfo->ia.attributeCount	  = 0;
	createInfo->ia.topology			  = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	createInfo->ia.primitiveRestart	  = VK_FALSE;
	createInfo->ia.domainOrigin		  = VK_TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT;
	createInfo->ia.patchControlPoints = 0;

	// Rasterization
	createInfo->rs.polygonMode			   = VK_POLYGON_MODE_FILL;
	createInfo->rs.cullMode				   = VK_CULL_MODE_NONE;
	createInfo->rs.frontFace			   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	createInfo->rs.rasterizerDiscardEnable = VK_FALSE;
	createInfo->rs.depthClipEnable		   = VK_FALSE;
	createInfo->rs.depthClampEnable		   = VK_FALSE;
	createInfo->rs.depthBiasEnable		   = VK_FALSE;

	// Rasterization:Multi-sample
	createInfo->rs.rasterizationSamples	 = VK_SAMPLE_COUNT_1_BIT;
	createInfo->rs.sampleShading		 = SAMPLE_SHADING_OFF;
	createInfo->rs.alphaToCoverageEnable = VK_FALSE;
	createInfo->rs.alphaToOneEnable		 = VK_FALSE;

	// Depth Stencil
	createInfo->ds.depthTestEnable		 = VK_FALSE;
	createInfo->ds.depthWriteEnable		 = VK_FALSE;
	createInfo->ds.depthCompareOp		 = VK_COMPARE_OP_NEVER;
	createInfo->ds.depthBoundsTestEnable = VK_FALSE;
	createInfo->ds.stencilTestEnable	 = VK_FALSE;
	createInfo->ds.stencilReadMask		 = CINDER_DEFAULT_STENCIL_READ_MASK;
	createInfo->ds.stencilWriteMask		 = CINDER_DEFAULT_STENCIL_WRITE_MASK;
	createInfo->ds.stencilReference		 = CINDER_DEFAULT_STENCIL_REFERENCE;
	createInfo->ds.front.failOp			 = VK_STENCIL_OP_KEEP;
	createInfo->ds.front.passOp			 = VK_STENCIL_OP_KEEP;
	createInfo->ds.front.depthFailOp	 = VK_STENCIL_OP_KEEP;
	createInfo->ds.front.compareOp		 = VK_COMPARE_OP_NEVER;
	createInfo->ds.back.failOp			 = VK_STENCIL_OP_KEEP;
	createInfo->ds.back.passOp			 = VK_STENCIL_OP_KEEP;
	createInfo->ds.back.depthFailOp		 = VK_STENCIL_OP_KEEP;
	createInfo->ds.back.compareOp		 = VK_COMPARE_OP_NEVER;

	// Color Blend
	createInfo->cb.logicOpEnable   = VK_FALSE;
	createInfo->cb.logicOp		   = VK_LOGIC_OP_CLEAR;
	createInfo->cb.attachmentCount = 0;
	for ( uint32_t i = 0; i < CINDER_MAX_RENDER_TARGETS; ++i ) {
		createInfo->cb.attachments[i].blendEnable		  = VK_FALSE;
		createInfo->cb.attachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		createInfo->cb.attachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		createInfo->cb.attachments[i].colorBlendOp		  = VK_BLEND_OP_ADD;
		createInfo->cb.attachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		createInfo->cb.attachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		createInfo->cb.attachments[i].alphaBlendOp		  = VK_BLEND_OP_ADD;
		createInfo->cb.attachments[i].colorWriteMask	  = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}

	// Output Merger
	// 
	// NOTE: the memset above will set createInfo.om to all zeros.
	//
}

vk::PipelineRef Pipeline::create( const vk::Pipeline::GraphicsPipelineCreateInfo &createInfo, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return vk::PipelineRef( new vk::Pipeline( device, createInfo ) );
}

uint64_t Pipeline::calculateHash( const vk::Pipeline::GraphicsPipelineCreateInfo* createInfo )
{
	XXH64_hash_t hash = XXH64( createInfo, sizeof( createInfo ), 0xF33DC0D3 );
	return static_cast<uint64_t>( hash );
}

Pipeline::Pipeline( vk::DeviceRef device, const GraphicsPipelineCreateInfo &createInfo )
	: vk::DeviceChildObject( device )
{
	initGraphicsPipeline( createInfo );
}

Pipeline::~Pipeline()
{
	if ( mPipelineHandle ) {
		// vkDestroyPipeline(
		getDevice()->vkfn()->DestroyPipeline(
			getDeviceHandle(),
			mPipelineHandle,
			nullptr );
		mPipelineHandle = VK_NULL_HANDLE;
	}
}

void Pipeline::initShaderStages(
	const vk::Pipeline::GraphicsPipelineCreateInfo &createInfo,
	std::vector<VkPipelineShaderStageCreateInfo>	 &shaderStages )
{
	// VS
	auto shaderModule = createInfo.vert;
	if ( shaderModule ) {
		VkPipelineShaderStageCreateInfo ssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ssci.flags							 = 0;
		ssci.pSpecializationInfo			 = nullptr;
		ssci.pName							 = shaderModule->getEntryPoint().c_str();
		ssci.stage							 = VK_SHADER_STAGE_VERTEX_BIT;
		ssci.module							 = shaderModule->getShaderModuleHandle();
		shaderStages.push_back( ssci );
	}

	// PS
	shaderModule = createInfo.frag;
	if ( shaderModule ) {
		VkPipelineShaderStageCreateInfo ssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ssci.flags							 = 0;
		ssci.pSpecializationInfo			 = nullptr;
		ssci.pName							 = shaderModule->getEntryPoint().c_str();
		ssci.stage							 = VK_SHADER_STAGE_FRAGMENT_BIT;
		ssci.module							 = shaderModule->getShaderModuleHandle();
		shaderStages.push_back( ssci );
	}

	// HS
	shaderModule = createInfo.tesc;
	if ( shaderModule ) {
		VkPipelineShaderStageCreateInfo ssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ssci.flags							 = 0;
		ssci.pSpecializationInfo			 = nullptr;
		ssci.pName							 = shaderModule->getEntryPoint().c_str();
		ssci.stage							 = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		ssci.module							 = shaderModule->getShaderModuleHandle();
		shaderStages.push_back( ssci );
	}

	// DS
	shaderModule = createInfo.tese;
	if ( shaderModule ) {
		VkPipelineShaderStageCreateInfo ssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ssci.flags							 = 0;
		ssci.pSpecializationInfo			 = nullptr;
		ssci.pName							 = shaderModule->getEntryPoint().c_str();
		ssci.stage							 = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		ssci.module							 = shaderModule->getShaderModuleHandle();
		shaderStages.push_back( ssci );
	}

	// GS
	shaderModule = createInfo.geom;
	if ( shaderModule ) {
		VkPipelineShaderStageCreateInfo ssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ssci.flags							 = 0;
		ssci.pSpecializationInfo			 = nullptr;
		ssci.pName							 = shaderModule->getEntryPoint().c_str();
		ssci.stage							 = VK_SHADER_STAGE_GEOMETRY_BIT;
		ssci.module							 = shaderModule->getShaderModuleHandle();
		shaderStages.push_back( ssci );
	}
}

void Pipeline::initVertexInput(
	const vk::Pipeline::GraphicsPipelineCreateInfo &createInfo,
	std::vector<VkVertexInputAttributeDescription> &vertexAttributes,
	std::vector<VkVertexInputBindingDescription>	 &vertexBindings,
	VkPipelineVertexInputStateCreateInfo			 &vertexInputStateCreateInfo )
{
	// Populate attribute and bindings
	for ( uint32_t i = 0; i < createInfo.ia.attributeCount; ++i ) {
		const auto	   &attr		= createInfo.ia.attributes[i];
		uint32_t		  location	= attr.getLocation();
		uint32_t		  binding	= attr.getBinding();
		VkVertexInputRate inputRate = attr.getInputRate();

		if ( location == CINDER_APPEND_VERTEX_LOCATION ) {
			if ( i > 0 ) {
				location = vertexAttributes[i - 1].location;
			}
			else {
				location = 0;
			}
		}

		VkVertexInputAttributeDescription attrDesc = {};
		attrDesc.location						   = location;
		attrDesc.binding						   = binding;
		attrDesc.format							   = attr.getFormat();
		attrDesc.offset							   = attr.getOffset();
		vertexAttributes.push_back( attrDesc );

		auto it = std::find_if(
			vertexBindings.begin(),
			vertexBindings.end(),
			[binding]( const VkVertexInputBindingDescription &elem ) -> bool {
				return elem.binding == binding;
			} );
		if ( it == vertexBindings.end() ) {
			VkVertexInputBindingDescription bindingDesc = {};
			bindingDesc.binding							= binding;
			bindingDesc.stride							= 0;
			bindingDesc.inputRate						= attr.getInputRate();
			vertexBindings.push_back( bindingDesc );
		}
		else {
			if ( it->inputRate != inputRate ) {
				throw VulkanExc( "vertex attribute input rate conflicts with vertex binding input rate" );
			}
		}
	}

	// Calculate offset
	for ( size_t j = 0; j < vertexBindings.size(); ++j ) {
		VkVertexInputBindingDescription &binding	   = vertexBindings[j];
		size_t							 prevAttrIndex = UINT32_MAX;
		for ( size_t i = 0; i < vertexAttributes.size(); ++i ) {
			VkVertexInputAttributeDescription &attr = vertexAttributes[i];
			if ( attr.binding != binding.binding ) {
				continue;
			}

			if ( attr.offset == CINDER_APPEND_VERTEX_OFFSET ) {
				attr.offset = ( prevAttrIndex == UINT32_MAX ) ? 0 : binding.stride;
			}

			uint32_t size  = formatSize( attr.format );
			binding.stride = attr.offset + size;

			prevAttrIndex = static_cast<uint32_t>( i );
		}
	}

	vertexInputStateCreateInfo.flags						   = 0;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount   = countU32( vertexBindings );
	vertexInputStateCreateInfo.pVertexBindingDescriptions	   = dataPtr( vertexBindings );
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = countU32( vertexAttributes );
	vertexInputStateCreateInfo.pVertexAttributeDescriptions	   = dataPtr( vertexAttributes );
}

void Pipeline::initPrimitiveTopology(
	const vk::Pipeline::GraphicsPipelineCreateInfo	   &createInfo,
	VkPipelineInputAssemblyStateCreateInfo			   &inputAssemblyStateCreateInfo,
	VkPipelineTessellationDomainOriginStateCreateInfoKHR &domainOriginStateCreateInfo,
	VkPipelineTessellationStateCreateInfo				  &tessellationStateCreateInfo )
{
	inputAssemblyStateCreateInfo.flags					= 0;
	inputAssemblyStateCreateInfo.topology				= createInfo.ia.topology;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = createInfo.ia.primitiveRestart ? VK_TRUE : VK_FALSE;

	domainOriginStateCreateInfo.domainOrigin = createInfo.ia.domainOrigin;

	tessellationStateCreateInfo.flags			   = 0;
	tessellationStateCreateInfo.patchControlPoints = createInfo.ia.patchControlPoints;
}

void Pipeline::initRasterizer(
	const vk::Pipeline::GraphicsPipelineCreateInfo	   &createInfo,
	VkPipelineRasterizationDepthClipStateCreateInfoEXT &depthClipStateCreateInfo,
	VkPipelineRasterizationStateCreateInfo			   &rasterizationStateCreateInfo,
	VkPipelineMultisampleStateCreateInfo				 &multisampleStateCreateInfo )
{
	rasterizationStateCreateInfo.flags					 = 0;
	rasterizationStateCreateInfo.depthClampEnable		 = createInfo.rs.depthClampEnable;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = createInfo.rs.rasterizerDiscardEnable;
	rasterizationStateCreateInfo.polygonMode			 = createInfo.rs.polygonMode;
	rasterizationStateCreateInfo.cullMode				 = createInfo.rs.cullMode;
	rasterizationStateCreateInfo.frontFace				 = createInfo.rs.frontFace;
	rasterizationStateCreateInfo.depthBiasEnable		 = createInfo.rs.depthBiasEnable;
	rasterizationStateCreateInfo.depthBiasConstantFactor = CINDER_DEFAULT_DEPTH_BIAS;
	rasterizationStateCreateInfo.depthBiasClamp			 = CINDER_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizationStateCreateInfo.depthBiasSlopeFactor	 = CINDER_DEFAULT_DEPTH_BIAS_SCALE;
	rasterizationStateCreateInfo.lineWidth				 = CINDER_DEFAULT_LINE_WIDTH;

	depthClipStateCreateInfo.flags			 = 0;
	depthClipStateCreateInfo.depthClipEnable = createInfo.rs.depthClipEnable;

	multisampleStateCreateInfo.flags				 = 0;
	multisampleStateCreateInfo.rasterizationSamples	 = createInfo.rs.rasterizationSamples;
	multisampleStateCreateInfo.sampleShadingEnable	 = ( createInfo.rs.sampleShading != SAMPLE_SHADING_OFF ) ? VK_TRUE : VK_FALSE;
	multisampleStateCreateInfo.minSampleShading		 = ( createInfo.rs.sampleShading != SAMPLE_SHADING_OFF ) ? ( static_cast<float>( createInfo.rs.sampleShading ) / 10.0f ) : 0.0f;
	multisampleStateCreateInfo.pSampleMask			 = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = createInfo.rs.alphaToCoverageEnable;
	multisampleStateCreateInfo.alphaToOneEnable		 = createInfo.rs.alphaToOneEnable;
}

void Pipeline::initDepthStencil(
	const vk::Pipeline::GraphicsPipelineCreateInfo &createInfo,
	VkPipelineDepthStencilStateCreateInfo		  &depthStencilStateCreateInfo )
{
	depthStencilStateCreateInfo.flags				  = 0;
	depthStencilStateCreateInfo.depthTestEnable		  = createInfo.ds.depthTestEnable;
	depthStencilStateCreateInfo.depthWriteEnable	  = createInfo.ds.depthWriteEnable;
	depthStencilStateCreateInfo.depthCompareOp		  = createInfo.ds.depthCompareOp;
	depthStencilStateCreateInfo.depthBoundsTestEnable = createInfo.ds.depthBoundsTestEnable;
	depthStencilStateCreateInfo.stencilTestEnable	  = createInfo.ds.stencilTestEnable;
	depthStencilStateCreateInfo.front.failOp		  = createInfo.ds.front.failOp;
	depthStencilStateCreateInfo.front.passOp		  = createInfo.ds.front.passOp;
	depthStencilStateCreateInfo.front.depthFailOp	  = createInfo.ds.front.depthFailOp;
	depthStencilStateCreateInfo.front.compareOp		  = createInfo.ds.front.compareOp;
	depthStencilStateCreateInfo.front.compareMask	  = createInfo.ds.stencilReadMask;
	depthStencilStateCreateInfo.front.writeMask		  = createInfo.ds.stencilWriteMask;
	depthStencilStateCreateInfo.front.reference		  = createInfo.ds.stencilReference;
	depthStencilStateCreateInfo.back.failOp			  = createInfo.ds.back.failOp;
	depthStencilStateCreateInfo.back.passOp			  = createInfo.ds.back.passOp;
	depthStencilStateCreateInfo.back.depthFailOp	  = createInfo.ds.back.depthFailOp;
	depthStencilStateCreateInfo.back.compareOp		  = createInfo.ds.back.compareOp;
	depthStencilStateCreateInfo.back.compareMask	  = createInfo.ds.stencilReadMask;
	depthStencilStateCreateInfo.back.writeMask		  = createInfo.ds.stencilWriteMask;
	depthStencilStateCreateInfo.back.reference		  = createInfo.ds.stencilReference;
	depthStencilStateCreateInfo.minDepthBounds		  = CINDER_DEFAULT_VIEWPORT_MIN_DEPTH;
	depthStencilStateCreateInfo.maxDepthBounds		  = CINDER_DEFAULT_VIEWPORT_MAX_DEPTH;
}

void Pipeline::initColorBlend(
	const vk::Pipeline::GraphicsPipelineCreateInfo   &createInfo,
	std::vector<VkPipelineColorBlendAttachmentState> &attachments,
	VkPipelineColorBlendStateCreateInfo				&blendStateCreateInfo )
{
	for ( uint32_t i = 0; i < createInfo.cb.attachmentCount; ++i ) {
		const ColorBlendAttachment &cba = createInfo.cb.attachments[i];

		VkPipelineColorBlendAttachmentState attachment = {};
		attachment.blendEnable						   = cba.blendEnable;
		attachment.srcColorBlendFactor				   = cba.srcColorBlendFactor;
		attachment.dstColorBlendFactor				   = cba.dstColorBlendFactor;
		attachment.colorBlendOp						   = cba.colorBlendOp;
		attachment.srcAlphaBlendFactor				   = cba.srcAlphaBlendFactor;
		attachment.dstAlphaBlendFactor				   = cba.dstAlphaBlendFactor;
		attachment.alphaBlendOp						   = cba.alphaBlendOp;
		attachment.colorWriteMask					   = cba.colorWriteMask;

		attachments.push_back( attachment );
	}

	blendStateCreateInfo.flags			   = 0;
	blendStateCreateInfo.logicOpEnable	   = createInfo.cb.logicOpEnable;
	blendStateCreateInfo.logicOp		   = createInfo.cb.logicOp;
	blendStateCreateInfo.attachmentCount   = countU32( attachments );
	blendStateCreateInfo.pAttachments	   = dataPtr( attachments );
	blendStateCreateInfo.blendConstants[0] = CINDER_DEFAULT_BLEND_R;
	blendStateCreateInfo.blendConstants[1] = CINDER_DEFAULT_BLEND_G;
	blendStateCreateInfo.blendConstants[2] = CINDER_DEFAULT_BLEND_B;
	blendStateCreateInfo.blendConstants[3] = CINDER_DEFAULT_BLEND_A;
}

void Pipeline::initDynamicState(
	const vk::Pipeline::GraphicsPipelineCreateInfo &createInfo,
	std::vector<VkDynamicState>					&dynamicStates,
	VkPipelineDynamicStateCreateInfo				 &dynamcStateCreateInfo )
{
	// Core dynamic states
	dynamicStates.push_back( VK_DYNAMIC_STATE_VIEWPORT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_SCISSOR );
	dynamicStates.push_back( VK_DYNAMIC_STATE_LINE_WIDTH );
	dynamicStates.push_back( VK_DYNAMIC_STATE_DEPTH_BIAS );
	dynamicStates.push_back( VK_DYNAMIC_STATE_BLEND_CONSTANTS );
	dynamicStates.push_back( VK_DYNAMIC_STATE_DEPTH_BOUNDS );
	dynamicStates.push_back( VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK );
	dynamicStates.push_back( VK_DYNAMIC_STATE_STENCIL_WRITE_MASK );
	dynamicStates.push_back( VK_DYNAMIC_STATE_STENCIL_REFERENCE );

	// Provided by VK_EXT_extended_dynamic_state
	dynamicStates.push_back( VK_DYNAMIC_STATE_CULL_MODE_EXT );
	// dynamicStates.push_back( VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE_EXT );
	// dynamicStates.push_back( VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_FRONT_FACE_EXT );
	// dynamicStates.push_back( VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT );
	// dynamicStates.push_back( VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT_EXT );
	// dynamicStates.push_back( VK_DYNAMIC_STATE_STENCIL_OP_EXT );
	// dynamicStates.push_back( VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE_EXT );
	// dynamicStates.push_back( VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE_EXT );
	// dynamicStates.push_back( VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT_EXT );

	/*
		// Provided by VK_EXT_extended_dynamic_state2
		dynamicStates.push_back( VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE_EXT );
		dynamicStates.push_back( VK_DYNAMIC_STATE_LOGIC_OP_EXT );
		dynamicStates.push_back( VK_DYNAMIC_STATE_PATCH_CONTROL_POINTS_EXT );
		dynamicStates.push_back( VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE_EXT );
		dynamicStates.push_back( VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE_EXT );
	*/

	dynamcStateCreateInfo.flags				= 0;
	dynamcStateCreateInfo.dynamicStateCount = countU32( dynamicStates );
	dynamcStateCreateInfo.pDynamicStates	= dataPtr( dynamicStates );
}

void Pipeline::initGraphicsPipeline( const GraphicsPipelineCreateInfo &createInfo )
{
	VkGraphicsPipelineCreateInfo vkci = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

	// Shaders
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	initShaderStages( createInfo, shaderStages );

	// Vertex input
	std::vector<VkVertexInputAttributeDescription> vertexAttributes;
	std::vector<VkVertexInputBindingDescription>   vertexBindings;
	VkPipelineVertexInputStateCreateInfo		   vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	initVertexInput( createInfo, vertexAttributes, vertexBindings, vertexInputState );

	// Input assembly / primitive topology
	VkPipelineInputAssemblyStateCreateInfo				 inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	VkPipelineTessellationDomainOriginStateCreateInfoKHR domainOriginState	= { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO_KHR };
	VkPipelineTessellationStateCreateInfo				 tessellationState	= { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
	initPrimitiveTopology( createInfo, inputAssemblyState, domainOriginState, tessellationState );
	tessellationState.pNext = ( tessellationState.patchControlPoints > 0 ) ? &domainOriginState : nullptr;

	// Viewports
	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.flags								= 0;
	viewportState.viewportCount						= 1;
	viewportState.pViewports						= nullptr;
	viewportState.scissorCount						= 1;
	viewportState.pScissors							= nullptr;

	// Rasterizer / multisample
	VkPipelineRasterizationDepthClipStateCreateInfoEXT depthClipStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT };
	VkPipelineRasterizationStateCreateInfo			   rasterizationState		= { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	VkPipelineMultisampleStateCreateInfo			   multisampleState			= { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	initRasterizer( createInfo, depthClipStateCreateInfo, rasterizationState, multisampleState );
	rasterizationState.pNext = &depthClipStateCreateInfo;

	// Depth/stencil
	VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	initDepthStencil( createInfo, depthStencilState );

	// Color blend
	std::vector<VkPipelineColorBlendAttachmentState> attachments;
	VkPipelineColorBlendStateCreateInfo				 colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	initColorBlend( createInfo, attachments, colorBlendState );

	// Dynamic states
	std::vector<VkDynamicState>		 dynamicStatesArray;
	VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	initDynamicState( createInfo, dynamicStatesArray, dynamicState );

	//// Create temporary render pass
	////
	// VkRenderPassPtr renderPass = VK_NULL_HANDLE;
	//{
	//	std::vector<VkFormat> renderTargetFormats;
	//	for ( uint32_t i = 0; i < pCreateInfo->outputState.renderTargetCount; ++i ) {
	//		renderTargetFormats.push_back( ToVkFormat( pCreateInfo->outputState.renderTargetFormats[i] ) );
	//	}
	//	VkFormat depthStencilFormat = ToVkFormat( pCreateInfo->outputState.depthStencilFormat );
	//
	//	VkResult vkres = vk::CreateTransientRenderPass(
	//		ToApi( GetDevice() )->GetVkDevice(),
	//		CountU32( renderTargetFormats ),
	//		DataPtr( renderTargetFormats ),
	//		depthStencilFormat,
	//		ToVkSampleCount( pCreateInfo->rasterState.rasterizationSamples ),
	//		&renderPass );
	//	if ( vkres != VK_SUCCESS ) {
	//		PPX_ASSERT_MSG( false, "vk::CreateTransientRenderPass failed: " << ToString( vkres ) );
	//		return ppx::ERROR_API_FAILURE;
	//	}
	// }

	VkPipelineRenderingCreateInfoKHR renderingCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
	renderingCreateInfo.colorAttachmentCount			 = createInfo.om.renderTargetCount;
	renderingCreateInfo.pColorAttachmentFormats			 = createInfo.om.renderTargets;
	renderingCreateInfo.depthAttachmentFormat			 = createInfo.om.depthStencil;
	renderingCreateInfo.stencilAttachmentFormat			 = createInfo.om.depthStencil;

	// Fill in pointers nad remaining values
	//
	vkci.pNext				 = &renderingCreateInfo;
	vkci.flags				 = 0;
	vkci.stageCount			 = countU32( shaderStages );
	vkci.pStages			 = dataPtr( shaderStages );
	vkci.pVertexInputState	 = &vertexInputState;
	vkci.pInputAssemblyState = &inputAssemblyState;
	vkci.pTessellationState	 = &tessellationState;
	vkci.pViewportState		 = &viewportState;
	vkci.pRasterizationState = &rasterizationState;
	vkci.pMultisampleState	 = &multisampleState;
	vkci.pDepthStencilState	 = &depthStencilState;
	vkci.pColorBlendState	 = &colorBlendState;
	vkci.pDynamicState		 = &dynamicState;
	vkci.layout				 = createInfo.pipelineLayout->getPipelineLayoutHandle();
	vkci.renderPass			 = VK_NULL_HANDLE;
	vkci.subpass			 = 0; // One subpass to rule them all
	vkci.basePipelineHandle	 = VK_NULL_HANDLE;
	vkci.basePipelineIndex	 = -1;

	VkResult vkres = CI_VK_DEVICE_FN( CreateGraphicsPipelines(
		getDeviceHandle(),
		VK_NULL_HANDLE,
		1,
		&vkci,
		nullptr,
		&mPipelineHandle ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkCreateGraphicsPipelines", vkres );
	}

	// VkResult vkres = vkCreateGraphicsPipelines(
	// VkResult vkres = CI_VK_DEVICE_FN( CreateGraphicsPipelines(
	//	getDeviceHandle(),
	//	VK_NULL_HANDLE,
	//	1,
	//	&vkci,
	//	nullptr,
	//	&mPipelineHandle ) );
	//// Destroy transient render pass
	// if ( renderPass ) {
	//	vkDestroyRenderPass(
	//		ToApi( GetDevice() )->GetVkDevice(),
	//		renderPass,
	//		nullptr );
	// }
	//// Process result
	// if ( vkres != VK_SUCCESS ) {
	//	PPX_ASSERT_MSG( false, "vkCreateGraphicsPipelines failed: " << ToString( vkres ) );
	//	return ppx::ERROR_API_FAILURE;
	// }
}

} // namespace cinder::vk
