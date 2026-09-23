#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace PixieLog {

class LogCategory {
  public:
	LogCategory() = default;
	explicit LogCategory(std::string_view name);

	std::string_view Name() const noexcept;
	uint32_t Id() const noexcept {
		return m_id;
	}
	bool IsValid() const noexcept {
		return m_id != kInvalidId;
	}

	bool operator==(const LogCategory& o) const noexcept {
		return m_id == o.m_id;
	}
	bool operator!=(const LogCategory& o) const noexcept {
		return m_id != o.m_id;
	}

	static constexpr uint32_t kInvalidId = UINT32_MAX;

  private:
	uint32_t m_id = kInvalidId;

	explicit LogCategory(uint32_t id) noexcept : m_id(id) {
	}

	friend class LogCategoryRegistry;
};

struct LogCategoryHash {
	size_t operator()(const LogCategory& c) const noexcept {
		return std::hash<uint32_t>{}(c.Id());
	}
};

class LogCategoryRegistry {
  public:
	static LogCategory Intern(std::string_view name);
	static std::string_view NameOf(uint32_t id);
	static uint32_t Count();
	static std::vector<LogCategory> All();
};

} // namespace PixieLog
