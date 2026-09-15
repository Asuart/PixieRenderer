#include "PixieUIApplication.h"

#include "UIOpenGL.h"
#include "UIVulkan.h"

using namespace PixieRenderer;

namespace PixieApp {

PixieUIApplication::PixieUIApplication(
    const std::string& name,
    glm::ivec2 resolution,
    RenderAPI renderAPI,
    bool docking
)
    : PixieApplication(name, resolution, renderAPI) {
	switch (m_window->GetRenderAPI()) {
	case RenderAPI::OpenGL: {
		WindowOpenGL* mainWindowOpenGL = reinterpret_cast<WindowOpenGL*>(m_window);
		m_ui = new PixieUI::UIOpenGL(mainWindowOpenGL, docking);
		break;
	}
	case RenderAPI::Vulkan: {
		WindowVulkan* mainWindowVulkan = reinterpret_cast<WindowVulkan*>(m_window);
		m_ui = new PixieUI::UIVulkan(mainWindowVulkan, docking);
		break;
	}
	default:
		throw "UI initialization failed";
	}
}

void PixieUIApplication::BeforeDrawFrame() {
	m_ui->OnBeforeDrawFrame();
}

void PixieUIApplication::OnDrawFrame() {
	m_ui->Draw();
}

} // namespace PixieApp
