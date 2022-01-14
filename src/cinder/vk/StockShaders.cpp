#include "cinder/vk/StockShaders.h"
#include "cinder/vk/Context.h"
#include "cinder/vk/Descriptor.h"
#include "cinder/vk/ShaderProg.h"
#include "cinder/vk/Texture.h"

#include "xxh3.h"

#include <sstream>

namespace cinder::vk {

////////////////////////////////////////////////////////////////////////////////////////////////////
// ShaderDef

// ShaderDef::ShaderDef()
//{
//	memset( &mHashKey, 0, sizeof( mHashKey ) );
// }
//
// ShaderDef &ShaderDef::texture( const vk::Texture2dRef &texture )
//{
//	mHashKey.textureMapping = true;
//
//	if ( texture ) {
//		mHashKey.textureMappingRectangle = texture->getUnnormalizedCoordinates();
//	}
// }
//
// ShaderDef &ShaderDef::textureRectangle()
//{
//	mHashKey.textureMappingRectangle = true;
// }
//
// ShaderDef &ShaderDef::color()
//{
//	mHashKey.color = true;
// }
//
// ShaderDef &ShaderDef::lambert()
//{
//	mHashKey.lambert = true;
// }
//
// ShaderDef &ShaderDef::uniformBasedPosAndTexCoord()
//{
//	mHashKey.uniformBasedPosAndTexCoord = true;
// }

////////////////////////////////////////////////////////////////////////////////////////////////////
// StockShaderManager

static const char *sDrawTextureVert = R"vert(
#version 450

layout(push_constant) uniform constants {
	mat4 ciModelViewProjection;
	vec2 uPositionOffset, uPositionScale;
	vec2 uTexCoordOffset, uTexCoordScale;
};

layout( location = 0 ) out vec2 TexCoord;

void main( void ) {
    const vec2 positions[6] = vec2[6](
        vec2(1, 0),
        vec2(0, 0),
        vec2(1, 1),
        vec2(0, 1),
        vec2(0, 0),
        vec2(1, 1));
        
    const vec2 texCoords[6] = vec2[6](
        vec2(1, 0),
        vec2(0, 0),
        vec2(1, 1),
        vec2(0, 1),
        vec2(0, 0),
        vec2(1, 1));

	gl_Position = ciModelViewProjection * ( vec4( uPositionOffset, 0, 0 ) + vec4( uPositionScale, 1, 1 ) * vec4(positions[gl_VertexID], 0, 1) );
	TexCoord = uTexCoordOffset + uTexCoordScale * texCoords[gl_VertexID];
}
)vert";

static const char *sDrawTextureFrag = R"frag(
#version 450

layout( binding = 0, set = 0 ) uniform sampler2D uTex0;

layout( location = 0 ) in  vec2 TexCoord;
layout( location = 0 ) out vec4 oColor;

void main( void ) {
	oColor = texture( uTex0, TexCoord.st );
}
)frag";

static const char *sDrawTextureRectangleFrag = R"frag(
#version 450

layout( binding = 0, set = 0 ) uniform sampler2DRect uTex0;

layout( location = 0 ) in  vec2 TexCoord;
layout( location = 0 ) out vec4 oColor;

void main( void ) {
	oColor = texture( uTex0, TexCoord.st );
}
)frag";

StockShaderManager::StockShaderManager( vk::ContextRef context )
	: vk::ContextChildObject( context )
{
	vk::DescriptorSetLayout::Options setLayoutOptions = vk::DescriptorSetLayout::Options()
															.addCombinedImageSampler(0 + CINDER_CONTEXT_PS_BINDING_SHIFT_TEXTURE)
															.pushDescriptor();
	mDrawTextureSetLayout = vk::DescriptorSetLayout::create( setLayoutOptions, context->getDevice() );

	uint32_t					size	  = sizeof( mat4 ) + 4 * sizeof( vec2 );
	vk::PipelineLayout::Options plOptions = vk::PipelineLayout::Options()
												.addPushConstantRange( 0, size, VK_SHADER_STAGE_VERTEX_BIT )
												.addSetLayout( mDrawTextureSetLayout );
	mDrawTexturePipelineLayout = vk::PipelineLayout::create( plOptions, context->getDevice() );

	mDrawTextureProg		  = vk::GlslProg::create( getContext(), std::string( sDrawTextureVert ), std::string( sDrawTextureFrag ) );
	mDrawTextureRectangleProg = vk::GlslProg::create( getContext(), std::string( sDrawTextureVert ), std::string( sDrawTextureRectangleFrag ) );
}

StockShaderManager::~StockShaderManager()
{
}

const vk::PipelineLayout *StockShaderManager::getDrawTexturePipelineLayout() const
{
	return mDrawTexturePipelineLayout.get();
}

const vk::GlslProg *StockShaderManager::getDrawTextureProg( bool rectangle ) const
{
	return rectangle ? mDrawTextureRectangleProg.get() : mDrawTextureProg.get();
}

} // namespace cinder::vk
