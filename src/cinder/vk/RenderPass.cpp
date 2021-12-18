#include "cinder/vk/RenderPass.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Image.h"
#include "cinder/app/RendererVk.h"

namespace cinder::vk {

/*

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RenderPass

vk::RenderPassRef RenderPass::create( const Desc &desc, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return RenderPassRef( new RenderPass( device, desc ) );
}

RenderPass::RenderPass( vk::DeviceRef device, const Desc &desc )
	: vk::DeviceChildObject( device )
{
	// Attachments are always stored with all render targets first and
	// depth stencil as the last entry. If there are no render targets
	// present then depth stencil is the first and last entry.
	//
	std::vector<VkAttachmentDescription> attachments;

	const uint32_t numRenderTargets = countU32( desc.mRenderTargets );
	for ( uint32_t i = 0; i < numRenderTargets; ++i ) {
		attachments.push_back( desc.mRenderTargets[i] );
	}

	if ( desc.mHasDepthStencil ) {
		attachments.push_back( desc.mDepthStencil );
	}

	std::vector<VkAttachmentReference> renderTargetAttachmentReferences( numRenderTargets );
	for ( uint32_t i = 0; i < numRenderTargets; ++i ) {
		renderTargetAttachmentReferences[i].attachment = i;
		renderTargetAttachmentReferences[i].layout	   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentReference depthStencilAttachmentReference = {};
	if ( desc.mHasDepthStencil ) {
		depthStencilAttachmentReference.attachment = countU32( attachments ) - 1;
		depthStencilAttachmentReference.layout	   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription subpass	= {};
	subpass.flags					= 0;
	subpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount	= 0;
	subpass.pInputAttachments		= nullptr;
	subpass.colorAttachmentCount	= countU32( renderTargetAttachmentReferences );
	subpass.pColorAttachments		= dataPtr( renderTargetAttachmentReferences );
	subpass.pResolveAttachments		= 0;
	subpass.pDepthStencilAttachment = desc.mHasDepthStencil ? &depthStencilAttachmentReference : nullptr;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments	= nullptr;

	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass		  = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass		  = 0;
	subpassDependency.srcStageMask		  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependency.dstStageMask		  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask		  = 0;
	subpassDependency.dstAccessMask		  = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subpassDependency.dependencyFlags	  = 0;

	VkRenderPassCreateInfo vkci = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	vkci.pNext					= nullptr;
	vkci.flags					= 0;
	vkci.attachmentCount		= countU32( attachments );
	vkci.pAttachments			= dataPtr( attachments );
	vkci.subpassCount			= 1;
	vkci.pSubpasses				= &subpass;
	vkci.dependencyCount		= 1;
	vkci.pDependencies			= &subpassDependency;

	VkResult vkres = CI_VK_DEVICE_FN( CreateRenderPass(
		getDeviceHandle(),
		&vkci,
		nullptr,
		&mRenderPassHandle ) );
	if ( vkres != VK_SUCCESS ) {
		throw grfx::GraphicsApiExc( "vkCreateRenderPass failed" );
	}
}

RenderPass::~RenderPass()
{
	if ( mRenderPassHandle ) {
		CI_VK_DEVICE_FN( DestroyRenderPass(
			getDeviceHandle(),
			mRenderPassHandle,
			nullptr ) );
		mRenderPassHandle = VK_NULL_HANDLE;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UnbufferedRenderPass

FramebufferRef Framebuffer::create( VkRenderPass renderPassHandle, uint32_t width, uint32_t height, const ImageDesc &desc, vk::DeviceRef device )
{
	Framebuffer::ViewDesc viewDesc = Framebuffer::ViewDesc();

	for ( size_t i = 0; i < desc.mImages.size(); ++i ) {
		ImageView::Options imageViewDesc = ImageView::Desc();
		ImageViewRef	view		  = ImageView::create( desc.mImages[i], imageViewDesc, device );
		viewDesc.addAttachment( view );
	}

	return Framebuffer::create( renderPassHandle, width, height, viewDesc, device );
}

FramebufferRef Framebuffer::create( VkRenderPass renderPassHandle, uint32_t width, uint32_t height, const ViewDesc &desc, vk::DeviceRef device )
{
	if ( !device ) {
		device = app::RendererVk::getCurrentRenderer()->getDevice();
	}

	return FramebufferRef( new Framebuffer( device, renderPassHandle, width, height, desc ) );
}

Framebuffer::Framebuffer( vk::DeviceRef device, VkRenderPass renderPassHandle, uint32_t width, uint32_t height, const ViewDesc &desc )
	: vk::DeviceChildObject( device ),
	  mExtent( { width, height } ),
	  mViews( desc.mViews )
{
	std::vector<VkImageView> attachments;
	for ( size_t i = 0; i < mViews.size(); ++i ) {
		attachments.push_back( desc.mViews[i]->getImageViewHandle() );
	}

	VkFramebufferCreateInfo vkci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	vkci.pNext					 = nullptr;
	vkci.flags					 = 0;
	vkci.renderPass				 = renderPassHandle;
	vkci.attachmentCount		 = countU32( attachments );
	vkci.pAttachments			 = dataPtr( attachments );
	vkci.width					 = width;
	vkci.height					 = height;
	vkci.layers					 = 1;

	VkResult vkres = CI_VK_DEVICE_FN( CreateFramebuffer(
		getDeviceHandle(),
		&vkci,
		nullptr,
		&mFramebufferHandle ) );
	if ( vkres != VK_SUCCESS ) {
		throw VulkanFnFailedExc( "vkCreateFramebuffer", vkres );
	}
}

Framebuffer::~Framebuffer()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BufferedRenderPass

vk::BufferedRenderPassRef BufferedRenderPass::create( uint32_t width, uint32_t height, const FormatDesc &desc, vk::DeviceRef device )
{
	// Create render pass
	RenderPass::Desc renderPassDesc = RenderPass::Desc();
	for ( uint32_t i = 0; i < desc.mRenderTargets.size(); ++i ) {
		renderPassDesc.addRenderTarget( desc.mRenderTargets[i] );
	}
	if ( desc.mHasDepthStencil ) {
		renderPassDesc.setDepthStencil( desc.mDepthStencil );
	}

	RenderPassRef renderPass = RenderPass::create( renderPassDesc, device );

	// Create frame buffer
	Framebuffer::ImageDesc framebufferDesc = Framebuffer::ImageDesc();
	for ( uint32_t i = 0; i < desc.mRenderTargets.size(); ++i ) {
		const VkAttachmentDescription &attachment = desc.mRenderTargets[i];

		Image::Usage imageUsage = Image::Usage().sampledImage().renderTarget();

		Image::Options imageOptions = Image::Options()
									   .samples( attachment.samples );

		ImageRef image = Image::create( width, height, attachment.format, imageUsage, MemoryUsage::GPU_ONLY, imageOptions, device );
		framebufferDesc.addAttachment( image );
	}
	if ( desc.mHasDepthStencil ) {
		const VkAttachmentDescription &attachment = desc.mDepthStencil;

		Image::Usage imageUsage = Image::Usage().sampledImage().depthStencil();

		Image::Options imageOptions = Image::Options()
									   .samples( attachment.samples );

		ImageRef image = Image::create( width, height, attachment.format, imageUsage, MemoryUsage::GPU_ONLY, imageOptions, device );
		framebufferDesc.addAttachment( image );
	}

	FramebufferRef framebuffer = Framebuffer::create( renderPass->getRenderPassHandle(), width, height, framebufferDesc, device );

	return BufferedRenderPassRef( new BufferedRenderPass( device, renderPass, framebuffer ) );
}

vk::BufferedRenderPassRef BufferedRenderPass::create( uint32_t width, uint32_t height, const ImageDesc &desc, vk::DeviceRef device )
{
	// Create render pass
	RenderPass::Desc renderPassDesc = RenderPass::Desc();
	for ( uint32_t i = 0; i < desc.mRenderTargets.size(); ++i ) {
		ImageRef image = desc.mRenderTargets[i].image;

		VkAttachmentDescription attachment = {};
		attachment.flags				   = 0;
		attachment.format				   = image->getFormat();
		attachment.samples				   = image->getSamples();
		attachment.loadOp				   = desc.mRenderTargets[i].loadOp;
		attachment.storeOp				   = desc.mRenderTargets[i].storeOp;
		attachment.stencilLoadOp		   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp		   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout		   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.finalLayout			   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		renderPassDesc.addRenderTarget( attachment );
	}
	if ( desc.mDepthStencil.image ) {
		ImageRef image = desc.mDepthStencil.image;

		VkAttachmentDescription attachment = {};
		attachment.flags				   = 0;
		attachment.format				   = image->getFormat();
		attachment.samples				   = image->getSamples();
		attachment.loadOp				   = desc.mDepthStencil.loadOp;
		attachment.storeOp				   = desc.mDepthStencil.storeOp;
		attachment.stencilLoadOp		   = desc.mDepthStencil.stencilLoadOp;
		attachment.stencilStoreOp		   = desc.mDepthStencil.stencilStoreOp;
		attachment.initialLayout		   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachment.finalLayout			   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		renderPassDesc.setDepthStencil( attachment );
	}

	RenderPassRef renderPass = RenderPass::create( renderPassDesc, device );

	// Create frame buffer
	Framebuffer::ImageDesc framebufferDesc = Framebuffer::ImageDesc();
	for ( uint32_t i = 0; i < desc.mRenderTargets.size(); ++i ) {
		framebufferDesc.addAttachment( desc.mRenderTargets[i].image );
	}
	if ( desc.mDepthStencil.image ) {
		framebufferDesc.addAttachment( desc.mDepthStencil.image );
	}

	FramebufferRef framebuffer = Framebuffer::create( renderPass->getRenderPassHandle(), width, height, framebufferDesc, device );

	return BufferedRenderPassRef( new BufferedRenderPass( device, renderPass, framebuffer ) );
}

vk::BufferedRenderPassRef BufferedRenderPass::create( uint32_t width, uint32_t height, const ViewDesc &desc, vk::DeviceRef device )
{
	// Create render pass
	RenderPass::Desc renderPassDesc = RenderPass::Desc();
	for ( uint32_t i = 0; i < desc.mRenderTargets.size(); ++i ) {
		ImageViewRef view = desc.mRenderTargets[i].view;

		VkAttachmentDescription attachment = {};
		attachment.flags				   = 0;
		attachment.format				   = view->getFormat();
		attachment.samples				   = view->getSamples();
		attachment.loadOp				   = desc.mRenderTargets[i].loadOp;
		attachment.storeOp				   = desc.mRenderTargets[i].storeOp;
		attachment.stencilLoadOp		   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp		   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout		   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.finalLayout			   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		renderPassDesc.addRenderTarget( attachment );
	}
	if ( desc.mDepthStencil.view ) {
		ImageViewRef view = desc.mDepthStencil.view;

		VkAttachmentDescription attachment = {};
		attachment.flags				   = 0;
		attachment.format				   = view->getFormat();
		attachment.samples				   = view->getSamples();
		attachment.loadOp				   = desc.mDepthStencil.loadOp;
		attachment.storeOp				   = desc.mDepthStencil.storeOp;
		attachment.stencilLoadOp		   = desc.mDepthStencil.stencilLoadOp;
		attachment.stencilStoreOp		   = desc.mDepthStencil.stencilStoreOp;
		attachment.initialLayout		   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachment.finalLayout			   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		renderPassDesc.setDepthStencil( attachment );
	}

	RenderPassRef renderPass = RenderPass::create( renderPassDesc, device );

	// Create frame buffer
	Framebuffer::ViewDesc framebufferDesc = Framebuffer::ViewDesc();
	for ( uint32_t i = 0; i < desc.mRenderTargets.size(); ++i ) {
		framebufferDesc.addAttachment( desc.mRenderTargets[i].view );
	}
	if ( desc.mDepthStencil.view ) {
		framebufferDesc.addAttachment( desc.mDepthStencil.view );
	}

	FramebufferRef framebuffer = Framebuffer::create( renderPass->getRenderPassHandle(), width, height, framebufferDesc, device );

	return BufferedRenderPassRef( new BufferedRenderPass( device, renderPass, framebuffer ) );
}

BufferedRenderPass::BufferedRenderPass( vk::DeviceRef device, vk::RenderPassRef renderPass, vk::FramebufferRef framebuffer )
	: mRenderPass( renderPass ),
	  mFramebuffer( framebuffer )
{
	mRenderArea = { { 0, 0 }, mFramebuffer->getExtent() };
}

BufferedRenderPass::~BufferedRenderPass()
{
	mFramebuffer.reset();
	mRenderPass.reset();
}

*/

} // namespace cinder::vk
