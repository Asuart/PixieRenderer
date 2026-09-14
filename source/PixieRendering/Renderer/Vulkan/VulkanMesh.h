#pragma once
#include <cstdint>

#include <vulkan/vulkan.h>

#include "PixieRendering/Mesh/Mesh.h"
#include "VulkanBuffer.h"

namespace PixieRenderer {

class VulkanDevice;

class VulkanMesh {
  public:
	VulkanMesh(VulkanDevice& parentDevice, const Mesh* mesh);
	~VulkanMesh();

	const VulkanBuffer& GetVertexBuffer() const;
	const VulkanBuffer& GetIndexBuffer() const;
	uint32_t GetIndexCount() const;
	uint32_t GetVertexCount() const;

	void Load(const Mesh* mesh);
	void Free();

  private:
	VulkanDevice& m_device;
	VulkanBuffer m_vertexBuffer;
	VulkanBuffer m_indexBuffer;
	uint32_t m_indexCount = 0;
	uint32_t m_vertexCount = 0;

  public:
	static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
	static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
};

} // namespace PixieRenderer
