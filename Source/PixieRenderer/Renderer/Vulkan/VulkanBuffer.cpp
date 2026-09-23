#include "VulkanBuffer.h"
#include "PixieRenderer/pch.h"

#include <vulkan/vk_enum_string_helper.h>

#include "PixieRenderer/LogCategories.h"
#include "VulkanDevice.h"

namespace PixieRenderer {

VulkanBuffer::VulkanBuffer(VulkanDevice& parentDevice, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    : m_device(parentDevice), m_usage(usage), m_properties(properties) {
}

VulkanBuffer::VulkanBuffer(
    VulkanDevice& parentDevice,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties
)
    : m_device(parentDevice), m_usage(usage), m_properties(properties) {
	if (!Resize(size)) {
		Log::Error(LogCat::vkBuffer, "VulkanBuffer: Resize failed.");
	}
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
		Log::Error(LogCat::vkBuffer, "Load: nullptr is passed as source.");
		return;
	}

	if (size == 0) {
		return;
	}

	if (m_size != size && !Resize(size)) {
		Log::Error(LogCat::vkBuffer, "Load: Resize failed.");
		return;
	}

	if (m_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
		if (m_mappedMemory == nullptr) {
			Log::Error(LogCat::vkBuffer, "Load: HOST_VISIBLE buffer is not mapped");
			return;
		}
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
		Log::Error(LogCat::vkBuffer, "LoadSubData: nullptr is passed as source.");
		return;
	}
	if (size == 0) {
		return;
	}
	if (offset > m_size || size > m_size - offset) {
		Log::Error(LogCat::vkBuffer, "LoadSubData: Load range exceeds buffer size. No data is written.");
		return;
	}

	if (m_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
		if (m_mappedMemory == nullptr) {
			Log::Error(LogCat::vkBuffer, "LoadSubData: HOST_VISIBLE buffer is not mapped");
			return;
		}
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
		Log::Error(LogCat::vkBuffer, "ReadData: nullptr is passed as destination.");
		return;
	}
	if (size == 0) {
		return;
	}
	if (!m_mappedMemory) {
		Log::Error(LogCat::vkBuffer, "ReadData: Buffer not mapped");
		return;
	}
	if (offset > m_size || size > m_size - offset) {
		Log::Error(LogCat::vkBuffer, "ReadData: Read out of bounds. No data copied.");
		return;
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

bool VulkanBuffer::Resize(VkDeviceSize size) {
	if (size == m_size && m_buffer != VK_NULL_HANDLE && m_memory != VK_NULL_HANDLE) {
		return true;
	}
	if (size == 0) {
		Log::Error(LogCat::vkBuffer, "Resize: size must be > 0");
		return false;
	}

	VkDevice device = m_device.GetDevice();

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = m_usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer newBuffer = VK_NULL_HANDLE;
	VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &newBuffer);
	if (result != VK_SUCCESS) {
		Log::Error(LogCat::vkBuffer, "Resize: vkCreateBuffer failed: {}", string_VkResult(result));
		return false;
	}

	VkMemoryRequirements memReq{};
	vkGetBufferMemoryRequirements(device, newBuffer, &memReq);

	const uint32_t memoryTypeIndex = m_device.FindMemoryType(memReq.memoryTypeBits, m_properties);
	if (memoryTypeIndex == VulkanDevice::kInvalidMemoryType) {
		Log::Error(
		    LogCat::vkBuffer,
		    "Resize: no memory type for bits=0x{:x}, props=0x{:x}",
		    memReq.memoryTypeBits,
		    m_properties
		);
		vkDestroyBuffer(device, newBuffer, nullptr);
		return false;
	}

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	VkDeviceMemory newMemory = VK_NULL_HANDLE;
	result = vkAllocateMemory(device, &allocInfo, nullptr, &newMemory);
	if (result != VK_SUCCESS) {
		Log::Error(LogCat::vkBuffer, "Resize: vkAllocateMemory failed: {}", string_VkResult(result));
		vkDestroyBuffer(device, newBuffer, nullptr);
		return false;
	}

	result = vkBindBufferMemory(device, newBuffer, newMemory, 0);
	if (result != VK_SUCCESS) {
		Log::Error(LogCat::vkBuffer, "Resize: vkBindBufferMemory failed: {}", string_VkResult(result));
		vkFreeMemory(device, newMemory, nullptr);
		vkDestroyBuffer(device, newBuffer, nullptr);
		return false;
	}

	void* newMapped = nullptr;
	const bool needsMapping = (m_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
	if (needsMapping) {
		result = vkMapMemory(device, newMemory, 0, VK_WHOLE_SIZE, 0, &newMapped);
		if (result != VK_SUCCESS) {
			Log::Error(LogCat::vkBuffer, "Resize: vkMapMemory failed: {}", string_VkResult(result));
			vkFreeMemory(device, newMemory, nullptr);
			vkDestroyBuffer(device, newBuffer, nullptr);
			return false;
		}
	}

	Free();

	m_buffer = newBuffer;
	m_memory = newMemory;
	m_mappedMemory = newMapped;
	m_size = size;

	return true;
}

void VulkanBuffer::Flush(VkDeviceSize size, VkDeviceSize offset) const {
	if (!(m_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) && m_mappedMemory != nullptr) {
		VkMappedMemoryRange range{};
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = m_memory;
		range.offset = offset;
		range.size = size == VK_WHOLE_SIZE ? m_size - offset : size;

		VkResult r = vkFlushMappedMemoryRanges(m_device.GetDevice(), 1, &range);
		if (r != VK_SUCCESS) {
			Log::Error(LogCat::vkBuffer, "vkFlushMappedMemoryRanges failed: {}", string_VkResult(r));
		}
	}
}

void VulkanBuffer::Invalidate(VkDeviceSize size, VkDeviceSize offset) const {
	if (!(m_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) && m_mappedMemory != nullptr) {
		VkMappedMemoryRange range{};
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = m_memory;
		range.offset = offset;
		range.size = size == VK_WHOLE_SIZE ? m_size - offset : size;

		VkResult r = vkInvalidateMappedMemoryRanges(m_device.GetDevice(), 1, &range);
		if (r != VK_SUCCESS) {
			Log::Error(LogCat::vkBuffer, "vkInvalidateMappedMemoryRanges failed: {}", string_VkResult(r));
		}
	}
}

} // namespace PixieRenderer
