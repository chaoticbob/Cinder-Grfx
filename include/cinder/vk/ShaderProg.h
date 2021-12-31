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
		Format &vertex( vk::ShaderModuleRef module ) { mVSModule = module; return *this; }
		Format &vertex( DataSourceRef dataSource ) { mVSDataSource = dataSource; return *this; }

		Format &fragment( vk::ShaderModuleRef module ) { mPSModule = module; return *this; }
		Format &fragment( DataSourceRef dataSource ) { mPSDataSource = dataSource; return *this; }

		Format &pixel( vk::ShaderModuleRef module ) { fragment(module); return *this;}
		Format &pixel( DataSourceRef dataSource ) { fragment(dataSource); return *this; }

		Format &geometry( vk::ShaderModuleRef module ) { mGSModule = module; return *this; }
		Format &geometry( DataSourceRef dataSource ) { mGSDataSource = dataSource; return *this; }

		Format &tessellationCtrl( vk::ShaderModuleRef module ) { mHSModule = module; return *this; }
		Format &tessellationCtrl( DataSourceRef dataSource ) { mHSDataSource = dataSource; return *this; }

		Format &hull( vk::ShaderModuleRef module ) { tessellationCtrl(module); return *this;}
		Format &hull( DataSourceRef dataSource ) { tessellationCtrl(dataSource); return *this; }

		Format &tessellationEval( vk::ShaderModuleRef module ) { mDSModule = module; return *this; }
		Format &tessellationEval( DataSourceRef dataSource ) { mDSDataSource = dataSource; return *this; }

		Format &domain( vk::ShaderModuleRef module ) { tessellationEval(module); return *this;}
		Format &domain( DataSourceRef dataSource ) { tessellationEval(dataSource); return *this; }

		Format &compute( vk::ShaderModuleRef module ) { mCSModule = module; return *this; }
		Format &compute( DataSourceRef dataSource ) { mCSDataSource = dataSource; return *this; }
		// clang-format on

	private:
		static std::vector<uint8_t> setShaderDataSource( DataSourceRef dataSource );

	private:
		// Vertex
		vk::ShaderModuleRef mVSModule;
		DataSourceRef		mVSDataSource;

		// Pixel / Fragment
		vk::ShaderModuleRef mPSModule;
		DataSourceRef		mPSDataSource;

		// Geometry
		vk::ShaderModuleRef mGSModule;
		DataSourceRef		mGSDataSource;

		// Hull / Tessellation Control
		vk::ShaderModuleRef mHSModule;
		DataSourceRef		mHSDataSource;

		// Domain / Tessellation Evaluate
		vk::ShaderModuleRef mDSModule;
		DataSourceRef		mDSDataSource;

		// Compute
		vk::ShaderModuleRef mCSModule;
		DataSourceRef		mCSDataSource;

		friend class ShaderProg;
	};

	virtual ~ShaderProg();

	//! Create a shader program from shader modules or SPIR-V data sources
	static vk::ShaderProgRef create( vk::DeviceRef device, const Format &format );

	//! Create a shader program from shader modules
	static vk::ShaderProgRef create(
		vk::ShaderModuleRef	  vertOrCompModule,
		VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT );

	static vk::ShaderProgRef create(
		vk::ShaderModuleRef vertModule,
		vk::ShaderModuleRef fragModule,
		vk::ShaderModuleRef geomModule = nullptr,
		vk::ShaderModuleRef teseModule = nullptr,
		vk::ShaderModuleRef tescModule = nullptr );

	//! Create a shader program from SPIR-V data sources
	static vk::ShaderProgRef create(
		vk::DeviceRef		  device,
		const DataSourceRef & vertOrCompSpirvDataSource,
		VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT );

	static vk::ShaderProgRef create(
		vk::DeviceRef		 device,
		const DataSourceRef &vertSpirvDataSource,
		const DataSourceRef &fragSpirvDataSource,
		const DataSourceRef &geomSpirvDataSource = nullptr,
		const DataSourceRef &teseSpirvDataSource = nullptr,
		const DataSourceRef &tescSpirvDataSource = nullptr );

	//! Create a shader program from SPIR-V
	static vk::ShaderProgRef create(
		vk::DeviceRef			 device,
		const vk::SpirvBytecode &vsOrCsSpirv,
		VkShaderStageFlagBits	 shaderStage = VK_SHADER_STAGE_COMPUTE_BIT );

	static vk::ShaderProgRef create(
		vk::DeviceRef			 device,
		const vk::SpirvBytecode &vsSpirv,
		const vk::SpirvBytecode &psSpirv,
		const vk::SpirvBytecode &gsSpirv = vk::SpirvBytecode{ 0, nullptr },
		const vk::SpirvBytecode &dsSpirv = vk::SpirvBytecode{ 0, nullptr },
		const vk::SpirvBytecode &hsSpirv = vk::SpirvBytecode{ 0, nullptr } );

	bool isGraphics() const { return mVs ? true : false; }
	bool isCompute() const { return mCs ? true : false; }

	vk::ShaderModuleRef getVertexShader() const { return mVs; }
	vk::ShaderModuleRef getFragmentShader() const { return mPs; }
	vk::ShaderModuleRef getGeometryShader() const { return mGs; }
	vk::ShaderModuleRef getTessellationEvalShader() const { return mDs; }
	vk::ShaderModuleRef getTessellationCtrlShader() const { return mHs; }
	vk::ShaderModuleRef getComputeShader() const { return mVs; }

	vk::ShaderModuleRef getPixelShader() const { return getFragmentShader(); }
	vk::ShaderModuleRef getDomainShader() const { return getTessellationEvalShader(); }
	vk::ShaderModuleRef getHullShader() const { return getTessellationCtrlShader(); }

	const std::vector<vk::InterfaceVariable> &getVertexAttributes() const;

	const std::map<uint32_t, std::vector<DescriptorBinding>> &getDescriptorsetBindings() const { return mDescriptorSetBindings; }

	const UniformBlock *getDefaultUniformBlock() const { return mDefaultUniformBlock; }

protected:
	ShaderProg(
		vk::ShaderModuleRef vs,
		vk::ShaderModuleRef ps,
		vk::ShaderModuleRef gs,
		vk::ShaderModuleRef ds,
		vk::ShaderModuleRef hs,
		vk::ShaderModuleRef cs );

	ShaderProg(
		vk::DeviceRef		  device,
		std::vector<char> &&  vsOrCsSpirv,
		VkShaderStageFlagBits shaderStage );

	ShaderProg(
		vk::DeviceRef		device,
		std::vector<char> &&vsSpirv,
		std::vector<char> &&psSpirv,
		std::vector<char> &&gsSpirv,
		std::vector<char> &&dsSpirv,
		std::vector<char> &&hsSpirv );

private:
	void parseModules();
	void parseDscriptorBindings( const vk::ShaderModule *shader );
	void parseUniformBlocks( const vk::ShaderModule *shader );

private:
	vk::ShaderModuleRef mVs; // Vertex
	vk::ShaderModuleRef mPs; // Pixel / Fragment
	vk::ShaderModuleRef mGs; // Geometry
	vk::ShaderModuleRef mDs; // Domain / Tessellation Evaluate
	vk::ShaderModuleRef mHs; // Hull / Tessellation Control
	vk::ShaderModuleRef mCs; // Compute

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

	// Create GLSL prog from data source using current context's device
	static GlslProgRef create(
		const DataSourceRef & vsOrCsTextDataSource,
		VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT );

	static GlslProgRef create(
		const DataSourceRef &vsTextDataSource,
		const DataSourceRef &psTextDataSource,
		const DataSourceRef &gsTextDataSource = nullptr,
		const DataSourceRef &dsTextDataSource = nullptr,
		const DataSourceRef &hsTextDataSource = nullptr );

	// Create GLSL prog from data source using device
	static GlslProgRef create(
		vk::DeviceRef		  device,
		const DataSourceRef & vsOrCsTextDataSource,
		VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT );

	static GlslProgRef create(
		vk::DeviceRef		 device,
		const DataSourceRef &vsTextDataSource,
		const DataSourceRef &psTextDataSource,
		const DataSourceRef &gsTextDataSource = nullptr,
		const DataSourceRef &dsTextDataSource = nullptr,
		const DataSourceRef &hsTextDataSource = nullptr );

	// Create GLSL prog from text source code using current context's device
	static GlslProgRef create(
		const std::string &	  vsOrCsText,
		VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT );

	static GlslProgRef create(
		const std::string &vsText,
		const std::string &psText,
		const std::string &gsText = "",
		const std::string &dsText = "",
		const std::string &hsText = "" );

	// Create GLSL prog from text source code using device
	static GlslProgRef create(
		vk::DeviceRef		  device,
		const std::string &	  vsOrCsText,
		VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT );

	static GlslProgRef create(
		vk::DeviceRef	   device,
		const std::string &vsText,
		const std::string &psText,
		const std::string &gsText = "",
		const std::string &dsText = "",
		const std::string &hsText = "" );

	vk::ShaderProgRef getShaderProg() const { return mShaderProg; }

private:
	GlslProg(
		vk::DeviceRef		  device,
		std::vector<char> &&  vsOrCsSpirv,
		VkShaderStageFlagBits shaderStage );

	GlslProg(
		vk::DeviceRef		device,
		std::vector<char> &&vsSpirv,
		std::vector<char> &&psSpirv,
		std::vector<char> &&gsSpirv,
		std::vector<char> &&dsSpirv,
		std::vector<char> &&hsSpirv );

	static std::vector<char> compileShader(
		vk::DeviceRef		  device,
		std::string			  text,
		VkShaderStageFlagBits stage );

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

	// Create HLSL prog from data source using current context's device
	static HlslProgRef create(
		const DataSourceRef & vsOrCsTextDataSource,
		VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT );

	static HlslProgRef create(
		const DataSourceRef &vsTextDataSource,
		const DataSourceRef &psTextDataSource,
		const DataSourceRef &gsTextDataSource = nullptr,
		const DataSourceRef &dsTextDataSource = nullptr,
		const DataSourceRef &hsTextDataSource = nullptr );

	// Create HLSL prog from data source using device
	static HlslProgRef create(
		vk::DeviceRef		  device,
		const DataSourceRef & vsOrCsTextDataSource,
		VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT );

	static HlslProgRef create(
		vk::DeviceRef		 device,
		const DataSourceRef &vsTextDataSource,
		const DataSourceRef &psTextDataSource,
		const DataSourceRef &gsTextDataSource = nullptr,
		const DataSourceRef &dsTextDataSource = nullptr,
		const DataSourceRef &hsTextDataSource = nullptr );

	// Create HLSL prog from text source code using current context's device
	static HlslProgRef create(
		const std::string &	  vsOrCsText,
		VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT );

	static HlslProgRef create(
		const std::string &vsText,
		const std::string &psText,
		const std::string &gsText = "",
		const std::string &dsText = "",
		const std::string &hsText = "" );

	// Create HLSL prog from text source code using device
	static HlslProgRef create(
		vk::DeviceRef		  device,
		const std::string &	  vsOrCsText,
		VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT );

	static HlslProgRef create(
		vk::DeviceRef	   device,
		const std::string &vsText,
		const std::string &psText,
		const std::string &gsText = "",
		const std::string &dsText = "",
		const std::string &hsText = "" );

	vk::ShaderProgRef getShaderProg() const { return mShaderProg; }

private:
	HlslProg(
		vk::DeviceRef		  device,
		std::vector<char> &&  vsOrCsSpirv,
		VkShaderStageFlagBits shaderStage );

	HlslProg(
		vk::DeviceRef		device,
		std::vector<char> &&vsSpirv,
		std::vector<char> &&psSpirv,
		std::vector<char> &&gsSpirv,
		std::vector<char> &&dsSpirv,
		std::vector<char> &&hsSpirv );

	static std::vector<char> compileShader(
		vk::DeviceRef			  device,
		const std::string &		  sourceText,
		VkShaderStageFlagBits	  shaderStage,
		const std::string &		  entryPointName = "",
		vk::HlslProg::ShaderModel shaderModel	 = vk::HlslProg::ShaderModel::SM_6_6 );

private:
	vk::ShaderProgRef mShaderProg;
};

} // namespace cinder::vk
