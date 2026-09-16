#pragma once
#include <array>
#include <vector>

#include <vulkan/vulkan.h>

#include "QueueFamilyIndices.h"
#include "SwapChainSupportDetails.h"
#include "VulkanConfig.h"

namespace PixieRenderer {

class VulkanPhysicalDeviceUtils {
  public:
	static QueueFamilyIndices FindQueueFamilies(
	    VkPhysicalDevice physicalDevice,
	    VkSurfaceKHR surface
	);
	static bool CheckExtensionSupport(
	    VkPhysicalDevice physicalDevice,
	    const std::vector<const char*>& deviceExtensions
	);
	static SwapChainSupportDetails QuerySwapChainSupport(
	    VkPhysicalDevice physicalDevice,
	    VkSurfaceKHR surface
	);
	static VkImageAspectFlags GetAspectMask(VkFormat format);
	static void PrintPhysicalDeviceProperties(VkPhysicalDevice physicalDevice);
};

} // namespace PixieRenderer
