#include "cinder/vk/Mesh.h"
#include "cinder/vk/Util.h"
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
			vk::Buffer::Usage usage = vk::Buffer::Usage().indexBuffer().transferSrc().transferDst();
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
			vk::Buffer::Usage usage = vk::Buffer::Usage().indexBuffer().transferSrc().transferDst();
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

/*
uint32_t BufferedMesh::Layout::getStride() const
{
	uint32_t stride = 0;
	for ( const auto &elem : mAttribInfos ) {
		stride += static_cast<uint32_t>( elem.getStride() );
	}
	return stride;
}
*/

void BufferedMesh::Layout::allocate( vk::DeviceRef device, size_t numVertices, geom::BufferLayout *resultBufferLayout, vk::BufferRef *resultVertexBuffer ) const
{
	auto attribInfos = mAttribInfos;

	// setup offsets and strides based on interleaved or planar
	size_t totalDataBytes;
	if ( mInterleave ) {
		size_t totalStride = 0;
		for ( const auto &attrib : attribInfos )
			totalStride += attrib.getByteSize();
		size_t currentOffset = 0;
		for ( auto &attrib : attribInfos ) {
			attrib.setOffset( currentOffset );
			attrib.setStride( totalStride );
			currentOffset += attrib.getByteSize();
		}
		totalDataBytes = currentOffset * numVertices;
	}
	else { // planar data
		size_t currentOffset = 0;
		for ( auto &attrib : attribInfos ) {
			attrib.setOffset( currentOffset );
			attrib.setStride( attrib.getByteSize() );
			currentOffset += attrib.getByteSize() * numVertices;
		}
		totalDataBytes = currentOffset;
	}

	*resultBufferLayout = geom::BufferLayout( attribInfos );

	if ( resultVertexBuffer ) {
		if ( *resultVertexBuffer ) { // non-null shared_ptr means the VBO should be resized
			( *resultVertexBuffer )->ensureMinimumSize( totalDataBytes );
		}
		else { // else allocate
			vk::Buffer::Usage usage = vk::Buffer::Usage().vertexBuffer().transferSrc().transferDst();
			*resultVertexBuffer		= vk::Buffer::create( totalDataBytes, usage, vk::MemoryUsage::GPU_ONLY, device );
		}
	}
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

BufferedMeshRef BufferedMesh::create( const geom::Source &source, const geom::AttribSet &requestedAttribs, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	// make an interleaved VboMesh::Layout with 'requestedAttribs'
	vk::BufferedMesh::Layout layout;
	for ( const auto &attrib : requestedAttribs ) {
		layout.attrib( attrib, 0 ); // 0 dim implies querying the Source for its dimension
	}

	return BufferedMeshRef( new BufferedMesh( device, source, { { layout, nullptr } }, nullptr ) );
}

BufferedMeshRef BufferedMesh::create( const geom::Source &source, const std::vector<BufferedMesh::Layout> &vertexBufferLayouts, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	std::vector<std::pair<vk::BufferedMesh::Layout, vk::BufferRef>> layoutVbos;
	for ( const auto &vertexArrayLayout : vertexBufferLayouts ) {
		layoutVbos.push_back( std::make_pair( vertexArrayLayout, ( vk::BufferRef ) nullptr ) );
	}

	return BufferedMeshRef( new BufferedMesh( device, source, layoutVbos, nullptr ) );
}

BufferedMesh::BufferedMesh( vk::DeviceRef device, const geom::Source &source, std::vector<std::pair<vk::BufferedMesh::Layout, vk::BufferRef>> vertexBuffers, const vk::BufferRef &indexBuffer )
	: vk::DeviceChildObject( device )
{
	// An empty vertexArrayBuffers implies we should just pull whatever attribs the Source is pushing. We arrived here from VboMesh::create( Source& )
	if ( vertexBuffers.empty() ) {
		// Create an interleaved Layout based on what's in the Source
		vertexBuffers.push_back( std::pair<Layout, vk::BufferRef>( Layout().interleave(), nullptr ) );
		for ( auto &attrib : source.getAvailableAttribs() ) {
			auto dims = source.getAttribDims( attrib );
			if ( dims > 0 ) {
				vertexBuffers.back().first.attrib( attrib, dims );
			}
		}
	}
	else {
		// For any attributes whose dims == 0, set the dims to be whatever dims the Source is pushing.
		for ( auto &layoutVbo : vertexBuffers ) {
			for ( auto &attribInfo : layoutVbo.first.getAttribs() ) {
				if ( attribInfo.getDims() == 0 ) {
					attribInfo.setDims( source.getAttribDims( attribInfo.getAttrib() ) );
				}
			}
		}
	}

	// determine the requestedAttribs by iterating all the Layouts
	geom::AttribSet requestedAttribs;
	for ( const auto &vertexArrayBuffer : vertexBuffers ) {
		for ( const auto &attribInfo : vertexArrayBuffer.first.getAttribs() ) {
			requestedAttribs.insert( attribInfo.getAttrib() );
		}
	}

	// Vertex count and primitive type
	mNumVertices = (uint32_t)source.getNumVertices();
	mPrimitive	 = toVkPrimitive( source.getPrimitive() );

	// iterate 'vertexArrayBuffers' and allocate mVertexArrayVbos, which is the parallel vector of <geom::BufferLayout,VboRef> pairs
	for ( const auto &vertexBuffer : vertexBuffers ) {
		geom::BufferLayout bufferLayout;
		vk::BufferRef	   buffer = vertexBuffer.second;
		// we pass nullptr for the VBO if we already have one, to prevent re-allocation by allocate()
		vertexBuffer.first.allocate( device, mNumVertices, &bufferLayout, &buffer );
		mVertexBuffers.push_back( make_pair( bufferLayout, buffer ) );
	}

	// Set our indices VBO to indexArrayVBO, which may well be empty, so that the target doesn't blow it away. Must do this before we loadInto().
	mIndices = indexBuffer;

	BufferedMeshGeomTarget target( source.getPrimitive(), this );
	source.loadInto( &target, requestedAttribs );
	// we need to let the target know it can copy from its internal buffers to our vertexData VBOs
	target.copyBuffers();

	/*
	// determine the requestedAttribs by iterating all the Layouts
	geom::AttribSet requestedAttribs;
	for ( size_t i = 0; i < vertexBufferLayouts.size(); ++i ) {
		auto &vertexBufferLayout = vertexBufferLayouts[i];
		for ( const auto &attribInfo : vertexBufferLayout.getAttribs() ) {
			requestedAttribs.insert( attribInfo.getAttrib() );
		}
	}

	// An empty vertexArrayBuffers implies we should just pull whatever attribs the Source is pushing. We arrived here from VboMesh::create( Source& )
	if( vertexArrayBuffers.empty() ) {
		// Create an interleaved Layout based on what's in the Source
		vertexArrayBuffers.push_back( std::pair<Layout,VboRef>( VboMesh::Layout().usage( GL_STATIC_DRAW ).interleave(), nullptr ) );
		for( auto &attrib : source.getAvailableAttribs() ) {
			auto dims = source.getAttribDims( attrib );
			if( dims > 0 )
				vertexArrayBuffers.back().first.attrib( attrib, dims );
		}
	}
	else {
		// For any attributes whose dims == 0, set the dims to be whatever dims the Source is pushing.
		for( auto &layoutVbo : vertexArrayBuffers ) {
			for( auto &attribInfo : layoutVbo.first.getAttribs() ) {
				if( attribInfo.getDims() == 0 )
					attribInfo.setDims( source.getAttribDims( attribInfo.getAttrib() ) );
			}
		}
	}

	// Vertex count and primitive type
	mNumVertices = (uint32_t)source.getNumVertices();
	mPrimitive	 = toVkPrimitive( source.getPrimitive() );

	for ( const auto &vertexBufferLayout : vertexBufferLayouts ) {
		const uint32_t	  stride	  = vertexBufferLayout.getStride();
		const uint32_t	  numVertices = static_cast<uint32_t>( source.getNumVertices() );
		uint64_t		  size		  = stride * numVertices;
		vk::Buffer::Usage usage		  = vk::Buffer::Usage().vertexBuffer().transferDst();

		geom::BufferLayout bufferLayout = geom::BufferLayout( vertexBufferLayout.getAttribs() );

		vk::BufferRef buffer = vk::Buffer::create( size, usage, vk::MemoryUsage::GPU_ONLY, device );
		mVertexBuffers.push_back( std::make_pair( bufferLayout, buffer ) );
	}

	BufferedMeshGeomTarget target( source.getPrimitive(), this );
	source.loadInto( &target, requestedAttribs );
	// we need to let the target know it can copy from its internal buffers to our vertexData VBOs
	target.copyBuffers();
*/
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
