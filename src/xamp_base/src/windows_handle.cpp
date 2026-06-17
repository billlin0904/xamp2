#include <base/windows_handle.h>

XAMP_BASE_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN

HANDLE HandleTraits::invalid() {
	return nullptr;
}

void HandleTraits::close(HANDLE value) {
	::CloseHandle(value);
}

HANDLE FileHandleTraits::invalid() {
	return INVALID_HANDLE_VALUE;
}

void FileHandleTraits::close(HANDLE value) {
	::CloseHandle(value);
}

HMODULE ModuleHandleTraits::invalid() {
	return nullptr;
}

void ModuleHandleTraits::close(HMODULE value) {
	::FreeLibrary(value);
}

HANDLE MappingFileHandleTraits::invalid() {
	return nullptr;
}

void MappingFileHandleTraits::close(HANDLE value) {
	::CloseHandle(value);
}

void* MappingMemoryAddressTraits::invalid() {
	return nullptr;
}

void MappingMemoryAddressTraits::close(void* value) {
	::UnmapViewOfFile(value);
}

HANDLE TimerQueueTraits::invalid() {
	return nullptr;
}

void TimerQueueTraits::close(HANDLE value) {
	(void) ::DeleteTimerQueueEx(value, INVALID_HANDLE_VALUE);
}

HKEY RegTraits::invalid() {
	return nullptr;
}

void RegTraits::close(HKEY value) {
	::RegCloseKey(value);
}

#endif

XAMP_BASE_NAMESPACE_END
