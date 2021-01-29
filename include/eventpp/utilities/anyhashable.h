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

#ifndef ANYHASHABLE_H_320827729782
#define ANYHASHABLE_H_320827729782

#include <functional>

namespace eventpp {

class AnyHashable
{
public:
	AnyHashable() : hashValue() {
	}

	template <typename T>
	AnyHashable(const T & value) : hashValue(std::hash<T>()(value)) {
	}

	std::size_t getHash() const {
		return hashValue;
	}

private:
	std::size_t hashValue;
};

bool operator == (const AnyHashable & a, const AnyHashable & b)
{
	return a.getHash() == b.getHash();
}

bool operator < (const AnyHashable & a, const AnyHashable & b)
{
	return a.getHash() < b.getHash();
}

} //namespace eventpp

namespace std
{
template<> struct hash<eventpp::AnyHashable>
{
	std::size_t operator()(const eventpp::AnyHashable & value) const noexcept
	{
		return value.getHash();
	}
};
} //namespace std

#endif

