// eventpp library
// Copyright (C) 2018 Wang Qi (wqking)
// Github: https://github.com/wqking/eventpp
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//   http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef EVENTDISPATCHER_H_319010983013
#define EVENTDISPATCHER_H_319010983013

#include "callbacklist.h"

#include <string>
#include <functional>
#include <type_traits>
#include <map>
#include <mutex>
#include <algorithm>
#include <memory>

namespace eventpp {

struct EventGetterBase {};

template <typename E>
struct PrimaryEventGetter : public EventGetterBase
{
	using Event = E;

	template <typename U, typename ...Args>
	static Event getEvent(U && e, const Args &...) {
		return e;
	}
};

struct ArgumentPassingAutoDetect
{
	enum {
		canIncludeEventType = true,
		canExcludeEventType = true
	};
};

struct ArgumentPassingIncludeEvent
{
	enum {
		canIncludeEventType = true,
		canExcludeEventType = false
	};
};

struct ArgumentPassingExcludeEvent
{
	enum {
		canIncludeEventType = false,
		canExcludeEventType = true
	};
};

namespace internal_ {

template <size_t ...Indexes>
struct IndexSequence
{
};

template <size_t N, size_t ...Indexes>
struct MakeIndexSequence : MakeIndexSequence <N - 1, N - 1, Indexes...>
{
};

template <std::size_t ...Indexes>
struct MakeIndexSequence<0, Indexes...>
{
	using Type = IndexSequence<Indexes...>;
};

template <typename T>
struct CounterGuard
{
	explicit CounterGuard(T & v) : value(v) {
		++value;
	}

	~CounterGuard() {
		--value;
	}

	T & value;
};

template <
	typename EventGetterType,
	typename CallbackType,
	typename ArgumentPassingMode,
	typename Threading,
	typename ReturnType, typename ...Args
>
class EventDispatcherBase;

template <
	typename EventGetterType,
	typename CallbackType,
	typename ArgumentPassingMode,
	typename Threading,
	typename ReturnType, typename ...Args
>
class EventDispatcherBase <
	EventGetterType,
	CallbackType,
	ArgumentPassingMode,
	Threading,
	ReturnType (Args...)
>
{
protected:
	using EventGetter = typename std::conditional<
		std::is_base_of<EventGetterBase, EventGetterType>::value,
		EventGetterType,
		PrimaryEventGetter<EventGetterType>
	>::type;

	using Mutex = typename Threading::Mutex;

	using Callback_ = typename std::conditional<
		std::is_same<CallbackType, void>::value,
		std::function<ReturnType (Args...)>,
		CallbackType
	>::type;
	using CallbackList_ = CallbackList<ReturnType (Args...), Callback_, Threading>;

	using Filter = std::function<bool (typename std::add_lvalue_reference<Args>::type...)>;
	using FilterList = CallbackList<bool (Args...), Filter, Threading>;

	enum {
		canIncludeEventType = ArgumentPassingMode::canIncludeEventType,
		canExcludeEventType = ArgumentPassingMode::canExcludeEventType
	};

	using Handle_ = typename CallbackList_::Handle;
	using Event_ = typename EventGetter::Event;

public:
	using Handle = Handle_;
	using Callback = Callback_;
	using Event = Event_;
	using FilterHandle = typename FilterList::Handle;;

public:
	EventDispatcherBase()
		:
			eventCallbackListMap(),
			listenerMutex(),
			filterList()
	{
	}

	EventDispatcherBase(EventDispatcherBase &&) = delete;
	EventDispatcherBase(const EventDispatcherBase &) = delete;
	EventDispatcherBase & operator = (const EventDispatcherBase &) = delete;

	Handle appendListener(const Event & event, const Callback & callback)
	{
		std::lock_guard<Mutex> lockGuard(listenerMutex);

		return eventCallbackListMap[event].append(callback);
	}

	Handle prependListener(const Event & event, const Callback & callback)
	{
		std::lock_guard<Mutex> lockGuard(listenerMutex);

		return eventCallbackListMap[event].prepend(callback);
	}

	Handle insertListener(const Event & event, const Callback & callback, const Handle before)
	{
		std::lock_guard<Mutex> lockGuard(listenerMutex);

		return eventCallbackListMap[event].insert(callback, before);
	}

	bool removeListener(const Event & event, const Handle handle)
	{
		CallbackList_ * callableList = doFindCallableList(event);
		if(callableList) {
			return callableList->remove(handle);
		}

		return false;
	}

	FilterHandle appendFilter(const Filter & filter)
	{
		return filterList.append(filter);
	}

	bool removeFilter(const FilterHandle & filterHandle)
	{
		return filterList.remove(filterHandle);
	}

	template <typename Func>
	void forEach(const Event & event, Func && func) const
	{
		const CallbackList_ * callableList = doFindCallableList(event);
		if(callableList) {
			callableList->forEach(std::forward<Func>(func));
		}
	}

	template <typename Func>
	bool forEachIf(const Event & event, Func && func) const
	{
		const CallbackList_ * callableList = doFindCallableList(event);
		if (callableList) {
			return callableList->forEachIf(std::forward<Func>(func));
		}

		return true;
	}

	void dispatch(Args ...args) const
	{
		static_assert(canIncludeEventType, "Dispatching arguments count doesn't match required (Event type should be included).");

		// can't std::forward<Args>(args) in EventGetter::getEvent because the pass by value arguments will be moved to getEvent
		// then the other std::forward<Args>(args) to doDispatch will get empty values.
		doDispatch(
			EventGetter::getEvent(args...),
			std::forward<Args>(args)...
		);
	}

	template <typename T>
	void dispatch(T && first, Args ...args) const
	{
		static_assert(canExcludeEventType, "Dispatching arguments count doesn't match required (Event type should NOT be included).");

		doDispatch(
			EventGetter::getEvent(std::forward<T>(first), args...),
			std::forward<Args>(args)...
		);
	}

protected:
	void doDispatch(const Event & e, Args ...args) const
	{
		if(! filterList.empty()) {
			if(
				! filterList.forEachIf([&args...](typename FilterList::Callback & callback) {
				return callback(typename std::add_lvalue_reference<Args>::type(args)...);
			})
				) {
				return;
			}
		}

		const CallbackList_ * callableList = doFindCallableList(e);
		if(callableList) {
			(*callableList)(std::forward<Args>(args)...);
		}
	}

private:
	// template helper to avoid code duplication in doFindCallableList
	template <typename T>
	static auto doFindCallableListHelper(T * self, const Event & e)
		-> typename std::conditional<std::is_const<T>::value, const CallbackList_ *, CallbackList_ *>::type
	{
		std::lock_guard<Mutex> lockGuard(self->listenerMutex);

		auto it = self->eventCallbackListMap.find(e);
		if(it != self->eventCallbackListMap.end()) {
			return &it->second;
		}
		else {
			return nullptr;
		}
	}

	const CallbackList_ * doFindCallableList(const Event & e) const
	{
		return doFindCallableListHelper(this, e);
	}

	CallbackList_ * doFindCallableList(const Event & e)
	{
		return doFindCallableListHelper(this, e);
	}

private:
	std::map<Event, CallbackList_> eventCallbackListMap;
	mutable Mutex listenerMutex;

	FilterList filterList;
};


} //namespace internal_

template <
	typename EventGetter,
	typename Prototype,
	typename Callback = void,
	typename ArgumentPassingMode = ArgumentPassingAutoDetect,
	typename Threading = MultipleThreading
>
class EventDispatcher : public internal_::EventDispatcherBase<
	EventGetter, Callback, ArgumentPassingMode, Threading, Prototype>
{
};


} //namespace eventpp


#endif

