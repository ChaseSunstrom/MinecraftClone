#ifndef EVENT_HPP
#define EVENT_HPP

#include "types.hpp"
#include <memory>
#include <variant>
#include <concepts>
#include <tuple>

namespace MC {
	class IEvent {
	public:
		virtual ~IEvent() = default;
	};

	template<typename _Ty> class Event : public virtual IEvent {
	public:
		virtual ~Event() = default;
	};

	template<typename _Ty>
	concept IsEvent = std::derived_from<_Ty, Event<_Ty>>;

	template<typename EventType> using EventPtr = std::shared_ptr<EventType>;

	template<typename... EventTypes> class MultiEvent : public IEvent {
	public:
		using VariantType = std::variant<std::shared_ptr<EventTypes>...>;

		MultiEvent(const EventPtr<IEvent>& base_event) { ((TrySetEvent<EventTypes>(base_event)) || ...); }

		template<typename _Ty> bool Is() const { return std::holds_alternative<std::shared_ptr<_Ty>>(event); }

		template<typename _Ty> const std::shared_ptr<_Ty>& Get() const { return std::get<std::shared_ptr<_Ty>>(event); }

		static auto GetEventTypes() { return std::tuple<EventTypes...>{}; }

	private:
		template<typename _Ty> bool TrySetEvent(const EventPtr<IEvent>& base_event) {
			if (auto derived = std::dynamic_pointer_cast<_Ty>(base_event)) {
				event = derived;
				return true;
			}
			return false;
		}

		VariantType event;
	};

	template<typename... EventTypes> using MultiEventPtr = std::shared_ptr<MultiEvent<EventTypes...>>;

	struct KeyPressedEvent : public Event<KeyPressedEvent> {
		KeyPressedEvent(i32 key, i32 repeat)
			: key(key)
			, repeat(repeat) {}
		i32 key;
		i32 repeat;
	};

	struct KeyReleasedEvent : public Event<KeyReleasedEvent> {
		KeyReleasedEvent(i32 key)
			: key(key) {}
		i32 key;
	};

	struct KeyHeldEvent : public Event<KeyHeldEvent> {
		KeyHeldEvent(i32 key)
			: key(key) {}
		i32 key;
	};

	struct MouseMovedEvent : public Event<MouseMovedEvent> {
		MouseMovedEvent(f64 xpos, f64 ypos)
			: xpos(xpos)
			, ypos(ypos) {}
		f64 xpos;
		f64 ypos;
	};

	struct MouseButtonPressedEvent : public Event<MouseButtonPressedEvent> {
		MouseButtonPressedEvent(i32 button)
			: button(button) {}
		i32 button;
	};

	struct MouseButtonReleasedEvent : public Event<MouseButtonReleasedEvent> {
		MouseButtonReleasedEvent(i32 button)
			: button(button) {}
		i32 button;
	};

	struct MouseScrolledEvent : public Event<MouseScrolledEvent> {
		MouseScrolledEvent(f64 x, f64 y)
			: x(x)
			, y(y) {}
		f64 x;
		f64 y;
	};
	
	struct WindowResizedEvent : public Event<WindowResizedEvent> {
		WindowResizedEvent(i32 width, i32 height)
			: width(width)
			, height(height) {}
		i32 width;
		i32 height;
	};

	struct WindowClosedEvent : public Event<WindowClosedEvent> {};

}


#endif