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

struct xxx{};
static_assert(eventpp::internal_::FindPrototypeByCallable<std::tuple<void (), void (int, int)>, void() >::index == 0, "");
static_assert(eventpp::internal_::FindPrototypeByCallable<std::tuple<void (int), void (int, int)>, void(char)>::index == 0, "");
static_assert(eventpp::internal_::FindPrototypeByCallable<std::tuple<void (int), void (int, int)>, void(char, int)>::index == 1, "");
static_assert(eventpp::internal_::FindPrototypeByCallable<std::tuple<void (int), void (int, int), void (int, const xxx &)>, void(int, xxx)>::index == 2, "");
static_assert(eventpp::internal_::FindPrototypeByCallable<std::tuple<void (), void (const std::string &)>, void(const std::string &) >::index == 1, "");

template <bool condition>
struct FindTypeByIndexHelper;

template <>
struct FindTypeByIndexHelper <true>
{
	template <int N, int M, typename C, typename ...Args>
	static constexpr void find(const int index, const C & c, Args && ...args)
	{
		if(index == N) {
			c.template operator()<N>(std::forward<Args>(args)...);
		}

		FindTypeByIndexHelper<(N + 1 < M)>::template find<N + 1, M>(index, c, std::forward<Args>(args)...);
	}

};

template <>
struct FindTypeByIndexHelper <false>
{
	template <int N, int M, typename C, typename ...Args>
	static constexpr void find(const int index, const C & c, Args && ...args)
	{
	}

};

template <int N, int M, typename C, typename ...Args>
constexpr void findTypeByIndex(const int index, const C & c, Args && ...args)
{
	FindTypeByIndexHelper<(N < M)>::template find<N, M>(index, c, std::forward<Args>(args)...);
}

struct Back
{
	template <int N, typename T>
	void operator()(T *, int * p) const {
		*p = (int)sizeof(typename std::tuple_element<N, T>::type);
	}
};

using FreeFunc = void (*)(void *);
template <typename T>
void freeIt(void * p)
{
	*(int *)p = sizeof(T);
}

template <typename ...Args>
struct YYY
{
	static const FreeFunc * get() {
		static std::array<FreeFunc, sizeof...(Args)> data {
			&freeIt<Args>...
		};

		return data.data();
	}
};

TEST_CASE("HeterEventDispatcher, 1")
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

