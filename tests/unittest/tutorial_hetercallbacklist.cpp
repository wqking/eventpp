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
#include "eventpp/hetercallbacklist.h"

#include "test.h"

#include <iostream>

TEST_CASE("HeterCallbackList tutorial 1, basic")
{
	std::cout << "HeterCallbackList tutorial 1, basic" << std::endl;

	// The namespace is eventpp
	// the first parameter is a HeterTuple of the listener prototypes.
	eventpp::HeterCallbackList<eventpp::HeterTuple<void (), void (int)> > callbackList;

	// Add a callback.
	// []() {} is the callback.
	// Lambda is not required, any function or std::function
	// or whatever function object with the required prototype is fine.
	callbackList.append([]() {
		std::cout << "Got callback for void()." << std::endl;
	});
	callbackList.append([](int) {
		std::cout << "Got callback for void(int)." << std::endl;
	});

	// Invoke the callback list
	// Output: Got callback for void().
	callbackList();
	// Output: Got callback for void(int).
	callbackList(5);
}

#if 0
TEST_CASE("HeterCallbackList tutorial 2, for each")
{
	std::cout << "HeterCallbackList tutorial 4, for each" << std::endl;

	using CL = eventpp::HeterCallbackList<eventpp::HeterTuple<void (), void (int), void (int, int)> >;
	CL callbackList;

	// Add some callbacks.
	callbackList.append([]() {
		std::cout << "Got callback void()." << std::endl;
	});
	callbackList.append([](int a) {
		std::cout << "Got callback void(" << a << ")." << std::endl;
	});
	callbackList.append([](int a, int b) {
		std::cout << "Got callback void(" << a << ", " << b << ")." << std::endl;
	});

	// Now call forEach to remove the second callback
	// The forEach callback prototype is void(const HeterCallbackList::Handle & handle, const HeterCallbackList::Callback & callback)
	int index = 0;
	callbackList.forEach([&callbackList, &index](const CL::Handle & handle, const CL::Callback & /*callback*/) {
		std::cout << "forEach(Handle, Callback), invoked " << index << std::endl;
		if(index == 1) {
			callbackList.remove(handle);
			std::cout << "forEach(Handle, Callback), removed second callback" << std::endl;
		}
		++index;
	});

	// The forEach callback prototype can also be void(const HeterCallbackList::Handle & handle)
	callbackList.forEach([&callbackList, &index](const CL::Handle & /*handle*/) {
		std::cout << "forEach(Handle), invoked" << std::endl;
	});

	// The forEach callback prototype can also be void(const HeterCallbackList::Callback & callback)
	callbackList.forEach([&callbackList, &index](const CL::Callback & /*callback*/) {
		std::cout << "forEach(Callback), invoked" << std::endl;
	});

	// Invoke the callback list
	// The "Got callback 2" callback should not be triggered.
	callbackList();
}
#endif
