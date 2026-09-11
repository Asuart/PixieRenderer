#include "GlobalTimer.h"

namespace PixieApp {

void GlobalTimer::StartTimer(const std::string& name) {
	Timer(name).start = TimeMeasurement::Clock::now();
}

void GlobalTimer::StopTimer(const std::string& name) {
	auto& t = Timer(name);
	t.end = TimeMeasurement::Clock::now();
	t.lastDelta = std::chrono::duration<double>(t.end - t.start).count();
	t.Push(t.lastDelta);
}

TimeMeasurement* GlobalTimer::FindTimer(const std::string& name) {
	auto it = s_timers.find(name);
	return it == s_timers.end() ? nullptr : &it->second;
}

const std::unordered_map<std::string, TimeMeasurement>& GlobalTimer::GetTimers() {
	return s_timers;
}

TimeMeasurement& GlobalTimer::Timer(const std::string& name) {
	auto [it, inserted] = s_timers.try_emplace(name);
	if (inserted) {
		it->second.name = name;
	}
	return it->second;
}

} // namespace PixieApp
