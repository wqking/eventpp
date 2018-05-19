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

#include <thread>
#include <numeric>
#include <algorithm>
#include <random>
#include <vector>
#include <atomic>

TEST_CASE("queue, std::string, void (const std::string &)")
{
	eventpp::EventQueue<std::string, void (const std::string &)> dispatcher;

	int a = 1;
	int b = 5;

	dispatcher.appendListener("event1", [&a](const std::string &) {
		a = 2;
	});
	dispatcher.appendListener("event1", eraseArgs1([&b]() {
		b = 8;
	}));

	REQUIRE(a != 2);
	REQUIRE(b != 8);

	dispatcher.enqueue("event1");
	dispatcher.process();
	REQUIRE(a == 2);
	REQUIRE(b == 8);
}

TEST_CASE("queue, int, void ()")
{
	eventpp::EventQueue<int, void ()> dispatcher;

	int a = 1;
	int b = 5;

	dispatcher.appendListener(3, [&a]() {
		a += 1;
	});
	dispatcher.appendListener(3, [&b]() {
		b += 3;
	});

	REQUIRE(a != 2);
	REQUIRE(b != 8);

	dispatcher.enqueue(3);
	dispatcher.process();

	REQUIRE(a == 2);
	REQUIRE(b == 8);
}

TEST_CASE("queue, int, void (const std::string &, int)")
{
	eventpp::EventQueue<int, void (const std::string &, int)> dispatcher;

	const int event = 3;

	std::vector<std::string> sList(2);
	std::vector<int> iList(sList.size());

	dispatcher.appendListener(event, [&sList, &iList](const std::string & s, const int i) {
		sList[0] = s;
		iList[0] = i;
	});
	dispatcher.appendListener(event, [&sList, &iList](const std::string & s, const int i) {
		sList[1] = s + "2";
		iList[1] = i + 5;
	});

	REQUIRE(sList[0] != "first");
	REQUIRE(sList[1] != "first2");
	REQUIRE(iList[0] != 3);
	REQUIRE(iList[1] != 8);

	SECTION("Parameters") {
		dispatcher.enqueue(event, "first", 3);
		dispatcher.process();

		REQUIRE(sList[0] == "first");
		REQUIRE(sList[1] == "first2");
		REQUIRE(iList[0] == 3);
		REQUIRE(iList[1] == 8);
	}

	SECTION("Reference parameters should not be modified") {
		std::string s = "first";
		dispatcher.enqueue(event, s, 3);
		s = "";
		dispatcher.process();

		REQUIRE(sList[0] == "first");
		REQUIRE(sList[1] == "first2");
		REQUIRE(iList[0] == 3);
		REQUIRE(iList[1] == 8);
	}
}

TEST_CASE("queue multi threading, int, void (int)")
{
	using ED = eventpp::EventQueue<int, void (int)>;
	ED dispatcher;

	constexpr int threadCount = 256;
	constexpr int dataCountPerThread = 1024 * 4;
	constexpr int itemCount = threadCount * dataCountPerThread;

	std::vector<int> eventList(itemCount);
	std::iota(eventList.begin(), eventList.end(), 0);
	std::shuffle(eventList.begin(), eventList.end(), std::mt19937(std::random_device()()));

	std::vector<int> dataList(itemCount);

	for(int i = 0; i < itemCount; ++i) {
		dispatcher.appendListener(eventList[i], [&dispatcher, i, &dataList](const int d) {
			dataList[i] += d;
		});
	}

	std::vector<std::thread> threadList;
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([i, dataCountPerThread, &dispatcher, itemCount]() {
			for(int k = i * dataCountPerThread; k < (i + 1) * dataCountPerThread; ++k) {
				dispatcher.enqueue(k, 3);
			}
			for(int k = 0; k < 10; ++k) {
				dispatcher.process();
			}
		});
	}
	for(int i = 0; i < threadCount; ++i) {
		threadList[i].join();
	}

	std::vector<int> compareList(itemCount);
	std::fill(compareList.begin(), compareList.end(), 3);
	REQUIRE(dataList == compareList);
}

TEST_CASE("queue multi threading wait")
{
	using ED = eventpp::EventQueue<int, void (int)>;
	ED dispatcher;

	// note, all events will be process from the other thread instead of main thread
	constexpr int stopEvent = 1;
	constexpr int otherEvent = 2;

	constexpr int itemCount = 5;

	std::vector<int> dataList(itemCount);

	std::atomic<int> threadProcessCount(0);

	std::thread thread([stopEvent, otherEvent, &dataList, &dispatcher, &threadProcessCount]() {
		volatile bool shouldStop = false;
		dispatcher.appendListener(stopEvent, [&shouldStop](int) {
			shouldStop = true;
		});
		dispatcher.appendListener(otherEvent, [&dataList](const int index) {
			dataList[index] += index + 1;
		});

		while(! shouldStop) {
			dispatcher.wait();

			++threadProcessCount;

			dispatcher.process();
		}
	});
	
	REQUIRE(threadProcessCount.load() == 0);

	auto waitUntilQueueEmpty = [&dispatcher]() {
		while(dispatcher.waitFor(std::chrono::nanoseconds(0))) ;
	};

	SECTION("Enqueue one by one") {
		dispatcher.enqueue(otherEvent, 1);
		waitUntilQueueEmpty();
		REQUIRE(threadProcessCount.load() == 1);
		REQUIRE(dispatcher.isQueueEmpty());
		REQUIRE(dataList == std::vector<int>{ 0, 2, 0, 0, 0 });

		dispatcher.enqueue(otherEvent, 3);
		waitUntilQueueEmpty();
		REQUIRE(threadProcessCount.load() == 2);
		REQUIRE(dispatcher.isQueueEmpty());
		REQUIRE(dataList == std::vector<int>{ 0, 2, 0, 4, 0 });
	}

	SECTION("Enqueue two") {
		dispatcher.enqueue(otherEvent, 1);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		REQUIRE(threadProcessCount.load() == 1);
		REQUIRE(dispatcher.isQueueEmpty());

		dispatcher.enqueue(otherEvent, 3);
		waitUntilQueueEmpty();

		REQUIRE(threadProcessCount.load() == 2);
		REQUIRE(dataList == std::vector<int>{ 0, 2, 0, 4, 0 });
	}

	SECTION("Batching enqueue") {
		{
			ED::DisableQueueNotify disableNotify(&dispatcher);

			dispatcher.enqueue(otherEvent, 2);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			REQUIRE(threadProcessCount.load() == 0);
			REQUIRE(! dispatcher.isQueueEmpty());

			dispatcher.enqueue(otherEvent, 4);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			REQUIRE(threadProcessCount.load() == 0);
			REQUIRE(! dispatcher.isQueueEmpty());
		}

		waitUntilQueueEmpty();
		REQUIRE(threadProcessCount.load() == 1);
		REQUIRE(dataList == std::vector<int>{ 0, 0, 3, 0, 5 });
	}

	dispatcher.enqueue(stopEvent, 1);
	thread.join();
}

