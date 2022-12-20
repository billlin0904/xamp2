#include <base/platfrom_handle.h>
#include <base/memory.h>
#include <base/exception.h>
#include <base/memory_mapped_file.h>
#include <base/dll.h>

namespace xamp::base {

#ifdef XAMP_OS_WIN
ModuleHandle LoadModule(const std::string_view& file_name) {
	auto module = ::LoadLibraryA(file_name.data());
	if (!module) {
		throw LoadDllFailureException(file_name);
	}
	return ModuleHandle(module);
}

Path GetModulePath(const ModuleHandle& module) {
    char path[MAX_PATH]{ 0 };
    ::GetModuleFileNameA(module.get(), path, MAX_PATH - 1);
    return path;
}

void* LoadModuleSymbol(const ModuleHandle& dll, const std::string_view& name) {
    auto func = ::GetProcAddress(dll.get(), name.data());
    if (!func) {
        throw NotFoundDllExportFuncException(name);
    }
    return func;
}

bool PrefetchModule(ModuleHandle const& module) {
    const auto path = GetModulePath(module);
    MemoryMappedFile file;
    file.Open(path.wstring(), true);
    return PrefetchMemory(const_cast<void*>(file.GetData()), file.GetLength());
}
#else
Path GetModulePath(const ModuleHandle& module) {
    return "";
}

ModuleHandle OpenSharedLibrary(const std::string_view& file_name) {
    return LoadModule(GetSharedLibraryName(file_name));
}

ModuleHandle LoadModule(const std::string_view& name) {
    auto module = ::dlopen(name.data(), RTLD_NOW);
    if (!module) {
        throw LoadDllFailureException(name);
    }
    return ModuleHandle(module);
}

void* LoadModuleSymbol(const ModuleHandle& dll, const std::string_view& name) {
     auto func = ::dlsym(dll.get(), name.data());
     if (!func) {
         throw NotFoundDllExportFuncException(name);
     }
     return func;
}

bool PrefetchModule(ModuleHandle const& module) {
    return true;
}
#endif

}
