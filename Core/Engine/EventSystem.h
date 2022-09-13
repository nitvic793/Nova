#pragma once
//Event System Reference: https://medium.com/@savas/nomad-game-engine-part-7-the-event-system-45a809ccb68f

#include <Lib/ScopedPtr.h>
#include <Lib/StringHash.h>
#include "Memory.h"
#include <Lib/Map.h>
#include <vector>

namespace nv
{
	struct IEvent {};

	class EventHandlerBase
	{
	public:
		inline void Execute(IEvent* event)
		{
			CallEvent(event);
		}
		virtual ~EventHandlerBase() {}
	protected:
		virtual void CallEvent(IEvent* event) = 0;
	};

	template<typename T, typename EventType>
	class EventHandler : public EventHandlerBase
	{
	public:
		typedef void (T::* MemberFunction)(EventType*);
		virtual void CallEvent(IEvent* event) override;
		EventHandler(T* instancePtr, MemberFunction function) :
			mInstance(instancePtr), mEventFunction(function) {}
	private:
		T* mInstance;
		MemberFunction mEventFunction;
	};


	class EventBus
	{
	public:
		template<typename EventType>
		void Publish(EventType* event);

		template<typename T, typename EventType>
		void Subscribe(T* mInstance, void (T::* memberFunction)(EventType*));

	private:
		HashMap<std::string_view, std::vector<ScopedPtr<EventHandlerBase, true>>> mSubscribers;
	};

	template<typename EventType>
	inline void EventBus::Publish(EventType* event)
	{
		static constexpr auto typeName = TypeName<EventType>();
		std::vector<ScopedPtr<EventHandlerBase, true>>& handlers = mSubscribers[typeName];

		for (auto& handler : handlers)
		{
			handler->Execute(event);
		}
	}

	template<typename T, typename EventType>
	inline void EventBus::Subscribe(T* mInstance, void(T::* memberFunction)(EventType*))
	{
		constexpr auto typeName = TypeName<EventType>();
		std::vector<ScopedPtr<EventHandlerBase, true>>& handlers = mSubscribers[typeName];

		void* buffer = Alloc(sizeof(EventHandler<T, EventType>));

		EventHandler<T, EventType>* handler = new(buffer) EventHandler<T, EventType>(mInstance, memberFunction);
		ScopedPtr<EventHandlerBase, true> eventHandler = ScopedPtr<EventHandlerBase, true>((EventHandlerBase*)handler);

		handlers.push_back(std::move(eventHandler));
	}

	template<typename T, typename EventType>
	inline void EventHandler<T, EventType>::CallEvent(IEvent* event)
	{
		(mInstance->*mEventFunction)(static_cast<EventType*>(event));
	}

	extern EventBus gEventBus;
}