#pragma once

#include "cinder/vk/vk_config.h"

namespace cinder::vk {

class ShaderModule;
class UniformBlock;

using UniformBlockRef = std::shared_ptr<UniformBlock>;

enum UniformSemantic
{
	UNIFORM_MODEL_MATRIX = 0,
	UNIFORM_MODEL_MATRIX_INVERSE,
	UNIFORM_MODEL_MATRIX_INVERSE_TRANSPOSE,
	UNIFORM_VIEW_MATRIX,
	UNIFORM_VIEW_MATRIX_INVERSE,
	UNIFORM_MODEL_VIEW,
	UNIFORM_MODEL_VIEW_INVERSE,
	UNIFORM_MODEL_VIEW_INVERSE_TRANSPOSE,
	UNIFORM_MODEL_VIEW_PROJECTION,
	UNIFORM_MODEL_VIEW_PROJECTION_INVERSE,
	UNIFORM_PROJECTION_MATRIX,
	UNIFORM_PROJECTION_MATRIX_INVERSE,
	UNIFORM_VIEW_PROJECTION,
	UNIFORM_NORMAL_MATRIX,
	UNIFORM_VIEWPORT_MATRIX,
	UNIFORM_WINDOW_SIZE,
	UNIFORM_ELAPSED_SECONDS,
	UNIFORM_USER_DEFINED
};

struct CI_API Uniform
{
	Uniform() {}

	Uniform( const std::string &name, vk::DataType dataType, vk::UniformSemantic uniformSemantic, uint32_t offset, uint32_t arraySize = 1, uint32_t arrayStride = 0 );

	bool operator==( const Uniform &obj ) const
	{
		bool isSame = ( mName == obj.mName ) && ( mDataType == obj.mDataType ) && ( mOffset == obj.mOffset );
		return isSame;
	}

	bool operator!=( const Uniform &obj ) const
	{
		bool isNotSame = ( mName != obj.mName ) || ( mDataType != obj.mDataType ) || ( mOffset != obj.mOffset );
		return isNotSame;
	}

	const std::string  &getName() const { return mName; }
	vk::DataType		getDataType() const { return mDataType; }
	vk::UniformSemantic getUniformSemantic() const { return mUniformSemantic; }
	uint32_t			getOffset() const { return mOffset; }
	uint32_t			getArraySize() const { return mArraySize; }

private:
	std::string			mName			 = "";
	vk::DataType		mDataType		 = vk::DataType::UNKNOWN;
	vk::UniformSemantic mUniformSemantic = vk::UniformSemantic::UNIFORM_USER_DEFINED;
	uint32_t			mOffset			 = UINT32_MAX;
	uint32_t			mArraySize		 = 1;
	uint32_t			mArrayStride	 = 0;

	friend class UniformBlock;
};

class CI_API UniformBlock
{
public:
	UniformBlock() {}

	UniformBlock(
		const std::string		  &name,
		uint32_t					size,
		uint32_t					binding,
		uint32_t					set,
		const std::vector<Uniform> &uniforms );

	const std::string &getName() const { return mName; }

	uint32_t getSize() const { return mSize; }

	uint32_t getBinding() const { return mBinding; }

	uint32_t getSet() const { return mSet; }

	const std::vector<Uniform> &getUniforms() const { return mUniforms; }

	const Uniform *getUniform( const std::string &name ) const;

private:
	std::string			 mName;
	uint32_t			 mSize	  = 0;
	uint32_t			 mBinding = UINT32_MAX;
	uint32_t			 mSet	  = UINT32_MAX;
	std::vector<Uniform> mUniforms;

	friend class ShaderModule;
};

} // namespace cinder::vk
