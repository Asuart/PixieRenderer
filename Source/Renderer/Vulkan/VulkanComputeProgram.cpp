#include "PixieRendering/pch.h"
#include "VulkanComputeProgram.h"

#include "VulkanConfig.h"
#include "VulkanDevice.h"

namespace PixieRenderer {

VulkanComputeProgram::VulkanComputeProgram(VulkanDevice& device, const std::string& source)
    : VulkanProgram(device) {
	VkDevice vkDevice = device.GetDevice();

	CompiledComputeShader compiled =
	    ShaderCompilerVulkan::CompileComputeShader(vkDevice, source.c_str());

	m_shaderModule = compiled.stage;
	m_shaderStageCreateInfo = compiled.stageCreateInfo;

	Init(compiled.bindingsInfo);

	CreatePipeline();
}

VulkanComputeProgram::~VulkanComputeProgram() {
	VkDevice device = m_device.GetDevice();

	if (m_pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(device, m_pipeline, nullptr);
	}
	if (m_shaderModule != VK_NULL_HANDLE) {
		vkDestroyShaderModule(device, m_shaderModule, nullptr);
	}
}

void VulkanComputeProgram::CreatePipeline() {
	VkDevice device = m_device.GetDevice();

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.stage = m_shaderStageCreateInfo;

	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) !=
	    VK_SUCCESS) {
		throw std::runtime_error("Failed to create compute pipeline");
	}
}

void VulkanComputeProgram::Dispatch(
    VkCommandBuffer cmdBuf,
    uint32_t frameIndex,
    uint32_t x,
    uint32_t y,
    uint32_t z,
    const void* pushData,
    uint32_t pushSize
) {
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

	const auto& sets = GetDescriptorSets();
	if (frameIndex >= sets.size()) {
		throw std::runtime_error("Frame index out of range");
	}
	vkCmdBindDescriptorSets(
	    cmdBuf,
	    VK_PIPELINE_BIND_POINT_COMPUTE,
	    m_pipelineLayout,
	    0,
	    1,
	    &sets[frameIndex],
	    0,
	    nullptr
	);

	if (pushSize > 0) {
		vkCmdPushConstants(cmdBuf, m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, pushSize, pushData);
	}

	vkCmdDispatch(cmdBuf, x, y, z);
}

VkPipeline VulkanComputeProgram::GetPipeline() const {
	return m_pipeline;
}

} // namespace PixieRenderer
