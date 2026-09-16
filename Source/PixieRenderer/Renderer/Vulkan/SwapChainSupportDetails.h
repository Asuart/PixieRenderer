#pragma once
#include <vector>

#include <vulkan/vulkan.h>

namespace PixieRenderer {

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities = {};
	std::vector<VkSurfaceFormatKHR> formats= {};
	std::vector<VkPresentModeKHR> presentModes = {};
};

} // namespace PixieRenderer
