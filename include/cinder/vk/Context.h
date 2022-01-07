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
		struct DrawCall
		{
			bool				 inUse;
			vk::DescriptorSetRef descriptorSet;
			vk::BufferRef		 defaultUniformBuffer;
		};

		vk::DescriptorPoolRef descriptorPool;
		// vk::DescriptorSetRef							descriptorSet;
		// vk::BufferRef									defaultUniformBuffer;
		// std::vector<std::pair<uint32_t, vk::BufferRef>> defaultUniformBuffers;
		// vk::Buffer									 *currentDefaultUniformBuffer = nullptr;

		std::vector<std::unique_ptr<DrawCall>> drawCalls;
		DrawCall							  *currentDrawCall = nullptr;

		std::vector<vk::ImageRef>	  renderTargets;
		vk::ImageRef				  depthStencil;
		std::vector<vk::ImageViewRef> rtvs;
		vk::ImageViewRef			  dsv;
		vk::CommandBufferRef		  commandBuffer;
		uint64_t					  frameSignaledValue;

		// void resetDefaultUniformBuffers();
		// void nextDefaultUniformBuffer();

		void resetDrawCalls();
		void nextDrawCall( const vk::DescriptorSetLayoutRef& defaultSetLayout );
	};

public:
	struct Options
	{
		Options() {}

		Options( VkFormat renderTargetFormat, VkFormat depthStencilFormat, uint32_t samples );

		// clang-format off
		Options &numInFlightFrames( uint32_t value ) { mNumInFlightFrames = std::max<uint32_t>( 1, value ); return *this; }
		Options &setRenderTargets( std::vector<VkFormat> formats ) { mRenderTargetFormats = formats; return *this; }
		Options &setDepthStencil( VkFormat format ) { mDepthStencilFormat = format; return *this; }
		Options &sampleCount( uint32_t value );
		// clang-format on

	private:
		uint32_t			  mNumInFlightFrames   = 2;
		std::vector<VkFormat> mRenderTargetFormats = { VK_FORMAT_R8G8B8A8_UNORM };
		VkFormat			  mDepthStencilFormat  = VK_FORMAT_D32_SFLOAT_S8_UINT;
		VkSampleCountFlagBits mSampleCount		   = VK_SAMPLE_COUNT_1_BIT;

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
	//void enableDepthWrite( bool enable ) { mGraphicsState.ds.depthWriteEnable = enable; }
	//void enableDepthTest( bool enable ) { mGraphicsState.ds.depthTestEnable = enable; }
	//void enableStencilTest( bool enable ) { mGraphicsState.ds.stencilTestEnable = enable; }
	void enableDepthWrite( bool enable ) { mDynamicStates.depthWrite = enable; }
	void enableDepthTest( bool enable ) { mDynamicStates.depthTest = enable; }
	void enableStencilTest( bool enable ) { mDynamicStates.stencilTest = enable; }
	// clang-format on

	std::vector<ci::mat4> &getModelMatrixStack() { return mModelMatrixStack; }
	std::vector<ci::mat4> &getViewMatrixStack() { return mViewMatrixStack; }
	std::vector<ci::mat4> &getProjectionMatrixStack() { return mProjectionMatrixStack; }

	std::pair<ivec2, ivec2> getViewport();

	void bindShaderProg( vk::ShaderProgRef prog );

	void				   bindTexture( const vk::TextureBase *texture, uint32_t binding );
	void				   unbindTexture( uint32_t binding );
	void				   pushTextureBinding( const vk::TextureBase *texture );
	void				   pushTextureBinding( const vk::TextureBase *texture, uint32_t binding );
	void				   popTextureBinding( uint32_t binding, bool forceRestore = false );
	const vk::TextureBase *getTextureBinding();
	const vk::TextureBase *getTextureBinding( uint32_t binding );

	void	 setActiveTexture( uint32_t binding );
	void	 pushActiveTexture( uint32_t binding );
	void	 pushActiveTexture();
	void	 popActiveTexture( bool forceRestore = false );
	uint32_t getActiveTexture();

	void setDefaultShaderVars();

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	vk::CommandBuffer *getCommandBuffer() const { return getCurrentFrame().commandBuffer.get(); }

	uint32_t			 getFrameIndex() const { return mFrameIndex; }
	uint32_t			 getNumRenderTargets() const { return countU32( mRenderTargetFormats ); }
	const vk::ImageView *getRenderTargetView( uint32_t index ) const { return getCurrentFrame().rtvs[index].get(); }
	const vk::ImageView *getDepthStencilView() const { return getCurrentFrame().dsv.get(); }

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
	Frame		  &getCurrentFrame();
	const Frame &getCurrentFrame() const;

	void assignVertexAttributeLocations();
	void initTextureBindingStack( uint32_t binding );
	void setDynamicStates( bool force = false );

	//! Returns \c true if \a value is different from the previous top of the stack
	template <typename T>
	bool pushStackState( std::vector<T> &stack, T value );
	//! Returns \c true if the new top of \a stack is different from the previous top, or the stack is empty
	template <typename T>
	bool popStackState( std::vector<T> &stack );
	//! Returns \c true if \a value is different from the previous top of the stack
	template <typename T>
	bool setStackState( std::vector<T> &stack, T value );
	//! Returns \c true if \a result is valid; will return \c false when \a stack was empty
	template <typename T>
	bool getStackState( std::vector<T> &stack, T *result );

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

	template <typename ValueT>
	class StateT
	{
	public:
		StateT( const ValueT &value = ValueT() )
			: mValue( value ), mDirty( true ) {}
		~StateT() {}

		bool isDirty() const { return mDirty; }
		void setClean() { mDirty = false; }

		void setValue( const ValueT &value )
		{
			mDirty = ( mValue != value );
			if ( mDirty ) {
				mValue = value;
			}
		}

		const ValueT &getValue() const
		{
			return mValue;
		}

		StateT &operator=( const ValueT &value )
		{
			setValue( value );
			return *this;
		}

	private:
		bool   mDirty = true;
		ValueT mValue = ValueT();
	};

	struct DynamicStates
	{
		StateT<bool>		depthWrite	= StateT<bool>( false );
		StateT<bool>		depthTest	= StateT<bool>( false );
		StateT<bool>		stencilTest = StateT<bool>( false );
		StateT<VkFrontFace> frontFace	= StateT<VkFrontFace>( VK_FRONT_FACE_COUNTER_CLOCKWISE );
	};

	class DescriptorState
	{
	public:
		DescriptorState();
		~DescriptorState() {}

		void bindUniformBuffer( uint32_t bindingNumber, const vk::Buffer *buffer );
		void bindCombinedImageSampler( uint32_t bindingNumber, const vk::ImageView *imageView, const vk::Sampler *sampler );

	private:
		struct BufferInfo
		{
			const vk::Buffer *buffer;
		};

		struct ImageInfo
		{
			const vk::ImageView *imageView;
			const vk::Sampler	  *sampler;
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

			Descriptor( uint32_t aBindingNumber, const vk::Buffer *aBuffer )
				: type( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ), bindingNumber( aBindingNumber ), bufferInfo( { aBuffer } ) {}

			Descriptor( uint32_t aBindingNumber, const vk::ImageView *aImageView, const vk::Sampler *aSampler )
				: type( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ), bindingNumber( aBindingNumber ), imageInfo( { aImageView, aSampler } ) {}
		};

		std::map<uint32_t, Descriptor> mDescriptors;

		friend class Context;
	};

	uint32_t			  mNumFramesInFlight   = 0;
	uint32_t			  mWidth			   = 0;
	uint32_t			  mHeight			   = 0;
	std::vector<VkFormat> mRenderTargetFormats = {};
	VkFormat			  mDepthStencilFormat  = VK_FORMAT_UNDEFINED;
	VkSampleCountFlagBits mSampleCount		   = VK_SAMPLE_COUNT_1_BIT;

	CommandPoolRef			 mCommandPool;
	std::vector<Frame>		 mFrames;
	uint64_t				 mFrameCount		 = 0;
	uint32_t				 mFrameIndex		 = 0;
	uint32_t				 mPreviousFrameIndex = 0;
	vk::CountingSemaphoreRef mFrameSyncSemaphore;

	// descriptor bindig -> combined image / sampler
	std::map<uint32_t, std::vector<const vk::TextureBase *>> mTextureBindingStack;
	// This stores the descriptor binding number for textures starting from 0
	std::vector<uint32_t>									 mActiveTextureStack;

	ClearValues							 mClearValues = {};
	DynamicStates						 mDynamicStates;
	std::vector<std::pair<ivec2, ivec2>> mViewportStack;
	std::vector<std::pair<ivec2, ivec2>> mScissorStack;
	std::vector<ci::mat4>				 mModelMatrixStack;
	std::vector<ci::mat4>				 mViewMatrixStack;
	std::vector<ci::mat4>				 mProjectionMatrixStack;

	std::vector<std::pair<geom::BufferLayout, vk::BufferRef>> mVertexBuffers;
	vk::ShaderProgRef										  mShaderProgram;
	vk::Pipeline::GraphicsPipelineCreateInfo				  mGraphicsState;
	uint64_t												  mCurrentGraphicsPipelineHash;

	vk::DescriptorSetLayoutRef			mDefaultSetLayout;
	vk::PipelineLayoutRef				mDefaultPipelineLayout;
	DescriptorState						mDescriptorState;
	vk::PipelineRef						mGraphicsPipeline;
	std::map<uint64_t, vk::PipelineRef> mGraphicsPipelines;
};

} // namespace cinder::vk
