#include "cinder/app/App.h"
#include "cinder/app/RendererVk.h"
#include "cinder/vk/vk.h"
#include "cinder/ImageIo.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
namespace gl = ci::vk;

class RotatingCubeApp : public App
{
public:
	void setup() override;
	void resize() override;
	void update() override;
	void draw() override;

	CameraPersp		mCam;
	gl::BatchRef	mBatch;
	gl::TextureRef	mTexture;
	gl::GlslProgRef mGlsl;
	mat4			mCubeRotation;
};

void RotatingCubeApp::setup()
{
	vk::Pipeline::Options options = vk::Pipeline::Options();

	mCam.lookAt( vec3( 3, 2, 4 ), vec3( 0 ) );

	try {
		mTexture = gl::Texture::create( loadImage( loadAsset( "texture.jpg" ) ), gl::Texture::Format().mipmap() );
		mTexture->bind();
		CI_LOG_I( "Loaded texture" );
	}
	catch ( const std::exception &e ) {
		CI_LOG_E( "Texture Error: " << e.what() );
	}

	try {
		mGlsl = gl::GlslProg::create( loadAsset( "shader.vert" ), loadAsset( "shader.frag" ) );
		CI_LOG_I( "Loaded shader" );
	}
	catch ( const std::exception &e ) {
		CI_LOG_E( "Shader Error: " << e.what() );
	}

	try {
		mBatch = gl::Batch::create( geom::Cube(), mGlsl );
		CI_LOG_I( "Created batch" );
	}
	catch ( const std::exception &e ) {
		CI_LOG_E( "Shader Error: " << e.what() );
	}

	gl::enableDepthWrite();
	gl::enableDepthRead();
}

void RotatingCubeApp::resize()
{
	mCam.setPerspective( 60, getWindowAspectRatio(), 1, 1000 );
	gl::setMatrices( mCam );
}

void RotatingCubeApp::update()
{
	//console() << "RotatingCubeApp::update()" << std::endl;

	// Rotate the cube by 0.2 degrees around the y-axis
	mCubeRotation *= rotate( toRadians( 0.2f ), normalize( vec3( 0, 1, 0 ) ) );
}

void RotatingCubeApp::draw()
{
	gl::clear( ColorA( 1, 0, 0, 1 ) );

	gl::setMatrices( mCam );

	gl::ScopedModelMatrix modelScope;
	gl::multModelMatrix( mCubeRotation );

	mTexture->bind();
	mBatch->draw();
}

CINDER_APP( RotatingCubeApp, RendererVk( RendererVk::Options().msaa( 16 ).enableValidation() ) )
