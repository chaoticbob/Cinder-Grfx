#include "cinder/app/GrfxRenderer.h"

#if defined( CINDER_MSW_DESKTOP )
	#include "cinder/app/msw/AppImplMsw.h"
#endif

namespace cinder::app {

HWND GrfxRenderer::getHwnd() const
{
	return mWindowImpl->getHwnd();
}

HDC GrfxRenderer::getDc() const
{
	return mWindowImpl->getDc();
}

} // namespace cinder::app
