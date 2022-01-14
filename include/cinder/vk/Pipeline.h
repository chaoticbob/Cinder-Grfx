#pragma once

#include "cinder/vk/ChildObject.h"
#include "cinder/GeomIo.h"

namespace cinder::vk {

//! @class PipelineLayout
//!
//!
class PipelineLayout
	: public vk::DeviceChildObject
{
public:
	static const VkShaderStageFlags DEFAULT_STAGE_FLAGS = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	struct Options
	{
		Options() {}

		// clang-format off
		Options& addSetLayout( VkDescriptorSetLayout layout ) { mSetLayouts.push_back( layout ); return *this; }
		Options& addSetLayout( const vk::DescriptorSetLayoutRef& layout );
		Options& addPushConstantRange( const VkPushConstantRange& range ) { mPushConstantRanges.push_back(range); return *this; }
		Options& addPushConstantRange( uint32_t offset, uint32_t size, VkShaderStageFlags stageFlags = DEFAULT_STAGE_FLAGS ) { mPushConstantRanges.push_back( { stageFlags, offset, size } ); return *this; }
		// clang-format on

	private:
		VkPipelineLayoutCreateFlags		   mFlags = 0; // Reserved for future use
		std::vector<VkDescriptorSetLayout> mSetLayouts;
		std::vector<VkPushConstantRange>   mPushConstantRanges;

		friend PipelineLayout;
	};

	virtual ~PipelineLayout();

	static PipelineLayoutRef create( const Options &options = Options(), vk::DeviceRef device = vk::DeviceRef() );

	VkPipelineLayout getPipelineLayoutHandle() const { return mPipelineLayoutHandle; }

private:
	PipelineLayout( vk::DeviceRef device, const Options &options );

private:
	VkPipelineLayout mPipelineLayoutHandle = VK_NULL_HANDLE;
};

//! @class Pipeline
//!
//!
class Pipeline
	: public vk::DeviceChildObject
{
public:
	// Usage:
	//   - Vertex attribute locations are independent of offset and binding. Location
	//     is how the attribute is communicated to the shader.
	//     Use CINDER_APPEND_VERTEX_LOCATION for convenience to define the current
	//     location directly after the previous one.
	//   - Vertex atttribute offsets is based on the format are local to the binding.
	//     Meaning that multiple attributes can have the same value as long the binding is different.
	//     Use CINDER_APPEND_VERTEX_OFFSET for convenience to define the current
	//     offset directly after the previous one.
	//   - Vertex attribute binding corresponds to the binding number determined
	//     by the parameters passed into vkCmdBindVertexBuffers.
	//
	struct Attribute
	{
		Attribute() {}
		Attribute( VkFormat format, uint32_t location = CINDER_APPEND_VERTEX_LOCATION, uint32_t offset = CINDER_APPEND_VERTEX_OFFSET, uint32_t binding = 0 )
			: mFormat( format ), mLocation( location ), mOffset( offset ), mBinding( binding ), mInputRate( VK_VERTEX_INPUT_RATE_VERTEX ) {}

		// clang-format off
		Attribute& format(VkFormat format) { mFormat = format; return *this; }
		Attribute& location(uint32_t value) { mLocation = value; return *this; }
		Attribute& offset(uint32_t value) { mOffset = value; return *this; }
		Attribute& binding(uint32_t value) { mBinding = value; return *this; }
		Attribute& inputRate(VkVertexInputRate value) { mInputRate = value; return *this; }
		// clang-format on

		VkFormat		  getFormat() const { return mFormat; }
		uint32_t		  getLocation() const { return mLocation; }
		uint32_t		  getOffset() const { return mOffset; }
		uint32_t		  getBinding() const { return mBinding; }
		VkVertexInputRate getInputRate() const { return mInputRate; }

	private:
		VkFormat		  mFormat	 = VK_FORMAT_UNDEFINED;
		uint32_t		  mLocation	 = 0;
		uint32_t		  mOffset	 = 0;
		uint32_t		  mBinding	 = 0;
		VkVertexInputRate mInputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	};

	struct InputAssemblyState
	{
		uint32_t				   attributeCount;						 // = 0;
		Attribute				   attributes[CINDER_MAX_VERTEX_INPUTS]; // = {};
		VkPrimitiveTopology		   topology;							 // = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		VkBool32				   primitiveRestart;					 // = VK_FALSE;
		VkTessellationDomainOrigin domainOrigin;						 // = VK_TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT;
		uint32_t				   patchControlPoints;					 // = 0;
	};

	enum SampleShading : int32_t
	{
		SAMPLE_SHADING_OFF	   = 0,
		SAMPLE_SHADING_LOW	   = 1,
		SAMPLE_SHADING_MEDIUM  = 5,
		SAMPLE_SHADING_HIGH	   = 8,
		SAMPLE_SHADING_HIGHEST = 10,
	};

	struct RasterizerState
	{
		// Rasterization
		VkPolygonMode	polygonMode;			 // = VK_POLYGON_MODE_FILL;
		VkCullModeFlags cullMode;				 // = VK_CULL_MODE_NONE;
		VkFrontFace		frontFace;				 // = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		VkBool32		rasterizerDiscardEnable; // = VK_FALSE;
		VkBool32		depthClipEnable;		 // = VK_FALSE;
		VkBool32		depthClampEnable;		 // = VK_FALSE;
		VkBool32		depthBiasEnable;		 // = VK_FALSE;

		// Multi-sample
		VkSampleCountFlagBits rasterizationSamples;	 // = VK_SAMPLE_COUNT_1_BIT;
		SampleShading		  sampleShading;		 // = SAMPLE_SHADING_OFF;
		VkBool32			  alphaToCoverageEnable; // = VK_FALSE;
		VkBool32			  alphaToOneEnable;		 // = VK_FALSE;
	};

	struct StencilOpParams
	{
		VkStencilOp failOp;		 // = VK_STENCIL_OP_KEEP;
		VkStencilOp passOp;		 // = VK_STENCIL_OP_KEEP;
		VkStencilOp depthFailOp; // = VK_STENCIL_OP_KEEP;
		VkCompareOp compareOp;	 // = VK_COMPARE_OP_NEVER;
	};

	struct DepthStencilState
	{
		VkBool32		depthTestEnable;	   // = VK_FALSE;
		VkBool32		depthWriteEnable;	   // = VK_FALSE;
		VkCompareOp		depthCompareOp;		   // = VK_COMPARE_OP_NEVER;
		VkBool32		depthBoundsTestEnable; // = VK_FALSE;
		VkBool32		stencilTestEnable;	   // = VK_FALSE;
		uint32_t		stencilReadMask;	   // = CINDER_DEFAULT_STENCIL_READ_MASK;
		uint32_t		stencilWriteMask;	   // = CINDER_DEFAULT_STENCIL_WRITE_MASK;
		uint32_t		stencilReference;	   // = CINDER_DEFAULT_STENCIL_REFERENCE;
		StencilOpParams front;				   // = {};
		StencilOpParams back;				   // = {};
	};

	struct ColorBlendAttachment
	{
		VkBool32			  blendEnable;		   // = VK_FALSE;
		VkBlendFactor		  srcColorBlendFactor; // = VK_BLEND_FACTOR_ONE;
		VkBlendFactor		  dstColorBlendFactor; // = VK_BLEND_FACTOR_ZERO;
		VkBlendOp			  colorBlendOp;		   // = VK_BLEND_OP_ADD;
		VkBlendFactor		  srcAlphaBlendFactor; // = VK_BLEND_FACTOR_ONE;
		VkBlendFactor		  dstAlphaBlendFactor; // = VK_BLEND_FACTOR_ZERO;
		VkBlendOp			  alphaBlendOp;		   // = VK_BLEND_OP_ADD;
		VkColorComponentFlags colorWriteMask;	   // = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	};

	struct ColorBlendState
	{
		VkBool32			 logicOpEnable;							 // = VK_FALSE;
		VkLogicOp			 logicOp;								 // = VK_LOGIC_OP_CLEAR;
		uint32_t			 attachmentCount;						 // = 0;
		ColorBlendAttachment attachments[CINDER_MAX_RENDER_TARGETS]; // = { {}, {}, {}, {}, {}, {}, {}, {} };
	};

	struct OutputMergerState
	{
		uint32_t renderTargetCount;						   // = 0;
		VkFormat renderTargets[CINDER_MAX_RENDER_TARGETS]; // = { VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED };
		VkFormat depthStencil;							   // = VK_FORMAT_UNDEFINED;
	};

	// GraphicsPipelineCreateInfo encapsulates a set of fields
	// that is later used for graphics pipeline creation and
	// application level pipeline caching. The caching uses
	// xxHash to hash shader pointers, pipeline layout, and
	// graphics states. The cost of hashing + lookup is faster
	// than going through a full pipeline creation. In order
	// for the lookup to be effective, the hashing has to be
	// deterministic. Meaning that for input data that's the
	// same, the xxHash output is always the same. Below are
	// requirements for any struct that is must be a member
	// of GraphicsPipelineState.
	//
	// Requirements for state structs in GraphicsPipelineState
	//   - All fields must be initializable to zero
	//   - Do not use float for any fields
	//   - Use VkBool32 instead of bool since bool
	//
	//
	// ** NITTY GRITTY DETAILS **
	//
	// If you intend to hash a aggregate initialization object,
	// use memset to zero out the objectbefore populating it.
	// DO NOT rely on brace initialization (otherwise known as
	// aggregate initialization or default initailization) to
	// zero out the object. Brace initailization will initalize
	// members to their default values. Zero is used if a
	// default value is not explicitly assigned. What brace
	// initialization DOESN'T DO is set values for any padding
	// that may be inserted by the compiler  between members.
	// The padding can contain random values and this of course
	// would make the hashing non-deterministic.
	//
	struct GraphicsPipelineCreateInfo
	{
		const vk::ShaderModule   *vert;			  // = nullptr;
		const vk::ShaderModule   *frag;			  // = nullptr;
		const vk::ShaderModule   *geom;			  // = nullptr;
		const vk::ShaderModule   *tese;			  // = nullptr;
		const vk::ShaderModule   *tesc;			  // = nullptr;
		const vk::PipelineLayout *pipelineLayout; // = nullptr;
		InputAssemblyState		  ia;			  // = {};
		RasterizerState			  rs;			  // = {};
		DepthStencilState		  ds;			  // = {};
		ColorBlendState			  cb;			  // = {};
		OutputMergerState		  om;			  // = {};
	};

	// struct Options
	//{
	//	Options() {}
	//	Options( const GraphicsPipelineState &gs );
	//
	//	// clang-format off
	//	/*
	//	Options& attributeFormat(uint32_t index, VkFormat format);
	//	Options& attributeLocation(uint32_t index, uint32_t location);
	//	Options& attributeBinding(uint32_t index, uint32_t binding);
	//	Options& attribute(uint32_t index, const Attribute& attr);
	//	Options& depthTest(bool value = true) { mGraphicsState.depthStencilState.depthTestEnable = value; return *this; }
	//	Options& depthWrite(bool value = true) { mGraphicsState.depthStencilState.depthWriteEnable = value; return *this; }
	//	Options& addRenderTarget(VkFormat format);
	//	Options& setDepthStencil(VkFormat format) { mGraphicsState.outputState.depthStencil = format; return *this; }
	//	*/
	//	// clang-format on
	//
	// private:
	//	GraphicsPipelineState mGraphicsState = {};
	//	friend Pipeline;
	// };
	//
	virtual ~Pipeline();

	static void setDefaults( GraphicsPipelineCreateInfo *createInfo );

	static vk::PipelineRef create( const GraphicsPipelineCreateInfo &createInfo, vk::DeviceRef device = nullptr );

	static uint64_t calculateHash( const GraphicsPipelineCreateInfo *createInfo );

	VkPipeline getPipelineHandle() const { return mPipelineHandle; }

private:
	Pipeline( vk::DeviceRef device, const GraphicsPipelineCreateInfo &createInfo );

	void initShaderStages(
		const GraphicsPipelineCreateInfo			 &createInfo,
		std::vector<VkPipelineShaderStageCreateInfo> &shaderStages );
	void initVertexInput(
		const GraphicsPipelineCreateInfo				 &createInfo,
		std::vector<VkVertexInputAttributeDescription> &vertexAttributes,
		std::vector<VkVertexInputBindingDescription>	 &vertexBindings,
		VkPipelineVertexInputStateCreateInfo			 &stateCreateInfo );
	void initPrimitiveTopology(
		const GraphicsPipelineCreateInfo					 &createInfo,
		VkPipelineInputAssemblyStateCreateInfo			   &inputAssemblyStateCreateInfo,
		VkPipelineTessellationDomainOriginStateCreateInfoKHR &domainOriginStateCreateInfo,
		VkPipelineTessellationStateCreateInfo				  &tessellationStateCreateInfo );
	void initRasterizer(
		const GraphicsPipelineCreateInfo					 &createInfo,
		VkPipelineRasterizationDepthClipStateCreateInfoEXT &depthClipStateCreateInfo,
		VkPipelineRasterizationStateCreateInfo			   &rasterizationStateCreateInfo,
		VkPipelineMultisampleStateCreateInfo				 &multisampleStateCreateInfo );
	void initDepthStencil(
		const GraphicsPipelineCreateInfo		 &createInfo,
		VkPipelineDepthStencilStateCreateInfo &stateCreateInfo );
	void initColorBlend(
		const GraphicsPipelineCreateInfo				 &createInfo,
		std::vector<VkPipelineColorBlendAttachmentState> &vkAttachments,
		VkPipelineColorBlendStateCreateInfo				&stateCreateInfo );
	void initDynamicState(
		const GraphicsPipelineCreateInfo &createInfo,
		std::vector<VkDynamicState>		&dynamicStates,
		VkPipelineDynamicStateCreateInfo &stateCreateInfo );

	void initGraphicsPipeline( const GraphicsPipelineCreateInfo &createInfo );

private:
	VkPipeline mPipelineHandle = VK_NULL_HANDLE;
};

//! @class PipelineManager
//!
//!
class PipelineManager
	: public vk::DeviceChildObject
{
public:
	PipelineManager( vk::DeviceRef device );
	~PipelineManager();

	vk::PipelineRef CompilePipeline( const vk::Pipeline::GraphicsPipelineCreateInfo &createInfo );
};

} // namespace cinder::vk
