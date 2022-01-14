#include "cinder/vk/draw.h"
#include "cinder/vk/Command.h"
#include "cinder/vk/Context.h"
#include "cinder/vk/Mesh.h"
#include "cinder/vk/Pipeline.h"
#include "cinder/vk/Texture.h"
#include "cinder/vk/scoped.h"
#include "cinder/vk/wrapper.h"
#include "cinder/Log.h"

namespace cinder::vk {

void draw( const vk::BufferedMeshRef &mesh, int32_t first, int32_t count )
{
	auto				ctx			= vk::context();
	const vk::GlslProg *curGlslProg = ctx->getGlslProg();
	if ( !curGlslProg ) {
		CI_LOG_E( "No shader program bound" );
		return;
	}

	// ctx->pushVao();
	// ctx->getDefaultVao()->replacementBindBegin();
	// mesh->buildVao( curGlslProg );
	// ctx->getDefaultVao()->replacementBindEnd();
	ctx->setDefaultShaderVars();
	ctx->bindDefaultDescriptorSet();
	ctx->bindIndexBuffers( mesh );
	ctx->bindVertexBuffers( mesh );
	ctx->bindGraphicsPipeline();
	// mesh->drawImpl( first, count );
	// ctx->popVao();

	if ( count < 0 ) {
		// count = mMesh->getNumVertices();
		count = mesh->getNumIndices();
	}

	// ctx->draw( first, count );
	ctx->drawIndexed( first, count );
}

void draw( const vk::Texture2dRef &texture, const Area &srcArea, const Rectf &dstRect )
{
	if ( !texture )
		return;

	auto ctx = context();

	Rectf texRect = texture->getAreaTexCoords( srcArea );

	auto			   prog = ctx->getStockShaderManager()->getDrawTextureProg( texture->getUnnormalizedCoordinates() );
	vk::ScopedGlslProg glslScp( prog );

	auto pipelineLayout = ctx->getStockShaderManager()->getDrawTexturePipelineLayout();

	auto modelViewProjection = vk::getModelViewProjection();
	ctx->getCommandBuffer()->pushConstants( pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( modelViewProjection ), &modelViewProjection );

	auto uPositionOffset = dstRect.getUpperLeft();
	auto uPositionScale	 = dstRect.getSize();
	auto uTexCoordOffset = texRect.getUpperLeft();
	auto uTexCoordScale	 = texRect.getSize();

	ctx->getCommandBuffer()->pushConstants( pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof( modelViewProjection ) + 0 * sizeof( vec2 ), sizeof(vec2), &uPositionOffset );
	ctx->getCommandBuffer()->pushConstants( pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof( modelViewProjection ) + 1 * sizeof( vec2 ), sizeof(vec2), &uPositionScale );
	ctx->getCommandBuffer()->pushConstants( pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof( modelViewProjection ) + 2 * sizeof( vec2 ), sizeof(vec2), &uTexCoordOffset );
	ctx->getCommandBuffer()->pushConstants( pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof( modelViewProjection ) + 3 * sizeof( vec2 ), sizeof(vec2), &uTexCoordScale );

	ctx->getCommandBuffer()->pushDescriptor(
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0 + CINDER_CONTEXT_PS_BINDING_SHIFT_TEXTURE,
		0,
		texture.get() );

	ctx->bindGraphicsPipeline( pipelineLayout );
	ctx->getCommandBuffer()->draw( 6, 1, 0, 0 );

	// ScopedVao vaoScp( ctx->getDrawTextureVao() );
	// ScopedBuffer vboScp( ctx->getDrawTextureVbo() );
	// ScopedTextureBind texBindScope( texture );
	//
	// auto glsl = getStockShader( ShaderDef().uniformBasedPosAndTexCoord().color().texture( texture ) );
	// ScopedGlslProg glslScp( glsl );
	// glsl->uniform( "uTex0", 0 );
	// glsl->uniform( "uPositionOffset", dstRect.getUpperLeft() );
	// glsl->uniform( "uPositionScale", dstRect.getSize() );
	// glsl->uniform( "uTexCoordOffset", texRect.getUpperLeft() );
	// glsl->uniform( "uTexCoordScale", texRect.getSize() );
	//
	// ctx->setDefaultShaderVars();
	// ctx->drawArrays( GL_TRIANGLE_STRIP, 0, 4 );
}

void draw( const vk::Texture2dRef &texture, const Rectf &dstRect )
{
	if ( !texture )
		return;

	draw( texture, texture->getBounds(), dstRect );
}

void draw( const vk::Texture2dRef &texture, const vec2 &dstOffset )
{
	if ( !texture )
		return;

	draw( texture, texture->getBounds(), Rectf( texture->getBounds() ) + dstOffset );
}

} // namespace cinder::vk
