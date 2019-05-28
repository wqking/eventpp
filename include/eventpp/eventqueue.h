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
	using Threading = typename super::Threading;
	using ConditionVariable = typename Threading::ConditionVariable;

	using QueuedEvent_ = std::tuple<
		typename std::remove_cv<typename std::remove_reference<typename super::Event>::type>::type,
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
	using super::Event;
	using super::Handle;
	using super::Callback;
	using Mutex = typename super::Mutex;

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

	EventQueueBase(const EventQueueBase & other)
		: super(other)
	{
	}

	EventQueueBase(EventQueueBase && other) noexcept
		: super(std::move(other))
	{
	}

	EventQueueBase & operator = (const EventQueueBase & other)
	{
		super::operator = (other);
		return *this;
	}
	
	EventQueueBase & operator = (EventQueueBase && other) noexcept
	{
		super::operator = (std::move(other));
		return *this;
	}

	template <typename ...A>
	auto enqueue(A ...args) -> typename std::enable_if<sizeof...(A) == sizeof...(Args), void>::type
	{
		static_assert(super::ArgumentPassingMode::canIncludeEventType, "Enqueuing arguments count doesn't match required (Event type should be included).");

		using GetEvent = typename SelectGetEvent<Policies, EventType, HasFunctionGetEvent<Policies, Args...>::value>::Type;

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

		using GetEvent = typename SelectGetEvent<Policies, EventType, HasFunctionGetEvent<Policies, T &&, Args...>::value>::Type;

		doEnqueue(QueuedEvent(
			GetEvent::getEvent(std::forward<T>(first), args...),
			std::forward<A>(args)...
		));

		if(doCanProcess()) {
			queueListConditionVariable.notify_one();
		}
	}

	bool empty() const
	{
		return queueList.empty() && (queueEmptyCounter.load(std::memory_order_acquire) == 0);
	}
	
	void clearEvents()
	{
		if(! queueList.empty()) {
			std::list<QueuedItem> tempList;

			{
				std::lock_guard<Mutex> queueListLock(queueListMutex);
				std::swap(queueList, tempList);
			}

			if(! tempList.empty()) {
				for(auto & item : tempList) {
					item.clear();
				}

				std::lock_guard<Mutex> queueListLock(freeListMutex);
				freeList.splice(freeList.end(), tempList);
			}
		}
	}

	bool process()
	{
		if(! queueList.empty()) {
			std::list<QueuedItem> tempList;

			// Use a counter to tell the queue list is not empty during processing
			// even though queueList is swapped to empty.
			CounterGuard<decltype(queueEmptyCounter)> counterGuard(queueEmptyCounter);

			{
				std::lock_guard<Mutex> queueListLock(queueListMutex);
				std::swap(queueList, tempList);
			}

			if(! tempList.empty()) {
				for(auto & item : tempList) {
					doDispatchQueuedEvent(
						item.get(),
						typename internal_::MakeIndexSequence<sizeof...(Args) + 1>::Type()
					);
					item.clear();
				}

				std::lock_guard<Mutex> queueListLock(freeListMutex);
				freeList.splice(freeList.end(), tempList);
				
				return true;
			}
		}
		
		return false;
	}

	bool processOne()
	{
		if(! queueList.empty()) {
			std::list<QueuedItem> tempList;

			// Use a counter to tell the queue list is not empty during processing
			// even though queueList is swapped to empty.
			CounterGuard<decltype(queueEmptyCounter)> counterGuard(queueEmptyCounter);

			{
				std::lock_guard<Mutex> queueListLock(queueListMutex);
				if(! queueList.empty()) {
					tempList.splice(tempList.end(), queueList, queueList.begin());
				}
			}

			if(! tempList.empty()) {
				auto & item = tempList.front();
				doDispatchQueuedEvent(
					item.get(),
					typename internal_::MakeIndexSequence<sizeof...(Args) + 1>::Type()
				);
				item.clear();

				std::lock_guard<Mutex> queueListLock(freeListMutex);
				freeList.splice(freeList.end(), tempList);
				
				return true;
			}
		}
		
		return false;
	}

	template <typename F>
	bool processIf(F && func)
	{
		if(! queueList.empty()) {
			std::list<QueuedItem> tempList;
			std::list<QueuedItem> idleList;

			// Use a counter to tell the queue list is not empty during processing
			// even though queueList is swapped to empty.
			CounterGuard<decltype(queueEmptyCounter)> counterGuard(queueEmptyCounter);

			{
				std::lock_guard<Mutex> queueListLock(queueListMutex);
				std::swap(queueList, tempList);
			}

			if(! tempList.empty()) {
				for(auto it = tempList.begin(); it != tempList.end(); ) {
					if(doInvokeFuncWithQueuedEvent(
							func,
							it->get(),
							typename internal_::MakeIndexSequence<sizeof...(Args) + 1>::Type())
						) {
						doDispatchQueuedEvent(
							it->get(),
							typename internal_::MakeIndexSequence<sizeof...(Args) + 1>::Type()
						);
						it->clear();
						
						auto tempIt = it;
						++it;
						idleList.splice(idleList.end(), tempList, tempIt);
					}
					else {
						++it;
					}
				}

				if (! tempList.empty()) {
					std::lock_guard<Mutex> queueListLock(queueListMutex);
					queueList.splice(queueList.begin(), tempList);
				}

				if(! idleList.empty()) {
					std::lock_guard<Mutex> queueListLock(freeListMutex);
					freeList.splice(freeList.end(), idleList);
					
					return true;
				}
			}
		}
		
		return false;
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

	template <typename U>
	auto dispatch(const U & queuedEvent)
		-> typename std::enable_if<std::is_same<U, QueuedEvent>::value, void>::type
	{
		doDispatchQueuedEvent(
			queuedEvent,
			typename internal_::MakeIndexSequence<sizeof...(Args) + 1>::Type()
		);
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

protected:
	bool doCanProcess() const {
		return ! empty() && doCanNotifyQueueAvailable();
	}

	bool doCanNotifyQueueAvailable() const {
		return queueNotifyCounter.load(std::memory_order_acquire) == 0;
	}

	template <typename T, size_t ...Indexes>
	void doDispatchQueuedEvent(T && item, IndexSequence<Indexes...>)
	{
		this->doDispatch(std::get<Indexes>(std::forward<T>(item))...);
	}

	template <typename F, typename T, size_t ...Indexes>
	bool doInvokeFuncWithQueuedEvent(F && func, T && item, IndexSequence<Indexes...>) const
	{
		return doInvokeFuncWithQueuedEventHelper(std::forward<F>(func), std::get<Indexes>(std::forward<T>(item))...);
	}
	
	template <typename F>
	bool doInvokeFuncWithQueuedEventHelper(F && func, const typename super::Event & /*e*/, Args ...args) const
	{
		return func(std::forward<Args>(args)...);
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
	typename Event_,
	typename Prototype_,
	typename Policies_ = DefaultPolicies
>
class EventQueue : public internal_::InheritMixins<
		internal_::EventQueueBase<Event_, Prototype_, Policies_>,
		typename internal_::SelectMixins<Policies_, internal_::HasTypeMixins<Policies_>::value >::Type
	>::Type, public TagEventDispatcher, public TagEventQueue
{
private:
	using super = typename internal_::InheritMixins<
		internal_::EventQueueBase<Event_, Prototype_, Policies_>,
		typename internal_::SelectMixins<Policies_, internal_::HasTypeMixins<Policies_>::value >::Type
	>::Type;

public:
	using super::super;
};


} //namespace eventpp


#endif

