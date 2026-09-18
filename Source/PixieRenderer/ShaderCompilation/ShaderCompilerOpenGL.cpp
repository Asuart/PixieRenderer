#include "PixieRenderer/pch.h"
#include "ShaderCompilerOpenGL.h"

namespace PixieRenderer {

GLuint CompileShaderOpenGL(
    const char* vertexShaderSource,
    const char* framgentShaderSource,
    const char* geometryShaderSource
) {
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

	GLint result = GL_FALSE;
	int32_t logLength;

	glShaderSource(vertShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertShader);

	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength) {
		std::vector<char> shaderError((logLength > 1) ? logLength : 1);
		glGetShaderInfoLog(vertShader, logLength, NULL, &shaderError[0]);
		std::cerr << std::string_view(&shaderError[0]) << "\n";
	}

	glShaderSource(fragShader, 1, &framgentShaderSource, NULL);
	glCompileShader(fragShader);

	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength) {
		std::vector<char> shaderError((logLength > 1) ? logLength : 1);
		glGetShaderInfoLog(fragShader, logLength, NULL, &shaderError[0]);
		std::cerr << std::string_view(&shaderError[0]) << "\n";
	}

	GLuint geometryShader = 0;
	if (geometryShaderSource) {
		geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometryShader, 1, &geometryShaderSource, NULL);
		glCompileShader(geometryShader);

		glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &result);
		glGetShaderiv(geometryShader, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength) {
			std::vector<char> shaderError((logLength > 1) ? logLength : 1);
			glGetShaderInfoLog(geometryShader, logLength, NULL, &shaderError[0]);
			std::cerr << std::string_view(&shaderError[0]) << "\n";
		}
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	if (geometryShaderSource) {
		glAttachShader(program, geometryShader);
	}
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &result);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength) {
		std::vector<char> shaderError((logLength > 1) ? logLength : 1);
		glGetShaderInfoLog(program, logLength, NULL, &shaderError[0]);
		std::cerr << std::string_view(&shaderError[0]) << "\n";
	}

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	return program;
}

GLuint CompileOpenGLComputeProgram(const char* computeShaderSource) {
	GLint result = GL_FALSE;
	int32_t logLength;
	GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(computeShader, 1, &computeShaderSource, NULL);
	glCompileShader(computeShader);

	glGetShaderiv(computeShader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(computeShader, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength) {
		std::vector<char> shaderError((logLength > 1) ? logLength : 1);
		glGetShaderInfoLog(computeShader, logLength, NULL, &shaderError[0]);
		std::cerr << std::string_view(&shaderError[0]) << "\n";
	}

	GLuint computeProgram = glCreateProgram();
	glAttachShader(computeProgram, computeShader);
	glLinkProgram(computeProgram);

	glGetProgramiv(computeProgram, GL_LINK_STATUS, &result);
	glGetProgramiv(computeProgram, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength) {
		std::vector<char> shaderError((logLength > 1) ? logLength : 1);
		glGetShaderInfoLog(computeShader, logLength, NULL, &shaderError[0]);
		std::cerr << std::string_view(&shaderError[0]) << "\n";
	}

	return computeProgram;
}

} // namespace PixieRenderer
