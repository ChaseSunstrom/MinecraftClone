#include "application.hpp"

namespace MC {

	Application::Application(f32 delta_time, std::unique_ptr<ThreadPool> tp)
		: m_thread_pool(std::move(tp))
		, m_event_handler(std::make_unique<EventHandler>(*m_thread_pool))
		, m_delta_time(delta_time) {}

	Application::~Application() {
		RunShutdownFunctions();
	}

	void Application::Start() {
		if (!m_window) {
			LOG_FATAL("Window has not been created!");
		}


		RunStartupFunctions();

		m_startup_functions.clear();

		while (Running()) {
			Update();

		}
	}

	void Application::Update() {
		RunUpdateFunctions();
		m_window->Update();
	}

	Application& Application::AddStartupFunction(const ApplicationFunction& fn, const FunctionSettings settings) {
		m_startup_functions.push_back({ fn, settings });
		return *this;
	}

	Application& Application::AddUpdateFunction(const ApplicationFunction& fn, const FunctionSettings settings) {
		m_update_functions.push_back({ fn, settings });
		return *this;
	}

	Application& Application::AddShutdownFunction(const ApplicationFunction& fn, const FunctionSettings settings) {
		m_shutdown_functions.push_back({ fn, settings });
		return *this;
	}

	Application& Application::AddAllEventsFunction(const ApplicationEventFunction<IEvent>& fn, const FunctionSettings settings) {
		m_event_handler->SubscribeToAllEvents(
			[this, fn, settings](const EventPtr<IEvent>& event) {
				std::unique_lock<std::mutex> lock;
				if (settings.threaded)
					lock = std::unique_lock(m_mutex);
				fn(*this, event);
			},
			settings);
		return *this;
	}

	void Application::RunStartupFunctions() {
		for (const auto& [fn, settings] : m_startup_functions) {
			if (settings.threaded) {
				m_thread_pool->Enqueue(TaskPriority::HIGH, settings.wait, [this, &fn, settings]() {
					std::unique_lock<std::mutex> lock;
					if (settings.threaded)
						lock = std::unique_lock(m_mutex);
					fn(*this);
					});
			}
			else {
				fn(*this);
			}
		}

		m_thread_pool->SyncRegisteredTasks();
	}

	void Application::RunUpdateFunctions() {
		for (const auto& [fn, settings] : m_update_functions) {
			if (settings.threaded) {
				m_thread_pool->Enqueue(TaskPriority::NORMAL, settings.wait, [this, &fn, settings]() {
					std::unique_lock<std::mutex> lock;
					if (settings.threaded)
						lock = std::unique_lock(m_mutex);
					fn(*this);
					});
			}
			else {
				fn(*this);
			}
		}

		for (const auto& [fn, settings] : m_query_functions) {
			if (settings.threaded) {
				m_thread_pool->Enqueue(TaskPriority::NORMAL, settings.wait, [this, &fn, settings]() {
					std::unique_lock<std::mutex> lock;
					if (settings.threaded)
						lock = std::unique_lock(m_mutex);
					fn->Execute(*this);
					});
			}
			else {
				fn->Execute(*this);
			}
		}

		m_thread_pool->SyncRegisteredTasks();
	}

	void Application::RunShutdownFunctions() {
		for (const auto& [fn, settings] : m_shutdown_functions) {
			if (settings.threaded) {
				m_thread_pool->Enqueue(TaskPriority::NORMAL, settings.wait, [this, &fn, settings]() {
					std::unique_lock<std::mutex> lock;
					if (settings.threaded)
						lock = std::unique_lock(m_mutex);
					fn(*this);
					});
			}
			else {
				fn(*this);
			}
		}

		m_thread_pool->SyncRegisteredTasks();
	}


	Application& Application::CreateWindow(const std::string& title, i32 width, i32 height) {
		m_window = std::make_unique<Window>(title, width, height, *m_event_handler);
		return *this;
	}


	Window& Application::GetWindow() const { return *m_window; }

	EventHandler& Application::GetEventHandler() const { return *m_event_handler; }

	const bool Application::Running() const { return m_window->Running(); }

	ThreadPool& Application::GetThreadPool() const { return *m_thread_pool; }

	void Application::SetDeltaTime(f32 delta_time) { m_delta_time = delta_time; }

} // namespace Spark