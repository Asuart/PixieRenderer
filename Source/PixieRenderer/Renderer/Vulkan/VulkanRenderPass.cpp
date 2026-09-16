#include "PixieRenderer/pch.h"
#include "VulkanRenderPass.h"

#include "VulkanDevice.h"

namespace PixieRenderer {

VulkanRenderPass::VulkanRenderPass(
    VulkanDevice& parentDevice,
    VkFormat colorFormat,
    VkImageLayout finalLayout
)
    : m_device(parentDevice), m_colorFormat(colorFormat), m_finalLayout(finalLayout) {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_colorFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = m_finalLayout;

	m_depthFormat = m_device.FindDepthFormat();
	if (m_depthFormat == VK_FORMAT_UNDEFINED) {
		throw std::runtime_error("failed to find depth format!");
	}

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = m_depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
	                          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
	                           VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
	                          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
	                           VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_device.GetDevice(), &renderPassInfo, nullptr, &m_renderPass) !=
	    VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

VulkanRenderPass::~VulkanRenderPass() {
	if (m_renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(m_device.GetDevice(), m_renderPass, nullptr);
	}
}

VkRenderPass VulkanRenderPass::GetRenderPass() const {
	return m_renderPass;
}

void VulkanRenderPass::AddRenderRequest(RenderRequest request) {
	m_renderRequests.push_back(request);
}

void VulkanRenderPass::Begin(
    VkCommandBuffer cmdBuf,
    uint32_t frameIndex,
    VulkanFrameBuffer& frameBuffer
) {
	frameBuffer.Transition(
	    cmdBuf,
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	    VK_ACCESS_MEMORY_READ_BIT,
	    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	    VK_IMAGE_ASPECT_COLOR_BIT
	);

	Begin(
	    cmdBuf,
	    frameIndex,
	    frameBuffer.GetFrameBuffer(),
	    frameBuffer.GetExtent(),
	    frameBuffer.GetViewport(),
	    frameBuffer.GetScissor()
	);
}

void VulkanRenderPass::Begin(
    VkCommandBuffer cmdBuf,
    uint32_t currentFrame,
    VkFramebuffer frameBuffer,
    VkExtent2D extent
) {
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = extent;
	Begin(cmdBuf, currentFrame, frameBuffer, extent, viewport, scissor);
}

void VulkanRenderPass::Begin(
    VkCommandBuffer cmdBuf,
    uint32_t currentFrame,
    VkFramebuffer frameBuffer,
    VkExtent2D extent,
    VkViewport viewport,
    VkRect2D scissor
) {
	m_currentCommandBuffer = cmdBuf;
	m_currentFrame = currentFrame;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = frameBuffer;
	renderPassInfo.renderArea.extent = extent;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

	m_renderRequests.clear();
}

void VulkanRenderPass::Execute(
    std::vector<ResourceEntry<VulkanMesh>>& meshes,
    std::vector<ResourceEntry<VulkanGraphicsProgram>>& graphicsPrograms
) {
	for (const RenderRequest& req : m_renderRequests) {
		if (!req.materialHandle || !req.meshHandle)
			continue;

		auto& mesh = *meshes[req.meshHandle.GetId() & 0xffffffff].resource;
		auto& graphicsProgram = *graphicsPrograms[req.materialHandle.GetId() & 0xffffffff].resource;

		VkPipelineLayout layout = graphicsProgram.GetPipelineLayout();

		vkCmdBindPipeline(
		    m_currentCommandBuffer,
		    VK_PIPELINE_BIND_POINT_GRAPHICS,
		    graphicsProgram.GetOrCreatePipeline(m_renderPass)
		);

		if (req.pushConstantsSize > 0) {
			VkShaderStageFlags stages = graphicsProgram.GetBindingsInfo().pushConstantStages;
			if (stages == 0) {
				stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			}
			vkCmdPushConstants(
			    m_currentCommandBuffer,
			    layout,
			    stages,
			    0,
			    req.pushConstantsSize,
			    req.pushConstants.data()
			);
		}

		VkBuffer vertexBuffer = mesh.GetVertexBuffer().GetBuffer();
		VkBuffer indexBuffer = mesh.GetIndexBuffer().GetBuffer();
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(m_currentCommandBuffer, 0, 1, &vertexBuffer, &offset);
		vkCmdBindIndexBuffer(m_currentCommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(
		    m_currentCommandBuffer,
		    VK_PIPELINE_BIND_POINT_GRAPHICS,
		    layout,
		    0,
		    1,
		    &graphicsProgram.GetDescriptorSets()[m_currentFrame],
		    0,
		    nullptr
		);

		vkCmdDrawIndexed(m_currentCommandBuffer, mesh.GetIndexCount(), 1, 0, 0, 0);
	}
}

void VulkanRenderPass::End() {
	vkCmdEndRenderPass(m_currentCommandBuffer);
}

} // namespace PixieRenderer
