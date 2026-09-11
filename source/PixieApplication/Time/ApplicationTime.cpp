#include "ApplicationTime.h"

namespace PixieApp {

void Time::Update() {
	auto now = Clock::now();
	rawDeltaTime = std::chrono::duration<double>(now - lastTime).count();
	lastTime = now;
	deltaTime = static_cast<float>(rawDeltaTime);
}

} // namespace PixieApp
