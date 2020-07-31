//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>
#include <cassert>

#include <base/base.h>
#include <output_device/win32/wasapi.h>

#ifdef XAMP_OS_WIN

namespace xamp::output_device::win32 {

template <typename Base>
class UnknownImpl : public Base {
public:
	UnknownImpl() noexcept
		: refcount_(0) {
	}

#ifdef _DEBUG
	virtual ~UnknownImpl() {
		assert(refcount_ == 0);
	}
#endif

	ULONG STDMETHODCALLTYPE AddRef() override {
		return ::InterlockedIncrement(&refcount_);
	}

	ULONG STDMETHODCALLTYPE Release() override {
		ULONG result = ::InterlockedDecrement(&refcount_);
		if (result == 0) {
			delete this;
		}
		return result;
	}

private:
	ULONG refcount_;
};

}

#endif

