//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

namespace xamp::base {

// https://preshing.com/20130930/double-checked-locking-is-fixed-in-cpp11/

template <typename T>
class XAMP_BASE_API_ONLY_EXPORT Singleton {
public:
	/// <summary>
	/// 不允許跨Library呼叫, 因為不會產生單例物件.
	/// </summary>
	/// <returns></returns>
	static T& GetInstance();

protected:
	Singleton() noexcept = default;
	virtual ~Singleton() noexcept = default;

public:
	XAMP_DISABLE_COPY(Singleton)
};

template <typename T>
T& Singleton<T>::GetInstance() {
	static T instance;
	return instance;
}

}
