#include <base/dll.h>

namespace xamp::base {

#ifdef _WIN32
ModuleHandle LoadDll(std::string_view name) {
	auto module = ::LoadLibraryA(name.data());
	if (!module) {
		throw LoadDllFailureException();
	}
	return ModuleHandle(module);
}

void* LoadDllSymbol(const ModuleHandle& dll, std::string_view name) {
    return ::GetProcAddress(dll.get(), name.data());
}
#else
ModuleHandle LoadDll(std::string_view name) {
    auto module = dlopen(name.data(), RTLD_LAZY);
    if (!module) {
        throw LoadDllFailureException();
    }
    return ModuleHandle(module);
}

void* LoadDllSymbol(const ModuleHandle& dll, std::string_view name) {
     return dlsym(dll.get(), name.data());
}
#endif

}
