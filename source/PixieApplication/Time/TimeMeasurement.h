#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <string>

namespace PixieApp {

class TimeMeasurement {
  public:
	using Clock = std::chrono::steady_clock;
	static constexpr size_t kWindow = 120;

	std::string name;
	Clock::time_point start{};
	Clock::time_point end{};
	double lastDelta = 0.0;

	void Push(double seconds);

	double Average() const;
	double Min() const;
	double Max() const;
	double Percentile(double p) const;
	double FPS() const;

  private:
	std::array<double, kWindow> m_samples{};
	size_t m_head = 0;
	size_t m_count = 0;
};

} // namespace PixieApp
