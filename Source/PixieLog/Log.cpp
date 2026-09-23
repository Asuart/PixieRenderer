#include "Log.h"

#include <array>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <mutex>
#include <unordered_set>

namespace PixieLog {

namespace {

std::mutex g_mutex;
LogCallback g_callback;
LogLevel g_minLevel = LogLevel::Trace;

std::unordered_set<uint32_t> g_disabledCategories;

constexpr std::array<const char*, 6> cLevelNames = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRIT" };

} // namespace

const char* ToString(LogLevel level) {
	const auto i = static_cast<size_t>(level);
	return i < cLevelNames.size() ? cLevelNames[i] : "?";
}

void Log::SetCallback(LogCallback cb) {
	std::lock_guard lk(g_mutex);
	g_callback = std::move(cb);
}

void Log::SetMinLevel(LogLevel lvl) {
	std::lock_guard lk(g_mutex);
	g_minLevel = lvl;
}

LogLevel Log::GetMinLevel() {
	std::lock_guard lk(g_mutex);
	return g_minLevel;
}

void Log::EnableCategory(LogCategory c) {
	std::lock_guard lk(g_mutex);
	g_disabledCategories.erase(c.Id());
}

void Log::DisableCategory(LogCategory c) {
	std::lock_guard lk(g_mutex);
	g_disabledCategories.insert(c.Id());
}

void Log::EnableCategory(std::string_view name) {
	EnableCategory(LogCategory{ name });
}

void Log::DisableCategory(std::string_view name) {
	DisableCategory(LogCategory{ name });
}

void Log::EnableAllCategories() {
	std::lock_guard lk(g_mutex);
	g_disabledCategories.clear();
}

void Log::DisableAllCategories() {
	std::lock_guard lk(g_mutex);
	g_disabledCategories.clear();
	for (uint32_t i = 0; i < LogCategoryRegistry::Count(); i++) {
		g_disabledCategories.insert(i);
	}
}

bool Log::IsCategoryEnabled(LogCategory c) {
	std::lock_guard lk(g_mutex);
	return g_disabledCategories.find(c.Id()) == g_disabledCategories.end();
}

bool Log::IsEnabled(LogLevel lvl, LogCategory cat) noexcept {
	std::lock_guard lk(g_mutex);
	if (static_cast<uint8_t>(lvl) < static_cast<uint8_t>(g_minLevel)) {
		return false;
	}
	return g_disabledCategories.find(cat.Id()) == g_disabledCategories.end();
}

void Log::Write(LogLevel level, LogCategory category, std::string_view message, std::source_location location) {
	LogCallback cb;
	{
		std::lock_guard lk(g_mutex);
		if (static_cast<uint8_t>(level) < static_cast<uint8_t>(g_minLevel)) {
			return;
		}
		if (g_disabledCategories.find(category.Id()) != g_disabledCategories.end()) {
			return;
		}
		cb = g_callback;
	}
	if (!cb) {
		return;
	}

	LogMessage msg{ level, category, message, std::chrono::system_clock::now(), location };
	cb(msg);
}

void Log::InstallDefaultCallback() {
	SetCallback([](const LogMessage& m) {
		const std::time_t t = std::chrono::system_clock::to_time_t(m.time);
		std::tm tm{};
#ifdef _WIN32
		localtime_s(&tm, &t);
#else
		localtime_r(&t, &tm);
#endif
		char timeBuf[16];
		std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &tm);

		const bool isError = static_cast<uint8_t>(m.level) >= static_cast<uint8_t>(LogLevel::Error);
		std::ostream& os = isError ? std::cerr : std::cout;

		os << timeBuf << " [" << ToString(m.level) << "]"
		   << " [" << m.category.Name() << "] " << m.message;

		if (static_cast<uint8_t>(m.level) <= static_cast<uint8_t>(LogLevel::Debug)) {
			os << "  (" << m.location.file_name() << ":" << m.location.line() << ")";
		}
		os << "\n";
	});
}

} // namespace PixieLog
