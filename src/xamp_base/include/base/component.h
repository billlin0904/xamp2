//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <any>
#include <base/base.h>

namespace xamp::base {

template <typename Interface>
struct XAMP_BASE_API_ONLY_EXPORT Implementation {
public:
    template<typename ConcreteType>
    Implementation(ConcreteType&& object)
        : storage_{ std::forward<ConcreteType>(object) }
		, getter_{ [](std::any& storage) -> Interface& { return std::any_cast<ConcreteType&>(storage); } } {	    
    }

    Interface* operator->() {
	    return &getter_(storage_);
    }

private:
    std::any storage_;
    Interface& (*getter_)(std::any&);
};

}
