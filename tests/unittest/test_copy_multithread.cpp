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
#include "eventpp/hetercallbacklist.h"
#include "eventpp/hetereventdispatcher.h"

#include <thread>
#include <atomic>
#include <vector>

// These tests copy, assign and swap a list or a dispatcher while other threads are adding
// callbacks and invoking it. They verify that the operations don't dead lock and the final
// copy is complete.

namespace {

constexpr int threadCount = 4;
constexpr int callbackCountPerThread = 1024;
constexpr int copyCount = 256;

template <typename CL>
int countCallbacks(const CL & callbackList)
{
	int count = 0;
	callbackList.forEach([&count](const typename CL::Callback &) {
		++count;
	});
	return count;
}

} //unnamed namespace

TEST_CASE("CallbackList, multi threading, copy, assign and swap while appending and invoking")
{
	using CL = eventpp::CallbackList<void (int)>;
	CL callbackList;
	std::atomic<int> invokedCount(0);

	std::vector<std::thread> threadList;
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([&callbackList, &invokedCount]() {
			for(int k = 0; k < callbackCountPerThread; ++k) {
				callbackList.append([&invokedCount](int) {
					++invokedCount;
				});
				if((k & 63) == 0) {
					callbackList(0);
				}
			}
		});
	}
	threadList.emplace_back([&callbackList]() {
		CL assigned;
		for(int k = 0; k < copyCount; ++k) {
			CL copied(callbackList);
			copied(0);
			assigned = callbackList;
			assigned(0);
			CL swapped;
			swapped.swap(copied);
			CL moved(std::move(swapped));
			moved = std::move(assigned);
			moved(0);
		}
	});
	for(auto & thread : threadList) {
		thread.join();
	}

	REQUIRE(countCallbacks(callbackList) == threadCount * callbackCountPerThread);

	CL finalCopy(callbackList);
	REQUIRE(countCallbacks(finalCopy) == threadCount * callbackCountPerThread);
	REQUIRE(invokedCount.load() > 0);
}

TEST_CASE("EventDispatcher, multi threading, copy, assign and swap while adding listeners and dispatching")
{
	using ED = eventpp::EventDispatcher<int, void (int)>;
	ED dispatcher;
	std::atomic<int> invokedCount(0);

	std::vector<std::thread> threadList;
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([i, &dispatcher, &invokedCount]() {
			for(int k = 0; k < callbackCountPerThread; ++k) {
				dispatcher.appendListener(k % 16, [&invokedCount](int) {
					++invokedCount;
				});
				if((k & 63) == 0) {
					dispatcher.dispatch(i % 16, 0);
				}
			}
		});
	}
	threadList.emplace_back([&dispatcher]() {
		ED assigned;
		for(int k = 0; k < copyCount; ++k) {
			ED copied(dispatcher);
			copied.dispatch(k % 16, 0);
			assigned = dispatcher;
			assigned.dispatch(k % 16, 0);
			ED swapped;
			swapped.swap(copied);
			ED moved(std::move(swapped));
			moved = std::move(assigned);
			moved.dispatch(k % 16, 0);
		}
	});
	for(auto & thread : threadList) {
		thread.join();
	}

	ED finalCopy(dispatcher);
	int count = 0;
	for(int event = 0; event < 16; ++event) {
		finalCopy.forEach(event, [&count](const ED::Callback &) {
			++count;
		});
	}
	REQUIRE(count == threadCount * callbackCountPerThread);
	REQUIRE(invokedCount.load() > 0);
}

TEST_CASE("HeterCallbackList, multi threading, copy, assign and swap while appending and invoking")
{
	using CL = eventpp::HeterCallbackList<eventpp::HeterTuple<void (int), void (int, int)> >;
	CL callbackList;
	std::atomic<int> invokedCount(0);

	std::vector<std::thread> threadList;
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([&callbackList, &invokedCount]() {
			for(int k = 0; k < callbackCountPerThread; ++k) {
				if((k & 1) == 0) {
					callbackList.append([&invokedCount](int) {
						++invokedCount;
					});
				}
				else {
					callbackList.append([&invokedCount](int, int) {
						++invokedCount;
					});
				}
				if((k & 63) == 0) {
					callbackList(0);
					callbackList(0, 0);
				}
			}
		});
	}
	threadList.emplace_back([&callbackList]() {
		CL assigned;
		for(int k = 0; k < copyCount; ++k) {
			CL copied(callbackList);
			copied(0);
			assigned = callbackList;
			assigned(0, 0);
			CL swapped;
			swapped.swap(copied);
			CL moved(std::move(swapped));
			moved = std::move(assigned);
			moved(0);
		}
	});
	for(auto & thread : threadList) {
		thread.join();
	}

	CL finalCopy(callbackList);
	int count = 0;
	finalCopy.forEach<void (int)>([&count](const std::function<void (int)> &) {
		++count;
	});
	finalCopy.forEach<void (int, int)>([&count](const std::function<void (int, int)> &) {
		++count;
	});
	REQUIRE(count == threadCount * callbackCountPerThread);
	REQUIRE(invokedCount.load() > 0);
}

TEST_CASE("HeterEventDispatcher, multi threading, copy, assign and swap while adding listeners and dispatching")
{
	using ED = eventpp::HeterEventDispatcher<int, eventpp::HeterTuple<void (int)> >;
	ED dispatcher;
	std::atomic<int> invokedCount(0);

	std::vector<std::thread> threadList;
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([i, &dispatcher, &invokedCount]() {
			for(int k = 0; k < callbackCountPerThread; ++k) {
				dispatcher.appendListener(k % 16, [&invokedCount](int) {
					++invokedCount;
				});
				if((k & 63) == 0) {
					dispatcher.dispatch(i % 16, 0);
				}
			}
		});
	}
	threadList.emplace_back([&dispatcher]() {
		ED assigned;
		for(int k = 0; k < copyCount; ++k) {
			ED copied(dispatcher);
			copied.dispatch(k % 16, 0);
			assigned = dispatcher;
			assigned.dispatch(k % 16, 0);
			ED swapped;
			swapped.swap(copied);
			ED moved(std::move(swapped));
			moved = std::move(assigned);
			moved.dispatch(k % 16, 0);
		}
	});
	for(auto & thread : threadList) {
		thread.join();
	}

	ED finalCopy(dispatcher);
	int count = 0;
	for(int event = 0; event < 16; ++event) {
		finalCopy.forEach<void (int)>(event, [&count](const std::function<void (int)> &) {
			++count;
		});
	}
	REQUIRE(count == threadCount * callbackCountPerThread);
	REQUIRE(invokedCount.load() > 0);
}

