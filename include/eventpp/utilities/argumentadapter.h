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

#ifndef ARGUMENTADAPTER_H_566280692673
#define ARGUMENTADAPTER_H_566280692673

namespace eventpp {

template <typename Func, typename Prototype>
struct ArgumentAdapter;

template <typename Func, typename R, typename ...Args>
struct ArgumentAdapter<Func, R(Args...)>
{
	ArgumentAdapter(Func f) : func(std::move(f)) {}

	template <typename ...A>
	void operator() (A &&...args) {
		func(std::forward<Args>(static_cast<Args>(args))...);
	}

	Func func;
};

template <template <typename> class Func, typename R, typename ...Args>
ArgumentAdapter<Func<R(Args...)>, R(Args...)> argumentAdapter(Func<R(Args...)> f)
{
	return ArgumentAdapter<Func<R(Args...)>, R(Args...)>(f);
}

template <typename Prototype, typename Func>
ArgumentAdapter<Func, Prototype> argumentAdapter(Func f)
{
	return ArgumentAdapter<Func, Prototype>(f);
}

template <typename R, typename ...Args>
ArgumentAdapter<R(*)(Args...), R(Args...)> argumentAdapter(R(*f)(Args...))
{
	return ArgumentAdapter<R(*)(Args...), R(Args...)>(f);
}


} //namespace eventpp


#endif

