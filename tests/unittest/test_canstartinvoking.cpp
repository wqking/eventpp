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

#include "test.h"
#include "eventpp/callbacklist.h"
#include "eventpp/eventdispatcher.h"
#include "eventpp/eventqueue.h"

namespace {

struct MyEvent
{
	MyEvent() : type(0), canceled(false) {
	}
	explicit MyEvent(const int type, const bool canceled = false)
		: type(type), canceled(canceled) {
	}

	int type;
	mutable bool canceled;
};

// Only canContinueInvoking, this is the policy from the document.
struct ContinuePolicies
{
	static int getEvent(const MyEvent & e) {
		return e.type;
	}

	static bool canContinueInvoking(const MyEvent & e) {
		return ! e.canceled;
	}
};

// canContinueInvoking plus canStartInvoking
struct StartPolicies : public ContinuePolicies
{
	static bool canStartInvoking(const MyEvent & e) {
		return ! e.canceled;
	}
};

} //unnamed namespace

TEST_CASE("CallbackList, canStartInvoking, no policy keeps old behavior")
{
	eventpp::CallbackList<void (const MyEvent &), ContinuePolicies> callbackList;

	int count = 0;
	callbackList.append([&count](const MyEvent &) {
		++count;
	});
	callbackList.append([&count](const MyEvent &) {
		++count;
	});

	// without canStartInvoking, the first callback is always invoked,
	// then canContinueInvoking stops the invoking.
	callbackList(MyEvent(1, true));
	REQUIRE(count == 1);

	callbackList(MyEvent(1, false));
	REQUIRE(count == 3);
}

TEST_CASE("CallbackList, canStartInvoking, canceled event invokes no callback")
{
	eventpp::CallbackList<void (const MyEvent &), StartPolicies> callbackList;

	int count = 0;
	callbackList.append([&count](const MyEvent &) {
		++count;
	});
	callbackList.append([&count](const MyEvent &) {
		++count;
	});

	callbackList(MyEvent(1, true));
	REQUIRE(count == 0);

	callbackList(MyEvent(1, false));
	REQUIRE(count == 2);
}

TEST_CASE("CallbackList, canStartInvoking, canContinueInvoking still works after start")
{
	eventpp::CallbackList<void (const MyEvent &), StartPolicies> callbackList;

	int count = 0;
	callbackList.append([&count](const MyEvent & e) {
		++count;
		e.canceled = true;
	});
	callbackList.append([&count](const MyEvent &) {
		++count;
	});

	callbackList(MyEvent(1));
	REQUIRE(count == 1);
}

TEST_CASE("EventDispatcher, canStartInvoking")
{
	eventpp::EventDispatcher<int, void (const MyEvent &), StartPolicies> dispatcher;

	int count = 0;
	dispatcher.appendListener(3, [&count](const MyEvent &) {
		++count;
	});
	dispatcher.appendListener(3, [&count](const MyEvent &) {
		++count;
	});

	dispatcher.dispatch(MyEvent(3, true));
	REQUIRE(count == 0);

	dispatcher.dispatch(MyEvent(3));
	REQUIRE(count == 2);
}

TEST_CASE("EventQueue, canStartInvoking, checked when processing")
{
	eventpp::EventQueue<int, void (const MyEvent &), StartPolicies> queue;

	int count = 0;
	queue.appendListener(3, [&count](const MyEvent &) {
		++count;
	});

	// The queue stores a copy of the event, canStartInvoking sees the stored copy when processing.
	queue.enqueue(MyEvent(3));
	queue.enqueue(MyEvent(3, true));
	queue.enqueue(MyEvent(3));
	REQUIRE(count == 0);

	queue.process();
	REQUIRE(count == 2);
}

