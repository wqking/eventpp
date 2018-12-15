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
#include "eventpp/hetereventqueue.h"

TEST_CASE("xxx HeterEventQueue 1")
{
	eventpp::HeterEventQueue<std::string, std::tuple<void (), void (const std::string &)> > queue;

	int a = 1;
	int b = 5;

	queue.appendListener("event1", [&a](const std::string &) {
		a = 2;
	});
	queue.appendListener("event1", [&b]() {
		b = 8;
	});

	REQUIRE(a != 2);
	REQUIRE(b != 8);

	queue.enqueue("event1");
	queue.process();
	REQUIRE(a == 1);
	REQUIRE(b == 8);

	queue.enqueue("event1", "a");
	queue.process();
	REQUIRE(a == 2);
	REQUIRE(b == 8);
}

