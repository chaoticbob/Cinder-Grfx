#pragma once

#include "cinder/vk/ChildObject.h"

namespace cinder::vk {

// class StockShaderManager;
//
// class CI_API ShaderDef
//{
// public:
//	ShaderDef();
//	~ShaderDef() {}
//
//	ShaderDef &texture( const vk::Texture2dRef &texture = nullptr );
//	ShaderDef &textureRectangle();
//	ShaderDef &color();
//	ShaderDef &lambert();
//
//	// Used by draw(TextureRef&) stock shader; scales ciPosition and ciTexCoord according to
//	// uniform "uPositionScale", "uPositionOffset", "uTexCoord0Scale", "uTexCoord0Offset"
//	ShaderDef &uniformBasedPosAndTexCoord();
//
// private:
//	struct ShaderHashKey
//	{
//		bool textureMapping				= false;
//		bool textureMappingRectangle	= false; // Sam as OpenGL's rectangle rexture
//		bool color						= false;
//		bool lambert					= false;
//		bool uniformBasedPosAndTexCoord = false;
//		bool pushConstant				= false;
//	};
//
//	ShaderHashKey mHashKey;
//
//	friend class vk::StockShaderManager;
// };

class CI_API StockShaderManager
	: public vk::ContextChildObject
{
public:
	StockShaderManager( vk::ContextRef context );
	~StockShaderManager();

	const vk::PipelineLayout *getDrawTexturePipelineLayout() const;
	const vk::GlslProg	   *getDrawTextureProg( bool rectangle = false ) const;

private:
	virtual void flightSync( uint32_t currentFrameIndex, uint32_t previousFrameIndex ) override {}

private:
	// These shader program *must* use push constants and push descriptors.
	vk::DescriptorSetLayoutRef mDrawTextureSetLayout;
	vk::PipelineLayoutRef	   mDrawTexturePipelineLayout;
	vk::GlslProgRef			   mDrawTextureProg;
	vk::GlslProgRef			   mDrawTextureRectangleProg;
};

} // namespace cinder::vk
