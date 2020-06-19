// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <base/dll.h>

namespace xamp::base {

#ifdef XAMP_OS_WIN
ModuleHandle LoadModule(std::string_view file_name) {
	auto module = ::LoadLibraryA(file_name.data());
	if (!module) {
		throw LoadDllFailureException(file_name);
	}
	return ModuleHandle(module);
}

void* LoadModuleSymbol(const ModuleHandle& dll, uint64_t addr) {
    auto func = ::GetProcAddress(dll.get(), (LPCSTR)addr);
    if (!func) {
        throw NotFoundDllExportFuncException();
    }
    return func;
}

void* LoadModuleSymbol(const ModuleHandle& dll, std::string_view name) {
    auto func = ::GetProcAddress(dll.get(), name.data());
    if (!func) {
        throw NotFoundDllExportFuncException();
    }
    return func;
}
#else
ModuleHandle LoadModule(std::string_view name) {
    auto module = ::dlopen(name.data(), RTLD_NOW);
    if (!module) {
        throw LoadDllFailureException(name);
    }
    return ModuleHandle(module);
}

void* LoadModuleSymbol(const ModuleHandle& dll, std::string_view name) {
     auto func = ::dlsym(dll.get(), name.data());
     if (!func) {
         throw NotFoundDllExportFuncException();
     }
     return func;
}
#endif

}
