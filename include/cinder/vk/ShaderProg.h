#pragma once

#include "cinder/vk/DeviceChildObject.h"
#include "cinder/vk/UniformBlock.h"
#include "cinder/vk/Util.h"

#include "cinder/DataSource.h"
#include "cinder/GeomIo.h"

#include "spirv_reflect.h"

namespace cinder::vk {

//! @class ShaderModule
//!
//!
class ShaderModule
	: public vk::DeviceChildObject
{
public:
	~ShaderModule();

	static vk::ShaderModuleRef create( DataSourceRef dataSource, vk::DeviceRef device = vk::DeviceRef() );
	static vk::ShaderModuleRef create( size_t sizeInBytes, const char *pSpirvCode, vk::DeviceRef device = vk::DeviceRef() );
	static vk::ShaderModuleRef create( const std::vector<char> &spirv, vk::DeviceRef device = vk::DeviceRef() );

	VkShaderStageFlagBits getShaderStage() const { return mShaderStage; }

	const std::string &getEntryPoint() const { return mEntryPoint; }

	SpvSourceLanguage getSourceLanguage() const { return mSourceLanguage; }

	const std::vector<vk::InterfaceVariable> &getInputVariables() const { return mInputVariables; }

	const std::vector<vk::InterfaceVariable> &getOutputVariables() const { return mOutputVariables; }

	const std::vector<vk::InterfaceVariable> &getVertexAttributes() const;

	const std::vector<vk::DescriptorBinding> &getDescriptorBindings() const { return mDescriptorBindings; }

	const std::vector<std::unique_ptr<vk::UniformBlock>> &getUniformBlocks() const { return mUniformBlocks; }

	VkShaderModule getShaderModuleHandle() const { return mShaderModuleHandle; }

private:
	ShaderModule( vk::DeviceRef device, size_t spirvSize, const char *pSpirvCode );

	void parseInterfaceVariables( const spv_reflect::ShaderModule &reflection );
	void parseDescriptorBindings( const std::vector<SpvReflectDescriptorBinding *> &spirvBindings );
	void parseUniformBlocks( const std::vector<SpvReflectDescriptorBinding *> &spirvBindings );

private:
	VkShaderStageFlagBits						   mShaderStage = static_cast<VkShaderStageFlagBits>( 0 );
	std::string									   mEntryPoint;
	SpvSourceLanguage							   mSourceLanguage = SpvSourceLanguageUnknown;
	std::vector<vk::InterfaceVariable>			   mInputVariables;
	std::vector<vk::InterfaceVariable>			   mOutputVariables;
	std::vector<vk::InterfaceVariable>			   mNullVariables;
	std::vector<vk::DescriptorBinding>			   mDescriptorBindings;
	std::vector<std::unique_ptr<vk::UniformBlock>> mUniformBlocks;
	vk::UniformBlock *							   mDefaultUniformBlock = nullptr;

	VkShaderModule mShaderModuleHandle = VK_NULL_HANDLE;
};

//! @class ShaderProg
//!
//!
class CI_API ShaderProg
{
public:
	struct CI_API Format
	{
		Format() {}

		// clang-format off
		Format &vertex( vk::ShaderModuleRef shader ) { mVertexShader = shader; return *this; }
		Format &vertex( DataSourceRef dataSource ) { mVertexDataSource = dataSource; return *this; }

		Format &fragment( vk::ShaderModuleRef shader ) { mPixelShader = shader; return *this; }
		Format &fragment( DataSourceRef dataSource ) { mVertexDataSource = dataSource; return *this; }

		Format &pixel( vk::ShaderModuleRef shader ) { fragment(shader); return *this;}
		Format &pixel( DataSourceRef dataSource ) { mVertexDataSource = dataSource; return *this; }

		Format &geometry( vk::ShaderModuleRef shader ) { mGeometryShader = shader; return *this; }
		Format &geometry( DataSourceRef dataSource ) { mVertexDataSource = dataSource; return *this; }

		Format &tessellationCtrl( vk::ShaderModuleRef shader ) { mHullShader = shader; return *this; }
		Format &tessellationCtrl( DataSourceRef dataSource ) { mVertexDataSource = dataSource; return *this; }

		Format &hull( vk::ShaderModuleRef shader ) { tessellationCtrl(shader); return *this;}
		Format &hull( DataSourceRef dataSource ) { tessellationCtrl(dataSource); return *this; }

		Format &tessellationEval( vk::ShaderModuleRef shader ) { mDomainShader = shader; return *this; }
		Format &tessellationEval( DataSourceRef dataSource ) { mVertexDataSource = dataSource; return *this; }

		Format &domain( vk::ShaderModuleRef shader ) { tessellationEval(shader); return *this;}
		Format &domain( DataSourceRef dataSource ) { tessellationEval(dataSource); return *this; }

		Format &compute( vk::ShaderModuleRef shader ) { mComputeShader = shader; return *this; }
		Format &compute( DataSourceRef dataSource ) { mVertexDataSource = dataSource; return *this; }
		// clang-format on

	private:
		static std::vector<uint8_t> setShaderDataSource( DataSourceRef dataSource );

	private:
		// Vertex
		vk::ShaderModuleRef mVertexShader;
		DataSourceRef		mVertexDataSource;

		// Pixel / Fragment
		vk::ShaderModuleRef mPixelShader;
		DataSourceRef		mPixelDataSource;

		// Geometry
		vk::ShaderModuleRef mGeometryShader;
		DataSourceRef		mGeometryDataSource;

		// Hull / Tessellation Control
		vk::ShaderModuleRef mHullShader;
		DataSourceRef		mHullDataSource;

		// Domain / Tessellation Evaluate
		vk::ShaderModuleRef mDomainShader;
		DataSourceRef		mDomainDataSource;

		// Compute
		vk::ShaderModuleRef mComputeShader;
		DataSourceRef		mComputeDataSource;

		friend class ShaderProg;
	};

	virtual ~ShaderProg();

	//! Create a shader program from shader modules or SPIR-V data sources
	static vk::ShaderProgRef create( vk::DeviceRef device, const Format &format );

	//! Create a shader program from shader modules
	static ShaderProgRef create(
		vk::ShaderModuleRef vertModule,
		vk::ShaderModuleRef fragModule = vk::ShaderModuleRef(),
		vk::ShaderModuleRef geomModule = vk::ShaderModuleRef(),
		vk::ShaderModuleRef teseModule = vk::ShaderModuleRef(),
		vk::ShaderModuleRef tescModule = vk::ShaderModuleRef() );

	//! Create a shader program from SPIR-V data sources
	static ShaderProgRef create(
		vk::DeviceRef device,
		DataSourceRef vertSpirv,
		DataSourceRef fragSpirv = DataSourceRef(),
		DataSourceRef geomSpirv = DataSourceRef(),
		DataSourceRef teseSpirv = DataSourceRef(),
		DataSourceRef tescSpirv = DataSourceRef() );

	bool isCompute() const { return mCS ? true : false; }

	vk::ShaderModuleRef getVertexShader() const { return mVS; }
	vk::ShaderModuleRef getFragmentShader() const { return mPS; }
	vk::ShaderModuleRef getGeometryShader() const { return mGS; }
	vk::ShaderModuleRef getTessellationCtrlShader() const { return mHS; }
	vk::ShaderModuleRef getTessellationEvalShader() const { return mDS; }
	vk::ShaderModuleRef getComputeShader() const { return mVS; }

	vk::ShaderModuleRef getPixelShader() const { return mPS; }
	vk::ShaderModuleRef getHullShader() const { return mHS; }
	vk::ShaderModuleRef getDomainShader() const { return mDS; }

	const std::vector<vk::InterfaceVariable> &getVertexAttributes() const;

	const std::map<uint32_t, std::vector<DescriptorBinding>> &getDescriptorsetBindings() const { return mDescriptorSetBindings; }

	const UniformBlock *getDefaultUniformBlock() const { return mDefaultUniformBlock; }

protected:
	ShaderProg( vk::DeviceRef device, const Format &format );

	void parseDscriptorBindings( const vk::ShaderModule *shader );
	void parseUniformBlocks( const vk::ShaderModule *shader );

private:
	vk::ShaderModuleRef mVS; // Vertex
	vk::ShaderModuleRef mPS; // Pixel / Fragment
	vk::ShaderModuleRef mGS; // Geometry
	vk::ShaderModuleRef mHS; // Hull / Tessellation Control
	vk::ShaderModuleRef mDS; // Domain / Tessellation Evaluate
	vk::ShaderModuleRef mCS; // Compute

	std::vector<vk::InterfaceVariable>				   mNullVariables;
	std::map<uint32_t, std::vector<DescriptorBinding>> mDescriptorSetBindings;
	std::vector<std::unique_ptr<UniformBlock>>		   mUniformBlocks;
	UniformBlock *									   mDefaultUniformBlock = nullptr;
};

//! @class GlslProg
//!
//!
class GlslProg
	: public vk::ShaderProg
{
public:
	virtual ~GlslProg();

	static GlslProgRef create(
		const DataSourceRef &vertTextDataSource,
		const DataSourceRef &fragTextDataSource = DataSourceRef(),
		const DataSourceRef &geomTextDataSource = DataSourceRef(),
		const DataSourceRef &teseTextDataSource = DataSourceRef(),
		const DataSourceRef &tescTextDataSource = DataSourceRef() );

	static GlslProgRef create(
		vk::DeviceRef device,
		const DataSourceRef &vertTextDataSource,
		const DataSourceRef &fragTextDataSource = DataSourceRef(),
		const DataSourceRef &geomTextDataSource = DataSourceRef(),
		const DataSourceRef &teseTextDataSource = DataSourceRef(),
		const DataSourceRef &tescTextDataSource = DataSourceRef() );

	static GlslProgRef create(
		const std::string &vertText,
		const std::string &fragText = std::string(),
		const std::string &geomText = std::string(),
		const std::string &teseText = std::string(),
		const std::string &tescText = std::string() );

	static GlslProgRef create(
		vk::DeviceRef	   device,
		const std::string &vertText,
		const std::string &fragText = std::string(),
		const std::string &geomText = std::string(),
		const std::string &teseText = std::string(),
		const std::string &tescText = std::string() );

	vk::ShaderProgRef getShaderProg() const { return mShaderProg; }

private:
	GlslProg( vk::DeviceRef device, const vk::ShaderProg::Format &format );

	//static void				   loadShader( DataSourceRef dataSource, std::string &sourceTarget );
	static vk::ShaderModuleRef compileShader( vk::DeviceRef device, const std::string &source, VkShaderStageFlagBits stage );

private:
	vk::ShaderProgRef mShaderProg;
};

//! @class HlslProg
//!
//!
class HlslProg
	: public vk::ShaderProg
{
public:
	enum class ShaderModel : uint32_t
	{
		SM_6_0 = 0,
		SM_6_1,
		SM_6_2,
		SM_6_3,
		SM_6_4,
		SM_6_5,
		SM_6_6,
	};

	////////////////////////////////////////////////////////////////////////////////////////////////

	virtual ~HlslProg();

	static HlslProgRef create(
		const DataSourceRef &vsTextDataSource,
		const DataSourceRef &psTextDataSource = DataSourceRef(),
		const DataSourceRef &gsTextDataSource = DataSourceRef(),
		const DataSourceRef &dsTextDataSource = DataSourceRef(),
		const DataSourceRef &hsTextDataSource = DataSourceRef() );

	static HlslProgRef create(
		vk::DeviceRef device,
		const DataSourceRef &vsTextDataSource,
		const DataSourceRef &psTextDataSource = DataSourceRef(),
		const DataSourceRef &gsTextDataSource = DataSourceRef(),
		const DataSourceRef &dsTextDataSource = DataSourceRef(),
		const DataSourceRef &hsTextDataSource = DataSourceRef() );

	static HlslProgRef create(
		const std::string &vsText,
		const std::string &psText = std::string(),
		const std::string &gsText = std::string(),
		const std::string &dsText = std::string(),
		const std::string &hsText = std::string() );

	static HlslProgRef create(
		vk::DeviceRef	   device,
		const std::string &vsText,
		const std::string &psText = std::string(),
		const std::string &gsText = std::string(),
		const std::string &dsText = std::string(),
		const std::string &hsText = std::string() );

	vk::ShaderProgRef getShaderProg() const { return mShaderProg; }

private:
	HlslProg( vk::DeviceRef device, const vk::ShaderProg::Format &format );

	static vk::ShaderModuleRef compileShader(
		vk::DeviceRef			  device,
		const std::string &		  sourceText,
		VkShaderStageFlagBits	  shaderStage,
		const std::string &		  entryPointName = "",
		vk::HlslProg::ShaderModel shaderModel	 = vk::HlslProg::ShaderModel::SM_6_0 );

private:
	vk::ShaderProgRef mShaderProg;
};

} // namespace cinder::vk
