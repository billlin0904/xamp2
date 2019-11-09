//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cassert>
#include <type_traits>
#include <string_view>

#include <base/base.h>
#include <base/exception.h>

#ifdef _WIN32
#include <base/windows_handle.h>
#else
#include <base/posix_handle.h>
#endif

namespace xamp::base {

XAMP_BASE_API ModuleHandle LoadDll(std::string_view name);

XAMP_BASE_API void* LoadDllSymbol(const ModuleHandle& dll, std::string_view name);

template <typename T, typename U = std::enable_if_t<std::is_function<T>::value>>
class DllFunction final {
public:
    DllFunction(const ModuleHandle& dll, std::string_view name) noexcept {
        assert(dll.is_valid());
        *(void**)& func_ = LoadDllSymbol(dll, name);
    }

    XAMP_DISABLE_COPY(DllFunction)

    XAMP_ALWAYS_INLINE operator T* () const noexcept {
        assert(func_ != nullptr);
        return func_;
    }

    XAMP_ALWAYS_INLINE operator bool() const noexcept {
        return func_ != nullptr;
    }
private:
    T* func_;
};

#define XAMP_DEFINE_DLL_API(ImportFunc) DllFunction<decltype(ImportFunc)>

}
