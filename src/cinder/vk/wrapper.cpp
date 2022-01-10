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

void setMatricesWindowPersp( int screenWidth, int screenHeight, float fovDegrees, float nearPlane, float farPlane, bool originUpperLeft )
{
	auto ctx = vk::context();

	CameraPersp cam( screenWidth, screenHeight, fovDegrees, nearPlane, farPlane );
	ctx->getModelMatrixStack().back() = mat4();
	ctx->getProjectionMatrixStack().back() = cam.getProjectionMatrix();
	ctx->getViewMatrixStack().back() = cam.getViewMatrix();
	if( originUpperLeft ) {
		ctx->getViewMatrixStack().back() *= glm::scale( vec3( 1, -1, 1 ) );								// invert Y axis so increasing Y goes down.
		ctx->getViewMatrixStack().back() *= glm::translate( vec3( 0, (float) - screenHeight, 0 ) );		// shift origin up to upper-left corner.
	}
}

void setMatricesWindowPersp( const ci::ivec2& screenSize, float fovDegrees, float nearPlane, float farPlane, bool originUpperLeft )
{
	setMatricesWindowPersp( screenSize.x, screenSize.y, fovDegrees, nearPlane, farPlane, originUpperLeft );
}

void setMatricesWindow( int screenWidth, int screenHeight, bool originUpperLeft )
{
	auto ctx = vk::context();
	ctx->getModelMatrixStack().back() = mat4();
	ctx->getViewMatrixStack().back() = mat4();

	float sx = 2.0f / (float)screenWidth;
	float sy = 2.0f / (float)screenHeight;
	float ty = -1;

	if( originUpperLeft ) {
		sy *= -1;
		ty *= -1;
	}

	mat4 &m = ctx->getProjectionMatrixStack().back();
	m = mat4( sx, 0,  0, 0,
			  0, sy,  0, 0,
			  0,  0, -1, 0,
			 -1, ty,  0, 1 );
}

void setMatricesWindow( const ci::ivec2& screenSize, bool originUpperLeft )
{
	setMatricesWindow( screenSize.x, screenSize.y, originUpperLeft );
}

void rotate( const quat &quat )
{
	auto ctx = vk::context();
	ctx->getModelMatrixStack().back() *= toMat4( quat );
}

void rotate( float angleRadians, const vec3 &axis )
{
	if( math<float>::abs( angleRadians ) > EPSILON_VALUE ) {
		auto ctx = vk::context();
		ctx->getModelMatrixStack().back() *= glm::rotate( angleRadians, axis );
	}
}

void scale( const ci::vec3& v )
{
	auto ctx = vk::context();
	ctx->getModelMatrixStack().back() *= glm::scale( v );
}

void translate( const ci::vec3& v )
{
	auto ctx = vk::context();
	ctx->getModelMatrixStack().back() *= glm::translate( v );
}

void color( float r, float g, float b )
{
	auto ctx = vk::context();
	ctx->setCurrentColor( ColorAf( r, g, b, 1.0f ) );
}

void color( float r, float g, float b, float a )
{
	auto ctx = vk::context();
	ctx->setCurrentColor( ColorAf( r, g, b, a ) );
}

void color( const ci::Color &c )
{
	auto ctx = vk::context();
	ctx->setCurrentColor( c );
}

void color( const ci::ColorA &c )
{
	auto ctx = vk::context();
	ctx->setCurrentColor( c );
}

void color( const ci::Color8u &c )
{
	auto ctx = vk::context();
	ctx->setCurrentColor( c );
}

void color( const ci::ColorA8u &c )
{
	auto ctx = vk::context();
	ctx->setCurrentColor( c );
}


} // namespace cinder::vk
