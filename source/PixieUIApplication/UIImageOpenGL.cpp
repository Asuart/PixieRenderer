#include "UIImageOpenGL.h"

#include <PixieRendering/Renderer/OpenGL/RendererOpenGL.h>

using namespace PixieRenderer;

namespace PixieUI {

UIImageOpenGL::UIImageOpenGL(IRenderer* renderer) : m_renderer(renderer) {
}

UIImageOpenGL::~UIImageOpenGL() {
}

void UIImageOpenGL::SetTexture(TextureHandle texture) {
	m_currentTexture = texture;
	m_currentFrameBuffer = {};
	if (texture) {
		RendererOpenGL* renderer = reinterpret_cast<RendererOpenGL*>(m_renderer);
		GLuint glTex = renderer->GetInternalTextureID(texture);
		m_displayTexture = (ImTextureID)(uintptr_t)glTex;
	} else {
		m_displayTexture = 0;
	}
}

void UIImageOpenGL::SetFrameBuffer(FrameBufferHandle frameBuffer) {
	m_currentFrameBuffer = frameBuffer;
	m_currentTexture = {};
	if (frameBuffer) {
		RendererOpenGL* renderer = reinterpret_cast<RendererOpenGL*>(m_renderer);
		GLuint glTex = renderer->GetInternalFrameBufferColorAttachmentID(frameBuffer);
		m_displayTexture = (ImTextureID)(uintptr_t)glTex;
	} else {
		m_displayTexture = 0;
	}
}

ImTextureID UIImageOpenGL::GetTextureID() const {
	return m_displayTexture;
}

} // namespace PixieUI
