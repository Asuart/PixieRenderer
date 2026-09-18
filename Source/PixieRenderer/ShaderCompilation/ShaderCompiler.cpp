#include "ShaderCompiler.h"
#include "PixieRenderer/pch.h"

#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>
#include <spirv_reflect.h>

namespace PixieRenderer {

namespace {

glslang_stage_t ToGlslangStage(ShaderStage stage) {
	switch (stage) {
	case ShaderStage::Vertex:
		return GLSLANG_STAGE_VERTEX;
	case ShaderStage::Fragment:
		return GLSLANG_STAGE_FRAGMENT;
	case ShaderStage::Geometry:
		return GLSLANG_STAGE_GEOMETRY;
	case ShaderStage::Compute:
		return GLSLANG_STAGE_COMPUTE;
	}
	return GLSLANG_STAGE_VERTEX;
}

DescriptorType FromReflectDescriptorType(SpvReflectDescriptorType t) {
	switch (t) {
	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
		return DescriptorType::Sampler;
	case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		return DescriptorType::CombinedImageSampler;
	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		return DescriptorType::SampledImage;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		return DescriptorType::StorageImage;
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		return DescriptorType::UniformTexelBuffer;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
		return DescriptorType::StorageTexelBuffer;
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		return DescriptorType::UniformBuffer;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		return DescriptorType::StorageBuffer;
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		return DescriptorType::UniformBufferDynamic;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
		return DescriptorType::StorageBufferDynamic;
	case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
		return DescriptorType::InputAttachment;
	}
	return DescriptorType::Unknown;
}

bool IsUniformBuffer(DescriptorType t) {
	return t == DescriptorType::UniformBuffer || t == DescriptorType::UniformBufferDynamic;
}

} // namespace

ShaderBinding::ShaderBinding(
    const std::string& _name,
    DescriptorType _type,
    uint32_t _binding,
    uint32_t _set,
    uint32_t _size,
    uint32_t _count,
    ShaderStageMask _stageFlags
)
    : name(_name), type(_type), binding(_binding), set(_set), size(_size), count(_count), stageFlags(_stageFlags) {
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

SpirVBinary ShaderCompiler::CompileToSPIRV(ShaderStage stage, const char* shaderSource) {
	SpirVBinary bin{};
	if (!shaderSource) {
		return bin;
	}

	glslang_input_t input = {
		.language = GLSLANG_SOURCE_GLSL,
		.stage = ToGlslangStage(stage),
		.client = GLSLANG_CLIENT_VULKAN,
		.client_version = GLSLANG_TARGET_VULKAN_1_0,
		.target_language = GLSLANG_TARGET_SPV,
		.target_language_version = GLSLANG_TARGET_SPV_1_0,
		.code = shaderSource,
		.default_version = 100,
		.default_profile = GLSLANG_NO_PROFILE,
		.force_default_version_and_profile = false,
		.forward_compatible = false,
		.messages = GLSLANG_MSG_DEFAULT_BIT,
		.resource = glslang_default_resource(),
	};

	glslang_shader_t* shader = glslang_shader_create(&input);
	if (!shader) {
		return bin;
	}

	if (!glslang_shader_preprocess(shader, &input)) {
		printf(
		    "GLSL preprocessing failed.\n%s\n%s\n",
		    glslang_shader_get_info_log(shader),
		    glslang_shader_get_info_debug_log(shader)
		);
		glslang_shader_delete(shader);
		return bin;
	}

	if (!glslang_shader_parse(shader, &input)) {
		printf(
		    "GLSL parsing failed.\n%s\n%s\n",
		    glslang_shader_get_info_log(shader),
		    glslang_shader_get_info_debug_log(shader)
		);
		glslang_shader_delete(shader);
		return bin;
	}

	glslang_program_t* program = glslang_program_create();
	glslang_program_add_shader(program, shader);

	if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
		printf(
		    "GLSL linking failed.\n%s\n%s\n",
		    glslang_program_get_info_log(program),
		    glslang_program_get_info_debug_log(program)
		);
		glslang_program_delete(program);
		glslang_shader_delete(shader);
		return bin;
	}

	glslang_program_SPIRV_generate(program, input.stage);

	bin.size = static_cast<int32_t>(glslang_program_SPIRV_get_size(program));
	bin.words = new uint32_t[bin.size];
	glslang_program_SPIRV_get(program, bin.words);

	if (const char* msgs = glslang_program_SPIRV_get_messages(program)) {
		printf("%s\n", msgs);
	}

	glslang_program_delete(program);
	glslang_shader_delete(shader);
	return bin;
}

void ShaderCompiler::FreeSPIRV(SpirVBinary& binary) {
	delete[] binary.words;
	binary.words = nullptr;
	binary.size = 0;
}

BindingsInfo ShaderCompiler::ReflectSPIRV(const SpirVBinary& binary, ShaderStage stage) {
	BindingsInfo result;
	if (!binary.words || binary.size == 0) {
		return result;
	}

	SpvReflectShaderModule module{};
	SpvReflectResult
	    r = spvReflectCreateShaderModule(static_cast<size_t>(binary.size) * sizeof(uint32_t), binary.words, &module);
	if (r != SPV_REFLECT_RESULT_SUCCESS) {
		return result;
	}

	const ShaderStageMask stageMask = ToMask(stage);

	uint32_t bindingCount = 0;
	spvReflectEnumerateDescriptorBindings(&module, &bindingCount, nullptr);
	std::vector<SpvReflectDescriptorBinding*> bindings(bindingCount);
	spvReflectEnumerateDescriptorBindings(&module, &bindingCount, bindings.data());

	for (const SpvReflectDescriptorBinding* b : bindings) {
		const DescriptorType type = FromReflectDescriptorType(b->descriptor_type);

		uint32_t blockSize = 0;
		if (type == DescriptorType::UniformBuffer || type == DescriptorType::StorageBuffer) {
			blockSize = static_cast<uint32_t>(b->block.size);
		}

		const uint32_t arrayCount = (b->count > 0) ? b->count : 1;

		result.bindings.push_back(
		    ShaderBinding(b->name ? b->name : "", type, b->binding, b->set, blockSize, arrayCount, stageMask)
		);

		if (IsUniformBuffer(type)) {
			result.uniformBufferBindings.push_back(b->binding);
		}
	}

	uint32_t pcCount = 0;
	spvReflectEnumeratePushConstantBlocks(&module, &pcCount, nullptr);
	if (pcCount > 0) {
		std::vector<SpvReflectBlockVariable*> pcs(pcCount);
		spvReflectEnumeratePushConstantBlocks(&module, &pcCount, pcs.data());
		for (const SpvReflectBlockVariable* pc : pcs) {
			if (pc->size > result.pushConstantSize) {
				result.pushConstantSize = pc->size;
			}
		}
		result.pushConstantStages |= stageMask;
	}

	spvReflectDestroyShaderModule(&module);
	return result;
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
				    "Block size mismatch for set " + std::to_string(src.set) + " binding " + std::to_string(src.binding)
				);
			}
			it->second.stageFlags |= src.stageFlags;
		}
	};

	for (const auto& s : a)
		add(s);
	for (const auto& s : b)
		add(s);

	std::vector<ShaderBinding> result;
	result.reserve(map.size());
	for (auto& [key, binding] : map) {
		result.push_back(std::move(binding));
	}

	std::sort(result.begin(), result.end(), [](const ShaderBinding& lhs, const ShaderBinding& rhs) {
		return lhs.binding < rhs.binding;
	});

	return result;
}

BindingsInfo ShaderCompiler::MergeBindingsInfo(const BindingsInfo& a, const BindingsInfo& b) {
	BindingsInfo result;
	result.bindings = MergeBindings(a.bindings, b.bindings);

	result.uniformBufferBindings.clear();
	for (const auto& binding : result.bindings) {
		if (IsUniformBuffer(binding.type)) {
			result.uniformBufferBindings.push_back(binding.binding);
		}
	}

	result.pushConstantSize = std::max(a.pushConstantSize, b.pushConstantSize);
	result.pushConstantStages = a.pushConstantStages | b.pushConstantStages;
	return result;
}

} // namespace PixieRenderer
