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

// source code for heterogeneous dispatcher

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
	struct Handle_
	{
		int index;
		std::weak_ptr<void> homoHandle;

		operator bool () const noexcept {
			return (bool)homoHandle;
		}
	};

	class HomoDispatcherTypeBase
	{
	public:
		virtual bool doRemoveListener(const EventType & event, const Handle_ & handle) = 0;
	};

	template <typename T>
	class HomoDispatcherType : public EventDispatcher<EventType, T, PoliciesType>, public HomoDispatcherTypeBase
	{
	private:
		using super = EventDispatcher<EventType, T, PoliciesType>;

	public:
		virtual bool doRemoveListener(const EventType & event, const Handle_ & handle) override {
			auto sp = handle.homoHandle.lock();
			if(! sp) {
				return false;
			}
			return this->removeListener(event, typename super::Handle(std::static_pointer_cast<typename super::Handle::element_type>(sp)));
		}
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

	// Disable ArgumentPassingMode explicitly
	static_assert(! HasTypeArgumentPassingMode<Policies>::value, "Policies can't have ArgumentPassingMode in heterogeneous dispatcher.");

	enum { prototypeCount = PrototypeList_::size };

public:
	using Handle = Handle_;
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
	Handle appendListener(const Event & event, const F<RT(Args...)> & callback)
	{
		using FindResult = FindCallablePrototype<PrototypeList_, Args...>;
		auto dispatcher = doFindDispatcher<FindResult>();
		return Handle {
			FindResult::index,
			dispatcher->appendListener(event, callback)
		};
	}

	template <template <typename> class F, typename RT, typename ...Args>
	Handle prependListener(const Event & event, const F<RT(Args...)> & callback)
	{
		using FindResult = FindCallablePrototype<PrototypeList_, Args...>;
		auto dispatcher = doFindDispatcher<FindResult>();
		return Handle {
			FindResult::index,
			dispatcher->prependListener(event, callback)
		};
	}

	bool removeListener(const Event & event, const Handle & handle)
	{
		auto dispatcher = dispatcherList[handle.index];
		if(dispatcher) {
			return dispatcher->doRemoveListener(event, handle);
		}

		return false;
	}

	template <typename T, typename ...Args>
	void dispatch(T && first, Args ...args) const
	{
		using FindResult = FindCallablePrototype<PrototypeList_, Args...>;
		auto dispatcher = doFindDispatcher<FindResult>();
		dispatcher->dispatch(std::forward<T>(first), std::forward<Args>(args)...);
	}

private:
	template <typename FindResult>
	auto doFindDispatcher() const
		-> HomoDispatcherType<typename FindResult::Prototype> *
	{
		static_assert(FindResult::index >= 0, "Can't find invoker for the given argument types.");

		if(! dispatcherList[FindResult::index]) {
			if(! dispatcherList[FindResult::index]) {
				std::lock_guard<Mutex> lockGuard(callableInvokerListMutex);
				dispatcherList[FindResult::index] = std::make_shared<HomoDispatcherType<typename FindResult::Prototype> >();
			}
		}

		return static_cast<HomoDispatcherType<typename FindResult::Prototype> *>(dispatcherList[FindResult::index].get());
	}

private:
	mutable std::array<std::shared_ptr<HomoDispatcherTypeBase>, prototypeCount> dispatcherList;
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

