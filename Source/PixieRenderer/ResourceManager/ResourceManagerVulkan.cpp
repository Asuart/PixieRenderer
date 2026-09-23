#include "PixieRenderer/pch.h"
#include "ResourceManagerVulkan.h"

namespace PixieRenderer {

ResourceManagerVulkan::ResourceManagerVulkan(VulkanDevice& device) : m_device(device) {
}

void ResourceManagerVulkan::AddRef(uint64_t id) {
	auto [type, index] = DecodeId(id);
	switch (type) {
	case ResourceType::Texture:
		if (index < m_textures.size() && m_textures[index].resource)
			++m_textures[index].refCount;
		break;
	case ResourceType::Mesh:
		if (index < m_meshes.size() && m_meshes[index].resource)
			++m_meshes[index].refCount;
		break;
	case ResourceType::Material:
		if (index < m_graphicsPrograms.size() && m_graphicsPrograms[index].resource)
			++m_graphicsPrograms[index].refCount;
		break;
	case ResourceType::ComputeProgram:
		if (index < m_computePrograms.size() && m_computePrograms[index].resource)
			++m_computePrograms[index].refCount;
		break;
	case ResourceType::Buffer:
		if (index < m_buffers.size() && m_buffers[index].resource)
			++m_buffers[index].refCount;
		break;
	case ResourceType::FrameBuffer:
		if (index < m_frameBuffers.size() && m_frameBuffers[index].resource)
			++m_frameBuffers[index].refCount;
		break;
	default:
		assert(false);
	}
}

void ResourceManagerVulkan::Release(uint64_t id) {
	auto [type, index] = DecodeId(id);
	switch (type) {
	case ResourceType::Texture:
		if (index < m_textures.size() && m_textures[index].resource) {
			if (--m_textures[index].refCount == 0) {
				m_textures[index].resource.reset();
			}
		}
		break;
	case ResourceType::Mesh:
		if (index < m_meshes.size() && m_meshes[index].resource) {
			if (--m_meshes[index].refCount == 0) {
				m_meshes[index].resource.reset();
			}
		}
		break;
	case ResourceType::Material:
		if (index < m_graphicsPrograms.size() && m_graphicsPrograms[index].resource) {
			if (--m_graphicsPrograms[index].refCount == 0) {
				m_graphicsPrograms[index].resource.reset();
			}
		}
		break;
	case ResourceType::ComputeProgram:
		if (index < m_computePrograms.size() && m_computePrograms[index].resource) {
			if (--m_computePrograms[index].refCount == 0) {
				m_computePrograms[index].resource.reset();
			}
		}
		break;
	case ResourceType::Buffer:
		if (index < m_buffers.size() && m_buffers[index].resource) {
			if (--m_buffers[index].refCount == 0) {
				m_buffers[index].resource.reset();
			}
		}
		break;
	case ResourceType::FrameBuffer:
		if (index < m_frameBuffers.size() && m_frameBuffers[index].resource) {
			if (--m_frameBuffers[index].refCount == 0) {
				m_frameBuffers[index].resource.reset();
			}
		}
		break;
	default:
		assert(false);
	}
}

VulkanTexture* ResourceManagerVulkan::GetTexture(TextureHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	if (index >= m_textures.size()) {
		return nullptr;
	}
	return m_textures[index].resource.get();
}

VulkanMesh* ResourceManagerVulkan::GetMesh(MeshHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	if (index >= m_meshes.size()) {
		return nullptr;
	}
	return m_meshes[index].resource.get();
}

VulkanGraphicsProgram* ResourceManagerVulkan::GetGraphicsProgram(MaterialHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	if (index >= m_graphicsPrograms.size()) {
		return nullptr;
	}
	return m_graphicsPrograms[index].resource.get();
}

VulkanComputeProgram* ResourceManagerVulkan::GetComputeProgram(ComputeProgramHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	if (index >= m_computePrograms.size()) {
		return nullptr;
	}
	return m_computePrograms[index].resource.get();
}

VulkanBuffer* ResourceManagerVulkan::GetBuffer(BufferHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	if (index >= m_buffers.size()) {
		return nullptr;
	}
	return m_buffers[index].resource.get();
}

VulkanFrameBuffer* ResourceManagerVulkan::GetFrameBuffer(FrameBufferHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	if (index >= m_frameBuffers.size()) {
		return nullptr;
	}
	return m_frameBuffers[index].resource.get();
}

std::vector<ResourceEntry<VulkanMesh>>& ResourceManagerVulkan::GetMeshes() {
	return m_meshes;
}

std::vector<ResourceEntry<VulkanGraphicsProgram>>& ResourceManagerVulkan::GetGraphicsPrograms() {
	return m_graphicsPrograms;
}

} // namespace PixieRenderer
