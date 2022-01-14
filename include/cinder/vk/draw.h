#pragma

#include "cinder/vk/vk_config.h"

#include "cinder/Camera.h"
#include "cinder/GeomIo.h"

namespace cinder::vk {

using BufferedMeshRef = std::shared_ptr<class BufferedMesh>;

//! Draws the VboMesh \a mesh. Consider a vk::Batch as a faster alternative. Optionally specify a \a first vertex index and a \a count of vertices.
CI_API void draw( const vk::BufferedMeshRef &mesh, int32_t first = 0, int32_t count = -1 );
//! Draws a Texture2d \a texture, fitting it to \a dstRect. Ignores currently bound shader.
CI_API void draw( const vk::Texture2dRef &texture, const Rectf &dstRect );
//! Draws a subregion \a srcArea of a Texture (expressed as upper-left origin pixels).
CI_API void draw( const vk::Texture2dRef &texture, const Area &srcArea, const Rectf &dstRect );
CI_API void draw( const vk::Texture2dRef &texture, const vec2 &dstOffset = vec2() );

} // namespace cinder::vk
