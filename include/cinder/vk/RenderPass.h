#pragma once

#include "cinder/vk/ChildObject.h"

namespace cinder::vk {

/*

// Render pass
class RenderPass
	: vk::DeviceChildObject
{
public:
	// clang-format off
	class Desc {
	public:
		Desc() {}
	
		Desc& addRenderTarget(const VkAttachmentDescription& desc) { mRenderTargets.push_back(desc); return *this; }
		Desc& setDepthStencil(const VkAttachmentDescription& desc) { mDepthStencil = desc; mHasDepthStencil = true; return *this; }

	private:
		std::vector<VkAttachmentDescription>	mRenderTargets		= {};
		bool									mHasDepthStencil	= false;
		VkAttachmentDescription					mDepthStencil		= {};
	
		friend RenderPass;
	};
	// clang-format on

	virtual ~RenderPass();

	static vk::RenderPassRef create( const Desc &desc, vk::DeviceRef device = vk::DeviceRef() );

	VkRenderPass getRenderPassHandle() const { return mRenderPassHandle; }

private:
	RenderPass( vk::DeviceRef device, const Desc &desc );

private:
	VkRenderPass mRenderPassHandle = VK_NULL_HANDLE;
};

// Frambuffer
class Framebuffer
	: vk::DeviceChildObject
{
public:
	// clang-format off
	class ImageDesc {
	public:
		ImageDesc() {}

		ImageDesc& addAttachment(ImageRef image) { mImages.push_back(image); return *this; }

	private:
		std::vector<ImageRef>	mImages;

		friend class Framebuffer;
	};
	// clang-format on

	// clang-format off
	class ViewDesc {
	public:
		ViewDesc() {}

		ViewDesc& addAttachment(ImageViewRef view) { mViews.push_back(view); return *this; }

	private:
		std::vector<ImageViewRef>	mViews;

		friend class Framebuffer;
	};
	// clang-format on

	~Framebuffer();

	static FramebufferRef create( VkRenderPass renderPassHandle, uint32_t width, uint32_t height, const ImageDesc &desc, vk::DeviceRef device = vk::DeviceRef() );
	static FramebufferRef create( VkRenderPass renderPassHandle, uint32_t width, uint32_t height, const ViewDesc &desc, vk::DeviceRef device = vk::DeviceRef() );

	VkFramebuffer getFramebufferHandle() const { return mFramebufferHandle; }

	const VkExtent2D &getExtent() const { return mExtent; }

	const std::vector<ImageViewRef> &getImageViews() const { return mViews; }

private:
	Framebuffer( vk::DeviceRef device, VkRenderPass renderPassHandle, uint32_t width, uint32_t height, const ViewDesc &desc );

private:
	VkFramebuffer			  mFramebufferHandle = VK_NULL_HANDLE;
	VkExtent2D				  mExtent			 = {};
	std::vector<ImageViewRef> mViews;
};

//! Render pass with framebuffer
class BufferedRenderPass
{
public:
	// clang-format off
	class FormatDesc {
	public:
		FormatDesc() {}
	
		FormatDesc& samples(VkSampleCountFlagBits value) { mSamples = value; return *this; }
		FormatDesc& addRenderTarget(const VkAttachmentDescription& desc) { mRenderTargets.push_back(desc); return *this; }
		FormatDesc& setDepthStencil(const VkAttachmentDescription& desc) { mDepthStencil = desc; mHasDepthStencil = true; return *this; }

	private:
		VkSampleCountFlagBits					mSamples			= VK_SAMPLE_COUNT_1_BIT;
		std::vector<VkAttachmentDescription>	mRenderTargets		= {};
		bool									mHasDepthStencil	= false;
		VkAttachmentDescription					mDepthStencil		= {};
	
		friend BufferedRenderPass;
	};
	// clang-format on

	// clang-format off
	class ImageDesc {
	public:
		ImageDesc() {}
	
		ImageDesc& addRenderTarget(const ImageRef& image, VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_LOAD, VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE) { mRenderTargets.push_back({image, loadOp, storeOp}); return *this; }
		ImageDesc& setDepthStencil(const ImageRef& image, VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_LOAD, VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE, VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD, VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE) { mDepthStencil = {image, loadOp, storeOp, stencilLoadOp, stencilStoreOp}; return *this; }

	private:
		struct Attachment
		{
			vk::ImageRef		image;
			VkAttachmentLoadOp	loadOp			= VK_ATTACHMENT_LOAD_OP_LOAD;
			VkAttachmentStoreOp storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
			VkAttachmentLoadOp	stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_LOAD;
			VkAttachmentStoreOp stencilStoreOp	= VK_ATTACHMENT_STORE_OP_STORE;
		};

		std::vector<Attachment>		mRenderTargets	= {};
		Attachment					mDepthStencil	= {};
	
		friend BufferedRenderPass;
	};
	// clang-format on

	// clang-format off
	class ViewDesc {
	public:
		ViewDesc() {}
	
		ViewDesc& addRenderTarget(const ImageViewRef& view, VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_LOAD, VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE) { mRenderTargets.push_back({view, loadOp, storeOp}); return *this; }
		ViewDesc& setDepthStencil(const ImageViewRef& view, VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_LOAD, VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE, VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD, VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE) { mDepthStencil = {view, loadOp, storeOp, stencilLoadOp, stencilStoreOp}; return *this; }

	private:
		struct Attachment
		{
			vk::ImageViewRef	view;
			VkAttachmentLoadOp	loadOp			= VK_ATTACHMENT_LOAD_OP_LOAD;
			VkAttachmentStoreOp storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
			VkAttachmentLoadOp	stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_LOAD;
			VkAttachmentStoreOp stencilStoreOp	= VK_ATTACHMENT_STORE_OP_STORE;
		};

		std::vector<Attachment>		mRenderTargets	= {};
		Attachment					mDepthStencil	= {};
	
		friend BufferedRenderPass;
	};
	// clang-format on

	virtual ~BufferedRenderPass();

	static vk::BufferedRenderPassRef create( uint32_t width, uint32_t height, const FormatDesc &desc, vk::DeviceRef device = vk::DeviceRef() );
	static vk::BufferedRenderPassRef create( uint32_t width, uint32_t height, const ImageDesc &desc, vk::DeviceRef device = vk::DeviceRef() );
	static vk::BufferedRenderPassRef create( uint32_t width, uint32_t height, const ViewDesc &desc, vk::DeviceRef device = vk::DeviceRef() );

	const VkExtent2D &getExtent() const { return mFramebuffer->getExtent(); }
	VkRenderPass	  getRenderPassHandle() const { return mRenderPass->getRenderPassHandle(); }
	VkFramebuffer	  getFramebufferHandle() const { return mFramebuffer->getFramebufferHandle(); }

	const VkRect2D &getRenderArea() const { return mRenderArea; }

private:
	BufferedRenderPass( vk::DeviceRef device, vk::RenderPassRef renderPass, vk::FramebufferRef framebuffer );

private:
	vk::RenderPassRef  mRenderPass;
	vk::FramebufferRef mFramebuffer;
	VkRect2D		   mRenderArea = {};
};

*/

} // namespace cinder::vk
