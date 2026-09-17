#include "ShaderPreprocessor.h"

#include <format>

namespace PixieRenderer {

static std::string ReplaceAll(const std::string& str, const std::string& find, const std::string& replace) {
	std::string result;
	size_t find_len = find.size();
	size_t pos, from = 0;
	while (std::string::npos != (pos = str.find(find, from))) {
		result.append(str, from, pos - from);
		result.append(replace);
		from = pos + find_len;
	}
	result.append(str, from, std::string::npos);
	return result;
}

ShaderPreprocessor::ShaderPreprocessor() {
}

void ShaderPreprocessor::SetBinding(const std::string& name, int32_t index, ShaderPreprocessorType type) {
	for (ShaderStorageBufferBinding& binding : m_bindings) {
		if (binding.name == name) {
			binding.index = index;
			binding.type = type;
			return;
		}
	}
	m_bindings.push_back(ShaderStorageBufferBinding{ name, index, type });
}

void ShaderPreprocessor::SetUniform(const std::string& name, ShaderPreprocessorType type) {
	for (ShaderPreprocessorUniform& uniform : m_uniforms) {
		if (uniform.name == name) {
			uniform.type = type;
			return;
		}
	}
	m_uniforms.push_back(ShaderPreprocessorUniform{ name, type });
}

void ShaderPreprocessor::SetConstant(const std::string& name, float value, ShaderPreprocessorType type) {
	for (ShaderPreprocessorConstant& constant : m_constants) {
		if (constant.name == name) {
			constant.value = value;
			constant.type = type;
			return;
		}
	}
	m_constants.push_back(ShaderPreprocessorConstant{ name, value, type });
}

std::string ShaderPreprocessor::PreprocessComputeShader(
    const std::string& source,
    int32_t workGroupSizeX,
    int32_t workGroupSizeY,
    int32_t workGroupSizeZ
) const {
	const std::string shaderVersion = "#version 430 core\n\n";
	const std::string shaderWorkgroupSize = std::format(
	    "layout(local_size_x = {}, local_size_y = {}, local_size_z = {}) in;\n\n",
	    workGroupSizeX,
	    workGroupSizeY,
	    workGroupSizeZ
	);

	std::string assembledSource = shaderVersion + shaderWorkgroupSize;

	for (const ShaderStorageBufferBinding& binding : m_bindings) {
		if (source.find(binding.name) == std::string::npos)
			continue;
		std::string bufferCode = std::to_string(binding.index);
		std::string
		    bindingCode = std::format("layout(std430, binding = {}) buffer {}_buffer ", bufferCode, binding.name) +
		                  "{\n" + std::format("{} {}[];", TypeToStringGLSL(binding.type), binding.name) + "\n};\n\n";
		assembledSource += bindingCode;
	}

	std::string preprocessed = source;
	for (const ShaderPreprocessorConstant& constant : m_constants) {
		preprocessed = ReplaceAll(preprocessed, constant.name, ConstantToStringGLSL(constant));
	}

	assembledSource += preprocessed;

	return assembledSource;
}

std::string ShaderPreprocessor::TypeToStringGLSL(ShaderPreprocessorType type) const {
	switch (type) {
	case ShaderPreprocessorType::Int32:
		return "int";
	case ShaderPreprocessorType::UInt32:
		return "uint";
	case ShaderPreprocessorType::Float:
		return "float";
	case ShaderPreprocessorType::Double:
		return "double";
	case ShaderPreprocessorType::Vector2:
		return "vec2";
	case ShaderPreprocessorType::Vector3:
		return "vec3";
	case ShaderPreprocessorType::Vector4:
		return "vec4";
	case ShaderPreprocessorType::Matrix2:
		return "mat2";
	case ShaderPreprocessorType::Matrix3:
		return "mat3";
	case ShaderPreprocessorType::Matrix4:
		return "mat4";
	default:
		return "<undifined>";
	}
}

std::string ShaderPreprocessor::ConstantToStringGLSL(const ShaderPreprocessorConstant& constant) const {
	switch (constant.type) {
	case ShaderPreprocessorType::Int32:
		return std::to_string(static_cast<int32_t>(constant.value));
	case ShaderPreprocessorType::UInt32:
		return std::to_string(static_cast<uint32_t>(constant.value));
	case ShaderPreprocessorType::Float:
		return std::to_string(static_cast<float>(constant.value));
	case ShaderPreprocessorType::Double:
		return std::to_string(static_cast<double>(constant.value));
	default:
		throw "Only primitive types are supported.";
	}
}

} // namespace PixieRenderer