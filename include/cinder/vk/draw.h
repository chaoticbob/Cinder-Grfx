#pragma

#include "cinder/Camera.h"
#include "cinder/GeomIo.h"

namespace cinder::vk {

using BufferedMeshRef = std::shared_ptr<class BufferedMesh>;

//! Draws the VboMesh \a mesh. Consider a vk::Batch as a faster alternative. Optionally specify a \a first vertex index and a \a count of vertices.
CI_API void draw( const vk::BufferedMeshRef &mesh, int32_t first = 0, int32_t count = -1 );

} // namespace cinder::vk
