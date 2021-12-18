#pragma once

#include "cinder/app/GrfxRenderer.h"

namespace cinder::app {

typedef std::shared_ptr<class RendererDx12> RendererDx12Ref;

class RendererDx12
	: public GrfxRenderer
{
public:
	// clang-format off
	class Options : public GrfxRenderer::Options
	{
	public:
		Options() {}

		Options&	msaa( int samples ) { GrfxRenderer::Options::msaa(samples); return *this; }
	};
	// clang-format on

protected:
	RendererDx12( const RendererDx12& renderer );

public:
	RendererDx12( const Options& options = Options() );
	virtual ~RendererDx12() {}

	RendererRef clone() const override { return RendererDx12Ref( new RendererDx12( *this ) ); }

#if defined( CINDER_MSW_DESKTOP )
	virtual void setup( WindowImplMsw* windowImpl, RendererRef sharedRenderer ) override;
	virtual void kill() override;
#elif defined( CINDER_LINUX )
#if defined( CINDER_HEADLESS )
	virtual void setup( ci::ivec2 renderSize, RendererRef sharedRenderer ) override;
#else
	virtual void setup( void* nativeWindow, RendererRef sharedRenderer ) override;
#endif
#endif

	const Options& getOptions() const;

	Surface8u copyWindowSurface( const Area& area, int32_t windowHeightPixels ) override;

private:
	Options mOptions;

	struct Impl;
	std::shared_ptr<Impl> mImpl;
};

} // namespace cinder::app
