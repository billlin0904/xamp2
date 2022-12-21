#include <base/platfrom_handle.h>
#include <base/memory.h>
#include <base/exception.h>
#include <base/memory_mapped_file.h>
#include <base/dll.h>

namespace xamp::base {

#ifdef XAMP_OS_WIN
SharedLibraryHandle LoadSharedLibrary(const std::string_view& file_name) {
	auto module = ::LoadLibraryA(file_name.data());
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
    return "";
}

SharedLibraryHandle LoadSharedLibrary(const std::string_view& name) {
    auto module = ::dlopen(name.data(), RTLD_NOW);
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
