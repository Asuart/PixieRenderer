#include "VulkanInstance.h"

#include <iostream>
#include <stdexcept>

#include "VulkanConfig.h"
#include "DebugVulkan.h"
#include "VulkanConfig.h"
#include "VulkanDevice.h"
#include "VulkanPhysicalDeviceUtils.h"

namespace PixieRenderer {

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

VulkanInstance::VulkanInstance() {
}

VulkanInstance::~VulkanInstance() {
	if (m_instance != VK_NULL_HANDLE) {
		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
		}
		vkDestroyInstance(m_instance, nullptr);
	}
}

void VulkanInstance::Initialize(std::vector<const char*> requiredExtensions) {
	PrintAvailableLayers();
	PrintAvailableExtensions();

	if (enableValidationLayers && !CheckValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = cApplicationName.c_str();
	appInfo.applicationVersion = cApplicationVersion;
	appInfo.pEngineName = cEngineName.c_str();
	appInfo.engineVersion = cEngineVersion;
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	if (enableValidationLayers) {
		requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}

	SetupDebugMessenger();
}

const std::vector<const char*>& VulkanInstance::GetDeviceExtensions() const {
	return deviceExtensions;
}

VkPhysicalDevice VulkanInstance::PickPhysicalDevice(VkSurfaceKHR surface) const {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	for (const auto& device : devices) {
		if (IsDeviceSuitable(device, surface)) {
			physicalDevice = device;
			break;
		}
	}

	return physicalDevice;
}

VkInstance VulkanInstance::GetInstance() const {
	return m_instance;
}

bool VulkanInstance::CheckValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) {
			return false;
		}
	}

	return true;
}

void VulkanInstance::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo
) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
	                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
	                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
	                         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
	                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
}

void VulkanInstance::SetupDebugMessenger() {
	if (!enableValidationLayers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) !=
	    VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

bool VulkanInstance::IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const {
	QueueFamilyIndices indices = VulkanPhysicalDeviceUtils::FindQueueFamilies(device, surface);

	bool extensionsSupported = VulkanPhysicalDeviceUtils::
	    CheckExtensionSupport(device, deviceExtensions);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails
		    swapChainSupport = VulkanPhysicalDeviceUtils::QuerySwapChainSupport(device, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() &&
		                    !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.IsComplete() && extensionsSupported && swapChainAdequate &&
	       supportedFeatures.samplerAnisotropy;
}

void VulkanInstance::PrintAvailableLayers() {
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	std::cout << "Available instance validation layers:\n";
	for (const auto& layer : availableLayers) {
		std::cout << "\t" << layer.layerName << " - " << layer.description << "\n";
	}
	std::cout << "\n";
}

void VulkanInstance::PrintAvailableExtensions() {
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

	std::cout << "Available instance extensions:\n";
	for (const auto& extension : availableExtensions) {
		std::cout << "\t" << extension.extensionName << " (Spec Version: " << extension.specVersion
		          << ")\n";
	}
	std::cout << "\n";
}

} // namespace PixieRenderer
