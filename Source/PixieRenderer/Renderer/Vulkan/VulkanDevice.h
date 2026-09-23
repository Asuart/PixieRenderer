#pragma once
#include <memory>

#include <vulkan/vulkan.h>

#include "QueueFamilyIndices.h"
#include "SwapChainSupportDetails.h"
#include "VulkanFrameBuffer.h"
#include "VulkanTexture.h"

namespace PixieRenderer {

class VulkanSwapchain;

class VulkanDevice {
  public:
	static constexpr uint32_t kInvalidMemoryType = UINT32_MAX;

	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	VkQueue m_presentQueue = VK_NULL_HANDLE;
	QueueFamilyIndices m_queueFamilyIndices = {};
	VkCommandPool m_commandPool = VK_NULL_HANDLE;

	void Initialize(
	    VkPhysicalDevice physicalDevice,
	    VkSurfaceKHR surface,
	    const std::vector<const char*>& deviceExtensions
	);
	void Cleanup();

	VkPhysicalDevice GetPhysicalDevice() const;
	VkDevice GetDevice() const;
	VkSurfaceKHR GetSurface() const;
	VkFormat GetSurfaceFormat() const;
	VkQueue GetGraphicsQueue() const;
	VkQueue GetPresentQueue() const;

	void CreateImage(
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
	);
	void DestroyImage(VkImage image, VkDeviceMemory memory);
	void CreateImageView(
	    VkImage image,
	    VkFormat format,
	    VkImageAspectFlags aspectFlags,
	    uint32_t mipLevels,
	    VkImageView& outImageView
	);
	void DestroyImageView(VkImageView imageView);

	void CreateCommandPool(VkCommandPool& outCommandPool);
	void DestroyCommandPool(VkCommandPool commandPool);

	void CreateCommandBuffer(VkCommandPool commandPool, VkCommandBuffer& outCommandBuffer);
	void DestroyCommandBuffer(VkCommandPool commandPool, VkCommandBuffer commandBuffer);

	void CopyBuffer(
	    VkBuffer srcBuffer,
	    VkBuffer dstBuffer,
	    VkDeviceSize size,
	    VkDeviceSize srcOffset = 0,
	    VkDeviceSize dstOffset = 0
	);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void CreateFence(VkFence& outFence);
	void DestroyFence(VkFence fence);
	void ResetFences(uint32_t fenceCount, const VkFence* fences);
	void WaitFences(uint32_t fenceCount, const VkFence* fences, VkBool32 waitAll, uint64_t timeout);

	void CreateSemaphore(VkSemaphore& outSemaphore);
	void DestroySemaphore(VkSemaphore semaphore);

	void WaitIdle();

	VkCommandBuffer BeginSingleTimeCommands(VkCommandPool commandPool);
	void EndSingleTimeCommands(VkCommandPool commandPool, VkCommandBuffer commandBuffer);
	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

	void TransitionImageSingleTime(
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
	);
	void TransitionImage(
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
	);

	QueueFamilyIndices GetQueueFamilyIndices() const;
	SwapChainSupportDetails QuerySwapChainSupport() const;

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat FindDepthFormat();
	VkFormat FindSupportedFormat(
	    const std::vector<VkFormat>& candidates,
	    VkImageTiling tiling,
	    VkFormatFeatureFlags features
	);

  private:
	void CreateLogicalDevice(const std::vector<const char*>& deviceExtensions);
};

} // namespace PixieRenderer
