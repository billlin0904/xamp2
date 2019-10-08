#include <base/dll.h>

namespace xamp::base {

ModuleHandle LoadDll(std::string_view name) {
	auto module = LoadLibraryA(name.data());
	if (!module) {
		throw LoadDllFailureException();
	}
	return ModuleHandle(module);
}

}
