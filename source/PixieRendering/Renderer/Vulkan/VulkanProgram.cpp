#include "VulkanProgram.h"
#include "VulkanConfig.h"
#include "VulkanDevice.h"

#include <iostream>
#include <stdexcept>

namespace PixieRenderer {

VulkanProgram::VulkanProgram(VulkanDevice& device) : m_device(device) {
}

void VulkanProgram::Init(const BindingsInfo& bindingsInfo) {
	m_bindingsInfo = bindingsInfo;
	for (const auto& binding : m_bindingsInfo.bindings) {
		m_nameToBinding[binding.name] = binding.binding;
	}

	CreateDescriptorSetLayout();
	CreateDescriptorPool();
	AllocateDescriptorSets();
	CreateUniformBuffers();
	CreatePipelineLayout();
	UpdateDescriptorSetsForUniforms();
}

VulkanProgram::~VulkanProgram() {
	VkDevice device = m_device.GetDevice();

	m_uniformBuffers.clear();

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

VkDescriptorSetLayout VulkanProgram::GetDescriptorSetLayout() const {
	return m_descriptorSetLayout;
}

VkPipelineLayout VulkanProgram::GetPipelineLayout() const {
	return m_pipelineLayout;
}

const std::vector<VkDescriptorSet>& VulkanProgram::GetDescriptorSets() const {
	return m_descriptorSets;
}

const std::unordered_map<uint32_t, std::vector<VulkanBuffer>>& VulkanProgram::GetUniformBuffers(
) const {
	return m_uniformBuffers;
}

VulkanBuffer* VulkanProgram::GetUniformBuffer(const std::string& name, uint32_t frameIndex) {
	auto it = m_nameToBinding.find(name);
	if (it == m_nameToBinding.end()) {
		throw std::runtime_error("Uniform binding not found: " + name);
	}
	uint32_t binding = it->second;
	auto bufIt = m_uniformBuffers.find(binding);
	if (bufIt == m_uniformBuffers.end()) {
		throw std::runtime_error("Uniform buffer not found for binding");
	}
	auto& buffers = bufIt->second;
	if (frameIndex >= buffers.size()) {
		throw std::runtime_error("Frame index out of range");
	}
	return &buffers[frameIndex];
}

uint32_t VulkanProgram::GetBindingIndex(const std::string& name) const {
	auto it = m_nameToBinding.find(name);
	if (it == m_nameToBinding.end()) {
		throw std::runtime_error("Binding not found: " + name);
	}
	return it->second;
}

void VulkanProgram::BindTexture(
    const std::string& name,
    TextureHandle /*handle*/,
    VulkanTexture& texture,
    uint32_t frameIndex,
    uint32_t index
) {
	VkDevice device = m_device.GetDevice();
	uint32_t binding = GetBindingIndex(name);

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = texture.GetImageView();
	imageInfo.sampler = texture.GetSampler();

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_descriptorSets[frameIndex];
	write.dstBinding = binding;
	write.dstArrayElement = index;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

void VulkanProgram::UpdateUniformBuffer(
    uint32_t binding,
    uint32_t frameIndex,
    const void* data,
    size_t size,
    size_t offset
) {
	auto it = m_uniformBuffers.find(binding);
	if (it == m_uniformBuffers.end()) {
		throw std::runtime_error("Uniform buffer binding not found");
	}
	auto& buffers = it->second;
	if (frameIndex >= buffers.size()) {
		throw std::runtime_error("Frame index out of range");
	}
	buffers[frameIndex].LoadSubData(data, size, offset);
}

void VulkanProgram::CreateDescriptorSetLayout() {
	VkDevice device = m_device.GetDevice();

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
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

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_descriptorSetLayout) !=
	    VK_SUCCESS) {
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

void VulkanProgram::CreateUniformBuffers() {
	for (const auto& b : m_bindingsInfo.bindings) {
		if (b.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
		    b.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
			uint32_t binding = b.binding;
			VkDeviceSize blockSize = b.size;

			std::vector<VulkanBuffer> buffers;
			buffers.reserve(cMaxFramesInFlight);
			for (uint32_t frame = 0; frame < cMaxFramesInFlight; ++frame) {
				buffers.emplace_back(
				    m_device,
				    blockSize,
				    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				);
			}
			m_uniformBuffers[binding] = std::move(buffers);
		}
	}
}

void VulkanProgram::UpdateDescriptorSetsForUniforms() {
	VkDevice device = m_device.GetDevice();

	for (uint32_t frame = 0; frame < cMaxFramesInFlight; ++frame) {
		std::vector<VkWriteDescriptorSet> writes;
		std::vector<VkDescriptorBufferInfo> bufferInfos;
		bufferInfos.reserve(m_uniformBuffers.size());

		for (const auto& [binding, buffers] : m_uniformBuffers) {
			const VulkanBuffer& buffer = buffers[frame];

			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = buffer.GetBuffer();
			bufferInfo.offset = 0;
			bufferInfo.range = buffer.GetSize();
			bufferInfos.push_back(bufferInfo);

			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = m_descriptorSets[frame];
			write.dstBinding = binding;
			write.dstArrayElement = 0;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.pBufferInfo = &bufferInfos.back();
			writes.push_back(write);
		}

		if (!writes.empty()) {
			vkUpdateDescriptorSets(
			    device,
			    static_cast<uint32_t>(writes.size()),
			    writes.data(),
			    0,
			    nullptr
			);
		}
	}
}

void VulkanProgram::CreatePipelineLayout() {
	VkDevice device = m_device.GetDevice();

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;

	if (m_bindingsInfo.pushConstantSize > 0) {
		VkPushConstantRange pushRange{};
		pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT |
		                       VK_SHADER_STAGE_COMPUTE_BIT;
		pushRange.offset = 0;
		pushRange.size = m_bindingsInfo.pushConstantSize;

		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushRange;
	}

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) !=
	    VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}
}

} // namespace PixieRenderer
