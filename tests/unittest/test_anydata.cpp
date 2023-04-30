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
#include "eventpp/eventqueue.h"
#include "eventpp/utilities/anydata.h"

#include <vector>
#include <map>
#include <string>
#include <any>

namespace {

TEST_CASE("AnyData, maxSizeOf")
{
	REQUIRE(eventpp::maxSizeOf<
		std::uint8_t,
		std::uint16_t,
		std::uint32_t,
		std::uint64_t
		>() == sizeof(std::uint64_t)
	);
	REQUIRE(eventpp::maxSizeOf<
		std::uint64_t,
		std::uint32_t,
		std::uint8_t,
		std::uint16_t
		>() == sizeof(std::uint64_t)
	);
}

TEST_CASE("AnyData, default")
{
	using Data = eventpp::AnyData<64>;
	eventpp::EventQueue<int, void (const Data &)> queue;
	queue.appendListener(3, [](const Data & value) {
		REQUIRE(value.isType<int>());
		REQUIRE((long)value == 5);
		REQUIRE(value.get<long>() == 5);
	});
	queue.enqueue(3, 5);
	queue.process();
}

TEST_CASE("AnyData, unique_ptr")
{
	using Ptr = std::unique_ptr<int>;
	using Data = eventpp::AnyData<sizeof(Ptr)>;
	Data data(Ptr(new int(5)));
	REQUIRE(data.isType<Ptr>());
	REQUIRE(*data.get<Ptr>() == 5);
	Data data2(data);
	REQUIRE(data2.isType<Ptr>());
	REQUIRE(*data2.get<Ptr>() == 5);
}

TEST_CASE("AnyData, shared_ptr")
{
	using Ptr = std::shared_ptr<int>;
	using Data = eventpp::AnyData<sizeof(Ptr)>;
	Ptr ptr(std::make_shared<int>(8));
	REQUIRE(ptr.use_count() == 1);
	Data data(ptr);
	REQUIRE(ptr.use_count() == 2);
	REQUIRE(data.isType<Ptr>());
	REQUIRE(*data.get<Ptr>() == 8);
	Data data2(data);
	REQUIRE(ptr.use_count() == 3);
	REQUIRE(data2.isType<Ptr>());
	REQUIRE(*data2.get<Ptr>() == 8);
	REQUIRE(*data.get<Ptr>() == 8);
	REQUIRE(*ptr == 8);

	*ptr = 5;
	REQUIRE(*data2.get<Ptr>() == 5);
	REQUIRE(*data.get<Ptr>() == 5);
	REQUIRE(*ptr == 5);
}

enum class EventType {
	mouse = 1,
	key = 2
};

struct Event {
	EventType type;

	explicit Event(const EventType type) : type(type) {
	}
};

struct EventKey : Event {
	int key;

	explicit EventKey(const int key) : Event(EventType::key), key(key) {
	}
};

struct EventMouse : Event {
	int x;
	int y;

	EventMouse(const int x, const int y) : Event(EventType::mouse), x(x), y(y) {
	}
};
constexpr std::size_t eventMaxSize = eventpp::maxSizeOf<
	Event, EventKey, EventMouse, std::string
>();

TEST_CASE("AnyData, data")
{
	using Data = eventpp::AnyData<eventMaxSize>;
	eventpp::EventQueue<EventType, void (const Data &)> queue;
	queue.appendListener(EventType::key, [](const Data & value) {
		REQUIRE(value.isType<EventKey>());
		REQUIRE(value.get<EventKey>().type == EventType::key);
		REQUIRE(value.get<EventKey>().key == 5);
	});
	queue.enqueue(EventType::key, EventKey(5));
	queue.process();
}

TEST_CASE("AnyData, Policies")
{
	using Data = eventpp::AnyData<eventMaxSize>;
	struct Policies {
		using Callback = std::function<void (const Event &)>;
	};
	eventpp::EventQueue<EventType, void (const Data &), Policies> queue;
	int expectedKey;
	int expectedX;
	int expectedY;
	queue.appendListener(EventType::key, [&expectedKey](const Event & event) {
		REQUIRE(event.type == EventType::key);
		REQUIRE(static_cast<const EventKey &>(event).key == expectedKey);
	});
	queue.appendListener(EventType::mouse, [&expectedX, &expectedY](const Event & event) {
		REQUIRE(event.type == EventType::mouse);
		REQUIRE(static_cast<const EventMouse &>(event).x == expectedX);
		REQUIRE(static_cast<const EventMouse &>(event).y == expectedY);
	});
	expectedKey = 5;
	queue.enqueue(EventType::key, EventKey(5));
	queue.process();
	expectedKey = 8;
	expectedX = 12345678;
	expectedY = 9876532;
	queue.enqueue(EventType::mouse, EventMouse(12345678, 9876532));
	queue.enqueue(EventType::key, EventKey(8));
	queue.process();
}

} // unnamed namespace
