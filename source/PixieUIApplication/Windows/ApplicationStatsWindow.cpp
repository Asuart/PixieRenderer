#include "ApplicationStatsWindow.h"

#include <imgui.h>

#include <algorithm>
#include <string>
#include <vector>

#include <PixieApplication/Time/GlobalTimer.h>

using namespace PixieRenderer;
using namespace PixieApp;

namespace PixieUI {

ApplicationStatsWindow::ApplicationStatsWindow(UI* ui, IRenderer* renderer)
    : UIWindow(ui, renderer) {
}

void ApplicationStatsWindow::Draw() {
	ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Stats", nullptr)) {
		ImGui::End();
		return;
	}

	std::vector<const TimeMeasurement*> timers;
	timers.reserve(GlobalTimer::GetTimers().size());
	for (const auto& [name, t] : GlobalTimer::GetTimers()) {
		timers.push_back(&t);
	}
	std::sort(timers.begin(), timers.end(), [](const TimeMeasurement* a, const TimeMeasurement* b) {
		return a->name < b->name;
	});

	if (ImGui::BeginTable(
	        "timers",
	        6,
	        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp
	    )) {
		ImGui::TableSetupColumn("Timer");
		ImGui::TableSetupColumn("avg");
		ImGui::TableSetupColumn("min");
		ImGui::TableSetupColumn("max");
		ImGui::TableSetupColumn("p99");
		ImGui::TableSetupColumn("FPS");
		ImGui::TableHeadersRow();

		for (const TimeMeasurement* t : timers) {
			const double msAvg = t->Average() * 1000.0;
			const double msMin = t->Min() * 1000.0;
			const double msMax = t->Max() * 1000.0;
			const double msP99 = t->Percentile(0.99) * 1000.0;
			const double fps = t->FPS();

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(t->name.c_str());

			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%.3f ms", msAvg);

			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%.3f ms", msMin);

			ImGui::TableSetColumnIndex(3);
			if (msMax > msAvg * 1.5) {
				ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%.3f ms", msMax);
			} else {
				ImGui::Text("%.3f ms", msMax);
			}

			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%.3f ms", msP99);

			ImGui::TableSetColumnIndex(5);
			ImGui::Text("%.1f", fps);
		}
		ImGui::EndTable();
	}

	if (auto* frame = GlobalTimer::FindTimer("Frame")) {
		ImGui::Separator();
		const double onePercentLow = 1.0 / std::max(frame->Percentile(0.99), 1e-9);
		ImGui::Text("Frame: avg %.1f FPS | 1%% low %.1f FPS", frame->FPS(), onePercentLow);
	}

	ImGui::End();
}

} // namespace PixieUI
