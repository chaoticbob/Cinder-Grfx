#include "cinder/vk/Batch.h"
#include "cinder/vk/GlslProg.h"
#include "cinder/vk/Mesh.h"
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

		uint8_t dims   = source.getAttribDims( vertexAttrib.semantic );
		size_t	stride = dims * sizeof( float );

		geom::AttribInfo attribInfo = geom::AttribInfo( vertexAttrib.semantic, dims, stride, offset );
		offset += stride;

		bufferLayout.attrib( attribInfo );
	}

	mMesh = vk::BufferedMesh::create( source, std::vector<BufferedMesh::Layout>{ bufferLayout }, device );
}

Batch::~Batch()
{
}

void Batch::draw( int32_t first, int32_t count )
{
}

} // namespace cinder::vk
