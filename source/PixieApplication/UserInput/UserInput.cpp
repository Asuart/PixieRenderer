#include "UserInput.h"

#include <GLFW/glfw3.h>

namespace PixieApp {

void UserInput::Initialize(GLFWwindow* window) {
	s_window = window;
	if (!window)
		return;

	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, CursorPosCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	glfwSetWindowFocusCallback(window, WindowFocusCallback);

	s_keyDown.fill(0);
	s_keyPressed.fill(0);
	s_keyReleased.fill(0);
	s_mouseDown.fill(0);
	s_mousePressed.fill(0);
	s_mouseReleased.fill(0);
	s_mousePos = glm::dvec2(0.0);
	s_mouseDelta = glm::dvec2(0.0);
	s_mouseScroll = glm::dvec2(0.0);
	s_firstMouse = true;
	s_cursorCaptured = false;
	s_hasFocus = true;
}

void UserInput::Shutdown() {
	if (s_window) {
		glfwSetKeyCallback(s_window, nullptr);
		glfwSetMouseButtonCallback(s_window, nullptr);
		glfwSetCursorPosCallback(s_window, nullptr);
		glfwSetScrollCallback(s_window, nullptr);
		glfwSetWindowFocusCallback(s_window, nullptr);
	}
	s_window = nullptr;
}

void UserInput::Reset() {
	s_keyPressed.fill(0);
	s_keyReleased.fill(0);
	s_mousePressed.fill(0);
	s_mouseReleased.fill(0);
	s_mouseDelta = glm::dvec2(0.0);
	s_mouseScroll = glm::dvec2(0.0);
}

bool UserInput::IsKeyDown(int key) {
	return key >= 0 && static_cast<size_t>(key) < s_keyDown.size() && s_keyDown[key] != 0;
}

bool UserInput::IsKeyPressed(int key) {
	return key >= 0 && static_cast<size_t>(key) < s_keyPressed.size() && s_keyPressed[key] != 0;
}

bool UserInput::IsKeyReleased(int key) {
	return key >= 0 && static_cast<size_t>(key) < s_keyReleased.size() && s_keyReleased[key] != 0;
}

bool UserInput::IsMouseButtonDown(int b) {
	return b >= 0 && static_cast<size_t>(b) < s_mouseDown.size() && s_mouseDown[b] != 0;
}

bool UserInput::IsMouseButtonPressed(int b) {
	return b >= 0 && static_cast<size_t>(b) < s_mousePressed.size() && s_mousePressed[b] != 0;
}

bool UserInput::IsMouseButtonReleased(int b) {
	return b >= 0 && static_cast<size_t>(b) < s_mouseReleased.size() && s_mouseReleased[b] != 0;
}

glm::dvec2 UserInput::GetMousePosition() {
	return s_mousePos;
}

glm::dvec2 UserInput::GetMouseDelta() {
	return s_mouseDelta;
}

glm::dvec2 UserInput::GetMouseScroll() {
	return s_mouseScroll;
}

void UserInput::SetCursorCaptured(bool captured) {
	if (s_cursorCaptured == captured) {
		return;
	}
	s_cursorCaptured = captured;

	if (!s_window) {
		return;
	}

	glfwSetInputMode(s_window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

	if (captured && glfwRawMouseMotionSupported()) {
		glfwSetInputMode(s_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	} else {
		glfwSetInputMode(s_window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
	}

	s_firstMouse = true;
}

bool UserInput::IsCursorCaptured() {
	return s_cursorCaptured;
}

void UserInput::KeyCallback(GLFWwindow*, int key, int /*scancode*/, int action, int /*mods*/) {
	if (key < 0 || static_cast<size_t>(key) >= s_keyDown.size())
		return;
	switch (action) {
	case GLFW_PRESS:
		s_keyDown[key] = 1;
		s_keyPressed[key] = 1;
		break;
	case GLFW_RELEASE:
		s_keyDown[key] = 0;
		s_keyReleased[key] = 1;
		break;
	case GLFW_REPEAT:
		break;
	default:
		break;
	}
}

void UserInput::MouseButtonCallback(GLFWwindow*, int button, int action, int /*mods*/) {
	if (button < 0 || static_cast<size_t>(button) >= s_mouseDown.size())
		return;
	if (action == GLFW_PRESS) {
		s_mouseDown[button] = 1;
		s_mousePressed[button] = 1;
	} else if (action == GLFW_RELEASE) {
		s_mouseDown[button] = 0;
		s_mouseReleased[button] = 1;
	}
}

void UserInput::CursorPosCallback(GLFWwindow*, double x, double y) {
	const glm::dvec2 newPos(x, y);
	if (s_firstMouse) {
		s_mousePos = newPos;
		s_firstMouse = false;
		return;
	}
	s_mouseDelta += newPos - s_mousePos;
	s_mousePos = newPos;
}

void UserInput::ScrollCallback(GLFWwindow*, double xoff, double yoff) {
	s_mouseScroll += glm::dvec2(xoff, yoff);
}

void UserInput::WindowFocusCallback(GLFWwindow*, int focused) {
	s_hasFocus = (focused == GLFW_TRUE);
	if (!s_hasFocus) {
		s_keyDown.fill(0);
		s_mouseDown.fill(0);
	}
}

} // namespace PixieApp
