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

// Include the head
#include "eventpp/utilities/argumentadapter.h"

#include "eventpp/eventdispatcher.h"
#include "tutorial.h"

#include <iostream>

// In the tutorials here, we will use two event class, MouseEvent derives from Event.
// The callback prototype in EventDispatcher is reference or pointer to Event,
// then we should only be able to add listeners that only accept reference or pointer to Event,
// not MouseEvent.
// But with argumentAdapter, the listeners can accept reference or pointer to MouseEvent,
// and argumentAdapter converts any reference or pointer to Event to MouseEvent automatically, as
// long as object pointed to the reference or pointer is a MouseEvent.

class Event
{
};

class MouseEvent : public Event
{
public:
	MouseEvent(const int x, const int y) : x(x), y(y) {
	}

	int getX() const { return x; }
	int getY() const { return y; }

private:
	int x;
	int y;
};

constexpr int mouseEventId = 3;

// A free function that will be added as listener later.
// argumentAdapter works on all types of callables, include but not limited to,
// lambda, functor object, std::function, free function, etc.
void tutorialArgumentAdapterFreeFunction(const MouseEvent & e)
{
	std::cout << "Received MouseEvent in free function, x=" << e.getX() << " y=" << e.getY() << std::endl;
}

TEST_CASE("ArgumentAdapter tutorial 1, basic")
{
	std::cout << std::endl << "ArgumentAdapter tutorial 1, basic" << std::endl;

	eventpp::EventDispatcher<int, void (const Event &)> eventDispatcher;

	// callback 1 -- lambda, or any functor object

	// This can't compile because a 'const Event &' can be passed to 'const MouseEvent &'
	//eventDispatcher.appendListener(mouseEventId, [](const MouseEvent & e) {});

	// This compiles. eventpp::argumentAdapter creates a functor object that static_cast 
	// 'const Event &' to 'const MouseEvent &' automatically.
	// Note we need to pass the function type to eventpp::argumentAdapter because the lambda
	// doesn't have any function type information and eventpp::argumentAdapter can't deduce
	// the type. This rule also applies to other functor object.
	eventDispatcher.appendListener(
		mouseEventId,
		eventpp::argumentAdapter<void(const MouseEvent &)>([](const MouseEvent & e) {
			std::cout << "Received MouseEvent in lambda, x=" << e.getX() << " y=" << e.getY() << std::endl;
		})
	);

	// callback 2 -- std::function
	// We don't need to pass the function type to eventpp::argumentAdapter because it can
	// deduce the type from the std::function
	eventDispatcher.appendListener(
		mouseEventId,
		eventpp::argumentAdapter(std::function<void(const MouseEvent &)>([](const MouseEvent & e) {
			std::cout << "Received MouseEvent in std::function, x=" << e.getX() << " y=" << e.getY() << std::endl;
		}))
	);

	// callback 3 -- free function
	// We don't need to pass the function type to eventpp::argumentAdapter because it can
	// deduce the type from the free function
	eventDispatcher.appendListener(
		mouseEventId,
		eventpp::argumentAdapter(tutorialArgumentAdapterFreeFunction)
	);

	eventDispatcher.dispatch(mouseEventId, MouseEvent(3, 5));
}

TEST_CASE("ArgumentAdapter tutorial 2, arguments with std::shared_ptr")
{
	std::cout << std::endl << "ArgumentAdapter tutorial 2, arguments with std::shared_ptr" << std::endl;

	// Note the argument can't be any reference to std::shared_ptr, such as 'const std::shared_ptr<Event> &',
	// because eventpp::argumentAdapter uses std::static_pointer_cast to cast the pointer and it doesn't
	// work on reference.
	eventpp::EventDispatcher<int, void(std::shared_ptr<Event>)> eventDispatcher;

	// This can't compile because a 'std::shared_ptr<Event>' can be passed to 'std::shared_ptr<MouseEvent>'
	//eventDispatcher.appendListener(mouseEventId, [](std::shared_ptr<MouseEvent> e) {});

	// This compiles. eventpp::argumentAdapter creates a functor object that static_cast 
	// 'std::shared_ptr<Event>' to 'std::shared_ptr<MouseEvent>' automatically.
	eventDispatcher.appendListener(
		mouseEventId,
		eventpp::argumentAdapter<void(std::shared_ptr<MouseEvent>)>([](std::shared_ptr<MouseEvent> e) {
			std::cout << "Received MouseEvent as std::shared_ptr, x=" << e->getX() << " y=" << e->getY() << std::endl;
		})
	);

	eventDispatcher.dispatch(mouseEventId, std::make_shared<MouseEvent>(3, 5));
}

