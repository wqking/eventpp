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
#include "eventpp/eventpolicies.h"
#include "eventpp/callbacklist.h"

#include <thread>
#include <mutex>
#include <vector>

TEST_CASE("SpinLock, lock, try_lock, unlock")
{
	eventpp::SpinLock spinLock;

	REQUIRE(spinLock.try_lock());
	REQUIRE(! spinLock.try_lock());
	spinLock.unlock();
	REQUIRE(spinLock.try_lock());
	spinLock.unlock();

	spinLock.lock();
	REQUIRE(! spinLock.try_lock());
	spinLock.unlock();

	{
		std::lock_guard<eventpp::SpinLock> lockGuard(spinLock);
		REQUIRE(! spinLock.try_lock());
	}
	REQUIRE(spinLock.try_lock());
	spinLock.unlock();
}

TEST_CASE("SpinLock, multi threading")
{
	eventpp::SpinLock spinLock;

	// more threads than cores to exercise the yield
	constexpr int threadCount = 64;
	constexpr int incrementCountPerThread = 1024 * 16;

	int counter = 0;

	std::vector<std::thread> threadList;
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([&spinLock, &counter, incrementCountPerThread]() {
			for(int k = 0; k < incrementCountPerThread; ++k) {
				std::lock_guard<eventpp::SpinLock> lockGuard(spinLock);
				++counter;
			}
		});
	}
	for(int i = 0; i < threadCount; ++i) {
		threadList[i].join();
	}

	REQUIRE(counter == threadCount * incrementCountPerThread);
}

TEST_CASE("CallbackList, SpinLock, multi threading")
{
	using CL = eventpp::CallbackList<void (int), eventpp::GeneralThreading<eventpp::SpinLock> >;
	CL callbackList;

	constexpr int threadCount = 16;
	constexpr int callbackCountPerThread = 1024;

	std::vector<std::thread> threadList;
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([&callbackList, callbackCountPerThread]() {
			for(int k = 0; k < callbackCountPerThread; ++k) {
				callbackList.append([](int) {});
			}
		});
	}
	for(int i = 0; i < threadCount; ++i) {
		threadList[i].join();
	}

	int count = 0;
	callbackList.forEach([&count](const CL::Callback &) {
		++count;
	});
	REQUIRE(count == threadCount * callbackCountPerThread);
}

