#pragma once
#include <glad/glad.h>

<<<<<<< Updated upstream:Source/PixieRenderer/ShaderCompilation/ShaderCompilerOpenGL.h
#include "Pixierenderer/Renderer/OpenGL/OpenGLGraphicsProgram.h"
#include "Pixierenderer/Renderer/OpenGL/OpenGLComputeProgram.h"
=======
<<<<<<< Updated upstream:Source/PixieRenderer/Renderer/OpenGL/ShaderCompilationOpenGL.h
#include "OpenGLGraphicsProgram.h"
#include "OpenGLComputeProgram.h"
=======
#include "PixieRenderer/Renderer/OpenGL/OpenGLGraphicsProgram.h"
#include "PixieRenderer/Renderer/OpenGL/OpenGLComputeProgram.h"
>>>>>>> Stashed changes:Source/PixieRenderer/ShaderCompilation/ShaderCompilerOpenGL.h
>>>>>>> Stashed changes:Source/PixieRenderer/Renderer/OpenGL/ShaderCompilationOpenGL.h

namespace PixieRenderer {

GLuint CompileShaderOpenGL(
    const char* vertexShaderSource,
    const char* framgentShaderSource,
    const char* geometryShaderSource = nullptr
);

GLuint CompileOpenGLComputeProgram(const char* computeShaderSource);

} // namespace PixieRenderer
