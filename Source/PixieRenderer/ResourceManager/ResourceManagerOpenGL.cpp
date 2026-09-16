#include "PixieRenderer/pch.h"
#include "ResourceManagerOpenGL.h"

namespace PixieRenderer {

void ResourceManagerOpenGL::AddRef(uint64_t id) {
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
	case ResourceType::FrameBuffer:
		if (index < m_frameBuffers.size() && m_frameBuffers[index].resource)
			++m_frameBuffers[index].refCount;
		break;
	case ResourceType::Material:
		if (index < m_materials.size() && m_materials[index].resource)
			++m_materials[index].refCount;
		break;
	case ResourceType::ComputeProgram:
		if (index < m_computeShaders.size() && m_computeShaders[index].resource)
			++m_computeShaders[index].refCount;
		break;
	case ResourceType::Buffer:
		if (index < m_buffers.size() && m_buffers[index].resource)
			++m_buffers[index].refCount;
		break;
	default:
		assert(false);
	}
}

void ResourceManagerOpenGL::Release(uint64_t id) {
	auto [type, index] = DecodeId(id);
	switch (type) {
	case ResourceType::Texture:
		if (index < m_textures.size() && m_textures[index].resource) {
			if (--m_textures[index].refCount == 0)
				m_textures[index].resource.reset();
		}
		break;
	case ResourceType::Mesh:
		if (index < m_meshes.size() && m_meshes[index].resource) {
			if (--m_meshes[index].refCount == 0)
				m_meshes[index].resource.reset();
		}
		break;
	case ResourceType::FrameBuffer:
		if (index < m_frameBuffers.size() && m_frameBuffers[index].resource) {
			if (--m_frameBuffers[index].refCount == 0)
				m_frameBuffers[index].resource.reset();
		}
		break;
	case ResourceType::Material:
		if (index < m_materials.size() && m_materials[index].resource) {
			if (--m_materials[index].refCount == 0)
				m_materials[index].resource.reset();
		}
		break;
	case ResourceType::ComputeProgram:
		if (index < m_computeShaders.size() && m_computeShaders[index].resource) {
			if (--m_computeShaders[index].refCount == 0)
				m_computeShaders[index].resource.reset();
		}
		break;
	case ResourceType::Buffer:
		if (index < m_buffers.size() && m_buffers[index].resource) {
			if (--m_buffers[index].refCount == 0)
				m_buffers[index].resource.reset();
		}
		break;
	default:
		assert(false);
	}
}

OpenGLTexture& ResourceManagerOpenGL::GetTexture(TextureHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	assert(
	    type == ResourceType::Texture && index < m_textures.size() && m_textures[index].resource
	);
	return *m_textures[index].resource;
}

OpenGLMesh& ResourceManagerOpenGL::GetMesh(MeshHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	assert(type == ResourceType::Mesh && index < m_meshes.size() && m_meshes[index].resource);
	return *m_meshes[index].resource;
}

OpenGLFrameBuffer& ResourceManagerOpenGL::GetFrameBuffer(FrameBufferHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	assert(
	    type == ResourceType::FrameBuffer && index < m_frameBuffers.size() &&
	    m_frameBuffers[index].resource
	);
	return *m_frameBuffers[index].resource;
}

OpenGLGraphicsProgram& ResourceManagerOpenGL::GetMaterial(MaterialHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	assert(
	    type == ResourceType::Material && index < m_materials.size() && m_materials[index].resource
	);
	return *m_materials[index].resource;
}

OpenGLComputeProgram& ResourceManagerOpenGL::GetComputeProgram(ComputeProgramHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	assert(
	    type == ResourceType::ComputeProgram && index < m_computeShaders.size() &&
	    m_computeShaders[index].resource
	);
	return *m_computeShaders[index].resource;
}

OpenGLBuffer& ResourceManagerOpenGL::GetBuffer(
    BufferHandle handle
) {
	auto [type, index] = DecodeId(handle.GetId());
	assert(
	    type == ResourceType::Buffer && index < m_buffers.size() &&
	    m_buffers[index].resource
	);
	return *m_buffers[index].resource;
}

} // namespace PixieRenderer
