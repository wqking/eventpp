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

#ifndef HETEREVENTDISPATCHER_I_H
#define HETEREVENTDISPATCHER_I_H

namespace eventpp {

namespace internal_ {

template <int N, typename PrototypeList_, typename Callable>
struct FindPrototypeByCallableHelper;

template <int N, typename RT, typename ...Args, typename ...Others, typename Callable>
struct FindPrototypeByCallableHelper <N, std::tuple<RT (Args...), Others...>, Callable>
{
	enum { canInvoke = CanInvoke<Callable, Args...>::value };

	using Prototype = typename std::conditional<
		canInvoke,
		RT (Args...),
		typename FindPrototypeByCallableHelper<N + 1, std::tuple<Others...>, Callable>::Prototype
	>::type;

	using ArgsTuple = typename std::conditional<
		canInvoke,
		std::tuple<typename std::remove_cv<typename std::remove_reference<Args>::type>::type...>,
		typename FindPrototypeByCallableHelper<N + 1, std::tuple<Others...>, Callable>::ArgsTuple
	>::type;

	enum {
		index = canInvoke
			? N
			: FindPrototypeByCallableHelper<N + 1, std::tuple<Others...>, Callable>::index
	};
};

template <int N, typename Callable>
struct FindPrototypeByCallableHelper <N, std::tuple<>, Callable>
{
	using Prototype = void;

	using ArgsTuple = std::tuple<>;

	enum {
		index = -1
	};
};

template <typename PrototypeList_, typename Callable>
struct FindPrototypeByCallable : public FindPrototypeByCallableHelper <0, PrototypeList_, Callable>
{
};

template <int N, typename PrototypeList_, typename ...InArgs>
struct FindPrototypeByArgsHelper;

template <int N, typename RT, typename ...Args, typename ...Others, typename ...InArgs>
struct FindPrototypeByArgsHelper <N, std::tuple<RT (Args...), Others...>, InArgs...>
{
	enum { canInvoke = CanInvoke<RT (Args...), InArgs...>::value };

	using Prototype = typename std::conditional<
		canInvoke,
		RT (Args...),
		typename FindPrototypeByArgsHelper<N + 1, std::tuple<Others...>, InArgs...>::Prototype
	>::type;

	using ArgsTuple = typename std::conditional<
		canInvoke,
		std::tuple<typename std::remove_cv<typename std::remove_reference<Args>::type>::type...>,
		typename FindPrototypeByArgsHelper<N + 1, std::tuple<Others...>, InArgs...>::ArgsTuple
	>::type;

	enum {
		index = canInvoke
		? N
		: FindPrototypeByArgsHelper<N + 1, std::tuple<Others...>, InArgs...>::index
	};
};

template <int N, typename ...InArgs>
struct FindPrototypeByArgsHelper <N, std::tuple<>, InArgs...>
{
	using Prototype = void;

	using ArgsTuple = std::tuple<>;

	enum { index = -1 };
};

template <typename PrototypeList_, typename ...InArgs>
struct FindPrototypeByArgs : public FindPrototypeByArgsHelper <0, PrototypeList_, InArgs...>
{
};

template <int I, int N, typename PrototypeList_>
struct FindPrototypeByIndexHelper;

template <int I, int N, typename RT, typename ...Args, typename ...Others>
struct FindPrototypeByIndexHelper <I, N, std::tuple<RT (Args...), Others...> >
{
	using NextType = FindPrototypeByIndexHelper<I + 1, N, std::tuple<Others...> >;
	using Prototype = typename NextType::Prototype;

	using ArgsTuple = typename NextType::ArgsTuple;

	enum { index = NextType::index };
};

template <int N, typename RT, typename ...Args, typename ...Others>
struct FindPrototypeByIndexHelper <N, N, std::tuple<RT (Args...), Others...> >
{
	using Prototype = RT (Args...);

	using ArgsTuple = std::tuple<typename std::remove_cv<typename std::remove_reference<Args>::type>::type...>;

	enum { index = N };
};

template <int I, int N>
struct FindPrototypeByIndexHelper <I, N, std::tuple<>>
{
	using Prototype = void;

	using ArgsTuple = std::tuple<>;

	enum { index = -1 };
};

template <typename PrototypeList_, int N>
struct FindPrototypeByIndex : public FindPrototypeByIndexHelper <0, N, PrototypeList_>
{
};

template <typename PrototypeList_, template <typename ...> class Record>
struct GetCallablePrototypeMaxSize;

template <typename RT, typename ...Args, typename ...Others, template <typename ...> class Record>
struct GetCallablePrototypeMaxSize <std::tuple<RT (Args...), Others...>, Record>
{
	enum {
		my = sizeof(Record<Args...>),
		other = GetCallablePrototypeMaxSize<std::tuple<Others...>, Record>::value
	};
	enum { value = (my > other ? (int)my : (int)other) };
};

template <template <typename ...> class Record>
struct GetCallablePrototypeMaxSize <std::tuple<>, Record>
{
	enum { value = 1 }; // set minimum size to 1 instead of 0
};

} //namespace internal_


} //namespace eventpp

#endif
