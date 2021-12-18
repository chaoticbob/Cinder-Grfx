#pragma once

#include "cinder/vk/vk_config.h"

#include "cinder/Camera.h"
#include "cinder/CinderGlm.h"
#include "cinder/Color.h"

namespace cinder::vk {

class Context;

Context *context();

/////////////////////////////////////////////////////////////////////////////////////////////////
// Command buffer functions

CI_API void setRenderPass( const std::vector<const vk::ImageView*>& colorAttachments, const vk::ImageView* pDepthStencilAttachment = nullptr );

/////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL style functions

CI_API void clear( const ColorA &color = ColorA::black(), bool clearDepthBuffer = true );
	
CI_API void clear( VkImageAspectFlags mask );
CI_API void clearColor( const ColorA &color );
CI_API void clearDepth( const double depth );
CI_API void clearDepth( const float depth );
CI_API void clearStencil( const int s );

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

} // namespace cinder::vk
