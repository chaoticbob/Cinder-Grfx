#pragma once

#include "cinder/vk/DeviceChildObject.h"
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
	// If depth and stencil is combined, then depth resources
	// and stencil resources are ignored.
	//
	struct Frame
	{
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

		// clang-format off
		Options &numInFlightFrames( uint32_t value ) { mNumInFlightFrames = std::max<uint32_t>( 1, value ); return *this; }
		Options &setRenderTarget( VkFormat format ) { mRenderTargetFormat = format; return *this; }
		Options &setDepthStencilFormat( VkFormat format ) { mDepthFormat = format; mStencilFormat = format; return *this; }
		Options &setDepthFormat( VkFormat format ) { mDepthFormat = format; return *this; }
		Options &setStencilFormat( VkFormat format ) { mStencilFormat = format; return *this; }
		Options &addRenderTarget( VkFormat format ) { mAdditionalRenderTargets.push_back( format ); return *this; }
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
		const vk::Semaphore *semaphore;
		uint64_t			 value;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct SemaphoreList
	{
		SemaphoreList() {}
		SemaphoreList( const Semaphore *semaphore ) { addSemaphore( semaphore ); }
		SemaphoreList( const Semaphore *semaphore, uint64_t value ) { addSemaphore( semaphore, value ); }

		// clang-format off
		//! Adds a binary semaphore 
		SemaphoreList &addSemaphore( const Semaphore *semaphore ) { mSemaphores.push_back( { semaphore, 0 } ); return *this; }
		//! Adds a timeline semaphore 
		SemaphoreList &addSemaphore( const Semaphore *semaphore, uint64_t value ) { mSemaphores.push_back( { semaphore, value } ); return *this; }
		// clang-format on

	private:
		std::vector<SemaphoreInfo> mSemaphores;

		friend class Context;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	~Context();

	static ContextRef create( const Options &options = Options(), vk::DeviceRef device = vk::DeviceRef() );
	static ContextRef create( uint32_t width, uint32_t height, const Options &options = Options(), vk::DeviceRef device = vk::DeviceRef() );

	void			makeCurrent();
	static Context *getCurrentContext();
	void			submit( const SemaphoreList &waits = SemaphoreList(), const SemaphoreList &signals = SemaphoreList() );
	void			waitForCompletion();

	bool isRenderable() const { return ( mWidth > 0 ) && ( mHeight > 0 ); }

	void clearColor( const ColorA &color ) { mClearState.get().color = color; }
	void clearDepth( const float depth ) { mClearState.get().depth = depth; }
	void clearStencil( const int stencil ) { mClearState.get().stencil = static_cast<uint8_t>( stencil ); }

	void enableDepthWrite( bool enable ) { mDepthState.get().depthWrite = enable; }
	void enableDepthTest( bool enable ) { mDepthState.get().depthTest = enable; }
	void enableStencilTest( bool enable ) { mStencilState.get().stencilTest = enable; }

	std::vector<ci::mat4> &getModelMatrixStack() { return mModelMatrixStack; }
	std::vector<ci::mat4> &getViewMatrixStack() { return mViewMatrixStack; }
	std::vector<ci::mat4> &getProjectionMatrixStack() { return mProjectionMatrixStack; }

	vk::CommandBuffer *getCommandBuffer() const { return getCurrentFrame().commandBuffer.get(); }

	uint32_t			 getNumRenderTargets() const { return countU32( mRenderTargetFormats ); }
	const vk::ImageView *getRenderTargetView( uint32_t index ) const { return getCurrentFrame().rtvs[index].get(); }
	const vk::ImageView *getDepthStencilView() const { return mCombinedDepthStencil ? getCurrentFrame().dtv.get() : nullptr; }
	const vk::ImageView *getDepthTargetView() const { return getCurrentFrame().dtv.get(); }
	const vk::ImageView *getStencilTargetView() const { return getCurrentFrame().stv.get(); }

private:
	Context( vk::DeviceRef device, uint32_t width, uint32_t height, const Options &options );

	void		 initializeFrame( vk::CommandBufferRef commandBuffer, Frame &frame );
	Frame &		 getCurrentFrame();
	const Frame &getCurrentFrame() const;

private:
	struct ClearState
	{
		ColorA	color	= ColorA( 0, 0, 0, 0 );
		float	depth	= 0.0f;
		uint8_t stencil = 0xFF;
	};

	struct DepthState
	{
		bool depthWrite = false;
		bool depthTest	= false;
	};

	struct StencilState
	{
		bool stencilTest = false;
	};

	template <typename StateT>
	class StateStack
	{
	public:
		StateStack( uint32_t initialSize = 1 )
			: mStack( 1 ), mIndex( 0 ) {}

		~StateStack() {}

		void set( const StateT &state )
		{
			mStatck[mIndex] = state;
		}

		StateT &get()
		{
			return mStack[mIndex];
		}

		const StateT &get() const
		{
			return mStack[mIndex];
		}

		void push()
		{
			uint32_t newIndex = mIndex + 1;
			if ( newIndex >= mStack.size() ) {
				StateT current = get();
				mStack.emplace_back( current );
			}
			mIndex = newIndex;
		}

		void pop()
		{
			if ( mIndex == 0 ) {
				return;
			}
			--mIndex;
		}

	private:
		std::vector<StateT> mStack;
		uint32_t			mIndex;
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
	uint64_t				 mFrameCount = 0;
	uint32_t				 mFrameIndex = 0;
	vk::CountingSemaphoreRef mFrameSyncSemaphore;

	StateStack<ClearState>	 mClearState;
	StateStack<DepthState>	 mDepthState;
	StateStack<StencilState> mStencilState;
	std::vector<ci::mat4>	 mModelMatrixStack		= std::vector<ci::mat4>( 1 );
	std::vector<ci::mat4>	 mViewMatrixStack		= std::vector<ci::mat4>( 1 );
	std::vector<ci::mat4>	 mProjectionMatrixStack = std::vector<ci::mat4>( 1 );
};

} // namespace cinder::vk
