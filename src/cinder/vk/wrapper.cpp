#include "cinder/vk/wrapper.h"
#include "cinder/vk/Command.h"
#include "cinder/vk/Context.h"
#include "cinder/vk/Image.h"

namespace cinder::vk {

Context *context()
{
	return Context::getCurrentContext();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Command buffer functions

//void setRenderPass( const std::vector<const vk::ImageView *> &colorAttachments, const vk::ImageView *pDepthStencilAttachment )
//{
//	vk::CommandBuffer::RenderingInfo ri = {};
//
//	for ( const auto &elem : colorAttachments ) {
//		ri.addColorAttachment( elem );
//	}
//
//	if ( pDepthStencilAttachment != nullptr ) {
//		ri.setDepthStencilAttachment( pDepthStencilAttachment );
//	}
//
//	auto ctx = vk::context();
//	ctx->getCommandBuffer()->beginRendering( ri );
//}

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
		ctx->clearColorAttachment( 0 );
	}

	if ( ( mask & VK_IMAGE_ASPECT_DEPTH_BIT ) || ( mask & VK_IMAGE_ASPECT_STENCIL_BIT ) ) {
		ctx->clearDepthStencilAttachment( mask );
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

std::pair<ivec2, ivec2> getViewport()
{
	auto ctx = vk::context();
	auto view = ctx->getViewport();
	return view;
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

mat4 getModelMatrix()
{
	auto ctx = vk::context();
	return ctx->getModelMatrixStack().back();
}

mat4 getViewMatrix()
{
	auto ctx = vk::context();
	return ctx->getViewMatrixStack().back();
}

mat4 getProjectionMatrix()
{
	auto ctx = vk::context();
	return ctx->getProjectionMatrixStack().back();
}

mat4 getModelView()
{
	auto ctx = context();
	return ctx->getViewMatrixStack().back() * ctx->getModelMatrixStack().back();
}

mat4 getModelViewProjection()
{
	auto ctx = context();
	return ctx->getProjectionMatrixStack().back() * ctx->getViewMatrixStack().back() * ctx->getModelMatrixStack().back();
}

mat4 calcViewMatrixInverse()
{
	return glm::inverse( getViewMatrix() );
}

mat3 calcNormalMatrix()
{
	return glm::inverseTranspose( glm::mat3( getModelView() ) );
}
	
mat3 calcModelMatrixInverseTranspose()
{
	auto m = glm::inverseTranspose( getModelMatrix() );
	return mat3( m );
}
	
mat4 calcViewportMatrix()
{
	auto curViewport = vk::getViewport();
	
	const float a = ( curViewport.second.x - curViewport.first.x ) / 2.0f;
	const float b = ( curViewport.second.y - curViewport.first.y ) / 2.0f;
	const float c = 1.0f / 2.0f;
	
	const float tx = ( curViewport.second.x + curViewport.first.x ) / 2.0f;
	const float ty = ( curViewport.second.y + curViewport.second.y ) / 2.0f;
	const float tz = 1.0f / 2.0f;
	
	return mat4(
		a, 0, 0, 0,
		0, b, 0, 0,
		0, 0, c, 0,
		tx, ty, tz, 1
	);
}

} // namespace cinder::vk
