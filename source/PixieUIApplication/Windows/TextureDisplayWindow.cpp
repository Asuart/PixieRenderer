#include "TextureDisplayWindow.h"

#include <cstring>
#include <string>

#include "../../dependencies/imgui/backends/imgui_impl_vulkan.h"
#include <imgui.h>

#include <PixieRendering/Renderer/Vulkan/RendererVulkan.h>

#include "PixieUIApplication/UI.h"

using namespace PixieRenderer;

namespace PixieUI {

static const char* VERTEX_SHADER_SOURCE = R"(
#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in ivec4 boneIDs;
layout(location = 4) in vec4 boneWeights;

layout(location = 0) out vec2 fTexCoord;

layout(set = 0, binding = 0) uniform PlaneUBO {
    vec2 pos;
    vec2 size;
} plane;

void main() {
    fTexCoord = aTexCoord;
    vec2 transformedPosition = vec2(
        aPos.x * plane.size.x + plane.pos.x,
        -aPos.y * plane.size.y - plane.pos.y
    ) * 2.0 - vec2(1.0, -1.0);
    gl_Position = vec4(transformedPosition, 0.0, 1.0);
}
)";

static const char* FRAGMENT_SHADER_SOURCE = R"(
#version 450 core

layout(location = 0) in vec2 fTexCoord;
layout(location = 0) out vec4 color;

layout(set = 0, binding = 1) uniform sampler2D displayTexture;

void main() {
    vec4 pixel = texture(displayTexture, fTexCoord);
    color = vec4(pixel.rgb, 1.0);
}
)";

namespace {

struct PlaceUBO {
	glm::vec2 position = { 0.0f, 0.0f };
	glm::vec2 size = { 1.0f, 1.0f };
};

Mesh MakeScreenPlaneMesh() {
	Mesh mesh;
	mesh.vertexes = {
		{ glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0, 0, 1), glm::vec2(0.0f, 0.0f) },
		{ glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(0, 0, 1), glm::vec2(0.0f, 1.0f) },
		{ glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0, 0, 1), glm::vec2(1.0f, 1.0f) },
		{ glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0, 0, 1), glm::vec2(1.0f, 0.0f) },
	};
	mesh.indexes = { 0, 1, 2, 0, 2, 3 };
	return mesh;
}

} // namespace

void TextureDisplayWindow::CreateShader() {
	IMaterial mat{ VERTEX_SHADER_SOURCE, FRAGMENT_SHADER_SOURCE };
	m_shader = m_renderer->CreateMaterial(&mat);
}

void TextureDisplayWindow::CreateScreenPlane() {
	Mesh mesh = MakeScreenPlaneMesh();
	m_screenPlane = m_renderer->CreateMesh(&mesh);
}

void TextureDisplayWindow::UpdatePlaneUBO() {
	PlaceUBO planeUBO{};

	float textureAspect = 1.0f;
	if (m_targetTexture) {
		textureAspect = Aspect(glm::ivec2(m_renderer->GetTextureResolution(m_targetTexture)));
	} else if (m_targetFrameBuffer) {
		textureAspect = Aspect(glm::ivec2(m_renderer->GetFrameBufferResolution(m_targetFrameBuffer)));
	}

	float viewportAspect = Aspect(glm::ivec2(m_viewportResolution));
	if (viewportAspect > textureAspect) {
		planeUBO.size.x = textureAspect / viewportAspect;
		planeUBO.position.x = (1.0f - planeUBO.size.x) * 0.5f;
	} else {
		planeUBO.size.y = viewportAspect / textureAspect;
		planeUBO.position.y = (1.0f - planeUBO.size.y) * 0.5f;
	}

	const auto bytes = std::as_bytes(std::span{ &planeUBO, 1 });
	if (!m_planeUBO) {
		m_planeUBO = m_renderer->CreateBuffer(BufferType::Uniform, bytes);
	} else {
		m_renderer->UpdateBuffer(m_planeUBO, bytes);
	}
}

TextureDisplayWindow::TextureDisplayWindow(UI* ui, IRenderer* renderer, TextureHandle texture)
    : UIWindow(ui, renderer), m_targetTexture(texture) {
	m_viewportResolution = { 1280, 720 };
	m_frameBuffer = m_renderer->CreateFrameBuffer(m_viewportResolution, TextureFormat::RGBA32f);

	CreateShader();
	CreateScreenPlane();
	UpdatePlaneUBO();

	SetTexture(texture);
}

TextureDisplayWindow::TextureDisplayWindow(UI* ui, IRenderer* renderer, FrameBufferHandle frameBuffer)
    : UIWindow(ui, renderer), m_targetFrameBuffer(frameBuffer) {
	m_viewportResolution = { 1280, 720 };
	m_frameBuffer = m_renderer->CreateFrameBuffer(m_viewportResolution, TextureFormat::RGBA32f);

	CreateShader();
	CreateScreenPlane();
	UpdatePlaneUBO();

	SetFrameBuffer(frameBuffer);
}

TextureDisplayWindow::~TextureDisplayWindow() {
	delete m_image;
	m_image = nullptr;
}

void TextureDisplayWindow::OnBeforeDraw() {
	//if (!m_resolutionChanged && m_viewportResolution == m_appliedResolution) {
	//	return;
	//}
	//m_resolutionChanged = false;

	//if (m_viewportResolution.x == 0 || m_viewportResolution.y == 0) {
	//	return;
	//}

	//UpdatePlaneUBO();

	//if (m_viewportResolution != m_appliedResolution) {
	//	m_renderer->WaitIdle();
	//	m_renderer->SetFrameBufferResolution(m_frameBuffer, m_viewportResolution);
	//	m_appliedResolution = m_viewportResolution;
	//}

	//m_renderer->BeginRenderPass(m_frameBuffer);

	//m_renderer->BindBuffer(m_shader, "PlaneUBO", m_planeUBO);

	//if (m_targetTexture) {
	//	m_renderer->BindTexture(m_shader, "displayTexture", m_targetTexture, 0);
	//}

	//m_renderer->DrawMesh(DrawRequest{
	//    .material = m_shader,
	//    .mesh = m_screenPlane,
	//});

	//m_renderer->EndRenderPass();
}

void TextureDisplayWindow::Draw() {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.3f, 0.4f, 1.0f));

	if (ImGui::Begin("Texture View")) {
		ImVec2 avail = ImGui::GetContentRegionAvail();
		if (m_viewportResolution.x != avail.x || m_viewportResolution.y != avail.y) {
			m_viewportResolution = { static_cast<uint32_t>(avail.x), static_cast<uint32_t>(avail.y) };
			m_resolutionChanged = true;
		}

		ImTextureID texID = m_image ? m_image->GetTextureID() : 0;
		ImGui::Image(texID, avail, { 0.0f, 1.0f }, { 1.0f, 0.0f });
	}
	ImGui::End();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

void TextureDisplayWindow::SetTexture(TextureHandle texture) {
	delete m_image;
	m_image = m_ui->CreateUIImage(m_renderer, texture);
	m_targetTexture = texture;
	m_targetFrameBuffer = {};
	m_resolutionChanged = true;
}

void TextureDisplayWindow::SetFrameBuffer(FrameBufferHandle frameBuffer) {
	delete m_image;
	m_image = m_ui->CreateUIImage(m_renderer, frameBuffer);
	m_targetFrameBuffer = frameBuffer;
	m_targetTexture = {};
	m_resolutionChanged = true;
}

float TextureDisplayWindow::Aspect(glm::ivec2 resolution) {
	if (resolution.y == 0) {
		return 1.0f;
	}
	return static_cast<float>(resolution.x) / static_cast<float>(resolution.y);
}

} // namespace PixieUI
