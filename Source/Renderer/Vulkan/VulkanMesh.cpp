#include "PixieRendering/pch.h"
#include "VulkanMesh.h"

#include "VulkanDevice.h"

namespace PixieRenderer {

VulkanMesh::VulkanMesh(VulkanDevice& parentDevice, const Mesh* mesh)
    : m_device(parentDevice),
      m_vertexBuffer(
          parentDevice,
          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
      ),
      m_indexBuffer(
          parentDevice,
          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
      ) {
	if (mesh != nullptr) {
		Load(mesh);
	}
}

VulkanMesh::~VulkanMesh() {
}

const VulkanBuffer& VulkanMesh::GetVertexBuffer() const {
	return m_vertexBuffer;
}

const VulkanBuffer& VulkanMesh::GetIndexBuffer() const {
	return m_indexBuffer;
}

uint32_t VulkanMesh::GetIndexCount() const {
	return m_indexCount;
}

uint32_t VulkanMesh::GetVertexCount() const {
	return m_vertexCount;
}

void VulkanMesh::Load(const Mesh* mesh) {
	if (!mesh) {
		throw std::runtime_error("Mesh pointer is null");
	}
	m_vertexBuffer.Load(mesh->vertexes.data(), mesh->GetVertexBufferSize());
	m_indexBuffer.Load(mesh->indexes.data(), mesh->GetIndexBufferSize());
	m_indexCount = static_cast<uint32_t>(mesh->indexes.size());
	m_vertexCount = static_cast<uint32_t>(mesh->vertexes.size());
}

void VulkanMesh::Free() {
	m_vertexBuffer.Free();
	m_indexBuffer.Free();
	m_indexCount = 0;
	m_vertexCount = 0;
}

std::vector<VkVertexInputBindingDescription> VulkanMesh::GetBindingDescriptions() {
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return { bindingDescription };
}

std::vector<VkVertexInputAttributeDescription> VulkanMesh::GetAttributeDescriptions() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, normal);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, uv);

	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SINT;
	attributeDescriptions[3].offset = offsetof(Vertex, boneIDs);

	attributeDescriptions[4].binding = 0;
	attributeDescriptions[4].location = 4;
	attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[4].offset = offsetof(Vertex, boneWeights);

	return attributeDescriptions;
}

} // namespace PixieRenderer
