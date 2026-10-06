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
#include "eventpp/eventdispatcher.h"
#include "eventpp/eventqueue.h"
#include "eventpp/hetereventdispatcher.h"

#include <thread>
#include <numeric>
#include <random>
#include <algorithm>
#include <mutex>
#include <atomic>

namespace {

// A shared mutex that counts how many times it's locked shared or exclusive.
// It's not a real reader/writer lock, it's only to check which lock the dispatcher uses.
struct CountingSharedMutex
{
	void lock() {
		++exclusiveCount;
		mutex.lock();
	}

	void unlock() {
		mutex.unlock();
	}

	void lock_shared() {
		++sharedCount;
		mutex.lock();
	}

	void unlock_shared() {
		mutex.unlock();
	}

	static void reset() {
		sharedCount = 0;
		exclusiveCount = 0;
	}

	static std::atomic<int> sharedCount;
	static std::atomic<int> exclusiveCount;

	std::mutex mutex;
};

std::atomic<int> CountingSharedMutex::sharedCount(0);
std::atomic<int> CountingSharedMutex::exclusiveCount(0);

struct CountingThreading : public eventpp::MultipleThreading
{
	using SharedMutex = CountingSharedMutex;
};

struct CountingPolicies
{
	using Threading = CountingThreading;
};

} //unnamed namespace

TEST_CASE("EventDispatcher, SharedMutex, lock shared for lookup and lock exclusive for adding")
{
	using ED = eventpp::EventDispatcher<int, void (int), CountingPolicies>;
	ED dispatcher;

	CountingSharedMutex::reset();

	ED::Handle handle = dispatcher.appendListener(1, [](int) {});
	dispatcher.prependListener(1, [](int) {});
	dispatcher.insertListener(2, [](int) {}, handle);
	REQUIRE(CountingSharedMutex::exclusiveCount == 3);
	REQUIRE(CountingSharedMutex::sharedCount == 0);

	dispatcher.dispatch(1, 0);
	dispatcher.dispatch(2, 0);
	dispatcher.dispatch(3, 0);
	REQUIRE(dispatcher.hasAnyListener(1));
	REQUIRE(! dispatcher.hasAnyListener(3));
	REQUIRE(dispatcher.ownsHandle(1, handle));
	dispatcher.forEach(1, [](const ED::Callback &) {});
	dispatcher.forEachIf(1, [](const ED::Callback &) { return true; });
	REQUIRE(dispatcher.removeListener(1, handle));
	REQUIRE(CountingSharedMutex::exclusiveCount == 3);
	REQUIRE(CountingSharedMutex::sharedCount == 9);
}

TEST_CASE("EventDispatcher, SharedMutex, int, void (int)")
{
	using ED = eventpp::EventDispatcher<int, void (int), CountingPolicies>;
	ED dispatcher;

	std::vector<int> dataList(3);

	dispatcher.appendListener(3, [&dataList](const int e) {
		dataList[0] += e;
	});
	dispatcher.appendListener(5, [&dataList](const int e) {
		dataList[1] += e;
	});
	dispatcher.appendListener(5, [&dataList](const int e) {
		dataList[2] += e;
	});

	REQUIRE(dataList == std::vector<int>{ 0, 0, 0 });

	dispatcher.dispatch(3, 1);
	REQUIRE(dataList == std::vector<int>{ 1, 0, 0 });

	dispatcher.dispatch(5, 2);
	REQUIRE(dataList == std::vector<int>{ 1, 2, 2 });

	// nested dispatching and adding listener inside a listener
	dispatcher.appendListener(7, [&dispatcher, &dataList](const int) {
		dispatcher.dispatch(3, 10);
		dispatcher.appendListener(9, [&dataList](const int e) {
			dataList[0] += e;
		});
	});
	dispatcher.dispatch(7, 0);
	REQUIRE(dataList == std::vector<int>{ 11, 2, 2 });
	dispatcher.dispatch(9, 100);
	REQUIRE(dataList == std::vector<int>{ 111, 2, 2 });
}

TEST_CASE("EventDispatcher, SharedMutex, multi threading, int, void (int)")
{
	using ED = eventpp::EventDispatcher<int, void (int), CountingPolicies>;
	ED dispatcher;

	constexpr int threadCount = 256;
	constexpr int eventCountPerThread = 1024 * 4;
	constexpr int itemCount = threadCount * eventCountPerThread;

	std::vector<int> eventList(itemCount);
	std::iota(eventList.begin(), eventList.end(), 0);
	std::shuffle(eventList.begin(), eventList.end(), std::mt19937(std::random_device()()));

	std::vector<int> dataList(itemCount);
	std::vector<ED::Handle> handleList(itemCount);

	std::vector<std::thread> threadList;

	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([i, eventCountPerThread, &dispatcher, &eventList, &handleList, &dataList]() {
			for(int k = i * eventCountPerThread; k < (i + 1) * eventCountPerThread; ++k) {
				handleList[k] = dispatcher.appendListener(eventList[k], [&dispatcher, k, &dataList, &eventList, &handleList](const int e) {
					dataList[k] += e;
					dispatcher.removeListener(eventList[k], handleList[k]);
				});
			}
		});
	}
	for(int i = 0; i < threadCount; ++i) {
		threadList[i].join();
	}

	threadList.clear();
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([i, eventCountPerThread, &dispatcher, &eventList]() {
			for(int k = i * eventCountPerThread; k < (i + 1) * eventCountPerThread; ++k) {
				dispatcher.dispatch(eventList[k]);
			}
		});
	}
	for(int i = 0; i < threadCount; ++i) {
		threadList[i].join();
	}

	std::sort(eventList.begin(), eventList.end());
	std::sort(dataList.begin(), dataList.end());

	REQUIRE(eventList == dataList);
}

TEST_CASE("EventQueue, SharedMutex, int, void (int)")
{
	eventpp::EventQueue<int, void (int), CountingPolicies> queue;

	std::vector<int> dataList(2);

	queue.appendListener(3, [&dataList](const int e) {
		dataList[0] += e;
	});
	queue.appendListener(5, [&dataList](const int e) {
		dataList[1] += e;
	});

	queue.enqueue(3, 1);
	queue.enqueue(5, 2);
	REQUIRE(dataList == std::vector<int>{ 0, 0 });

	queue.process();
	REQUIRE(dataList == std::vector<int>{ 1, 2 });
}

TEST_CASE("HeterEventDispatcher, SharedMutex")
{
	using ED = eventpp::HeterEventDispatcher<int, eventpp::HeterTuple<void (int), void (int, int)>, CountingPolicies>;
	ED dispatcher;

	CountingSharedMutex::reset();

	std::vector<int> dataList(2);

	dispatcher.appendListener(3, [&dataList](const int e) {
		dataList[0] += e;
	});
	dispatcher.appendListener(3, [&dataList](const int a, const int b) {
		dataList[1] += a + b;
	});
	REQUIRE(CountingSharedMutex::exclusiveCount == 2);
	REQUIRE(CountingSharedMutex::sharedCount == 0);

	dispatcher.dispatch(3, 1);
	dispatcher.dispatch(3, 2, 3);
	REQUIRE(dataList == std::vector<int>{ 1, 5 });
	REQUIRE(CountingSharedMutex::exclusiveCount == 2);
	REQUIRE(CountingSharedMutex::sharedCount == 2);
}

#ifdef EVENTPP_HAS_STD_SHARED_MUTEX

TEST_CASE("EventDispatcher, MultipleThreadingSharedMutex, multi threading, int, void (int)")
{
	struct MyPolicies
	{
		using Threading = eventpp::MultipleThreadingSharedMutex;
	};
	using ED = eventpp::EventDispatcher<int, void (int), MyPolicies>;
	ED dispatcher;

	constexpr int threadCount = 64;
	constexpr int eventCountPerThread = 1024;
	constexpr int itemCount = threadCount * eventCountPerThread;

	std::vector<int> eventList(itemCount);
	std::iota(eventList.begin(), eventList.end(), 0);
	std::shuffle(eventList.begin(), eventList.end(), std::mt19937(std::random_device()()));

	std::vector<int> dataList(itemCount);

	for(int k = 0; k < itemCount; ++k) {
		dispatcher.appendListener(eventList[k], [k, &dataList](const int e) {
			dataList[k] += e;
		});
	}

	std::vector<std::thread> threadList;
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([i, eventCountPerThread, &dispatcher, &eventList]() {
			for(int k = i * eventCountPerThread; k < (i + 1) * eventCountPerThread; ++k) {
				dispatcher.dispatch(eventList[k]);
			}
		});
	}
	for(int i = 0; i < threadCount; ++i) {
		threadList[i].join();
	}

	std::sort(eventList.begin(), eventList.end());
	std::sort(dataList.begin(), dataList.end());

	REQUIRE(eventList == dataList);
}

#endif

