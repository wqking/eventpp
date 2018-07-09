#include "test.h"

#include "eventpp/utilities/scopedremover.h"
#include "eventpp/eventdispatcher.h"

TEST_CASE("ScopedRemover, EventDispatcher")
{
	eventpp::EventDispatcher<int, void ()> dispatcher;
	using Remover = eventpp::ScopedRemover<eventpp::EventDispatcher<int, void ()> >;
	constexpr int event = 3;
	
	std::vector<int> dataList(4);
	
	dispatcher.appendListener(event, [&dataList]() {
		++dataList[0];
	});
	
	{
		Remover r1(dispatcher);
		r1.prependListener(event, [&dataList]() {
			++dataList[1];
		});
		{
			Remover r2(dispatcher);
			auto handle = r2.appendListener(event, [&dataList]() {
				++dataList[2];
			});
			{
				Remover r3(dispatcher);
				r3.insertListener(event, [&dataList]() {
					++dataList[3];
				}, handle);

				REQUIRE(dataList == std::vector<int> { 0, 0, 0, 0 });
				
				dispatcher.dispatch(event);
				REQUIRE(dataList == std::vector<int> { 1, 1, 1, 1 });
			}

			dispatcher.dispatch(event);
			REQUIRE(dataList == std::vector<int> { 2, 2, 2, 1 });
		}

		dispatcher.dispatch(event);
		REQUIRE(dataList == std::vector<int> { 3, 3, 2, 1 });
	}
	
	dispatcher.dispatch(event);
	REQUIRE(dataList == std::vector<int> { 4, 3, 2, 1 });
}

TEST_CASE("ScopedRemover, CallbackList")
{
	eventpp::CallbackList<void ()> callbackList;
	using Remover = eventpp::ScopedRemover<eventpp::CallbackList<void ()> >;
	
	std::vector<int> dataList(4);
	
	callbackList.append([&dataList]() {
		++dataList[0];
	});
	
	{
		Remover r1(callbackList);
		r1.prepend([&dataList]() {
			++dataList[1];
		});
		{
			Remover r2(callbackList);
			auto handle = r2.append([&dataList]() {
				++dataList[2];
			});
			{
				Remover r3(callbackList);
				r3.insert([&dataList]() {
					++dataList[3];
				}, handle);

				REQUIRE(dataList == std::vector<int> { 0, 0, 0, 0 });
				
				callbackList();
				REQUIRE(dataList == std::vector<int> { 1, 1, 1, 1 });
			}

			callbackList();
			REQUIRE(dataList == std::vector<int> { 2, 2, 2, 1 });
		}

		callbackList();
		REQUIRE(dataList == std::vector<int> { 3, 3, 2, 1 });
	}
	
	callbackList();
	REQUIRE(dataList == std::vector<int> { 4, 3, 2, 1 });
}

