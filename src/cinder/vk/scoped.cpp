#include "cinder/vk/scoped.h"
#include "cinder/vk/Context.h"

namespace cinder::vk {

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
