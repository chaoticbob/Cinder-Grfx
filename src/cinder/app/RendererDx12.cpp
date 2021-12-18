#include "cinder/app/RendererDx12.h"

namespace cinder::app {

RendererDx12::RendererDx12( const RendererDx12& renderer )
	: mOptions( renderer.mOptions )
{
}

RendererDx12::RendererDx12( const Options& options )
	: mOptions( options )
{
}

#if defined( CINDER_MSW_DESKTOP )
void RendererDx12::setup( WindowImplMsw* windowImpl, RendererRef sharedRenderer )
{
	this->mWindowImpl = windowImpl;
	
	RendererDx12Ref sharedRendererDx12 = std::dynamic_pointer_cast<RendererDx12>(sharedRenderer);
	if (!sharedRenderer) {
		throw ExcRendererAllocation( "sharedRenderer is not a RendererDx12 object." );
	}

	this->mImpl = sharedRendererDx12->mImpl;
}

void RendererDx12::kill()
{
}
#elif defined( CINDER_LINUX )
#if defined( CINDER_HEADLESS )
void RendererDx12::setup( ci::ivec2 renderSize, RendererRef sharedRenderer )
{
}
#else
void RendererDx12::setup( void* nativeWindow, RendererRef sharedRenderer )
{
}
#endif
#endif

const RendererDx12::Options& RendererDx12::getOptions() const
{
	return mOptions;
}

Surface8u RendererDx12::copyWindowSurface( const Area& area, int32_t windowHeightPixels )
{
	return Surface8u();
}

} // namespace cinder::app
