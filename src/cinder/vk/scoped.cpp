#include "cinder/vk/scoped.h"
#include "cinder/vk/Context.h"

namespace cinder::vk {

///////////////////////////////////////////////////////////////////////////////////////////
// ScopedGlslProg
ScopedGlslProg::ScopedGlslProg( vk::GlslProgRef &prog )
	: mCtx( vk::context() )
{
	mCtx->pushGlslProg( prog.get() );
}

ScopedGlslProg::ScopedGlslProg( const std::shared_ptr<const vk::GlslProg> &prog )
	: mCtx( vk::context() )
{
	mCtx->pushGlslProg( std::const_pointer_cast<GlslProg>( prog ).get() );
}

ScopedGlslProg::ScopedGlslProg( const vk::GlslProg *prog )
	: mCtx( vk::context() )
{
	mCtx->pushGlslProg( const_cast<vk::GlslProg *>( prog ) );
}

ScopedGlslProg::~ScopedGlslProg()
{
	mCtx->popGlslProg();
}

///////////////////////////////////////////////////////////////////////////////////////////
// ScopedTextureBind
ScopedTextureBind::ScopedTextureBind( const vk::TextureBaseRef &texture )
	: mCtx( vk::context() )
{
	mBinding = mCtx->getActiveTexture();
	mCtx->pushTextureBinding( texture.get(), mBinding );
}

ScopedTextureBind::ScopedTextureBind( const vk::TextureBaseRef &texture, uint32_t binding )
	: mCtx( vk::context() )
{
	mCtx->pushTextureBinding( texture.get(), binding );
}

ScopedTextureBind::~ScopedTextureBind()
{
	mCtx->popTextureBinding( mBinding );
}

} // namespace cinder::vk
