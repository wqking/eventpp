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

#ifndef HETEREVENTDISPATCHER_H_127766658555
#define HETEREVENTDISPATCHER_H_127766658555

#include "eventdispatcher.h"

#include <array>
#include <string>
#include <functional>
#include <type_traits>
#include <map>
#include <unordered_map>
#include <mutex>
#include <algorithm>
#include <memory>
#include <tuple>

namespace eventpp {

template <typename ...Types>
struct PrototypeList
{
	enum { size = sizeof...(Types) };
};

template <typename ...Args>
struct CanConvert
{
	enum { value = false };
};

template <typename From, typename ...FromArgs, typename To, typename ...ToArgs>
struct CanConvert <std::tuple<From, FromArgs...>, std::tuple<To, ToArgs...> >
{
	enum { value = std::is_convertible<From, To>::value && CanConvert<std::tuple<FromArgs...>, std::tuple<ToArgs...> >::value };
};

template <>
struct CanConvert <std::tuple<>, std::tuple<> >
{
	enum { value = true };
};

template <int N, typename PrototypeList_, typename ...InArgs>
struct FindCallablePrototypeHelper
{
	using Prototype = void;
	enum {index = -1 };
};

template <int N, typename RT, typename ...Args, typename ...Others, typename ...InArgs>
struct FindCallablePrototypeHelper <N, PrototypeList<RT (Args...), Others...>, InArgs...>
{
	using Prototype = typename std::conditional<
		CanConvert<std::tuple<InArgs...>, std::tuple<Args...> >::value,
		RT (Args...),
		typename FindCallablePrototypeHelper<N + 1, PrototypeList<Others...>, InArgs...>::Prototype
	>::type;
	enum {
		index = CanConvert<std::tuple<InArgs...>, std::tuple<Args...> >::value
			? N
			: FindCallablePrototypeHelper<N + 1, PrototypeList<Others...>, InArgs...>::index
	};
};

template <typename PrototypeList_, typename ...InArgs>
struct FindCallablePrototype : public FindCallablePrototypeHelper <0, PrototypeList_, InArgs...>
{
};



namespace internal_ {

template <
	typename EventType,
	typename PrototypeList_,
	typename PoliciesType,
	typename MixinRoot_
>
class HeterEventDispatcherBase
{
private:
	template <typename T>
	class CallableInvoker
	{
	};

	template <typename RT, typename ...Args>
	class CallableInvoker <RT (Args...)> : public EventDispatcher<EventType, RT (Args...), PoliciesType>
	{
	public:
	private:
	};

protected:
	using ThisType = HeterEventDispatcherBase<
		EventType,
		PrototypeList_,
		PoliciesType,
		MixinRoot_
	>;

	using Policies = PoliciesType;
	using Threading = typename SelectThreading<Policies, HasTypeThreading<Policies>::value>::Type;

	enum { prototypeCount = PrototypeList_::size };

public:
	using Event = EventType;
	using Mutex = typename Threading::Mutex;

public:
	HeterEventDispatcherBase()
	{
	}

	HeterEventDispatcherBase(const HeterEventDispatcherBase & other)
	{
	}

	HeterEventDispatcherBase(HeterEventDispatcherBase && other) noexcept
	{
	}

	HeterEventDispatcherBase & operator = (const HeterEventDispatcherBase & other)
	{
	}

	HeterEventDispatcherBase & operator = (HeterEventDispatcherBase && other) noexcept
	{
	}

	void swap(HeterEventDispatcherBase & other) noexcept {
		using std::swap;
		
		//swap(eventCallbackListMap, other.eventCallbackListMap);
	}
	
	friend void swap(HeterEventDispatcherBase & first, HeterEventDispatcherBase & second) noexcept {
		first.swap(second);
	}

	template <template <typename> class F, typename RT, typename ...Args>
	void appendListener(const Event & event, const F<RT(Args...)> & callback)
	{
		auto invoker = doFindCallableInvoker<Args...>();
		invoker->appendListener(event, callback);
	}

	template <typename ...Args>
	void dispatch(Args ...args) const
	{
		auto invoker = doFindCallableInvoker<Args...>();
		invoker->dispatch(std::forward<Args>(args)...);
	}

private:
	template <typename ...Args>
	auto doFindCallableInvoker() const
		-> std::shared_ptr<CallableInvoker<typename FindCallablePrototype<PrototypeList_, Args...>::Prototype> >
	{
		using FindResult = FindCallablePrototype<PrototypeList_, Args...>;
		static_assert(FindResult::index >= 0, "Can't find invoker for the given argument types.");

		if(! callableInvokerList[FindResult::index]) {
			if(! callableInvokerList[FindResult::index]) {
				std::lock_guard<Mutex> lockGuard(callableInvokerListMutex);
				callableInvokerList[FindResult::index] = std::make_shared<CallableInvoker<typename FindResult::Prototype> >();
			}
		}

		return std::static_pointer_cast<CallableInvoker<typename FindResult::Prototype> >(callableInvokerList[FindResult::index]);
	}

private:
	mutable std::array<std::shared_ptr<void>, prototypeCount> callableInvokerList;
	mutable Mutex callableInvokerListMutex;
};


} //namespace internal_

template <
	typename Event_,
	typename PrototypeList_,
	typename Policies_ = DefaultPolicies
>
class HeterEventDispatcher : public internal_::InheritMixins<
		internal_::HeterEventDispatcherBase<Event_, PrototypeList_, Policies_, void>,
		typename internal_::SelectMixins<Policies_, internal_::HasTypeMixins<Policies_>::value >::Type
	>::Type, public TagEventDispatcher
{
private:
	using super = typename internal_::InheritMixins<
		internal_::HeterEventDispatcherBase<Event_, PrototypeList_, Policies_, void>,
		typename internal_::SelectMixins<Policies_, internal_::HasTypeMixins<Policies_>::value >::Type
	>::Type;

public:
	using super::super;
};


} //namespace eventpp


#endif

