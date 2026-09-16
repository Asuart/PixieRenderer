#pragma once
#include <glad/glad.h>

#include "OpenGLGraphicsProgram.h"
#include "OpenGLComputeProgram.h"

namespace PixieRenderer {

GLuint CompileShaderOpenGL(
    const char* vertexShaderSource,
    const char* framgentShaderSource,
    const char* geometryShaderSource = nullptr
);

GLuint CompileOpenGLComputeProgram(const char* computeShaderSource);

} // namespace PixieRenderer
