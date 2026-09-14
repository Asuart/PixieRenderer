#include "PixieApplication.h"

#include "PixieRendering/PixieRendering.h"

#include "Time/ApplicationTime.h"
#include "Time/GlobalTimer.h"
#include "UserInput/UserInput.h"

using namespace PixieRenderer;

namespace PixieApp {

PixieApplication::PixieApplication(
    const std::string& name,
    glm::ivec2 resolution,
    RenderAPI renderAPI
) {
	m_window = CreateWindow(name, resolution, renderAPI);
	m_renderer = m_window->GetRenderer();
	UserInput::Initialize(m_window->GetGLFWWindow());
}

void PixieApplication::Start() {
	OnStart();
	while (!m_window->GetShouldClose()) {
		GlobalTimer::StartTimer("Frame");

		Time::Update();

		GlobalTimer::StartTimer("Acquire");
		bool hasFrame = m_renderer->BeginFrame();
		GlobalTimer::StopTimer("Acquire");

		if (!hasFrame) {
			UserInput::Reset();
			m_window->PollEvents();
			GlobalTimer::StopTimer("Frame");
			continue;
		}

		GlobalTimer::StartTimer("Record");
		BeforeDrawFrame();
		m_renderer->BeginRenderPass();
		OnDrawFrame();
		m_renderer->EndRenderPass();
		AfterDrawFrame();
		GlobalTimer::StopTimer("Record");

		GlobalTimer::StartTimer("Present");
		m_renderer->EndFrame();
		GlobalTimer::StopTimer("Present");

		if (m_renderer->GetRenderAPI() == RenderAPI::OpenGL)
			m_window->SwapBuffers();

		GlobalTimer::StartTimer("Events");
		UserInput::Reset();
		m_window->PollEvents();
		GlobalTimer::StopTimer("Events");

		GlobalTimer::StopTimer("Frame");
	}
	OnClose();
};

} // namespace PixieApp
