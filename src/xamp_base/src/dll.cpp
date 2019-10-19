#include <base/dll.h>

namespace xamp::base {

#ifdef _WIN32
ModuleHandle LoadDll(std::string_view name) {
	auto module = LoadLibraryA(name.data());
	if (!module) {
		throw LoadDllFailureException();
	}
	return ModuleHandle(module);
}
#else
ModuleHandle LoadDll(std::string_view name) {
    auto module = dlopen(name.data(), RTLD_LAZY);
    if (!module) {
        throw LoadDllFailureException();
    }
    return ModuleHandle(module);
}
#endif

}
