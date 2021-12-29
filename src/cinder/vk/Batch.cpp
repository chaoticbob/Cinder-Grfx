#include "cinder/vk/Batch.h"
#include "cinder/vk/GlslProg.h"
#include "cinder/vk/Mesh.h"
#include "cinder/vk/wrapper.h"
#include "cinder/app/RendererVk.h"

namespace cinder::vk {

BatchRef Batch::create( const geom::Source &source, const vk::ShaderProgRef &shaderProg, const AttributeMapping &attributeMapping, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return BatchRef( new Batch( device, source, shaderProg, attributeMapping ) );
}

Batch::Batch( vk::DeviceRef device, const geom::Source &source, const vk::ShaderProgRef &shaderProg, const AttributeMapping &attributeMapping )
	: vk::DeviceChildObject( device ),
	  mShaderProg( shaderProg )
{
	geom::AttribSet attribs;
	// include all the attributes in the custom attributeMapping
	for ( const auto &attrib : attributeMapping ) {
		if ( source.getAttribDims( attrib.first ) )
			attribs.insert( attrib.first );
	}
	// and then the attributes references by the GLSL
	for ( const auto &attrib : shaderProg->getVertexAttributes() ) {
		if ( source.getAttribDims( attrib.getSemantic() ) ) {
			attribs.insert( attrib.getSemantic() );
		}
	}
	mMesh = vk::BufferedMesh::create( source, attribs );

	/*
	BufferedMesh::Layout bufferLayout = BufferedMesh::Layout();

	// Shader program expects vertex data for all vertex attributes.
	// BufferedMesh will skip copeis for all vertex attributes
	// not found in source (i.e. dims = 0). Just keep in mind that
	// missing data for vertex attribute will result in rendering
	// errors.
	//
	size_t		offset		  = 0;
	const auto &vertexAttribs = shaderProg->getVertexAttributes();
	for ( size_t i = 0; i < vertexAttribs.size(); ++i ) {
		const vk::VertexAttribute &vertexAttrib = vertexAttribs[i];

		uint8_t dims = source.getAttribDims( vertexAttrib.semantic );
		//if ( dims == 0 ) {
		//	continue;
		//}

		size_t stride = dims * sizeof( float );

		geom::AttribInfo attribInfo = geom::AttribInfo( vertexAttrib.semantic, dims, stride, offset );
		offset += stride;

		bufferLayout.attrib( attribInfo );
	}

	mMesh = vk::BufferedMesh::create( source, std::vector<BufferedMesh::Layout>{ bufferLayout }, device );
*/
}

Batch::~Batch()
{
}

void Batch::draw( int32_t first, int32_t count )
{
	auto ctx = vk::context();
	ctx->bindShaderProg( mShaderProg );
	ctx->setDefaultShaderVars();
	ctx->bindDefaultDescriptorSet();
	ctx->bindIndexBuffers( mMesh );
	ctx->bindVertexBuffers( mMesh );
	ctx->bindGraphicsPipeline();

	/*
	auto vertexBuffersPairs = mMesh->getVertexBuffers();

	std::vector<vk::BufferRef> vertexBuffers;
	for ( auto &elem : vertexBuffersPairs ) {
		vertexBuffers.push_back( elem.second );
	}

	if (count < 0) {
		count = mMesh->getNumVertices();
	}

	ctx->bindVertexBuffers(0, vertexBuffers);
	*/

	if ( count < 0 ) {
		//count = mMesh->getNumVertices();
		count = mMesh->getNumIndices();
	}

	//ctx->draw( first, count );
	ctx->drawIndexed( first, count );
}

} // namespace cinder::vk
