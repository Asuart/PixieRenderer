#pragma once
#include <memory>

#include "ResourceHandles.h"

namespace PixieRenderer {

template <typename T> struct ResourceEntry {
	std::unique_ptr<T> resource;
	uint32_t refCount = 0;
};

class IResourceManager {
  public:
	virtual ~IResourceManager() = default;

	virtual void AddRef(uint64_t id) = 0;
	virtual void Release(uint64_t id) = 0;

	static constexpr uint64_t MakeId(ResourceType type, uint32_t index) {
		return (static_cast<uint64_t>(type) << 32) | static_cast<uint64_t>(index);
	}

	static std::pair<ResourceType, uint32_t> DecodeId(uint64_t id) {
		ResourceType type = static_cast<ResourceType>(id >> 32);
		uint32_t index = static_cast<uint32_t>(id & 0xFFFFFFFF);
		return { type, index };
	}
};

} // namespace PixieRenderer
