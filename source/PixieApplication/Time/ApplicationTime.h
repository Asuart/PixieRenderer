#pragma once
#include <chrono>

namespace PixieApp {

class Time {
  public:
	using Clock = std::chrono::steady_clock;

	inline static Clock::time_point GetCurrentTime() {
		return Clock::now();
	}

	inline static double Seconds(Clock::time_point tp) {
		return std::chrono::duration<double>(tp.time_since_epoch()).count();
	}

	inline static const float fixedDeltaTime = 0.002f;
	inline static float deltaTime = 0.0f;
	inline static double rawDeltaTime = 0.0;
	inline static Clock::time_point lastTime = Clock::now();

	static void Update();
};

} // namespace PixieApp
