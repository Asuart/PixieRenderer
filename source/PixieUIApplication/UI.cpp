#include "UI.h"

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <vulkan/vulkan.hpp>

#include <PixieApplication/Log/Log.h>
#include <PixieRendering/Window/IWindow.h>

#include "UIWindow.h"

namespace PixieUI {

UI::UI(PixieRenderer::IWindow* mainWindow, bool docking)
    : m_window(mainWindow), m_isDocking(docking) {
	IMGUI_CHECKVERSION();
}

UI::~UI() {
	for (size_t i = 0; i < m_windows.size(); i++) {
		delete m_windows[i];
	}
}

void UI::AddWindow(PixieUI::UIWindow* window) {
	if (!window) {
		return;
	}
	m_windows.push_back(window);
}

void UI::OnBeforeDrawFrame() {
	for (PixieUI::UIWindow* window : m_windows) {
		window->OnBeforeDraw();
	}
}

} // namespace PixieUI
