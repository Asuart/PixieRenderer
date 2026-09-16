#include "PixieRenderer/pch.h"
#include "VulkanBuffer.h"

#include "VulkanDevice.h"

namespace PixieRenderer {

VulkanBuffer::VulkanBuffer(
    VulkanDevice& parentDevice,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties
)
    : m_device(parentDevice), m_usage(usage), m_properties(properties) {
}

VulkanBuffer::VulkanBuffer(
    VulkanDevice& parentDevice,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties
)
    : m_device(parentDevice), m_usage(usage), m_properties(properties) {
	Resize(size);
}

VulkanBuffer::~VulkanBuffer() {
	Free();
}

VkBuffer VulkanBuffer::GetBuffer() const {
	return m_buffer;
}

VkDeviceSize VulkanBuffer::GetSize() const {
	return m_size;
}

void* VulkanBuffer::GetMappedData() const {
	return m_mappedMemory;
}

void VulkanBuffer::Load(const void* bufferData, VkDeviceSize size) {
	if (!bufferData) {
		throw std::runtime_error("Data pointer is null");
	}
	if (size == 0) {
		throw std::runtime_error("Data size is 0");
	}

	if (m_size != size) {
		Resize(size);
	}

	if (m_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
		memcpy(static_cast<char*>(m_mappedMemory), bufferData, static_cast<size_t>(size));
		Flush(size, 0);
		return;
	}

	VulkanBuffer stagingBuffer(
	    m_device,
	    size,
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	memcpy(stagingBuffer.m_mappedMemory, bufferData, static_cast<size_t>(size));

	m_device.CopyBuffer(stagingBuffer.m_buffer, m_buffer, size, 0, 0);
}

void VulkanBuffer::LoadSubData(const void* bufferData, VkDeviceSize size, VkDeviceSize offset) {
	if (!bufferData) {
		throw std::runtime_error("Data pointer is null");
	}
	if (size == 0) {
		throw std::runtime_error("Data size is 0");
	}
	if (offset + size > m_size) {
		throw std::runtime_error("Load range exceeds buffer size");
	}

	if (m_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
		memcpy(static_cast<char*>(m_mappedMemory) + offset, bufferData, static_cast<size_t>(size));
		Flush(size, offset);
		return;
	}

	VulkanBuffer stagingBuffer(
	    m_device,
	    size,
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	memcpy(stagingBuffer.m_mappedMemory, bufferData, static_cast<size_t>(size));

	m_device.CopyBuffer(stagingBuffer.m_buffer, m_buffer, size, 0, offset);
}

void VulkanBuffer::ReadData(void* outData, VkDeviceSize size, VkDeviceSize offset) const {
	if (outData == nullptr) {
		throw std::runtime_error("outData is null");
	}
	if (size == 0) {
		throw std::runtime_error("read size id 0");
	}
	if (!m_mappedMemory) {
		throw std::runtime_error("Buffer not mapped");
	}
	if (offset + size > m_size) {
		throw std::runtime_error("Read out of bounds");
	}

	Invalidate(size, offset);

	memcpy(outData, static_cast<char*>(m_mappedMemory) + offset, static_cast<size_t>(size));
}

void VulkanBuffer::Free() {
	VkDevice device = m_device.GetDevice();
	if (m_mappedMemory != nullptr) {
		vkUnmapMemory(device, m_memory);
		m_mappedMemory = VK_NULL_HANDLE;
	}
	if (m_buffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(device, m_buffer, nullptr);
		m_buffer = VK_NULL_HANDLE;
	}
	if (m_memory != VK_NULL_HANDLE) {
		vkFreeMemory(device, m_memory, nullptr);
		m_memory = VK_NULL_HANDLE;
	}
	m_size = 0;
}

void VulkanBuffer::Resize(VkDeviceSize size) {
	Free();

	VkDevice device = m_device.GetDevice();

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = m_usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, m_buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex =
	    m_device.FindMemoryType(memRequirements.memoryTypeBits, m_properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	if (vkBindBufferMemory(device, m_buffer, m_memory, 0) != VK_SUCCESS) {
		throw std::runtime_error("failed to bind buffer memory!");
	}

	if (m_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
		if (vkMapMemory(device, m_memory, 0, size, 0, &m_mappedMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to map buffer memory!");
		}
	}

	m_size = size;
}

void VulkanBuffer::Flush(VkDeviceSize size, VkDeviceSize offset) const {
	if (!(m_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) && m_mappedMemory != nullptr) {
		VkMappedMemoryRange range{};
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = m_memory;
		range.offset = offset;
		range.size = size == VK_WHOLE_SIZE ? m_size - offset : size;
		vkFlushMappedMemoryRanges(m_device.GetDevice(), 1, &range);
	}
}

void VulkanBuffer::Invalidate(VkDeviceSize size, VkDeviceSize offset) const {
	if (!(m_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) && m_mappedMemory != nullptr) {
		VkMappedMemoryRange range{};
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = m_memory;
		range.offset = offset;
		range.size = size == VK_WHOLE_SIZE ? m_size - offset : size;
		vkInvalidateMappedMemoryRanges(m_device.GetDevice(), 1, &range);
	}
}

} // namespace PixieRenderer
