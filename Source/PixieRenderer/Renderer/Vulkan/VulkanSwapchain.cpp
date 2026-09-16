#include "PixieRenderer/pch.h"
#include "VulkanSwapchain.h"

#include "VulkanDevice.h"
#include "VulkanPhysicalDeviceUtils.h"

namespace PixieRenderer {

VulkanSwapchain::VulkanSwapchain(
    VulkanDevice& parentDevice,
    VkExtent2D extent,
    VkRenderPass renderPass
)
    : m_device(parentDevice), m_renderPass(renderPass) {
	VkDevice device = m_device.GetDevice();

	SwapChainSupportDetails swapChainSupport = m_device.QuerySwapChainSupport();

	VkSurfaceFormatKHR surfaceFormat = VulkanSwapchain::ChooseSurfaceFormat(swapChainSupport.formats
	);
	m_format = surfaceFormat.format;

	VkPresentModeKHR presentMode = VulkanSwapchain::ChoosePresentMode(swapChainSupport.presentModes
	);

	m_extent = VulkanSwapchain::ChooseExtent(extent, swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 &&
	    imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_device.GetSurface();

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = m_format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = m_extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = m_device.GetQueueFamilyIndices();
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	if (vkGetSwapchainImagesKHR(device, m_swapchain, &imageCount, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("failed to retrieve swapchain image count!");
	}

	m_images.resize(imageCount);
	if (vkGetSwapchainImagesKHR(device, m_swapchain, &imageCount, m_images.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to retrieve swapchain images!");
	}

	m_imageViews.resize(m_images.size());
	for (uint32_t i = 0; i < m_images.size(); i++) {
		m_device
		    .CreateImageView(m_images[i], m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1, m_imageViews[i]);
	}

	m_depthFormat = m_device.FindDepthFormat();
	if (m_depthFormat == VK_FORMAT_UNDEFINED) {
		throw std::runtime_error("No suitable depth format found!");
	}

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
	m_device.TransitionImageSingleTime(
	    m_depthImage,
	    VK_IMAGE_LAYOUT_UNDEFINED,
	    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	    0,
	    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
	    VulkanPhysicalDeviceUtils::GetAspectMask(m_depthFormat),
	    1,
	    1
	);

	m_framebuffers.resize(m_imageViews.size());
	for (size_t i = 0; i < m_imageViews.size(); i++) {
		std::array<VkImageView, 2> attachments = { m_imageViews[i], m_depthImageView };

		VkFramebufferCreateInfo fbInfo{};
		fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbInfo.renderPass = renderPass;
		fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		fbInfo.pAttachments = attachments.data();
		fbInfo.width = m_extent.width;
		fbInfo.height = m_extent.height;
		fbInfo.layers = 1;

		if (vkCreateFramebuffer(device, &fbInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}

	m_imageLayouts.resize(m_images.size(), VK_IMAGE_LAYOUT_UNDEFINED);
}

VulkanSwapchain::~VulkanSwapchain() {
	VkDevice device = m_device.GetDevice();

	for (VkFramebuffer framebuffer : m_framebuffers) {
		if (framebuffer != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}
	}

	for (VkImageView imageView : m_imageViews) {
		if (imageView != VK_NULL_HANDLE) {
			vkDestroyImageView(device, imageView, nullptr);
		}
	}

	if (m_depthImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(device, m_depthImageView, nullptr);
	}
	if (m_depthImage != VK_NULL_HANDLE) {
		vkDestroyImage(device, m_depthImage, nullptr);
	}
	if (m_depthImageMemory != VK_NULL_HANDLE) {
		vkFreeMemory(device, m_depthImageMemory, nullptr);
	}

	if (m_swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(device, m_swapchain, nullptr);
	}
}

VkExtent2D VulkanSwapchain::GetExtent() const {
	return m_extent;
}

VkRenderPass VulkanSwapchain::GetRenderPass() const {
	return m_renderPass;
}

VkSwapchainKHR VulkanSwapchain::GetSwapChain() const {
	return m_swapchain;
}

VkFormat VulkanSwapchain::GetFormat() const {
	return m_format;
}

VkFramebuffer VulkanSwapchain::GetFrameBuffer(uint32_t index) const {
	if (index >= m_framebuffers.size()) {
		throw std::out_of_range("Invalid framebuffer index");
	}
	return m_framebuffers[index];
}

uint64_t VulkanSwapchain::GetImageCount() const {
	return m_images.size();
}

void VulkanSwapchain::Transition(
    VkCommandBuffer cmdBuf,
    VkImageLayout newLayout,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage,
    VkImageAspectFlags aspectMask,
    uint32_t frameIndex
) {
	m_device.TransitionImage(
	    cmdBuf,
	    m_images[frameIndex],
	    m_imageLayouts[frameIndex],
	    newLayout,
	    srcAccessMask,
	    dstAccessMask,
	    srcStage,
	    dstStage,
	    aspectMask,
	    1,
	    1
	);
	m_imageLayouts[frameIndex] = newLayout;
}

VkSurfaceFormatKHR VulkanSwapchain::ChooseSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats
) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
		    availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& available
) {
	if (available.size() == 0) {
		return VK_PRESENT_MODE_FIFO_KHR;
	}
	for (VkPresentModeKHR mode : available) {
		if (mode == VK_PRESENT_MODE_FIFO_KHR) {
			return mode;
		}
	}
	for (VkPresentModeKHR mode : available) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return mode;
		}
	}
	for (VkPresentModeKHR mode : available) {
		if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			return mode;
		}
	}
	return available[0];
}

VkExtent2D VulkanSwapchain::ChooseExtent(
    VkExtent2D extent,
    const VkSurfaceCapabilitiesKHR& capabilities
) {
	VkExtent2D actualExtent{};
	actualExtent.width = std::
	    clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(
	    extent.height,
	    capabilities.minImageExtent.height,
	    capabilities.maxImageExtent.height
	);
	return actualExtent;
}

} // namespace PixieRenderer
