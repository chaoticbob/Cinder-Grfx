#pragma once

#include "cinder/vk/vk_config.h"

#include "cinder/Camera.h"
#include "cinder/CinderGlm.h"
#include "cinder/Color.h"

namespace cinder::vk {

/////////////////////////////////////////////////////////////////////////////////////////////////
// Context functions

class Context *context();

/////////////////////////////////////////////////////////////////////////////////////////////////
// Command buffer functions

//CI_API void setRenderPass( const std::vector<const vk::ImageView*>& colorAttachments, const vk::ImageView* pDepthStencilAttachment = nullptr );

/////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL style functions

CI_API void clear( const ColorA &color = ColorA::black(), bool clearDepthBuffer = true );
	
CI_API void clear( VkImageAspectFlags mask );
CI_API void clearColor( const ColorA &color );
CI_API void clearDepth( const double depth );
CI_API void clearDepth( const float depth );
CI_API void clearStencil( const int s );

CI_API std::pair<ivec2, ivec2> getViewport();
CI_API void viewport( const std::pair<ivec2, ivec2> positionAndSize );
CI_API inline void viewport( int x, int y, int width, int height ) { viewport( std::pair<ivec2, ivec2>( ivec2( x, y ), ivec2( width, height ) ) ); }
CI_API inline void viewport( const ivec2 &position, const ivec2 &size ) { viewport( std::pair<ivec2, ivec2>( position, size ) ); }
CI_API inline void viewport( const ivec2 &size ) { viewport( ivec2(), size ); }
CI_API void pushViewport( const std::pair<ivec2, ivec2> positionAndSize );
CI_API inline void pushViewport() { pushViewport( getViewport() ); }
CI_API inline void pushViewport( int x, int y, int width, int height ) { pushViewport( std::pair<ivec2, ivec2>( ivec2( x, y ), ivec2( width, height ) ) ); }
CI_API inline void pushViewport( const ivec2 &position, const ivec2 &size ) { pushViewport( std::pair<ivec2, ivec2>( position, size ) ); }
CI_API inline void pushViewport( const ivec2 &size ) { pushViewport( ivec2(), size ); }
CI_API void popViewport();

CI_API std::pair<ivec2, ivec2> getScissor();
CI_API void scissor( const std::pair<ivec2, ivec2> positionAndSize );
CI_API inline void scissor( int x, int y, int width, int height ) { scissor( std::pair<ivec2, ivec2>( ivec2( x, y ), ivec2( width, height ) ) ); }
CI_API inline void scissor( const ivec2 &position, const ivec2 &size ) { scissor( std::pair<ivec2, ivec2>( position, size ) ); }

CI_API void enableBlending( bool enable = true, uint32_t attachmentIndex = 0 );
CI_API inline void disableBlending( uint32_t attachmentIndex = 0) { enableBlending( false, attachmentIndex ); }
CI_API void enableAlphaBlending( bool enable = true, uint32_t attachmentIndex = 0 );
CI_API void enableAlphaBlendingPremult( uint32_t attachmentIndex = 0);
CI_API inline void disableAlphaBlending( uint32_t attachmentIndex = 0) { disableBlending(); }
CI_API void enableAdditiveBlending(uint32_t attachmentIndex = 0 );

//! Disables reading / testing from the depth buffer. Disables \c GL_DEPTH_TEST
CI_API void disableDepthRead();
//! Disables writing to depth buffer; analogous to calling glDepthMask( GL_FALSE );
CI_API void disableDepthWrite();
//! Enables or disables reading / testing from depth buffer; analogous to setting \c GL_DEPTH_TEST to \p enable
CI_API void enableDepthRead( bool enable = true );
//! Enables or disables writing to depth buffer; analogous to calling glDepthMask( \p enable ); Note that reading must also be enabled for writing to have any effect.
CI_API void enableDepthWrite( bool enable = true );
//! Enables or disables writing to and reading / testing from depth buffer
CI_API void enableDepth( bool enable = true );

//! Enables or disables the stencil test operation, which controls reading and writing to the stencil buffer. Analagous to `glEnable( GL_STENCIL_TEST, enable );`
CI_API void enableStencilTest( bool enable = true );
//! Disables the stencil test operation. Analagous to `glEnable( GL_STENCIL_TEST, false );`
CI_API void disableStencilTest();

//! Sets the View and Projection matrices based on a Camera
CI_API void setMatrices( const ci::Camera &cam );
CI_API void setModelMatrix( const ci::mat4 &m );
CI_API void setViewMatrix( const ci::mat4 &m );
CI_API void setProjectionMatrix( const ci::mat4 &m );
CI_API void pushModelMatrix();
CI_API void popModelMatrix();
CI_API void pushViewMatrix();
CI_API void popViewMatrix();
CI_API void pushProjectionMatrix();
CI_API void popProjectionMatrix();
//! Pushes Model and View matrices
CI_API void pushModelView();
//! Pops Model and View matrices
CI_API void popModelView();
//! Pushes Model, View and Projection matrices
CI_API void pushMatrices();
//! Pops Model, View and Projection matrices
CI_API void popMatrices();
CI_API void multModelMatrix( const ci::mat4 &mtx );
CI_API void multViewMatrix( const ci::mat4 &mtx );
CI_API void multProjectionMatrix( const ci::mat4 &mtx );

CI_API mat4 getModelMatrix();
CI_API mat4 getViewMatrix();
CI_API mat4 getProjectionMatrix();
CI_API mat4 getModelView();
CI_API mat4 getModelViewProjection();
CI_API mat4 calcViewMatrixInverse();
CI_API mat3 calcModelMatrixInverseTranspose();
CI_API mat3 calcNormalMatrix();
CI_API mat4 calcViewportMatrix();

CI_API void setMatricesWindowPersp( int screenWidth, int screenHeight, float fovDegrees = 60.0f, float nearPlane = 1.0f, float farPlane = 1000.0f, bool originUpperLeft = true );
CI_API void setMatricesWindowPersp( const ci::ivec2 &screenSize, float fovDegrees = 60.0f, float nearPlane = 1.0f, float farPlane = 1000.0f, bool originUpperLeft = true );
CI_API void setMatricesWindow( int screenWidth, int screenHeight, bool originUpperLeft = true );
CI_API void setMatricesWindow( const ci::ivec2 &screenSize, bool originUpperLeft = true );

CI_API void rotate( const quat &quat );
//! Rotates the Model matrix by \a angleRadians around the \a axis
CI_API void rotate( float angleRadians, const ci::vec3 &axis );
//! Rotates the Model matrix by \a angleRadians around the axis (\a x,\a y,\a z)
CI_API inline void rotate( float angleRadians, float xAxis, float yAxis, float zAxis ) { rotate( angleRadians, ci::vec3(xAxis, yAxis, zAxis) ); }
//! Rotates the Model matrix by \a zRadians around the z-axis
CI_API inline void rotate( float zRadians ) { rotate( zRadians, vec3( 0, 0, 1 ) ); }

//! Scales the Model matrix by \a v
CI_API void scale( const ci::vec3 &v );
//! Scales the Model matrix by (\a x,\a y, \a z)
CI_API inline void scale( float x, float y, float z ) { scale( vec3( x, y, z ) ); }
//! Scales the Model matrix by \a v
CI_API inline void scale( const ci::vec2 &v ) { scale( vec3( v.x, v.y, 1 ) ); }
//! Scales the Model matrix by (\a x,\a y, 1)
CI_API inline void scale( float x, float y ) { scale( vec3( x, y, 1 ) ); }

//! Translates the Model matrix by \a v
CI_API void translate( const ci::vec3 &v );
//! Translates the Model matrix by (\a x,\a y,\a z )
CI_API inline void translate( float x, float y, float z ) { translate( vec3( x, y, z ) ); }
//! Translates the Model matrix by \a v
CI_API inline void translate( const ci::vec2 &v ) { translate( vec3( v, 0 ) ); }
//! Translates the Model matrix by (\a x,\a y)
CI_API inline void translate( float x, float y ) { translate( vec3( x, y, 0 ) ); }

CI_API void color( float r, float g, float b );
CI_API void color( float r, float g, float b, float a );
CI_API void color( const ci::Color &c );
CI_API void color( const ci::ColorA &c );
CI_API void color( const ci::Color8u &c );
CI_API void color( const ci::ColorA8u &c );

} // namespace cinder::vk
