#pragma once

#include "cinder/app/Renderer.h"

namespace cinder::app {

class GrfxRenderer
	: public ci::app::Renderer
{
public:
	// clang-format off
	class Options {
	public:
		Options() {}

		Options&	msaa( uint32_t samples ) { mSamples = samples; return *this; }

		uint32_t	getMsaa() const  { return mSamples; }
	private:
		uint32_t mSamples = 1;
	};
	// clang-format on

protected:
	GrfxRenderer() {}

public:
	virtual ~GrfxRenderer() {}

#if defined( CINDER_MSW_DESKTOP )
	virtual HWND getHwnd() const override;
	virtual HDC	 getDc() const override;
#endif

protected:
#if defined( CINDER_MSW_DESKTOP )
	WindowImplMsw *mWindowImpl = nullptr;
#endif
};

} // namespace cinder::app
