#include "TimeMeasurement.h"

namespace PixieApp {

void TimeMeasurement::Push(double seconds) {
	m_samples[m_head] = seconds;
	m_head = (m_head + 1) % kWindow;
	m_count = std::min(m_count + 1, kWindow);
}

double TimeMeasurement::Average() const {
	if (m_count == 0) {
		return 0.0;
	}
	double s = 0.0;
	for (size_t i = 0; i < m_count; i++) {
		s += m_samples[i];
	}
	return s / m_count;
}

double TimeMeasurement::Min() const {
	if (m_count == 0) {
		return 0.0;
	}
	double m = m_samples[0];
	for (size_t i = 1; i < m_count; i++) {
		m = std::min(m, m_samples[i]);
	}
	return m;
}

double TimeMeasurement::Max() const {
	if (m_count == 0) {
		return 0.0;
	}
	double m = m_samples[0];
	for (size_t i = 1; i < m_count; i++) {
		m = std::max(m, m_samples[i]);
	}
	return m;
}

double TimeMeasurement::Percentile(double p) const {
	if (m_count == 0) {
		return 0.0;
	}
	std::array<double, kWindow> c{};
	std::copy(m_samples.begin(), m_samples.begin() + m_count, c.begin());
	std::sort(c.begin(), c.begin() + m_count);
	return c[static_cast<size_t>(p * (m_count - 1))];
}

double TimeMeasurement::FPS() const {
	double a = Average();
	return a > 0.0 ? 1.0 / a : 0.0;
}

} // namespace PixieApp
