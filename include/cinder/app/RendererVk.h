#pragma once

#include "cinder/app/GrfxRenderer.h"
#include "cinder/vk/Context.h"

#include <vector>

namespace cinder::app {

typedef std::shared_ptr<class RendererVk> RendererVkRef;

class RendererVk
	: public GrfxRenderer
{
public:
	// clang-format off
	class Options : public GrfxRenderer::Options
	{
	public:
		Options(uint32_t apiVersion = VK_API_VERSION_1_1) : mApiVersion(apiVersion) {}

		Options&	apiVersion(uint32_t apiVersion) { mApiVersion = apiVersion; return *this; }
		Options&	enableValidation(bool value = true) { mEnableValidation = value ; return *this;}
		Options&	addInstanceLayer(const std::string& value) { mInstanceLayers.push_back(value); return *this; }
		Options&	addInstanceExtension(const std::string& value) { mInstanceExtensions.push_back(value); return *this; }
		Options&	addDeviceExtension(const std::string& value) { mDeviceExtensions.push_back(value); return *this; }
		Options&	gpuIndex(uint32_t index) { mGpuIndex = index; return*this; }
		Options&	enableComputeQueue(bool value = true) { mEnableComputeQueue = value; return *this; }
		Options&	enableTransferQueue(bool value = true) { mEnableTransferQueue = value; return *this; }
		Options&	numFramesInFlight(uint32_t value) { mNumFramesInFlight = value; return *this; }

		Options&	msaa( int samples ) { GrfxRenderer::Options::msaa(samples); return *this; }

		uint32_t						getApiVersion() const { return mApiVersion; }
		bool							getEnableValidation() const { return mEnableValidation; }
		const std::vector<std::string>&	getInstanceLayers() const { return mInstanceLayers; }
		const std::vector<std::string>&	getInstanceExtensions() const { return mInstanceExtensions; }
		const std::vector<std::string>&	getDeviceExtensions() const { return mDeviceExtensions; }
		uint32_t						getGpuIndex() const { return mGpuIndex; }
		bool							getEnableComputeQueue() const { return mEnableComputeQueue; }
		bool							getEnableTransferQueue() const { return mEnableTransferQueue; }
		uint32_t						getNumFramesInFlight() const { return mNumFramesInFlight; }

	private:
		uint32_t					mApiVersion = VK_API_VERSION_1_1;
		bool						mEnableValidation = false;
		std::vector<std::string>	mInstanceLayers;
		std::vector<std::string>	mInstanceExtensions;
		std::vector<std::string>	mDeviceExtensions;
		uint32_t					mGpuIndex = 0; // Physical device index
		bool						mEnableComputeQueue = false;
		bool						mEnableTransferQueue = false;
		uint32_t					mNumFramesInFlight = 2;
	};
	// clang-format on

protected:
	RendererVk( const RendererVk &renderer );

public:
	RendererVk( const Options &options = Options() );
	virtual ~RendererVk();

	// Returns current thread's renderer, makeCurrentContext() must have been called before on same thread.
	static RendererVk *getCurrentRenderer();

	// Return pointer to renderer's device
	vk::DeviceRef getDevice() const;

	//! Return pointer to renderer's swapchain, null if cloned renderer
	vk::SwapchainRef getSwapchain() const;

	// Clone this renderer, cloned renderers do not have a swapchain
	RendererRef clone() const override;

#if defined( CINDER_MSW_DESKTOP )
	virtual void setup( WindowImplMsw *windowImpl, RendererRef sharedRenderer ) override;
#elif defined( CINDER_LINUX )
#if defined( CINDER_HEADLESS )
	virtual void setup( ci::ivec2 renderSize, RendererRef sharedRenderer ) override;
#else
	virtual void setup( void *nativeWindow, RendererRef sharedRenderer ) override;
#endif
#endif
	virtual void kill() override;

	const Options &getOptions() const;

	void	  startDraw() override;
	void	  finishDraw() override;
	void	  defaultResize() override;
	void	  makeCurrentContext( bool force = false ) override;
	void	  swapBuffers() override;
	Surface8u copyWindowSurface( const Area &area, int32_t windowHeightPixels ) override;

private:
	void setupDevice( const std::string &appName, RendererRef sharedRenderer );
	void setupFrames( uint32_t windowWidth, uint32_t windowHeight );

private:
	static thread_local RendererVk *sCurrentRenderer;

	Options			 mOptions;
	vk::DeviceRef	 mDevice;
	vk::ContextRef	 mContext;
	vk::SwapchainRef mSwapchain;

	struct Frame;
	std::vector<Frame>		 mFrames;
	vk::CountingSemaphoreRef mFrameSync;
	vk::CommandPoolRef		 mCommandPool;

	std::function<void( Renderer * )> mStartDrawFn;
	std::function<void( Renderer * )> mFinishDrawFn;
};

} // namespace cinder::app
