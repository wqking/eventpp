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
#include "eventpp/callbacklist.h"
#include "eventpp/eventdispatcher.h"
#include "eventpp/hetereventdispatcher.h"

#include <memory>
#include <functional>

namespace {

// A callable that counts how many times it's copied. It holds a large array so that
// std::function must allocate it on the heap, which makes the copy observable.
struct CopyCountingCallable
{
	explicit CopyCountingCallable(std::shared_ptr<int> copyCount)
		: copyCount(copyCount), padding()
	{
	}

	CopyCountingCallable(const CopyCountingCallable & other)
		: copyCount(other.copyCount), padding()
	{
		++*copyCount;
	}

	CopyCountingCallable(CopyCountingCallable && other) noexcept
		: copyCount(std::move(other.copyCount)), padding()
	{
	}

	void operator() (int) const {
	}

	std::shared_ptr<int> copyCount;
	char padding[256];
};

} //unnamed namespace

TEST_CASE("CallbackList, rvalue callback is moved, lvalue callback is copied")
{
	using CL = eventpp::CallbackList<void (int)>;
	CL callbackList;

	std::shared_ptr<int> copyCount = std::make_shared<int>(0);

	SECTION("append") {
		CL::Callback callback = CopyCountingCallable(copyCount);
		const int baseline = *copyCount;

		callbackList.append(callback);
		REQUIRE(*copyCount == baseline + 1);

		callbackList.append(std::move(callback));
		REQUIRE(*copyCount == baseline + 1);
	}

	SECTION("prepend") {
		CL::Callback callback = CopyCountingCallable(copyCount);
		const int baseline = *copyCount;

		callbackList.prepend(callback);
		REQUIRE(*copyCount == baseline + 1);

		callbackList.prepend(std::move(callback));
		REQUIRE(*copyCount == baseline + 1);
	}

	SECTION("insert") {
		CL::Handle handle = callbackList.append([](int) {});

		CL::Callback callback = CopyCountingCallable(copyCount);
		const int baseline = *copyCount;

		callbackList.insert(callback, handle);
		REQUIRE(*copyCount == baseline + 1);

		callbackList.insert(std::move(callback), handle);
		REQUIRE(*copyCount == baseline + 1);

		// insert with an invalid handle appends
		CL::Callback callback2 = CopyCountingCallable(copyCount);
		const int baseline2 = *copyCount;
		callbackList.insert(std::move(callback2), CL::Handle());
		REQUIRE(*copyCount == baseline2);
	}

	SECTION("temporary std::function and plain callable are moved") {
		const int baseline = *copyCount;
		callbackList.append(CL::Callback(CopyCountingCallable(copyCount)));
		REQUIRE(*copyCount == baseline);
	}

	// the callbacks are really added and invokable
	int invoked = 0;
	callbackList.forEach([&invoked](const CL::Callback &) {
		++invoked;
	});
	REQUIRE(invoked > 0);
	callbackList(0);
}

TEST_CASE("EventDispatcher, rvalue listener is moved, lvalue listener is copied")
{
	using ED = eventpp::EventDispatcher<int, void (int)>;
	ED dispatcher;

	std::shared_ptr<int> copyCount = std::make_shared<int>(0);

	SECTION("appendListener") {
		ED::Callback callback = CopyCountingCallable(copyCount);
		const int baseline = *copyCount;

		dispatcher.appendListener(3, callback);
		REQUIRE(*copyCount == baseline + 1);

		dispatcher.appendListener(3, std::move(callback));
		REQUIRE(*copyCount == baseline + 1);
	}

	SECTION("prependListener") {
		ED::Callback callback = CopyCountingCallable(copyCount);
		const int baseline = *copyCount;

		dispatcher.prependListener(3, callback);
		REQUIRE(*copyCount == baseline + 1);

		dispatcher.prependListener(3, std::move(callback));
		REQUIRE(*copyCount == baseline + 1);
	}

	SECTION("insertListener") {
		ED::Handle handle = dispatcher.appendListener(3, [](int) {});

		ED::Callback callback = CopyCountingCallable(copyCount);
		const int baseline = *copyCount;

		dispatcher.insertListener(3, callback, handle);
		REQUIRE(*copyCount == baseline + 1);

		dispatcher.insertListener(3, std::move(callback), handle);
		REQUIRE(*copyCount == baseline + 1);
	}

	REQUIRE(dispatcher.hasAnyListener(3));
	dispatcher.dispatch(3, 0);
}

TEST_CASE("EventDispatcher, listener added as lambda still works")
{
	eventpp::EventDispatcher<int, void (int)> dispatcher;

	int count = 0;
	auto lambda = [&count](int v) {
		count += v;
	};
	// lvalue lambda, rvalue lambda, function pointer style std::function
	dispatcher.appendListener(3, lambda);
	dispatcher.appendListener(3, [&count](int v) {
		count += v * 10;
	});
	std::function<void (int)> func = lambda;
	dispatcher.appendListener(3, func);
	dispatcher.appendListener(3, std::move(func));

	dispatcher.dispatch(3, 1);
	REQUIRE(count == 13);
}

TEST_CASE("HeterEventDispatcher, listener callable is not copied more than once")
{
	eventpp::HeterEventDispatcher<int, eventpp::HeterTuple<void (int)> > dispatcher;

	std::shared_ptr<int> copyCount = std::make_shared<int>(0);
	CopyCountingCallable callable(copyCount);
	const int baseline = *copyCount;

	// the callable is copied once into the std::function, the std::function itself is then moved
	dispatcher.appendListener(3, callable);
	REQUIRE(*copyCount == baseline + 1);

	dispatcher.dispatch(3, 0);
}

