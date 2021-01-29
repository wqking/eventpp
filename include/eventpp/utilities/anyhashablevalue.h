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

#ifndef ANYHASHABLEVALUE_H_831812529057
#define ANYHASHABLEVALUE_H_831812529057

#include <functional>
#include <any>

namespace eventpp {

class AnyHashableValue
{
public:
	AnyHashableValue()
		: hashValue(), value() {
	}

	template <typename T>
	AnyHashableValue(const T & value)
		: hashValue(std::hash<T>()(value)), value(value) {
	}

	std::size_t getHash() const {
		return hashValue;
	}

	const std::any & getValue() const {
		return value;
	}

private:
	std::size_t hashValue;
	std::any value;
};

bool operator == (const AnyHashableValue & a, const AnyHashableValue & b)
{
	return a.getHash() == b.getHash();
}

bool operator < (const AnyHashableValue & a, const AnyHashableValue & b)
{
	return a.getHash() < b.getHash();
}

} //namespace eventpp

namespace std
{
template<> struct hash<eventpp::AnyHashableValue>
{
	std::size_t operator()(const eventpp::AnyHashableValue & value) const noexcept
	{
		return value.getHash();
	}
};
} //namespace std

#endif

