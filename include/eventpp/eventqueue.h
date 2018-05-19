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

#ifndef EVENTQUEUE_H_705786053037
#define EVENTQUEUE_H_705786053037

#include "eventdispatcher.h"

#include <list>
#include <tuple>
#include <chrono>
#include <mutex>

namespace eventpp {

namespace internal_ {

template <
	typename EventGetterType,
	typename CallbackType,
	typename ArgumentPassingMode,
	typename Threading,
	typename ReturnType, typename ...Args
>
class EventQueueBase;

template <
	typename EventGetterType,
	typename CallbackType,
	typename ArgumentPassingMode,
	typename Threading,
	typename ReturnType, typename ...Args
>
class EventQueueBase <
		EventGetterType,
		CallbackType,
		ArgumentPassingMode,
		Threading,
		ReturnType (Args...)
	> : public EventDispatcherBase<
		EventGetterType,
		CallbackType,
		ArgumentPassingMode,
		Threading,
		ReturnType (Args...)
	>
{
private:
	using super = EventDispatcherBase<
		EventGetterType,
		CallbackType,
		ArgumentPassingMode,
		Threading,
		ReturnType (Args...)
	>;

	using EventGetter = typename super::EventGetter;
	using Event = typename super::Event;
	using Mutex = typename super::Mutex;
	using ConditionVariable = typename Threading::ConditionVariable;

	using QueueItem = std::tuple<
		typename std::remove_cv<typename std::remove_reference<Event>::type>::type,
		typename std::remove_cv<typename std::remove_reference<Args>::type>::type...
	>;

public:
	struct DisableQueueNotify
	{
		DisableQueueNotify(EventQueueBase * queue)
			: queue(queue)
		{
			++queue->queueNotifyCounter;
		}

		~DisableQueueNotify()
		{
			--queue->queueNotifyCounter;

			if(queue->doCanNotifyQueueAvailable() && ! queue->empty()) {
				queue->queueListConditionVariable.notify_one();
			}
		}

		EventQueueBase * queue;
	};

	friend struct QueueNotifyDiable;

public:
	EventQueueBase()
		:
			queueListConditionVariable(),
			queueEmptyCounter(0),
			queueNotifyCounter(0),
			queueListMutex(),
			queueList(),
			freeListMutex(),
			freeList()
	{
	}

	EventQueueBase(EventQueueBase &&) = delete;
	EventQueueBase(const EventQueueBase &) = delete;
	EventQueueBase & operator = (const EventQueueBase &) = delete;

	void enqueue(Args ...args)
	{
		static_assert(super::canIncludeEventType, "Enqueuing arguments count doesn't match required (Event type should be included).");

		doEnqueue(QueueItem(
			EventGetter::getEvent(args...),
			std::forward<Args>(args)...
		));

		if(doCanProcess()) {
			queueListConditionVariable.notify_one();
		}
	}

	template <typename T>
	void enqueue(T && first, Args ...args)
	{
		static_assert(super::canExcludeEventType, "Enqueuing arguments count doesn't match required (Event type should NOT be included).");

		doEnqueue(QueueItem(
			EventGetter::getEvent(std::forward<T>(first), args...),
			std::forward<Args>(args)...
		));

		if(doCanProcess()) {
			queueListConditionVariable.notify_one();
		}
	}

	bool empty() const {
		return queueList.empty() && (queueEmptyCounter.load(std::memory_order_acquire) == 0);
	}

	void process()
	{
		if(! queueList.empty()) {
			std::list<QueueItem> tempList;

			// Use a counter to tell the queue list is not empty during processing
			// even though queueList is swapped to empty.
			CounterGuard<decltype(queueEmptyCounter)> counterGuard(queueEmptyCounter);

			{
				std::lock_guard<Mutex> queueListLock(queueListMutex);
				using namespace std;
				swap(queueList, tempList);
			}

			if(! tempList.empty()) {
				for(auto & item : tempList) {
					doProcessItem(item, typename internal_::MakeIndexSequence<sizeof...(Args) + 1>::Type());
					item = QueueItem();
				}

				std::lock_guard<Mutex> queueListLock(freeListMutex);
				freeList.splice(freeList.end(), tempList);
			}
		}
	}

	void wait() const
	{
		std::unique_lock<Mutex> queueListLock(queueListMutex);
		queueListConditionVariable.wait(queueListLock, [this]() -> bool {
			return doCanProcess();
		});
	}

	template <class Rep, class Period>
	bool waitFor(const std::chrono::duration<Rep, Period> & duration) const
	{
		std::unique_lock<Mutex> queueListLock(queueListMutex);
		return queueListConditionVariable.wait_for(queueListLock, duration, [this]() -> bool {
			return doCanProcess();
		});
	}

private:
	bool doCanProcess() const {
		return ! empty() && doCanNotifyQueueAvailable();
	}

	bool doCanNotifyQueueAvailable() const {
		return queueNotifyCounter.load(std::memory_order_acquire) == 0;
	}

	template <size_t ...Indexes>
	void doProcessItem(QueueItem & item, internal_::IndexSequence<Indexes...>)
	{
		this->doDispatch(std::get<Indexes>(item)...);
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

private:
	mutable ConditionVariable queueListConditionVariable;
	typename Threading::template Atomic<int> queueEmptyCounter;
	typename Threading::template Atomic<int> queueNotifyCounter;
	mutable Mutex queueListMutex;
	std::list<QueueItem> queueList;
	Mutex freeListMutex;
	std::list<QueueItem> freeList;
};

} //namespace internal_

template <
	typename EventGetter,
	typename Prototype,
	typename Callback = void,
	typename ArgumentPassingMode = ArgumentPassingAutoDetect,
	typename Threading = MultipleThreading
>
class EventQueue : public internal_::EventQueueBase<
	EventGetter, Callback, ArgumentPassingMode, Threading, Prototype>
{
};


} //namespace eventpp


#endif

