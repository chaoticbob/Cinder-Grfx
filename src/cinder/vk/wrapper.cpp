#include "cinder/vk/wrapper.h"
#include "cinder/vk/Command.h"
#include "cinder/vk/Context.h"

namespace cinder::vk {

Context *context()
{
	return Context::getCurrentContext();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Command buffer functions

void setRenderPass( const std::vector<const vk::ImageView *> &colorAttachments, const vk::ImageView *pDepthStencilAttachment )
{
	vk::CommandBuffer::RenderingInfo ri = {};

	for ( const auto &elem : colorAttachments ) {
		ri.addColorAttachment( elem );
	}

	if ( pDepthStencilAttachment != nullptr ) {
		ri.setDepthStencilAttachment( pDepthStencilAttachment );
	}

	auto ctx = vk::context();
	ctx->getCommandBuffer()->beginRendering( ri );
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL style functions

void clear( const ColorA &color, bool clearDepthBuffer )
{
	clearColor( color );
	if ( clearDepthBuffer ) {
		clear( VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT );
	}
	else {
		clear( VK_IMAGE_ASPECT_COLOR_BIT );
	}
}

void clear( VkImageAspectFlags mask )
{
	auto ctx = vk::context();

	if ( mask & VK_IMAGE_ASPECT_COLOR_BIT ) {
	}

	if ( ( mask & VK_IMAGE_ASPECT_DEPTH_BIT ) || ( mask & VK_IMAGE_ASPECT_STENCIL_BIT ) ) {
	}
}

void clearColor( const ColorA &color )
{
	auto ctx = vk::context();
	ctx->clearColor( color );
}

void clearDepth( const double depth )
{
	auto ctx = vk::context();
	ctx->clearDepth( static_cast<float>( depth ) );
}

void clearDepth( const float depth )
{
	auto ctx = vk::context();
	ctx->clearDepth( depth );
}

void clearStencil( const int stencil )
{
	auto ctx = vk::context();
	ctx->clearStencil( stencil );
}

void disableDepthRead()
{
	auto ctx = vk::context();
	ctx->enableDepthTest( false );
}

void disableDepthWrite()
{
	auto ctx = vk::context();
	ctx->enableDepthWrite( false );
}

void enableDepthRead( bool enable )
{
	auto ctx = vk::context();
	ctx->enableDepthTest( enable );
}

void enableDepthWrite( bool enable )
{
	auto ctx = vk::context();
	ctx->enableDepthWrite( enable );
}

void enableDepth( bool enable )
{
	enableDepthRead( enable );
	enableDepthWrite( enable );
}

void enableStencilTest( bool enable )
{
	auto ctx = vk::context();
	ctx->enableStencilTest( enable );
}

void disableStencilTest()
{
	auto ctx = vk::context();
	ctx->enableStencilTest( false );
}

void setMatrices( const ci::Camera &cam )
{
	auto ctx							   = vk::context();
	ctx->getViewMatrixStack().back()	   = cam.getViewMatrix();
	ctx->getProjectionMatrixStack().back() = cam.getProjectionMatrix();
	ctx->getModelMatrixStack().back()	   = mat4();
}

void setModelMatrix( const ci::mat4 &m )
{
	auto ctx						  = vk::context();
	ctx->getModelMatrixStack().back() = m;
}

void setViewMatrix( const ci::mat4 &m )
{
	auto ctx						 = vk::context();
	ctx->getViewMatrixStack().back() = m;
}

void setProjectionMatrix( const ci::mat4 &m )
{
	auto ctx							   = vk::context();
	ctx->getProjectionMatrixStack().back() = m;
}

void pushModelMatrix()
{
	auto ctx = vk::context();
	ctx->getModelMatrixStack().push_back( ctx->getModelMatrixStack().back() );
}

void popModelMatrix()
{
	auto ctx = vk::context();
	ctx->getModelMatrixStack().pop_back();
}

void pushViewMatrix()
{
	auto ctx = vk::context();
	ctx->getViewMatrixStack().push_back( ctx->getViewMatrixStack().back() );
}

void popViewMatrix()
{
	auto ctx = vk::context();
	ctx->getViewMatrixStack().pop_back();
}

void pushProjectionMatrix()
{
	auto ctx = vk::context();
	ctx->getProjectionMatrixStack().push_back( ctx->getProjectionMatrixStack().back() );
}

void popProjectionMatrix()
{
	auto ctx = vk::context();
	ctx->getProjectionMatrixStack().pop_back();
}

void pushModelView()
{
	auto ctx = vk::context();
	ctx->getModelMatrixStack().push_back( ctx->getModelMatrixStack().back() );
	ctx->getViewMatrixStack().push_back( ctx->getViewMatrixStack().back() );
}

void popModelView()
{
	auto ctx = vk::context();
	ctx->getModelMatrixStack().pop_back();
	ctx->getViewMatrixStack().pop_back();
}

void pushMatrices()
{
	auto ctx = vk::context();
	ctx->getModelMatrixStack().push_back( ctx->getModelMatrixStack().back() );
	ctx->getViewMatrixStack().push_back( ctx->getViewMatrixStack().back() );
	ctx->getProjectionMatrixStack().push_back( ctx->getProjectionMatrixStack().back() );
}

void popMatrices()
{
	auto ctx = vk::context();
	ctx->getModelMatrixStack().pop_back();
	ctx->getViewMatrixStack().pop_back();
	ctx->getProjectionMatrixStack().pop_back();
}

void multModelMatrix( const ci::mat4 &mtx )
{
	auto ctx = vk::context();
	ctx->getModelMatrixStack().back() *= mtx;
}

void multViewMatrix( const ci::mat4 &mtx )
{
	auto ctx = vk::context();
	ctx->getViewMatrixStack().back() *= mtx;
}

void multProjectionMatrix( const ci::mat4 &mtx )
{
	auto ctx = vk::context();
	ctx->getProjectionMatrixStack().back() *= mtx;
}

} // namespace cinder::vk
