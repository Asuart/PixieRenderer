#pragma once
#include <memory>

#include <vulkan/vulkan.h>

#include "VulkanSampler.h"

namespace PixieRenderer {

class VulkanDevice;
class VulkanRenderPass;

class VulkanFrameBuffer {
  public:
	VulkanFrameBuffer(
	    VulkanDevice& parentDevice,
	    VkExtent2D extent,
	    VkFormat colorFormat,
	    VulkanRenderPass* renderPass
	);
	~VulkanFrameBuffer();

	VkFramebuffer GetFrameBuffer() const;
	VkRenderPass GetRenderPass() const;
	VkSampler GetSampler() const;
	VkImageView GetColorImageView() const;
	VkExtent2D GetExtent() const;
	VulkanRenderPass* GetRenderPassObject();

	VkImage GetColorImage() const {
		return m_colorImage;
	}
	VkFormat GetColorFormat() const {
		return m_colorFormat;
	}

	VkViewport GetViewport() const;
	void SetViewport(VkViewport viewport);
	void ResetViewport();

	VkRect2D GetScissor() const;
	void SetScissor(VkRect2D scissor);
	void ResetScissor();

	void Resize(VkExtent2D extent);

	void Transition(
	    VkCommandBuffer cmdBuf,
	    VkImageLayout newLayout,
	    VkAccessFlags srcAccessMask,
	    VkAccessFlags dstAccessMask,
	    VkPipelineStageFlags srcStage,
	    VkPipelineStageFlags dstStage,
	    VkImageAspectFlags aspectMask
	);

  private:
	VulkanDevice& m_device;
	VkExtent2D m_extent = { 0, 0 };
	VkImage m_colorImage = VK_NULL_HANDLE;
	VkDeviceMemory m_colorImageMemory = VK_NULL_HANDLE;
	VkImageView m_colorImageView = VK_NULL_HANDLE;
	VkImage m_depthImage = VK_NULL_HANDLE;
	VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
	VkImageView m_depthImageView = VK_NULL_HANDLE;
	std::unique_ptr<VulkanSampler> m_sampler = nullptr;
	VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
	VkFormat m_colorFormat = VK_FORMAT_UNDEFINED;
	VkFormat m_depthFormat = VK_FORMAT_UNDEFINED;
	VkImageLayout m_imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkViewport m_viewport = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	VkRect2D m_scissor = { { 0, 0 }, { 0, 0 } };
	VulkanRenderPass* m_renderPass = nullptr;

	void CreateImages();
	void FreeImages();
};

} // namespace PixieRenderer
