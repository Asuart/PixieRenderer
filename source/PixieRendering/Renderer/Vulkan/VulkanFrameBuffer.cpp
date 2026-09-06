#include "VulkanFrameBuffer.h"

#include "VulkanDevice.h"
#include "VulkanRenderPass.h"

namespace PixieRenderer {

VulkanFrameBuffer::VulkanFrameBuffer(
    VulkanDevice& parentDevice,
    VkExtent2D extent,
    VkFormat colorFormat,
    VulkanRenderPass* renderPass
)
    : m_device(parentDevice),
      m_extent(extent),
      m_colorFormat(colorFormat),
      m_renderPass(renderPass) {
	m_depthFormat = m_device.FindDepthFormat();
	if (m_depthFormat == VK_FORMAT_UNDEFINED) {
		throw std::runtime_error("No suitable depth format found!");
	}

	ResetViewport();
	ResetScissor();

	CreateImages();

	m_sampler = std::make_unique<VulkanSampler>(m_device);

	std::array<VkImageView, 2> fbAttachments = { m_colorImageView, m_depthImageView };
	VkFramebufferCreateInfo fbInfo{};
	fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbInfo.renderPass = m_renderPass->GetRenderPass();
	fbInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
	fbInfo.pAttachments = fbAttachments.data();
	fbInfo.width = m_extent.width;
	fbInfo.height = m_extent.height;
	fbInfo.layers = 1;

	if (vkCreateFramebuffer(m_device.GetDevice(), &fbInfo, nullptr, &m_framebuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create framebuffer");
	}
}

VulkanFrameBuffer::~VulkanFrameBuffer() {
	VkDevice device = m_device.GetDevice();

	if (m_framebuffer != VK_NULL_HANDLE) {
		vkDestroyFramebuffer(device, m_framebuffer, nullptr);
	}

	FreeImages();

	m_sampler.reset();
}

VkFramebuffer VulkanFrameBuffer::GetFrameBuffer() const {
	return m_framebuffer;
}

VkRenderPass VulkanFrameBuffer::GetRenderPass() const {
	if (m_renderPass == nullptr) {
		return VK_NULL_HANDLE;
	}
	return m_renderPass->GetRenderPass();
}

VkSampler VulkanFrameBuffer::GetSampler() const {
	if (m_sampler == nullptr) {
		return VK_NULL_HANDLE;
	}
	return m_sampler->GetSampler();
}

VkImageView VulkanFrameBuffer::GetColorImageView() const {
	return m_colorImageView;
}

VkExtent2D VulkanFrameBuffer::GetExtent() const {
	return m_extent;
}

VkViewport VulkanFrameBuffer::GetViewport() const {
	return m_viewport;
}

void VulkanFrameBuffer::SetViewport(VkViewport viewport) {
	m_viewport = viewport;
}

void VulkanFrameBuffer::ResetViewport() {
	m_viewport.x = 0.0f;
	m_viewport.y = 0.0f;
	m_viewport.width = static_cast<float>(m_extent.width);
	m_viewport.height = static_cast<float>(m_extent.height);
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;
}

VkRect2D VulkanFrameBuffer::GetScissor() const {
	return m_scissor;
}

VulkanRenderPass* VulkanFrameBuffer::GetRenderPassObject() {
	return m_renderPass;
}

void VulkanFrameBuffer::SetScissor(VkRect2D scissor) {
	m_scissor = scissor;
}

void VulkanFrameBuffer::ResetScissor() {
	m_scissor.offset = { 0, 0 };
	m_scissor.extent = m_extent;
}

void VulkanFrameBuffer::Resize(VkExtent2D extent) {
	if (m_extent.width == extent.width && m_extent.height == extent.height) {
		return;
	}

	VkDevice device = m_device.GetDevice();

	if (m_framebuffer != VK_NULL_HANDLE) {
		vkDestroyFramebuffer(device, m_framebuffer, nullptr);
		m_framebuffer = VK_NULL_HANDLE;
	}

	FreeImages();

	m_extent = extent;

	ResetViewport();
	ResetScissor();

	CreateImages();

	std::array<VkImageView, 2> attachments = { m_colorImageView, m_depthImageView };
	VkFramebufferCreateInfo fbInfo{};
	fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbInfo.renderPass = m_renderPass->GetRenderPass();
	fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	fbInfo.pAttachments = attachments.data();
	fbInfo.width = m_extent.width;
	fbInfo.height = m_extent.height;
	fbInfo.layers = 1;

	if (vkCreateFramebuffer(device, &fbInfo, nullptr, &m_framebuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to recreate framebuffer during resize!");
	}
}

void VulkanFrameBuffer::Transition(
    VkCommandBuffer cmdBuf,
    VkImageLayout newLayout,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage,
    VkImageAspectFlags aspectMask
) {
	m_device.TransitionImage(
		cmdBuf,
	    m_colorImage,
	    m_imageLayout,
	    newLayout,
	    srcAccessMask,
	    dstAccessMask,
	    srcStage,
	    dstStage,
	    aspectMask,
	    1,
		1
	);
	m_imageLayout = newLayout;
}

void VulkanFrameBuffer::CreateImages() {
	m_device.CreateImage(
	    m_extent.width,
	    m_extent.height,
	    1,
	    VK_SAMPLE_COUNT_1_BIT,
	    m_colorFormat,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	    m_colorImage,
	    m_colorImageMemory
	);
	m_device.CreateImageView(
	    m_colorImage,
	    m_colorFormat,
	    VK_IMAGE_ASPECT_COLOR_BIT,
	    1,
	    m_colorImageView
	);

	m_device.CreateImage(
	    m_extent.width,
	    m_extent.height,
	    1,
	    VK_SAMPLE_COUNT_1_BIT,
	    m_depthFormat,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	    m_depthImage,
	    m_depthImageMemory
	);
	m_device.CreateImageView(
	    m_depthImage,
	    m_depthFormat,
	    VK_IMAGE_ASPECT_DEPTH_BIT,
	    1,
	    m_depthImageView
	);
}

void VulkanFrameBuffer::FreeImages() {
	VkDevice device = m_device.GetDevice();
	if (m_colorImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(device, m_colorImageView, nullptr);
		m_colorImageView = VK_NULL_HANDLE;
	}
	if (m_colorImage != VK_NULL_HANDLE) {
		vkDestroyImage(device, m_colorImage, nullptr);
		m_colorImage = VK_NULL_HANDLE;
	}
	if (m_colorImageMemory != VK_NULL_HANDLE) {
		vkFreeMemory(device, m_colorImageMemory, nullptr);
		m_colorImageMemory = VK_NULL_HANDLE;
	}

	if (m_depthImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(device, m_depthImageView, nullptr);
		m_depthImageView = VK_NULL_HANDLE;
	}
	if (m_depthImage != VK_NULL_HANDLE) {
		vkDestroyImage(device, m_depthImage, nullptr);
		m_depthImage = VK_NULL_HANDLE;
	}
	if (m_depthImageMemory != VK_NULL_HANDLE) {
		vkFreeMemory(device, m_depthImageMemory, nullptr);
		m_depthImageMemory = VK_NULL_HANDLE;
	}

	m_imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

} // namespace PixieRenderer
