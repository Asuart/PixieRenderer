#pragma once
#include <array>
#include <cstdint>
#include <string>

#include <vulkan/vulkan.hpp>

#include "../ShaderCompiler.h"

namespace PixieRenderer {

struct CompiledShader {
	std::vector<VkShaderModule> stages;
	std::vector<VkPipelineShaderStageCreateInfo> stagesCreateInfo;
	BindingsInfo bindingsInfo;
};

struct CompiledComputeShader {
	VkShaderModule stage;
	VkPipelineShaderStageCreateInfo stageCreateInfo;
	BindingsInfo bindingsInfo;
};

class ShaderCompilerVulkan {
  public:
	static CompiledShader CompileShader(
	    VkDevice device,
	    const char* vertexShaderSource,
	    const char* framgentShaderSource
	);
	static CompiledComputeShader CompileComputeShader(VkDevice device, const char* source);

  private:
	static SpirVBinary CompileShaderToSPIRV(glslang_stage_t stage, const char* shaderSource);
	static VkShaderModule CreateShaderModule(VkDevice device, SpirVBinary binary);
	static BindingsInfo ReflectSPIRV(const SpirVBinary& binary);
};

} // namespace PixieRenderer
