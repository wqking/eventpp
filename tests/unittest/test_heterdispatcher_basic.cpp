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

#include <iostream>

struct xxx{};
static_assert(eventpp::internal_::CanConvert<std::tuple<int, int>, std::tuple<int ,int> >::value, "");
static_assert(eventpp::internal_::FindCallablePrototype<std::tuple<void (), void (int, int)> >::index == 0, "");
static_assert(eventpp::internal_::FindCallablePrototype<std::tuple<void (int), void (int, int)>, char>::index == 0, "");
static_assert(eventpp::internal_::FindCallablePrototype<std::tuple<void (int), void (int, int)>, char, int>::index == 1, "");
static_assert(eventpp::internal_::FindCallablePrototype<std::tuple<void (int), void (int, int), void (int, const xxx &)>, int, xxx>::index == 2, "");

TEST_CASE("HeterEventDispatcher, xxx")
{
	eventpp::HeterEventDispatcher<int, std::tuple<void (), void (int)> > a;
	auto handle = a.appendListener(3, std::function<void ()>([]() {
		std::cout << "empty" << std::endl;
	}));
	a.removeListener(3, handle);
	a.appendListener(3, std::function<void (int)>([](int a) {
		std::cout << "3 a = " << a << std::endl;
	}));
	a.appendListener(8, std::function<void (int)>([](int a) {
		std::cout << "8 a = " << a << std::endl;
	}));
	a.dispatch(3);
	a.dispatch(8);
	a.dispatch(8, 5);
}

