#pragma once

#include "cinder/vk/Buffer.h"
#include "cinder/vk/DeviceChildObject.h"

#include "cinder/Color.h"
#include "cinder/Vector.h"
#include "cinder/TriMesh.h"
#include "cinder/GeomIo.h"

namespace cinder::vk {

class BufferedMesh
	: vk::DeviceChildObject
{
public:
	class CI_API Layout
	{
	public:
		Layout()
			: mInterleave( true ) {}

		// clang-format off
		//! Specifies whether the data is stored planar or interleaved. Deafult is interleaved.
		Layout&		interleave( bool interleave = true ) { mInterleave = interleave; return *this; }
		//! Returns whether the Layout stores data as interleaved (rather than planar)
		bool		getInterleave() const { return mInterleave; }
		//! Appends an attribute of semantic \a attrib which is \a dims-dimensional. Replaces AttribInfo if it exists for \a attrib
		Layout&		attrib( geom::Attrib attrib, uint8_t dims );
		//! Appends an attribute using a geom::AttribInfo. Replaces AttribInfo if it exists for \a attribInfo.getAttrib()
		Layout&		attrib( const geom::AttribInfo &attribInfo );
		// clang-format on

		std::vector<geom::AttribInfo>		  &getAttribs() { return mAttribInfos; }
		const std::vector<geom::AttribInfo> &getAttribs() const { return mAttribInfos; }
		//! Clears all attributes in the Layout
		void								 clearAttribs() { mAttribInfos.clear(); }

		bool hasAttrib( geom::Attrib attrib ) const;

	protected:
		//! If \a resultVbo is null then no VBO is allocated
		void allocate( vk::DeviceRef device, size_t numVertices, geom::BufferLayout *resultBufferLayout, vk::BufferRef *resultVertexBuffer ) const;

		bool						  mInterleave;
		std::vector<geom::AttribInfo> mAttribInfos;

		friend class vk::BufferedMesh;
	};

	//! Creates a BufferedMesh which represents the geom::Source \a source using 1 or more BufferedMesh::Layouts for vertex data.
	static vk::BufferedMeshRef create( const geom::Source &source, const std::vector<vk::BufferedMesh::Layout> &layouts, vk::DeviceRef device = nullptr );
	//! Creates a BufferedMesh which represents the geom::Source \a source using \a layout.
	static vk::BufferedMeshRef create( const geom::Source &source, const vk::BufferedMesh::Layout &layout, vk::DeviceRef device = nullptr );

	////! Creates a VboMesh which represents the geom::Source \a source. Layout is derived from the contents of \a source.
	// static BufferedMeshRef create( const geom::Source &source, vk::DeviceRef device = vk::DeviceRef() );
	//! Creates a VboMesh which represents the geom::Source \a source using \a layout.
	//static vk::BufferedMeshRef create( const geom::Source &source, const geom::AttribSet &requestedAttribs, vk::DeviceRef device = nullptr );
	//! Creates a VboMesh which represents the geom::Source \a source using 1 or more VboMesh::Layouts for vertex data.
	// static vk::BufferedMeshRef create( const geom::Source &source, const std::vector<BufferedMesh::Layout> &vertexBufferLayouts, vk::DeviceRef device = nullptr );
	////! Creates a VboMesh which represents the geom::Source \a source using 1 or more Vbo/VboMesh::Layout pairs. A null VboRef requests allocation.
	// static BufferedMeshRef create( const geom::Source &source, const std::vector<std::pair<BufferedMesh::Layout, vk::BufferRef>> &vertexBuffers, vk::BufferRef indexBuffer = vk::BufferRef() );
	////! Creates a VboMesh which represents the user's vertex buffer objects. Allows optional \a indexVbo to enable indexed vertices; creates a static index VBO if none provided.
	// static BufferedMeshRef create( uint32_t numVertices, const std::vector<std::pair<geom::BufferLayout, vk::BufferRef>> &vertexBuffers, uint32_t numIndices = 0, VkIndexType indexType = VK_INDEX_TYPE_UINT16, vk::BufferRef indexBuffer = vk::BufferRef() );
	////! Creates a VboMesh which represents the user's vertex buffer objects. Allows optional \a indexVbo to enable indexed vertices; creates a static index VBO if none provided.
	// static BufferedMeshRef create( uint32_t numVertices, const std::vector<Layout> &vertexBuffers, uint32_t numIndices = 0, VkIndexType indexType = VK_INDEX_TYPE_UINT16, vk::BufferRef indexBuffer = vk::BufferRef() );

	//! Returns the number of vertices in the mesh
	uint32_t getNumVertices() const { return mNumVertices; }
	//! Returns the number of indices for indexed geometry, otherwise 0
	uint32_t getNumIndices() const { return mNumIndices; }

	uint8_t getAttribDims( geom::Attrib attr ) const;

	vk::BufferRef getIndices() const { return mIndices; }

	const std::vector<std::pair<geom::BufferLayout, vk::BufferRef>> &getVertexBuffers() const { return mVertexBuffers; }

	VkIndexType getIndexType() const { return mIndexType; }

private:
	BufferedMesh( vk::DeviceRef device, const geom::Source &source, std::vector<std::pair<Layout, vk::BufferRef>> vertexBuffers, const vk::BufferRef &indexBuffer );

private:
	uint32_t												  mNumVertices = 0;
	uint32_t												  mNumIndices  = 0;
	std::vector<std::pair<geom::BufferLayout, vk::BufferRef>> mVertexBuffers;
	vk::BufferRef											  mIndices;
	VkPrimitiveTopology										  mPrimitive;
	VkIndexType												  mIndexType = VK_INDEX_TYPE_UINT16;

	friend class BufferedMeshGeomTarget;
};

} // namespace cinder::vk
