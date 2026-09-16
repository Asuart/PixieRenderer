#pragma once
#include "IResourceManager.h"

#include <vector>

#include "PixieRenderer/Renderer/OpenGL/OpenGLComputeProgram.h"
#include "PixieRenderer/Renderer/OpenGL/OpenGLFrameBuffer.h"
#include "PixieRenderer/Renderer/OpenGL/OpenGLGraphicsProgram.h"
#include "PixieRenderer/Renderer/OpenGL/OpenGLMesh.h"
#include "PixieRenderer/Renderer/OpenGL/OpenGLBuffer.h"
#include "PixieRenderer/Renderer/OpenGL/OpenGLTexture.h"

namespace PixieRenderer {

class ResourceManagerOpenGL : public IResourceManager {
  public:
	ResourceManagerOpenGL() = default;

	template <typename... Args> MeshHandle CreateMesh(Args&&... args) {
		uint32_t index = static_cast<uint32_t>(m_meshes.size());
		m_meshes.emplace_back();
		m_meshes.back().resource = std::make_unique<OpenGLMesh>(std::forward<Args>(args)...);
		m_meshes.back().refCount = 0;
		uint64_t id = MakeId(ResourceType::Mesh, index);
		return MeshHandle(this, id);
	}

	template <typename... Args> TextureHandle CreateTexture(Args&&... args) {
		uint32_t index = static_cast<uint32_t>(m_textures.size());
		m_textures.emplace_back();
		m_textures.back().resource = std::make_unique<OpenGLTexture>(std::forward<Args>(args)...);
		m_textures.back().refCount = 0;
		uint64_t id = MakeId(ResourceType::Texture, index);
		return TextureHandle(this, id);
	}

	template <typename... Args> FrameBufferHandle CreateFrameBuffer(Args&&... args) {
		uint32_t index = static_cast<uint32_t>(m_frameBuffers.size());
		m_frameBuffers.emplace_back();
		m_frameBuffers.back().resource = std::make_unique<OpenGLFrameBuffer>(std::forward<Args>(args
		)...);
		m_frameBuffers.back().refCount = 0;
		uint64_t id = MakeId(ResourceType::FrameBuffer, index);
		return FrameBufferHandle(this, id);
	}

	template <typename... Args> MaterialHandle CreateMaterial(Args&&... args) {
		uint32_t index = static_cast<uint32_t>(m_materials.size());
		m_materials.emplace_back();
		m_materials.back().resource = std::make_unique<OpenGLGraphicsProgram>(std::forward<Args>(args)...);
		m_materials.back().refCount = 0;
		uint64_t id = MakeId(ResourceType::Material, index);
		return MaterialHandle(this, id);
	}

	template <typename... Args> ComputeProgramHandle CreateComputeProgram(Args&&... args) {
		uint32_t index = static_cast<uint32_t>(m_computeShaders.size());
		m_computeShaders.emplace_back();
		m_computeShaders.back().resource = std::make_unique<OpenGLComputeProgram>(
		    std::forward<Args>(args)...
		);
		m_computeShaders.back().refCount = 0;
		uint64_t id = MakeId(ResourceType::ComputeProgram, index);
		return ComputeProgramHandle(this, id);
	}

	template <typename... Args>
	BufferHandle CreateBuffer(Args&&... args) {
		uint32_t index = static_cast<uint32_t>(m_buffers.size());
		m_buffers.emplace_back();
		m_buffers.back().resource = std::make_unique<OpenGLBuffer>(
		    std::forward<Args>(args)...
		);
		m_buffers.back().refCount = 0;
		uint64_t id = MakeId(ResourceType::Buffer, index);
		return BufferHandle(this, id);
	}

	void AddRef(uint64_t id) override;
	void Release(uint64_t id) override;

	OpenGLTexture& GetTexture(TextureHandle handle);
	OpenGLMesh& GetMesh(MeshHandle handle);
	OpenGLFrameBuffer& GetFrameBuffer(FrameBufferHandle handle);
	OpenGLGraphicsProgram& GetMaterial(MaterialHandle handle);
	OpenGLComputeProgram& GetComputeProgram(ComputeProgramHandle handle);
	OpenGLBuffer& GetBuffer(BufferHandle handle);

	std::vector<ResourceEntry<OpenGLMesh>>& GetMeshes() {
		return m_meshes;
	}

	std::vector<ResourceEntry<OpenGLGraphicsProgram>>& GetMaterials() {
		return m_materials;
	}

  private:
	std::vector<ResourceEntry<OpenGLMesh>> m_meshes;
	std::vector<ResourceEntry<OpenGLTexture>> m_textures;
	std::vector<ResourceEntry<OpenGLFrameBuffer>> m_frameBuffers;
	std::vector<ResourceEntry<OpenGLGraphicsProgram>> m_materials;
	std::vector<ResourceEntry<OpenGLComputeProgram>> m_computeShaders;
	std::vector<ResourceEntry<OpenGLBuffer>> m_buffers;
};

} // namespace PixieRenderer
