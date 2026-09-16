#pragma once
#include <vulkan/vulkan.h>

namespace PixieRenderer {

class VulkanDevice;

class VulkanBuffer {
  public:
	VulkanBuffer(
	    VulkanDevice& parentDevice,
	    VkBufferUsageFlags usage,
	    VkMemoryPropertyFlags properties
	);
	VulkanBuffer(
	    VulkanDevice& parentDevice,
	    VkDeviceSize size,
	    VkBufferUsageFlags usage,
	    VkMemoryPropertyFlags properties
	);
	~VulkanBuffer();

	VkBuffer GetBuffer() const;
	VkDeviceSize GetSize() const; 
	void* GetMappedData() const;

	void Load(const void* bufferData, VkDeviceSize size);
	void LoadSubData(const void* bufferData, VkDeviceSize size, VkDeviceSize offset);
	void ReadData(void* outData, VkDeviceSize size, VkDeviceSize offset = 0) const;
	void Free();

  private:
	VulkanDevice& m_device;
	VkDeviceSize m_size = 0;
	VkBufferUsageFlags m_usage = 0;
	VkMemoryPropertyFlags m_properties = 0;
	VkBuffer m_buffer = VK_NULL_HANDLE;
	VkDeviceMemory m_memory = VK_NULL_HANDLE;
	void* m_mappedMemory = nullptr;

	void Resize(VkDeviceSize size);
	void Flush(VkDeviceSize size, VkDeviceSize offset) const;
	void Invalidate(VkDeviceSize size, VkDeviceSize offset) const;
};

} // namespace PixieRenderer
