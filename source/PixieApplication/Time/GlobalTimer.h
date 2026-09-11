#pragma once
#include <string>
#include <unordered_map>

#include "TimeMeasurement.h"

namespace PixieApp {

class GlobalTimer {
  public:
	static void StartTimer(const std::string& name);
	static void StopTimer(const std::string& name);

	static TimeMeasurement* FindTimer(const std::string& name);
	static const std::unordered_map<std::string, TimeMeasurement>& GetTimers();

  private:
	inline static std::unordered_map<std::string, TimeMeasurement> s_timers;

	static TimeMeasurement& Timer(const std::string& name);
};

} // namespace PixieApp
