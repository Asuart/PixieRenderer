#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace PixieRenderer {

enum class ShaderStage : uint32_t {
	Vertex,
	Fragment,
	Geometry,
	Compute,
};

using ShaderStageMask = uint32_t;

namespace ShaderStageBit {
constexpr ShaderStageMask None = 0u;
constexpr ShaderStageMask Vertex = 1u << 0;
constexpr ShaderStageMask Fragment = 1u << 1;
constexpr ShaderStageMask Geometry = 1u << 2;
constexpr ShaderStageMask Compute = 1u << 3;
} // namespace ShaderStageBit

inline constexpr ShaderStageMask ToMask(ShaderStage s) {
	switch (s) {
	case ShaderStage::Vertex:
		return ShaderStageBit::Vertex;
	case ShaderStage::Fragment:
		return ShaderStageBit::Fragment;
	case ShaderStage::Geometry:
		return ShaderStageBit::Geometry;
	case ShaderStage::Compute:
		return ShaderStageBit::Compute;
	}
	return ShaderStageBit::None;
}

enum class DescriptorType : uint32_t {
	Sampler = 0,
	CombinedImageSampler,
	SampledImage,
	StorageImage,
	UniformTexelBuffer,
	StorageTexelBuffer,
	UniformBuffer,
	StorageBuffer,
	UniformBufferDynamic,
	StorageBufferDynamic,
	InputAttachment,
	Unknown,
};

struct ShaderBinding {
	std::string name;
	DescriptorType type = DescriptorType::Unknown;
	uint32_t binding = UINT32_MAX;
	uint32_t set = UINT32_MAX;
	uint32_t size = UINT32_MAX;
	uint32_t count = UINT32_MAX;
	ShaderStageMask stageFlags = ShaderStageBit::None;

	ShaderBinding() = default;

	ShaderBinding(
	    const std::string& _name,
	    DescriptorType _type,
	    uint32_t _binding,
	    uint32_t _set,
	    uint32_t _size,
	    uint32_t _count,
	    ShaderStageMask _stageFlags
	);
};

struct SpirVBinary {
	uint32_t* words = nullptr;
	int32_t size = 0;
};

struct BindingsInfo {
	std::vector<ShaderBinding> bindings;
	std::vector<uint32_t> uniformBufferBindings;
	uint32_t pushConstantSize = 0;
	ShaderStageMask pushConstantStages = ShaderStageBit::None;
};

class ShaderCompiler {
	static inline bool s_isInitialized = false;

  public:
	static void Initialize();
	static void Free();
	static bool IsInitialized();

	static SpirVBinary CompileToSPIRV(ShaderStage stage, const char* shaderSource);
	static void FreeSPIRV(SpirVBinary& binary);

	static BindingsInfo ReflectSPIRV(const SpirVBinary& binary, ShaderStage stage);

	static std::vector<ShaderBinding> MergeBindings(
	    const std::vector<ShaderBinding>& a,
	    const std::vector<ShaderBinding>& b
	);

	static BindingsInfo MergeBindingsInfo(const BindingsInfo& a, const BindingsInfo& b);
};

} // namespace PixieRenderer
