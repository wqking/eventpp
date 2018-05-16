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
#include "eventpp/eventdispatcher.h"

#include "test.h"

#include <iostream>

TEST_CASE("EventDispatcher tutorial 1, basic")
{
	std::cout << "EventDispatcher tutorial 1, basic" << std::endl;

	// The namespace is eventpp
	// The first template parameter int is the event type,
	// the event type can be any type such as std::string, int, etc.
	// The second is the prototype of the listener.
	eventpp::EventDispatcher<int, void ()> dispatcher;

	// Add a listener. As the type of dispatcher,
	// here 3 and 5 is the event type,
	// []() {} is the listener.
	// Lambda is not required, any function or std::function
	// or whatever function object with the required prototype is fine.
	dispatcher.appendListener(3, []() {
		std::cout << "Got event 3." << std::endl;
	});
	dispatcher.appendListener(5, []() {
		std::cout << "Got event 5." << std::endl;
	});
	dispatcher.appendListener(5, []() {
		std::cout << "Got another event 5." << std::endl;
	});

	// Dispatch the events, the first argument is always the event type.
	dispatcher.dispatch(3);
	dispatcher.dispatch(5);
}

TEST_CASE("EventDispatcher tutorial 2, listener with parameters")
{
	std::cout << "EventDispatcher tutorial 2, listener with parameters" << std::endl;

	// The listener has two parameters.
	eventpp::EventDispatcher<int, void (const std::string &, const bool)> dispatcher;

	dispatcher.appendListener(3, [](const std::string & s, const bool b) {
		std::cout << std::boolalpha << "Got event 3, s is " << s << " b is " << b << std::endl;
	});
	// The listener prototype doesn't need to be exactly same as the dispatcher.
	// It would be find as long as the arguments is compatible with the dispatcher.
	dispatcher.appendListener(5, [](std::string s, int b) {
		std::cout << std::boolalpha << "Got event 5, s is " << s << " b is " << b << std::endl;
	});
	dispatcher.appendListener(5, [](const std::string & s, const bool b) {
		std::cout << std::boolalpha << "Got another event 5, s is " << s << " b is " << b << std::endl;
	});

	// Dispatch the events, the first argument is always the event type.
	dispatcher.dispatch(3, "Hello", true);
	dispatcher.dispatch(5, "World", false);
}

TEST_CASE("EventDispatcher tutorial 3, customized Event struct")
{
	std::cout << "EventDispatcher tutorial 3, customized Event struct" << std::endl;

	// Define an Event to hold all parameters.
	struct MyEvent {
		int type;
		std::string message;
		int param;
	};

	// Define an event type getter to let the dispatcher knows how to
	// extract the event type.
	// The getter must derive from eventpp::EventGetterBase
	// The getter must have:
	// 1, A type named Event indicating the event type.
	// 2, A static member function named getEvent. It receives all parameters
	// same as the dispatcher prototype, and returns Event.
	struct MyEventTypeGetter : public eventpp::EventGetterBase
	{
		using Event = int;

		static Event getEvent(const MyEvent & e, bool /*b*/) {
			return e.type;
		}
	};

	// Pass MyEventTypeGetter as the first template argument of EventDispatcher
	eventpp::EventDispatcher<
		MyEventTypeGetter,
		void (const MyEvent &, bool)
	> dispatcher;

	// Add a listener.
	// Note: the first argument, event type, is MyEventTypeGetter::Event,
	// not Event
	dispatcher.appendListener(3, [](const MyEvent & e, bool b) {
		std::cout
			<< std::boolalpha
			<< "Got event 3" << std::endl
			<< "Event::type is " << e.type << std::endl
			<< "Event::message is " << e.message << std::endl
			<< "Event::param is " << e.param << std::endl
			<< "b is " << b << std::endl
		;
	});

	// Dispatch the event.
	// The first argument is Event.
	dispatcher.dispatch(MyEvent { 3, "Hello world", 38 }, true);
}

TEST_CASE("EventDispatcher tutorial 4, event queue")
{
	std::cout << "EventDispatcher tutorial 4, event queue" << std::endl;

	eventpp::EventDispatcher<int, void (const std::string &, const bool)> dispatcher;

	dispatcher.appendListener(3, [](const std::string & s, const bool b) {
		std::cout << std::boolalpha << "Got event 3, s is " << s << " b is " << b << std::endl;
	});
	// The listener prototype doesn't need to be exactly same as the dispatcher.
	// It would be find as long as the arguments is compatible with the dispatcher.
	dispatcher.appendListener(5, [](std::string s, int b) {
		std::cout << std::boolalpha << "Got event 5, s is " << s << " b is " << b << std::endl;
	});
	dispatcher.appendListener(5, [](const std::string & s, const bool b) {
		std::cout << std::boolalpha << "Got another event 5, s is " << s << " b is " << b << std::endl;
	});

	// Enqueue the events, the first argument is always the event type.
	// The listeners are not triggered during enqueue.
	dispatcher.enqueue(3, "Hello", true);
	dispatcher.enqueue(5, "World", false);

	// Process the event queue, dispatch all queued events.
	dispatcher.process();
}

TEST_CASE("EventDispatcher tutorial 5, event filter")
{
	std::cout << "EventDispatcher tutorial 5, event filter" << std::endl;

	eventpp::EventDispatcher<int, void (int e, int i, std::string)> dispatcher;

	dispatcher.appendListener(3, [](const int /*e*/, const int i, const std::string & s) {
		std::cout << "Got event 3, i was 1 but actural is " << i << " s was Hello but actural is " << s << std::endl;
	});
	dispatcher.appendListener(5, [](const int /*e*/, const int /*i*/, const std::string & /*s*/) {
		std::cout << "Shout not got event 5" << std::endl;
	});

	// Add three event filters.

	// The first filter modifies the input arguments to other values, then the subsequence filters
	// and listeners will see the modified values.
	dispatcher.appendFilter([](const int e, int & i, std::string & s) -> bool {
		std::cout << "Filter 1, e is " << e << " passed in i is " << i << " s is " << s << std::endl;
		i = 38;
		s = "Hi";
		std::cout << "Filter 1, changed i is " << i << " s is " << s << std::endl;
		return true;
	});

	// The second filter filters out all event of 5. So no listeners on event 5 can be triggered.
	// The third filter is not invoked on event 5 also.
	dispatcher.appendFilter([](const int e, int & i, std::string & s) -> bool {
		std::cout << "Filter 2, e is " << e << " passed in i is " << i << " s is " << s << std::endl;
		if(e == 5) {
			return false;
		}
		return true;
	});

	// The third filter just prints the input arguments.
	dispatcher.appendFilter([](const int e, int & i, std::string & s) -> bool {
		std::cout << "Filter 3, e is " << e << " passed in i is " << i << " s is " << s << std::endl;
		return true;
	});

	// Dispatch the events, the first argument is always the event type.
	dispatcher.dispatch(3, 1, "Hello");
	dispatcher.dispatch(5, 2, "World");
}

