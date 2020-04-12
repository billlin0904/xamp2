#include <base/dll.h>

namespace xamp::base {

#ifdef XAMP_OS_WIN

ModuleHandle LoadDllSearchPath(std::string_view file_name) {
	auto module = ::LoadLibraryExA(file_name.data(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!module) {
		throw LoadDllFailureException(file_name);
	}
	return ModuleHandle(module);
}

ModuleHandle LoadDll(std::string_view file_name) {
	auto module = ::LoadLibraryA(file_name.data());
	if (!module) {
		throw LoadDllFailureException(file_name);
	}
	return ModuleHandle(module);
}

void* LoadDllSymbol(const ModuleHandle& dll, std::string_view name) noexcept {
    return ::GetProcAddress(dll.get(), name.data());
}
#else
ModuleHandle LoadDll(std::string_view name) {
    auto module = ::dlopen(name.data(), RTLD_LAZY);
    if (!module) {
        throw LoadDllFailureException(name);
    }
    return ModuleHandle(module);
}

void* LoadDllSymbol(const ModuleHandle& dll, std::string_view name) noexcept {
     return ::dlsym(dll.get(), name.data());
}
#endif

}
