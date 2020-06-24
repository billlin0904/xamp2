//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
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

XAMP_BASE_API ModuleHandle LoadModule(std::string_view file_name);

XAMP_BASE_API void* LoadModuleSymbol(const ModuleHandle& dll, std::string_view name);

#ifdef XAMP_OS_WIN
XAMP_BASE_API void* LoadModuleSymbol(const ModuleHandle& dll, uint64_t addr);
#endif

template <typename T, typename U = std::enable_if_t<std::is_function<T>::value>>
class XAMP_BASE_API_ONLY_EXPORT DllFunction final {
public:
    DllFunction(ModuleHandle const& dll, std::string_view name) {
        *reinterpret_cast<void**>(&func_) = LoadModuleSymbol(dll, name);
    }

#ifdef XAMP_OS_WIN
    DllFunction(ModuleHandle const& dll, uint64_t addr) {
        *reinterpret_cast<void**>(&func_) = LoadModuleSymbol(dll, addr);
    }
#endif

    XAMP_ALWAYS_INLINE operator T* () const noexcept {
        assert(func_ != nullptr);
        return func_;
    }

    XAMP_ALWAYS_INLINE operator bool() const noexcept {
        return func_ != nullptr;
    }
private:
    T *func_;
};

#define XAMP_DECLARE_DLL(Export_C_Func) DllFunction<decltype(Export_C_Func)>

}
