#include "cinder/vk/Mesh.h"
#include "cinder/app/RendererVk.h"
#include "cinder/Log.h"

namespace cinder::vk {

/////////////////////////////////////////////////////////////////////////////////////////////////
// BufferedMeshGeomTarget

class BufferedMeshGeomTarget : public geom::Target
{
public:
	struct BufferData
	{
		BufferData( const geom::BufferLayout &layout, uint8_t *data, size_t dataSize )
			: mLayout( layout ), mData( data ), mDataSize( dataSize )
		{
		}
		BufferData( BufferData &&rhs )
			: mLayout( rhs.mLayout ), mData( std::move( rhs.mData ) ), mDataSize( rhs.mDataSize )
		{
		}

		geom::BufferLayout		   mLayout;
		std::unique_ptr<uint8_t[]> mData;
		size_t					   mDataSize;
	};

	BufferedMeshGeomTarget( geom::Primitive prim, vk::BufferedMesh *pMesh )
		: mPrimitive( prim ), mMesh( pMesh )
	{
		// this may be replaced later with a copyIndices call
		mMesh->mNumIndices = 0;

		// create a vector of temporary data that parallels the VboMesh's vertexData
		for ( const auto &vertexBuffer : mMesh->getVertexBuffers() ) {
			size_t requiredBytes = vertexBuffer.first.calcRequiredStorage( mMesh->mNumVertices );
			mBufferData.push_back( BufferData( vertexBuffer.first, new uint8_t[requiredBytes], requiredBytes ) );
		}
	}

	virtual geom::Primitive getPrimitive() const;
	uint8_t					getAttribDims( geom::Attrib attr ) const override;
	void					copyAttrib( geom::Attrib attr, uint8_t dims, size_t strideBytes, const float *srcData, size_t count ) override;
	void					copyIndices( geom::Primitive primitive, const uint32_t *source, size_t numIndices, uint8_t requiredBytesPerIndex ) override;

	//! Must be called in order to upload temporary 'mBufferData' to VBOs
	void copyBuffers();

protected:
	geom::Primitive			mPrimitive;
	std::vector<BufferData> mBufferData;
	vk::BufferedMesh *		mMesh;
};

geom::Primitive BufferedMeshGeomTarget::getPrimitive() const
{
	return mPrimitive;
}

uint8_t BufferedMeshGeomTarget::getAttribDims( geom::Attrib attr ) const
{
	return mMesh->getAttribDims( attr );
}

void BufferedMeshGeomTarget::copyAttrib( geom::Attrib attr, uint8_t dims, size_t /*strideBytes*/, const float *srcData, size_t count )
{
	// if we don't have it we don't want it
	if ( getAttribDims( attr ) == 0 ) {
		return;
	}

	// we need to find which element of 'mBufferData' containts 'attr'
	uint8_t *dstData = nullptr;
	uint8_t	 dstDims;
	size_t	 dstStride, dstDataSize;
	for ( const auto &bufferData : mBufferData ) {
		if ( bufferData.mLayout.hasAttrib( attr ) ) {
			auto attrInfo = bufferData.mLayout.getAttribInfo( attr );
			dstDims		  = attrInfo.getDims();
			dstStride	  = attrInfo.getStride();
			dstData		  = bufferData.mData.get() + attrInfo.getOffset();
			dstDataSize	  = bufferData.mDataSize;
			break;
		}
	}
	CI_ASSERT( dstData );

	// verify we've been called with the number of vertices we were promised earlier
	if ( count != mMesh->mNumVertices ) {
		CI_LOG_E( "copyAttrib() called with " << count << " elements. " << mMesh->mNumVertices << " expected." );
		return;
	}

	// verify we have room for this data
	auto testDstStride = dstStride ? dstStride : ( dstDims * sizeof( float ) );
	if ( dstDataSize < count * testDstStride ) {
		CI_LOG_E( "copyAttrib() called with inadequate attrib data storage allocated" );
		return;
	}

	if ( dstData ) {
		geom::copyData( dims, srcData, count, dstDims, dstStride, reinterpret_cast<float *>( dstData ) );
	}
}

void BufferedMeshGeomTarget::copyIndices( geom::Primitive /*primitive*/, const uint32_t *source, size_t numIndices, uint8_t requiredBytesPerIndex )
{
	mMesh->mNumIndices = (uint32_t)numIndices;
	if ( mMesh->mNumIndices == 0 ) {
		mMesh->mIndices.reset();
	}
	else if ( requiredBytesPerIndex <= 2 ) {
		const uint64_t srcDataSize = numIndices * sizeof( uint16_t );
		mMesh->mIndexType		   = VK_INDEX_TYPE_UINT16;
		std::unique_ptr<uint16_t[]> indices( new uint16_t[numIndices] );
		copyIndexData( source, numIndices, indices.get() );
		if ( !mMesh->mIndices ) {
			vk::Buffer::Usage usage = vk::Buffer::Usage().indexBuffer().transferDst();
			mMesh->mIndices			= vk::Buffer::create( srcDataSize, indices.get(), usage, vk::MemoryUsage::GPU_ONLY, mMesh->getDevice() );
		}
		else {
			mMesh->mIndices->copyData( srcDataSize, indices.get() );
		}
	}
	else {
		const uint64_t srcDataSize = numIndices * sizeof( uint32_t );
		mMesh->mIndexType		   = VK_INDEX_TYPE_UINT32;
		std::unique_ptr<uint32_t[]> indices( new uint32_t[numIndices] );
		copyIndexData( source, numIndices, indices.get() );
		if ( !mMesh->mIndices ) {
			vk::Buffer::Usage usage = vk::Buffer::Usage().indexBuffer().transferDst();
			mMesh->mIndices			= vk::Buffer::create( srcDataSize, indices.get(), usage, vk::MemoryUsage::GPU_ONLY, mMesh->getDevice() );
		}
		else {
			mMesh->mIndices->copyData( srcDataSize, indices.get() );
		}
	}
}

void BufferedMeshGeomTarget::copyBuffers()
{
	// iterate all the buffers in mBufferData and upload them to the corresponding VBO in the VboMesh
	for ( auto bufferDataIt = mBufferData.begin(); bufferDataIt != mBufferData.end(); ++bufferDataIt ) {
		auto vertexArrayIt = mMesh->mVertexBuffers.begin() + std::distance( mBufferData.begin(), bufferDataIt );
		vertexArrayIt->second->copyData( bufferDataIt->mDataSize, bufferDataIt->mData.get() );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BufferedMesh::Layout

BufferedMesh::Layout &BufferedMesh::Layout::attrib( geom::Attrib attrib, uint8_t dims )
{
	return this->attrib( geom::AttribInfo( attrib, dims, 0, 0 ) );
}

BufferedMesh::Layout &BufferedMesh::Layout::attrib( const geom::AttribInfo &attribInfo )
{
	geom::Attrib attrib = attribInfo.getAttrib();

	auto it = std::find_if(
		mAttribInfos.begin(),
		mAttribInfos.end(),
		[attrib]( const geom::AttribInfo &elem ) -> bool {
			return elem.getAttrib() == attrib;
		} );

	if ( it == mAttribInfos.end() ) {
		mAttribInfos.push_back( attribInfo );
	}
	else {
		*it = attribInfo;
	}

	return *this;
}

uint32_t BufferedMesh::Layout::getStride() const
{
	uint32_t stride = 0;
	for ( const auto &elem : mAttribInfos ) {
		stride += static_cast<uint32_t>( elem.getStride() );
	}
	return stride;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BufferedMesh

//BufferedMeshRef BufferedMesh::create( const geom::Source &source, vk::DeviceRef device )
//{
//	if ( !device ) {
//		device = app::RendererVk::getCurrentRenderer()->getDevice();
//	}
//
//	return BufferedMeshRef();
//}
//
//BufferedMeshRef BufferedMesh::create( const geom::Source &source, const geom::AttribSet &requestedAttribs, vk::DeviceRef device )
//{
//	if ( !device ) {
//		device = app::RendererVk::getCurrentRenderer()->getDevice();
//	}
//
//	return BufferedMeshRef();
//}

BufferedMeshRef BufferedMesh::create( const geom::Source &source, const std::vector<BufferedMesh::Layout> &vertexBufferLayouts, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return BufferedMeshRef( new BufferedMesh( device, source, vertexBufferLayouts ) );
}

BufferedMesh::BufferedMesh( vk::DeviceRef device, const geom::Source &source, const std::vector<BufferedMesh::Layout> &vertexBufferLayouts )
	: vk::DeviceChildObject( device )
{
	// determine the requestedAttribs by iterating all the Layouts
	geom::AttribSet requestedAttribs;
	for ( const auto &vertexBufferLayout : vertexBufferLayouts ) {
		for ( const auto &attribInfo : vertexBufferLayout.getAttribs() ) {
			requestedAttribs.insert( attribInfo.getAttrib() );
		}
	}

	for ( const auto &vertexBufferLayout : vertexBufferLayouts ) {
		uint64_t		  size	= vertexBufferLayout.getStride() * source.getNumVertices();
		vk::Buffer::Usage usage = vk::Buffer::Usage().vertexBuffer().transferDst();

		geom::BufferLayout bufferLayout = geom::BufferLayout( vertexBufferLayout.getAttribs() );

		vk::BufferRef buffer = vk::Buffer::create( size, usage, vk::MemoryUsage::GPU_ONLY, device );
		mVertexBuffers.push_back( std::make_pair( bufferLayout, buffer ) );
	}

	BufferedMeshGeomTarget target( source.getPrimitive(), this );
	source.loadInto( &target, requestedAttribs );
	// we need to let the target know it can copy from its internal buffers to our vertexData VBOs
	target.copyBuffers();
}

uint8_t BufferedMesh::getAttribDims( geom::Attrib attr ) const
{
	for ( const auto &vertexBuffer : mVertexBuffers ) {
		// now iterate the attributes associated with this VBO
		for ( const auto &attribInfo : vertexBuffer.first.getAttribs() ) {
			if ( attribInfo.getAttrib() == attr )
				return attribInfo.getDims();
		}
	}

	// not found
	return 0;
}

} // namespace cinder::vk
