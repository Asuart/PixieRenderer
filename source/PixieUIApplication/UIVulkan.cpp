#include "UIVulkan.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <vulkan/vulkan.hpp>

#include <PixieRendering/Renderer/Vulkan/RendererVulkan.h>
#include <PixieRendering/Window/IWindow.h>
#include <PixieRendering/Window/WindowVulkan.h>

#include <PixieApplication/Log/Log.h>

#include "UIImageVulkan.h"
#include "UIWindow.h"

using namespace PixieRenderer;

namespace PixieUI {

UIVulkan::UIVulkan(WindowVulkan* mainWindow, bool docking) : UI(mainWindow, docking) {
	VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 5000 },
		                                  { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5000 },
		                                  { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 5000 },
		                                  { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 5000 },
		                                  { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 5000 },
		                                  { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 5000 },
		                                  { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5000 },
		                                  { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5000 },
		                                  { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 5000 },
		                                  { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 5000 },
		                                  { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 5000 } };

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 2000;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	RendererVulkan* renderer = reinterpret_cast<WindowVulkan*>(mainWindow)->GetRendererVulkan();

	VkDescriptorPool imguiPool;
	if (vkCreateDescriptorPool(renderer->GetDevice(), &pool_info, nullptr, &imguiPool) != VK_SUCCESS) {
		PixieApp::Log::Error("Failed to create Vulkan descriptor pool.");
		exit(1);
	}
	m_pool = imguiPool;

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();

	if (!ImGui_ImplGlfw_InitForVulkan(mainWindow->GetGLFWWindow(), true)) {
		PixieApp::Log::Error("Failed to init imgui for glfw vulkan.");
		exit(2);
	}

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.ApiVersion = VK_API_VERSION_1_0;
	init_info.Instance = renderer->GetInstance();
	init_info.PhysicalDevice = renderer->GetPhysicalDevice();
	init_info.Device = renderer->GetDevice();
	init_info.Queue = renderer->GetGraphicsQueue();
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.PipelineInfoMain.RenderPass = renderer->GetPresentRenderPass();
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.PipelineInfoMain.Subpass = 0;

	if (!ImGui_ImplVulkan_Init(&init_info)) {
		PixieApp::Log::Error("Failed to init imgui for vulkan.");
		exit(3);
	}

	// Draw ImGui while the present render pass is still active.
	renderer->SetPresentOverlayHook([renderer]() {
		ImDrawData* drawData = ImGui::GetDrawData();
		if (!drawData || drawData->CmdListsCount == 0)
			return;
		ImGui_ImplVulkan_RenderDrawData(drawData, renderer->GetCurrentFrameCommandBuffer());
	});
}

UIVulkan::~UIVulkan() {
	if (m_window) {
		if (auto* renderer = reinterpret_cast<WindowVulkan*>(m_window)->GetRendererVulkan()) {
			renderer->SetPresentOverlayHook(nullptr);
		}
	}

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	if (m_pool != VK_NULL_HANDLE && m_window) {
		if (auto* renderer = reinterpret_cast<WindowVulkan*>(m_window)->GetRendererVulkan()) {
			vkDestroyDescriptorPool(renderer->GetDevice(), m_pool, nullptr);
		}
		m_pool = VK_NULL_HANDLE;
	}
}

void UIVulkan::Draw() {
	ImGui_ImplVulkan_NewFrame();
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
	windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
	               ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode) {
		windowFlags |= ImGuiWindowFlags_NoBackground;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace", &m_isDocking, windowFlags);
	ImGui::PopStyleVar(3);

	if (m_isDocking) {
		ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
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

	// Actual draw data is submitted via SetPresentOverlayHook while the
	// present render pass is active.
}

UIImage* UIVulkan::CreateUIImage(IRenderer* renderer, FrameBufferHandle handle) {
	UIImageVulkan* image = new UIImageVulkan(renderer);
	image->SetFrameBuffer(handle);
	return image;
}

UIImage* UIVulkan::CreateUIImage(IRenderer* renderer, TextureHandle handle) {
	UIImageVulkan* image = new UIImageVulkan(renderer);
	image->SetTexture(handle);
	return image;
}

} // namespace PixieUI
