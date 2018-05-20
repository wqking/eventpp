#include "test.h"

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
