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

#define private public
#include "eventpp/callbacklist.h"
#undef private

#include <vector>
#include <thread>
#include <chrono>
#include <numeric>
#include <random>
#include <algorithm>

using namespace eventpp;

namespace {

template <typename CL, typename T>
void verifyLinkedList(CL & callbackList, const std::vector<T> & dataList)
{
	const int count = (int)dataList.size();
	if(count == 0) {
		REQUIRE(! callbackList.head);
		REQUIRE(! callbackList.tail);
		return;
	}

	REQUIRE(! callbackList.head->previous);
	REQUIRE(! callbackList.tail->next);

	if(count == 1) {
		REQUIRE(callbackList.head);
		REQUIRE(callbackList.head == callbackList.tail);
	}

	auto node = callbackList.head;
	for(int i = 0; i < count; ++i) {
		REQUIRE(node);
		
		if(i == 0) {
			REQUIRE(! node->previous);
			REQUIRE(node == callbackList.head);
		}
		if(i == count - 1) {
			REQUIRE(! node->next);
			REQUIRE(node == callbackList.tail);
		}
		
		REQUIRE(node->callback == dataList[i]);

		node = node->next;
	}
}

template <typename CL, typename T>
void verifyDisorderedLinkedList(CL & callbackList, std::vector<T> dataList)
{
	std::vector<T> buffer;

	auto node = callbackList.head;
	while(node) {
		buffer.push_back(node->callback);
		node = node->next;
	}

	std::sort(buffer.begin(), buffer.end());
	std::sort(dataList.begin(), dataList.end());

	REQUIRE(buffer == dataList);
}

template <typename CL>
auto extractCallbackListHandles(CL & callbackList)
	-> std::vector<typename CL::Handle>
{
	std::vector<typename CL::Handle> result;

	auto node = callbackList.head;
	while(node) {
		result.push_back(typename CL::Handle(node));
		node = node->next;
	}

	return result;
}

template <typename T>
void verifyNoMemoryLeak(const std::vector<T> & nodeList)
{
	for(const auto & node : nodeList) {
		REQUIRE(! node.lock());
	}
}

} //unnamed namespace

using Prototype = void();
using CL = CallbackList<Prototype, int>;

TEST_CASE("CallbackList, no memory leak after callback list is freed")
{
	std::vector<CL::Handle> nodeList;

	{
		CL callbackList;
		for(int i = 100; i < 200; ++i) {
			callbackList.append(i);
		}

		nodeList = extractCallbackListHandles(callbackList);
	}

	verifyNoMemoryLeak(nodeList);
}

TEST_CASE("CallbackList, no memory leak after all callbacks are removed")
{
	std::vector<CL::Handle> nodeList;
	std::vector<CL::Handle> handleList;

	CL callbackList;
	for(int i = 100; i < 200; ++i) {
		handleList.push_back(callbackList.append(i));
	}

	nodeList = extractCallbackListHandles(callbackList);

	for(auto & handle : handleList) {
		callbackList.remove(handle);
	}

	verifyNoMemoryLeak(nodeList);
}

TEST_CASE("CallbackList, append/remove/insert")
{
	CL callbackList;

	REQUIRE(! callbackList.head);
	REQUIRE(! callbackList.tail);

	CL::Handle h100, h101, h102, h103, h104, h105, h106, h107;

	{
		auto handle = callbackList.append(100);
		h100 = handle;
		verifyLinkedList(callbackList, std::vector<int>{ 100 });
	}

	{
		auto handle = callbackList.append(101);
		h101 = handle;
		verifyLinkedList(callbackList, std::vector<int>{ 100, 101 });
	}

	{
		auto handle = callbackList.append(102);
		h102 = handle;
		verifyLinkedList(callbackList, std::vector<int>{ 100, 101, 102 });
	}

	{
		auto handle = callbackList.append(103);
		h103 = handle;
		verifyLinkedList(callbackList, std::vector<int>{ 100, 101, 102, 103 });
	}

	{
		auto handle = callbackList.append(104);
		h104 = handle;
		verifyLinkedList(callbackList, std::vector<int>{ 100, 101, 102, 103, 104 });
	}

	{
		auto handle = callbackList.insert(105, h103); // before 103
		h105 = handle;
		verifyLinkedList(callbackList, std::vector<int>{ 100, 101, 102, 105, 103, 104 });
		
		h107 = callbackList.insert(107, h100); // before 100
		verifyLinkedList(callbackList, std::vector<int>{ 107, 100, 101, 102, 105, 103, 104 });

		h106 = callbackList.insert(106, handle); // before 105
		verifyLinkedList(callbackList, std::vector<int>{ 107, 100, 101, 102, 106, 105, 103, 104 });
	}

	callbackList.remove(h100);
	verifyLinkedList(callbackList, std::vector<int>{ 107, 101, 102, 106, 105, 103, 104 });

	callbackList.remove(h103);
	callbackList.remove(h102);
	verifyLinkedList(callbackList, std::vector<int>{ 107, 101, 106, 105, 104 });

	callbackList.remove(h105);
	callbackList.remove(h104);
	callbackList.remove(h106);
	callbackList.remove(h101);
	callbackList.remove(h107);
	verifyLinkedList(callbackList, std::vector<int>{});
}

TEST_CASE("CallbackList, insert")
{
	CL callbackList;
	
	auto h100 = callbackList.append(100);
	auto h101 = callbackList.append(101);
	auto h102 = callbackList.append(102);
	auto h103 = callbackList.append(103);
	auto h104 = callbackList.append(104);

	SECTION("before front") {
		callbackList.insert(105, h100);
		verifyLinkedList(callbackList, std::vector<int>{ 105, 100, 101, 102, 103, 104 });
	}

	SECTION("before second") {
		callbackList.insert(105, h101);
		verifyLinkedList(callbackList, std::vector<int>{ 100, 105, 101, 102, 103, 104 });
	}

	SECTION("before nonexist by handle") {
		callbackList.insert(105, CL::Handle());
		verifyLinkedList(callbackList, std::vector<int>{ 100, 101, 102, 103, 104, 105 });
	}
}

TEST_CASE("CallbackList, remove")
{
	CL callbackList;

	auto h100 = callbackList.append(100);
	auto h101 = callbackList.append(101);
	auto h102 = callbackList.append(102);
	auto h103 = callbackList.append(103);
	auto h104 = callbackList.append(104);

	SECTION("remove front") {
		callbackList.remove(h100);
		verifyLinkedList(callbackList, std::vector<int>{ 101, 102, 103, 104 });

		callbackList.remove(h100);
		verifyLinkedList(callbackList, std::vector<int>{ 101, 102, 103, 104 });
	}

	SECTION("remove second") {
		callbackList.remove(h101);
		verifyLinkedList(callbackList, std::vector<int>{ 100, 102, 103, 104 });

		callbackList.remove(h101);
		verifyLinkedList(callbackList, std::vector<int>{ 100, 102, 103, 104 });
	}

	SECTION("remove end") {
		callbackList.remove(h104);
		verifyLinkedList(callbackList, std::vector<int>{ 100, 101, 102, 103 });

		callbackList.remove(h104);
		verifyLinkedList(callbackList, std::vector<int>{ 100, 101, 102, 103 });
	}

	SECTION("remove nonexist") {
		callbackList.remove(CL::Handle());
		verifyLinkedList(callbackList, std::vector<int>{ 100, 101, 102, 103, 104 });

		callbackList.remove(CL::Handle());
		verifyLinkedList(callbackList, std::vector<int>{ 100, 101, 102, 103, 104 });
	}

	SECTION("remove all") {
		callbackList.remove(h102);
		callbackList.remove(h104);
		callbackList.remove(h103);
		callbackList.remove(h101);
		callbackList.remove(h100);
		verifyLinkedList(callbackList, std::vector<int>{ });
	}
}


TEST_CASE("CallbackList, multi threading, append")
{
	CL callbackList;

	constexpr int threadCount = 256;
	constexpr int taskCountPerThread = 1024 * 4;
	constexpr int itemCount = threadCount * taskCountPerThread;

	std::vector<int> taskList(itemCount);
	std::iota(taskList.begin(), taskList.end(), 0);
	std::shuffle(taskList.begin(), taskList.end(), std::mt19937(std::random_device()()));

	std::vector<std::thread> threadList;
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([i, taskCountPerThread, &callbackList, &taskList]() {
			for(int k = i * taskCountPerThread; k < (i + 1) * taskCountPerThread; ++k) {
				callbackList.append(taskList[k]);
			}
		});
	}

	for(auto & thread : threadList) {
		thread.join();
	}

	taskList.clear();

	std::vector<int> compareList(itemCount);
	std::iota(compareList.begin(), compareList.end(), 0);

	verifyDisorderedLinkedList(callbackList, compareList);
}

TEST_CASE("CallbackList, multi threading, remove")
{
	CL callbackList;

	// total count can't be too large because the time complixity
	// of remove() is O(n) which is quite slow.
	constexpr int threadCount = 128;
	constexpr int taskCountPerThread = 128;
	constexpr int itemCount = threadCount * taskCountPerThread;

	std::vector<int> taskList(itemCount);
	std::iota(taskList.begin(), taskList.end(), 0);
	std::shuffle(taskList.begin(), taskList.end(), std::mt19937(std::random_device()()));

	std::vector<CL::Handle> handleList;

	for(const auto & item : taskList) {
		handleList.push_back(callbackList.append(item));
	}

	std::vector<std::thread> threadList;
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([i, taskCountPerThread, &callbackList, &handleList]() {
			for(int k = i * taskCountPerThread; k < (i + 1) * taskCountPerThread; ++k) {
				callbackList.remove(handleList[k]);
			}
		});
	}

	for(auto & thread : threadList) {
		thread.join();
	}

	taskList.clear();

	REQUIRE(! callbackList.head);
	REQUIRE(! callbackList.tail);
}

TEST_CASE("CallbackList, multi threading, double remove")
{
	CL callbackList;

	// total count can't be too large because the time complixity
	// of remove() is O(n) which is quite slow.
	constexpr int threadCount = 128;
	constexpr int taskCountPerThread = 128;
	constexpr int itemCount = threadCount * taskCountPerThread;

	std::vector<int> taskList(itemCount);
	std::iota(taskList.begin(), taskList.end(), 0);
	std::shuffle(taskList.begin(), taskList.end(), std::mt19937(std::random_device()()));

	std::vector<CL::Handle> handleList;

	for(const auto & item : taskList) {
		handleList.push_back(callbackList.append(item));
	}

	std::vector<std::thread> threadList;
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([i, taskCountPerThread, &callbackList, &handleList, threadCount]() {
			// make start and end overlap other threads to so double remove.
			int start = i;
			int end = i + 1;
			if(i > 0) {
				--start;
			}
			else if(i < threadCount - 1) {
				++end;
			}
			for(int k = start * taskCountPerThread; k < end * taskCountPerThread; ++k) {
				callbackList.remove(handleList[k]);
			}
		});
	}

	for(auto & thread : threadList) {
		thread.join();
	}

	taskList.clear();

	REQUIRE(! callbackList.head);
	REQUIRE(! callbackList.tail);
}

TEST_CASE("CallbackList, multi threading, remove by handle")
{
	CL callbackList;

	constexpr int threadCount = 256;
	constexpr int taskCountPerThread = 1024 * 4;
	constexpr int itemCount = threadCount * taskCountPerThread;

	std::vector<int> taskList(itemCount);
	std::iota(taskList.begin(), taskList.end(), 0);
	std::shuffle(taskList.begin(), taskList.end(), std::mt19937(std::random_device()()));

	std::vector<CL::Handle> handleList;

	for(const auto & item : taskList) {
		handleList.push_back(callbackList.append(item));
	}

	std::vector<std::thread> threadList;
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([i, taskCountPerThread, &callbackList, &handleList]() {
			for(int k = i * taskCountPerThread; k < (i + 1) * taskCountPerThread; ++k) {
				callbackList.remove(handleList[k]);
			}
		});
	}

	for(auto & thread : threadList) {
		thread.join();
	}

	taskList.clear();

	REQUIRE(! callbackList.head);
	REQUIRE(! callbackList.tail);
}

TEST_CASE("CallbackList, multi threading, double remove by handle")
{
	CL callbackList;

	constexpr int threadCount = 256;
	constexpr int taskCountPerThread = 1024 * 4;
	constexpr int itemCount = threadCount * taskCountPerThread;

	std::vector<int> taskList(itemCount);
	std::iota(taskList.begin(), taskList.end(), 0);
	std::shuffle(taskList.begin(), taskList.end(), std::mt19937(std::random_device()()));

	std::vector<CL::Handle> handleList;

	for(const auto & item : taskList) {
		handleList.push_back(callbackList.append(item));
	}

	std::vector<std::thread> threadList;
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([i, taskCountPerThread, &callbackList, &handleList, threadCount]() {
			int start = i;
			int end = i + 1;
			if(i > 0) {
				--start;
			}
			else if(i < threadCount - 1) {
				++end;
			}
			for(int k = start * taskCountPerThread; k < end * taskCountPerThread; ++k) {
				callbackList.remove(handleList[k]);
			}
		});
	}

	for(auto & thread : threadList) {
		thread.join();
	}

	taskList.clear();

	REQUIRE(! callbackList.head);
	REQUIRE(! callbackList.tail);
}

TEST_CASE("CallbackList, multi threading, append/double remove by handle")
{
	CL callbackList;

	constexpr int threadCount = 256;
	constexpr int taskCountPerThread = 1024 * 4;
	constexpr int itemCount = threadCount * taskCountPerThread;

	std::vector<int> taskList(itemCount);
	std::iota(taskList.begin(), taskList.end(), 0);
	std::shuffle(taskList.begin(), taskList.end(), std::mt19937(std::random_device()()));

	std::vector<CL::Handle> handleList(taskList.size());

	std::vector<std::thread> threadList;
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([i, taskCountPerThread, &callbackList, &handleList, threadCount, &taskList]() {
			for(int k = i * taskCountPerThread; k < (i + 1) * taskCountPerThread; ++k) {
				handleList[k] = callbackList.append(taskList[k]);
			}
			int start = i;
			int end = i + 1;
			if(i > 0) {
				--start;
			}
			else if(i < threadCount - 1) {
				++end;
			}
			for(int k = start * taskCountPerThread; k < end * taskCountPerThread; ++k) {
				callbackList.remove(handleList[k]);
			}
		});
	}

	for(auto & thread : threadList) {
		thread.join();
	}

	taskList.clear();

	REQUIRE(! callbackList.head);
	REQUIRE(! callbackList.tail);
}


TEST_CASE("CallbackList, multi threading, insert")
{
	CL callbackList;

	constexpr int threadCount = 256;
	constexpr int taskCountPerThread = 1024;
	constexpr int itemCount = threadCount * taskCountPerThread;

	std::vector<int> taskList(itemCount);
	std::iota(taskList.begin(), taskList.end(), 0);
	std::shuffle(taskList.begin(), taskList.end(), std::mt19937(std::random_device()()));

	std::vector<CL::Handle> handleList(taskList.size());

	std::vector<std::thread> threadList;
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([i, taskCountPerThread, &callbackList, &taskList, &handleList]() {
			int k = i * taskCountPerThread;
			for(; k < (i + 1) * taskCountPerThread / 2; ++k) {
				handleList[k] = callbackList.append(taskList[k]);
			}
			int offset = 0;
			for(; k < (i + 1) * taskCountPerThread / 2 + (i + 1) * taskCountPerThread / 4; ++k) {
				handleList[k] = callbackList.insert(taskList[k], handleList[offset++]);
			}
			for(; k < (i + 1) * taskCountPerThread; ++k) {
				handleList[k] = callbackList.insert(taskList[k], handleList[offset++]);
			}
		});
	}

	for(auto & thread : threadList) {
		thread.join();
	}

	taskList.clear();

	std::vector<int> compareList(itemCount);
	std::iota(compareList.begin(), compareList.end(), 0);

	verifyDisorderedLinkedList(callbackList, compareList);
}

