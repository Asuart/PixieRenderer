#include "PixieRendering/pch.h"
#include "VulkanDevice.h"

#include "DebugVulkan.h"
#include "VulkanBuffer.h"
#include "VulkanPhysicalDeviceUtils.h"
#include "VulkanSwapchain.h"

namespace PixieRenderer {

void VulkanDevice::Initialize(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    const std::vector<const char*>& deviceExtensions
) {
	m_physicalDevice = physicalDevice;
	m_surface = surface;
	m_queueFamilyIndices = VulkanPhysicalDeviceUtils::FindQueueFamilies(physicalDevice, surface);
	CreateLogicalDevice(deviceExtensions);
	CreateCommandPool(m_commandPool);
}

void VulkanDevice::Cleanup() {
	vkDestroyCommandPool(m_device, m_commandPool, nullptr);
	vkDestroyDevice(m_device, nullptr);
}

VkPhysicalDevice VulkanDevice::GetPhysicalDevice() const {
	return m_physicalDevice;
}

VkDevice VulkanDevice::GetDevice() const {
	return m_device;
}

VkSurfaceKHR VulkanDevice::GetSurface() const {
	return m_surface;
}

VkFormat VulkanDevice::GetSurfaceFormat() const {
	SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport();
	return VulkanSwapchain::ChooseSurfaceFormat(swapChainSupport.formats).format;
}

VkQueue VulkanDevice::GetGraphicsQueue() const {
	return m_graphicsQueue;
}

VkQueue VulkanDevice::GetPresentQueue() const {
	return m_presentQueue;
}

void VulkanDevice::CreateImage(
    uint32_t width,
    uint32_t height,
    uint32_t mipLevels,
    VkSampleCountFlagBits numSamples,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& imageMemory
) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = numSamples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(m_device, image, imageMemory, 0);
}

void VulkanDevice::DestroyImage(VkImage image, VkDeviceMemory memory) {
	vkDestroyImage(m_device, image, nullptr);
	vkFreeMemory(m_device, memory, nullptr);
}

void VulkanDevice::CreateImageView(
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectFlags,
    uint32_t mipLevels,
    VkImageView& outImageView
) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(m_device, &viewInfo, nullptr, &outImageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image view!");
	}
}

void VulkanDevice::DestroyImageView(VkImageView imageView) {
	vkDestroyImageView(m_device, imageView, nullptr);
}

void VulkanDevice::CreateCommandPool(VkCommandPool& outCommandPool) {
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = m_queueFamilyIndices.graphicsFamily;

	if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &outCommandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics command pool!");
	}
}

void VulkanDevice::DestroyCommandPool(VkCommandPool commandPool) {
	vkDestroyCommandPool(m_device, commandPool, nullptr);
}

void VulkanDevice::CreateCommandBuffer(
    VkCommandPool commandPool,
    VkCommandBuffer& outCommandBufer
) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(m_device, &allocInfo, &outCommandBufer) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void VulkanDevice::DestroyCommandBuffer(VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
	vkFreeCommandBuffers(m_device, commandPool, 1, &commandBuffer);
}

void VulkanDevice::CopyBuffer(
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize size,
    VkDeviceSize srcOffset,
    VkDeviceSize dstOffset
) {
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(m_commandPool);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = srcOffset;
	copyRegion.dstOffset = dstOffset;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(m_commandPool, commandBuffer);
}

void VulkanDevice::CopyBufferToImage(
    VkBuffer buffer,
    VkImage image,
    uint32_t width,
    uint32_t height
) {
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(m_commandPool);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(
	    commandBuffer,
	    buffer,
	    image,
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    1,
	    &region
	);

	EndSingleTimeCommands(m_commandPool, commandBuffer);
}

void VulkanDevice::CreateFence(VkFence& outFence) {
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	if (vkCreateFence(m_device, &fenceInfo, nullptr, &outFence) != VK_SUCCESS) {
		throw std::runtime_error("failed to create fence!");
	}
}

void VulkanDevice::DestroyFence(VkFence fence) {
	vkDestroyFence(m_device, fence, nullptr);
}

void VulkanDevice::ResetFences(uint32_t fenceCount, const VkFence* fences) {
	vkResetFences(m_device, fenceCount, fences);
}

void VulkanDevice::WaitFences(
    uint32_t fenceCount,
    const VkFence* fences,
    VkBool32 waitAll,
    uint64_t timeout
) {
	vkWaitForFences(m_device, fenceCount, fences, waitAll, timeout);
}

void VulkanDevice::CreateSemaphore(VkSemaphore& outSemaphore) {
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &outSemaphore);
}

void VulkanDevice::DestroySemaphore(VkSemaphore semaphore) {
	vkDestroySemaphore(m_device, semaphore, nullptr);
}

void VulkanDevice::WaitIdle() {
	vkDeviceWaitIdle(m_device);
}

VkCommandBuffer VulkanDevice::BeginSingleTimeCommands(VkCommandPool commandPool) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanDevice::EndSingleTimeCommands(VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);

	vkFreeCommandBuffers(m_device, commandPool, 1, &commandBuffer);
}

VkCommandBuffer VulkanDevice::BeginSingleTimeCommands() {
	return BeginSingleTimeCommands(m_commandPool);
}

void VulkanDevice::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
	EndSingleTimeCommands(m_commandPool, commandBuffer);
}

void VulkanDevice::TransitionImageSingleTime(
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage,
    VkImageAspectFlags aspectMask,
    uint32_t layersCount,
    uint32_t mipLevels
) {
	if (oldLayout == newLayout) {
		return;
	}

	if (debug_isInRenderPass) {
		throw "should not trasition during render pass";
	}

	VkCommandBuffer buf = BeginSingleTimeCommands();

	TransitionImage(
	    buf,
		image,
	    oldLayout,
	    newLayout,
	    srcAccessMask,
	    dstAccessMask,
	    srcStage,
	    dstStage,
	    aspectMask,
		layersCount,
	    mipLevels
	);

	EndSingleTimeCommands(buf);
}

void VulkanDevice::TransitionImage(
	VkCommandBuffer cmdBuf,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage,
    VkImageAspectFlags aspectMask,
	uint32_t layersCount,
    uint32_t mipLevels
) {
	if (oldLayout == newLayout) {
		return;
	}

	if (debug_isInRenderPass) {
		throw "should not trasition during render pass";
	}

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspectMask;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layersCount;
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstAccessMask = dstAccessMask;

	vkCmdPipelineBarrier(cmdBuf, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

QueueFamilyIndices VulkanDevice::GetQueueFamilyIndices() const {
	return m_queueFamilyIndices;
}

SwapChainSupportDetails VulkanDevice::QuerySwapChainSupport() const {
	return VulkanPhysicalDeviceUtils::QuerySwapChainSupport(m_physicalDevice, m_surface);
}

uint32_t VulkanDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) &&
		    (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
}

VkFormat VulkanDevice::FindDepthFormat() {
	return FindSupportedFormat(
	    { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat VulkanDevice::FindSupportedFormat(
    const std::vector<VkFormat>& candidates,
    VkImageTiling tiling,
    VkFormatFeatureFlags features
) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);
		if (tiling == VK_IMAGE_TILING_LINEAR &&
		    (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
		           (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}
	throw std::runtime_error("failed to find supported format!");
}

void VulkanDevice::CreateLogicalDevice(const std::vector<const char*>& deviceExtensions) {
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { m_queueFamilyIndices.graphicsFamily,
		                                       m_queueFamilyIndices.presentFamily };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.enabledLayerCount = 0;

	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentFamily, 0, &m_presentQueue);
}

} // namespace PixieRenderer
