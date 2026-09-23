#pragma once
#include "IResourceManager.h"

#include <vector>

#include "../Renderer/Vulkan/VulkanBuffer.h"
#include "../Renderer/Vulkan/VulkanComputeProgram.h"
#include "../Renderer/Vulkan/VulkanFrameBuffer.h"
#include "../Renderer/Vulkan/VulkanGraphicsProgram.h"
#include "../Renderer/Vulkan/VulkanMesh.h"
#include "../Renderer/Vulkan/VulkanRenderPass.h"
#include "../Renderer/Vulkan/VulkanTexture.h"

namespace PixieRenderer {

class ResourceManagerVulkan : public IResourceManager {
  public:
	explicit ResourceManagerVulkan(VulkanDevice& device);

	template <typename... Args> MeshHandle CreateMesh(Args&&... args) {
		uint32_t index = static_cast<uint32_t>(m_meshes.size());
		m_meshes.emplace_back();
		m_meshes.back()
		    .resource = std::make_unique<VulkanMesh>(m_device, std::forward<Args>(args)...);
		m_meshes.back().refCount = 0; 
		uint64_t id = MakeId(ResourceType::Mesh, index);
		return MeshHandle(this, id);
	}

	template <typename... Args> TextureHandle CreateTexture(Args&&... args) {
		uint32_t index = static_cast<uint32_t>(m_textures.size());
		m_textures.emplace_back();
		m_textures.back()
		    .resource = std::make_unique<VulkanTexture>(m_device, std::forward<Args>(args)...);
		m_textures.back().refCount = 0;
		uint64_t id = MakeId(ResourceType::Texture, index);
		return TextureHandle(this, id);
	}

	template <typename... Args> MaterialHandle CreateGraphicsProgram(Args&&... args) {
		uint32_t index = static_cast<uint32_t>(m_graphicsPrograms.size());
		m_graphicsPrograms.emplace_back();
		m_graphicsPrograms.back().resource = std::make_unique<
		    VulkanGraphicsProgram>(m_device, std::forward<Args>(args)...);
		m_graphicsPrograms.back().refCount = 0;
		uint64_t id = MakeId(ResourceType::Material, index);
		return MaterialHandle(this, id);
	}

	template <typename... Args> ComputeProgramHandle CreateComputeProgram(Args&&... args) {
		uint32_t index = static_cast<uint32_t>(m_computePrograms.size());
		m_computePrograms.emplace_back();
		m_computePrograms.back().resource = std::make_unique<
		    VulkanComputeProgram>(m_device, std::forward<Args>(args)...);
		m_computePrograms.back().refCount = 0;
		uint64_t id = MakeId(ResourceType::ComputeProgram, index);
		return ComputeProgramHandle(this, id);
	}

	template <typename... Args> BufferHandle CreateBuffer(Args&&... args) {
		uint32_t index = static_cast<uint32_t>(m_buffers.size());
		m_buffers.emplace_back();
		m_buffers.back()
		    .resource = std::make_unique<VulkanBuffer>(m_device, std::forward<Args>(args)...);
		m_buffers.back().refCount = 0;
		uint64_t id = MakeId(ResourceType::Buffer, index);
		return BufferHandle(this, id);
	}

	template <typename... Args> FrameBufferHandle CreateFrameBuffer(Args&&... args) {
		uint32_t index = static_cast<uint32_t>(m_frameBuffers.size());
		m_frameBuffers.emplace_back();
		m_frameBuffers.back()
		    .resource = std::make_unique<VulkanFrameBuffer>(m_device, std::forward<Args>(args)...);
		m_frameBuffers.back().refCount = 0;
		uint64_t id = MakeId(ResourceType::FrameBuffer, index);
		return FrameBufferHandle(this, id);
	}

	void AddRef(uint64_t id) override;
	void Release(uint64_t id) override;

	VulkanTexture* GetTexture(TextureHandle handle);
	VulkanMesh* GetMesh(MeshHandle handle);
	VulkanGraphicsProgram* GetGraphicsProgram(MaterialHandle handle);
	VulkanComputeProgram* GetComputeProgram(ComputeProgramHandle handle);
	VulkanBuffer* GetBuffer(BufferHandle handle);
	VulkanFrameBuffer* GetFrameBuffer(FrameBufferHandle handle);

	std::vector<ResourceEntry<VulkanMesh>>& GetMeshes();
	std::vector<ResourceEntry<VulkanGraphicsProgram>>& GetGraphicsPrograms();

  private:
	VulkanDevice& m_device;

	std::vector<ResourceEntry<VulkanMesh>> m_meshes;
	std::vector<ResourceEntry<VulkanTexture>> m_textures;
	std::vector<ResourceEntry<VulkanGraphicsProgram>> m_graphicsPrograms;
	std::vector<ResourceEntry<VulkanComputeProgram>> m_computePrograms;
	std::vector<ResourceEntry<VulkanBuffer>> m_buffers;
	std::vector<ResourceEntry<VulkanFrameBuffer>> m_frameBuffers;
};

} // namespace PixieRenderer
