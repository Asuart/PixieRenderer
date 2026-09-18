#include "PixieRenderer/pch.h"
#include "ShaderCompiler.h"

namespace PixieRenderer {

ShaderBinding::ShaderBinding(
    const std::string& _name,
    uint32_t _type,
    uint32_t _binding,
    uint32_t _set,
    uint32_t _size,
    uint32_t _count,
    uint32_t _stageFlags
)
    : name(_name),
      type(_type),
      binding(_binding),
      set(_set),
      size(_size),
      count(_count),
      stageFlags(_stageFlags) {
}

void ShaderCompiler::Initialize() {
	if (!s_isInitialized) {
		glslang_initialize_process();
		s_isInitialized = true;
	}
}

void ShaderCompiler::Free() {
	if (s_isInitialized) {
		s_isInitialized = false;
		glslang_finalize_process();
	}
}

bool ShaderCompiler::IsInitialized() {
	return s_isInitialized;
}

std::vector<ShaderBinding> ShaderCompiler::MergeBindings(
    const std::vector<ShaderBinding>& a,
    const std::vector<ShaderBinding>& b
) {
	std::unordered_map<uint64_t, ShaderBinding> map;

	auto add = [&](const ShaderBinding& src) {
		uint64_t key = (static_cast<uint64_t>(src.set) << 32) | src.binding;
		auto it = map.find(key);
		if (it == map.end()) {
			map[key] = src;
		} else {
			if (it->second.type != src.type) {
				throw std::runtime_error(
				    "Binding type mismatch for set " + std::to_string(src.set) + " binding " +
				    std::to_string(src.binding)
				);
			}
			if (it->second.count != src.count) {
				throw std::runtime_error(
				    "Binding count mismatch for set " + std::to_string(src.set) + " binding " +
				    std::to_string(src.binding)
				);
			}
			if (it->second.size != src.size) {
				throw std::runtime_error(
				    "Block size mismatch for set " + std::to_string(src.set) + " binding " +
				    std::to_string(src.binding)
				);
			}
			it->second.stageFlags |= src.stageFlags;
		}
	};

	for (const auto& s : a) {
		add(s);
	}
	for (const auto& s : b) {
		add(s);
	}

	std::vector<ShaderBinding> result;
	result.reserve(map.size());
	for (auto& [key, binding] : map) {
		result.push_back(binding);
	}

	std::sort(result.begin(), result.end(), [](const ShaderBinding& lhs, const ShaderBinding& rhs) {
		return lhs.binding < rhs.binding;
	});

	return result;
}

} // namespace PixieRenderer
