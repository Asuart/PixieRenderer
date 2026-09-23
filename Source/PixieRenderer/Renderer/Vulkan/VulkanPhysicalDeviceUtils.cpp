#include "VulkanPhysicalDeviceUtils.h"
#include "PixieRenderer/pch.h"

#include "PixieRenderer/LogCategories.h"

namespace PixieRenderer {

QueueFamilyIndices VulkanPhysicalDeviceUtils::FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	for (size_t i = 0; i < queueFamilies.size(); i++) {
		const auto& queueFamily = queueFamilies[i];

		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = static_cast<uint32_t>(i);
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, static_cast<uint32_t>(i), surface, &presentSupport);

		if (presentSupport) {
			indices.presentFamily = static_cast<uint32_t>(i);
		}

		if (indices.IsComplete()) {
			break;
		}
	}

	return indices;
}

bool VulkanPhysicalDeviceUtils::CheckExtensionSupport(
    VkPhysicalDevice physicalDevice,
    const std::vector<const char*>& deviceExtensions
) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

SwapChainSupportDetails VulkanPhysicalDeviceUtils::QuerySwapChainSupport(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface
) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
		    physicalDevice,
		    surface,
		    &presentModeCount,
		    details.presentModes.data()
		);
	}

	return details;
}

VkImageAspectFlags VulkanPhysicalDeviceUtils::GetAspectMask(VkFormat format) {
	switch (format) {
	case VK_FORMAT_D16_UNORM:
	case VK_FORMAT_D32_SFLOAT:
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	case VK_FORMAT_D24_UNORM_S8_UINT:
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	default:
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

namespace {

std::string FormatUuid(const uint8_t* uuid, size_t n) {
	std::string s;
	s.reserve(n * 2 + 4);
	for (size_t i = 0; i < n; ++i) {
		s += std::format("{:02x}", uuid[i]);
		if (i == 3 || i == 5 || i == 7 || i == 9)
			s += '-';
	}
	return s;
}

const char* DeviceTypeToString(VkPhysicalDeviceType t) {
	switch (t) {
	case VK_PHYSICAL_DEVICE_TYPE_OTHER:
		return "Other";
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		return "Integrated GPU";
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		return "Discrete GPU";
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		return "Virtual GPU";
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		return "CPU";
	default:
		return "Unknown";
	}
}

std::string QueueFlagsToString(VkQueueFlags flags) {
	std::string s;
	if (flags & VK_QUEUE_GRAPHICS_BIT)
		s += "GRAPHICS ";
	if (flags & VK_QUEUE_COMPUTE_BIT)
		s += "COMPUTE ";
	if (flags & VK_QUEUE_TRANSFER_BIT)
		s += "TRANSFER ";
	if (flags & VK_QUEUE_SPARSE_BINDING_BIT)
		s += "SPARSE_BINDING ";
	if (flags & VK_QUEUE_PROTECTED_BIT)
		s += "PROTECTED ";
	if (!s.empty() && s.back() == ' ')
		s.pop_back();
	return s.empty() ? "-" : s;
}

std::string MemoryHeapFlagsToString(VkMemoryHeapFlags flags) {
	std::string s;
	if (flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
		s += "DEVICE_LOCAL ";
	if (flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)
		s += "MULTI_INSTANCE ";
	if (!s.empty() && s.back() == ' ')
		s.pop_back();
	return s.empty() ? "-" : s;
}

std::string MemoryPropertyFlagsToString(VkMemoryPropertyFlags flags) {
	std::string s;
	if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		s += "DEVICE_LOCAL ";
	if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		s += "HOST_VISIBLE ";
	if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		s += "HOST_COHERENT ";
	if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
		s += "HOST_CACHED ";
	if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
		s += "LAZILY_ALLOCATED ";
	if (flags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
		s += "PROTECTED ";
	if (!s.empty() && s.back() == ' ')
		s.pop_back();
	return s.empty() ? "-" : s;
}

} // namespace

void VulkanPhysicalDeviceUtils::PrintPhysicalDeviceProperties(VkPhysicalDevice physicalDevice) {
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(physicalDevice, &props);

	Log::Info(LogCat::vkPhysicalDeviceUtils, "===== PHYSICAL DEVICE PROPERTIES =====");
	Log::Info(LogCat::vkPhysicalDeviceUtils, "Device name         : {}", (const char*)props.deviceName);
	Log::Info(LogCat::vkPhysicalDeviceUtils, "Device type         : {}", DeviceTypeToString(props.deviceType));
	Log::Info(
	    LogCat::vkPhysicalDeviceUtils,
	    "API version         : {}.{}.{}",
	    VK_API_VERSION_MAJOR(props.apiVersion),
	    VK_API_VERSION_MINOR(props.apiVersion),
	    VK_API_VERSION_PATCH(props.apiVersion)
	);
	Log::Info(LogCat::vkPhysicalDeviceUtils, "Driver version      : {}", props.driverVersion);
	Log::Info(LogCat::vkPhysicalDeviceUtils, "Vendor ID           : 0x{:x}", props.vendorID);
	Log::Info(LogCat::vkPhysicalDeviceUtils, "Device ID           : 0x{:x}", props.deviceID);
	Log::Info(
	    LogCat::vkPhysicalDeviceUtils,
	    "Pipeline cache UUID : {}",
	    FormatUuid(props.pipelineCacheUUID, VK_UUID_SIZE)
	);
	Log::Info(LogCat::vkPhysicalDeviceUtils, "");

	const auto& lim = props.limits;
	Log::Info(LogCat::vkPhysicalDeviceUtils, "----- LIMITS -----");
#define PRINT_LIMIT(name) Log::Info(LogCat::vkPhysicalDeviceUtils, "  {}: {}", #name, lim.name)
	PRINT_LIMIT(maxImageDimension1D);
	PRINT_LIMIT(maxImageDimension2D);
	PRINT_LIMIT(maxImageDimension3D);
	PRINT_LIMIT(maxImageDimensionCube);
	PRINT_LIMIT(maxImageArrayLayers);
	PRINT_LIMIT(maxTexelBufferElements);
	PRINT_LIMIT(maxUniformBufferRange);
	PRINT_LIMIT(maxStorageBufferRange);
	PRINT_LIMIT(maxPushConstantsSize);
	PRINT_LIMIT(maxMemoryAllocationCount);
	PRINT_LIMIT(maxSamplerAllocationCount);
	PRINT_LIMIT(bufferImageGranularity);
	PRINT_LIMIT(sparseAddressSpaceSize);
	PRINT_LIMIT(maxBoundDescriptorSets);
	PRINT_LIMIT(maxPerStageDescriptorSamplers);
	PRINT_LIMIT(maxPerStageDescriptorUniformBuffers);
	PRINT_LIMIT(maxPerStageDescriptorStorageBuffers);
	PRINT_LIMIT(maxPerStageDescriptorSampledImages);
	PRINT_LIMIT(maxPerStageDescriptorStorageImages);
	PRINT_LIMIT(maxPerStageDescriptorInputAttachments);
	PRINT_LIMIT(maxPerStageResources);
	PRINT_LIMIT(maxDescriptorSetSamplers);
	PRINT_LIMIT(maxDescriptorSetUniformBuffers);
	PRINT_LIMIT(maxDescriptorSetUniformBuffersDynamic);
	PRINT_LIMIT(maxDescriptorSetStorageBuffers);
	PRINT_LIMIT(maxDescriptorSetStorageBuffersDynamic);
	PRINT_LIMIT(maxDescriptorSetSampledImages);
	PRINT_LIMIT(maxDescriptorSetStorageImages);
	PRINT_LIMIT(maxDescriptorSetInputAttachments);
	PRINT_LIMIT(maxVertexInputAttributes);
	PRINT_LIMIT(maxVertexInputBindings);
	PRINT_LIMIT(maxVertexInputAttributeOffset);
	PRINT_LIMIT(maxVertexInputBindingStride);
	PRINT_LIMIT(maxVertexOutputComponents);
	PRINT_LIMIT(maxTessellationGenerationLevel);
	PRINT_LIMIT(maxTessellationPatchSize);
	PRINT_LIMIT(maxTessellationControlPerVertexInputComponents);
	PRINT_LIMIT(maxTessellationControlPerVertexOutputComponents);
	PRINT_LIMIT(maxTessellationControlPerPatchOutputComponents);
	PRINT_LIMIT(maxTessellationControlTotalOutputComponents);
	PRINT_LIMIT(maxTessellationEvaluationInputComponents);
	PRINT_LIMIT(maxTessellationEvaluationOutputComponents);
	PRINT_LIMIT(maxGeometryShaderInvocations);
	PRINT_LIMIT(maxGeometryInputComponents);
	PRINT_LIMIT(maxGeometryOutputComponents);
	PRINT_LIMIT(maxGeometryOutputVertices);
	PRINT_LIMIT(maxGeometryTotalOutputComponents);
	PRINT_LIMIT(maxFragmentInputComponents);
	PRINT_LIMIT(maxFragmentOutputAttachments);
	PRINT_LIMIT(maxFragmentDualSrcAttachments);
	PRINT_LIMIT(maxFragmentCombinedOutputResources);
	PRINT_LIMIT(maxComputeSharedMemorySize);
	Log::Info(
	    LogCat::vkPhysicalDeviceUtils,
	    "  maxComputeWorkGroupCount: {} {} {}",
	    lim.maxComputeWorkGroupCount[0],
	    lim.maxComputeWorkGroupCount[1],
	    lim.maxComputeWorkGroupCount[2]
	);
	PRINT_LIMIT(maxComputeWorkGroupInvocations);
	Log::Info(
	    LogCat::vkPhysicalDeviceUtils,
	    "  maxComputeWorkGroupSize : {} {} {}",
	    lim.maxComputeWorkGroupSize[0],
	    lim.maxComputeWorkGroupSize[1],
	    lim.maxComputeWorkGroupSize[2]
	);
	PRINT_LIMIT(subPixelPrecisionBits);
	PRINT_LIMIT(subTexelPrecisionBits);
	PRINT_LIMIT(mipmapPrecisionBits);
	PRINT_LIMIT(maxDrawIndexedIndexValue);
	PRINT_LIMIT(maxDrawIndirectCount);
	PRINT_LIMIT(maxSamplerLodBias);
	PRINT_LIMIT(maxSamplerAnisotropy);
	PRINT_LIMIT(maxViewports);
	Log::Info(
	    LogCat::vkPhysicalDeviceUtils,
	    "  maxViewportDimensions   : {} x {}",
	    lim.maxViewportDimensions[0],
	    lim.maxViewportDimensions[1]
	);
	Log::Info(
	    LogCat::vkPhysicalDeviceUtils,
	    "  viewportBoundsRange     : {} .. {}",
	    lim.viewportBoundsRange[0],
	    lim.viewportBoundsRange[1]
	);
	PRINT_LIMIT(viewportSubPixelBits);
	PRINT_LIMIT(minMemoryMapAlignment);
	PRINT_LIMIT(minTexelBufferOffsetAlignment);
	PRINT_LIMIT(minUniformBufferOffsetAlignment);
	PRINT_LIMIT(minStorageBufferOffsetAlignment);
	PRINT_LIMIT(minTexelOffset);
	PRINT_LIMIT(maxTexelOffset);
	PRINT_LIMIT(minTexelGatherOffset);
	PRINT_LIMIT(maxTexelGatherOffset);
	PRINT_LIMIT(minInterpolationOffset);
	PRINT_LIMIT(maxInterpolationOffset);
	PRINT_LIMIT(subPixelInterpolationOffsetBits);
	PRINT_LIMIT(maxFramebufferWidth);
	PRINT_LIMIT(maxFramebufferHeight);
	PRINT_LIMIT(maxFramebufferLayers);
	PRINT_LIMIT(framebufferColorSampleCounts);
	PRINT_LIMIT(framebufferDepthSampleCounts);
	PRINT_LIMIT(framebufferStencilSampleCounts);
	PRINT_LIMIT(framebufferNoAttachmentsSampleCounts);
	PRINT_LIMIT(maxColorAttachments);
	PRINT_LIMIT(sampledImageColorSampleCounts);
	PRINT_LIMIT(sampledImageIntegerSampleCounts);
	PRINT_LIMIT(sampledImageDepthSampleCounts);
	PRINT_LIMIT(sampledImageStencilSampleCounts);
	PRINT_LIMIT(storageImageSampleCounts);
	PRINT_LIMIT(maxSampleMaskWords);
	Log::Info(
	    LogCat::vkPhysicalDeviceUtils,
	    "  timestampComputeAndGraphics: {}",
	    lim.timestampComputeAndGraphics ? "true" : "false"
	);
	PRINT_LIMIT(timestampPeriod);
	PRINT_LIMIT(maxClipDistances);
	PRINT_LIMIT(maxCullDistances);
	PRINT_LIMIT(maxCombinedClipAndCullDistances);
	PRINT_LIMIT(discreteQueuePriorities);
	Log::Info(
	    LogCat::vkPhysicalDeviceUtils,
	    "  pointSizeRange          : {} .. {}",
	    lim.pointSizeRange[0],
	    lim.pointSizeRange[1]
	);
	Log::Info(
	    LogCat::vkPhysicalDeviceUtils,
	    "  lineWidthRange          : {} .. {}",
	    lim.lineWidthRange[0],
	    lim.lineWidthRange[1]
	);
	PRINT_LIMIT(pointSizeGranularity);
	PRINT_LIMIT(lineWidthGranularity);
	Log::Info(LogCat::vkPhysicalDeviceUtils, "  strictLines             : {}", lim.strictLines ? "true" : "false");
	Log::Info(
	    LogCat::vkPhysicalDeviceUtils,
	    "  standardSampleLocations : {}",
	    lim.standardSampleLocations ? "true" : "false"
	);
	PRINT_LIMIT(optimalBufferCopyOffsetAlignment);
	PRINT_LIMIT(optimalBufferCopyRowPitchAlignment);
	PRINT_LIMIT(nonCoherentAtomSize);
#undef PRINT_LIMIT
	Log::Info(LogCat::vkPhysicalDeviceUtils, "");

	uint32_t extCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, extensions.data());
	Log::Info(LogCat::vkPhysicalDeviceUtils, "----- DEVICE EXTENSIONS ({}) -----", extCount);
	for (const auto& ext : extensions) {
		Log::Info(LogCat::vkPhysicalDeviceUtils, "  {} (spec {})", ext.extensionName, ext.specVersion);
	}
	Log::Info(LogCat::vkPhysicalDeviceUtils, "");

	uint32_t qfCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qfCount, nullptr);
	std::vector<VkQueueFamilyProperties> qfProps(qfCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qfCount, qfProps.data());
	Log::Info(LogCat::vkPhysicalDeviceUtils, "----- QUEUE FAMILIES ({}) -----", qfCount);
	for (uint32_t i = 0; i < qfCount; ++i) {
		Log::Info(LogCat::vkPhysicalDeviceUtils, "  Family {}:", i);
		Log::Info(LogCat::vkPhysicalDeviceUtils, "    queueCount                 : {}", qfProps[i].queueCount);
		Log::Info(LogCat::vkPhysicalDeviceUtils, "    timestampValidBits         : {}", qfProps[i].timestampValidBits);
		Log::Info(
		    LogCat::vkPhysicalDeviceUtils,
		    "    minImageTransferGranularity: {}x{}x{}",
		    qfProps[i].minImageTransferGranularity.width,
		    qfProps[i].minImageTransferGranularity.height,
		    qfProps[i].minImageTransferGranularity.depth
		);
		Log::Info(
		    LogCat::vkPhysicalDeviceUtils,
		    "    flags                      : {}",
		    QueueFlagsToString(qfProps[i].queueFlags)
		);
		Log::Info(LogCat::vkPhysicalDeviceUtils, "");
	}

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
	Log::Info(LogCat::vkPhysicalDeviceUtils, "----- MEMORY PROPERTIES -----");
	Log::Info(LogCat::vkPhysicalDeviceUtils, "  Memory heaps ({}):", memProps.memoryHeapCount);
	for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
		Log::Info(
		    LogCat::vkPhysicalDeviceUtils,
		    "    Heap {}: size = {} bytes, flags = [{}]",
		    i,
		    memProps.memoryHeaps[i].size,
		    MemoryHeapFlagsToString(memProps.memoryHeaps[i].flags)
		);
	}
	Log::Info(LogCat::vkPhysicalDeviceUtils, "  Memory types ({}):", memProps.memoryTypeCount);
	for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
		Log::Info(
		    LogCat::vkPhysicalDeviceUtils,
		    "    Type {}: heap = {}, properties = [{}]",
		    i,
		    memProps.memoryTypes[i].heapIndex,
		    MemoryPropertyFlagsToString(memProps.memoryTypes[i].propertyFlags)
		);
	}
	Log::Info(LogCat::vkPhysicalDeviceUtils, "");

	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(physicalDevice, &features);
	Log::Info(LogCat::vkPhysicalDeviceUtils, "----- DEVICE FEATURES -----");
#define PRINT_FEATURE(f) Log::Info(LogCat::vkPhysicalDeviceUtils, "  {}: {}", #f, (features.f ? "true" : "false"))
	PRINT_FEATURE(robustBufferAccess);
	PRINT_FEATURE(fullDrawIndexUint32);
	PRINT_FEATURE(imageCubeArray);
	PRINT_FEATURE(independentBlend);
	PRINT_FEATURE(geometryShader);
	PRINT_FEATURE(tessellationShader);
	PRINT_FEATURE(sampleRateShading);
	PRINT_FEATURE(dualSrcBlend);
	PRINT_FEATURE(logicOp);
	PRINT_FEATURE(multiDrawIndirect);
	PRINT_FEATURE(drawIndirectFirstInstance);
	PRINT_FEATURE(depthClamp);
	PRINT_FEATURE(depthBiasClamp);
	PRINT_FEATURE(fillModeNonSolid);
	PRINT_FEATURE(depthBounds);
	PRINT_FEATURE(wideLines);
	PRINT_FEATURE(largePoints);
	PRINT_FEATURE(alphaToOne);
	PRINT_FEATURE(multiViewport);
	PRINT_FEATURE(samplerAnisotropy);
	PRINT_FEATURE(textureCompressionETC2);
	PRINT_FEATURE(textureCompressionASTC_LDR);
	PRINT_FEATURE(textureCompressionBC);
	PRINT_FEATURE(occlusionQueryPrecise);
	PRINT_FEATURE(pipelineStatisticsQuery);
	PRINT_FEATURE(vertexPipelineStoresAndAtomics);
	PRINT_FEATURE(fragmentStoresAndAtomics);
	PRINT_FEATURE(shaderTessellationAndGeometryPointSize);
	PRINT_FEATURE(shaderImageGatherExtended);
	PRINT_FEATURE(shaderStorageImageExtendedFormats);
	PRINT_FEATURE(shaderStorageImageMultisample);
	PRINT_FEATURE(shaderStorageImageReadWithoutFormat);
	PRINT_FEATURE(shaderStorageImageWriteWithoutFormat);
	PRINT_FEATURE(shaderUniformBufferArrayDynamicIndexing);
	PRINT_FEATURE(shaderSampledImageArrayDynamicIndexing);
	PRINT_FEATURE(shaderStorageBufferArrayDynamicIndexing);
	PRINT_FEATURE(shaderStorageImageArrayDynamicIndexing);
	PRINT_FEATURE(shaderClipDistance);
	PRINT_FEATURE(shaderCullDistance);
	PRINT_FEATURE(shaderFloat64);
	PRINT_FEATURE(shaderInt64);
	PRINT_FEATURE(shaderInt16);
	PRINT_FEATURE(shaderResourceResidency);
	PRINT_FEATURE(shaderResourceMinLod);
	PRINT_FEATURE(sparseBinding);
	PRINT_FEATURE(sparseResidencyBuffer);
	PRINT_FEATURE(sparseResidencyImage2D);
	PRINT_FEATURE(sparseResidencyImage3D);
	PRINT_FEATURE(sparseResidency2Samples);
	PRINT_FEATURE(sparseResidency4Samples);
	PRINT_FEATURE(sparseResidency8Samples);
	PRINT_FEATURE(sparseResidency16Samples);
	PRINT_FEATURE(sparseResidencyAliased);
	PRINT_FEATURE(variableMultisampleRate);
	PRINT_FEATURE(inheritedQueries);
#undef PRINT_FEATURE
}

} // namespace PixieRenderer
