#pragma once
#include <glad/glad.h>

#include "Pixierenderer/Renderer/OpenGL/OpenGLGraphicsProgram.h"
#include "Pixierenderer/Renderer/OpenGL/OpenGLComputeProgram.h"

namespace PixieRenderer {

GLuint CompileShaderOpenGL(
    const char* vertexShaderSource,
    const char* framgentShaderSource,
    const char* geometryShaderSource = nullptr
);

GLuint CompileOpenGLComputeProgram(const char* computeShaderSource);

} // namespace PixieRenderer
