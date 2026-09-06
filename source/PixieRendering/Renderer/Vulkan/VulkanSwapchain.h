#pragma once
#include <vector>

#include <vulkan/vulkan.h>

namespace PixieRenderer {

class VulkanDevice;

class VulkanSwapchain {
  public:
	VulkanSwapchain(VulkanDevice& parentDevice, VkExtent2D extent, VkRenderPass renderPass);
	~VulkanSwapchain();

	VkExtent2D GetExtent() const;
	VkRenderPass GetRenderPass() const;
	VkSwapchainKHR GetSwapChain() const;
	VkFormat GetFormat() const;
	VkFramebuffer GetFrameBuffer(uint32_t index) const;
	uint64_t GetImageCount() const;

	VkImage GetImage(uint32_t index) const {
		return m_images[index];
	}
	VkImageLayout GetImageLayout(uint32_t index) const {
		return m_imageLayouts[index];
	}
	void SetImageLayout(uint32_t index, VkImageLayout layout) {
		m_imageLayouts[index] = layout;
	}

	void Transition(
		VkCommandBuffer cmdBuf,
	    VkImageLayout newLayout,
	    VkAccessFlags srcAccessMask,
	    VkAccessFlags dstAccessMask,
	    VkPipelineStageFlags srcStage,
	    VkPipelineStageFlags dstStage,
	    VkImageAspectFlags aspectMask,
	    uint32_t frameIndex
	);

  private:
	VulkanDevice& m_device;
	VkExtent2D m_extent = { 0, 0 };
	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	VkFormat m_format = VK_FORMAT_UNDEFINED;
	std::vector<VkImage> m_images = {};
	std::vector<VkImageView> m_imageViews = {};
	std::vector<VkFramebuffer> m_framebuffers = {};
	std::vector<VkImageLayout> m_imageLayouts;

	VkFormat m_depthFormat = VK_FORMAT_UNDEFINED;
	VkImage m_depthImage = VK_NULL_HANDLE;
	VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
	VkImageView m_depthImageView = VK_NULL_HANDLE;

  public:
	static VkSurfaceFormatKHR ChooseSurfaceFormat(
	    const std::vector<VkSurfaceFormatKHR>& availableFormats
	);
	static VkPresentModeKHR ChoosePresentMode(
	    const std::vector<VkPresentModeKHR>& availablePresentModes
	);
	static VkExtent2D ChooseExtent(VkExtent2D extent, const VkSurfaceCapabilitiesKHR& capabilities);
};

} // namespace PixieRenderer
