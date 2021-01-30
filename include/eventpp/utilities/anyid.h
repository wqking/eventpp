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

#ifndef ANYID_H_320827729782
#define ANYID_H_320827729782

#include <functional>
#include <type_traits>

namespace eventpp {

namespace internal_ {

template <typename T, typename Enabled = void>
struct MakeHash;

template <typename T>
struct MakeHash <T, typename std::enable_if<std::is_convertible<T, std::size_t>::value>::type>
{
	std::size_t operator() (T value) const {
		return static_cast<std::size_t>(value);
	}
};

template <typename T>
struct MakeHash <T, typename std::enable_if<! std::is_convertible<T, std::size_t>::value>::type>
{
	std::size_t operator() (const T & value) const {
		return std::hash<T>()(value);
	}
};

} //namespace internal_

struct EmptyStorage
{
	EmptyStorage() {}

	template <typename T>
	EmptyStorage(const T &) {}
};

template <template <typename> class Digester, typename Storage = EmptyStorage>
class AnyId
{
public:
	using DigestType = decltype(std::declval<Digester<int>>()(0));

public:
	AnyId() : digest(), value() {
	}

	template <typename T>
	AnyId(const T & value)
		: digest(Digester<T>()(value)), value(value)
	{
	}

	DigestType getDigest() const {
		return digest;
	}

	const Storage & getValue() const {
		return value;
	}

private:
	DigestType digest;
	Storage value;
};

template <template <typename> class Digester, typename Storage>
bool operator == (const AnyId<Digester, Storage> & a, const AnyId<Digester, Storage> & b)
{
	return a.getDigest() == b.getDigest();
}

template <template <typename> class Digester, typename Storage>
bool operator < (const AnyId<Digester, Storage> & a, const AnyId<Digester, Storage> & b)
{
	return a.getDigest() < b.getDigest();
}

using AnyHashableId = AnyId<std::hash>;

} //namespace eventpp

namespace std
{
template <template <typename> class Digester>
struct hash<eventpp::AnyId<Digester> >
{
	std::size_t operator()(const eventpp::AnyId<Digester> & value) const noexcept
	{
		return eventpp::internal_::MakeHash<typename eventpp::AnyId<Digester>::DigestType>()(value.getDigest());
	}
};
} //namespace std

#endif

