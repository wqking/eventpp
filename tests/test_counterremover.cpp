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
#include "eventpp/utilities/counterremover.h"
#include "eventpp/eventdispatcher.h"

TEST_CASE("CounterRemover, EventDispatcher")
{
	eventpp::EventDispatcher<int, void ()> dispatcher;
	constexpr int event = 3;
	
	std::vector<int> dataList(4);
	
	dispatcher.appendListener(event, [&dataList]() {
		++dataList[0];
	});
	
	eventpp::counterRemover(dispatcher).prependListener(event, [&dataList]() {
		++dataList[1];
	});
	auto handle = eventpp::counterRemover(dispatcher).appendListener(event, [&dataList]() {
		++dataList[2];
	}, 2);
	eventpp::counterRemover(dispatcher).insertListener(event, [&dataList]() {
		++dataList[3];
	}, handle, 3);
	
	REQUIRE(dataList == std::vector<int> { 0, 0, 0, 0 });

	dispatcher.dispatch(event);
	REQUIRE(dataList == std::vector<int> { 1, 1, 1, 1 });

	dispatcher.dispatch(event);
	REQUIRE(dataList == std::vector<int> { 2, 1, 2, 2 });

	dispatcher.dispatch(event);
	REQUIRE(dataList == std::vector<int> { 3, 1, 2, 3 });

	dispatcher.dispatch(event);
	REQUIRE(dataList == std::vector<int> { 4, 1, 2, 3 });
}

TEST_CASE("CounterRemover, CallbackList")
{
	eventpp::CallbackList<void ()> callbackList;
	
	std::vector<int> dataList(4);
	
	callbackList.append([&dataList]() {
		++dataList[0];
	});
	
	eventpp::counterRemover(callbackList).prepend([&dataList]() {
		++dataList[1];
	});
	auto handle = eventpp::counterRemover(callbackList).append([&dataList]() {
		++dataList[2];
	}, 2);
	eventpp::counterRemover(callbackList).insert([&dataList]() {
		++dataList[3];
	}, handle, 3);
	
	REQUIRE(dataList == std::vector<int> { 0, 0, 0, 0 });

	callbackList();
	REQUIRE(dataList == std::vector<int> { 1, 1, 1, 1 });

	callbackList();
	REQUIRE(dataList == std::vector<int> { 2, 1, 2, 2 });

	callbackList();
	REQUIRE(dataList == std::vector<int> { 3, 1, 2, 3 });

	callbackList();
	REQUIRE(dataList == std::vector<int> { 4, 1, 2, 3 });
}

