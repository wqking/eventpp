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

#include <functional>
#include <vector>

// To enable benchmark, change below line to #if 1
#if 1

namespace {

template <typename Policies>
void doCallbackListVsFunctionList(const std::string & message)
{
	eventpp::CallbackList<void (size_t)> callbackList;
	std::vector<std::function<void (size_t)> > functionList;
	constexpr size_t callbackCount = 100;
	constexpr size_t iterateCount = 1000 * 1000;
	volatile size_t data = 0;
	
	for(size_t i = 0; i < callbackCount; ++i) {
		callbackList.append([&data, i](size_t index) {
			data += i + index;
		});
		functionList.push_back([&data, i](size_t index) {
			data += i + index;
		});
	}
	const uint64_t timeCallbackList = measureElapsedTime(
		[iterateCount, &callbackList]() {
			for(size_t iterate = 0; iterate < iterateCount; ++iterate) {
				callbackList(iterate);
			}
		}
	);
	const uint64_t timeFunctionList = measureElapsedTime(
		[iterateCount, &functionList]() {
			for(size_t iterate = 0; iterate < iterateCount; ++iterate) {
				for(auto & func : functionList) {
					func(iterate);
				}
			}
		}
	);
	REQUIRE(data >= 0);

	std::cout << message << " timeCallbackList " << timeCallbackList << std::endl;
	std::cout << message << " timeFunctionList " << timeFunctionList << std::endl;
}

} //unnamed namespace

TEST_CASE("benchmark, CallbackList vs vector of functions")
{
	struct PoliciesMultiThreading {
		using Threading = eventpp::MultipleThreading;
	};
	doCallbackListVsFunctionList<PoliciesMultiThreading>("Multi thread");

	struct PoliciesSingleThreading {
		using Threading = eventpp::SingleThreading;
	};
	doCallbackListVsFunctionList<PoliciesSingleThreading>("Single thread");
}


#endif
