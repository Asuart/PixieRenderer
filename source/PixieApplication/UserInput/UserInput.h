#pragma once
#include <array>
#include <cstdint>

#include <glm/glm.hpp>

struct GLFWwindow;

namespace PixieApp {

class UserInput {
  public:
	static void Initialize(GLFWwindow* window);
	static void Shutdown();

	static void Reset();

	static bool IsKeyDown(int key);     
	static bool IsKeyPressed(int key); 
	static bool IsKeyReleased(int key); 

	static bool IsMouseButtonDown(int button);
	static bool IsMouseButtonPressed(int button);
	static bool IsMouseButtonReleased(int button);

	static glm::dvec2 GetMousePosition();
	static glm::dvec2 GetMouseDelta();
	static glm::dvec2 GetMouseScroll();

	static void SetCursorCaptured(bool captured);
	static bool IsCursorCaptured();

  private:
	static constexpr size_t kKeyCount = 512;
	static constexpr size_t kMouseCount = 16;

	inline static GLFWwindow* s_window = nullptr;

	inline static std::array<uint8_t, kKeyCount> s_keyDown{};
	inline static std::array<uint8_t, kKeyCount> s_keyPressed{};
	inline static std::array<uint8_t, kKeyCount> s_keyReleased{};

	inline static std::array<uint8_t, kMouseCount> s_mouseDown{};
	inline static std::array<uint8_t, kMouseCount> s_mousePressed{};
	inline static std::array<uint8_t, kMouseCount> s_mouseReleased{};

	inline static glm::dvec2 s_mousePos{ 0.0 };
	inline static glm::dvec2 s_mouseDelta{ 0.0 };
	inline static glm::dvec2 s_mouseScroll{ 0.0 };
	inline static bool s_firstMouse = true;
	inline static bool s_cursorCaptured = false;
	inline static bool s_hasFocus = true;

	static void KeyCallback(GLFWwindow*, int key, int scancode, int action, int mods);
	static void MouseButtonCallback(GLFWwindow*, int button, int action, int mods);
	static void CursorPosCallback(GLFWwindow*, double x, double y);
	static void ScrollCallback(GLFWwindow*, double xoff, double yoff);
	static void WindowFocusCallback(GLFWwindow*, int focused);
};

} // namespace PixieApp
