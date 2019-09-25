#include <base/dll.h>

namespace xamp::base {

ModuleHandle LoadDll(const char* name) {
	auto module = LoadLibraryA(name);
	if (!module) {
		throw LoadDllFailureException();
	}
	return ModuleHandle(module);
}

}
