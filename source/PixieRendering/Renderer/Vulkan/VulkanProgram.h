#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include "PixieRendering/ResourceManager/ResourceHandles.h"
#include "ShaderCompilationVulkan.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"

namespace PixieRenderer {

class VulkanDevice;

class VulkanProgram {
  protected:
	VulkanProgram(VulkanDevice& device);
	void Init(const BindingsInfo& bindingsInfo);
	virtual ~VulkanProgram();

  public:
	VulkanProgram(const VulkanProgram&) = delete;
	VulkanProgram& operator=(const VulkanProgram&) = delete;

	VkDescriptorSetLayout GetDescriptorSetLayout() const;
	VkPipelineLayout GetPipelineLayout() const;
	const std::vector<VkDescriptorSet>& GetDescriptorSets() const;
	const std::unordered_map<uint32_t, std::vector<VulkanBuffer>>& GetUniformBuffers() const;
	VulkanBuffer* GetUniformBuffer(const std::string& name, uint32_t frameIndex);

	uint32_t GetBindingIndex(const std::string& name) const;

	void BindTexture(
	    const std::string& name,
	    TextureHandle handle,
	    VulkanTexture& texture,
	    uint32_t frameIndex,
	    uint32_t index
	);

	void UpdateUniformBuffer(
	    uint32_t binding,
	    uint32_t frameIndex,
	    const void* data,
	    size_t size,
	    size_t offset = 0
	);

  protected:
	VulkanDevice& m_device;
	BindingsInfo m_bindingsInfo;
	std::unordered_map<std::string, uint32_t> m_nameToBinding;

	VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> m_descriptorSets;
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

	std::unordered_map<uint32_t, std::vector<VulkanBuffer>> m_uniformBuffers;

  public:
	void CreateDescriptorSetLayout();
	void CreateDescriptorPool();
	void CreatePipelineLayout();
	void AllocateDescriptorSets();
	void CreateUniformBuffers();
	void UpdateDescriptorSetsForUniforms();
};

} // namespace PixieRenderer
