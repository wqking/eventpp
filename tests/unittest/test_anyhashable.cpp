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
#include "eventpp/utilities/anyhashable.h"
#include "eventpp/eventqueue.h"
#include "eventpp/eventdispatcher.h"

#include <vector>
#include <map>
#include <string>

TEST_CASE("AnyHashable, unordered_map")
{
	eventpp::EventQueue<eventpp::AnyHashable, void()> eventQueue;

	std::vector<int> dataList(3);

	eventQueue.appendListener(3, [&dataList]() {
		++dataList[0];
	});
	eventQueue.appendListener(std::string("hello"), [&dataList]() {
		++dataList[1];
	});
	eventQueue.appendListener(std::vector<bool>{ true, false, true }, [&dataList]() {
		++dataList[2];
	});

	REQUIRE(dataList == std::vector<int>{ 0, 0, 0});
	
	eventQueue.dispatch(std::string("hello"));
	REQUIRE(dataList == std::vector<int>{ 0, 1, 0});
	eventQueue.dispatch(3);
	REQUIRE(dataList == std::vector<int>{ 1, 1, 0});
	eventQueue.enqueue(std::vector<bool>{ true, false, true });
	eventQueue.process();
	REQUIRE(dataList == std::vector<int>{ 1, 1, 1});

	eventQueue.dispatch(std::string("hellox"));
	REQUIRE(dataList == std::vector<int>{ 1, 1, 1});
	eventQueue.dispatch(2);
	REQUIRE(dataList == std::vector<int>{ 1, 1, 1});
	eventQueue.dispatch(std::vector<bool>{ true, false, false });
	REQUIRE(dataList == std::vector<int>{ 1, 1, 1});
}

struct MyEventPolicies
{
	template <typename Key, typename T>
	using Map = std::map <Key, T>;
};
TEST_CASE("AnyHashable, map")
{
	using ED = eventpp::EventDispatcher<eventpp::AnyHashable, void(), MyEventPolicies>;
	ED dispatcher;

	std::vector<int> dataList(3);

	dispatcher.appendListener(3, [&dataList]() {
		++dataList[0];
		});
	dispatcher.appendListener(std::string("hello"), [&dataList]() {
		++dataList[1];
		});
	dispatcher.appendListener(std::vector<bool>{ true, false, true }, [&dataList]() {
		++dataList[2];
		});

	REQUIRE(dataList == std::vector<int>{ 0, 0, 0});

	dispatcher.dispatch(std::string("hello"));
	REQUIRE(dataList == std::vector<int>{ 0, 1, 0});
	dispatcher.dispatch(3);
	REQUIRE(dataList == std::vector<int>{ 1, 1, 0});
	dispatcher.dispatch(std::vector<bool>{ true, false, true });
	REQUIRE(dataList == std::vector<int>{ 1, 1, 1});
}

