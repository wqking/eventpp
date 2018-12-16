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
#include "eventpp/hetereventdispatcher.h"
#include "eventpp/mixins/mixinfilter.h"
#include "eventpp/mixins/mixinheterfilter.h"

TEST_CASE("xxx HeterEventDispatcher, 1")
{
	eventpp::HeterEventDispatcher<int, std::tuple<void (), void (int, int, int)> > dispatcher;

	std::array<int, 2> dataList{};

	dispatcher.appendListener(3, [&dataList]() {
		++dataList[0];
	});
	dispatcher.appendListener(3, [&dataList](const FromInt & a, int b, int c) -> int {
		dataList[1] += a.value + b + c;
		return 0;
	});
	dispatcher.appendListener(8, [&dataList](int a, int b, int c) {
		dataList[1] += a + b + c;
	});
	
	REQUIRE(dataList[0] == 0);
	REQUIRE(dataList[1] == 0);

	dispatcher.dispatch(3);
	REQUIRE(dataList[0] == 1);
	REQUIRE(dataList[1] == 0);

	dispatcher.dispatch(8);
	REQUIRE(dataList[0] == 1);
	REQUIRE(dataList[1] == 0);

	dispatcher.dispatch(8, 5, 1, 3);
	REQUIRE(dataList[0] == 1);
	REQUIRE(dataList[1] == 9);

	dispatcher.dispatch(3, 2, 6, 7);
	REQUIRE(dataList[0] == 1);
	REQUIRE(dataList[1] == 24);
}

TEST_CASE("xxx HeterEventDispatcher, event filter")
{
	struct MyPolicies {
		using Mixins = eventpp::MixinList<eventpp::MixinHeterFilter>;
	};
	using ED = eventpp::HeterEventDispatcher<int, std::tuple<void (int, int), void ()>, MyPolicies>;
	ED dispatcher;

	constexpr int itemCount = 5;
	std::vector<int> dataList(itemCount);

	for(int i = 0; i < itemCount; ++i) {
		dispatcher.appendListener(i, [&dataList, i](int e, int index) {
			dataList[e] = index;
		});
	}

	constexpr int filterCount = 2;
	std::vector<int> filterData(filterCount);

	SECTION("Filter invoked count") {
		auto handle1 = dispatcher.appendFilter([&filterData](int, int) -> bool {
			++filterData[0];
			return true;
		});
		auto handle2 = dispatcher.appendFilter([&filterData]() -> bool {
			++filterData[1];
			return true;
		});

		for(int i = 0; i < itemCount; ++i) {
			dispatcher.dispatch(i, i, 58);
			dispatcher.dispatch(i);
		}

		REQUIRE(filterData == std::vector<int>{ itemCount, itemCount });
		REQUIRE(dataList == std::vector<int>{ 58, 58, 58, 58, 58 });

		dispatcher.removeFilter(handle1);

		for(int i = 0; i < itemCount; ++i) {
			dispatcher.dispatch(i, i, 38);
			dispatcher.dispatch(i);
		}

		REQUIRE(filterData == std::vector<int>{ itemCount, itemCount * 2 });

		dispatcher.removeFilter(handle2);
		dispatcher.removeFilter(handle2);

		for(int i = 0; i < itemCount; ++i) {
			dispatcher.dispatch(i, i, 38);
			dispatcher.dispatch(i);
		}

		REQUIRE(filterData == std::vector<int>{ itemCount, itemCount * 2 });
	}
#if 0
	SECTION("First filter blocks all other filters and listeners") {
		dispatcher.appendFilter([&filterData](int e, int /*index*/) -> bool {
			++filterData[0];
			if(e >= 2) {
				return false;
			}
			return true;
		});
		dispatcher.appendFilter([&filterData](int /*e*/, int /*index*/) -> bool {
			++filterData[1];
			return true;
		});

		for(int i = 0; i < itemCount; ++i) {
			dispatcher.dispatch(i, 58);
		}

		REQUIRE(filterData == std::vector<int>{ itemCount, 2 });
		REQUIRE(dataList == std::vector<int>{ 58, 58, 0, 0, 0 });
	}

	SECTION("Second filter doesn't block first filter but all listeners") {
		dispatcher.appendFilter([&filterData](int /*e*/, int /*index*/) -> bool {
			++filterData[0];
			return true;
		});
		dispatcher.appendFilter([&filterData](int e, int /*index*/) -> bool {
			++filterData[1];
			if(e >= 2) {
				return false;
			}
			return true;
		});

		for(int i = 0; i < itemCount; ++i) {
			dispatcher.dispatch(i, 58);
		}

		REQUIRE(filterData == std::vector<int>{ itemCount, itemCount });
		REQUIRE(dataList == std::vector<int>{ 58, 58, 0, 0, 0 });
	}

	SECTION("Filter manipulates the parameters") {
		dispatcher.appendFilter([&filterData](int e, int & index) -> bool {
			++filterData[0];
			if(e >= 2) {
				++index;
			}
			return true;
		});
		dispatcher.appendFilter([&filterData](int /*e*/, int /*index*/) -> bool {
			++filterData[1];
			return true;
		});

		for(int i = 0; i < itemCount; ++i) {
			dispatcher.dispatch(i, 58);
		}

		REQUIRE(filterData == std::vector<int>{ itemCount, itemCount });
		REQUIRE(dataList == std::vector<int>{ 58, 58, 59, 59, 59 });
	}
#endif
}

