//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <type_traits>
#include <string_view>

#include <base/base.h>
#include <base/platfrom_handle.h>

namespace xamp::base {

XAMP_BASE_API ModuleHandle LoadModule(const std::string_view& file_name);

XAMP_BASE_API Path GetModulePath(const ModuleHandle &module);

XAMP_BASE_API void* LoadModuleSymbol(const ModuleHandle& dll, const std::string_view & name);

XAMP_BASE_API bool PrefetchModule(ModuleHandle const& module);

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
#define XAMP_LOAD_DLL_API(DLLFunc) DLLFunc(module_, #DLLFunc)

}
