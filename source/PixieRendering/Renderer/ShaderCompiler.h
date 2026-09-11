#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>

namespace PixieRenderer {

struct ShaderBinding {
	std::string name = "";
	uint32_t type = UINT32_MAX;
	uint32_t binding = UINT32_MAX;
	uint32_t set = UINT32_MAX;
	uint32_t size = UINT32_MAX;
	uint32_t count = UINT32_MAX;
	uint32_t stageFlags = UINT32_MAX;

	ShaderBinding() = default;

	ShaderBinding(
	    const std::string& _name,
	    uint32_t _type,
	    uint32_t _binding,
	    uint32_t _set,
	    uint32_t _size,
	    uint32_t _count,
	    uint32_t _stageFlags
	);
};

struct SpirVBinary {
	uint32_t* words;
	int32_t size;
};

struct BindingsInfo {
	std::vector<ShaderBinding> bindings;
	std::vector<uint32_t> uniformBufferBindings;
	uint32_t pushConstantSize = 0;
};

class ShaderCompiler {
	static inline bool s_isInitialized = false;

  public:
	static void Initialize();
	static void Free();
	static bool IsInitialized();

	static std::vector<ShaderBinding>
	MergeBindings(const std::vector<ShaderBinding>& a, const std::vector<ShaderBinding>& b);
};

} // namespace PixieRenderer
