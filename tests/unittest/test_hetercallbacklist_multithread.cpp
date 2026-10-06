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
#include "eventpp/hetercallbacklist.h"
#include "eventpp/hetereventdispatcher.h"

#include <thread>
#include <atomic>
#include <vector>

// The callback list of each prototype used to be created on the first use. Several threads using a
// fresh HeterCallbackList at the same time raced on the creation. Now the lists are created in the
// constructor. These tests use fresh objects from several threads at once, repeatedly. The race is
// only observable with a race detector such as ThreadSanitizer, without it the tests pass either way.

TEST_CASE("HeterCallbackList, multi threading, first use from several threads")
{
	using CL = eventpp::HeterCallbackList<eventpp::HeterTuple<void (int), void (int, int)> >;

	constexpr int repeatCount = 200;
	constexpr int threadCount = 4;

	for(int repeat = 0; repeat < repeatCount; ++repeat) {
		CL callbackList;
		std::atomic<int> invokedCount(0);

		std::vector<std::thread> threadList;
		for(int i = 0; i < threadCount; ++i) {
			threadList.emplace_back([i, &callbackList, &invokedCount]() {
				if((i & 1) == 0) {
					callbackList.append([&invokedCount](int) {
						++invokedCount;
					});
					callbackList(0);
				}
				else {
					callbackList.append([&invokedCount](int, int) {
						++invokedCount;
					});
					callbackList(0, 0);
				}
			});
		}
		for(auto & thread : threadList) {
			thread.join();
		}

		int count = 0;
		callbackList.forEach<void (int)>([&count](const std::function<void (int)> &) {
			++count;
		});
		callbackList.forEach<void (int, int)>([&count](const std::function<void (int, int)> &) {
			++count;
		});
		REQUIRE(count == threadCount);
		REQUIRE(invokedCount.load() >= threadCount);
	}
}

TEST_CASE("HeterEventDispatcher, multi threading, first use from several threads")
{
	using ED = eventpp::HeterEventDispatcher<int, eventpp::HeterTuple<void (int), void (int, int)> >;

	constexpr int repeatCount = 200;
	constexpr int threadCount = 4;

	for(int repeat = 0; repeat < repeatCount; ++repeat) {
		ED dispatcher;
		std::atomic<int> invokedCount(0);

		std::vector<std::thread> threadList;
		for(int i = 0; i < threadCount; ++i) {
			threadList.emplace_back([i, &dispatcher, &invokedCount]() {
				if((i & 1) == 0) {
					dispatcher.appendListener(3, [&invokedCount](int) {
						++invokedCount;
					});
					dispatcher.dispatch(3, 0);
				}
				else {
					dispatcher.appendListener(3, [&invokedCount](int, int) {
						++invokedCount;
					});
					dispatcher.dispatch(3, 0, 0);
				}
			});
		}
		for(auto & thread : threadList) {
			thread.join();
		}

		int count = 0;
		dispatcher.forEach<void (int)>(3, [&count](const std::function<void (int)> &) {
			++count;
		});
		dispatcher.forEach<void (int, int)>(3, [&count](const std::function<void (int, int)> &) {
			++count;
		});
		REQUIRE(count == threadCount);
		REQUIRE(invokedCount.load() >= threadCount);
	}
}

TEST_CASE("HeterCallbackList, moved-from object is still usable")
{
	using CL = eventpp::HeterCallbackList<eventpp::HeterTuple<void (int), void (int, int)> >;

	int count = 0;

	CL source;
	source.append([&count](int) {
		++count;
	});

	CL moved(std::move(source));
	REQUIRE(! moved.empty());
	REQUIRE(source.empty());

	// the moved-from list has new empty lists and can be used again
	source.append([&count](int, int) {
		count += 10;
	});
	source(0, 0);
	REQUIRE(count == 10);
	moved(0);
	REQUIRE(count == 11);

	CL assigned;
	assigned = std::move(moved);
	REQUIRE(moved.empty());
	moved.append([&count](int) {
		count += 100;
	});
	moved(0);
	REQUIRE(count == 111);
	assigned(0);
	REQUIRE(count == 112);
}

