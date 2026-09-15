#include "ShaderCompilationVulkan.h"
#include "PixieRendering/pch.h"

#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>

#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

namespace PixieRenderer {

SpirVBinary ShaderCompilerVulkan::CompileShaderToSPIRV(glslang_stage_t stage, const char* shaderSource) {
	glslang_input_t glslangShaderCreateInfo = {
		.language = GLSLANG_SOURCE_GLSL,
		.stage = stage,
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

	glslang_shader_t* shader = glslang_shader_create(&glslangShaderCreateInfo);

	SpirVBinary bin = {
		.words = NULL,
		.size = 0,
	};

	if (!glslang_shader_preprocess(shader, &glslangShaderCreateInfo)) {
		printf("GLSL preprocessing failed.\n");
		printf("%s\n", glslang_shader_get_info_log(shader));
		printf("%s\n", glslang_shader_get_info_debug_log(shader));
		printf("%s\n", glslangShaderCreateInfo.code);
		glslang_shader_delete(shader);
		return bin;
	}

	if (!glslang_shader_parse(shader, &glslangShaderCreateInfo)) {
		printf("GLSL parsing failed\n");
		printf("%s\n", glslang_shader_get_info_log(shader));
		printf("%s\n", glslang_shader_get_info_debug_log(shader));
		printf("%s\n", glslang_shader_get_preprocessed_code(shader));
		glslang_shader_delete(shader);
		return bin;
	}

	glslang_program_t* program = glslang_program_create();
	glslang_program_add_shader(program, shader);

	if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
		printf("GLSL linking failed\n");
		printf("%s\n", glslang_program_get_info_log(program));
		printf("%s\n", glslang_program_get_info_debug_log(program));
		glslang_program_delete(program);
		glslang_shader_delete(shader);
		return bin;
	}

	glslang_program_SPIRV_generate(program, stage);

	bin.size = static_cast<int32_t>(glslang_program_SPIRV_get_size(program));
	bin.words = new uint32_t[bin.size];
	glslang_program_SPIRV_get(program, bin.words);

	const char* spirv_messages = glslang_program_SPIRV_get_messages(program);
	if (spirv_messages) {
		printf("%s\b", spirv_messages);
	}

	glslang_program_delete(program);
	glslang_shader_delete(shader);

	return bin;
}

VkShaderModule ShaderCompilerVulkan::CreateShaderModule(VkDevice device, SpirVBinary binary) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = binary.size * sizeof(binary.words[0]);
	createInfo.pCode = reinterpret_cast<const uint32_t*>(binary.words);

	VkShaderModule shaderModule = nullptr;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw "Failed to create vulkan shader module";
	}

	return shaderModule;
}

CompiledShader ShaderCompilerVulkan::CompileShader(
    VkDevice device,
    const char* vertexShaderSource,
    const char* fragmentShaderSource
) {
	if (!ShaderCompiler::IsInitialized()) {
		ShaderCompiler::Initialize();
	}

	SpirVBinary vertexBinary = CompileShaderToSPIRV(GLSLANG_STAGE_VERTEX, vertexShaderSource);
	SpirVBinary fragmentBinary = CompileShaderToSPIRV(GLSLANG_STAGE_FRAGMENT, fragmentShaderSource);

	BindingsInfo vertexInfo = ReflectSPIRV(vertexBinary);
	BindingsInfo fragmentInfo = ReflectSPIRV(fragmentBinary);

	std::vector<ShaderBinding>
	    mergedBindings = ShaderCompiler::MergeBindings(vertexInfo.bindings, fragmentInfo.bindings);

	BindingsInfo finalInfo;
	finalInfo.bindings = std::move(mergedBindings);
	for (const auto& binding : finalInfo.bindings) {
		if (static_cast<uint32_t>(binding.type) == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
		    static_cast<uint32_t>(binding.type) == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
			finalInfo.uniformBufferBindings.push_back(binding.binding);
		}
	}
	finalInfo.pushConstantSize = std::max(vertexInfo.pushConstantSize, fragmentInfo.pushConstantSize);

	finalInfo.pushConstantStages = 0;
	if (vertexInfo.pushConstantSize > 0) {
		finalInfo.pushConstantStages |= VK_SHADER_STAGE_VERTEX_BIT;
	}
	if (fragmentInfo.pushConstantSize > 0) {
		finalInfo.pushConstantStages |= VK_SHADER_STAGE_FRAGMENT_BIT;
	}

	VkShaderModule vertShaderModule = CreateShaderModule(device, vertexBinary);
	VkShaderModule fragShaderModule = CreateShaderModule(device, fragmentBinary);

	delete[] vertexBinary.words;
	delete[] fragmentBinary.words;

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	return { { vertShaderModule, fragShaderModule }, { vertShaderStageInfo, fragShaderStageInfo }, finalInfo };
}

CompiledComputeShader ShaderCompilerVulkan::CompileComputeShader(VkDevice device, const char* source) {
	if (!ShaderCompiler::IsInitialized()) {
		ShaderCompiler::Initialize();
	}

	SpirVBinary computeBinary = CompileShaderToSPIRV(GLSLANG_STAGE_COMPUTE, source);
	VkShaderModule shaderModule = CreateShaderModule(device, computeBinary);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	vertShaderStageInfo.module = shaderModule;
	vertShaderStageInfo.pName = "main";

	BindingsInfo bindingInfo = ReflectSPIRV(computeBinary);

	if (bindingInfo.pushConstantSize > 0) {
		bindingInfo.pushConstantStages = VK_SHADER_STAGE_COMPUTE_BIT;
	}

	delete[] computeBinary.words;

	return { shaderModule, vertShaderStageInfo, bindingInfo };
}

BindingsInfo ShaderCompilerVulkan::ReflectSPIRV(const SpirVBinary& binary) {
	BindingsInfo result;

	spirv_cross::CompilerGLSL* compiler = new spirv_cross::CompilerGLSL(binary.words, binary.size);
	spirv_cross::ShaderResources resources = compiler->get_shader_resources();

	auto add_bindings = [&](const auto& resource_list, VkDescriptorType type, VkShaderStageFlagBits stage) {
		std::vector<uint32_t> bindingIndexes{};

		for (const auto& res : resource_list) {
			const auto& binding = compiler->get_decoration(res.id, spv::DecorationBinding);
			bindingIndexes.push_back(binding);

			const auto& set = compiler->get_decoration(res.id, spv::DecorationDescriptorSet);
			uint32_t setIndex = (set != 0) ? set : 0;

			auto type_id = compiler->get_type(res.type_id);

			uint32_t blockSize = 0;
			if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
				auto base_type_id = compiler->get_type(res.base_type_id);
				blockSize = static_cast<uint32_t>(compiler->get_declared_struct_size(base_type_id));
			}

			uint32_t count = 1;
			if (type_id.array.size() > 0) {
				count = type_id.array[0];
			}

			result.bindings.push_back(ShaderBinding(res.name, type, binding, setIndex, blockSize, count, stage));
		}

		return bindingIndexes;
	};

	result.uniformBufferBindings =
	    add_bindings(resources.uniform_buffers, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL);
	add_bindings(resources.storage_buffers, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_ALL);
	add_bindings(resources.sampled_images, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL);
	add_bindings(resources.separate_images, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_ALL);
	add_bindings(resources.separate_samplers, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_ALL);

	uint32_t pushSize = 0;
	for (const auto& res : resources.push_constant_buffers) {
		auto type = compiler->get_type(res.base_type_id);
		uint32_t sz = static_cast<uint32_t>(compiler->get_declared_struct_size(type));
		pushSize = std::max(pushSize, sz);
	}
	result.pushConstantSize = pushSize;

	delete compiler;

	return result;
}

} // namespace PixieRenderer
