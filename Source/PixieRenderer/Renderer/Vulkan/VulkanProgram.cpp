#include "VulkanProgram.h"
#include "PixieRenderer/pch.h"

#include "VulkanConfig.h"
#include "VulkanDevice.h"

namespace PixieRenderer {

VulkanProgram::VulkanProgram(VulkanDevice& device) : m_device(device) {
}

VulkanProgram::~VulkanProgram() {
	VkDevice device = m_device.GetDevice();

	if (m_descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
	}
	if (m_descriptorSetLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
	}
	if (m_pipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
	}
}

void VulkanProgram::Init(const BindingsInfo& bindingsInfo) {
	m_bindingsInfo = bindingsInfo;
	for (const ShaderBinding& b : m_bindingsInfo.bindings) {
		m_bindingsByName[b.name] = b;
	}

	CreateDescriptorSetLayout();
	CreateDescriptorPool();
	AllocateDescriptorSets();
	CreatePipelineLayout();
}

VkDescriptorSetLayout VulkanProgram::GetDescriptorSetLayout() const {
	return m_descriptorSetLayout;
}

VkPipelineLayout VulkanProgram::GetPipelineLayout() const {
	return m_pipelineLayout;
}

const std::vector<VkDescriptorSet>& VulkanProgram::GetDescriptorSets() const {
	return m_descriptorSets;
}

uint32_t VulkanProgram::GetBindingIndex(std::string_view name) const {
	auto it = m_bindingsByName.find(std::string(name));
	if (it == m_bindingsByName.end()) {
		throw std::runtime_error("Binding not found: " + std::string(name));
	}
	return it->second.binding;
}

VkDescriptorType VulkanProgram::GetDescriptorType(std::string_view name) const {
	auto it = m_bindingsByName.find(std::string(name));
	if (it == m_bindingsByName.end()) {
		throw std::runtime_error("Binding not found: " + std::string(name));
	}
	return static_cast<VkDescriptorType>(it->second.type);
}

void VulkanProgram::BindBuffer(
    std::string_view name,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkDeviceSize range,
    uint32_t frameIndex
) {
	if (frameIndex >= m_descriptorSets.size()) {
		throw std::runtime_error("Frame index out of range");
	}
	auto it = m_bindingsByName.find(std::string(name));
	if (it == m_bindingsByName.end()) {
		return;
	}

	VkDescriptorBufferInfo info{};
	info.buffer = buffer;
	info.offset = offset;
	info.range = range == 0 ? VK_WHOLE_SIZE : range;

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_descriptorSets[frameIndex];
	write.dstBinding = it->second.binding;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.descriptorType = static_cast<VkDescriptorType>(it->second.type);
	write.pBufferInfo = &info;

	vkUpdateDescriptorSets(m_device.GetDevice(), 1, &write, 0, nullptr);
}

void VulkanProgram::BindTexture(
    std::string_view name,
    VulkanTexture& texture,
    uint32_t frameIndex,
    uint32_t arrayIndex
) {
	BindTextureView(name, texture.GetImageView(), texture.GetSampler(), frameIndex, arrayIndex);
}

void VulkanProgram::BindTextureView(
    std::string_view name,
    VkImageView view,
    VkSampler sampler,
    uint32_t frameIndex,
    uint32_t arrayIndex
) {
	if (frameIndex >= m_descriptorSets.size())
		throw std::runtime_error("Frame index out of range");

	auto it = m_bindingsByName.find(std::string(name));
	if (it == m_bindingsByName.end())
		return;

	const VkDescriptorType type = static_cast<VkDescriptorType>(it->second.type);

	VkDescriptorImageInfo info{};
	if (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
		info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		info.imageView = view;
		info.sampler = VK_NULL_HANDLE;
	} else {
		info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.imageView = view;
		info.sampler = sampler;
	}

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_descriptorSets[frameIndex];
	write.dstBinding = it->second.binding;
	write.dstArrayElement = arrayIndex;
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pImageInfo = &info;

	vkUpdateDescriptorSets(m_device.GetDevice(), 1, &write, 0, nullptr);
}

void VulkanProgram::CreateDescriptorSetLayout() {
	VkDevice device = m_device.GetDevice();

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	layoutBindings.reserve(m_bindingsInfo.bindings.size());

	for (const auto& b : m_bindingsInfo.bindings) {
		VkDescriptorSetLayoutBinding binding{};
		binding.binding = b.binding;
		binding.descriptorType = static_cast<VkDescriptorType>(b.type);
		binding.descriptorCount = b.count;
		binding.stageFlags = b.stageFlags;
		layoutBindings.push_back(binding);
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutInfo.pBindings = layoutBindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void VulkanProgram::CreateDescriptorPool() {
	VkDevice device = m_device.GetDevice();

	std::unordered_map<VkDescriptorType, uint32_t> poolSizeCounts;
	for (const auto& b : m_bindingsInfo.bindings) {
		VkDescriptorType type = static_cast<VkDescriptorType>(b.type);
		poolSizeCounts[type] += b.count * cMaxFramesInFlight;
	}

	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.reserve(poolSizeCounts.size());
	for (const auto& [type, count] : poolSizeCounts) {
		poolSizes.push_back({ type, count });
	}

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = cMaxFramesInFlight;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool");
	}
}

void VulkanProgram::AllocateDescriptorSets() {
	VkDevice device = m_device.GetDevice();
	m_descriptorSets.resize(cMaxFramesInFlight);

	std::vector<VkDescriptorSetLayout> layouts(cMaxFramesInFlight, m_descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = cMaxFramesInFlight;
	allocInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets");
	}
}

void VulkanProgram::CreatePipelineLayout() {
	VkDevice device = m_device.GetDevice();

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;

	VkPushConstantRange pushRange{};
	if (m_bindingsInfo.pushConstantSize > 0 && m_bindingsInfo.pushConstantStages != 0) {
		pushRange.stageFlags = m_bindingsInfo.pushConstantStages;
		pushRange.offset = 0;
		pushRange.size = m_bindingsInfo.pushConstantSize;

		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushRange;
	}

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}
}

} // namespace PixieRenderer
