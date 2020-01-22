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
#include "eventpp/eventdispatcher.h"
#undef private

TEST_CASE("EventDispatcher, copy constructor from empty EventDispatcher")
{
	using ED = eventpp::EventDispatcher<int, void ()>;
	ED dispatcher;

	REQUIRE(dispatcher.eventCallbackListMap.empty());

	ED copiedDispatcher(dispatcher);
	REQUIRE(copiedDispatcher.eventCallbackListMap.empty());
	REQUIRE(dispatcher.eventCallbackListMap.empty());
}

TEST_CASE("EventDispatcher, copy constructor from non-empty EventDispatcher")
{
	using ED = eventpp::EventDispatcher<int, void ()>;
	ED dispatcher;
	constexpr int event = 3;

	std::vector<int> dataList(3);

	dispatcher.appendListener(event, [&dataList]() {
		++dataList[0];
	});
	dispatcher.appendListener(event, [&dataList]() {
		++dataList[1];
	});

	REQUIRE(dataList == std::vector<int> { 0, 0, 0 });
	dispatcher.dispatch(event);
	REQUIRE(dataList == std::vector<int> { 1, 1, 0 });
	
	ED copiedDispatcher(dispatcher);
	copiedDispatcher.dispatch(event);
	REQUIRE(dataList == std::vector<int> { 2, 2, 0 });
	copiedDispatcher.appendListener(event, [&dataList]() {
		++dataList[2];
	});
	copiedDispatcher.dispatch(event);
	REQUIRE(dataList == std::vector<int> { 3, 3, 1 });
	dispatcher.dispatch(event);
	REQUIRE(dataList == std::vector<int> { 4, 4, 1 });
}

