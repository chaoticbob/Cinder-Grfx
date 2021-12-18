#pragma once

#include "cinder/vk/DeviceChildObject.h"
#include "cinder/GeomIo.h"

namespace cinder::vk {

class Batch
	: public vk::DeviceChildObject
{
public:
	//! Maps a geom::Attrib to a named attribute in the GlslProg
	typedef std::map<geom::Attrib, std::string> AttributeMapping;

	virtual ~Batch();

	//! Builds a Batch from a geom::Source and a GlslProg. Attributes defined in \a attributeMapping override the default mapping
	static BatchRef create( const geom::Source &source, const vk::ShaderProgRef &shaderProg, const AttributeMapping &attributeMapping = AttributeMapping(), vk::DeviceRef device = vk::DeviceRef() );

	//! Draws the Batch. Optionally specify a \a first vertex/element and a \a count. Otherwise the entire geometry will be drawn.
	void			draw( int32_t first = 0, int32_t count = -1 );

private:
	Batch( vk::DeviceRef device, const geom::Source &source, const vk::ShaderProgRef &shaderProg, const AttributeMapping &attributeMapping );

private:
	vk::ShaderProgRef	mShaderProg;
	vk::BufferedMeshRef mMesh;
};

} // namespace cinder::vk
