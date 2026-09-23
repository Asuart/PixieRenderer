#include "LogCategory.h"

#include <mutex>
#include <unordered_map>

namespace PixieLog {

namespace {

struct Registry {
	std::mutex mutex;
	std::unordered_map<std::string, uint32_t> nameToId;
	std::vector<std::string> idToName;
};

Registry& GetRegistry() {
	static Registry r;
	return r;
}

} // namespace

LogCategory::LogCategory(std::string_view name) {
	if (name.empty()) {
		return;
	}
	*this = LogCategoryRegistry::Intern(name);
}

std::string_view LogCategory::Name() const noexcept {
	return LogCategoryRegistry::NameOf(m_id);
}

LogCategory LogCategoryRegistry::Intern(std::string_view name) {
	if (name.empty()) {
		return {};
	}

	auto& r = GetRegistry();
	std::lock_guard lk(r.mutex);

	const std::string key(name);
	if (auto it = r.nameToId.find(key); it != r.nameToId.end()) {
		return LogCategory(it->second);
	}

	const uint32_t id = static_cast<uint32_t>(r.idToName.size());
	r.idToName.push_back(key);
	r.nameToId.emplace(key, id);
	return LogCategory(id);
}

std::string_view LogCategoryRegistry::NameOf(uint32_t id) {
	auto& r = GetRegistry();
	std::lock_guard lk(r.mutex);
	if (id >= r.idToName.size()) {
		return {};
	}
	return r.idToName[id];
}

uint32_t LogCategoryRegistry::Count() {
	auto& r = GetRegistry();
	std::lock_guard lk(r.mutex);
	return static_cast<uint32_t>(r.idToName.size());
}

std::vector<LogCategory> LogCategoryRegistry::All() {
	auto& r = GetRegistry();
	std::lock_guard lk(r.mutex);
	std::vector<LogCategory> out;
	out.reserve(r.idToName.size());
	for (uint32_t i = 0; i < r.idToName.size(); i++) {
		out.emplace_back(LogCategory{ i });
	}
	return out;
}

} // namespace PixieLog
