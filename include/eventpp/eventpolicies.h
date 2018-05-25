#ifndef EVENTOPTIONS_H_730367862613
#define EVENTOPTIONS_H_730367862613

#include <mutex>
#include <atomic>
#include <condition_variable>

namespace eventpp {

namespace internal_ {

struct DummyMutex
{
	void lock() {}
	void unlock() {}
};

} //namespace internal_

struct MultipleThreading
{
	using Mutex = std::mutex;

	template <typename T>
	using Atomic = std::atomic<T>;

	using ConditionVariable = std::condition_variable;
};

struct SingleThreading
{
	using Mutex = internal_::DummyMutex;

	// May replace Atomic with dummy atomic later.
	template <typename T>
	using Atomic = std::atomic<T>;

	// May replace ConditionVariable with dummy condition variable later.
	using ConditionVariable = std::condition_variable;
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

template <typename E>
struct DefaultEventPolicies
{
	template <typename U, typename ...Args>
	static E getEvent(U && e, const Args &...) {
		return e;
	}

	using Callback = void;
	using Threading = MultipleThreading;
	using ArgumentPassingMode = ArgumentPassingAutoDetect;
};

struct DefaultCallbackListPolicies
{
	using Callback = void;
	using Threading = MultipleThreading;
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
template <typename T, bool> struct SelectCallback;
template <typename T> struct SelectCallback<T, true> { using Type = typename T::Callback; };
template <typename T> struct SelectCallback<T, false> { using Type = void; };

template <typename T>
class HasFunctionGetEvent
{
	template <typename C> static std::true_type test(decltype(&C::getEvent) *) ;
	template <typename C> static std::false_type test(...);    

public:
	enum { value = !! decltype(test<T>(0))() };
};
template <typename E>
struct BasicGetEvent
{
	template <typename U, typename ...Args>
	static E getEvent(U && e, const Args &...) {
		return e;
	}
};
template <typename T, typename Key, bool> struct SelectGetEvent;
template <typename T, typename Key> struct SelectGetEvent<T, Key, true> { using Type = T; };
template <typename T, typename Key> struct SelectGetEvent<T, Key, false> { using Type = BasicGetEvent<Key>; };

} //namespace internal_


} //namespace eventpp


#endif
