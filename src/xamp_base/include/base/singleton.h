//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

namespace xamp::base {

// https://preshing.com/20130930/double-checked-locking-is-fixed-in-cpp11/

template <typename T>
class Singleton {
public:
	static T& Get();

protected:
	Singleton() noexcept = default;
	virtual ~Singleton() noexcept = default;

public:
	XAMP_DISABLE_COPY(Singleton)
};

template <typename T>
XAMP_ALWAYS_INLINE T& Singleton<T>::Get() {
	static T instance;
	return instance;
}

}
