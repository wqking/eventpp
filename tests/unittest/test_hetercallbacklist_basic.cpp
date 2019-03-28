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

#include <vector>

TEST_CASE("HeterCallbackList, empty")
{
	using CL = eventpp::HeterCallbackList<eventpp::HeterTuple<void (), void (int)> >;
	SECTION("empty") {
		CL callbackList;
		REQUIRE(callbackList.empty());
	}

	SECTION("append") {
		CL callbackList;
		callbackList.append([](){});
		REQUIRE(! callbackList.empty());
	}

	SECTION("prepend") {
		CL callbackList;
		callbackList.prepend([](int){});
		REQUIRE(! callbackList.empty());
	}

	SECTION("remove") {
		CL callbackList;
		auto handle = callbackList.append([](){});
		REQUIRE(! callbackList.empty());

		callbackList.remove(handle);
		REQUIRE(callbackList.empty());
	}
}

TEST_CASE("HeterCallbackList, forEach")
{
	using CL = eventpp::HeterCallbackList<eventpp::HeterTuple<int (), int(int)> >;
	CL callbackList;

	callbackList.append([]() { return 1; });
	callbackList.append([]() { return 2; });
	callbackList.append([]() { return 3; });

	callbackList.append([](int n) { return n + 5 + 0; });
	callbackList.append([](int n) { return n + 5 + 1; });
	callbackList.append([](int n) { return n + 5 + 2; });

	int i = 1;
	callbackList.forEach<int ()>([&i](const std::function<int ()> & callback) {
		REQUIRE(callback() == i);
		++i;
	});

	i = 0;
	callbackList.forEach<int (int)>([&i](const std::function<int (int)> & callback) {
		REQUIRE(callback(3) == 8 + i);
		++i;
	});
}

TEST_CASE("HeterCallbackList, forEachIf")
{
	using CL = eventpp::HeterCallbackList<eventpp::HeterTuple<void (), void(int)> >;
	CL callbackList;

	std::vector<int> dataListNoArg(3);
	std::vector<int> dataListWithArg(3);

	callbackList.append([&dataListNoArg]() {
		dataListNoArg[0] += 1;
	});
	callbackList.append([&dataListNoArg]() {
		dataListNoArg[1] += 2;
	});
	callbackList.append([&dataListNoArg]() {
		dataListNoArg[2] += 3;
	});

	callbackList.append([&dataListWithArg](int n) {
		dataListWithArg[0] += n;
	});
	callbackList.append([&dataListWithArg](int n) {
		dataListWithArg[1] += n * 2;
	});
	callbackList.append([&dataListWithArg](int n) {
		dataListWithArg[2] += n * 3;
	});

	REQUIRE(dataListNoArg == std::vector<int>{ 0, 0, 0 });
	REQUIRE(dataListWithArg == std::vector<int>{ 0, 0, 0 });

	int i = 0;
	bool result = callbackList.forEachIf<void ()>([&i](const std::function<void ()> & callback) -> bool {
		callback();
		++i;

		return i != 2;
	});

	REQUIRE(! result);
	REQUIRE(dataListNoArg == std::vector<int>{ 1, 2, 0 });
	REQUIRE(dataListWithArg == std::vector<int>{ 0, 0, 0 });

	i = 0;
	result = callbackList.forEachIf<void (int)>([&i](const std::function<void (int)> & callback) -> bool {
		callback(3);
		++i;

		return i != 1;
	});

	REQUIRE(! result);
	REQUIRE(dataListNoArg == std::vector<int>{ 1, 2, 0 });
	REQUIRE(dataListWithArg == std::vector<int>{ 3, 0, 0 });
}

TEST_CASE("HeterCallbackList, invoke")
{
	using CL = eventpp::HeterCallbackList<eventpp::HeterTuple<void (), void(int)> >;
	CL callbackList;

	std::vector<int> dataListNoArg(3);
	std::vector<int> dataListWithArg(3);

	callbackList.append([&dataListNoArg]() {
		dataListNoArg[0] += 1;
	});
	callbackList.append([&dataListNoArg]() {
		dataListNoArg[1] += 2;
	});
	callbackList.append([&dataListNoArg]() {
		dataListNoArg[2] += 3;
	});

	callbackList.append([&dataListWithArg](int n) {
		dataListWithArg[0] += n;
	});
	callbackList.append([&dataListWithArg](int n) {
		dataListWithArg[1] += n * 2;
	});
	callbackList.append([&dataListWithArg](int n) {
		dataListWithArg[2] += n * 3;
	});

	REQUIRE(dataListNoArg == std::vector<int>{ 0, 0, 0 });
	REQUIRE(dataListWithArg == std::vector<int>{ 0, 0, 0 });

	callbackList();
	REQUIRE(dataListNoArg == std::vector<int>{ 1, 2, 3 });
	REQUIRE(dataListWithArg == std::vector<int>{ 0, 0, 0 });

	callbackList();
	REQUIRE(dataListNoArg == std::vector<int>{ 2, 4, 6 });
	REQUIRE(dataListWithArg == std::vector<int>{ 0, 0, 0 });

	callbackList(1);
	REQUIRE(dataListNoArg == std::vector<int>{ 2, 4, 6 });
	REQUIRE(dataListWithArg == std::vector<int>{ 1, 2, 3 });

	callbackList(2);
	REQUIRE(dataListNoArg == std::vector<int>{ 2, 4, 6 });
	REQUIRE(dataListWithArg == std::vector<int>{ 3, 6, 9 });
}
