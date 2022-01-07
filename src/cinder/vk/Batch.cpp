#include "cinder/vk/Batch.h"
#include "cinder/vk/GlslProg.h"
#include "cinder/vk/Mesh.h"
#include "cinder/vk/wrapper.h"
#include "cinder/app/RendererVk.h"
#include "cinder/Log.h"

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
	vk::BufferedMesh::Layout layout;

	// Add all attributes from the shader program.
	//
	const auto &shaderAttribs = shaderProg->getVertexAttributes();
	for ( auto &attrib : shaderAttribs ) {
		// Use the dims from the shader vertex attributes since
		// a dim mismatch could result in data underun or overrun.
		// If the dims between the shader and the source do not
		// match, then BufferdMesh will generate data instead of
		// copy it from the source. This is done to avoid a
		// potential crash.
		//
		geom::Attrib semantic = attrib.getSemantic();
		uint32_t	 dims	  = vk::formatComponentCount( attrib.getFormat() );
		layout.attrib( semantic, dims );
	}

	// Include all the attributes in the custom attributeMapping.
	//
	// 'shaderProg' may not make use of these attributes but
	// a later shader program, supplied by a replace call, might.
	// User is responsible for ensuring that the replacing
	// shader programs has an interface that's compatible with
	// any attributes it uses in 'attributeMapping'.
	//
	for ( const auto &attrib : attributeMapping ) {
		if ( !layout.hasAttrib( attrib.first ) ) {
			uint8_t dims = source.getAttribDims( attrib.first );
			if ( dims > 0 ) {
				layout.attrib( attrib.first, dims );
			}
		}
		else {
			CI_LOG_E( "attribute mapping has overlapping attribute with shader veretx attributes" );
		}
	}

	mMesh = vk::BufferedMesh::create( source, layout, device );

/*
	geom::AttribSet attribs;
	// include all the attributes in the custom attributeMapping
	for ( const auto &attrib : attributeMapping ) {
		if ( source.getAttribDims( attrib.first ) )
			attribs.insert( attrib.first );
	}
	// and then the attributes references by the shader
	for ( const auto &attrib : shaderProg->getVertexAttributes() ) {
		// Unlike OpenGL, Vulkan doesn't do any behind the scenes
		// tricks to make missing attributes work. If a shader
		// is expecting a vertex attribute, the application needs
		// to supply data for the vertex attribute or there will
		// be undefined behavior.
		//
		// In the case where the shader expects an attribute but
		// the source does not have data for it, we'll tell
		// BufferedMesh to generate data for it based on the
		// semantic and warn the user.
		//
	
		uint32_t dims = vk::formatComponentCount( attrib.getFormat() );
		if ( dims == 0 ) {
			throw VulkanExc( "FATAL: invalid format component count" );
		}
	
		// attribs.insert( attrib.getSemantic() );
	}
	mMesh = vk::BufferedMesh::create( source, attribs );
*/

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
		// count = mMesh->getNumVertices();
		count = mMesh->getNumIndices();
	}

	// ctx->draw( first, count );
	ctx->drawIndexed( first, count );
}

} // namespace cinder::vk
