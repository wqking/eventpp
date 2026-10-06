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
#include "eventpp/eventqueue.h"
#include "eventpp/hetereventqueue.h"
#include "eventpp/utilities/orderedqueuelist.h"

#include <stdexcept>
#include <vector>

// When a listener throws during processing, the event being dispatched is dropped (its listeners
// were invoked, maybe partially), the events not processed yet stay in the queue in their
// original order, and the exception propagates to the caller.

namespace {

struct ThrowOn
{
	explicit ThrowOn(const int value) : value(value) {
	}

	void operator() (const int e) const {
		if(e == value) {
			throw std::runtime_error("listener");
		}
	}

	int value;
};

struct OrderedListPolicies
{
	template <typename Item>
	using QueueList = eventpp::OrderedQueueList<Item, eventpp::OrderedQueueListCompare>;
};

} //unnamed namespace

TEST_CASE("EventQueue, process, listener throws, remaining events are kept")
{
	eventpp::EventQueue<int, void (int)> queue;

	std::vector<int> dataList;
	queue.appendListener(0, [&dataList](const int e) {
		dataList.push_back(e);
	});
	queue.appendListener(0, ThrowOn(2));
	std::vector<int> secondDataList;
	queue.appendListener(0, [&secondDataList](const int e) {
		secondDataList.push_back(e);
	});

	queue.enqueue(0, 1);
	queue.enqueue(0, 2);
	queue.enqueue(0, 3);
	queue.enqueue(0, 4);

	REQUIRE_THROWS_AS(queue.process(), std::runtime_error);

	// 1 was fully processed, 2 reached the first listener and threw in the second, 3 and 4 were not processed
	REQUIRE(dataList == std::vector<int>{ 1, 2 });
	REQUIRE(secondDataList == std::vector<int>{ 1 });
	REQUIRE(! queue.emptyQueue());

	// events enqueued after the exception come after the kept events
	queue.enqueue(0, 5);

	REQUIRE(queue.process());

	// 2 is not processed again
	REQUIRE(dataList == std::vector<int>{ 1, 2, 3, 4, 5 });
	REQUIRE(secondDataList == std::vector<int>{ 1, 3, 4, 5 });
	REQUIRE(queue.emptyQueue());
}

TEST_CASE("EventQueue, processOne, listener throws, the event is dropped")
{
	eventpp::EventQueue<int, void (int)> queue;

	std::vector<int> dataList;
	queue.appendListener(0, ThrowOn(2));
	queue.appendListener(0, [&dataList](const int e) {
		dataList.push_back(e);
	});

	queue.enqueue(0, 1);
	queue.enqueue(0, 2);
	queue.enqueue(0, 3);

	REQUIRE(queue.processOne());
	REQUIRE_THROWS_AS(queue.processOne(), std::runtime_error);
	REQUIRE(! queue.emptyQueue());
	REQUIRE(queue.processOne());
	REQUIRE(! queue.processOne());

	REQUIRE(dataList == std::vector<int>{ 1, 3 });
}

TEST_CASE("EventQueue, processIf, listener throws, remaining events are kept in order")
{
	eventpp::EventQueue<int, void (int)> queue;

	std::vector<int> dataList;
	queue.appendListener(0, ThrowOn(3));
	queue.appendListener(0, [&dataList](const int e) {
		dataList.push_back(e);
	});

	// 1 and 2 are not selected by the predictor, 3 throws, 4 and 5 are not processed yet
	for(int i = 1; i <= 5; ++i) {
		queue.enqueue(0, i);
	}

	REQUIRE_THROWS_AS(queue.processIf([](const int e) { return e >= 3; }), std::runtime_error);
	REQUIRE(dataList.empty());
	REQUIRE(! queue.emptyQueue());

	REQUIRE(queue.process());
	REQUIRE(dataList == std::vector<int>{ 1, 2, 4, 5 });
	REQUIRE(queue.emptyQueue());
}

TEST_CASE("EventQueue, processIf, predictor throws, the event is kept")
{
	eventpp::EventQueue<int, void (int)> queue;

	std::vector<int> dataList;
	queue.appendListener(0, [&dataList](const int e) {
		dataList.push_back(e);
	});

	queue.enqueue(0, 1);
	queue.enqueue(0, 2);
	queue.enqueue(0, 3);

	REQUIRE_THROWS_AS(queue.processIf([](const int e) -> bool {
		if(e == 2) {
			throw std::runtime_error("predictor");
		}
		return true;
	}), std::runtime_error);

	REQUIRE(dataList == std::vector<int>{ 1 });

	// nothing was dispatched for 2, so it's still in the queue
	REQUIRE(queue.process());
	REQUIRE(dataList == std::vector<int>{ 1, 2, 3 });
}

TEST_CASE("EventQueue, processUntil, listener throws, remaining events are kept")
{
	eventpp::EventQueue<int, void (int)> queue;

	std::vector<int> dataList;
	queue.appendListener(0, ThrowOn(2));
	queue.appendListener(0, [&dataList](const int e) {
		dataList.push_back(e);
	});

	for(int i = 1; i <= 4; ++i) {
		queue.enqueue(0, i);
	}

	REQUIRE_THROWS_AS(queue.processUntil([](const int e) { return e == 4; }), std::runtime_error);
	REQUIRE(dataList == std::vector<int>{ 1 });

	REQUIRE(queue.processUntil([](const int e) { return e == 4; }));
	REQUIRE(dataList == std::vector<int>{ 1, 3 });
	REQUIRE(! queue.emptyQueue());

	REQUIRE(queue.process());
	REQUIRE(dataList == std::vector<int>{ 1, 3, 4 });
}

TEST_CASE("EventQueue, process, listener throws, waiting thread is woken up for the kept events")
{
	eventpp::EventQueue<int, void (int)> queue;

	queue.appendListener(0, ThrowOn(1));
	queue.enqueue(0, 1);
	queue.enqueue(0, 2);

	REQUIRE_THROWS_AS(queue.process(), std::runtime_error);

	// waitFor must return true immediately because event 2 is still in the queue
	REQUIRE(queue.waitFor(std::chrono::milliseconds(0)));
}

TEST_CASE("EventQueue, ordered list, process, listener throws, remaining events are kept in order")
{
	eventpp::EventQueue<int, void (int), OrderedListPolicies> queue;

	std::vector<int> dataList;
	for(int e = 1; e <= 5; ++e) {
		queue.appendListener(e, ThrowOn(3));
		queue.appendListener(e, [&dataList](const int e) {
			dataList.push_back(e);
		});
	}

	// enqueued out of order, the ordered list sorts them by event
	queue.enqueue(5, 5);
	queue.enqueue(3, 3);
	queue.enqueue(1, 1);
	queue.enqueue(4, 4);
	queue.enqueue(2, 2);

	REQUIRE_THROWS_AS(queue.process(), std::runtime_error);
	REQUIRE(dataList == std::vector<int>{ 1, 2 });
	REQUIRE(! queue.emptyQueue());

	REQUIRE(queue.process());
	REQUIRE(dataList == std::vector<int>{ 1, 2, 4, 5 });
	REQUIRE(queue.emptyQueue());
}

TEST_CASE("HeterEventQueue, process, listener throws, remaining events are kept")
{
	eventpp::HeterEventQueue<int, eventpp::HeterTuple<void (int), void (int, int)> > queue;

	std::vector<int> dataList;
	queue.appendListener(0, ThrowOn(2));
	queue.appendListener(0, [&dataList](const int e) {
		dataList.push_back(e);
	});
	queue.appendListener(0, [&dataList](const int a, const int b) {
		dataList.push_back(a * 10 + b);
	});

	queue.enqueue(0, 1);
	queue.enqueue(0, 2);
	queue.enqueue(0, 3, 4);
	queue.enqueue(0, 5);

	REQUIRE_THROWS_AS(queue.process(), std::runtime_error);
	REQUIRE(dataList == std::vector<int>{ 1 });
	REQUIRE(! queue.emptyQueue());

	REQUIRE(queue.process());
	REQUIRE(dataList == std::vector<int>{ 1, 34, 5 });
	REQUIRE(queue.emptyQueue());
}

TEST_CASE("HeterEventQueue, processOne, listener throws, the event is dropped")
{
	eventpp::HeterEventQueue<int, eventpp::HeterTuple<void (int)> > queue;

	std::vector<int> dataList;
	queue.appendListener(0, ThrowOn(2));
	queue.appendListener(0, [&dataList](const int e) {
		dataList.push_back(e);
	});

	queue.enqueue(0, 1);
	queue.enqueue(0, 2);
	queue.enqueue(0, 3);

	REQUIRE(queue.processOne());
	REQUIRE_THROWS_AS(queue.processOne(), std::runtime_error);
	REQUIRE(queue.processOne());
	REQUIRE(! queue.processOne());

	REQUIRE(dataList == std::vector<int>{ 1, 3 });
}

TEST_CASE("HeterEventQueue, processIf, listener throws, remaining events are kept")
{
	eventpp::HeterEventQueue<int, eventpp::HeterTuple<void (int)> > queue;

	std::vector<int> dataList;
	queue.appendListener(0, ThrowOn(3));
	queue.appendListener(0, [&dataList](const int e) {
		dataList.push_back(e);
	});

	for(int i = 1; i <= 5; ++i) {
		queue.enqueue(0, i);
	}

	REQUIRE_THROWS_AS(queue.processIf([](const int e) { return e >= 3; }), std::runtime_error);
	REQUIRE(dataList.empty());

	REQUIRE(queue.process());
	REQUIRE(dataList == std::vector<int>{ 1, 2, 4, 5 });
}

