#include "PixieRendering/pch.h"
#include "VulkanPhysicalDeviceUtils.h"

namespace PixieRenderer {

QueueFamilyIndices VulkanPhysicalDeviceUtils::FindQueueFamilies(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface
) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(
	    physicalDevice,
	    &queueFamilyCount,
	    queueFamilies.data()
	);

	for (size_t i = 0; i < queueFamilies.size(); i++) {
		const auto& queueFamily = queueFamilies[i];

		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = static_cast<uint32_t>(i);
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(
		    physicalDevice,
		    static_cast<uint32_t>(i),
		    surface,
		    &presentSupport
		);

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
	vkEnumerateDeviceExtensionProperties(
	    physicalDevice,
	    nullptr,
	    &extensionCount,
	    availableExtensions.data()
	);

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
		vkGetPhysicalDeviceSurfaceFormatsKHR(
		    physicalDevice,
		    surface,
		    &formatCount,
		    details.formats.data()
		);
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

void VulkanPhysicalDeviceUtils::PrintPhysicalDeviceProperties(VkPhysicalDevice physicalDevice) {
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(physicalDevice, &props);

	std::cout << "===== PHYSICAL DEVICE PROPERTIES =====\n";
	std::cout << "Device name       : " << props.deviceName << '\n';
	std::cout << "Device type       : ";
	switch (props.deviceType) {
	case VK_PHYSICAL_DEVICE_TYPE_OTHER:
		std::cout << "Other";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		std::cout << "Integrated GPU";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		std::cout << "Discrete GPU";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		std::cout << "Virtual GPU";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		std::cout << "CPU";
		break;
	default:
		std::cout << "Unknown";
		break;
	}
	std::cout << '\n';
	std::cout << "API version       : " << VK_API_VERSION_MAJOR(props.apiVersion) << '.'
	          << VK_API_VERSION_MINOR(props.apiVersion) << '.'
	          << VK_API_VERSION_PATCH(props.apiVersion) << '\n';
	std::cout << "Driver version    : " << props.driverVersion << '\n';
	std::cout << "Vendor ID         : 0x" << std::hex << props.vendorID << std::dec << '\n';
	std::cout << "Device ID         : 0x" << std::hex << props.deviceID << std::dec << '\n';
	std::cout << "Pipeline cache UUID: ";
	for (uint32_t i = 0; i < VK_UUID_SIZE; ++i) {
		std::cout << std::hex << std::setw(2) << std::setfill('0')
		          << (int)props.pipelineCacheUUID[i] << (i < VK_UUID_SIZE - 1 ? "-" : "");
	}
	std::cout << std::dec << "\n\n";

	const auto& lim = props.limits;
	std::cout << "----- LIMITS -----\n";
#define PRINT_LIMIT(name) std::cout << #name << ": " << lim.name << '\n'
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
	std::cout << "maxComputeWorkGroupCount  : " << lim.maxComputeWorkGroupCount[0] << ", "
	          << lim.maxComputeWorkGroupCount[1] << ", " << lim.maxComputeWorkGroupCount[2] << '\n';
	PRINT_LIMIT(maxComputeWorkGroupInvocations);
	std::cout << "maxComputeWorkGroupSize   : " << lim.maxComputeWorkGroupSize[0] << ", "
	          << lim.maxComputeWorkGroupSize[1] << ", " << lim.maxComputeWorkGroupSize[2] << '\n';
	PRINT_LIMIT(subPixelPrecisionBits);
	PRINT_LIMIT(subTexelPrecisionBits);
	PRINT_LIMIT(mipmapPrecisionBits);
	PRINT_LIMIT(maxDrawIndexedIndexValue);
	PRINT_LIMIT(maxDrawIndirectCount);
	PRINT_LIMIT(maxSamplerLodBias);
	PRINT_LIMIT(maxSamplerAnisotropy);
	PRINT_LIMIT(maxViewports);
	std::cout << "maxViewportDimensions    : " << lim.maxViewportDimensions[0] << " x "
	          << lim.maxViewportDimensions[1] << '\n';
	std::cout << "viewportBoundsRange      : " << lim.viewportBoundsRange[0] << " .. "
	          << lim.viewportBoundsRange[1] << '\n';
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
	std::cout << "timestampComputeAndGraphics: "
	          << (lim.timestampComputeAndGraphics ? "true" : "false") << '\n';
	PRINT_LIMIT(timestampPeriod);
	PRINT_LIMIT(maxClipDistances);
	PRINT_LIMIT(maxCullDistances);
	PRINT_LIMIT(maxCombinedClipAndCullDistances);
	PRINT_LIMIT(discreteQueuePriorities);
	std::cout << "pointSizeRange          : " << lim.pointSizeRange[0] << " .. "
	          << lim.pointSizeRange[1] << '\n';
	std::cout << "lineWidthRange          : " << lim.lineWidthRange[0] << " .. "
	          << lim.lineWidthRange[1] << '\n';
	PRINT_LIMIT(pointSizeGranularity);
	PRINT_LIMIT(lineWidthGranularity);
	std::cout << "strictLines             : " << (lim.strictLines ? "true" : "false") << '\n';
	std::cout << "standardSampleLocations : " << (lim.standardSampleLocations ? "true" : "false")
	          << '\n';
	PRINT_LIMIT(optimalBufferCopyOffsetAlignment);
	PRINT_LIMIT(optimalBufferCopyRowPitchAlignment);
	PRINT_LIMIT(nonCoherentAtomSize);
#undef PRINT_LIMIT
	std::cout << '\n';

	uint32_t extCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, extensions.data());
	std::cout << "----- DEVICE EXTENSIONS (" << extCount << ") -----\n";
	for (const auto& ext : extensions) {
		std::cout << "  " << ext.extensionName << " (spec " << ext.specVersion << ")\n";
	}
	std::cout << '\n';

	uint32_t qfCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qfCount, nullptr);
	std::vector<VkQueueFamilyProperties> qfProps(qfCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qfCount, qfProps.data());
	std::cout << "----- QUEUE FAMILIES (" << qfCount << ") -----\n";
	for (uint32_t i = 0; i < qfCount; ++i) {
		std::cout << "  Family " << i << ":\n";
		std::cout << "    queueCount           : " << qfProps[i].queueCount << '\n';
		std::cout << "    timestampValidBits   : " << qfProps[i].timestampValidBits << '\n';
		std::cout << "    minImageTransferGranularity: "
		          << qfProps[i].minImageTransferGranularity.width << "x"
		          << qfProps[i].minImageTransferGranularity.height << "x"
		          << qfProps[i].minImageTransferGranularity.depth << '\n';
		std::cout << "    flags                : ";
		if (qfProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			std::cout << "GRAPHICS ";
		if (qfProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
			std::cout << "COMPUTE ";
		if (qfProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
			std::cout << "TRANSFER ";
		if (qfProps[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
			std::cout << "SPARSE_BINDING ";
		if (qfProps[i].queueFlags & VK_QUEUE_PROTECTED_BIT)
			std::cout << "PROTECTED ";
		std::cout << "\n\n";
	}

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
	std::cout << "----- MEMORY PROPERTIES -----\n";
	std::cout << "  Memory heaps (" << memProps.memoryHeapCount << "):\n";
	for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
		std::cout << "    Heap " << i << ": size = " << memProps.memoryHeaps[i].size
		          << " bytes, flags = " << memProps.memoryHeaps[i].flags << '\n';
	}
	std::cout << "  Memory types (" << memProps.memoryTypeCount << "):\n";
	for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
		std::cout << "    Type " << i << ": heap = " << memProps.memoryTypes[i].heapIndex
		          << ", propertyFlags = " << memProps.memoryTypes[i].propertyFlags << '\n';
	}
	std::cout << '\n';

	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(physicalDevice, &features);
	std::cout << "----- DEVICE FEATURES -----\n";
#define PRINT_FEATURE(f) std::cout << "  " #f ": " << (features.f ? "true" : "false") << '\n'
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