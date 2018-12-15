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

#ifndef MIXINHETERFILTER_H_990158796753
#define MIXINHETERFILTER_H_990158796753

#include "../callbacklist.h"
#include "../internal/typeutil_i.h"

#include <functional>
#include <type_traits>

namespace eventpp {

template <typename Base>
class MixinHeterFilter : public Base
{
private:
	using super = Base;

	struct FilterHandle_
	{
		int index;
		std::weak_ptr<void> homoHandle;

		operator bool () const noexcept {
			return (bool)homoHandle;
		}
	};

	struct RemoveFilterCallback
	{
		template <int N, typename PrototypeList>
		bool operator() (MixinHeterFilter * self, const PrototypeList *, const FilterHandle_ & filterHandle) const
		{
			auto sp = filterHandle.homoHandle.lock();
			if(! sp) {
				return false;
			}

			using PrototypeInfo = internal_::FindPrototypeByIndex<PrototypeList, N>;
			auto dispatcher = self->template doFindDispatcher<PrototypeInfo>();
			if(! dispatcher) {
				return false;
			}
			return dispatcher->removeFilter(typename decltype(dispatcher)::element_type::FilterHandle(
				std::static_pointer_cast<
					typename decltype(dispatcher)::element_type::FilterHandle::element_type
				>(sp)
			));
		}
	};

public:
	using FilterHandle = FilterHandle_;

public:
	template <typename C>
	FilterHandle appendFilter(const C & callback) const
	{
		using PrototypeInfo = internal_::FindPrototypeByCallable<typename super::PrototypeList, C>;
		static_assert(PrototypeInfo::index >= 0, "Can't find invoker for the given argument types.");

		auto dispatcher = this->template doFindDispatcher<PrototypeInfo>();
		return FilterHandle {
			PrototypeInfo::index,
			dispatcher->appendFilter(callback)
		};
	}

	bool removeFilter(const FilterHandle & filterHandle)
	{
		return internal_::intToConstant<std::tuple_size<typename super::PrototypeList>::value>(
			filterHandle.index,
			RemoveFilterCallback(),
			this,
			(typename super::PrototypeList *)nullptr,
			filterHandle
		);
	}

private:
};


} //namespace eventpp


#endif

