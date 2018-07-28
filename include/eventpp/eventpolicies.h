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

#ifndef EVENTOPTIONS_H_730367862613
#define EVENTOPTIONS_H_730367862613

#include <mutex>
#include <atomic>
#include <condition_variable>
#include <map>
#include <unordered_map>

namespace eventpp {

struct TagCallbackList {};
struct TagEventDispatcher {};
struct TagEventQueue {};

struct SpinLock
{
public:
	void lock() {
		while(locked.test_and_set(std::memory_order_acquire)) {
		}
	}

	void unlock() {
		locked.clear(std::memory_order_release);
	}
	
private:
    std::atomic_flag locked = ATOMIC_FLAG_INIT;
};

template <
	typename Mutex_,
	template <typename > class Atomic_ = std::atomic,
	typename ConditionVariable_ = std::condition_variable
>
struct GeneralThreading
{
	using Mutex = Mutex_;

	template <typename T>
	using Atomic = Atomic_<T>;

	using ConditionVariable = ConditionVariable_;
};

struct MultipleThreading
{
	using Mutex = std::mutex;

	template <typename T>
	using Atomic = std::atomic<T>;

	using ConditionVariable = std::condition_variable;
};

struct SingleThreading
{
	struct Mutex
	{
		void lock() {}
		void unlock() {}
	};
	
	template <typename T>
	struct Atomic
	{
		Atomic() noexcept = default;
		constexpr Atomic(T desired) noexcept
			: value(desired)
		{
		}

		void store(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept
		{
			value = desired;
		}
		
		T load(std::memory_order order = std::memory_order_seq_cst) const noexcept
		{
			return value;
		}
		
		T operator ++ () noexcept
		{
			return ++value;
		}

		T operator -- () noexcept
		{
			return --value;
		}

		T value;
	};

	struct ConditionVariable
	{
		void notify_one() noexcept
		{
		}
		
		template <class Predicate>
		void wait(std::unique_lock<std::mutex>& lock, Predicate pred)
		{
		}
		
		template <class Rep, class Period, class Predicate>
		bool wait_for(std::unique_lock<std::mutex> & lock,
				const std::chrono::duration<Rep, Period> & rel_time,
				Predicate pred
			)
		{
			return true;
		}
	};
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

struct DefaultPolicies
{
	/* default types for CallbackList
	using Callback = std::function<blah, blah>;
	using Threading = MultipleThreading;
	*/

	/* default types/implements for EventDispatch and EventQueue
	template <typename U, typename ...Args>
	static E getEvent(U && e, const Args &...) {
		return e;
	}

	using Callback = std::function<blah, blah>;
	using Threading = MultipleThreading;
	using ArgumentPassingMode = ArgumentPassingAutoDetect;

	template <typename Key, typename T>
	using Map = std::map <Key, T>;
	*/
};

template <template <typename> class ...Args>
struct MixinList
{
};


namespace internal_ {

template <typename T>
struct HasTypeArgumentPassingMode
{
	template <typename C> static std::true_type test(typename C::ArgumentPassingMode *) ;
	template <typename C> static std::false_type test(...);    

	enum { value = !! decltype(test<T>(0))() };
};
template <typename T, bool> struct SelectArgumentPassingMode;
template <typename T> struct SelectArgumentPassingMode <T, true> { using Type = typename T::ArgumentPassingMode; };
template <typename T> struct SelectArgumentPassingMode <T, false> { using Type = ArgumentPassingAutoDetect; };

template <typename T>
struct HasTypeThreading
{
	template <typename C> static std::true_type test(typename C::Threading *) ;
	template <typename C> static std::false_type test(...);    

	enum { value = !! decltype(test<T>(0))() };
};
template <typename T, bool> struct SelectThreading;
template <typename T> struct SelectThreading <T, true> { using Type = typename T::Threading; };
template <typename T> struct SelectThreading <T, false> { using Type = MultipleThreading; };

template <typename T>
struct HasTypeCallback
{
	template <typename C> static std::true_type test(typename C::Callback *) ;
	template <typename C> static std::false_type test(...);    

	enum { value = !! decltype(test<T>(0))() };
};
template <typename T, bool, typename D> struct SelectCallback;
template <typename T, typename D> struct SelectCallback<T, true, D> { using Type = typename T::Callback; };
template <typename T, typename D> struct SelectCallback<T, false, D> { using Type = D; };

template <typename T>
struct HasFunctionGetEvent
{
	template <typename C> static std::true_type test(decltype(&C::getEvent) *) ;
	template <typename C> static std::false_type test(...);    
	
	enum { value = !! decltype(test<T>(0))() };
};
template <typename E>
struct DefaultGetEvent
{
	template <typename U, typename ...Args>
	static E getEvent(U && e, Args && ...) {
		return e;
	}
};
template <typename T, typename Key, bool> struct SelectGetEvent;
template <typename T, typename Key> struct SelectGetEvent<T, Key, true> { using Type = T; };
template <typename T, typename Key> struct SelectGetEvent<T, Key, false> { using Type = DefaultGetEvent<Key>; };

template <typename T>
struct HasFunctionCanContinueInvoking
{
	template <typename C> static std::true_type test(decltype(&C::canContinueInvoking) *) ;
	template <typename C> static std::false_type test(...);    

	enum { value = !! decltype(test<T>(0))() };
};
struct DefaultCanContinueInvoking
{
	template <typename ...Args>
	static bool canContinueInvoking(Args && ...) {
		return true;
	}
};
template <typename T, bool> struct SelectCanContinueInvoking;
template <typename T> struct SelectCanContinueInvoking<T, true> { using Type = T; };
template <typename T> struct SelectCanContinueInvoking<T, false> { using Type = DefaultCanContinueInvoking; };

template <typename T>
struct HasTemplateMap
{
	template <typename C> static std::true_type test(typename C::template Map<int, int> *) ;
	template <typename C> static std::false_type test(...);    

	enum { value = !! decltype(test<T>(0))() };
};
template <typename T>
class HasHash
{
	template <typename C> static std::true_type test(decltype(std::hash<C>()(std::declval<C>())) *) ;
	template <typename C> static std::false_type test(...);    

public:
	enum { value = !! decltype(test<T>(0))() };
};
template <typename Key, typename Value, typename T, bool>
struct SelectMap;
template <typename Key, typename Value, typename T>
struct SelectMap<Key, Value, T, true>
{
	using Type = typename T::template Map<Key, Value>;
};
template <typename Key, typename Value, typename T>
struct SelectMap<Key, Value, T, false> {
	using Type = typename std::conditional<
		HasHash<Key>::value,
		std::unordered_map<Key, Value>,
		std::map<Key, Value>
	>::type;
};


template <typename T>
struct HasTypeMixins
{
	template <typename C> static std::true_type test(typename C::Mixins *) ;
	template <typename C> static std::false_type test(...);    

	enum { value = !! decltype(test<T>(0))() };
};
template <typename T, bool> struct SelectMixins;
template <typename T> struct SelectMixins <T, true> { using Type = typename T::Mixins; };
template <typename T> struct SelectMixins <T, false> { using Type = MixinList<>; };


template <typename Root, typename TList>
struct InheritMixins;

template <typename Root, template <typename> class T, template <typename> class ...Args>
struct InheritMixins <Root, MixinList<T, Args...> >
{
	using Type = T <typename InheritMixins<Root, MixinList<Args...> >::Type>;
};

template <typename Root>
struct InheritMixins <Root, MixinList<> >
{
	using Type = Root;
};

template <typename Root, typename TList, typename Func>
struct ForEachMixins;

template <typename Func, typename Root, template <typename> class T, template <typename> class ...Args>
struct ForEachMixins <Root, MixinList<T, Args...>, Func>
{
	using Type = typename InheritMixins<Root, MixinList<T, Args...> >::Type;

	template <typename ...A>
	static bool forEach(A && ...args) {
		if(Func::template forEach<Type>(std::forward<A>(args)...)) {
			return ForEachMixins<Root, MixinList<Args...>, Func>::forEach(std::forward<A>(args)...);
		}
		return false;
	}
};

template <typename Root, typename Func>
struct ForEachMixins <Root, MixinList<>, Func>
{
	using Type = Root;

	template <typename ...A>
	static bool forEach(A && ...args) {
		return true;
	}
};

template <typename T, typename ...Args>
struct HasFunctionMixinBeforeDispatch
{
	template <typename C> static std::true_type test(
		decltype(std::declval<C>().mixinBeforeDispatch(std::declval<Args>()...)) *
	);
	template <typename C> static std::false_type test(...);    

	enum { value = !! decltype(test<T>(0))() };
};

} //namespace internal_


} //namespace eventpp


#endif
