#include "cinder/vk/draw.h"
#include "cinder/vk/Context.h"
#include "cinder/vk/Mesh.h"
#include "cinder/vk/wrapper.h"
#include "cinder/Log.h"

namespace cinder::vk {

void draw( const vk::BufferedMeshRef &mesh, int32_t first, int32_t count )
{
	auto ctx = vk::context();
	const vk::GlslProg* curGlslProg = ctx->getGlslProg();
	if( ! curGlslProg ) {
		CI_LOG_E( "No shader program bound" );
		return;
	}

	//ctx->pushVao();
	//ctx->getDefaultVao()->replacementBindBegin();
	//mesh->buildVao( curGlslProg );
	//ctx->getDefaultVao()->replacementBindEnd();
	ctx->setDefaultShaderVars();
	ctx->bindDefaultDescriptorSet();
	ctx->bindIndexBuffers( mesh );
	ctx->bindVertexBuffers( mesh );
	ctx->bindGraphicsPipeline();
	//mesh->drawImpl( first, count );
	//ctx->popVao();

	if ( count < 0 ) {
		// count = mMesh->getNumVertices();
		count = mesh->getNumIndices();
	}

	// ctx->draw( first, count );
	ctx->drawIndexed( first, count );
}

} // namespace cinder::vk
