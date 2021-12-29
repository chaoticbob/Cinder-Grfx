#pragma once

#include "cinder/vk/DeviceChildObject.h"
#include "cinder/vk/Pipeline.h"
#include "cinder/Camera.h"
#include "cinder/CinderGlm.h"

namespace cinder::vk {

//! @class Context
//!
//!
class Context
	: public vk::DeviceChildObject
{
private:
	//
	// NOTE: Seperated depth/stencil is coming just not here yet.
	//
	struct Frame
	{
		vk::DescriptorPoolRef descriptorPool;
		vk::DescriptorSetRef  descriptorSet;
		vk::BufferRef		  defaultUniformBuffer;

		std::vector<vk::ImageRef>	  renderTargets;
		vk::ImageRef				  depthTarget;
		vk::ImageRef				  stencilTarget;
		std::vector<vk::ImageViewRef> rtvs;
		vk::ImageViewRef			  dtv;
		vk::ImageViewRef			  stv;
		vk::CommandBufferRef		  commandBuffer;
		uint64_t					  frameSignaledValue;
	};

public:
	struct Options
	{
		Options() {}

		Options( VkFormat renderTargetFormat, VkFormat depthStencilFormat, uint32_t samples );

		// clang-format off
		Options &numInFlightFrames( uint32_t value ) { mNumInFlightFrames = std::max<uint32_t>( 1, value ); return *this; }
		Options &setRenderTarget( VkFormat format ) { mRenderTargetFormat = format; return *this; }
		Options &setDepthStencilFormat( VkFormat format ) { mDepthFormat = format; mStencilFormat = format; return *this; }
		Options &addRenderTarget( VkFormat format ) { mAdditionalRenderTargets.push_back( format ); return *this; }
		Options &sampleCount( uint32_t value );
		// clang-format on

	private:
		// clang-format off
		// Make these public when seperated depth/stencil has landed.		
		Options &setDepthFormat( VkFormat format ) { mDepthFormat = format; return *this; }
		Options &setStencilFormat( VkFormat format ) { mStencilFormat = format; return *this; }
		// clang-format on

	private:
		uint32_t			  mNumInFlightFrames	   = 2;
		VkFormat			  mRenderTargetFormat	   = VK_FORMAT_R8G8B8A8_UNORM;
		VkFormat			  mDepthFormat			   = VK_FORMAT_D32_SFLOAT_S8_UINT;
		VkFormat			  mStencilFormat		   = VK_FORMAT_D32_SFLOAT_S8_UINT;
		VkSampleCountFlagBits mSampleCount			   = VK_SAMPLE_COUNT_1_BIT;
		std::vector<VkFormat> mAdditionalRenderTargets = {};

		friend class Context;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct SemaphoreInfo
	{
		vk::Semaphore *semaphore;
		uint64_t	   value = 0;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	~Context();

	static ContextRef create( const Options &options = Options(), vk::DeviceRef device = vk::DeviceRef() );
	static ContextRef create( uint32_t width, uint32_t height, const Options &options = Options(), vk::DeviceRef device = vk::DeviceRef() );

	void			makeCurrent( const std::vector<SemaphoreInfo> &externalWaits = std::vector<SemaphoreInfo>() );
	static Context *getCurrentContext();
	void			submit( const std::vector<SemaphoreInfo> &waits, const std::vector<SemaphoreInfo> &signals );
	void			waitForCompletion();

	bool isRenderable() const { return ( mWidth > 0 ) && ( mHeight > 0 ); }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Sets the clear value
	void clearColor( const ColorA &color ) { mClearValues.color = color; }
	void clearDepth( const float depth ) { mClearValues.depth = depth; }
	void clearStencil( const int stencil ) { mClearValues.stencil = static_cast<uint8_t>( stencil ); }

	// clang-format off
	void enableDepthWrite( bool enable ) { mGraphicsState.ds.depthWriteEnable = enable; }
	void enableDepthTest( bool enable ) { mGraphicsState.ds.depthTestEnable = enable; }
	void enableStencilTest( bool enable ) { mGraphicsState.ds.stencilTestEnable = enable; }
	// clang-format on

	std::vector<ci::mat4> &getModelMatrixStack() { return mModelMatrixStack; }
	std::vector<ci::mat4> &getViewMatrixStack() { return mViewMatrixStack; }
	std::vector<ci::mat4> &getProjectionMatrixStack() { return mProjectionMatrixStack; }

	std::pair<ivec2, ivec2> getViewport();

	void bindShaderProg( vk::ShaderProgRef prog );

	void bindTexture( uint32_t binding, vk::ImageView *imageView, vk::Sampler *sampler );
	void unbindTexture( uint32_t binding );

	void setDefaultShaderVars();

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	vk::CommandBuffer *getCommandBuffer() const { return getCurrentFrame().commandBuffer.get(); }

	uint32_t			 getFrameIndex() const { return mFrameIndex; }
	uint32_t			 getNumRenderTargets() const { return countU32( mRenderTargetFormats ); }
	const vk::ImageView *getRenderTargetView( uint32_t index ) const { return getCurrentFrame().rtvs[index].get(); }
	const vk::ImageView *getDepthStencilView() const { return mCombinedDepthStencil ? getCurrentFrame().dtv.get() : nullptr; }
	const vk::ImageView *getDepthTargetView() const { return getCurrentFrame().dtv.get(); }
	const vk::ImageView *getStencilTargetView() const { return getCurrentFrame().stv.get(); }

	void clearColorAttachment( uint32_t index );
	void clearDepthStencilAttachment( VkImageAspectFlags aspectMask );

	void bindDefaultDescriptorSet();
	void bindIndexBuffers( const vk::BufferedMeshRef &mesh );
	void bindVertexBuffers( const vk::BufferedMeshRef &mesh );
	void bindGraphicsPipeline();
	void draw( int32_t firstVertex, int32_t vertexCount );
	void drawIndexed( int32_t firstIndex, int32_t indexCount );

private:
	Context( vk::DeviceRef device, uint32_t width, uint32_t height, const Options &options );

	void		 initializeDescriptorSetLayouts();
	void		 initializePipelineLayout();
	void		 initializeFrame( vk::CommandBufferRef commandBuffer, Frame &frame );
	Frame &		 getCurrentFrame();
	const Frame &getCurrentFrame() const;

	void assignVertexAttributeLocations();

private:
	struct ClearValues
	{
		ColorA	color	= ColorA( 0, 0, 0, 0 );
		float	depth	= CINDER_DEFAULT_DEPTH;
		uint8_t stencil = CINDER_DEFAULT_STENCIL;
	};

	/*
	struct GraphicsState
	{
		std::vector<vk::Pipeline::Attribute> vertexAttributes;
		std::vector<vk::BufferRef>			 vertexBuffers;
		VkCullModeFlags						 cullMode	 = VK_CULL_MODE_NONE;
		VkFrontFace							 frontFace	 = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		bool								 depthWrite	 = false;
		bool								 depthTest	 = false;
		bool								 stencilTest = false;
		vk::ShaderProgRef					 program;
	};
*/

	class DescriptorState
	{
	public:
		DescriptorState();
		~DescriptorState() {}

		void bindUniformBuffer( uint32_t bindingNumber, vk::Buffer *buffer );
		void bindCombinedImageSampler( uint32_t bindingNumber, vk::ImageView *imageView, vk::Sampler *sampler );

	private:
		struct BufferInfo
		{
			vk::Buffer *buffer;
		};

		struct ImageInfo
		{
			vk::ImageView *imageView;
			vk::Sampler *  sampler;
		};

		struct Descriptor
		{
			VkDescriptorType type		   = static_cast<VkDescriptorType>( ~0 );
			uint32_t		 bindingNumber = UINT32_MAX;

			union
			{
				BufferInfo bufferInfo = {};
				ImageInfo  imageInfo;
			};

			Descriptor() {}

			Descriptor( uint32_t aBindingNumber, vk::Buffer *aBuffer )
				: type( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ), bindingNumber( aBindingNumber ), bufferInfo( { aBuffer } ) {}

			Descriptor( uint32_t aBindingNumber, vk::ImageView *aImageView, vk::Sampler *aSampler )
				: type( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ), bindingNumber( aBindingNumber ), imageInfo( { aImageView, aSampler } ) {}
		};

		std::map<uint32_t, Descriptor> mDescriptors;

		friend class Context;
	};

	uint32_t			  mNumFramesInFlight	= 0;
	uint32_t			  mWidth				= 0;
	uint32_t			  mHeight				= 0;
	std::vector<VkFormat> mRenderTargetFormats	= {};
	VkFormat			  mDepthFormat			= VK_FORMAT_UNDEFINED;
	VkFormat			  mStencilFormat		= VK_FORMAT_UNDEFINED;
	bool				  mCombinedDepthStencil = false;
	VkSampleCountFlagBits mSampleCount			= VK_SAMPLE_COUNT_1_BIT;

	CommandPoolRef			 mCommandPool;
	std::vector<Frame>		 mFrames;
	uint64_t				 mFrameCount		 = 0;
	uint32_t				 mFrameIndex		 = 0;
	uint32_t				 mPreviousFrameIndex = 0;
	vk::CountingSemaphoreRef mFrameSyncSemaphore;

	ClearValues							 mClearValues = {};
	std::vector<std::pair<ivec2, ivec2>> mViewportStack;
	std::vector<std::pair<ivec2, ivec2>> mScissorStack;
	std::vector<ci::mat4>				 mModelMatrixStack;
	std::vector<ci::mat4>				 mViewMatrixStack;
	std::vector<ci::mat4>				 mProjectionMatrixStack;

	std::vector<std::pair<geom::BufferLayout, vk::BufferRef>> mVertexBuffers;
	vk::ShaderProgRef										  mShaderProgram;
	vk::Pipeline::GraphicsPipelineState						  mGraphicsState = {};

	vk::DescriptorSetLayoutRef mDefaultSetLayout;
	vk::PipelineLayoutRef	   mDefaultPipelineLayout;
	DescriptorState			   mDescriptorState;
	vk::PipelineRef			   mGraphicsPipeline;
};

} // namespace cinder::vk
