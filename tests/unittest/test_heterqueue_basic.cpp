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
#include "eventpp/hetereventqueue.h"

TEST_CASE("HeterEventQueue 1")
{
	eventpp::HeterEventQueue<std::string, std::tuple<void (), void (const std::string &)> > queue;

	int a = 1;
	int b = 5;

	queue.appendListener("event1", [&a](const std::string &) {
		a = 2;
	});
	queue.appendListener("event1", [&b]() {
		b = 8;
	});

	REQUIRE(a != 2);
	REQUIRE(b != 8);

	queue.enqueue("event1");
	queue.process();
	REQUIRE(a == 1);
	REQUIRE(b == 8);

	queue.enqueue("event1", "a");
	queue.process();
	REQUIRE(a == 2);
	REQUIRE(b == 8);
}

TEST_CASE("HeterEventQueue, processIf")
{
	struct MyEventPolicies
	{
		using ArgumentPassingMode = eventpp::ArgumentPassingIncludeEvent;
	};

	eventpp::HeterEventQueue<int, std::tuple<void (int), void (int, int)>, MyEventPolicies> queue;

	std::vector<int> dataList(3);

	queue.appendListener(5, [&dataList](int) {
		++dataList[0];
	});
	queue.appendListener(6, [&dataList](int) {
		++dataList[1];
	});
	queue.appendListener(7, [&dataList](int, int) {
		++dataList[2];
	});

	REQUIRE(dataList == std::vector<int>{ 0, 0, 0 });

	queue.enqueue(5);
	queue.enqueue(6);
	queue.enqueue(7, 8);
	queue.process();
	REQUIRE(dataList == std::vector<int>{ 1, 1, 1 });

	queue.enqueue(5);
	queue.enqueue(6);
	queue.enqueue(7, 8);
	queue.processIf([](const int event) -> bool { return event == 6; });
	REQUIRE(dataList == std::vector<int>{ 1, 2, 1 });
	// Now the queue contains 5, 7

	queue.enqueue(5);
	queue.enqueue(6);
	queue.enqueue(7, 8);
	queue.processIf([](const int event) -> bool { return event == 5; });
	REQUIRE(dataList == std::vector<int>{ 3, 2, 1 });
	// Now the queue contains 6, 7, 7

	queue.enqueue(5);
	queue.enqueue(6);
	queue.enqueue(7, 8);
	queue.processIf([](const int event, int) -> bool { return event == 7; });
	REQUIRE(dataList == std::vector<int>{ 3, 2, 4 });
	// Now the queue contains 5, 6, 6

	queue.process();
	REQUIRE(dataList == std::vector<int>{ 4, 4, 4 });
}

