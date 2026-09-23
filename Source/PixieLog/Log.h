#pragma once
#include "LogCategories.h"
#include "LogCategory.h"

#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <source_location>
#include <string>
#include <string_view>

namespace PixieLog {

enum class LogLevel : uint8_t { Trace, Debug, Info, Warning, Error, Critical };

struct LogMessage {
	LogLevel level = LogLevel::Info;
	LogCategory category;
	std::string_view message;
	std::chrono::system_clock::time_point time;
	std::source_location location;
};

using LogCallback = std::function<void(const LogMessage&)>;

class Log {
  public:
	static void SetCallback(LogCallback cb);
	static void InstallDefaultCallback();

	static void SetMinLevel(LogLevel lvl);
	static LogLevel GetMinLevel();

	static void EnableCategory(LogCategory c);
	static void DisableCategory(LogCategory c);
	static void EnableCategory(std::string_view name);
	static void DisableCategory(std::string_view name);
	static void EnableAllCategories();
	static void DisableAllCategories();
	static bool IsCategoryEnabled(LogCategory c);

	static bool IsEnabled(LogLevel lvl, LogCategory cat) noexcept;

	static void Write(
	    LogLevel level,
	    LogCategory category,
	    std::string_view message,
	    std::source_location location = std::source_location::current()
	);

	template <typename... Args> static void Trace(LogCategory cat, std::format_string<Args...> fmt, Args&&... args) {
		if (!IsEnabled(LogLevel::Trace, cat)) {
			return;
		}
		Write(LogLevel::Trace, cat, std::format(fmt, std::forward<Args>(args)...));
	}

	template <typename... Args> static void Debug(LogCategory cat, std::format_string<Args...> fmt, Args&&... args) {
		if (!IsEnabled(LogLevel::Debug, cat)) {
			return;
		}
		Write(LogLevel::Debug, cat, std::format(fmt, std::forward<Args>(args)...));
	}

	template <typename... Args> static void Info(LogCategory cat, std::format_string<Args...> fmt, Args&&... args) {
		if (!IsEnabled(LogLevel::Info, cat)) {
			return;
		}
		Write(LogLevel::Info, cat, std::format(fmt, std::forward<Args>(args)...));
	}

	template <typename... Args> static void Warning(LogCategory cat, std::format_string<Args...> fmt, Args&&... args) {
		if (!IsEnabled(LogLevel::Warning, cat)) {
			return;
		}
		Write(LogLevel::Warning, cat, std::format(fmt, std::forward<Args>(args)...));
	}

	template <typename... Args> static void Error(LogCategory cat, std::format_string<Args...> fmt, Args&&... args) {
		if (!IsEnabled(LogLevel::Error, cat)) {
			return;
		}
		Write(LogLevel::Error, cat, std::format(fmt, std::forward<Args>(args)...));
	}

	template <typename... Args> static void Critical(LogCategory cat, std::format_string<Args...> fmt, Args&&... args) {
		if (!IsEnabled(LogLevel::Critical, cat)) {
			return;
		}
		Write(LogLevel::Critical, cat, std::format(fmt, std::forward<Args>(args)...));
	}
};

} // namespace PixieLog
