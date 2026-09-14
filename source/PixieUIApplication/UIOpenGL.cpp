#include "UIOpenGL.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include <PixieApplication/Log/Log.h>
#include <PixieRendering/Window/IWindow.h>
#include <PixieRendering/Window/WindowOpenGL.h>

#include "UIImageOpenGL.h"
#include "UIWindow.h"

using namespace PixieRenderer;

namespace PixieUI {

UIOpenGL::UIOpenGL(WindowOpenGL* mainWindow, bool docking) : UI(mainWindow, docking) {
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(mainWindow->GetGLFWWindow(), true);
	ImGui_ImplOpenGL3_Init();
}

UIOpenGL::~UIOpenGL() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void UIOpenGL::Draw() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
	               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode) {
		windowFlags |= ImGuiWindowFlags_NoBackground;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace", &m_isDocking, windowFlags);

	ImGui::PopStyleVar(3);

	if (m_isDocking) {
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspaceFlags);
	}

	for (UIWindow* window : m_windows) {
		window->Draw();
	}

	ImGui::End();

	ImGui::Render();
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

UIImage* UIOpenGL::CreateUIImage(IRenderer* renderer, FrameBufferHandle handle) {
	UIImageOpenGL* image = new UIImageOpenGL(renderer);
	image->SetFrameBuffer(handle);
	return image;
}

UIImage* UIOpenGL::CreateUIImage(IRenderer* renderer, TextureHandle handle) {
	UIImageOpenGL* image = new UIImageOpenGL(renderer);
	image->SetTexture(handle);
	return image;
}

} // namespace PixieUI
