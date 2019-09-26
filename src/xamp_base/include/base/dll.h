//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/exception.h>

#ifdef _WIN32
#include <base/windows_handle.h>
#endif

namespace xamp::base {

XAMP_BASE_API ModuleHandle LoadDll(const char* name);

#ifdef _WIN32
template <typename T>
class DllFunction {
public:
	DllFunction(const ModuleHandle& dll, const char* name)  {		
		*(void**)& func_ = ::GetProcAddress(dll.get(), name);
	}

	XAMP_DISABLE_COPY(DllFunction)

	XAMP_NEVER_INLINE operator T* () const noexcept {
		return func_;
	}

	XAMP_NEVER_INLINE operator bool() const noexcept {
		return func_ != nullptr;
	}
private:
	T* func_;
};

#define XAMP_DEFINE_DLL_API(ImportFunc) DllFunction<decltype(ImportFunc)>

#endif

}