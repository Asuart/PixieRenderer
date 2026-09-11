#include "Timer.h"
#include "PixieApplication/Log/Log.h"

namespace PixieApp {

Timer::Timer(const std::string& name) : m_name(name) {
	m_start = TimeMeasurement::Clock::now();
}

Timer::~Timer() {
	auto end = TimeMeasurement::Clock::now();
	double ms = std::chrono::duration<double, std::milli>(end - m_start).count();
	Log::Message("{}: {:.3f} ms", m_name, ms);
}

} // namespace PixieApp
