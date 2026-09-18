#pragma once
#include <array>
#include <cstdint>
#include <string>

#include <vulkan/vulkan.hpp>

#include "ShaderCompiler.h"

namespace PixieRenderer {

DescriptorType FromVkDescriptorType(VkDescriptorType t);
VkDescriptorType ToVkDescriptorType(DescriptorType t);
VkShaderStageFlagBits ToVkShaderStage(ShaderStage stage);
VkShaderStageFlags ToVkShaderStageMask(ShaderStageMask mask);

struct CompiledShader {
	std::vector<VkShaderModule> stages;
	std::vector<VkPipelineShaderStageCreateInfo> stagesCreateInfo;
	BindingsInfo bindingsInfo;
};

struct CompiledComputeShader {
	VkShaderModule stage = VK_NULL_HANDLE;
	VkPipelineShaderStageCreateInfo stageCreateInfo{};
	BindingsInfo bindingsInfo;
};

class ShaderCompilerVulkan {
  public:
	static CompiledShader CompileShader(
	    VkDevice device,
	    const char* vertexShaderSource,
	    const char* fragmentShaderSource
	);
	static CompiledComputeShader CompileComputeShader(VkDevice device, const char* source);

  private:
	static VkShaderModule CreateShaderModule(VkDevice device, const SpirVBinary& binary);
};

} // namespace PixieRenderer
