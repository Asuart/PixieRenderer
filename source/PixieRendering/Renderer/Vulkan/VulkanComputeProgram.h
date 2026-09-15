#pragma once

#include "ShaderCompilationVulkan.h"
#include "VulkanProgram.h"
#include <string>

namespace PixieRenderer {

class VulkanDevice;

class VulkanComputeProgram : public VulkanProgram {
  public:
	VulkanComputeProgram(VulkanDevice& device, const std::string& source);
	~VulkanComputeProgram() override;

	void Dispatch(
	    VkCommandBuffer cmdBuf,
	    uint32_t frameIndex,
	    uint32_t x,
	    uint32_t y,
	    uint32_t z,
	    const void* pushData,
	    uint32_t pushSize
	);

	VkPipeline GetPipeline() const;

  private:
	VkShaderModule m_shaderModule = VK_NULL_HANDLE;
	VkPipelineShaderStageCreateInfo m_shaderStageCreateInfo{};
	VkPipeline m_pipeline = VK_NULL_HANDLE;

	void CreatePipeline();
};

} // namespace PixieRenderer
