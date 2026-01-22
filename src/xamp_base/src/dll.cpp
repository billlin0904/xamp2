#include <base/platfrom_handle.h>
#include <base/memory.h>
#include <base/exception.h>
#include <base/memory_mapped_file.h>
#include <base/dll.h>
#include <base/stl.h>
#include <base/fastmutex.h>

#include <mutex>

XAMP_BASE_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN

SharedLibraryHandle PinSystemLibrary(const std::string_view& file_name) {
    auto library = LoadSharedLibrary(file_name);
    const auto library_path = GetSharedLibraryPath(library);

    HMODULE module = nullptr;
    if (!::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN,
        library_path.native().c_str(),
        &module)) {
        throw LoadDllFailureException(file_name);
    }    
    return SharedLibraryHandle(module);
}

void* LoadSharedLibrarySymbolEx(SharedLibraryHandle const& dll, const std::string_view name, uint32_t flags) {
	auto func = ::GetProcAddress(dll.get(), MAKEINTRESOURCEA(flags));
    if (!func) {
        throw NotFoundDllExportFuncException(name);
    }
    return func;
}

bool AddSharedLibrarySearchDirectory(const Path& path) {
    static FastMutex thread_safe_lock;
    std::lock_guard<FastMutex> guard{ thread_safe_lock };

    static std::once_flag once;
    static bool dll_dirs_ok = true;
    std::call_once(once, [&] {
        dll_dirs_ok = ::SetDefaultDllDirectories(
            LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
        ) != FALSE;
        });
    if (!dll_dirs_ok) {
        DWORD err = ::GetLastError();
        return false;
    }

    auto normalize_path = NormalizePathToWideString(path);
    if (!normalize_path) {
        return false;
	}

    static HashSet<std::wstring> added;
    if (added.contains(normalize_path.value())) {
        return true;
    }

    if (!::AddDllDirectory(normalize_path.value().c_str())) {
        DWORD err = ::GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
            return false;
        }
    }
	added.insert(normalize_path.value());
    return true;
}

SharedLibraryHandle LoadSharedLibrary(const std::string_view& file_name) {
	const auto module = ::LoadLibraryExA(file_name.data(),
	                                     nullptr,
	                                     LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	if (!module) {
		throw LoadDllFailureException(file_name);
	}
    SharedLibraryHandle shared_library(module);
    if (!shared_library) {
		throw LoadDllFailureException(file_name);
	}
    PrefetchSharedLibrary(shared_library);
	return shared_library;
}

Path GetSharedLibraryPath(const SharedLibraryHandle& module) {
    char path[MAX_PATH]{ 0 };
    ::GetModuleFileNameA(module.get(), path, MAX_PATH - 1);
    return path;
}

void* LoadSharedLibrarySymbol(const SharedLibraryHandle& dll, const std::string_view& name) {
    auto func = ::GetProcAddress(dll.get(), name.data());
    if (!func) {
        throw NotFoundDllExportFuncException(name);
    }
    return func;
}
#else

Path GetSharedLibraryPath(const SharedLibraryHandle& module) {
    Dl_info info{};
    ::dladdr(module.get(), &info);
    return info.dli_fname;
}

SharedLibraryHandle LoadSharedLibrary(const std::string_view& name) {
    auto path = GetComponentsFilePath() / name;
    auto path_string = path.native();
    auto module = ::dlopen(path_string.c_str(), RTLD_NOW);
    if (!module) {
        throw LoadDllFailureException(name);
    }
    return SharedLibraryHandle(module);
}

void* LoadSharedLibrarySymbol(const SharedLibraryHandle& dll, const std::string_view& name) {
     auto func = ::dlsym(dll.get(), name.data());
     if (!func) {
         throw NotFoundDllExportFuncException(name);
     }
     return func;
}
#endif

bool PrefetchSharedLibrary(SharedLibraryHandle const& module) {
    if (!module) {
        return false;
    }
    const auto path = GetSharedLibraryPath(module);
    MemoryMappedFile file_;
    if (file_.Open(path.wstring(), true)) {
        return PrefetchMemory(const_cast<void*>(file_.GetData()), file_.GetLength());
    }    
    return false;
}

SharedLibraryHandle OpenSharedLibrary(const std::string_view& file_name) {
    return LoadSharedLibrary(GetSharedLibraryName(file_name));
}

XAMP_BASE_NAMESPACE_END
