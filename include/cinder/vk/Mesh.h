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

		std::vector<geom::AttribInfo> &		 getAttribs() { return mAttribInfos; }
		const std::vector<geom::AttribInfo> &getAttribs() const { return mAttribInfos; }
		//! Clears all attributes in the Layout
		void clearAttribs() { mAttribInfos.clear(); }

		uint32_t getStride() const;

	protected:
		bool						  mInterleave;
		std::vector<geom::AttribInfo> mAttribInfos;

		friend class BufferedMesh;
	};

	////! Creates a VboMesh which represents the geom::Source \a source. Layout is derived from the contents of \a source.
	//static BufferedMeshRef create( const geom::Source &source, vk::DeviceRef device = vk::DeviceRef() );
	////! Creates a VboMesh which represents the geom::Source \a source using \a layout.
	//static BufferedMeshRef create( const geom::Source &source, const geom::AttribSet &requestedAttribs, vk::DeviceRef device = vk::DeviceRef() );
	//! Creates a VboMesh which represents the geom::Source \a source using 1 or more VboMesh::Layouts for vertex data.
	static BufferedMeshRef create( const geom::Source &source, const std::vector<BufferedMesh::Layout> &vertexBufferLayouts, vk::DeviceRef device = vk::DeviceRef() );
	////! Creates a VboMesh which represents the geom::Source \a source using 1 or more Vbo/VboMesh::Layout pairs. A null VboRef requests allocation.
	//static BufferedMeshRef create( const geom::Source &source, const std::vector<std::pair<BufferedMesh::Layout, vk::BufferRef>> &vertexBuffers, vk::BufferRef indexBuffer = vk::BufferRef() );
	////! Creates a VboMesh which represents the user's vertex buffer objects. Allows optional \a indexVbo to enable indexed vertices; creates a static index VBO if none provided.
	//static BufferedMeshRef create( uint32_t numVertices, const std::vector<std::pair<geom::BufferLayout, vk::BufferRef>> &vertexBuffers, uint32_t numIndices = 0, VkIndexType indexType = VK_INDEX_TYPE_UINT16, vk::BufferRef indexBuffer = vk::BufferRef() );
	////! Creates a VboMesh which represents the user's vertex buffer objects. Allows optional \a indexVbo to enable indexed vertices; creates a static index VBO if none provided.
	//static BufferedMeshRef create( uint32_t numVertices, const std::vector<Layout> &vertexBuffers, uint32_t numIndices = 0, VkIndexType indexType = VK_INDEX_TYPE_UINT16, vk::BufferRef indexBuffer = vk::BufferRef() );

	uint8_t	getAttribDims( geom::Attrib attr ) const;

	const std::vector<std::pair<geom::BufferLayout, vk::BufferRef>> &getVertexBuffers() const { return mVertexBuffers; }

private:
	BufferedMesh( vk::DeviceRef device, const geom::Source &source, const std::vector<BufferedMesh::Layout> &vertexBuffers );

private:
	uint32_t												  mNumVertices = 0;
	uint32_t												  mNumIndices = 0;
	std::vector<std::pair<geom::BufferLayout, vk::BufferRef>> mVertexBuffers;
	vk::BufferRef											  mIndices;
	VkIndexType												  mIndexType  = VK_INDEX_TYPE_UINT16;

	friend class BufferedMeshGeomTarget;
};

} // namespace cinder::vk
