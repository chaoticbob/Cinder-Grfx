#pragma once

#include "cinder/vk/wrapper.h"
#include "cinder/Noncopyable.h"

namespace cinder::vk {

struct CI_API ScopedModelMatrix : private Noncopyable {
	ScopedModelMatrix() { vk::pushModelMatrix(); }
	explicit ScopedModelMatrix( const mat4 &m )
	{
		vk::pushModelMatrix();
		vk::setModelMatrix( m );
	}
	~ScopedModelMatrix() { vk::popModelMatrix(); }
};

struct CI_API ScopedViewMatrix : private Noncopyable {
	ScopedViewMatrix() { vk::pushViewMatrix(); }
	explicit ScopedViewMatrix( const mat4 &m )
	{
		vk::pushViewMatrix();
		vk::setViewMatrix( m );
	}
	~ScopedViewMatrix() { vk::popViewMatrix(); }
};

struct CI_API ScopedProjectionMatrix : private Noncopyable {
	ScopedProjectionMatrix() { vk::pushProjectionMatrix(); }
	explicit ScopedProjectionMatrix( const mat4 &m )
	{
		vk::pushProjectionMatrix();
		vk::setProjectionMatrix( m );
	}
	~ScopedProjectionMatrix() { vk::popProjectionMatrix(); }
};

//! Preserves all matrices
struct CI_API ScopedMatrices : private Noncopyable {
	ScopedMatrices() { vk::pushMatrices(); }
	explicit ScopedMatrices( const Camera &cam )
	{
		vk::pushMatrices();
		vk::setMatrices( cam );
	}
	~ScopedMatrices() { vk::popMatrices(); }
};

} // namespace cinder::vk
