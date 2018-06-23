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
#include <array>
#include <cassert>

namespace eventpp {

namespace internal_ {

template <
	typename EventType,
	typename Prototype,
	typename Policies
>
class EventQueueBase;

template <
	typename EventType,
	typename PoliciesType,
	typename ReturnType, typename ...Args
>
class EventQueueBase <
		EventType,
		ReturnType (Args...),
		PoliciesType
	> : public EventDispatcherBase<
		EventType,
		ReturnType (Args...),
		PoliciesType,
		EventQueueBase <
			EventType,
			ReturnType (Args...),
			PoliciesType
		>
	>
{
private:
	using super = EventDispatcherBase<
		EventType,
		ReturnType (Args...),
		PoliciesType,
		EventQueueBase <
			EventType,
			ReturnType (Args...),
			PoliciesType
		>
	>;

	using Policies = typename super::Policies;
	using Event = typename super::Event;
	using Mutex = typename super::Mutex;
	using Threading = typename super::Threading;
	using ConditionVariable = typename Threading::ConditionVariable;
	using GetEvent = typename super::GetEvent;

	using QueuedEvent_ = std::tuple<
		typename std::remove_cv<typename std::remove_reference<Event>::type>::type,
		typename std::remove_cv<typename std::remove_reference<Args>::type>::type...
	>;

	class QueuedItem
	{
	public:
		QueuedItem() : buffer(), allocated(false)
		{
		}

		~QueuedItem()
		{
			if(allocated) {
				clear();
			}
		}

		QueuedItem(QueuedItem &&) = delete;
		QueuedItem(const QueuedItem &) = delete;
		QueuedItem & operator = (const QueuedItem &) = delete;

		void set(QueuedEvent_ && item) {
			assert(! allocated);

			new (buffer.data()) QueuedEvent_(std::move(item));
			allocated = true;
		}

		QueuedEvent_ & get() {
			assert(allocated);

			return *reinterpret_cast<QueuedEvent_ *>(buffer.data());
		}

		void clear() {
			assert(allocated);

			get().~QueuedEvent_();
			allocated = false;
		}

	private:
		std::array<char, sizeof(QueuedEvent_)> buffer;
		bool allocated;
	};

public:
	using QueuedEvent = QueuedEvent_;

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

public:
	EventQueueBase()
		:
			super(),
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

	template <typename ...A>
	auto enqueue(A ...args) -> typename std::enable_if<sizeof...(A) == sizeof...(Args), void>::type
	{
		static_assert(super::ArgumentPassingMode::canIncludeEventType, "Enqueuing arguments count doesn't match required (Event type should be included).");

		doEnqueue(QueuedEvent(
			GetEvent::getEvent(args...),
			std::forward<A>(args)...
		));

		if(doCanProcess()) {
			queueListConditionVariable.notify_one();
		}
	}

	template <typename T, typename ...A>
	auto enqueue(T && first, A ...args) -> typename std::enable_if<sizeof...(A) == sizeof...(Args), void>::type
	{
		static_assert(super::ArgumentPassingMode::canExcludeEventType, "Enqueuing arguments count doesn't match required (Event type should NOT be included).");

		doEnqueue(QueuedEvent(
			GetEvent::getEvent(std::forward<T>(first), args...),
			std::forward<A>(args)...
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
			std::list<QueuedItem> tempList;

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
					doDispatchQueuedEvent(item.get(), typename internal_::MakeIndexSequence<sizeof...(Args) + 1>::Type());
					item.clear();
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

	using super::dispatch;

	void dispatch(const QueuedEvent & queuedEvent)
	{
		doDispatchQueuedEvent(queuedEvent, typename internal_::MakeIndexSequence<sizeof...(Args) + 1>::Type());
	}

	void dispatch(QueuedEvent && queuedEvent)
	{
		doDispatchQueuedEvent(std::move(queuedEvent), typename internal_::MakeIndexSequence<sizeof...(Args) + 1>::Type());
	}

	bool peekEvent(QueuedEvent * queuedEvent)
	{
		if(! queueList.empty()) {
			std::lock_guard<Mutex> queueListLock(queueListMutex);
			
			if(! queueList.empty()) {
				*queuedEvent = queueList.front().get();
				return true;
			}
		}

		return false;
	}

	bool takeEvent(QueuedEvent * queuedEvent)
	{
		if(! queueList.empty()) {
			std::list<QueuedItem> tempList;

			{
				std::lock_guard<Mutex> queueListLock(queueListMutex);

				if(! queueList.empty()) {
					tempList.splice(tempList.end(), queueList, queueList.begin());
				}
			}

			if(! tempList.empty()) {
				*queuedEvent = std::move(tempList.front().get());
				tempList.front().clear();

				std::lock_guard<Mutex> queueListLock(freeListMutex);
				freeList.splice(freeList.end(), tempList);

				return true;
			}
		}

		return false;
	}

private:
	bool doCanProcess() const {
		return ! empty() && doCanNotifyQueueAvailable();
	}

	bool doCanNotifyQueueAvailable() const {
		return queueNotifyCounter.load(std::memory_order_acquire) == 0;
	}

	template <typename T, size_t ...Indexes>
	void doDispatchQueuedEvent(T && item, internal_::IndexSequence<Indexes...>)
	{
		this->doDispatch(std::get<Indexes>(std::forward<T>(item))...);
	}

	void doEnqueue(QueuedEvent && item)
	{
		std::list<QueuedItem> tempList;
		if(! freeList.empty()) {
			{
				std::lock_guard<Mutex> queueListLock(freeListMutex);
				if(! freeList.empty()) {
					tempList.splice(tempList.end(), freeList, freeList.begin());
				}
			}
		}

		if(tempList.empty()) {
			tempList.emplace_back();
		}

		auto it = tempList.begin();
		it->set(std::move(item));

		std::lock_guard<Mutex> queueListLock(queueListMutex);
		queueList.splice(queueList.end(), tempList, it);
	}

private:
	mutable ConditionVariable queueListConditionVariable;
	typename Threading::template Atomic<int> queueEmptyCounter;
	typename Threading::template Atomic<int> queueNotifyCounter;
	mutable Mutex queueListMutex;
	std::list<QueuedItem> queueList;
	Mutex freeListMutex;
	std::list<QueuedItem> freeList;
};

} //namespace internal_

template <
	typename Event,
	typename Prototype,
	typename Policies = DefaultPolicies
>
class EventQueue : public internal_::InheritMixins<
	internal_::EventQueueBase<Event, Prototype, Policies>,
	typename internal_::SelectMixins<Policies, internal_::HasTypeMixins<Policies>::value >::Type
>::Type
{
};


} //namespace eventpp


#endif

