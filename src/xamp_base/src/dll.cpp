#include <base/platfrom_handle.h>
#include <base/memory.h>
#include <base/exception.h>
#include <base/memory_mapped_file.h>
#include <base/dll.h>

#include <stdlib.h>

namespace xamp::base {

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
    SharedLibraryHandle temp(module);
    return library;
}

void* LoadSharedLibrarySymbolEx(SharedLibraryHandle const& dll, const std::string_view name, uint32_t flags) {
    auto func = ::GetProcAddress(dll.get(), MAKEINTRESOURCEA(flags));
    if (!func) {
        throw NotFoundDllExportFuncException(name);
    }
    return func;
}

bool AddSharedLibrarySearchDirectory(const Path& path) {
    ::SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);

	const auto utf16_string = path.wstring();

    wchar_t buffer[MAX_PATH];
    wchar_t** part = nullptr;
    ::GetFullPathNameW(utf16_string.c_str(), MAX_PATH, buffer, part);

    if (!::AddDllDirectory(buffer)) {
        if (::GetLastError() != ERROR_FILE_NOT_FOUND) {
            return false;
        }
    }
    return true;
}

SharedLibraryHandle LoadSharedLibrary(const std::string_view& file_name) {
    auto module = ::LoadLibraryExA(file_name.data(),
        nullptr,
        LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	if (!module) {
		throw LoadDllFailureException(file_name);
	}
    SharedLibraryHandle shared_library(module);
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
    const auto path = GetSharedLibraryPath(module);
    MemoryMappedFile file;
    file.Open(path.wstring(), true);
    return PrefetchMemory(const_cast<void*>(file.GetData()), file.GetLength());
}

SharedLibraryHandle OpenSharedLibrary(const std::string_view& file_name) {
    return LoadSharedLibrary(GetSharedLibraryName(file_name));
}

}
