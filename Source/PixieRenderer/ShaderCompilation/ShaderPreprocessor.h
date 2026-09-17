#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace PixieRenderer {

enum class ShaderPreprocessorType : int32_t {
	Int32,
	UInt32,
	Float,
	Double,
	Vector2,
	Vector3,
	Vector4,
	Matrix2,
	Matrix3,
	Matrix4
};

struct ShaderStorageBufferBinding {
	std::string name;
	int32_t index;
	ShaderPreprocessorType type;
};

struct ShaderPreprocessorUniform {
	std::string name;
	ShaderPreprocessorType type;
};

struct ShaderPreprocessorConstant {
	std::string name;
	float value;
	ShaderPreprocessorType type;
};

class ShaderPreprocessor {
  public:
	ShaderPreprocessor();

	void SetBinding(const std::string& name, int32_t index, ShaderPreprocessorType type);
	void SetUniform(const std::string& name, ShaderPreprocessorType type);
	void SetConstant(const std::string& name, float value, ShaderPreprocessorType type);

	std::string PreprocessComputeShader(
	    const std::string& source,
	    int32_t workGroupSizeX = 1,
	    int32_t workGroupSizeY = 1,
	    int32_t workGroupSizeZ = 1
	) const;

  protected:
	std::vector<ShaderStorageBufferBinding> m_bindings;
	std::vector<ShaderPreprocessorUniform> m_uniforms;
	std::vector<ShaderPreprocessorConstant> m_constants;

	std::string TypeToStringGLSL(ShaderPreprocessorType type) const;
	std::string ConstantToStringGLSL(const ShaderPreprocessorConstant& constant) const;
};

} // namespace PixieRenderer
