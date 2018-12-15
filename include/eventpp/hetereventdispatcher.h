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
#include "internal/hetereventdispatcher_i.h"

#include <array>
#include <string>
#include <functional>
#include <type_traits>
#include <mutex>
#include <memory>
#include <tuple>

// source code for heterogeneous dispatcher

namespace eventpp {

namespace internal_ {

template <
	typename EventType_,
	typename PrototypeList_,
	typename Policies_,
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
		virtual bool doRemoveListener(const EventType_ & event, const Handle_ & handle) = 0;
		virtual std::shared_ptr<HomoDispatcherTypeBase> doClone() = 0;
	};

	template <typename T>
	class HomoDispatcherType : public EventDispatcher<EventType_, T, Policies_>, public HomoDispatcherTypeBase
	{
	private:
		using super = EventDispatcher<EventType_, T, Policies_>;

	public:
		virtual bool doRemoveListener(const EventType_ & event, const Handle_ & handle) override {
			auto sp = handle.homoHandle.lock();
			if(! sp) {
				return false;
			}
			return this->removeListener(event, typename super::Handle(std::static_pointer_cast<typename super::Handle::element_type>(sp)));
		}

		virtual std::shared_ptr<HomoDispatcherTypeBase> doClone() override {
			return std::make_shared<HomoDispatcherType<T> >(*this);
		}
	};

protected:
	using ThisType = HeterEventDispatcherBase<
		EventType_,
		PrototypeList_,
		Policies_,
		MixinRoot_
	>;

	using Policies = Policies_;
	using Threading = typename SelectThreading<Policies, HasTypeThreading<Policies>::value>::Type;

	// Disable ArgumentPassingMode explicitly
	static_assert(! HasTypeArgumentPassingMode<Policies>::value, "Policies can't have ArgumentPassingMode in heterogeneous dispatcher.");

	enum { prototypeCount = std::tuple_size<PrototypeList_>::value };

public:
	using Handle = Handle_;
	using Event = EventType_;
	using Mutex = typename Threading::Mutex;

public:
	HeterEventDispatcherBase()
		:
			dispatcherList(),
			dispatcherListMutex()
	{
	}

	HeterEventDispatcherBase(const HeterEventDispatcherBase & other)
	{
		for(size_t i = 0; i < dispatcherList.size(); ++i) {
			dispatcherList[i] = other.dispatcherList[i]->doClone();
		}
	}

	HeterEventDispatcherBase(HeterEventDispatcherBase && other) noexcept
		:
			dispatcherList(std::move(other.dispatcherList)),
			dispatcherListMutex()
	{
	}

	HeterEventDispatcherBase & operator = (HeterEventDispatcherBase other)
	{
		swap(*this, other);

		return *this;
	}

	HeterEventDispatcherBase & operator = (HeterEventDispatcherBase && other) noexcept
	{
		dispatcherList = std::move(other.dispatcherList);

		return *this;
	}

	void swap(HeterEventDispatcherBase & other) noexcept {
		using std::swap;
		
		swap(dispatcherList, other.dispatcherList);
	}
	
	friend void swap(HeterEventDispatcherBase & first, HeterEventDispatcherBase & second) noexcept {
		first.swap(second);
	}

	template <typename C>
	Handle appendListener(const Event & event, const C & callback)
	{
		using PrototypeInfo = FindPrototypeByCallable<PrototypeList_, C>;
		static_assert(PrototypeInfo::index >= 0, "Can't find invoker for the given argument types.");

		auto dispatcher = doFindDispatcher<PrototypeInfo>();
		return Handle {
			PrototypeInfo::index,
			dispatcher->appendListener(event, callback)
		};
	}

	template <typename C>
	Handle prependListener(const Event & event, const C & callback)
	{
		using PrototypeInfo = FindPrototypeByCallable<PrototypeList_, C>;
		static_assert(PrototypeInfo::index >= 0, "Can't find invoker for the given argument types.");

		auto dispatcher = doFindDispatcher<PrototypeInfo>();
		return Handle {
			PrototypeInfo::index,
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
		using PrototypeInfo = FindPrototypeByArgs<PrototypeList_, Args...>;
		static_assert(PrototypeInfo::index >= 0, "Can't find invoker for the given argument types.");

		auto dispatcher = doFindDispatcher<PrototypeInfo>();
		dispatcher->dispatch(std::forward<T>(first), std::forward<Args>(args)...);
	}

protected:
	template <typename PrototypeInfo>
	auto doFindDispatcher() const
		-> HomoDispatcherType<typename PrototypeInfo::Prototype> *
	{
		static_assert(PrototypeInfo::index >= 0, "Can't find invoker for the given argument types.");

		if(! dispatcherList[PrototypeInfo::index]) {
			if(! dispatcherList[PrototypeInfo::index]) {
				std::lock_guard<Mutex> lockGuard(dispatcherListMutex);
				dispatcherList[PrototypeInfo::index] = std::make_shared<HomoDispatcherType<typename PrototypeInfo::Prototype> >();
			}
		}

		return static_cast<HomoDispatcherType<typename PrototypeInfo::Prototype> *>(dispatcherList[PrototypeInfo::index].get());
	}

private:
	mutable std::array<std::shared_ptr<HomoDispatcherTypeBase>, prototypeCount> dispatcherList;
	mutable Mutex dispatcherListMutex;
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

