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

#ifndef CALLBACKLIST_H_588722158669
#define CALLBACKLIST_H_588722158669

#include <functional>
#include <memory>
#include <mutex>
#include <utility>

namespace eventpp {

namespace _internal {

struct DummyMutex
{
	void lock() {}
	void unlock() {}
};

template <typename F, typename ...A>
struct CanInvoke
{
	template <typename U, typename ...X>
	static auto invoke(int) -> decltype(std::declval<U>()(std::declval<X>()...), std::true_type());

	template <typename U, typename ...X>
	static auto invoke(...) -> std::false_type;

	enum {
		value = !! decltype(invoke<F, A...>(0))()
	};
};

template <
	typename CallbackType,
	typename Threading,
	typename ReturnType, typename ...Args
>
class CallbackListBase;

template <
	typename CallbackType,
	typename Threading,
	typename ReturnType, typename ...Args
>
class CallbackListBase<
	CallbackType,
	Threading,
	ReturnType (Args...)
>
{
private:
	using Mutex = typename Threading::Mutex;
	using _Callback = typename std::conditional<
		std::is_same<CallbackType, void>::value,
		std::function<ReturnType (Args...)>,
		CallbackType
	>::type;

	struct Node;
	using NodePtr = std::shared_ptr<Node>;

	struct Node
	{
		_Callback callback;
		NodePtr previous;
		NodePtr next;
	};

	class _Handle : public std::weak_ptr<Node>
	{
	private:
		using super = std::weak_ptr<Node>;

	public:
		using super::super;

		operator bool () const noexcept {
			return ! this->expired();
		}
	};

public:
	using Callback = _Callback;
	using Handle = _Handle;

public:
	CallbackListBase() = default;
	CallbackListBase(CallbackListBase &&) = delete;
	CallbackListBase(const CallbackListBase &) = delete;
	CallbackListBase & operator = (const CallbackListBase &) = delete;

	~CallbackListBase()
	{
		// Don't lock mutex here since it may throw exception

		NodePtr node = head;
		head.reset();
		while(node) {
			NodePtr next = node->next;
			node->previous.reset();
			node->next.reset();
			node = next;
		}
		node.reset();
	}

	Handle append(const Callback & callback)
	{
		NodePtr node(std::make_shared<Node>());
		node->callback = callback;

		std::lock_guard<Mutex> lockGuard(mutex);

		if(! head) {
			head = node;
			tail = node;
		}
		else {
			node->previous = tail;
			tail->next = node;
			tail = node;
		}

		return Handle(node);
	}

	Handle prepend(const Callback & callback)
	{
		NodePtr node(std::make_shared<Node>());
		node->callback = callback;

		std::lock_guard<Mutex> lockGuard(mutex);

		if(! head) {
			head = node;
			tail = node;
		}
		else {
			node->next = head;
			head->previous = node;
			head = node;
		}

		return Handle(node);
	}

	Handle insert(const Callback & callback, const Handle before)
	{
		NodePtr beforeNode = before.lock();
		if(beforeNode) {
			NodePtr node(std::make_shared<Node>());
			node->callback = callback;

			std::lock_guard<Mutex> lockGuard(mutex);

			doInert(beforeNode, node);

			return Handle(node);
		}

		return append(callback);
	}

	bool remove(const Handle handle)
	{
		std::lock_guard<Mutex> lockGuard(mutex);
		auto node = handle.lock();
		if(node) {
			doRemoveNode(node);
			return true;
		}

		return false;
	}

	template <typename Func>
	void forEach(Func && func)
	{
		NodePtr node;

		{
			std::lock_guard<Mutex> lockGuard(mutex);
			node = head;
		}

		while(node) {
			doForEachInvoke(func, Handle(node), node->callback);

			{
				std::lock_guard<Mutex> lockGuard(mutex);
				node = node->next;
			}
		}
	}

	void operator() (Args ...args)
	{
		NodePtr node;

		{
			std::lock_guard<Mutex> lockGuard(mutex);
			node = head;
		}

		while(node) {
			// Must not hold any lock when invoking the callback
			// because the callback may append/remove/dispatch again and cause recursive lock
			node->callback(std::forward<Args>(args)...);

			{
				std::lock_guard<Mutex> lockGuard(mutex);
				node = node->next;
			}
		}
	}

private:
	template <typename Func>
	auto doForEachInvoke(Func && func, const Handle & handle, const Callback & callback)
		-> typename std::enable_if<CanInvoke<Func, Handle, Callback>::value, void>::type
	{
		func(handle, callback);
	}

	template <typename Func>
	auto doForEachInvoke(Func && func, const Handle & handle, const Callback & callback)
		-> typename std::enable_if<CanInvoke<Func, Handle>::value, void>::type
	{
		func(handle);
	}

	template <typename Func>
	auto doForEachInvoke(Func && func, const Handle & handle, const Callback & callback)
		-> typename std::enable_if<CanInvoke<Func, Callback>::value, void>::type
	{
		func(callback);
	}

	void doRemoveNode(NodePtr & node)
	{
		if(node->next) {
			node->next->previous = node->previous;
		}
		if(node->previous) {
			node->previous->next = node->next;
		}

		if(head == node) {
			head = node->next;
		}
		if(tail == node) {
			tail = node->previous;
		}

		// don't modify node->next or node->previous
		// because node may be still used in a loop.
	}

	void doInert(NodePtr & before, NodePtr & node)
	{
		node->previous = before->previous;
		node->next = before;
		if(before->previous) {
			before->previous->next = node;
		}
		before->previous = node;

		if(before == head) {
			head = node;
		}
	}

private:
	NodePtr head;
	NodePtr tail;
	Mutex mutex;

};


} //namespace _internal


struct MultipleThreading
{
	using Mutex = std::mutex;
};
struct SingleThreading
{
	using Mutex = _internal::DummyMutex;
};

template <
	typename Prototype,
	typename Callback = void,
	typename Threading = MultipleThreading
>
class CallbackList : public _internal::CallbackListBase<Callback, Threading, Prototype>
{
};


} //namespace eventpp


#endif
