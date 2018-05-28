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

#ifndef TYPEUTIL_H_879959971810
#define TYPEUTIL_H_879959971810

namespace eventpp {

template <typename F, template <typename> class T>
struct TransformArguments;

template <template <typename> class T, typename RT, typename ...Args>
struct TransformArguments <RT (Args...), T>
{
	using Type = RT (typename T<Args>::type...);
};

template <typename F, typename Replacement>
struct ReplaceReturnType;

template <typename Replacement, typename RT, typename ...Args>
struct ReplaceReturnType <RT (Args...), Replacement>
{
	using Type = Replacement (Args...);
};


} //namespace eventpp

#endif

