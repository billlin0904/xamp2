//=====================================================================================================================
// Copyright (c) 2018-2021 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

template <typename T>
class XAMP_BASE_API_ONLY_EXPORT Singleton {
public:
	/*
	* Get singleton instance.
	* 
	* note: 不允許跨Library呼叫, 因為不會產生單例物件.
	*/
	static T& GetInstance();

protected:
	Singleton() noexcept = default;
	virtual ~Singleton() noexcept = default;

public:
	XAMP_DISABLE_COPY(Singleton)
};

template <typename T>
T& Singleton<T>::GetInstance() {
	// https://preshing.com/20130930/double-checked-locking-is-fixed-in-cpp11/
	static T instance;
	return instance;
}

XAMP_BASE_NAMESPACE_END
