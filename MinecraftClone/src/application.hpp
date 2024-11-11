#ifndef SPARK_APPLICATION_HPP
#define SPARK_APPLICATION_HPP

#include "window.hpp"
#include "event_handler.hpp"
#include "thread_pool.hpp"

namespace MC {
	class Application {
	public:
		using ApplicationFunction = std::function<void(Application&)>;
		template<typename _Ty> using ApplicationEventFunction = std::function<void(Application&, const EventPtr<_Ty>&)>;
		template<typename... EventTypes> using ApplicationMultiEventFunction = std::function<void(Application&, const MultiEventPtr<EventTypes...>&)>;
		using ApplicationFunctionList = std::vector<std::pair<ApplicationFunction, FunctionSettings>>;

		Application(f32 delta_time = 60, std::unique_ptr<ThreadPool> tp = std::make_unique<ThreadPool>(std::thread::hardware_concurrency()));
		~Application();

		void         Start();
		void         Shutdown();
		const bool   Running() const;
		Application& AddStartupFunction(const ApplicationFunction& fn, const FunctionSettings settings = {});
		Application& AddUpdateFunction(const ApplicationFunction& fn, const FunctionSettings settings = {});
		Application& AddShutdownFunction(const ApplicationFunction& fn, const FunctionSettings settings = {});


		template<IsEvent EventType> Application& AddEventFunction(const ApplicationEventFunction<EventType>& fn, const FunctionSettings settings = {});

		template<IsEvent... EventTypes> Application& AddEventFunction(const ApplicationMultiEventFunction<EventTypes...>& fn, const FunctionSettings settings = {});

		Application& AddAllEventsFunction(const ApplicationEventFunction<IEvent>& fn, const FunctionSettings settings = {});

		Application& CreateWindow(const std::string& title, i32 width, i32 height);
		Window& GetWindow() const;
		EventHandler& GetEventHandler() const;
		ThreadPool& GetThreadPool() const;

		void SetDeltaTime(f32 delta_time);

	private:
		void RunStartupFunctions();
		void RunUpdateFunctions();
		void RunShutdownFunctions();
		void Update();

		struct IUpdateFunctionWrapper {
			virtual ~IUpdateFunctionWrapper() = default;
			virtual void Execute(Application& app) = 0;
		};

		struct IQueryEventFunctionWrapper {
			virtual ~IQueryEventFunctionWrapper() = default;
			virtual void Execute(Application& app, const EventPtr<IEvent>& event) = 0;
		};


		std::unique_ptr<ThreadPool>                                             m_thread_pool;
		std::unique_ptr<EventHandler>                                           m_event_handler;
		std::unique_ptr<Window>                                                 m_window;
		ApplicationFunctionList                                           m_startup_functions;
		ApplicationFunctionList                                           m_update_functions;
		ApplicationFunctionList                                           m_shutdown_functions;
		std::vector<std::pair<std::unique_ptr<IUpdateFunctionWrapper>, FunctionSettings>> m_query_functions;
		std::vector<std::unique_ptr<IQueryEventFunctionWrapper>>                     m_query_event_functions;
		f32                                                               m_delta_time;
		std::mutex                                                        m_mutex;
	};

	// Implementation of template methods

	template<IsEvent EventType> Application& Application::AddEventFunction(const ApplicationEventFunction<EventType>& fn, const FunctionSettings settings) {
		m_event_handler->SubscribeToEvent<EventType>(
			[this, fn, settings](const EventPtr<EventType>& event) {
				std::unique_lock<std::mutex> lock;
				if (settings.threaded)
					lock = std::unique_lock(m_mutex);
				fn(*this, event);
			},
			settings);
		return *this;
	}

	template<IsEvent... EventTypes> Application& Application::AddEventFunction(const ApplicationMultiEventFunction<EventTypes...>& fn, const FunctionSettings settings) {
		m_event_handler->SubscribeToMultipleEvents<EventTypes...>(
			[this, fn, settings](const MultiEventPtr<EventTypes...>& event) {
				std::unique_lock<std::mutex> lock;
				if (settings.threaded)
					lock = std::unique_lock(m_mutex);
				fn(*this, event);
			},
			settings);
		return *this;
	}

} // namespace Spark

#endif