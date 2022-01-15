#pragma once

#include "cinder/app/Renderer.h"

namespace cinder::app {

typedef std::shared_ptr<class RendererDx12> RendererDx12Ref;

class RendererDx12
	: public ci::app::Renderer
{
public:
	// clang-format off
	class Options
	{
	public:
		Options() {}

		Options& msaa( uint32_t samples ) { mSamples = samples; return *this; }

		uint32_t getMsaa() const { return mSamples; }

	private:
		uint32_t mSamples = 1;
	};
	// clang-format on

protected:
	RendererDx12( const RendererDx12 &renderer );

public:
	RendererDx12( const Options &options = Options() );
	virtual ~RendererDx12() {}

	RendererRef clone() const override { return RendererDx12Ref( new RendererDx12( *this ) ); }

	virtual void setup( WindowImplMsw *windowImpl, RendererRef sharedRenderer ) override;
	virtual void kill() override;
	virtual HWND getHwnd() const override;
	virtual HDC	 getDc() const override;

	const Options &getOptions() const;

	Surface8u copyWindowSurface( const Area &area, int32_t windowHeightPixels ) override;

private:
	Options		   mOptions;
	WindowImplMsw *mWindowImpl = nullptr;

	struct Impl;
	std::shared_ptr<Impl> mImpl;
};

} // namespace cinder::app
