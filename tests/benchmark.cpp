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

#include <chrono>
#include <map>
#include <unordered_map>
#include <random>
#include <string>
#include <iostream>

// To enable benchmark, change below line to #if 1
#if 0

namespace {

template <typename F>
uint64_t measureElapsedTime(F f)
{
	std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
	f();
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t).count();
}

int getRandomeInt()
{
	static std::random_device rd;
	static std::mt19937 engine(rd());
	static std::uniform_int_distribution<> dist;
	return dist(engine);
}

int getRandomeInt(const int max)
{
	return getRandomeInt() % max;
}

int getRandomeInt(const int min, const int max)
{
	if(min >= max) {
		return min;
	}
	return min + getRandomeInt() % (max - min);
}

std::string generateRandomString(const int length){
	static std::string possibleCharacters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	std::string result(length, 0);
	for(int i = 0; i < length; i++){
		result[i] = possibleCharacters[getRandomeInt((int)possibleCharacters.size())];
	}
	return result;
}

} //unnamed namespace

namespace {

#if defined(_MSC_VER)
#define NON_INLINE __declspec(noinline)
#else
// gcc
#define NON_INLINE __attribute__((noinline))
#endif

volatile int globalValue = 0;

void globalFunction(int a, const int b)
{
	globalValue += a + b;
}

NON_INLINE void nonInlineGlobalFunction(int a, const int b)
{
	globalValue += a + b;
}

struct FunctionObject
{
	void operator() (int a, const int b)
	{
		globalValue += a + b;
	}

	virtual void virFunc(int a, const int b)
	{
		globalValue += a + b;
	}

	void nonVirFunc(int a, const int b)
	{
		globalValue += a + b;
	}

	NON_INLINE virtual void nonInlineVirFunc(int a, const int b)
	{
		globalValue += a + b;
	}

	NON_INLINE void nonInlineNonVirFunc(int a, const int b)
	{
		globalValue += a + b;
	}
};

#undef NON_INLINE

} //unnamed namespace

TEST_CASE("benchmark, CallbackList invoking vs C++ invoking")
{
	constexpr int iterateCount = 1000 * 1000 * 100;

	{
		FunctionObject funcObject;
		const uint64_t cppTime = measureElapsedTime([iterateCount, &funcObject]() {
			for(int i = 0; i < iterateCount; ++i) {
				globalFunction(i, i);
			}
		});

		struct SingleThreadingPolicies {
			using Threading = eventpp::SingleThreading;
		};
		eventpp::CallbackList<void (int, int), SingleThreadingPolicies> callbackListSingleThreading;
		callbackListSingleThreading.append(&globalFunction);
		const uint64_t callbackListSingleThreadingTime = measureElapsedTime([iterateCount, &callbackListSingleThreading]() {
			for(int i = 0; i < iterateCount; ++i) {
				callbackListSingleThreading(i, i);
			}
		});

		struct MultiThreadingPolicies {
			using Threading = eventpp::MultipleThreading;
		};
		eventpp::CallbackList<void (int, int), MultiThreadingPolicies> callbackListMultiThreading;
		callbackListMultiThreading.append(&globalFunction);
		const uint64_t callbackListMultiThreadingTime = measureElapsedTime([iterateCount, &callbackListMultiThreading]() {
			for(int i = 0; i < iterateCount; ++i) {
				callbackListMultiThreading(i, i);
			}
		});

		std::cout << "globalFunction: " << cppTime << " " << callbackListSingleThreadingTime << " " << callbackListMultiThreadingTime << std::endl;
	}

	{
		FunctionObject funcObject;
		const uint64_t cppTime = measureElapsedTime([iterateCount, &funcObject]() {
			for(int i = 0; i < iterateCount; ++i) {
				nonInlineGlobalFunction(i, i);
			}
		});

		struct SingleThreadingPolicies {
			using Threading = eventpp::SingleThreading;
		};
		eventpp::CallbackList<void (int, int), SingleThreadingPolicies> callbackListSingleThreading;
		callbackListSingleThreading.append(&nonInlineGlobalFunction);
		const uint64_t callbackListSingleThreadingTime = measureElapsedTime([iterateCount, &callbackListSingleThreading]() {
			for(int i = 0; i < iterateCount; ++i) {
				callbackListSingleThreading(i, i);
			}
		});

		struct MultiThreadingPolicies {
			using Threading = eventpp::MultipleThreading;
		};
		eventpp::CallbackList<void (int, int), MultiThreadingPolicies> callbackListMultiThreading;
		callbackListMultiThreading.append(&nonInlineGlobalFunction);
		const uint64_t callbackListMultiThreadingTime = measureElapsedTime([iterateCount, &callbackListMultiThreading]() {
			for(int i = 0; i < iterateCount; ++i) {
				callbackListMultiThreading(i, i);
			}
		});

		std::cout << "nonInlineGlobalFunction: " << cppTime << " " << callbackListSingleThreadingTime << " " << callbackListMultiThreadingTime << std::endl;
	}

	{
		FunctionObject funcObject;
		const uint64_t cppTime = measureElapsedTime([iterateCount, &funcObject]() {
			for(int i = 0; i < iterateCount; ++i) {
				funcObject(i, i);
			}
		});

		struct SingleThreadingPolicies {
			using Threading = eventpp::SingleThreading;
		};
		eventpp::CallbackList<void (int, int), SingleThreadingPolicies> callbackListSingleThreading;
		callbackListSingleThreading.append(funcObject);
		const uint64_t callbackListSingleThreadingTime = measureElapsedTime([iterateCount, &callbackListSingleThreading]() {
			for(int i = 0; i < iterateCount; ++i) {
				callbackListSingleThreading(i, i);
			}
		});

		struct MultiThreadingPolicies {
			using Threading = eventpp::MultipleThreading;
		};
		eventpp::CallbackList<void (int, int), MultiThreadingPolicies> callbackListMultiThreading;
		callbackListMultiThreading.append(funcObject);
		const uint64_t callbackListMultiThreadingTime = measureElapsedTime([iterateCount, &callbackListMultiThreading]() {
			for(int i = 0; i < iterateCount; ++i) {
				callbackListMultiThreading(i, i);
			}
		});

		std::cout << "funcObject: " << cppTime << " " << callbackListSingleThreadingTime << " " << callbackListMultiThreadingTime << std::endl;
	}

	{
		FunctionObject funcObject;
		const uint64_t cppTime = measureElapsedTime([iterateCount, &funcObject]() {
			for(int i = 0; i < iterateCount; ++i) {
				funcObject.virFunc(i, i);
			}
		});

		struct SingleThreadingPolicies {
			using Threading = eventpp::SingleThreading;
		};
		eventpp::CallbackList<void (int, int), SingleThreadingPolicies> callbackListSingleThreading;
		callbackListSingleThreading.append(std::bind(&FunctionObject::virFunc, &funcObject, std::placeholders::_1, std::placeholders::_2));
		const uint64_t callbackListSingleThreadingTime = measureElapsedTime([iterateCount, &callbackListSingleThreading]() {
			for(int i = 0; i < iterateCount; ++i) {
				callbackListSingleThreading(i, i);
			}
		});

		struct MultiThreadingPolicies {
			using Threading = eventpp::MultipleThreading;
		};
		eventpp::CallbackList<void (int, int), MultiThreadingPolicies> callbackListMultiThreading;
		callbackListMultiThreading.append(std::bind(&FunctionObject::virFunc, &funcObject, std::placeholders::_1, std::placeholders::_2));
		const uint64_t callbackListMultiThreadingTime = measureElapsedTime([iterateCount, &callbackListMultiThreading]() {
			for(int i = 0; i < iterateCount; ++i) {
				callbackListMultiThreading(i, i);
			}
		});

		std::cout << "funcObject.virFunc: " << cppTime << " " << callbackListSingleThreadingTime << " " << callbackListMultiThreadingTime << std::endl;
	}

	{
		FunctionObject funcObject;
		const uint64_t cppTime = measureElapsedTime([iterateCount, &funcObject]() {
			for(int i = 0; i < iterateCount; ++i) {
				funcObject.nonVirFunc(i, i);
			}
		});

		struct SingleThreadingPolicies {
			using Threading = eventpp::SingleThreading;
		};
		eventpp::CallbackList<void (int, int), SingleThreadingPolicies> callbackListSingleThreading;
		callbackListSingleThreading.append(std::bind(&FunctionObject::nonVirFunc, &funcObject, std::placeholders::_1, std::placeholders::_2));
		const uint64_t callbackListSingleThreadingTime = measureElapsedTime([iterateCount, &callbackListSingleThreading]() {
			for(int i = 0; i < iterateCount; ++i) {
				callbackListSingleThreading(i, i);
			}
		});

		struct MultiThreadingPolicies {
			using Threading = eventpp::MultipleThreading;
		};
		eventpp::CallbackList<void (int, int), MultiThreadingPolicies> callbackListMultiThreading;
		callbackListMultiThreading.append(std::bind(&FunctionObject::nonVirFunc, &funcObject, std::placeholders::_1, std::placeholders::_2));
		const uint64_t callbackListMultiThreadingTime = measureElapsedTime([iterateCount, &callbackListMultiThreading]() {
			for(int i = 0; i < iterateCount; ++i) {
				callbackListMultiThreading(i, i);
			}
		});

		std::cout << "funcObject.nonVirFunc: " << cppTime << " " << callbackListSingleThreadingTime << " " << callbackListMultiThreadingTime << std::endl;
	}

	{
		FunctionObject funcObject;
		const uint64_t cppTime = measureElapsedTime([iterateCount, &funcObject]() {
			for(int i = 0; i < iterateCount; ++i) {
				funcObject.nonInlineVirFunc(i, i);
			}
		});

		struct SingleThreadingPolicies {
			using Threading = eventpp::SingleThreading;
		};
		eventpp::CallbackList<void (int, int), SingleThreadingPolicies> callbackListSingleThreading;
		callbackListSingleThreading.append(std::bind(&FunctionObject::nonInlineVirFunc, &funcObject, std::placeholders::_1, std::placeholders::_2));
		const uint64_t callbackListSingleThreadingTime = measureElapsedTime([iterateCount, &callbackListSingleThreading]() {
			for(int i = 0; i < iterateCount; ++i) {
				callbackListSingleThreading(i, i);
			}
		});

		struct MultiThreadingPolicies {
			using Threading = eventpp::MultipleThreading;
		};
		eventpp::CallbackList<void (int, int), MultiThreadingPolicies> callbackListMultiThreading;
		callbackListMultiThreading.append(std::bind(&FunctionObject::nonInlineVirFunc, &funcObject, std::placeholders::_1, std::placeholders::_2));
		const uint64_t callbackListMultiThreadingTime = measureElapsedTime([iterateCount, &callbackListMultiThreading]() {
			for(int i = 0; i < iterateCount; ++i) {
				callbackListMultiThreading(i, i);
			}
		});

		std::cout << "funcObject.nonInlineVirFunc: " << cppTime << " " << callbackListSingleThreadingTime << " " << callbackListMultiThreadingTime << std::endl;
	}

	{
		FunctionObject funcObject;
		const uint64_t cppTime = measureElapsedTime([iterateCount, &funcObject]() {
			for(int i = 0; i < iterateCount; ++i) {
				funcObject.nonInlineNonVirFunc(i, i);
			}
		});

		struct SingleThreadingPolicies {
			using Threading = eventpp::SingleThreading;
		};
		eventpp::CallbackList<void (int, int), SingleThreadingPolicies> callbackListSingleThreading;
		callbackListSingleThreading.append(std::bind(&FunctionObject::nonInlineNonVirFunc, &funcObject, std::placeholders::_1, std::placeholders::_2));
		const uint64_t callbackListSingleThreadingTime = measureElapsedTime([iterateCount, &callbackListSingleThreading]() {
			for(int i = 0; i < iterateCount; ++i) {
				callbackListSingleThreading(i, i);
			}
		});

		struct MultiThreadingPolicies {
			using Threading = eventpp::MultipleThreading;
		};
		eventpp::CallbackList<void (int, int), MultiThreadingPolicies> callbackListMultiThreading;
		callbackListMultiThreading.append(std::bind(&FunctionObject::nonInlineNonVirFunc, &funcObject, std::placeholders::_1, std::placeholders::_2));
		const uint64_t callbackListMultiThreadingTime = measureElapsedTime([iterateCount, &callbackListMultiThreading]() {
			for(int i = 0; i < iterateCount; ++i) {
				callbackListMultiThreading(i, i);
			}
		});

		std::cout << "funcObject.nonInlineNonVirFunc: " << cppTime << " " << callbackListSingleThreadingTime << " " << callbackListMultiThreadingTime << std::endl;
	}

	{
		FunctionObject funcObject;
		const uint64_t cppTime = measureElapsedTime([iterateCount, &funcObject]() {
			for(int i = 0; i < iterateCount; ++i) {
				globalFunction(i, i);
				nonInlineGlobalFunction(i, i);
				funcObject(i, i);
				funcObject.virFunc(i, i);
				funcObject.nonVirFunc(i, i);
				funcObject.nonInlineVirFunc(i, i);
				funcObject.nonInlineNonVirFunc(i, i);
			}
		});

		struct SingleThreadingPolicies {
			using Threading = eventpp::SingleThreading;
		};
		eventpp::CallbackList<void (int, int), SingleThreadingPolicies> callbackListSingleThreading;
		callbackListSingleThreading.append(&globalFunction);
		callbackListSingleThreading.append(&nonInlineGlobalFunction);
		callbackListSingleThreading.append(funcObject);
		callbackListSingleThreading.append(std::bind(&FunctionObject::virFunc, &funcObject, std::placeholders::_1, std::placeholders::_2));
		callbackListSingleThreading.append(std::bind(&FunctionObject::nonVirFunc, &funcObject, std::placeholders::_1, std::placeholders::_2));
		callbackListSingleThreading.append(std::bind(&FunctionObject::nonInlineVirFunc, &funcObject, std::placeholders::_1, std::placeholders::_2));
		callbackListSingleThreading.append(std::bind(&FunctionObject::nonInlineNonVirFunc, &funcObject, std::placeholders::_1, std::placeholders::_2));
		const uint64_t callbackListSingleThreadingTime = measureElapsedTime([iterateCount, &callbackListSingleThreading]() {
			for(int i = 0; i < iterateCount; ++i) {
				callbackListSingleThreading(i, i);
			}
		});

		struct MultiThreadingPolicies {
			using Threading = eventpp::MultipleThreading;
		};
		eventpp::CallbackList<void (int, int), MultiThreadingPolicies> callbackListMultiThreading;
		callbackListMultiThreading.append(&globalFunction);
		callbackListMultiThreading.append(&nonInlineGlobalFunction);
		callbackListMultiThreading.append(funcObject);
		callbackListMultiThreading.append(std::bind(&FunctionObject::virFunc, &funcObject, std::placeholders::_1, std::placeholders::_2));
		callbackListMultiThreading.append(std::bind(&FunctionObject::nonVirFunc, &funcObject, std::placeholders::_1, std::placeholders::_2));
		callbackListMultiThreading.append(std::bind(&FunctionObject::nonInlineVirFunc, &funcObject, std::placeholders::_1, std::placeholders::_2));
		callbackListMultiThreading.append(std::bind(&FunctionObject::nonInlineNonVirFunc, &funcObject, std::placeholders::_1, std::placeholders::_2));
		const uint64_t callbackListMultiThreadingTime = measureElapsedTime([iterateCount, &callbackListMultiThreading]() {
			for(int i = 0; i < iterateCount; ++i) {
				callbackListMultiThreading(i, i);
			}
		});

		std::cout << "All: " << cppTime << " " << callbackListSingleThreadingTime << " " << callbackListMultiThreadingTime << std::endl;
	}
}

TEST_CASE("benchmark, std::map vs std::unordered_map")
{
	constexpr int stringCount = 1000 * 1000;
	std::vector<std::string> stringList(stringCount);
	for(auto & s : stringList) {
		s = generateRandomString(getRandomeInt(3, 10));
	}

	constexpr int iterateCount = 1000 * 1000 * 1;

	uint64_t mapInsertTime = 0;
	uint64_t mapLookupTime = 0;
	{
		std::map<std::string, int> map;
		mapInsertTime = measureElapsedTime([iterateCount, stringCount, &map, &stringList]() {
			for(int i = 0; i < iterateCount; ++i) {
				map[stringList[i % stringCount]] = i;
			}
		});
		
		mapLookupTime = measureElapsedTime([iterateCount, stringCount, &map, &stringList]() {
			for(int i = iterateCount - 1; i >= 0; --i) {
				if(map.find(stringList[i % stringCount]) == map.end()) {
					stringList[i] = stringList[i];
				}
			}
		});
	}

	uint64_t unorderedMapInsertTime = 0;
	uint64_t unorderedMapLookupTime = 0;
	{
		std::unordered_map<size_t, int> map;
		unorderedMapInsertTime = measureElapsedTime([iterateCount, stringCount, &map, &stringList]() {
			for(int i = 0; i < iterateCount; ++i) {
				map[std::hash<std::string>()(stringList[i % stringCount])] = i;
			}
		});

		unorderedMapLookupTime = measureElapsedTime([iterateCount, stringCount, &map, &stringList]() {
			for(int i = stringCount - 1; i >= 0; --i) {
				if(map.find(std::hash<std::string>()(stringList[i])) == map.end()) {
					stringList[i] = stringList[i];
				}
			}
		});
	}
	std::cout << mapInsertTime << " " << mapLookupTime << std::endl;
	std::cout << unorderedMapInsertTime << " " << unorderedMapLookupTime << std::endl;
}

#endif
