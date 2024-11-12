
#ifndef EVENT_HANDLER_HPP
#define EVENT_HANDLER_HPP

#include "thread_pool.hpp"
#include "event.hpp"
#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>

namespace MC {
	struct FunctionSettings {
		bool threaded = false;
		bool wait = false;
	};

	class EventHandler {
	public:
		EventHandler(ThreadPool& thread_pool)
			: m_thread_pool(thread_pool) {}
		~EventHandler() = default;

		template<IsEvent _Ty> void SubscribeToEvent(std::function<void(const EventPtr<_Ty>&)> handler, const FunctionSettings settings = {}) {
			std::lock_guard<std::mutex> lock(m_mutex);

			m_single_event_handlers[typeid(_Ty)].push_back({ [handler](const EventPtr<IEvent>& base_event) {
								   if (auto derived_event = std::dynamic_pointer_cast<_Ty>(base_event)) {
									   handler(derived_event);
								   }
							   },
							   settings });
		}

		template<IsEvent... EventTypes> void SubscribeToMultipleEvents(std::function<void(const MultiEventPtr<EventTypes...>&)> handler, const FunctionSettings settings = {}) {
			std::lock_guard<std::mutex> lock(m_mutex);
			auto                        wrapper = [handler](const EventPtr<IEvent>& base_event) {
				if ((std::dynamic_pointer_cast<EventTypes>(base_event) || ...)) {
					auto multi_event = CreateMultiEvent<EventTypes...>(base_event);
					if (multi_event) {
						handler(multi_event);
					}
				}
				};

			(RegisterMultiEventHandler<EventTypes>(wrapper, settings), ...);
		}

		void SubscribeToAllEvents(std::function<void(const EventPtr<IEvent>&)> handler, const FunctionSettings settings = {}) {
			std::lock_guard<std::mutex> lock(m_mutex);
			m_all_event_handlers.push_back({ handler, settings });
		}

		template<IsEvent _Ty> void PublishEvent(const EventPtr<_Ty>& event) {
			std::vector<std::pair<std::function<void(const EventPtr<IEvent>&)>, FunctionSettings>> handlers_to_call;

			{
				std::lock_guard<std::mutex> lock(m_mutex);

				auto single_it = m_single_event_handlers.find(typeid(_Ty));
				if (single_it != m_single_event_handlers.end()) {
					handlers_to_call.insert(handlers_to_call.end(), single_it->second.begin(), single_it->second.end());
				}

				auto multi_it = m_multi_event_handlers.find(typeid(_Ty));
				if (multi_it != m_multi_event_handlers.end()) {
					handlers_to_call.insert(handlers_to_call.end(), multi_it->second.begin(), multi_it->second.end());
				}

				handlers_to_call.insert(handlers_to_call.end(), m_all_event_handlers.begin(), m_all_event_handlers.end());
			}

			for (const auto& [handler, settings] : handlers_to_call) {
				if (settings.threaded)
					m_thread_pool.Enqueue(TaskPriority::VERY_HIGH, settings.wait, [handler, event]() { handler(event); });
				else {
					handler(event);
				}
			}
		}

	private:
		template<IsEvent _Ty> void RegisterMultiEventHandler(const std::function<void(const EventPtr<IEvent>&)>& handler, const FunctionSettings& settings) {
			m_multi_event_handlers[typeid(_Ty)].push_back({ handler, settings });
		}

		template<IsEvent... EventTypes> static MultiEventPtr<EventTypes...> CreateMultiEvent(const EventPtr<IEvent>& base_event) { return std::make_shared<MultiEvent<EventTypes...>>(base_event); }

		std::unordered_map<std::type_index, std::vector<std::pair<std::function<void(const EventPtr<IEvent>&)>, FunctionSettings>>> m_single_event_handlers;
		std::unordered_map<std::type_index, std::vector<std::pair<std::function<void(const EventPtr<IEvent>&)>, FunctionSettings>>> m_multi_event_handlers;
		std::vector<std::pair<std::function<void(const EventPtr<IEvent>&)>, FunctionSettings>>                          m_all_event_handlers;
		ThreadPool& m_thread_pool;
		std::mutex                                                                                       m_mutex;
	};
} // namespace Spark

#endif
