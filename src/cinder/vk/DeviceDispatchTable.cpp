#include "cinder/vk/DeviceDispatchTable.h"

namespace cinder::vk {

void DeviceDispatchTable::patchPromotedFunctions()
{
	if ( this->GetSemaphoreCounterValue == nullptr ) {
		this->GetSemaphoreCounterValue = this->GetSemaphoreCounterValueKHR;
	}
	if ( this->WaitSemaphores == nullptr ) {
		this->WaitSemaphores = this->WaitSemaphoresKHR;
	}
	if ( this->SignalSemaphore == nullptr) {
		this->SignalSemaphore = this->SignalSemaphoreKHR;
	}
}

void LoadDeviceFunctions(
	PFN_vkGetDeviceProcAddr			 loadFn,
	VkDevice						 device,
	cinder::vk::DeviceDispatchTable *pTable )
{
	// ---- Core 1_0 commands
	pTable->GetDeviceProcAddr				 = (PFN_vkGetDeviceProcAddr)loadFn( device, "vkGetDeviceProcAddr" );
	pTable->DestroyDevice					 = (PFN_vkDestroyDevice)loadFn( device, "vkDestroyDevice" );
	pTable->GetDeviceQueue					 = (PFN_vkGetDeviceQueue)loadFn( device, "vkGetDeviceQueue" );
	pTable->QueueSubmit						 = (PFN_vkQueueSubmit)loadFn( device, "vkQueueSubmit" );
	pTable->QueueWaitIdle					 = (PFN_vkQueueWaitIdle)loadFn( device, "vkQueueWaitIdle" );
	pTable->DeviceWaitIdle					 = (PFN_vkDeviceWaitIdle)loadFn( device, "vkDeviceWaitIdle" );
	pTable->AllocateMemory					 = (PFN_vkAllocateMemory)loadFn( device, "vkAllocateMemory" );
	pTable->FreeMemory						 = (PFN_vkFreeMemory)loadFn( device, "vkFreeMemory" );
	pTable->MapMemory						 = (PFN_vkMapMemory)loadFn( device, "vkMapMemory" );
	pTable->UnmapMemory						 = (PFN_vkUnmapMemory)loadFn( device, "vkUnmapMemory" );
	pTable->FlushMappedMemoryRanges			 = (PFN_vkFlushMappedMemoryRanges)loadFn( device, "vkFlushMappedMemoryRanges" );
	pTable->InvalidateMappedMemoryRanges	 = (PFN_vkInvalidateMappedMemoryRanges)loadFn( device, "vkInvalidateMappedMemoryRanges" );
	pTable->GetDeviceMemoryCommitment		 = (PFN_vkGetDeviceMemoryCommitment)loadFn( device, "vkGetDeviceMemoryCommitment" );
	pTable->BindBufferMemory				 = (PFN_vkBindBufferMemory)loadFn( device, "vkBindBufferMemory" );
	pTable->BindImageMemory					 = (PFN_vkBindImageMemory)loadFn( device, "vkBindImageMemory" );
	pTable->GetBufferMemoryRequirements		 = (PFN_vkGetBufferMemoryRequirements)loadFn( device, "vkGetBufferMemoryRequirements" );
	pTable->GetImageMemoryRequirements		 = (PFN_vkGetImageMemoryRequirements)loadFn( device, "vkGetImageMemoryRequirements" );
	pTable->GetImageSparseMemoryRequirements = (PFN_vkGetImageSparseMemoryRequirements)loadFn( device, "vkGetImageSparseMemoryRequirements" );
	pTable->QueueBindSparse					 = (PFN_vkQueueBindSparse)loadFn( device, "vkQueueBindSparse" );
	pTable->CreateFence						 = (PFN_vkCreateFence)loadFn( device, "vkCreateFence" );
	pTable->DestroyFence					 = (PFN_vkDestroyFence)loadFn( device, "vkDestroyFence" );
	pTable->ResetFences						 = (PFN_vkResetFences)loadFn( device, "vkResetFences" );
	pTable->GetFenceStatus					 = (PFN_vkGetFenceStatus)loadFn( device, "vkGetFenceStatus" );
	pTable->WaitForFences					 = (PFN_vkWaitForFences)loadFn( device, "vkWaitForFences" );
	pTable->CreateSemaphore					 = (PFN_vkCreateSemaphore)loadFn( device, "vkCreateSemaphore" );
	pTable->DestroySemaphore				 = (PFN_vkDestroySemaphore)loadFn( device, "vkDestroySemaphore" );
	pTable->CreateEvent						 = (PFN_vkCreateEvent)loadFn( device, "vkCreateEvent" );
	pTable->DestroyEvent					 = (PFN_vkDestroyEvent)loadFn( device, "vkDestroyEvent" );
	pTable->GetEventStatus					 = (PFN_vkGetEventStatus)loadFn( device, "vkGetEventStatus" );
	pTable->SetEvent						 = (PFN_vkSetEvent)loadFn( device, "vkSetEvent" );
	pTable->ResetEvent						 = (PFN_vkResetEvent)loadFn( device, "vkResetEvent" );
	pTable->CreateQueryPool					 = (PFN_vkCreateQueryPool)loadFn( device, "vkCreateQueryPool" );
	pTable->DestroyQueryPool				 = (PFN_vkDestroyQueryPool)loadFn( device, "vkDestroyQueryPool" );
	pTable->GetQueryPoolResults				 = (PFN_vkGetQueryPoolResults)loadFn( device, "vkGetQueryPoolResults" );
	pTable->CreateBuffer					 = (PFN_vkCreateBuffer)loadFn( device, "vkCreateBuffer" );
	pTable->DestroyBuffer					 = (PFN_vkDestroyBuffer)loadFn( device, "vkDestroyBuffer" );
	pTable->CreateBufferView				 = (PFN_vkCreateBufferView)loadFn( device, "vkCreateBufferView" );
	pTable->DestroyBufferView				 = (PFN_vkDestroyBufferView)loadFn( device, "vkDestroyBufferView" );
	pTable->CreateImage						 = (PFN_vkCreateImage)loadFn( device, "vkCreateImage" );
	pTable->DestroyImage					 = (PFN_vkDestroyImage)loadFn( device, "vkDestroyImage" );
	pTable->GetImageSubresourceLayout		 = (PFN_vkGetImageSubresourceLayout)loadFn( device, "vkGetImageSubresourceLayout" );
	pTable->CreateImageView					 = (PFN_vkCreateImageView)loadFn( device, "vkCreateImageView" );
	pTable->DestroyImageView				 = (PFN_vkDestroyImageView)loadFn( device, "vkDestroyImageView" );
	pTable->CreateShaderModule				 = (PFN_vkCreateShaderModule)loadFn( device, "vkCreateShaderModule" );
	pTable->DestroyShaderModule				 = (PFN_vkDestroyShaderModule)loadFn( device, "vkDestroyShaderModule" );
	pTable->CreatePipelineCache				 = (PFN_vkCreatePipelineCache)loadFn( device, "vkCreatePipelineCache" );
	pTable->DestroyPipelineCache			 = (PFN_vkDestroyPipelineCache)loadFn( device, "vkDestroyPipelineCache" );
	pTable->GetPipelineCacheData			 = (PFN_vkGetPipelineCacheData)loadFn( device, "vkGetPipelineCacheData" );
	pTable->MergePipelineCaches				 = (PFN_vkMergePipelineCaches)loadFn( device, "vkMergePipelineCaches" );
	pTable->CreateGraphicsPipelines			 = (PFN_vkCreateGraphicsPipelines)loadFn( device, "vkCreateGraphicsPipelines" );
	pTable->CreateComputePipelines			 = (PFN_vkCreateComputePipelines)loadFn( device, "vkCreateComputePipelines" );
	pTable->DestroyPipeline					 = (PFN_vkDestroyPipeline)loadFn( device, "vkDestroyPipeline" );
	pTable->CreatePipelineLayout			 = (PFN_vkCreatePipelineLayout)loadFn( device, "vkCreatePipelineLayout" );
	pTable->DestroyPipelineLayout			 = (PFN_vkDestroyPipelineLayout)loadFn( device, "vkDestroyPipelineLayout" );
	pTable->CreateSampler					 = (PFN_vkCreateSampler)loadFn( device, "vkCreateSampler" );
	pTable->DestroySampler					 = (PFN_vkDestroySampler)loadFn( device, "vkDestroySampler" );
	pTable->CreateDescriptorSetLayout		 = (PFN_vkCreateDescriptorSetLayout)loadFn( device, "vkCreateDescriptorSetLayout" );
	pTable->DestroyDescriptorSetLayout		 = (PFN_vkDestroyDescriptorSetLayout)loadFn( device, "vkDestroyDescriptorSetLayout" );
	pTable->CreateDescriptorPool			 = (PFN_vkCreateDescriptorPool)loadFn( device, "vkCreateDescriptorPool" );
	pTable->DestroyDescriptorPool			 = (PFN_vkDestroyDescriptorPool)loadFn( device, "vkDestroyDescriptorPool" );
	pTable->ResetDescriptorPool				 = (PFN_vkResetDescriptorPool)loadFn( device, "vkResetDescriptorPool" );
	pTable->AllocateDescriptorSets			 = (PFN_vkAllocateDescriptorSets)loadFn( device, "vkAllocateDescriptorSets" );
	pTable->FreeDescriptorSets				 = (PFN_vkFreeDescriptorSets)loadFn( device, "vkFreeDescriptorSets" );
	pTable->UpdateDescriptorSets			 = (PFN_vkUpdateDescriptorSets)loadFn( device, "vkUpdateDescriptorSets" );
	pTable->CreateFramebuffer				 = (PFN_vkCreateFramebuffer)loadFn( device, "vkCreateFramebuffer" );
	pTable->DestroyFramebuffer				 = (PFN_vkDestroyFramebuffer)loadFn( device, "vkDestroyFramebuffer" );
	pTable->CreateRenderPass				 = (PFN_vkCreateRenderPass)loadFn( device, "vkCreateRenderPass" );
	pTable->DestroyRenderPass				 = (PFN_vkDestroyRenderPass)loadFn( device, "vkDestroyRenderPass" );
	pTable->GetRenderAreaGranularity		 = (PFN_vkGetRenderAreaGranularity)loadFn( device, "vkGetRenderAreaGranularity" );
	pTable->CreateCommandPool				 = (PFN_vkCreateCommandPool)loadFn( device, "vkCreateCommandPool" );
	pTable->DestroyCommandPool				 = (PFN_vkDestroyCommandPool)loadFn( device, "vkDestroyCommandPool" );
	pTable->ResetCommandPool				 = (PFN_vkResetCommandPool)loadFn( device, "vkResetCommandPool" );
	pTable->AllocateCommandBuffers			 = (PFN_vkAllocateCommandBuffers)loadFn( device, "vkAllocateCommandBuffers" );
	pTable->FreeCommandBuffers				 = (PFN_vkFreeCommandBuffers)loadFn( device, "vkFreeCommandBuffers" );
	pTable->BeginCommandBuffer				 = (PFN_vkBeginCommandBuffer)loadFn( device, "vkBeginCommandBuffer" );
	pTable->EndCommandBuffer				 = (PFN_vkEndCommandBuffer)loadFn( device, "vkEndCommandBuffer" );
	pTable->ResetCommandBuffer				 = (PFN_vkResetCommandBuffer)loadFn( device, "vkResetCommandBuffer" );
	pTable->CmdBindPipeline					 = (PFN_vkCmdBindPipeline)loadFn( device, "vkCmdBindPipeline" );
	pTable->CmdSetViewport					 = (PFN_vkCmdSetViewport)loadFn( device, "vkCmdSetViewport" );
	pTable->CmdSetScissor					 = (PFN_vkCmdSetScissor)loadFn( device, "vkCmdSetScissor" );
	pTable->CmdSetLineWidth					 = (PFN_vkCmdSetLineWidth)loadFn( device, "vkCmdSetLineWidth" );
	pTable->CmdSetDepthBias					 = (PFN_vkCmdSetDepthBias)loadFn( device, "vkCmdSetDepthBias" );
	pTable->CmdSetBlendConstants			 = (PFN_vkCmdSetBlendConstants)loadFn( device, "vkCmdSetBlendConstants" );
	pTable->CmdSetDepthBounds				 = (PFN_vkCmdSetDepthBounds)loadFn( device, "vkCmdSetDepthBounds" );
	pTable->CmdSetStencilCompareMask		 = (PFN_vkCmdSetStencilCompareMask)loadFn( device, "vkCmdSetStencilCompareMask" );
	pTable->CmdSetStencilWriteMask			 = (PFN_vkCmdSetStencilWriteMask)loadFn( device, "vkCmdSetStencilWriteMask" );
	pTable->CmdSetStencilReference			 = (PFN_vkCmdSetStencilReference)loadFn( device, "vkCmdSetStencilReference" );
	pTable->CmdBindDescriptorSets			 = (PFN_vkCmdBindDescriptorSets)loadFn( device, "vkCmdBindDescriptorSets" );
	pTable->CmdBindIndexBuffer				 = (PFN_vkCmdBindIndexBuffer)loadFn( device, "vkCmdBindIndexBuffer" );
	pTable->CmdBindVertexBuffers			 = (PFN_vkCmdBindVertexBuffers)loadFn( device, "vkCmdBindVertexBuffers" );
	pTable->CmdDraw							 = (PFN_vkCmdDraw)loadFn( device, "vkCmdDraw" );
	pTable->CmdDrawIndexed					 = (PFN_vkCmdDrawIndexed)loadFn( device, "vkCmdDrawIndexed" );
	pTable->CmdDrawIndirect					 = (PFN_vkCmdDrawIndirect)loadFn( device, "vkCmdDrawIndirect" );
	pTable->CmdDrawIndexedIndirect			 = (PFN_vkCmdDrawIndexedIndirect)loadFn( device, "vkCmdDrawIndexedIndirect" );
	pTable->CmdDispatch						 = (PFN_vkCmdDispatch)loadFn( device, "vkCmdDispatch" );
	pTable->CmdDispatchIndirect				 = (PFN_vkCmdDispatchIndirect)loadFn( device, "vkCmdDispatchIndirect" );
	pTable->CmdCopyBuffer					 = (PFN_vkCmdCopyBuffer)loadFn( device, "vkCmdCopyBuffer" );
	pTable->CmdCopyImage					 = (PFN_vkCmdCopyImage)loadFn( device, "vkCmdCopyImage" );
	pTable->CmdBlitImage					 = (PFN_vkCmdBlitImage)loadFn( device, "vkCmdBlitImage" );
	pTable->CmdCopyBufferToImage			 = (PFN_vkCmdCopyBufferToImage)loadFn( device, "vkCmdCopyBufferToImage" );
	pTable->CmdCopyImageToBuffer			 = (PFN_vkCmdCopyImageToBuffer)loadFn( device, "vkCmdCopyImageToBuffer" );
	pTable->CmdUpdateBuffer					 = (PFN_vkCmdUpdateBuffer)loadFn( device, "vkCmdUpdateBuffer" );
	pTable->CmdFillBuffer					 = (PFN_vkCmdFillBuffer)loadFn( device, "vkCmdFillBuffer" );
	pTable->CmdClearColorImage				 = (PFN_vkCmdClearColorImage)loadFn( device, "vkCmdClearColorImage" );
	pTable->CmdClearDepthStencilImage		 = (PFN_vkCmdClearDepthStencilImage)loadFn( device, "vkCmdClearDepthStencilImage" );
	pTable->CmdClearAttachments				 = (PFN_vkCmdClearAttachments)loadFn( device, "vkCmdClearAttachments" );
	pTable->CmdResolveImage					 = (PFN_vkCmdResolveImage)loadFn( device, "vkCmdResolveImage" );
	pTable->CmdSetEvent						 = (PFN_vkCmdSetEvent)loadFn( device, "vkCmdSetEvent" );
	pTable->CmdResetEvent					 = (PFN_vkCmdResetEvent)loadFn( device, "vkCmdResetEvent" );
	pTable->CmdWaitEvents					 = (PFN_vkCmdWaitEvents)loadFn( device, "vkCmdWaitEvents" );
	pTable->CmdPipelineBarrier				 = (PFN_vkCmdPipelineBarrier)loadFn( device, "vkCmdPipelineBarrier" );
	pTable->CmdBeginQuery					 = (PFN_vkCmdBeginQuery)loadFn( device, "vkCmdBeginQuery" );
	pTable->CmdEndQuery						 = (PFN_vkCmdEndQuery)loadFn( device, "vkCmdEndQuery" );
	pTable->CmdResetQueryPool				 = (PFN_vkCmdResetQueryPool)loadFn( device, "vkCmdResetQueryPool" );
	pTable->CmdWriteTimestamp				 = (PFN_vkCmdWriteTimestamp)loadFn( device, "vkCmdWriteTimestamp" );
	pTable->CmdCopyQueryPoolResults			 = (PFN_vkCmdCopyQueryPoolResults)loadFn( device, "vkCmdCopyQueryPoolResults" );
	pTable->CmdPushConstants				 = (PFN_vkCmdPushConstants)loadFn( device, "vkCmdPushConstants" );
	pTable->CmdBeginRenderPass				 = (PFN_vkCmdBeginRenderPass)loadFn( device, "vkCmdBeginRenderPass" );
	pTable->CmdNextSubpass					 = (PFN_vkCmdNextSubpass)loadFn( device, "vkCmdNextSubpass" );
	pTable->CmdEndRenderPass				 = (PFN_vkCmdEndRenderPass)loadFn( device, "vkCmdEndRenderPass" );
	pTable->CmdExecuteCommands				 = (PFN_vkCmdExecuteCommands)loadFn( device, "vkCmdExecuteCommands" );

	// ---- Core 1_1 commands
	pTable->BindBufferMemory2				  = (PFN_vkBindBufferMemory2)loadFn( device, "vkBindBufferMemory2" );
	pTable->BindImageMemory2				  = (PFN_vkBindImageMemory2)loadFn( device, "vkBindImageMemory2" );
	pTable->GetDeviceGroupPeerMemoryFeatures  = (PFN_vkGetDeviceGroupPeerMemoryFeatures)loadFn( device, "vkGetDeviceGroupPeerMemoryFeatures" );
	pTable->CmdSetDeviceMask				  = (PFN_vkCmdSetDeviceMask)loadFn( device, "vkCmdSetDeviceMask" );
	pTable->CmdDispatchBase					  = (PFN_vkCmdDispatchBase)loadFn( device, "vkCmdDispatchBase" );
	pTable->GetImageMemoryRequirements2		  = (PFN_vkGetImageMemoryRequirements2)loadFn( device, "vkGetImageMemoryRequirements2" );
	pTable->GetBufferMemoryRequirements2	  = (PFN_vkGetBufferMemoryRequirements2)loadFn( device, "vkGetBufferMemoryRequirements2" );
	pTable->GetImageSparseMemoryRequirements2 = (PFN_vkGetImageSparseMemoryRequirements2)loadFn( device, "vkGetImageSparseMemoryRequirements2" );
	pTable->TrimCommandPool					  = (PFN_vkTrimCommandPool)loadFn( device, "vkTrimCommandPool" );
	pTable->GetDeviceQueue2					  = (PFN_vkGetDeviceQueue2)loadFn( device, "vkGetDeviceQueue2" );
	pTable->CreateSamplerYcbcrConversion	  = (PFN_vkCreateSamplerYcbcrConversion)loadFn( device, "vkCreateSamplerYcbcrConversion" );
	pTable->DestroySamplerYcbcrConversion	  = (PFN_vkDestroySamplerYcbcrConversion)loadFn( device, "vkDestroySamplerYcbcrConversion" );
	pTable->CreateDescriptorUpdateTemplate	  = (PFN_vkCreateDescriptorUpdateTemplate)loadFn( device, "vkCreateDescriptorUpdateTemplate" );
	pTable->DestroyDescriptorUpdateTemplate	  = (PFN_vkDestroyDescriptorUpdateTemplate)loadFn( device, "vkDestroyDescriptorUpdateTemplate" );
	pTable->UpdateDescriptorSetWithTemplate	  = (PFN_vkUpdateDescriptorSetWithTemplate)loadFn( device, "vkUpdateDescriptorSetWithTemplate" );
	pTable->GetDescriptorSetLayoutSupport	  = (PFN_vkGetDescriptorSetLayoutSupport)loadFn( device, "vkGetDescriptorSetLayoutSupport" );

	// ---- Core 1_2 commands
	pTable->CmdDrawIndirectCount				= (PFN_vkCmdDrawIndirectCount)loadFn( device, "vkCmdDrawIndirectCount" );
	pTable->CmdDrawIndexedIndirectCount			= (PFN_vkCmdDrawIndexedIndirectCount)loadFn( device, "vkCmdDrawIndexedIndirectCount" );
	pTable->CreateRenderPass2					= (PFN_vkCreateRenderPass2)loadFn( device, "vkCreateRenderPass2" );
	pTable->CmdBeginRenderPass2					= (PFN_vkCmdBeginRenderPass2)loadFn( device, "vkCmdBeginRenderPass2" );
	pTable->CmdNextSubpass2						= (PFN_vkCmdNextSubpass2)loadFn( device, "vkCmdNextSubpass2" );
	pTable->CmdEndRenderPass2					= (PFN_vkCmdEndRenderPass2)loadFn( device, "vkCmdEndRenderPass2" );
	pTable->ResetQueryPool						= (PFN_vkResetQueryPool)loadFn( device, "vkResetQueryPool" );
	pTable->GetSemaphoreCounterValue			= (PFN_vkGetSemaphoreCounterValue)loadFn( device, "vkGetSemaphoreCounterValue" );
	pTable->WaitSemaphores						= (PFN_vkWaitSemaphores)loadFn( device, "vkWaitSemaphores" );
	pTable->SignalSemaphore						= (PFN_vkSignalSemaphore)loadFn( device, "vkSignalSemaphore" );
	pTable->GetBufferDeviceAddress				= (PFN_vkGetBufferDeviceAddress)loadFn( device, "vkGetBufferDeviceAddress" );
	pTable->GetBufferOpaqueCaptureAddress		= (PFN_vkGetBufferOpaqueCaptureAddress)loadFn( device, "vkGetBufferOpaqueCaptureAddress" );
	pTable->GetDeviceMemoryOpaqueCaptureAddress = (PFN_vkGetDeviceMemoryOpaqueCaptureAddress)loadFn( device, "vkGetDeviceMemoryOpaqueCaptureAddress" );

	// ---- VK_KHR_swapchain extension commands
	pTable->CreateSwapchainKHR					 = (PFN_vkCreateSwapchainKHR)loadFn( device, "vkCreateSwapchainKHR" );
	pTable->DestroySwapchainKHR					 = (PFN_vkDestroySwapchainKHR)loadFn( device, "vkDestroySwapchainKHR" );
	pTable->GetSwapchainImagesKHR				 = (PFN_vkGetSwapchainImagesKHR)loadFn( device, "vkGetSwapchainImagesKHR" );
	pTable->AcquireNextImageKHR					 = (PFN_vkAcquireNextImageKHR)loadFn( device, "vkAcquireNextImageKHR" );
	pTable->QueuePresentKHR						 = (PFN_vkQueuePresentKHR)loadFn( device, "vkQueuePresentKHR" );
	pTable->GetDeviceGroupPresentCapabilitiesKHR = (PFN_vkGetDeviceGroupPresentCapabilitiesKHR)loadFn( device, "vkGetDeviceGroupPresentCapabilitiesKHR" );
	pTable->GetDeviceGroupSurfacePresentModesKHR = (PFN_vkGetDeviceGroupSurfacePresentModesKHR)loadFn( device, "vkGetDeviceGroupSurfacePresentModesKHR" );
	pTable->AcquireNextImage2KHR				 = (PFN_vkAcquireNextImage2KHR)loadFn( device, "vkAcquireNextImage2KHR" );

	// ---- VK_KHR_display_swapchain extension commands
	pTable->CreateSharedSwapchainsKHR = (PFN_vkCreateSharedSwapchainsKHR)loadFn( device, "vkCreateSharedSwapchainsKHR" );

	// ---- VK_KHR_video_queue extension commands
#ifdef VK_ENABLE_BETA_EXTENSIONS
	pTable->CreateVideoSessionKHR = (PFN_vkCreateVideoSessionKHR)loadFn( device, "vkCreateVideoSessionKHR" );
#endif // VK_ENABLE_BETA_EXTENSIONS
#ifdef VK_ENABLE_BETA_EXTENSIONS
	pTable->DestroyVideoSessionKHR = (PFN_vkDestroyVideoSessionKHR)loadFn( device, "vkDestroyVideoSessionKHR" );
#endif // VK_ENABLE_BETA_EXTENSIONS
#ifdef VK_ENABLE_BETA_EXTENSIONS
	pTable->GetVideoSessionMemoryRequirementsKHR = (PFN_vkGetVideoSessionMemoryRequirementsKHR)loadFn( device, "vkGetVideoSessionMemoryRequirementsKHR" );
#endif // VK_ENABLE_BETA_EXTENSIONS
#ifdef VK_ENABLE_BETA_EXTENSIONS
	pTable->BindVideoSessionMemoryKHR = (PFN_vkBindVideoSessionMemoryKHR)loadFn( device, "vkBindVideoSessionMemoryKHR" );
#endif // VK_ENABLE_BETA_EXTENSIONS
#ifdef VK_ENABLE_BETA_EXTENSIONS
	pTable->CreateVideoSessionParametersKHR = (PFN_vkCreateVideoSessionParametersKHR)loadFn( device, "vkCreateVideoSessionParametersKHR" );
#endif // VK_ENABLE_BETA_EXTENSIONS
#ifdef VK_ENABLE_BETA_EXTENSIONS
	pTable->UpdateVideoSessionParametersKHR = (PFN_vkUpdateVideoSessionParametersKHR)loadFn( device, "vkUpdateVideoSessionParametersKHR" );
#endif // VK_ENABLE_BETA_EXTENSIONS
#ifdef VK_ENABLE_BETA_EXTENSIONS
	pTable->DestroyVideoSessionParametersKHR = (PFN_vkDestroyVideoSessionParametersKHR)loadFn( device, "vkDestroyVideoSessionParametersKHR" );
#endif // VK_ENABLE_BETA_EXTENSIONS
#ifdef VK_ENABLE_BETA_EXTENSIONS
	pTable->CmdBeginVideoCodingKHR = (PFN_vkCmdBeginVideoCodingKHR)loadFn( device, "vkCmdBeginVideoCodingKHR" );
#endif // VK_ENABLE_BETA_EXTENSIONS
#ifdef VK_ENABLE_BETA_EXTENSIONS
	pTable->CmdEndVideoCodingKHR = (PFN_vkCmdEndVideoCodingKHR)loadFn( device, "vkCmdEndVideoCodingKHR" );
#endif // VK_ENABLE_BETA_EXTENSIONS
#ifdef VK_ENABLE_BETA_EXTENSIONS
	pTable->CmdControlVideoCodingKHR = (PFN_vkCmdControlVideoCodingKHR)loadFn( device, "vkCmdControlVideoCodingKHR" );
#endif // VK_ENABLE_BETA_EXTENSIONS

	// ---- VK_KHR_video_decode_queue extension commands
#ifdef VK_ENABLE_BETA_EXTENSIONS
	pTable->CmdDecodeVideoKHR = (PFN_vkCmdDecodeVideoKHR)loadFn( device, "vkCmdDecodeVideoKHR" );
#endif // VK_ENABLE_BETA_EXTENSIONS

	// ---- VK_KHR_dynamic_rendering extension commands
	pTable->CmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)loadFn( device, "vkCmdBeginRenderingKHR" );
	pTable->CmdEndRenderingKHR	 = (PFN_vkCmdEndRenderingKHR)loadFn( device, "vkCmdEndRenderingKHR" );

	// ---- VK_KHR_device_group extension commands
	pTable->GetDeviceGroupPeerMemoryFeaturesKHR = (PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR)loadFn( device, "vkGetDeviceGroupPeerMemoryFeaturesKHR" );
	pTable->CmdSetDeviceMaskKHR					= (PFN_vkCmdSetDeviceMaskKHR)loadFn( device, "vkCmdSetDeviceMaskKHR" );
	pTable->CmdDispatchBaseKHR					= (PFN_vkCmdDispatchBaseKHR)loadFn( device, "vkCmdDispatchBaseKHR" );

	// ---- VK_KHR_maintenance1 extension commands
	pTable->TrimCommandPoolKHR = (PFN_vkTrimCommandPoolKHR)loadFn( device, "vkTrimCommandPoolKHR" );

	// ---- VK_KHR_external_memory_win32 extension commands
#ifdef VK_USE_PLATFORM_WIN32_KHR
	pTable->GetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)loadFn( device, "vkGetMemoryWin32HandleKHR" );
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
	pTable->GetMemoryWin32HandlePropertiesKHR = (PFN_vkGetMemoryWin32HandlePropertiesKHR)loadFn( device, "vkGetMemoryWin32HandlePropertiesKHR" );
#endif // VK_USE_PLATFORM_WIN32_KHR

	// ---- VK_KHR_external_memory_fd extension commands
	pTable->GetMemoryFdKHR			 = (PFN_vkGetMemoryFdKHR)loadFn( device, "vkGetMemoryFdKHR" );
	pTable->GetMemoryFdPropertiesKHR = (PFN_vkGetMemoryFdPropertiesKHR)loadFn( device, "vkGetMemoryFdPropertiesKHR" );

	// ---- VK_KHR_external_semaphore_win32 extension commands
#ifdef VK_USE_PLATFORM_WIN32_KHR
	pTable->ImportSemaphoreWin32HandleKHR = (PFN_vkImportSemaphoreWin32HandleKHR)loadFn( device, "vkImportSemaphoreWin32HandleKHR" );
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
	pTable->GetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)loadFn( device, "vkGetSemaphoreWin32HandleKHR" );
#endif // VK_USE_PLATFORM_WIN32_KHR

	// ---- VK_KHR_external_semaphore_fd extension commands
	pTable->ImportSemaphoreFdKHR = (PFN_vkImportSemaphoreFdKHR)loadFn( device, "vkImportSemaphoreFdKHR" );
	pTable->GetSemaphoreFdKHR	 = (PFN_vkGetSemaphoreFdKHR)loadFn( device, "vkGetSemaphoreFdKHR" );

	// ---- VK_KHR_push_descriptor extension commands
	pTable->CmdPushDescriptorSetKHR				= (PFN_vkCmdPushDescriptorSetKHR)loadFn( device, "vkCmdPushDescriptorSetKHR" );
	pTable->CmdPushDescriptorSetWithTemplateKHR = (PFN_vkCmdPushDescriptorSetWithTemplateKHR)loadFn( device, "vkCmdPushDescriptorSetWithTemplateKHR" );

	// ---- VK_KHR_descriptor_update_template extension commands
	pTable->CreateDescriptorUpdateTemplateKHR  = (PFN_vkCreateDescriptorUpdateTemplateKHR)loadFn( device, "vkCreateDescriptorUpdateTemplateKHR" );
	pTable->DestroyDescriptorUpdateTemplateKHR = (PFN_vkDestroyDescriptorUpdateTemplateKHR)loadFn( device, "vkDestroyDescriptorUpdateTemplateKHR" );
	pTable->UpdateDescriptorSetWithTemplateKHR = (PFN_vkUpdateDescriptorSetWithTemplateKHR)loadFn( device, "vkUpdateDescriptorSetWithTemplateKHR" );

	// ---- VK_KHR_create_renderpass2 extension commands
	pTable->CreateRenderPass2KHR   = (PFN_vkCreateRenderPass2KHR)loadFn( device, "vkCreateRenderPass2KHR" );
	pTable->CmdBeginRenderPass2KHR = (PFN_vkCmdBeginRenderPass2KHR)loadFn( device, "vkCmdBeginRenderPass2KHR" );
	pTable->CmdNextSubpass2KHR	   = (PFN_vkCmdNextSubpass2KHR)loadFn( device, "vkCmdNextSubpass2KHR" );
	pTable->CmdEndRenderPass2KHR   = (PFN_vkCmdEndRenderPass2KHR)loadFn( device, "vkCmdEndRenderPass2KHR" );

	// ---- VK_KHR_shared_presentable_image extension commands
	pTable->GetSwapchainStatusKHR = (PFN_vkGetSwapchainStatusKHR)loadFn( device, "vkGetSwapchainStatusKHR" );

	// ---- VK_KHR_external_fence_win32 extension commands
#ifdef VK_USE_PLATFORM_WIN32_KHR
	pTable->ImportFenceWin32HandleKHR = (PFN_vkImportFenceWin32HandleKHR)loadFn( device, "vkImportFenceWin32HandleKHR" );
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
	pTable->GetFenceWin32HandleKHR = (PFN_vkGetFenceWin32HandleKHR)loadFn( device, "vkGetFenceWin32HandleKHR" );
#endif // VK_USE_PLATFORM_WIN32_KHR

	// ---- VK_KHR_external_fence_fd extension commands
	pTable->ImportFenceFdKHR = (PFN_vkImportFenceFdKHR)loadFn( device, "vkImportFenceFdKHR" );
	pTable->GetFenceFdKHR	 = (PFN_vkGetFenceFdKHR)loadFn( device, "vkGetFenceFdKHR" );

	// ---- VK_KHR_performance_query extension commands
	pTable->AcquireProfilingLockKHR = (PFN_vkAcquireProfilingLockKHR)loadFn( device, "vkAcquireProfilingLockKHR" );
	pTable->ReleaseProfilingLockKHR = (PFN_vkReleaseProfilingLockKHR)loadFn( device, "vkReleaseProfilingLockKHR" );

	// ---- VK_KHR_get_memory_requirements2 extension commands
	pTable->GetImageMemoryRequirements2KHR		 = (PFN_vkGetImageMemoryRequirements2KHR)loadFn( device, "vkGetImageMemoryRequirements2KHR" );
	pTable->GetBufferMemoryRequirements2KHR		 = (PFN_vkGetBufferMemoryRequirements2KHR)loadFn( device, "vkGetBufferMemoryRequirements2KHR" );
	pTable->GetImageSparseMemoryRequirements2KHR = (PFN_vkGetImageSparseMemoryRequirements2KHR)loadFn( device, "vkGetImageSparseMemoryRequirements2KHR" );

	// ---- VK_KHR_sampler_ycbcr_conversion extension commands
	pTable->CreateSamplerYcbcrConversionKHR	 = (PFN_vkCreateSamplerYcbcrConversionKHR)loadFn( device, "vkCreateSamplerYcbcrConversionKHR" );
	pTable->DestroySamplerYcbcrConversionKHR = (PFN_vkDestroySamplerYcbcrConversionKHR)loadFn( device, "vkDestroySamplerYcbcrConversionKHR" );

	// ---- VK_KHR_bind_memory2 extension commands
	pTable->BindBufferMemory2KHR = (PFN_vkBindBufferMemory2KHR)loadFn( device, "vkBindBufferMemory2KHR" );
	pTable->BindImageMemory2KHR	 = (PFN_vkBindImageMemory2KHR)loadFn( device, "vkBindImageMemory2KHR" );

	// ---- VK_KHR_maintenance3 extension commands
	pTable->GetDescriptorSetLayoutSupportKHR = (PFN_vkGetDescriptorSetLayoutSupportKHR)loadFn( device, "vkGetDescriptorSetLayoutSupportKHR" );

	// ---- VK_KHR_draw_indirect_count extension commands
	pTable->CmdDrawIndirectCountKHR		   = (PFN_vkCmdDrawIndirectCountKHR)loadFn( device, "vkCmdDrawIndirectCountKHR" );
	pTable->CmdDrawIndexedIndirectCountKHR = (PFN_vkCmdDrawIndexedIndirectCountKHR)loadFn( device, "vkCmdDrawIndexedIndirectCountKHR" );

	// ---- VK_KHR_timeline_semaphore extension commands
	pTable->GetSemaphoreCounterValueKHR = (PFN_vkGetSemaphoreCounterValueKHR)loadFn( device, "vkGetSemaphoreCounterValueKHR" );
	pTable->WaitSemaphoresKHR			= (PFN_vkWaitSemaphoresKHR)loadFn( device, "vkWaitSemaphoresKHR" );
	pTable->SignalSemaphoreKHR			= (PFN_vkSignalSemaphoreKHR)loadFn( device, "vkSignalSemaphoreKHR" );

	// ---- VK_KHR_fragment_shading_rate extension commands
	pTable->CmdSetFragmentShadingRateKHR = (PFN_vkCmdSetFragmentShadingRateKHR)loadFn( device, "vkCmdSetFragmentShadingRateKHR" );

	// ---- VK_KHR_present_wait extension commands
	pTable->WaitForPresentKHR = (PFN_vkWaitForPresentKHR)loadFn( device, "vkWaitForPresentKHR" );

	// ---- VK_KHR_buffer_device_address extension commands
	pTable->GetBufferDeviceAddressKHR			   = (PFN_vkGetBufferDeviceAddressKHR)loadFn( device, "vkGetBufferDeviceAddressKHR" );
	pTable->GetBufferOpaqueCaptureAddressKHR	   = (PFN_vkGetBufferOpaqueCaptureAddressKHR)loadFn( device, "vkGetBufferOpaqueCaptureAddressKHR" );
	pTable->GetDeviceMemoryOpaqueCaptureAddressKHR = (PFN_vkGetDeviceMemoryOpaqueCaptureAddressKHR)loadFn( device, "vkGetDeviceMemoryOpaqueCaptureAddressKHR" );

	// ---- VK_KHR_deferred_host_operations extension commands
	pTable->CreateDeferredOperationKHR			  = (PFN_vkCreateDeferredOperationKHR)loadFn( device, "vkCreateDeferredOperationKHR" );
	pTable->DestroyDeferredOperationKHR			  = (PFN_vkDestroyDeferredOperationKHR)loadFn( device, "vkDestroyDeferredOperationKHR" );
	pTable->GetDeferredOperationMaxConcurrencyKHR = (PFN_vkGetDeferredOperationMaxConcurrencyKHR)loadFn( device, "vkGetDeferredOperationMaxConcurrencyKHR" );
	pTable->GetDeferredOperationResultKHR		  = (PFN_vkGetDeferredOperationResultKHR)loadFn( device, "vkGetDeferredOperationResultKHR" );
	pTable->DeferredOperationJoinKHR			  = (PFN_vkDeferredOperationJoinKHR)loadFn( device, "vkDeferredOperationJoinKHR" );

	// ---- VK_KHR_pipeline_executable_properties extension commands
	pTable->GetPipelineExecutablePropertiesKHR				= (PFN_vkGetPipelineExecutablePropertiesKHR)loadFn( device, "vkGetPipelineExecutablePropertiesKHR" );
	pTable->GetPipelineExecutableStatisticsKHR				= (PFN_vkGetPipelineExecutableStatisticsKHR)loadFn( device, "vkGetPipelineExecutableStatisticsKHR" );
	pTable->GetPipelineExecutableInternalRepresentationsKHR = (PFN_vkGetPipelineExecutableInternalRepresentationsKHR)loadFn( device, "vkGetPipelineExecutableInternalRepresentationsKHR" );

	// ---- VK_KHR_video_encode_queue extension commands
#ifdef VK_ENABLE_BETA_EXTENSIONS
	pTable->CmdEncodeVideoKHR = (PFN_vkCmdEncodeVideoKHR)loadFn( device, "vkCmdEncodeVideoKHR" );
#endif // VK_ENABLE_BETA_EXTENSIONS

	// ---- VK_KHR_synchronization2 extension commands
	pTable->CmdSetEvent2KHR			  = (PFN_vkCmdSetEvent2KHR)loadFn( device, "vkCmdSetEvent2KHR" );
	pTable->CmdResetEvent2KHR		  = (PFN_vkCmdResetEvent2KHR)loadFn( device, "vkCmdResetEvent2KHR" );
	pTable->CmdWaitEvents2KHR		  = (PFN_vkCmdWaitEvents2KHR)loadFn( device, "vkCmdWaitEvents2KHR" );
	pTable->CmdPipelineBarrier2KHR	  = (PFN_vkCmdPipelineBarrier2KHR)loadFn( device, "vkCmdPipelineBarrier2KHR" );
	pTable->CmdWriteTimestamp2KHR	  = (PFN_vkCmdWriteTimestamp2KHR)loadFn( device, "vkCmdWriteTimestamp2KHR" );
	pTable->QueueSubmit2KHR			  = (PFN_vkQueueSubmit2KHR)loadFn( device, "vkQueueSubmit2KHR" );
	pTable->CmdWriteBufferMarker2AMD  = (PFN_vkCmdWriteBufferMarker2AMD)loadFn( device, "vkCmdWriteBufferMarker2AMD" );
	pTable->GetQueueCheckpointData2NV = (PFN_vkGetQueueCheckpointData2NV)loadFn( device, "vkGetQueueCheckpointData2NV" );

	// ---- VK_KHR_copy_commands2 extension commands
	pTable->CmdCopyBuffer2KHR		 = (PFN_vkCmdCopyBuffer2KHR)loadFn( device, "vkCmdCopyBuffer2KHR" );
	pTable->CmdCopyImage2KHR		 = (PFN_vkCmdCopyImage2KHR)loadFn( device, "vkCmdCopyImage2KHR" );
	pTable->CmdCopyBufferToImage2KHR = (PFN_vkCmdCopyBufferToImage2KHR)loadFn( device, "vkCmdCopyBufferToImage2KHR" );
	pTable->CmdCopyImageToBuffer2KHR = (PFN_vkCmdCopyImageToBuffer2KHR)loadFn( device, "vkCmdCopyImageToBuffer2KHR" );
	pTable->CmdBlitImage2KHR		 = (PFN_vkCmdBlitImage2KHR)loadFn( device, "vkCmdBlitImage2KHR" );
	pTable->CmdResolveImage2KHR		 = (PFN_vkCmdResolveImage2KHR)loadFn( device, "vkCmdResolveImage2KHR" );

	// ---- VK_KHR_maintenance4 extension commands
	pTable->GetDeviceBufferMemoryRequirementsKHR	  = (PFN_vkGetDeviceBufferMemoryRequirementsKHR)loadFn( device, "vkGetDeviceBufferMemoryRequirementsKHR" );
	pTable->GetDeviceImageMemoryRequirementsKHR		  = (PFN_vkGetDeviceImageMemoryRequirementsKHR)loadFn( device, "vkGetDeviceImageMemoryRequirementsKHR" );
	pTable->GetDeviceImageSparseMemoryRequirementsKHR = (PFN_vkGetDeviceImageSparseMemoryRequirementsKHR)loadFn( device, "vkGetDeviceImageSparseMemoryRequirementsKHR" );

	// ---- VK_EXT_debug_marker extension commands
	pTable->DebugMarkerSetObjectTagEXT	= (PFN_vkDebugMarkerSetObjectTagEXT)loadFn( device, "vkDebugMarkerSetObjectTagEXT" );
	pTable->DebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)loadFn( device, "vkDebugMarkerSetObjectNameEXT" );
	pTable->CmdDebugMarkerBeginEXT		= (PFN_vkCmdDebugMarkerBeginEXT)loadFn( device, "vkCmdDebugMarkerBeginEXT" );
	pTable->CmdDebugMarkerEndEXT		= (PFN_vkCmdDebugMarkerEndEXT)loadFn( device, "vkCmdDebugMarkerEndEXT" );
	pTable->CmdDebugMarkerInsertEXT		= (PFN_vkCmdDebugMarkerInsertEXT)loadFn( device, "vkCmdDebugMarkerInsertEXT" );

	// ---- VK_EXT_transform_feedback extension commands
	pTable->CmdBindTransformFeedbackBuffersEXT = (PFN_vkCmdBindTransformFeedbackBuffersEXT)loadFn( device, "vkCmdBindTransformFeedbackBuffersEXT" );
	pTable->CmdBeginTransformFeedbackEXT	   = (PFN_vkCmdBeginTransformFeedbackEXT)loadFn( device, "vkCmdBeginTransformFeedbackEXT" );
	pTable->CmdEndTransformFeedbackEXT		   = (PFN_vkCmdEndTransformFeedbackEXT)loadFn( device, "vkCmdEndTransformFeedbackEXT" );
	pTable->CmdBeginQueryIndexedEXT			   = (PFN_vkCmdBeginQueryIndexedEXT)loadFn( device, "vkCmdBeginQueryIndexedEXT" );
	pTable->CmdEndQueryIndexedEXT			   = (PFN_vkCmdEndQueryIndexedEXT)loadFn( device, "vkCmdEndQueryIndexedEXT" );
	pTable->CmdDrawIndirectByteCountEXT		   = (PFN_vkCmdDrawIndirectByteCountEXT)loadFn( device, "vkCmdDrawIndirectByteCountEXT" );

	// ---- VK_NVX_binary_import extension commands
	pTable->CreateCuModuleNVX	 = (PFN_vkCreateCuModuleNVX)loadFn( device, "vkCreateCuModuleNVX" );
	pTable->CreateCuFunctionNVX	 = (PFN_vkCreateCuFunctionNVX)loadFn( device, "vkCreateCuFunctionNVX" );
	pTable->DestroyCuModuleNVX	 = (PFN_vkDestroyCuModuleNVX)loadFn( device, "vkDestroyCuModuleNVX" );
	pTable->DestroyCuFunctionNVX = (PFN_vkDestroyCuFunctionNVX)loadFn( device, "vkDestroyCuFunctionNVX" );
	pTable->CmdCuLaunchKernelNVX = (PFN_vkCmdCuLaunchKernelNVX)loadFn( device, "vkCmdCuLaunchKernelNVX" );

	// ---- VK_NVX_image_view_handle extension commands
	pTable->GetImageViewHandleNVX  = (PFN_vkGetImageViewHandleNVX)loadFn( device, "vkGetImageViewHandleNVX" );
	pTable->GetImageViewAddressNVX = (PFN_vkGetImageViewAddressNVX)loadFn( device, "vkGetImageViewAddressNVX" );

	// ---- VK_AMD_draw_indirect_count extension commands
	pTable->CmdDrawIndirectCountAMD		   = (PFN_vkCmdDrawIndirectCountAMD)loadFn( device, "vkCmdDrawIndirectCountAMD" );
	pTable->CmdDrawIndexedIndirectCountAMD = (PFN_vkCmdDrawIndexedIndirectCountAMD)loadFn( device, "vkCmdDrawIndexedIndirectCountAMD" );

	// ---- VK_AMD_shader_info extension commands
	pTable->GetShaderInfoAMD = (PFN_vkGetShaderInfoAMD)loadFn( device, "vkGetShaderInfoAMD" );

	// ---- VK_NV_external_memory_win32 extension commands
#ifdef VK_USE_PLATFORM_WIN32_KHR
	pTable->GetMemoryWin32HandleNV = (PFN_vkGetMemoryWin32HandleNV)loadFn( device, "vkGetMemoryWin32HandleNV" );
#endif // VK_USE_PLATFORM_WIN32_KHR

	// ---- VK_EXT_conditional_rendering extension commands
	pTable->CmdBeginConditionalRenderingEXT = (PFN_vkCmdBeginConditionalRenderingEXT)loadFn( device, "vkCmdBeginConditionalRenderingEXT" );
	pTable->CmdEndConditionalRenderingEXT	= (PFN_vkCmdEndConditionalRenderingEXT)loadFn( device, "vkCmdEndConditionalRenderingEXT" );

	// ---- VK_NV_clip_space_w_scaling extension commands
	pTable->CmdSetViewportWScalingNV = (PFN_vkCmdSetViewportWScalingNV)loadFn( device, "vkCmdSetViewportWScalingNV" );

	// ---- VK_EXT_display_control extension commands
	pTable->DisplayPowerControlEXT	= (PFN_vkDisplayPowerControlEXT)loadFn( device, "vkDisplayPowerControlEXT" );
	pTable->RegisterDeviceEventEXT	= (PFN_vkRegisterDeviceEventEXT)loadFn( device, "vkRegisterDeviceEventEXT" );
	pTable->RegisterDisplayEventEXT = (PFN_vkRegisterDisplayEventEXT)loadFn( device, "vkRegisterDisplayEventEXT" );
	pTable->GetSwapchainCounterEXT	= (PFN_vkGetSwapchainCounterEXT)loadFn( device, "vkGetSwapchainCounterEXT" );

	// ---- VK_GOOGLE_display_timing extension commands
	pTable->GetRefreshCycleDurationGOOGLE	= (PFN_vkGetRefreshCycleDurationGOOGLE)loadFn( device, "vkGetRefreshCycleDurationGOOGLE" );
	pTable->GetPastPresentationTimingGOOGLE = (PFN_vkGetPastPresentationTimingGOOGLE)loadFn( device, "vkGetPastPresentationTimingGOOGLE" );

	// ---- VK_EXT_discard_rectangles extension commands
	pTable->CmdSetDiscardRectangleEXT = (PFN_vkCmdSetDiscardRectangleEXT)loadFn( device, "vkCmdSetDiscardRectangleEXT" );

	// ---- VK_EXT_hdr_metadata extension commands
	pTable->SetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT)loadFn( device, "vkSetHdrMetadataEXT" );

	// ---- VK_EXT_debug_utils extension commands
	pTable->SetDebugUtilsObjectNameEXT	  = (PFN_vkSetDebugUtilsObjectNameEXT)loadFn( device, "vkSetDebugUtilsObjectNameEXT" );
	pTable->SetDebugUtilsObjectTagEXT	  = (PFN_vkSetDebugUtilsObjectTagEXT)loadFn( device, "vkSetDebugUtilsObjectTagEXT" );
	pTable->QueueBeginDebugUtilsLabelEXT  = (PFN_vkQueueBeginDebugUtilsLabelEXT)loadFn( device, "vkQueueBeginDebugUtilsLabelEXT" );
	pTable->QueueEndDebugUtilsLabelEXT	  = (PFN_vkQueueEndDebugUtilsLabelEXT)loadFn( device, "vkQueueEndDebugUtilsLabelEXT" );
	pTable->QueueInsertDebugUtilsLabelEXT = (PFN_vkQueueInsertDebugUtilsLabelEXT)loadFn( device, "vkQueueInsertDebugUtilsLabelEXT" );
	pTable->CmdBeginDebugUtilsLabelEXT	  = (PFN_vkCmdBeginDebugUtilsLabelEXT)loadFn( device, "vkCmdBeginDebugUtilsLabelEXT" );
	pTable->CmdEndDebugUtilsLabelEXT	  = (PFN_vkCmdEndDebugUtilsLabelEXT)loadFn( device, "vkCmdEndDebugUtilsLabelEXT" );
	pTable->CmdInsertDebugUtilsLabelEXT	  = (PFN_vkCmdInsertDebugUtilsLabelEXT)loadFn( device, "vkCmdInsertDebugUtilsLabelEXT" );

	// ---- VK_ANDROID_external_memory_android_hardware_buffer extension commands
#ifdef VK_USE_PLATFORM_ANDROID_KHR
	pTable->GetAndroidHardwareBufferPropertiesANDROID = (PFN_vkGetAndroidHardwareBufferPropertiesANDROID)loadFn( device, "vkGetAndroidHardwareBufferPropertiesANDROID" );
#endif // VK_USE_PLATFORM_ANDROID_KHR
#ifdef VK_USE_PLATFORM_ANDROID_KHR
	pTable->GetMemoryAndroidHardwareBufferANDROID = (PFN_vkGetMemoryAndroidHardwareBufferANDROID)loadFn( device, "vkGetMemoryAndroidHardwareBufferANDROID" );
#endif // VK_USE_PLATFORM_ANDROID_KHR

	// ---- VK_EXT_sample_locations extension commands
	pTable->CmdSetSampleLocationsEXT = (PFN_vkCmdSetSampleLocationsEXT)loadFn( device, "vkCmdSetSampleLocationsEXT" );

	// ---- VK_EXT_image_drm_format_modifier extension commands
	pTable->GetImageDrmFormatModifierPropertiesEXT = (PFN_vkGetImageDrmFormatModifierPropertiesEXT)loadFn( device, "vkGetImageDrmFormatModifierPropertiesEXT" );

	// ---- VK_EXT_validation_cache extension commands
	pTable->CreateValidationCacheEXT  = (PFN_vkCreateValidationCacheEXT)loadFn( device, "vkCreateValidationCacheEXT" );
	pTable->DestroyValidationCacheEXT = (PFN_vkDestroyValidationCacheEXT)loadFn( device, "vkDestroyValidationCacheEXT" );
	pTable->MergeValidationCachesEXT  = (PFN_vkMergeValidationCachesEXT)loadFn( device, "vkMergeValidationCachesEXT" );
	pTable->GetValidationCacheDataEXT = (PFN_vkGetValidationCacheDataEXT)loadFn( device, "vkGetValidationCacheDataEXT" );

	// ---- VK_NV_shading_rate_image extension commands
	pTable->CmdBindShadingRateImageNV		   = (PFN_vkCmdBindShadingRateImageNV)loadFn( device, "vkCmdBindShadingRateImageNV" );
	pTable->CmdSetViewportShadingRatePaletteNV = (PFN_vkCmdSetViewportShadingRatePaletteNV)loadFn( device, "vkCmdSetViewportShadingRatePaletteNV" );
	pTable->CmdSetCoarseSampleOrderNV		   = (PFN_vkCmdSetCoarseSampleOrderNV)loadFn( device, "vkCmdSetCoarseSampleOrderNV" );

	// ---- VK_NV_ray_tracing extension commands
	pTable->CreateAccelerationStructureNV				 = (PFN_vkCreateAccelerationStructureNV)loadFn( device, "vkCreateAccelerationStructureNV" );
	pTable->DestroyAccelerationStructureNV				 = (PFN_vkDestroyAccelerationStructureNV)loadFn( device, "vkDestroyAccelerationStructureNV" );
	pTable->GetAccelerationStructureMemoryRequirementsNV = (PFN_vkGetAccelerationStructureMemoryRequirementsNV)loadFn( device, "vkGetAccelerationStructureMemoryRequirementsNV" );
	pTable->BindAccelerationStructureMemoryNV			 = (PFN_vkBindAccelerationStructureMemoryNV)loadFn( device, "vkBindAccelerationStructureMemoryNV" );
	pTable->CmdBuildAccelerationStructureNV				 = (PFN_vkCmdBuildAccelerationStructureNV)loadFn( device, "vkCmdBuildAccelerationStructureNV" );
	pTable->CmdCopyAccelerationStructureNV				 = (PFN_vkCmdCopyAccelerationStructureNV)loadFn( device, "vkCmdCopyAccelerationStructureNV" );
	pTable->CmdTraceRaysNV								 = (PFN_vkCmdTraceRaysNV)loadFn( device, "vkCmdTraceRaysNV" );
	pTable->CreateRayTracingPipelinesNV					 = (PFN_vkCreateRayTracingPipelinesNV)loadFn( device, "vkCreateRayTracingPipelinesNV" );
	pTable->GetRayTracingShaderGroupHandlesKHR			 = (PFN_vkGetRayTracingShaderGroupHandlesKHR)loadFn( device, "vkGetRayTracingShaderGroupHandlesKHR" );
	pTable->GetRayTracingShaderGroupHandlesNV			 = (PFN_vkGetRayTracingShaderGroupHandlesNV)loadFn( device, "vkGetRayTracingShaderGroupHandlesNV" );
	pTable->GetAccelerationStructureHandleNV			 = (PFN_vkGetAccelerationStructureHandleNV)loadFn( device, "vkGetAccelerationStructureHandleNV" );
	pTable->CmdWriteAccelerationStructuresPropertiesNV	 = (PFN_vkCmdWriteAccelerationStructuresPropertiesNV)loadFn( device, "vkCmdWriteAccelerationStructuresPropertiesNV" );
	pTable->CompileDeferredNV							 = (PFN_vkCompileDeferredNV)loadFn( device, "vkCompileDeferredNV" );

	// ---- VK_EXT_external_memory_host extension commands
	pTable->GetMemoryHostPointerPropertiesEXT = (PFN_vkGetMemoryHostPointerPropertiesEXT)loadFn( device, "vkGetMemoryHostPointerPropertiesEXT" );

	// ---- VK_AMD_buffer_marker extension commands
	pTable->CmdWriteBufferMarkerAMD = (PFN_vkCmdWriteBufferMarkerAMD)loadFn( device, "vkCmdWriteBufferMarkerAMD" );

	// ---- VK_EXT_calibrated_timestamps extension commands
	pTable->GetCalibratedTimestampsEXT = (PFN_vkGetCalibratedTimestampsEXT)loadFn( device, "vkGetCalibratedTimestampsEXT" );

	// ---- VK_NV_mesh_shader extension commands
	pTable->CmdDrawMeshTasksNV				= (PFN_vkCmdDrawMeshTasksNV)loadFn( device, "vkCmdDrawMeshTasksNV" );
	pTable->CmdDrawMeshTasksIndirectNV		= (PFN_vkCmdDrawMeshTasksIndirectNV)loadFn( device, "vkCmdDrawMeshTasksIndirectNV" );
	pTable->CmdDrawMeshTasksIndirectCountNV = (PFN_vkCmdDrawMeshTasksIndirectCountNV)loadFn( device, "vkCmdDrawMeshTasksIndirectCountNV" );

	// ---- VK_NV_scissor_exclusive extension commands
	pTable->CmdSetExclusiveScissorNV = (PFN_vkCmdSetExclusiveScissorNV)loadFn( device, "vkCmdSetExclusiveScissorNV" );

	// ---- VK_NV_device_diagnostic_checkpoints extension commands
	pTable->CmdSetCheckpointNV		 = (PFN_vkCmdSetCheckpointNV)loadFn( device, "vkCmdSetCheckpointNV" );
	pTable->GetQueueCheckpointDataNV = (PFN_vkGetQueueCheckpointDataNV)loadFn( device, "vkGetQueueCheckpointDataNV" );

	// ---- VK_INTEL_performance_query extension commands
	pTable->InitializePerformanceApiINTEL		  = (PFN_vkInitializePerformanceApiINTEL)loadFn( device, "vkInitializePerformanceApiINTEL" );
	pTable->UninitializePerformanceApiINTEL		  = (PFN_vkUninitializePerformanceApiINTEL)loadFn( device, "vkUninitializePerformanceApiINTEL" );
	pTable->CmdSetPerformanceMarkerINTEL		  = (PFN_vkCmdSetPerformanceMarkerINTEL)loadFn( device, "vkCmdSetPerformanceMarkerINTEL" );
	pTable->CmdSetPerformanceStreamMarkerINTEL	  = (PFN_vkCmdSetPerformanceStreamMarkerINTEL)loadFn( device, "vkCmdSetPerformanceStreamMarkerINTEL" );
	pTable->CmdSetPerformanceOverrideINTEL		  = (PFN_vkCmdSetPerformanceOverrideINTEL)loadFn( device, "vkCmdSetPerformanceOverrideINTEL" );
	pTable->AcquirePerformanceConfigurationINTEL  = (PFN_vkAcquirePerformanceConfigurationINTEL)loadFn( device, "vkAcquirePerformanceConfigurationINTEL" );
	pTable->ReleasePerformanceConfigurationINTEL  = (PFN_vkReleasePerformanceConfigurationINTEL)loadFn( device, "vkReleasePerformanceConfigurationINTEL" );
	pTable->QueueSetPerformanceConfigurationINTEL = (PFN_vkQueueSetPerformanceConfigurationINTEL)loadFn( device, "vkQueueSetPerformanceConfigurationINTEL" );
	pTable->GetPerformanceParameterINTEL		  = (PFN_vkGetPerformanceParameterINTEL)loadFn( device, "vkGetPerformanceParameterINTEL" );

	// ---- VK_AMD_display_native_hdr extension commands
	pTable->SetLocalDimmingAMD = (PFN_vkSetLocalDimmingAMD)loadFn( device, "vkSetLocalDimmingAMD" );

	// ---- VK_EXT_buffer_device_address extension commands
	pTable->GetBufferDeviceAddressEXT = (PFN_vkGetBufferDeviceAddressEXT)loadFn( device, "vkGetBufferDeviceAddressEXT" );

	// ---- VK_EXT_full_screen_exclusive extension commands
#ifdef VK_USE_PLATFORM_WIN32_KHR
	pTable->AcquireFullScreenExclusiveModeEXT = (PFN_vkAcquireFullScreenExclusiveModeEXT)loadFn( device, "vkAcquireFullScreenExclusiveModeEXT" );
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
	pTable->ReleaseFullScreenExclusiveModeEXT = (PFN_vkReleaseFullScreenExclusiveModeEXT)loadFn( device, "vkReleaseFullScreenExclusiveModeEXT" );
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
	pTable->GetDeviceGroupSurfacePresentModes2EXT = (PFN_vkGetDeviceGroupSurfacePresentModes2EXT)loadFn( device, "vkGetDeviceGroupSurfacePresentModes2EXT" );
#endif // VK_USE_PLATFORM_WIN32_KHR

	// ---- VK_EXT_line_rasterization extension commands
	pTable->CmdSetLineStippleEXT = (PFN_vkCmdSetLineStippleEXT)loadFn( device, "vkCmdSetLineStippleEXT" );

	// ---- VK_EXT_host_query_reset extension commands
	pTable->ResetQueryPoolEXT = (PFN_vkResetQueryPoolEXT)loadFn( device, "vkResetQueryPoolEXT" );

	// ---- VK_EXT_extended_dynamic_state extension commands
	pTable->CmdSetCullModeEXT			   = (PFN_vkCmdSetCullModeEXT)loadFn( device, "vkCmdSetCullModeEXT" );
	pTable->CmdSetFrontFaceEXT			   = (PFN_vkCmdSetFrontFaceEXT)loadFn( device, "vkCmdSetFrontFaceEXT" );
	pTable->CmdSetPrimitiveTopologyEXT	   = (PFN_vkCmdSetPrimitiveTopologyEXT)loadFn( device, "vkCmdSetPrimitiveTopologyEXT" );
	pTable->CmdSetViewportWithCountEXT	   = (PFN_vkCmdSetViewportWithCountEXT)loadFn( device, "vkCmdSetViewportWithCountEXT" );
	pTable->CmdSetScissorWithCountEXT	   = (PFN_vkCmdSetScissorWithCountEXT)loadFn( device, "vkCmdSetScissorWithCountEXT" );
	pTable->CmdBindVertexBuffers2EXT	   = (PFN_vkCmdBindVertexBuffers2EXT)loadFn( device, "vkCmdBindVertexBuffers2EXT" );
	pTable->CmdSetDepthTestEnableEXT	   = (PFN_vkCmdSetDepthTestEnableEXT)loadFn( device, "vkCmdSetDepthTestEnableEXT" );
	pTable->CmdSetDepthWriteEnableEXT	   = (PFN_vkCmdSetDepthWriteEnableEXT)loadFn( device, "vkCmdSetDepthWriteEnableEXT" );
	pTable->CmdSetDepthCompareOpEXT		   = (PFN_vkCmdSetDepthCompareOpEXT)loadFn( device, "vkCmdSetDepthCompareOpEXT" );
	pTable->CmdSetDepthBoundsTestEnableEXT = (PFN_vkCmdSetDepthBoundsTestEnableEXT)loadFn( device, "vkCmdSetDepthBoundsTestEnableEXT" );
	pTable->CmdSetStencilTestEnableEXT	   = (PFN_vkCmdSetStencilTestEnableEXT)loadFn( device, "vkCmdSetStencilTestEnableEXT" );
	pTable->CmdSetStencilOpEXT			   = (PFN_vkCmdSetStencilOpEXT)loadFn( device, "vkCmdSetStencilOpEXT" );

	// ---- VK_NV_device_generated_commands extension commands
	pTable->GetGeneratedCommandsMemoryRequirementsNV = (PFN_vkGetGeneratedCommandsMemoryRequirementsNV)loadFn( device, "vkGetGeneratedCommandsMemoryRequirementsNV" );
	pTable->CmdPreprocessGeneratedCommandsNV		 = (PFN_vkCmdPreprocessGeneratedCommandsNV)loadFn( device, "vkCmdPreprocessGeneratedCommandsNV" );
	pTable->CmdExecuteGeneratedCommandsNV			 = (PFN_vkCmdExecuteGeneratedCommandsNV)loadFn( device, "vkCmdExecuteGeneratedCommandsNV" );
	pTable->CmdBindPipelineShaderGroupNV			 = (PFN_vkCmdBindPipelineShaderGroupNV)loadFn( device, "vkCmdBindPipelineShaderGroupNV" );
	pTable->CreateIndirectCommandsLayoutNV			 = (PFN_vkCreateIndirectCommandsLayoutNV)loadFn( device, "vkCreateIndirectCommandsLayoutNV" );
	pTable->DestroyIndirectCommandsLayoutNV			 = (PFN_vkDestroyIndirectCommandsLayoutNV)loadFn( device, "vkDestroyIndirectCommandsLayoutNV" );

	// ---- VK_EXT_private_data extension commands
	pTable->CreatePrivateDataSlotEXT  = (PFN_vkCreatePrivateDataSlotEXT)loadFn( device, "vkCreatePrivateDataSlotEXT" );
	pTable->DestroyPrivateDataSlotEXT = (PFN_vkDestroyPrivateDataSlotEXT)loadFn( device, "vkDestroyPrivateDataSlotEXT" );
	pTable->SetPrivateDataEXT		  = (PFN_vkSetPrivateDataEXT)loadFn( device, "vkSetPrivateDataEXT" );
	pTable->GetPrivateDataEXT		  = (PFN_vkGetPrivateDataEXT)loadFn( device, "vkGetPrivateDataEXT" );

	// ---- VK_NV_fragment_shading_rate_enums extension commands
	pTable->CmdSetFragmentShadingRateEnumNV = (PFN_vkCmdSetFragmentShadingRateEnumNV)loadFn( device, "vkCmdSetFragmentShadingRateEnumNV" );

	// ---- VK_EXT_vertex_input_dynamic_state extension commands
	pTable->CmdSetVertexInputEXT = (PFN_vkCmdSetVertexInputEXT)loadFn( device, "vkCmdSetVertexInputEXT" );

	// ---- VK_FUCHSIA_external_memory extension commands
#ifdef VK_USE_PLATFORM_FUCHSIA
	pTable->GetMemoryZirconHandleFUCHSIA = (PFN_vkGetMemoryZirconHandleFUCHSIA)loadFn( device, "vkGetMemoryZirconHandleFUCHSIA" );
#endif // VK_USE_PLATFORM_FUCHSIA
#ifdef VK_USE_PLATFORM_FUCHSIA
	pTable->GetMemoryZirconHandlePropertiesFUCHSIA = (PFN_vkGetMemoryZirconHandlePropertiesFUCHSIA)loadFn( device, "vkGetMemoryZirconHandlePropertiesFUCHSIA" );
#endif // VK_USE_PLATFORM_FUCHSIA

	// ---- VK_FUCHSIA_external_semaphore extension commands
#ifdef VK_USE_PLATFORM_FUCHSIA
	pTable->ImportSemaphoreZirconHandleFUCHSIA = (PFN_vkImportSemaphoreZirconHandleFUCHSIA)loadFn( device, "vkImportSemaphoreZirconHandleFUCHSIA" );
#endif // VK_USE_PLATFORM_FUCHSIA
#ifdef VK_USE_PLATFORM_FUCHSIA
	pTable->GetSemaphoreZirconHandleFUCHSIA = (PFN_vkGetSemaphoreZirconHandleFUCHSIA)loadFn( device, "vkGetSemaphoreZirconHandleFUCHSIA" );
#endif // VK_USE_PLATFORM_FUCHSIA

	// ---- VK_FUCHSIA_buffer_collection extension commands
#ifdef VK_USE_PLATFORM_FUCHSIA
	pTable->CreateBufferCollectionFUCHSIA = (PFN_vkCreateBufferCollectionFUCHSIA)loadFn( device, "vkCreateBufferCollectionFUCHSIA" );
#endif // VK_USE_PLATFORM_FUCHSIA
#ifdef VK_USE_PLATFORM_FUCHSIA
	pTable->SetBufferCollectionImageConstraintsFUCHSIA = (PFN_vkSetBufferCollectionImageConstraintsFUCHSIA)loadFn( device, "vkSetBufferCollectionImageConstraintsFUCHSIA" );
#endif // VK_USE_PLATFORM_FUCHSIA
#ifdef VK_USE_PLATFORM_FUCHSIA
	pTable->SetBufferCollectionBufferConstraintsFUCHSIA = (PFN_vkSetBufferCollectionBufferConstraintsFUCHSIA)loadFn( device, "vkSetBufferCollectionBufferConstraintsFUCHSIA" );
#endif // VK_USE_PLATFORM_FUCHSIA
#ifdef VK_USE_PLATFORM_FUCHSIA
	pTable->DestroyBufferCollectionFUCHSIA = (PFN_vkDestroyBufferCollectionFUCHSIA)loadFn( device, "vkDestroyBufferCollectionFUCHSIA" );
#endif // VK_USE_PLATFORM_FUCHSIA
#ifdef VK_USE_PLATFORM_FUCHSIA
	pTable->GetBufferCollectionPropertiesFUCHSIA = (PFN_vkGetBufferCollectionPropertiesFUCHSIA)loadFn( device, "vkGetBufferCollectionPropertiesFUCHSIA" );
#endif // VK_USE_PLATFORM_FUCHSIA

	// ---- VK_HUAWEI_subpass_shading extension commands
	pTable->GetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI = (PFN_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI)loadFn( device, "vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI" );
	pTable->CmdSubpassShadingHUAWEI						  = (PFN_vkCmdSubpassShadingHUAWEI)loadFn( device, "vkCmdSubpassShadingHUAWEI" );

	// ---- VK_HUAWEI_invocation_mask extension commands
	pTable->CmdBindInvocationMaskHUAWEI = (PFN_vkCmdBindInvocationMaskHUAWEI)loadFn( device, "vkCmdBindInvocationMaskHUAWEI" );

	// ---- VK_NV_external_memory_rdma extension commands
	pTable->GetMemoryRemoteAddressNV = (PFN_vkGetMemoryRemoteAddressNV)loadFn( device, "vkGetMemoryRemoteAddressNV" );

	// ---- VK_EXT_extended_dynamic_state2 extension commands
	pTable->CmdSetPatchControlPointsEXT		 = (PFN_vkCmdSetPatchControlPointsEXT)loadFn( device, "vkCmdSetPatchControlPointsEXT" );
	pTable->CmdSetRasterizerDiscardEnableEXT = (PFN_vkCmdSetRasterizerDiscardEnableEXT)loadFn( device, "vkCmdSetRasterizerDiscardEnableEXT" );
	pTable->CmdSetDepthBiasEnableEXT		 = (PFN_vkCmdSetDepthBiasEnableEXT)loadFn( device, "vkCmdSetDepthBiasEnableEXT" );
	pTable->CmdSetLogicOpEXT				 = (PFN_vkCmdSetLogicOpEXT)loadFn( device, "vkCmdSetLogicOpEXT" );
	pTable->CmdSetPrimitiveRestartEnableEXT	 = (PFN_vkCmdSetPrimitiveRestartEnableEXT)loadFn( device, "vkCmdSetPrimitiveRestartEnableEXT" );

	// ---- VK_EXT_color_write_enable extension commands
	pTable->CmdSetColorWriteEnableEXT = (PFN_vkCmdSetColorWriteEnableEXT)loadFn( device, "vkCmdSetColorWriteEnableEXT" );

	// ---- VK_EXT_multi_draw extension commands
	pTable->CmdDrawMultiEXT		   = (PFN_vkCmdDrawMultiEXT)loadFn( device, "vkCmdDrawMultiEXT" );
	pTable->CmdDrawMultiIndexedEXT = (PFN_vkCmdDrawMultiIndexedEXT)loadFn( device, "vkCmdDrawMultiIndexedEXT" );

	// ---- VK_EXT_pageable_device_local_memory extension commands
	pTable->SetDeviceMemoryPriorityEXT = (PFN_vkSetDeviceMemoryPriorityEXT)loadFn( device, "vkSetDeviceMemoryPriorityEXT" );

	// ---- VK_KHR_acceleration_structure extension commands
	pTable->CreateAccelerationStructureKHR				   = (PFN_vkCreateAccelerationStructureKHR)loadFn( device, "vkCreateAccelerationStructureKHR" );
	pTable->DestroyAccelerationStructureKHR				   = (PFN_vkDestroyAccelerationStructureKHR)loadFn( device, "vkDestroyAccelerationStructureKHR" );
	pTable->CmdBuildAccelerationStructuresKHR			   = (PFN_vkCmdBuildAccelerationStructuresKHR)loadFn( device, "vkCmdBuildAccelerationStructuresKHR" );
	pTable->CmdBuildAccelerationStructuresIndirectKHR	   = (PFN_vkCmdBuildAccelerationStructuresIndirectKHR)loadFn( device, "vkCmdBuildAccelerationStructuresIndirectKHR" );
	pTable->BuildAccelerationStructuresKHR				   = (PFN_vkBuildAccelerationStructuresKHR)loadFn( device, "vkBuildAccelerationStructuresKHR" );
	pTable->CopyAccelerationStructureKHR				   = (PFN_vkCopyAccelerationStructureKHR)loadFn( device, "vkCopyAccelerationStructureKHR" );
	pTable->CopyAccelerationStructureToMemoryKHR		   = (PFN_vkCopyAccelerationStructureToMemoryKHR)loadFn( device, "vkCopyAccelerationStructureToMemoryKHR" );
	pTable->CopyMemoryToAccelerationStructureKHR		   = (PFN_vkCopyMemoryToAccelerationStructureKHR)loadFn( device, "vkCopyMemoryToAccelerationStructureKHR" );
	pTable->WriteAccelerationStructuresPropertiesKHR	   = (PFN_vkWriteAccelerationStructuresPropertiesKHR)loadFn( device, "vkWriteAccelerationStructuresPropertiesKHR" );
	pTable->CmdCopyAccelerationStructureKHR				   = (PFN_vkCmdCopyAccelerationStructureKHR)loadFn( device, "vkCmdCopyAccelerationStructureKHR" );
	pTable->CmdCopyAccelerationStructureToMemoryKHR		   = (PFN_vkCmdCopyAccelerationStructureToMemoryKHR)loadFn( device, "vkCmdCopyAccelerationStructureToMemoryKHR" );
	pTable->CmdCopyMemoryToAccelerationStructureKHR		   = (PFN_vkCmdCopyMemoryToAccelerationStructureKHR)loadFn( device, "vkCmdCopyMemoryToAccelerationStructureKHR" );
	pTable->GetAccelerationStructureDeviceAddressKHR	   = (PFN_vkGetAccelerationStructureDeviceAddressKHR)loadFn( device, "vkGetAccelerationStructureDeviceAddressKHR" );
	pTable->CmdWriteAccelerationStructuresPropertiesKHR	   = (PFN_vkCmdWriteAccelerationStructuresPropertiesKHR)loadFn( device, "vkCmdWriteAccelerationStructuresPropertiesKHR" );
	pTable->GetDeviceAccelerationStructureCompatibilityKHR = (PFN_vkGetDeviceAccelerationStructureCompatibilityKHR)loadFn( device, "vkGetDeviceAccelerationStructureCompatibilityKHR" );
	pTable->GetAccelerationStructureBuildSizesKHR		   = (PFN_vkGetAccelerationStructureBuildSizesKHR)loadFn( device, "vkGetAccelerationStructureBuildSizesKHR" );

	// ---- VK_KHR_ray_tracing_pipeline extension commands
	pTable->CmdTraceRaysKHR									= (PFN_vkCmdTraceRaysKHR)loadFn( device, "vkCmdTraceRaysKHR" );
	pTable->CreateRayTracingPipelinesKHR					= (PFN_vkCreateRayTracingPipelinesKHR)loadFn( device, "vkCreateRayTracingPipelinesKHR" );
	pTable->GetRayTracingCaptureReplayShaderGroupHandlesKHR = (PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR)loadFn( device, "vkGetRayTracingCaptureReplayShaderGroupHandlesKHR" );
	pTable->CmdTraceRaysIndirectKHR							= (PFN_vkCmdTraceRaysIndirectKHR)loadFn( device, "vkCmdTraceRaysIndirectKHR" );
	pTable->GetRayTracingShaderGroupStackSizeKHR			= (PFN_vkGetRayTracingShaderGroupStackSizeKHR)loadFn( device, "vkGetRayTracingShaderGroupStackSizeKHR" );
	pTable->CmdSetRayTracingPipelineStackSizeKHR			= (PFN_vkCmdSetRayTracingPipelineStackSizeKHR)loadFn( device, "vkCmdSetRayTracingPipelineStackSizeKHR" );
};

} // namespace cinder::vk
