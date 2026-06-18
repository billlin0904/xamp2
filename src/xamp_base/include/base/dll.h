//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/fs.h>
#include <base/assert.h>
#include <base/platfrom_handle.h>

#include <type_traits>
#include <string_view>

XAMP_BASE_NAMESPACE_BEGIN

/*
* LoadSharedLibrary
* 
* load shared library.
* @param[in] file_name Library file name.
* @return Shared library handle.
*/
XAMP_BASE_API SharedLibraryHandle loadSharedLibrary(const std::string_view& file_name);

/*
* OpenSharedLibrary
* 
* open shared library.
* @param[in] file_name Library file name.
* @return Shared library handle.
*/
XAMP_BASE_API SharedLibraryHandle openSharedLibrary(const std::string_view& file_name);

/*
* GetSharedLibraryPath
* 
* Get shared library path.
* @param[in] module Shared library handle.
* @return Shared library path.
*/
XAMP_BASE_API Path getSharedLibraryPath(const SharedLibraryHandle &module);

/*
* LoadSharedLibrarySymbol
* 
* load shared library symbol.
* @param[in] dll Shared library handle.
* @param[in] name Symbol name.
*/
XAMP_BASE_API void* loadSharedLibrarySymbol(const SharedLibraryHandle& dll, const std::string_view & name);

/*
* PrefetchSharedLibrary
* 
* Prefetch shared library.
* @param[in] module Shared library handle.
* @return Prefetch success or not.
*/
XAMP_BASE_API bool prefetchSharedLibrary(SharedLibraryHandle const& module);

/*
* AddSharedLibrarySearchDirectory
* 
* Add shared library search directory.
* @param[in] path Search directory path.
* @TODO Repeat add search directory will cause crash.
*/
XAMP_BASE_API bool addSharedLibrarySearchDirectory(const Path &path);

#ifdef XAMP_OS_WIN
/*
* LoadSharedLibrarySymbolEx
* 
* load shared library symbol with flags.
* @param[in] dll Shared library handle.
* @param[in] name Symbol name.
* @param[in] flags Symbol flags.
* @return Symbol address.
*/
XAMP_BASE_API void* loadSharedLibrarySymbolEx(SharedLibraryHandle const& dll, const std::string_view name, uint32_t flags);

/*
* PinSystemLibrary
*
* Pin system library.
* @param[in] file_name Library file name.
* @return Shared library handle.
*/
XAMP_BASE_API SharedLibraryHandle pinSystemLibrary(const std::string_view& file_name);
#endif

/*
* SharedLibraryFunction
* 
* A wrapper class for shared library function.
* @param t Function type.
* @param U Must be std::enable_if_t<std::is_function_v<t>>.
*/
template
<
    typename t,
    typename U = std::enable_if_t<std::is_function_v<t>>
>
class SharedLibraryFunction final {
public:
    static_assert(std::is_function_v<std::remove_pointer_t<t>>, "t must be a function pointer type");
    
    using FuncPtr = t*;

    /*
    * Constructor.
    * 
    * @param dll Shared library handle.
    * @param name Function name.    
    */
    SharedLibraryFunction(SharedLibraryHandle const& dll, const std::string_view name) {
        func_ = reinterpret_cast<t*>(loadSharedLibrarySymbol(dll, name));
    }

#ifdef XAMP_OS_WIN
    /*
    * Constructor.
    * 
    * @param dll Shared library handle.
    * @param name Function name.
    * @param flags Function flags.
    */
    SharedLibraryFunction(SharedLibraryHandle const& dll, const std::string_view name, uint32_t flags) {
        func_ = reinterpret_cast<t*>(loadSharedLibrarySymbolEx(dll, name, flags));
    }
#endif
    
    auto operator()(auto&&... args) const {
        XAMP_ASSERT(func_ != nullptr);
        return func_(std::forward<decltype(args)>(args)...);
    }

    [[nodiscard]] t* Get() const XAMP_CHECK_LIFETIME {
        XAMP_ASSERT(func_ != nullptr);
        return func_;
    }

    XAMP_DISABLE_COPY_AND_MOVE(SharedLibraryFunction)
private:
    FuncPtr func_{nullptr};
};

#define XAMP_DECLARE_DLL(Func) SharedLibraryFunction<decltype(Func)>
#define XAMP_DECLARE_DLL_NAME(Func) SharedLibraryFunction<decltype(Func)> Func
#define XAMP_LOAD_DLL_API(Func) Func(module_, #Func)
#define XAMP_LOAD_DLL_API_EX(MemberName, Func) MemberName(module_, #Func)

XAMP_BASE_NAMESPACE_END
