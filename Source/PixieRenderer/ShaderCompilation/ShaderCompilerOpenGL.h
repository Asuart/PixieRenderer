#pragma once
#include <glad/glad.h>

namespace PixieRenderer {

GLuint CompileShaderOpenGL(
    const char* vertexShaderSource,
    const char* framgentShaderSource,
    const char* geometryShaderSource = nullptr
);

GLuint CompileOpenGLComputeProgram(const char* computeShaderSource);

} // namespace PixieRenderer
