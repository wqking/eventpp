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

#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <type_traits>

// std::condition_variable only works with std::mutex. GeneralThreading with any other mutex
// (such as SpinLock) must default to std::condition_variable_any, otherwise EventQueue::wait
// doesn't compile.

static_assert(std::is_same<eventpp::GeneralThreading<std::mutex>::ConditionVariable, std::condition_variable>::value,
	"GeneralThreading<std::mutex> must keep std::condition_variable");
static_assert(std::is_same<eventpp::GeneralThreading<eventpp::SpinLock>::ConditionVariable, std::condition_variable_any>::value,
	"GeneralThreading<SpinLock> must use std::condition_variable_any");
static_assert(std::is_same<eventpp::GeneralThreading<eventpp::SpinLock, std::atomic, std::condition_variable_any>::ConditionVariable, std::condition_variable_any>::value,
	"explicit ConditionVariable_ is kept");

namespace {

struct SpinLockPolicies
{
	using Threading = eventpp::GeneralThreading<eventpp::SpinLock>;
};

// The sample from the document
struct MultipleThreadingSpinLock
{
	using Mutex = eventpp::SpinLock;

	template <typename T>
	using Atomic = std::atomic<T>;

	using ConditionVariable = std::condition_variable_any;
};

struct DocumentSpinLockPolicies
{
	using Threading = MultipleThreadingSpinLock;
};

template <typename Queue>
void testWait()
{
	Queue queue;

	std::atomic<int> dataList(0);
	queue.appendListener(3, [&dataList](const int e) {
		dataList += e;
	});

	REQUIRE(! queue.waitFor(std::chrono::milliseconds(10)));

	std::thread producer([&queue]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		queue.enqueue(3, 5);
	});

	queue.wait();
	REQUIRE(queue.process());
	REQUIRE(dataList.load() == 5);

	producer.join();

	queue.enqueue(3, 7);
	REQUIRE(queue.waitFor(std::chrono::milliseconds(0)));
	REQUIRE(queue.process());
	REQUIRE(dataList.load() == 12);
}

} //unnamed namespace

TEST_CASE("EventQueue, GeneralThreading<SpinLock>, wait and waitFor")
{
	testWait<eventpp::EventQueue<int, void (int), SpinLockPolicies> >();
}

TEST_CASE("EventQueue, document sample MultipleThreadingSpinLock, wait and waitFor")
{
	testWait<eventpp::EventQueue<int, void (int), DocumentSpinLockPolicies> >();
}

TEST_CASE("HeterEventQueue, GeneralThreading<SpinLock>, wait and waitFor")
{
	testWait<eventpp::HeterEventQueue<int, eventpp::HeterTuple<void (int)>, SpinLockPolicies> >();
}

TEST_CASE("EventQueue, SingleThreading, wait and waitFor compile and don't block")
{
	// SingleThreading::ConditionVariable is a dummy, wait returns at once and waitFor returns true.
	struct SinglePolicies
	{
		using Threading = eventpp::SingleThreading;
	};
	eventpp::EventQueue<int, void (int), SinglePolicies> queue;

	int data = 0;
	queue.appendListener(3, [&data](const int e) {
		data += e;
	});

	queue.wait();
	REQUIRE(queue.waitFor(std::chrono::milliseconds(1)));

	queue.enqueue(3, 5);
	queue.wait();
	REQUIRE(queue.process());
	REQUIRE(data == 5);
}

