#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include "types.hpp"
#include "event_handler.hpp"

namespace MC {
	struct WindowData {
		WindowData(const std::string& title, i32 width, i32 height, EventHandler& event_handler) :
			title(title),
			width(width),
			height(height),
			event_handler(event_handler) { }

		std::string title;
		i32 width;
		i32 height;
		EventHandler& event_handler;
	};

	class Window {
	public:
		Window(const std::string& title, i32 width, i32 height, EventHandler& event_handler);
		~Window();
		void           DestroyWindow();
		void           Update();
		bool           Running();
		void Shutdown();
		void           SetTitle(const std::string&);
		void           SetSize(i32 width, i32 height);
		WindowData& GetWindowData() const;
		GLFWwindow* GetNativeWindow() const;

	private:
		void CreateWindow(i32 width, i32 height, const std::string& title);

	private:
		GLFWwindow* m_window;
		bool m_running;
		std::unique_ptr<WindowData> m_window_data;
	};
}

#endif