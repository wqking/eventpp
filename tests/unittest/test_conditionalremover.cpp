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
#include "eventpp/utilities/conditionalremover.h"
#include "eventpp/eventdispatcher.h"
#include "eventpp/eventqueue.h"

TEST_CASE("ConditionalRemover, EventDispatcher")
{
	eventpp::EventQueue<int, void ()> dispatcher;
	constexpr int event = 3;
	
	std::vector<int> dataList(4);
	
	dispatcher.appendListener(event, [&dataList]() {
		++dataList[0];
	});

	int removeCounter = 0;
	eventpp::conditionalRemover(dispatcher).prependListener(event, [&dataList]() {
		++dataList[1];
	}, [&removeCounter]() -> bool {
		return removeCounter == 1;
	});
	auto handle = eventpp::conditionalRemover(dispatcher).appendListener(event, [&dataList]() {
		++dataList[2];
	}, [&removeCounter]() -> bool {
		return removeCounter == 2;
	});
	eventpp::conditionalRemover(dispatcher).insertListener(event, [&dataList]() {
		++dataList[3];
	}, handle, [&removeCounter]() -> bool {
		return removeCounter == 3;
	});
	
	REQUIRE(dataList == std::vector<int> { 0, 0, 0, 0 });

	dispatcher.dispatch(event);
	REQUIRE(dataList == std::vector<int> { 1, 1, 1, 1 });

	++removeCounter;
	dispatcher.dispatch(event);
	REQUIRE(dataList == std::vector<int> { 2, 2, 2, 2 });

	++removeCounter;
	dispatcher.dispatch(event);
	REQUIRE(dataList == std::vector<int> { 3, 2, 3, 3 });

	++removeCounter;
	dispatcher.dispatch(event);
	REQUIRE(dataList == std::vector<int> { 4, 2, 3, 4 });

	++removeCounter;
	dispatcher.dispatch(event);
	REQUIRE(dataList == std::vector<int> { 5, 2, 3, 4 });
}

TEST_CASE("ConditionalRemover, CallbackList")
{
	eventpp::CallbackList<void ()> callbackList;
	
	std::vector<int> dataList(4);
	
	callbackList.append([&dataList]() {
		++dataList[0];
	});

	int removeCounter = 0;
	eventpp::conditionalRemover(callbackList).prepend([&dataList]() {
		++dataList[1];
	}, [&removeCounter]() -> bool {
		return removeCounter == 1;
	});
	auto handle = eventpp::conditionalRemover(callbackList).append([&dataList]() {
		++dataList[2];
	}, [&removeCounter]() -> bool {
		return removeCounter == 2;
	});
	eventpp::conditionalRemover(callbackList).insert([&dataList]() {
		++dataList[3];
	}, handle, [&removeCounter]() -> bool {
		return removeCounter == 3;
	});
	
	REQUIRE(dataList == std::vector<int> { 0, 0, 0, 0 });

	callbackList();
	REQUIRE(dataList == std::vector<int> { 1, 1, 1, 1 });

	++removeCounter;
	callbackList();
	REQUIRE(dataList == std::vector<int> { 2, 2, 2, 2 });

	++removeCounter;
	callbackList();
	REQUIRE(dataList == std::vector<int> { 3, 2, 3, 3 });

	++removeCounter;
	callbackList();
	REQUIRE(dataList == std::vector<int> { 4, 2, 3, 4 });

	++removeCounter;
	callbackList();
	REQUIRE(dataList == std::vector<int> { 5, 2, 3, 4 });
}

