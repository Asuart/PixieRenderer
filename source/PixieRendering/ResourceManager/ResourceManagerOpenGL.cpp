#include "ResourceManagerOpenGL.h"

#include <cassert>

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
	case ResourceType::IMaterial:
		if (index < m_materials.size() && m_materials[index].resource)
			++m_materials[index].refCount;
		break;
	case ResourceType::ComputeProgram:
		if (index < m_computeShaders.size() && m_computeShaders[index].resource)
			++m_computeShaders[index].refCount;
		break;
	case ResourceType::ShaderStorageBuffer:
		if (index < m_shaderStorageBuffers.size() && m_shaderStorageBuffers[index].resource)
			++m_shaderStorageBuffers[index].refCount;
		break;
	case ResourceType::UniformBuffer:
		if (index < m_uniformBuffers.size() && m_uniformBuffers[index].resource)
			++m_uniformBuffers[index].refCount;
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
	case ResourceType::IMaterial:
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
	case ResourceType::ShaderStorageBuffer:
		if (index < m_shaderStorageBuffers.size() && m_shaderStorageBuffers[index].resource) {
			if (--m_shaderStorageBuffers[index].refCount == 0)
				m_shaderStorageBuffers[index].resource.reset();
		}
		break;
	case ResourceType::UniformBuffer:
		if (index < m_uniformBuffers.size() && m_uniformBuffers[index].resource) {
			if (--m_uniformBuffers[index].refCount == 0)
				m_uniformBuffers[index].resource.reset();
		}
		break;
	default:
		assert(false);
	}
}

OpenGLTexture& ResourceManagerOpenGL::GetTextureEntry(TextureHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	assert(
	    type == ResourceType::Texture && index < m_textures.size() && m_textures[index].resource
	);
	return *m_textures[index].resource;
}

OpenGLMesh& ResourceManagerOpenGL::GetMeshEntry(MeshHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	assert(type == ResourceType::Mesh && index < m_meshes.size() && m_meshes[index].resource);
	return *m_meshes[index].resource;
}

OpenGLFrameBuffer& ResourceManagerOpenGL::GetFrameBufferEntry(FrameBufferHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	assert(
	    type == ResourceType::FrameBuffer && index < m_frameBuffers.size() &&
	    m_frameBuffers[index].resource
	);
	return *m_frameBuffers[index].resource;
}

OpenGLGraphicsProgram& ResourceManagerOpenGL::GetMaterialEntry(MaterialHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	assert(
	    type == ResourceType::IMaterial && index < m_materials.size() && m_materials[index].resource
	);
	return *m_materials[index].resource;
}

OpenGLComputeProgram& ResourceManagerOpenGL::GetComputeShaderEntry(ComputeProgramHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	assert(
	    type == ResourceType::ComputeProgram && index < m_computeShaders.size() &&
	    m_computeShaders[index].resource
	);
	return *m_computeShaders[index].resource;
}

OpenGLBuffer& ResourceManagerOpenGL::GetShaderStorageBufferEntry(
    ShaderStorageBufferHandle handle
) {
	auto [type, index] = DecodeId(handle.GetId());
	assert(
	    type == ResourceType::ShaderStorageBuffer && index < m_shaderStorageBuffers.size() &&
	    m_shaderStorageBuffers[index].resource
	);
	return *m_shaderStorageBuffers[index].resource;
}

OpenGLBuffer& ResourceManagerOpenGL::GetUniformBufferEntry(UniformBufferHandle handle) {
	auto [type, index] = DecodeId(handle.GetId());
	assert(
	    type == ResourceType::UniformBuffer && index < m_uniformBuffers.size() &&
	    m_uniformBuffers[index].resource
	);
	return *m_uniformBuffers[index].resource;
}

} // namespace PixieRenderer
