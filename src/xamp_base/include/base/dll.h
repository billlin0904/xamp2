//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cassert>
#include <type_traits>
#include <string_view>

#include <base/base.h>
#include <base/exception.h>

#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#else
#include <base/posix_handle.h>
#endif

namespace xamp::base {

XAMP_BASE_API ModuleHandle LoadModule(const std::string_view& file_name);

XAMP_BASE_API void* LoadModuleSymbol(const ModuleHandle& dll, const std::string_view & name);

template <typename T, typename U = std::enable_if_t<std::is_function_v<T>>>
class XAMP_BASE_API_ONLY_EXPORT DllFunction final {
public:
    DllFunction(ModuleHandle const& dll, const std::string_view name) {
        *reinterpret_cast<void**>(&func_) = LoadModuleSymbol(dll, name);
    }

    XAMP_ALWAYS_INLINE operator T* () const noexcept {
        return func_;
    }

    XAMP_DISABLE_COPY(DllFunction)
private:
    T *func_;
};

#define XAMP_DECLARE_DLL(Export_C_Func) DllFunction<decltype(Export_C_Func)>

}
