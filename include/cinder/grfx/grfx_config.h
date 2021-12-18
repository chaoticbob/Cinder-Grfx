#pragma once

#include "cinder/Cinder.h"
#include "cinder/CinderGlm.h"
#include "cinder/Exception.h"
#include "cinder/app/Renderer.h"
#include "cinder/grfx/Constants.h"
#include "cinder/grfx/Enums.h"

namespace cinder::app {

class GrfxRenderer;

} // namespace cinder::app

namespace cinder::grfx {

class CI_API GraphicsApiExc : public cinder::Exception
{
public:
	GraphicsApiExc( const std::string &description )
		: Exception( description ) {}
};

class CI_API TextureExc : public cinder::Exception
{
public:
	TextureExc( const std::string &description )
		: Exception( description ) {}
};

class RendererChildObject
{
public:
	virtual ~RendererChildObject() {}

	cinder::app::GrfxRenderer *getRenderer() const { return mRenderer; }

protected:
	RendererChildObject( cinder::app::GrfxRenderer *pRenderer )
		: mRenderer( pRenderer ) {}

private:
	cinder::app::GrfxRenderer *mRenderer = nullptr;
};

template <typename T>
T invalidValue( T value = static_cast<T>( ~0 ) )
{
	return value;
}

inline uint32_t roundUpPow2( uint32_t v )
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

} // namespace cinder::grfx
