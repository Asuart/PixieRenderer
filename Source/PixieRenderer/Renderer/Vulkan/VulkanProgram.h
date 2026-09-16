#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include "PixieRenderer/ResourceManager/ResourceHandles.h"
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
	const BindingsInfo& GetBindingsInfo() const {
		return m_bindingsInfo;
	}

	uint32_t GetBindingIndex(std::string_view name) const;
	VkDescriptorType GetDescriptorType(std::string_view name) const;

	void BindBuffer(
	    std::string_view name,
	    VkBuffer buffer,
	    VkDeviceSize offset,
	    VkDeviceSize range,
	    uint32_t frameIndex
	);

	void BindTexture(std::string_view name, VulkanTexture& texture, uint32_t frameIndex, uint32_t arrayIndex);
	void BindTextureView(
	    std::string_view name,
	    VkImageView view,
	    VkSampler sampler,
	    uint32_t frameIndex,
	    uint32_t arrayIndex = 0
	);

  protected:
	VulkanDevice& m_device;
	BindingsInfo m_bindingsInfo;
	std::unordered_map<std::string, ShaderBinding> m_bindingsByName;

	VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> m_descriptorSets;
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

	void CreateDescriptorSetLayout();
	void CreateDescriptorPool();
	void AllocateDescriptorSets();
	void CreatePipelineLayout();
};

} // namespace PixieRenderer
