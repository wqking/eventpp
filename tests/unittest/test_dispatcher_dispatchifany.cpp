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
#include "eventpp/hetercallbacklist.h"
#include "eventpp/eventdispatcher.h"
#include "eventpp/eventqueue.h"
#include "eventpp/hetereventdispatcher.h"
#include "eventpp/mixins/mixinfilter.h"

TEST_CASE("CallbackList, invokeIfAny")
{
	eventpp::CallbackList<void (int)> callbackList;

	std::vector<int> dataList(2);

	REQUIRE(! callbackList.invokeIfAny(3));
	REQUIRE(dataList == std::vector<int>{ 0, 0 });

	auto handle1 = callbackList.append([&dataList](const int e) {
		dataList[0] += e;
	});
	auto handle2 = callbackList.append([&dataList](const int e) {
		dataList[1] += e;
	});

	REQUIRE(callbackList.invokeIfAny(3));
	REQUIRE(dataList == std::vector<int>{ 3, 3 });

	callbackList.remove(handle1);
	REQUIRE(callbackList.invokeIfAny(5));
	REQUIRE(dataList == std::vector<int>{ 3, 8 });

	// The list is empty after removing all callbacks
	callbackList.remove(handle2);
	REQUIRE(! callbackList.invokeIfAny(7));
	REQUIRE(dataList == std::vector<int>{ 3, 8 });
}

struct StopAfterFirstCallbackPolicies
{
	static bool canContinueInvoking(int) {
		return false;
	}
};

TEST_CASE("CallbackList, invokeIfAny, canContinueInvoking stops after the first callback")
{
	eventpp::CallbackList<void (int), StopAfterFirstCallbackPolicies> callbackList;

	std::vector<int> dataList(2);

	callbackList.append([&dataList](const int e) {
		dataList[0] += e;
	});
	callbackList.append([&dataList](const int e) {
		dataList[1] += e;
	});

	// The first callback was invoked, so the result is true even though the second one was skipped
	REQUIRE(callbackList.invokeIfAny(3));
	REQUIRE(dataList == std::vector<int>{ 3, 0 });
}

TEST_CASE("HeterCallbackList, invokeIfAny")
{
	eventpp::HeterCallbackList<eventpp::HeterTuple<void (int), void (int, int)> > callbackList;

	std::vector<int> dataList(2);

	REQUIRE(! callbackList.invokeIfAny(3));
	REQUIRE(! callbackList.invokeIfAny(3, 5));

	callbackList.append([&dataList](const int e) {
		dataList[0] += e;
	});

	REQUIRE(callbackList.invokeIfAny(3));
	REQUIRE(dataList == std::vector<int>{ 3, 0 });

	// Only the callbacks matching the prototype count
	REQUIRE(! callbackList.invokeIfAny(3, 5));
	REQUIRE(dataList == std::vector<int>{ 3, 0 });

	callbackList.append([&dataList](const int a, const int b) {
		dataList[1] += a + b;
	});

	REQUIRE(callbackList.invokeIfAny(3, 5));
	REQUIRE(dataList == std::vector<int>{ 3, 8 });
}

TEST_CASE("EventDispatcher, dispatchIfAny, int, void (int)")
{
	eventpp::EventDispatcher<int, void (int)> dispatcher;

	std::vector<int> dataList(2);

	REQUIRE(! dispatcher.dispatchIfAny(3));
	REQUIRE(! dispatcher.dispatchIfAny(5));

	auto handle = dispatcher.appendListener(3, [&dataList](const int e) {
		dataList[0] += e;
	});
	dispatcher.appendListener(5, [&dataList](const int e) {
		dataList[1] += e;
	});

	REQUIRE(dispatcher.dispatchIfAny(3));
	REQUIRE(dataList == std::vector<int>{ 3, 0 });

	REQUIRE(dispatcher.dispatchIfAny(5));
	REQUIRE(dataList == std::vector<int>{ 3, 5 });

	// Event 7 was never registered
	REQUIRE(! dispatcher.dispatchIfAny(7));
	REQUIRE(dataList == std::vector<int>{ 3, 5 });

	// The callback list of 3 exists but is empty after removing the listener
	dispatcher.removeListener(3, handle);
	REQUIRE(! dispatcher.dispatchIfAny(3));
	REQUIRE(dataList == std::vector<int>{ 3, 5 });

	// dispatch still works and returns nothing
	dispatcher.dispatch(5);
	REQUIRE(dataList == std::vector<int>{ 3, 10 });
}

TEST_CASE("EventDispatcher, dispatchIfAny, int, void (int, int), exclude event")
{
	eventpp::EventDispatcher<int, void (int, int)> dispatcher;

	std::vector<int> dataList(2);

	dispatcher.appendListener(3, [&dataList](const int a, const int b) {
		dataList[0] += a + b;
	});

	REQUIRE(dispatcher.dispatchIfAny(3, 1, 2));
	REQUIRE(dataList == std::vector<int>{ 3, 0 });

	REQUIRE(! dispatcher.dispatchIfAny(5, 1, 2));
	REQUIRE(dataList == std::vector<int>{ 3, 0 });
}

TEST_CASE("EventDispatcher, dispatchIfAny, getEvent")
{
	struct MyEvent {
		int type;
		int param;
	};
	struct EventPolicies
	{
		static int getEvent(const MyEvent & e) {
			return e.type;
		}
	};

	eventpp::EventDispatcher<int, void (const MyEvent &), EventPolicies> dispatcher;

	int param = 0;
	dispatcher.appendListener(3, [&param](const MyEvent & e) {
		param = e.param;
	});

	REQUIRE(dispatcher.dispatchIfAny(MyEvent{ 3, 5 }));
	REQUIRE(param == 5);

	REQUIRE(! dispatcher.dispatchIfAny(MyEvent{ 4, 8 }));
	REQUIRE(param == 5);
}

TEST_CASE("EventDispatcher, dispatchIfAny, directDispatchIfAny bypasses getEvent")
{
	struct EventPolicies
	{
		static int getEvent(const int e) {
			return e * 10;
		}
	};

	eventpp::EventDispatcher<int, void (int), EventPolicies> dispatcher;

	int count = 0;
	dispatcher.appendListener(30, [&count](const int) {
		++count;
	});

	// getEvent maps 3 to 30
	REQUIRE(dispatcher.dispatchIfAny(3));
	REQUIRE(! dispatcher.dispatchIfAny(30));
	REQUIRE(count == 1);

	// directDispatchIfAny takes the event as is
	REQUIRE(dispatcher.directDispatchIfAny(30, 0));
	REQUIRE(count == 2);
	REQUIRE(! dispatcher.directDispatchIfAny(3, 0));
	REQUIRE(count == 2);
}

TEST_CASE("EventDispatcher, dispatchIfAny, blocked by MixinFilter returns false")
{
	struct MyPolicies {
		using Mixins = eventpp::MixinList<eventpp::MixinFilter>;
	};
	eventpp::EventDispatcher<int, void (int), MyPolicies> dispatcher;

	int count = 0;
	dispatcher.appendListener(3, [&count](const int) {
		++count;
	});

	REQUIRE(dispatcher.dispatchIfAny(3));
	REQUIRE(count == 1);

	dispatcher.appendFilter([](const int) -> bool {
		return false;
	});

	REQUIRE(! dispatcher.dispatchIfAny(3));
	REQUIRE(count == 1);
}

TEST_CASE("EventQueue, dispatchIfAny")
{
	eventpp::EventQueue<int, void (int)> queue;

	int count = 0;
	queue.appendListener(3, [&count](const int e) {
		count += e;
	});

	// EventQueue inherits dispatchIfAny from EventDispatcher
	REQUIRE(queue.dispatchIfAny(3));
	REQUIRE(! queue.dispatchIfAny(5));
	REQUIRE(count == 3);

	queue.enqueue(3);
	queue.process();
	REQUIRE(count == 6);
}

TEST_CASE("HeterEventDispatcher, dispatchIfAny")
{
	eventpp::HeterEventDispatcher<int, eventpp::HeterTuple<void (int), void (int, int)> > dispatcher;

	std::vector<int> dataList(2);

	REQUIRE(! dispatcher.dispatchIfAny(3, 1));
	REQUIRE(! dispatcher.dispatchIfAny(3, 1, 2));

	dispatcher.appendListener(3, [&dataList](const int e) {
		dataList[0] += e;
	});

	// Only the prototype void (int) has a listener
	REQUIRE(dispatcher.dispatchIfAny(3, 1));
	REQUIRE(! dispatcher.dispatchIfAny(3, 1, 2));
	REQUIRE(dataList == std::vector<int>{ 1, 0 });

	dispatcher.appendListener(3, [&dataList](const int a, const int b) {
		dataList[1] += a + b;
	});

	REQUIRE(dispatcher.dispatchIfAny(3, 1, 2));
	REQUIRE(dataList == std::vector<int>{ 1, 3 });

	REQUIRE(! dispatcher.dispatchIfAny(5, 1));

	REQUIRE(dispatcher.directDispatchIfAny(3, 1));
	REQUIRE(! dispatcher.directDispatchIfAny(5, 1));
	REQUIRE(dataList == std::vector<int>{ 2, 3 });
}

TEST_CASE("HeterEventDispatcher, dispatchIfAny, ArgumentPassingIncludeEvent")
{
	struct MyPolicies
	{
		using ArgumentPassingMode = eventpp::ArgumentPassingIncludeEvent;
	};
	eventpp::HeterEventDispatcher<int, eventpp::HeterTuple<void (int), void (int, int)>, MyPolicies> dispatcher;

	int count = 0;
	dispatcher.appendListener(3, [&count](const int e) {
		count += e;
	});

	REQUIRE(dispatcher.dispatchIfAny(3));
	REQUIRE(count == 3);
	REQUIRE(! dispatcher.dispatchIfAny(3, 1));
	REQUIRE(! dispatcher.dispatchIfAny(5));
	REQUIRE(count == 3);
}

