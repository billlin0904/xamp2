//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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
* Load shared library.
* @param[in] file_name Library file name.
* @return Shared library handle.
*/
XAMP_BASE_API SharedLibraryHandle LoadSharedLibrary(const std::string_view& file_name);

/*
* OpenSharedLibrary
* 
* Open shared library.
* @param[in] file_name Library file name.
* @return Shared library handle.
*/
XAMP_BASE_API SharedLibraryHandle OpenSharedLibrary(const std::string_view& file_name);

/*
* GetSharedLibraryPath
* 
* Get shared library path.
* @param[in] module Shared library handle.
* @return Shared library path.
*/
XAMP_BASE_API Path GetSharedLibraryPath(const SharedLibraryHandle &module);

/*
* LoadSharedLibrarySymbol
* 
* Load shared library symbol.
* @param[in] dll Shared library handle.
* @param[in] name Symbol name.
*/
XAMP_BASE_API void* LoadSharedLibrarySymbol(const SharedLibraryHandle& dll, const std::string_view & name);

/*
* PrefetchSharedLibrary
* 
* Prefetch shared library.
* @param[in] module Shared library handle.
* @return Prefetch success or not.
*/
XAMP_BASE_API bool PrefetchSharedLibrary(SharedLibraryHandle const& module);

/*
* AddSharedLibrarySearchDirectory
* 
* Add shared library search directory.
* @param[in] path Search directory path.
* @TODO Repeat add search directory will cause crash.
*/
XAMP_BASE_API bool AddSharedLibrarySearchDirectory(const Path &path);

#ifdef XAMP_OS_WIN
/*
* LoadSharedLibrarySymbolEx
* 
* Load shared library symbol with flags.
* @param[in] dll Shared library handle.
* @param[in] name Symbol name.
* @param[in] flags Symbol flags.
* @return Symbol address.
*/
XAMP_BASE_API void* LoadSharedLibrarySymbolEx(SharedLibraryHandle const& dll, const std::string_view name, uint32_t flags);

/*
* PinSystemLibrary
*
* Pin system library.
* @param[in] file_name Library file name.
* @return Shared library handle.
*/
XAMP_BASE_API SharedLibraryHandle PinSystemLibrary(const std::string_view& file_name);
#endif

/*
* SharedLibraryFunction
* 
* A wrapper class for shared library function.
* @param T Function type.
* @param U Must be std::enable_if_t<std::is_function_v<T>>.
*/
template
<
    typename T,
    typename U = std::enable_if_t<std::is_function_v<T>>
>
class XAMP_BASE_API_ONLY_EXPORT SharedLibraryFunction final {
public:
    /*
    * Constructor.
    * 
    * @param dll Shared library handle.
    * @param name Function name.    
    */
    SharedLibraryFunction(SharedLibraryHandle const& dll, const std::string_view name) {
        func_ = static_cast<T*>(LoadSharedLibrarySymbol(dll, name));
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
        func_ = static_cast<T*>(LoadSharedLibrarySymbolEx(dll, name, flags));
    }
#endif
    
    [[nodiscard]] XAMP_ALWAYS_INLINE operator T* () const noexcept {
        XAMP_EXPECTS(func_ != nullptr);
        return func_;
    }

    [[nodiscard]] XAMP_ALWAYS_INLINE bool IsValid() const noexcept {
        return func_ != nullptr;
    }

    [[nodiscard]] XAMP_ALWAYS_INLINE T* Get() const noexcept {
		return func_;
	}

    [[nodiscard]] XAMP_ALWAYS_INLINE T* operator->() const noexcept {
		return func_;
	}

    [[nodiscard]] XAMP_ALWAYS_INLINE T& operator*() const noexcept {
		return *func_;
	}

    XAMP_DISABLE_COPY_AND_MOVE(SharedLibraryFunction)
private:
    T *func_{nullptr};
};

#define XAMP_DECLARE_DLL(Func) SharedLibraryFunction<decltype(Func)>
#define XAMP_DECLARE_DLL_NAME(Func) SharedLibraryFunction<decltype(Func)> Func
#define XAMP_LOAD_DLL_API(Func) Func(module_, #Func)
#define XAMP_LOAD_DLL_API_EX(MemberName, Func) MemberName(module_, #Func)

XAMP_BASE_NAMESPACE_END
