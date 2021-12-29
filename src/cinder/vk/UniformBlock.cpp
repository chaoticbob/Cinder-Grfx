#include "cinder/vk/UniformBlock.h"

namespace cinder::vk {

Uniform::Uniform(
	const std::string & name,
	vk::DataType		dataType,
	vk::UniformSemantic uniformSemantic,
	uint32_t			offset,
	uint32_t			arraySize,
	uint32_t			arrayStride )
	: mName( name ),
	  mDataType( dataType ),
	  mUniformSemantic( uniformSemantic ),
	  mOffset( offset ),
	  mArraySize( arraySize ),
	  mArrayStride( arrayStride )
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UniformBlock

UniformBlock::UniformBlock(
	const std::string &			name,
	uint32_t					size,
	uint32_t					binding,
	uint32_t					set,
	const std::vector<Uniform> &uniforms )
	: mName( name ),
	  mSize( size ),
	  mBinding( binding ),
	  mSet( set ),
	  mUniforms( uniforms )
{
}

const Uniform *UniformBlock::getUniform( const std::string &name ) const
{
	auto it = std::find_if(
		mUniforms.begin(),
		mUniforms.end(),
		[name]( const Uniform &elem ) -> bool {
			return elem.mName == name;
		} );
	const Uniform *ptr = nullptr;
	if ( it != mUniforms.end() ) {
		ptr = &( *it );
	}
	return ptr;
}

} // namespace cinder::vk
