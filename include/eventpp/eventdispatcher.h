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
#include <list>
#include <tuple>
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
	static Event getEvent(U && e, Args...) {
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

namespace _internal {

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
private:
	using EventGetter = typename std::conditional<
		std::is_base_of<EventGetterBase, EventGetterType>::value,
		EventGetterType,
		PrimaryEventGetter<EventGetterType>
	>::type;
	using Mutex = typename Threading::Mutex;
	using _Callback = typename std::conditional<
		std::is_same<CallbackType, void>::value,
		std::function<ReturnType (Args...)>,
		CallbackType
	>::type;
	using _CallbackList = CallbackList<ReturnType (Args...), _Callback, Threading>;

	using Filter = std::function<bool (typename std::add_lvalue_reference<Args>::type...)>;
	using FilterList = CallbackList<bool (Args...), Filter, Threading>;

	enum {
		canIncludeEventType = ArgumentPassingMode::canIncludeEventType,
		canExcludeEventType = ArgumentPassingMode::canExcludeEventType
	};

	using _Handle = typename _CallbackList::Handle;
	using _Event = typename EventGetter::Event;

	using QueueItem = std::tuple<
		typename std::remove_cv<typename std::remove_reference<_Event>::type>::type,
		typename std::remove_cv<typename std::remove_reference<Args>::type>::type...
	>;

public:
	using Handle = _Handle;
	using Callback = _Callback;
	using Event = _Event;
	using FilterHandle = typename FilterList::Handle;;

public:
	EventDispatcherBase() = default;
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
		_CallbackList * callableList = doFindCallableList(event);
		if(callableList) {
			return callableList->remove(handle);
		}

		return false;
	}

	template <typename Func>
	void forEach(const Event & event, Func && func) const
	{
		const _CallbackList * callableList = doFindCallableList(event);
		if(callableList) {
			callableList->forEach(std::forward<Func>(func));
		}
	}

	template <typename Func>
	bool forEachIf(const Event & event, Func && func) const
	{
		const _CallbackList * callableList = doFindCallableList(event);
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

		// can't std::forward<Args>(args) in EventGetter::getEvent because the pass by value arguments will be moved to getEvent
		// then the other std::forward<Args>(args) to doDispatch will get empty values.
		doDispatch(
			EventGetter::getEvent(std::forward<T>(first), args...),
			std::forward<Args>(args)...
		);
	}

	void enqueue(Args ...args)
	{
		static_assert(canIncludeEventType, "Enqueuing arguments count doesn't match required (Event type should be included).");

		doEnqueue(QueueItem(std::get<0>(std::tie(args...)), std::forward<Args>(args)...));
	}

	template <typename T>
	void enqueue(T && first, Args ...args)
	{
		static_assert(canExcludeEventType, "Enqueuing arguments count doesn't match required (Event type should NOT be included).");

		doEnqueue(QueueItem(std::forward<T>(first), std::forward<Args>(args)...));
	}

	void process()
	{
		if(! queueList.empty()) {
			std::list<QueueItem> tempList;

			{
				std::lock_guard<Mutex> queueListLock(queueListMutex);
				using namespace std;
				swap(queueList, tempList);
			}

			if(! tempList.empty()) {
				for(auto & item : tempList) {
					doProcessItem(item, typename _internal::MakeIndexSequence<sizeof...(Args) + 1>::Type());
					item = QueueItem();
				}

				std::lock_guard<Mutex> queueListLock(freeListMutex);
				freeList.splice(freeList.end(), tempList);
			}
		}
	}

	FilterHandle appendFilter(const Filter & filter)
	{
		return filterList.append(filter);
	}

	bool removeFilter(const FilterHandle & filterHandle)
	{
		return filterList.remove(filterHandle);
	}

private:
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

		const _CallbackList * callableList = doFindCallableList(e);
		if(callableList) {
			(*callableList)(std::forward<Args>(args)...);
		}
	}

	template <size_t ...Indexes>
	void doProcessItem(QueueItem & item, _internal::IndexSequence<Indexes...>)
	{
		dispatch(std::get<Indexes>(item)...);
	}

	void doEnqueue(QueueItem && item)
	{
		if(! freeList.empty()) {
			std::list<QueueItem> tempList;

			{
				std::lock_guard<Mutex> queueListLock(freeListMutex);
				if(! freeList.empty()) {
					tempList.splice(tempList.end(), freeList, freeList.begin());
				}
			}

			if(! tempList.empty()) {
				auto it = tempList.begin();
				*it = item;

				std::lock_guard<Mutex> queueListLock(queueListMutex);
				queueList.splice(queueList.end(), tempList, it);

				return;
			}
		}

		std::lock_guard<Mutex> queueListLock(queueListMutex);
		queueList.emplace_back(item);
	}

	// template helper to avoid code duplication in doFindCallableList
	template <typename T>
	static auto doFindCallableListHelper(T * self, const Event & e)
		-> typename std::conditional<std::is_const<T>::value, const _CallbackList *, _CallbackList *>::type
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

	const _CallbackList * doFindCallableList(const Event & e) const
	{
		return doFindCallableListHelper(this, e);
	}

	_CallbackList * doFindCallableList(const Event & e)
	{
		return doFindCallableListHelper(this, e);
	}

private:
	std::map<Event, _CallbackList> eventCallbackListMap;
	mutable Mutex listenerMutex;

	Mutex queueListMutex;
	std::list<QueueItem> queueList;
	Mutex freeListMutex;
	std::list<QueueItem> freeList;

	FilterList filterList;
};


} //namespace _internal

template <
	typename EventGetter,
	typename Prototype,
	typename Callback = void,
	typename ArgumentPassingMode = ArgumentPassingAutoDetect,
	typename Threading = MultipleThreading
>
class EventDispatcher : public _internal::EventDispatcherBase<
	EventGetter, Callback, ArgumentPassingMode, Threading, Prototype>
{
};


} //namespace eventpp


#endif

