#include "PixieRenderer/pch.h"
#include "ShaderCompilerOpenGL.h"

#include "ShaderCompiler.h"

namespace PixieRenderer {

namespace {

GLenum ToGLStage(ShaderStage stage) {
	switch (stage) {
	case ShaderStage::Vertex:
		return GL_VERTEX_SHADER;
	case ShaderStage::Fragment:
		return GL_FRAGMENT_SHADER;
	case ShaderStage::Geometry:
		return GL_GEOMETRY_SHADER;
	case ShaderStage::Compute:
		return GL_COMPUTE_SHADER;
	}
	return 0;
}

GLuint CreateOpenGLShaderFromSPIRV(ShaderStage stage, const SpirVBinary& binary) {
	const GLenum glStage = ToGLStage(stage);
	if (glStage == 0 || !binary.words || binary.size == 0) {
		return 0;
	}

	GLuint shader = glCreateShader(glStage);

	// Требует GL 4.6 / GL_ARB_gl_spirv.
	glShaderBinary(
	    1,
	    &shader,
	    GL_SHADER_BINARY_FORMAT_SPIR_V,
	    binary.words,
	    static_cast<GLsizei>(binary.size) * static_cast<GLsizei>(sizeof(uint32_t))
	);

	glSpecializeShader(shader, "main", 0, nullptr, nullptr);

	GLint status = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		GLint logLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
		std::vector<char> log(logLen > 1 ? logLen : 1, '\0');
		glGetShaderInfoLog(shader, logLen, nullptr, log.data());
		std::cerr << "[OpenGL SPIR-V] " << log.data() << "\n";
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint CompileStage(ShaderStage stage, const char* source) {
	if (!source)
		return 0;

	if (!ShaderCompiler::IsInitialized()) {
		ShaderCompiler::Initialize();
	}
	SpirVBinary bin = ShaderCompiler::CompileToSPIRV(stage, source);
	GLuint shader = CreateOpenGLShaderFromSPIRV(stage, bin);
	ShaderCompiler::FreeSPIRV(bin);
	return shader;
}

GLuint LinkProgram(std::initializer_list<GLuint> shaders) {
	GLuint program = glCreateProgram();
	for (GLuint s : shaders) {
		if (s)
			glAttachShader(program, s);
	}
	glLinkProgram(program);

	GLint status = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		GLint logLen = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
		std::vector<char> log(logLen > 1 ? logLen : 1, '\0');
		glGetProgramInfoLog(program, logLen, nullptr, log.data());
		std::cerr << "[OpenGL link] " << log.data() << "\n";
	}

	for (GLuint s : shaders) {
		if (s) {
			glDetachShader(program, s);
			glDeleteShader(s);
		}
	}
	return program;
}

} // namespace

GLuint CompileShaderOpenGL(
    const char* vertexShaderSource,
    const char* framgentShaderSource,
    const char* geometryShaderSource
) {
	GLuint vert = CompileStage(ShaderStage::Vertex, vertexShaderSource);
	GLuint frag = CompileStage(ShaderStage::Fragment, framgentShaderSource);
	GLuint geom = geometryShaderSource ? CompileStage(ShaderStage::Geometry, geometryShaderSource) : 0;

	return LinkProgram({ vert, frag, geom });
}

GLuint CompileOpenGLComputeProgram(const char* computeShaderSource) {
	GLuint comp = CompileStage(ShaderStage::Compute, computeShaderSource);
	return LinkProgram({ comp });
}

} // namespace PixieRenderer
