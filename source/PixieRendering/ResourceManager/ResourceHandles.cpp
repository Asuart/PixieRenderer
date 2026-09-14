#include "PixieRendering/pch.h"
#include "ResourceHandles.h"

#include "IResourceManager.h"

namespace PixieRenderer {

template <ResourceType t>
ResourceHandle<t>::ResourceHandle(IResourceManager* mgr, uint64_t id) : m_manager(mgr), m_id(id) {
	if (m_manager && m_id != UINT64_MAX) {
		m_manager->AddRef(m_id);
	}
}

template <ResourceType t>
ResourceHandle<t>::ResourceHandle(const ResourceHandle& other)
    : m_manager(other.m_manager), m_id(other.m_id) {
	if (m_manager && m_id != UINT64_MAX) {
		m_manager->AddRef(m_id);
	}
}

template <ResourceType t>
ResourceHandle<t>::ResourceHandle(ResourceHandle&& other) noexcept
    : m_manager(other.m_manager), m_id(other.m_id) {
	other.m_manager = nullptr;
	other.m_id = UINT64_MAX;
}

template <ResourceType t> ResourceHandle<t>::~ResourceHandle() {
	if (m_manager && m_id != UINT64_MAX) {
		m_manager->Release(m_id);
	}
}

template <ResourceType t>
ResourceHandle<t>& ResourceHandle<t>::operator=(const ResourceHandle& other) {
	if (this != &other) {
		if (m_manager && m_id != UINT64_MAX) {
			m_manager->Release(m_id);
		}
		m_manager = other.m_manager;
		m_id = other.m_id;
		if (m_manager && m_id != UINT64_MAX) {
			m_manager->AddRef(m_id);
		}
	}
	return *this;
}

template <ResourceType t>
ResourceHandle<t>& ResourceHandle<t>::operator=(ResourceHandle&& other) noexcept {
	if (this != &other) {
		if (m_manager && m_id != UINT64_MAX) {
			m_manager->Release(m_id);
		}
		m_manager = other.m_manager;
		m_id = other.m_id;
		other.m_manager = nullptr;
		other.m_id = UINT64_MAX;
	}
	return *this;
}

template <ResourceType t>
ResourceHandle<t>::operator bool() const
{
	return m_id != UINT64_MAX && m_manager;
}

template <ResourceType t> void ResourceHandle<t>::Reset() {
	if (m_manager && m_id != UINT64_MAX) {
		m_manager->Release(m_id);
	}
	m_manager = nullptr;
	m_id = UINT64_MAX;
}

template class ResourceHandle<ResourceType::Texture>;
template class ResourceHandle<ResourceType::Mesh>;
template class ResourceHandle<ResourceType::IMaterial>;
template class ResourceHandle<ResourceType::ComputeProgram>;
template class ResourceHandle<ResourceType::FrameBuffer>;
template class ResourceHandle<ResourceType::ShaderStorageBuffer>;
template class ResourceHandle<ResourceType::UniformBuffer>;

} // namespace PixieRenderer
