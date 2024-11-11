#include "window.hpp"
#include "event.hpp"
#include "log.hpp"


namespace MC {
	Window::Window(const std::string& title, i32 width, i32 height, EventHandler& event_handler)
		: m_window_data(std::make_unique<WindowData>(title, width, height, event_handler)) {
		CreateWindow(width, height, title);
		
		// Subscribes to make sure window gets resized properly
		event_handler.SubscribeToEvent<WindowResizedEvent>([this](const EventPtr<WindowResizedEvent>& event) { SetSize(event->width, event->height); });
	}

	Window::~Window() { DestroyWindow(); }

	void Window::CreateWindow(i32 width, i32 height, const std::string& title) {

		glfwInit();

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		m_window = glfwCreateWindow(m_window_data->width, m_window_data->height, m_window_data->title.c_str(), nullptr, nullptr);
		glfwMakeContextCurrent(m_window);

		glfwSetWindowUserPointer(m_window, m_window_data.get());

		glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, i32 width, i32 height) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.width = width;
			data.height = height;
			data.event_handler.PublishEvent<WindowResizedEvent>(std::make_shared<WindowResizedEvent>(width, height));
			});

		glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			data.event_handler.PublishEvent<WindowClosedEvent>(std::make_shared<WindowClosedEvent>());
			});

		glfwSetKeyCallback(m_window, [](GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action) {
			case GLFW_PRESS: {
				data.event_handler.PublishEvent<KeyPressedEvent>(std::make_shared<KeyPressedEvent>(key, 0));
				break;
			}
			case GLFW_RELEASE: {
				data.event_handler.PublishEvent<KeyReleasedEvent>(std::make_shared<KeyReleasedEvent>(key));
				break;
			}
			case GLFW_REPEAT: {
				data.event_handler.PublishEvent<KeyHeldEvent>(std::make_shared<KeyHeldEvent>(key));
				break;
			}
			}
			});

		glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, i32 button, i32 action, i32 mods) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action) {
			case GLFW_PRESS: {
				data.event_handler.PublishEvent<MouseButtonPressedEvent>(std::make_shared<MouseButtonPressedEvent>(button));
				break;
			}
			case GLFW_RELEASE: {
				data.event_handler.PublishEvent<MouseButtonReleasedEvent>(std::make_shared<MouseButtonReleasedEvent>(button));
				break;
			}
			}
			});

		glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, f64 x, f64 y) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			data.event_handler.PublishEvent<MouseMovedEvent>(std::make_shared<MouseMovedEvent>(x, y));
			});

		glfwSetScrollCallback(m_window, [](GLFWwindow* window, f64 x, f64 y) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			data.event_handler.PublishEvent<MouseScrolledEvent>(std::make_shared<MouseScrolledEvent>(x, y));
			});

		glewInit();
	}

	void Window::DestroyWindow() {
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	void Window::Update() {
		glfwSwapBuffers(m_window);
		glfwPollEvents();
	}

	bool Window::Running() { return !glfwWindowShouldClose(m_window); }

	void Window::SetTitle(const std::string& title) {
		m_window_data->title = title;
		glfwSetWindowTitle(m_window, m_window_data->title.c_str());
	}

	void Window::SetSize(i32 width, i32 height) {
		m_window_data->width = width;
		m_window_data->height = height;
		glfwSetWindowSize(m_window, m_window_data->width, m_window_data->height);
	}

	WindowData& Window::GetWindowData() const { return *m_window_data; }

	GLFWwindow* Window::GetNativeWindow() const { return m_window; }
} // namespace Spark