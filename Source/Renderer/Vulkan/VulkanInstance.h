#pragma once
#include <vector>

#include <vulkan/vulkan.h>

namespace PixieRenderer {

class VulkanInstance {
  public:
	VulkanInstance();
	~VulkanInstance();

	void Initialize(std::vector<const char*> requiredExtensions);

	const std::vector<const char*>& GetDeviceExtensions() const;
	VkPhysicalDevice PickPhysicalDevice(VkSurfaceKHR surface) const;

	VkInstance GetInstance() const;

  private:
	VkInstance m_instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

	bool CheckValidationLayerSupport();
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void SetupDebugMessenger();
	bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const;
	void PrintAvailableLayers();
	void PrintAvailableExtensions();
};

} // namespace PixieRenderer
