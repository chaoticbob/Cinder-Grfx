#include "cinder/vk/Pipeline.h"
#include "cinder/vk/Descriptor.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/ShaderProg.h"
#include "cinder/vk/Util.h"
#include "cinder/app/RendererVk.h"

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

	//VkResult vkres = vkCreatePipelineLayout(
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

PipelineRef Pipeline::create( ShaderProgRef shaderProg, PipelineLayoutRef pipelineLayout, Options &options, DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return PipelineRef( new Pipeline( device, shaderProg, pipelineLayout, options ) );
}

Pipeline::Pipeline( DeviceRef device, ShaderProgRef shaderProg, PipelineLayoutRef pipelineLayout, Options &options )
	: vk::DeviceChildObject( device )
{
	if ( shaderProg->isCompute() ) {
		initComputePipeline( shaderProg, pipelineLayout );
	}
	else {
		initGraphicsPipeline( shaderProg, pipelineLayout, options );
	}
}

Pipeline::~Pipeline()
{
	if ( mPipelineHandle ) {
		//vkDestroyPipeline(
		getDevice()->vkfn()->DestroyPipeline(
			getDeviceHandle(),
			mPipelineHandle,
			nullptr );
		mPipelineHandle = VK_NULL_HANDLE;
	}
}

void Pipeline::initComputePipeline( ShaderProgRef shaderProg, PipelineLayoutRef pipelineLayout )
{
}

void Pipeline::initShaderStages(
	ShaderProgRef								  shaderProg,
	const Options &								  options,
	std::vector<VkPipelineShaderStageCreateInfo> &shaderStages )
{
	// VS
	vk::ShaderModuleRef shaderModule = shaderProg->getVertexShader();
	if ( shaderModule ) {
		VkPipelineShaderStageCreateInfo ssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ssci.flags							 = 0;
		ssci.pSpecializationInfo			 = nullptr;
		ssci.pName							 = shaderModule->getEntryPoint();
		ssci.stage							 = VK_SHADER_STAGE_VERTEX_BIT;
		ssci.module							 = shaderModule->getShaderModuleHandle();
		shaderStages.push_back( ssci );
	}

	// PS
	shaderModule = shaderProg->getPixelShader();
	if ( shaderModule ) {
		VkPipelineShaderStageCreateInfo ssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ssci.flags							 = 0;
		ssci.pSpecializationInfo			 = nullptr;
		ssci.pName							 = shaderModule->getEntryPoint();
		ssci.stage							 = VK_SHADER_STAGE_FRAGMENT_BIT;
		ssci.module							 = shaderModule->getShaderModuleHandle();
		shaderStages.push_back( ssci );
	}

	// HS
	shaderModule = shaderProg->getHullShader();
	if ( shaderModule ) {
		VkPipelineShaderStageCreateInfo ssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ssci.flags							 = 0;
		ssci.pSpecializationInfo			 = nullptr;
		ssci.pName							 = shaderModule->getEntryPoint();
		ssci.stage							 = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		ssci.module							 = shaderModule->getShaderModuleHandle();
		shaderStages.push_back( ssci );
	}

	// DS
	shaderModule = shaderProg->getDomainShader();
	if ( shaderModule ) {
		VkPipelineShaderStageCreateInfo ssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ssci.flags							 = 0;
		ssci.pSpecializationInfo			 = nullptr;
		ssci.pName							 = shaderModule->getEntryPoint();
		ssci.stage							 = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		ssci.module							 = shaderModule->getShaderModuleHandle();
		shaderStages.push_back( ssci );
	}

	// GS
	shaderModule = shaderProg->getGeometryShader();
	if ( shaderModule ) {
		VkPipelineShaderStageCreateInfo ssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ssci.flags							 = 0;
		ssci.pSpecializationInfo			 = nullptr;
		ssci.pName							 = shaderModule->getEntryPoint();
		ssci.stage							 = VK_SHADER_STAGE_GEOMETRY_BIT;
		ssci.module							 = shaderModule->getShaderModuleHandle();
		shaderStages.push_back( ssci );
	}
}

void Pipeline::initVertexInput(
	const Options &									options,
	std::vector<VkVertexInputAttributeDescription> &vertexAttributes,
	std::vector<VkVertexInputBindingDescription> &	vertexBindings,
	VkPipelineVertexInputStateCreateInfo &			vertexInputStateCreateInfo )
{
	// Populate attribute and bindings
	for ( uint32_t i = 0; i < options.mGraphicsState.ia.attributeCount; ++i ) {
		const auto &	  attr		= options.mGraphicsState.ia.attributes[i];
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
	const Options &										  options,
	VkPipelineInputAssemblyStateCreateInfo &			  inputAssemblyStateCreateInfo,
	VkPipelineTessellationDomainOriginStateCreateInfoKHR &domainOriginStateCreateInfo,
	VkPipelineTessellationStateCreateInfo &				  tessellationStateCreateInfo )
{
	inputAssemblyStateCreateInfo.flags					= 0;
	inputAssemblyStateCreateInfo.topology				= options.mGraphicsState.ia.topology;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = options.mGraphicsState.ia.primitiveRestart ? VK_TRUE : VK_FALSE;

	domainOriginStateCreateInfo.domainOrigin = options.mGraphicsState.ia.domainOrigin;

	tessellationStateCreateInfo.flags			   = 0;
	tessellationStateCreateInfo.patchControlPoints = options.mGraphicsState.ia.patchControlPoints;
}

void Pipeline::initRasterizer(
	const Options &										options,
	VkPipelineRasterizationDepthClipStateCreateInfoEXT &depthClipStateCreateInfo,
	VkPipelineRasterizationStateCreateInfo &			rasterizationStateCreateInfo,
	VkPipelineMultisampleStateCreateInfo &				multisampleStateCreateInfo )
{
	rasterizationStateCreateInfo.flags					 = 0;
	rasterizationStateCreateInfo.depthClampEnable		 = options.mGraphicsState.rs.depthClampEnable;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = options.mGraphicsState.rs.rasterizerDiscardEnable;
	rasterizationStateCreateInfo.polygonMode			 = options.mGraphicsState.rs.polygonMode;
	rasterizationStateCreateInfo.cullMode				 = options.mGraphicsState.rs.cullMode;
	rasterizationStateCreateInfo.frontFace				 = options.mGraphicsState.rs.frontFace;
	rasterizationStateCreateInfo.depthBiasEnable		 = options.mGraphicsState.rs.depthBiasEnable;
	rasterizationStateCreateInfo.depthBiasConstantFactor = CINDER_DEFAULT_DEPTH_BIAS;
	rasterizationStateCreateInfo.depthBiasClamp			 = CINDER_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizationStateCreateInfo.depthBiasSlopeFactor	 = CINDER_DEFAULT_DEPTH_BIAS_SCALE;
	rasterizationStateCreateInfo.lineWidth				 = CINDER_DEFAULT_LINE_WIDTH;

	depthClipStateCreateInfo.flags			 = 0;
	depthClipStateCreateInfo.depthClipEnable = options.mGraphicsState.rs.depthClipEnable;

	multisampleStateCreateInfo.flags				 = 0;
	multisampleStateCreateInfo.rasterizationSamples	 = options.mGraphicsState.rs.rasterizationSamples;
	multisampleStateCreateInfo.sampleShadingEnable	 = ( options.mGraphicsState.rs.sampleShading != SAMPLE_SHADING_OFF ) ? VK_TRUE : VK_FALSE;
	multisampleStateCreateInfo.minSampleShading		 = ( options.mGraphicsState.rs.sampleShading != SAMPLE_SHADING_OFF ) ? ( static_cast<float>( options.mGraphicsState.rs.sampleShading ) / 10.0f ) : 0.0f;
	multisampleStateCreateInfo.pSampleMask			 = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = options.mGraphicsState.rs.alphaToCoverageEnable;
	multisampleStateCreateInfo.alphaToOneEnable		 = options.mGraphicsState.rs.alphaToOneEnable;
}

void Pipeline::initDepthStencil(
	const Options &						   options,
	VkPipelineDepthStencilStateCreateInfo &depthStencilStateCreateInfo )
{
	depthStencilStateCreateInfo.flags				  = 0;
	depthStencilStateCreateInfo.depthTestEnable		  = options.mGraphicsState.ds.depthTestEnable;
	depthStencilStateCreateInfo.depthWriteEnable	  = options.mGraphicsState.ds.depthWriteEnable;
	depthStencilStateCreateInfo.depthCompareOp		  = options.mGraphicsState.ds.depthCompareOp;
	depthStencilStateCreateInfo.depthBoundsTestEnable = options.mGraphicsState.ds.depthBoundsTestEnable;
	depthStencilStateCreateInfo.stencilTestEnable	  = options.mGraphicsState.ds.stencilTestEnable;
	depthStencilStateCreateInfo.front.failOp		  = options.mGraphicsState.ds.front.failOp;
	depthStencilStateCreateInfo.front.passOp		  = options.mGraphicsState.ds.front.passOp;
	depthStencilStateCreateInfo.front.depthFailOp	  = options.mGraphicsState.ds.front.depthFailOp;
	depthStencilStateCreateInfo.front.compareOp		  = options.mGraphicsState.ds.front.compareOp;
	depthStencilStateCreateInfo.front.compareMask	  = options.mGraphicsState.ds.stencilReadMask;
	depthStencilStateCreateInfo.front.writeMask		  = options.mGraphicsState.ds.stencilWriteMask;
	depthStencilStateCreateInfo.front.reference		  = options.mGraphicsState.ds.stencilReference;
	depthStencilStateCreateInfo.back.failOp			  = options.mGraphicsState.ds.back.failOp;
	depthStencilStateCreateInfo.back.passOp			  = options.mGraphicsState.ds.back.passOp;
	depthStencilStateCreateInfo.back.depthFailOp	  = options.mGraphicsState.ds.back.depthFailOp;
	depthStencilStateCreateInfo.back.compareOp		  = options.mGraphicsState.ds.back.compareOp;
	depthStencilStateCreateInfo.back.compareMask	  = options.mGraphicsState.ds.stencilReadMask;
	depthStencilStateCreateInfo.back.writeMask		  = options.mGraphicsState.ds.stencilWriteMask;
	depthStencilStateCreateInfo.back.reference		  = options.mGraphicsState.ds.stencilReference;
	depthStencilStateCreateInfo.minDepthBounds		  = CINDER_DEFAULT_VIEWPORT_MIN_DEPTH;
	depthStencilStateCreateInfo.maxDepthBounds		  = CINDER_DEFAULT_VIEWPORT_MAX_DEPTH;
}

void Pipeline::initColorBlend(
	const Options &									  options,
	std::vector<VkPipelineColorBlendAttachmentState> &attachments,
	VkPipelineColorBlendStateCreateInfo &			  blendStateCreateInfo )
{
	for ( uint32_t i = 0; i < options.mGraphicsState.cb.attachmentCount; ++i ) {
		const ColorBlendAttachment &cba = options.mGraphicsState.cb.attachments[i];

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
	blendStateCreateInfo.logicOpEnable	   = options.mGraphicsState.cb.logicOpEnable;
	blendStateCreateInfo.logicOp		   = options.mGraphicsState.cb.logicOp;
	blendStateCreateInfo.attachmentCount   = countU32( attachments );
	blendStateCreateInfo.pAttachments	   = dataPtr( attachments );
	blendStateCreateInfo.blendConstants[0] = CINDER_DEFAULT_BLEND_R;
	blendStateCreateInfo.blendConstants[1] = CINDER_DEFAULT_BLEND_G;
	blendStateCreateInfo.blendConstants[2] = CINDER_DEFAULT_BLEND_B;
	blendStateCreateInfo.blendConstants[3] = CINDER_DEFAULT_BLEND_A;
}

void Pipeline::initDynamicState(
	const Options &					  options,
	std::vector<VkDynamicState> &	  dynamicStates,
	VkPipelineDynamicStateCreateInfo &dynamcStateCreateInfo )
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
	dynamicStates.push_back( VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_FRONT_FACE_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_STENCIL_OP_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT_EXT );

	// Provided by VK_EXT_extended_dynamic_state2
	dynamicStates.push_back( VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_LOGIC_OP_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_PATCH_CONTROL_POINTS_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE_EXT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE_EXT );

	dynamcStateCreateInfo.flags				= 0;
	dynamcStateCreateInfo.dynamicStateCount = countU32( dynamicStates );
	dynamcStateCreateInfo.pDynamicStates	= dataPtr( dynamicStates );
}

void Pipeline::initGraphicsPipeline( ShaderProgRef shaderProg, PipelineLayoutRef pipelineLayout, Options &options )
{
	VkGraphicsPipelineCreateInfo vkci = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

	// Shaders
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	initShaderStages( shaderProg, options, shaderStages );

	// Vertex input
	std::vector<VkVertexInputAttributeDescription> vertexAttributes;
	std::vector<VkVertexInputBindingDescription>   vertexBindings;
	VkPipelineVertexInputStateCreateInfo		   vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	initVertexInput( options, vertexAttributes, vertexBindings, vertexInputState );

	// Input assembly / primitive topology
	VkPipelineInputAssemblyStateCreateInfo				 inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	VkPipelineTessellationDomainOriginStateCreateInfoKHR domainOriginState	= { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO_KHR };
	VkPipelineTessellationStateCreateInfo				 tessellationState	= { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
	initPrimitiveTopology( options, inputAssemblyState, domainOriginState, tessellationState );
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
	initRasterizer( options, depthClipStateCreateInfo, rasterizationState, multisampleState );
	rasterizationState.pNext = &depthClipStateCreateInfo;

	// Depth/stencil
	VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	initDepthStencil( options, depthStencilState );

	// Color blend
	std::vector<VkPipelineColorBlendAttachmentState> attachments;
	VkPipelineColorBlendStateCreateInfo				 colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	initColorBlend( options, attachments, colorBlendState );

	// Dynamic states
	std::vector<VkDynamicState>		 dynamicStatesArray;
	VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	initDynamicState( options, dynamicStatesArray, dynamicState );

	//// Create temporary render pass
	////
	//VkRenderPassPtr renderPass = VK_NULL_HANDLE;
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
	//}

	// Fill in pointers nad remaining values
	//
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
	vkci.layout				 = pipelineLayout->getPipelineLayoutHandle();
	vkci.renderPass			 = VK_NULL_HANDLE;
	vkci.subpass			 = 0; // One subpass to rule them all
	vkci.basePipelineHandle	 = VK_NULL_HANDLE;
	vkci.basePipelineIndex	 = -1;

	//VkResult vkres = vkCreateGraphicsPipelines(
	//VkResult vkres = CI_VK_DEVICE_FN( CreateGraphicsPipelines(
	//	getDeviceHandle(),
	//	VK_NULL_HANDLE,
	//	1,
	//	&vkci,
	//	nullptr,
	//	&mPipelineHandle ) );
	//// Destroy transient render pass
	//if ( renderPass ) {
	//	vkDestroyRenderPass(
	//		ToApi( GetDevice() )->GetVkDevice(),
	//		renderPass,
	//		nullptr );
	//}
	//// Process result
	//if ( vkres != VK_SUCCESS ) {
	//	PPX_ASSERT_MSG( false, "vkCreateGraphicsPipelines failed: " << ToString( vkres ) );
	//	return ppx::ERROR_API_FAILURE;
	//}
}

} // namespace cinder::vk
