#include "ShaderCompilerVulkan.h"
#include "PixieRenderer/pch.h"

namespace PixieRenderer {

DescriptorType FromVkDescriptorType(VkDescriptorType t) {
	switch (t) {
	case VK_DESCRIPTOR_TYPE_SAMPLER:
		return DescriptorType::Sampler;
	case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		return DescriptorType::CombinedImageSampler;
	case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		return DescriptorType::SampledImage;
	case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		return DescriptorType::StorageImage;
	case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		return DescriptorType::UniformTexelBuffer;
	case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
		return DescriptorType::StorageTexelBuffer;
	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		return DescriptorType::UniformBuffer;
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		return DescriptorType::StorageBuffer;
	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		return DescriptorType::UniformBufferDynamic;
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
		return DescriptorType::StorageBufferDynamic;
	case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
		return DescriptorType::InputAttachment;
	default:
		return DescriptorType::Unknown;
	}
}

VkDescriptorType ToVkDescriptorType(DescriptorType t) {
	switch (t) {
	case DescriptorType::Sampler:
		return VK_DESCRIPTOR_TYPE_SAMPLER;
	case DescriptorType::CombinedImageSampler:
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	case DescriptorType::SampledImage:
		return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case DescriptorType::StorageImage:
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case DescriptorType::UniformTexelBuffer:
		return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
	case DescriptorType::StorageTexelBuffer:
		return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
	case DescriptorType::UniformBuffer:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case DescriptorType::StorageBuffer:
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case DescriptorType::UniformBufferDynamic:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	case DescriptorType::StorageBufferDynamic:
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	case DescriptorType::InputAttachment:
		return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	default:
		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}

VkShaderStageFlagBits ToVkShaderStage(ShaderStage stage) {
	switch (stage) {
	case ShaderStage::Vertex:
		return VK_SHADER_STAGE_VERTEX_BIT;
	case ShaderStage::Fragment:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	case ShaderStage::Geometry:
		return VK_SHADER_STAGE_GEOMETRY_BIT;
	case ShaderStage::Compute:
		return VK_SHADER_STAGE_COMPUTE_BIT;
	}
	return static_cast<VkShaderStageFlagBits>(0);
}

VkShaderStageFlags ToVkShaderStageMask(ShaderStageMask mask) {
	VkShaderStageFlags out = 0;
	if (mask & ShaderStageBit::Vertex)
		out |= VK_SHADER_STAGE_VERTEX_BIT;
	if (mask & ShaderStageBit::Fragment)
		out |= VK_SHADER_STAGE_FRAGMENT_BIT;
	if (mask & ShaderStageBit::Geometry)
		out |= VK_SHADER_STAGE_GEOMETRY_BIT;
	if (mask & ShaderStageBit::Compute)
		out |= VK_SHADER_STAGE_COMPUTE_BIT;
	return out;
}

VkShaderModule ShaderCompilerVulkan::CreateShaderModule(VkDevice device, const SpirVBinary& binary) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = static_cast<size_t>(binary.size) * sizeof(uint32_t);
	createInfo.pCode = binary.words;

	VkShaderModule module = VK_NULL_HANDLE;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create vulkan shader module");
	}
	return module;
}

CompiledShader ShaderCompilerVulkan::CompileShader(
    VkDevice device,
    const char* vertexShaderSource,
    const char* fragmentShaderSource
) {
	if (!ShaderCompiler::IsInitialized()) {
		ShaderCompiler::Initialize();
	}

	SpirVBinary vertBin = ShaderCompiler::CompileToSPIRV(ShaderStage::Vertex, vertexShaderSource);
	SpirVBinary fragBin = ShaderCompiler::CompileToSPIRV(ShaderStage::Fragment, fragmentShaderSource);

	BindingsInfo vertInfo = ShaderCompiler::ReflectSPIRV(vertBin, ShaderStage::Vertex);
	BindingsInfo fragInfo = ShaderCompiler::ReflectSPIRV(fragBin, ShaderStage::Fragment);
	BindingsInfo finalInfo = ShaderCompiler::MergeBindingsInfo(vertInfo, fragInfo);

	VkShaderModule vertModule = CreateShaderModule(device, vertBin);
	VkShaderModule fragModule = CreateShaderModule(device, fragBin);

	ShaderCompiler::FreeSPIRV(vertBin);
	ShaderCompiler::FreeSPIRV(fragBin);

	VkPipelineShaderStageCreateInfo vertStage{};
	vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStage.module = vertModule;
	vertStage.pName = "main";

	VkPipelineShaderStageCreateInfo fragStage{};
	fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStage.module = fragModule;
	fragStage.pName = "main";

	return { { vertModule, fragModule }, { vertStage, fragStage }, finalInfo };
}

CompiledComputeShader ShaderCompilerVulkan::CompileComputeShader(VkDevice device, const char* source) {
	if (!ShaderCompiler::IsInitialized()) {
		ShaderCompiler::Initialize();
	}

	SpirVBinary bin = ShaderCompiler::CompileToSPIRV(ShaderStage::Compute, source);
	BindingsInfo info = ShaderCompiler::ReflectSPIRV(bin, ShaderStage::Compute);

	VkShaderModule module = CreateShaderModule(device, bin);
	ShaderCompiler::FreeSPIRV(bin);

	VkPipelineShaderStageCreateInfo stage{};
	stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage.module = module;
	stage.pName = "main";

	return { module, stage, info };
}

} // namespace PixieRenderer
