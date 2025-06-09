#include <base/windows_handle.h>

XAMP_BASE_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN

HANDLE HandleTraits::invalid() noexcept {
	return nullptr;
}

void HandleTraits::Close(HANDLE value) noexcept {
	::CloseHandle(value);
}

HANDLE FileHandleTraits::invalid() noexcept {
	return INVALID_HANDLE_VALUE;
}

void FileHandleTraits::Close(HANDLE value) noexcept {
	::CloseHandle(value);
}

HMODULE ModuleHandleTraits::invalid() noexcept {
	return nullptr;
}

void ModuleHandleTraits::Close(HMODULE value) noexcept {
	::FreeLibrary(value);
}

HANDLE MappingFileHandleTraits::invalid() noexcept {
	return nullptr;
}

void MappingFileHandleTraits::Close(HANDLE value) noexcept {
	::CloseHandle(value);
}

void* MappingMemoryAddressTraits::invalid() noexcept {
	return nullptr;
}

void MappingMemoryAddressTraits::Close(void* value) noexcept {
	::UnmapViewOfFile(value);
}

HANDLE TimerQueueTraits::invalid() noexcept {
	return nullptr;
}

void TimerQueueTraits::Close(HANDLE value) noexcept {
	(void) ::DeleteTimerQueueEx(value, INVALID_HANDLE_VALUE);
}

HKEY RegTraits::invalid() noexcept {
	return nullptr;
}

void RegTraits::Close(HKEY value) noexcept {
	::RegCloseKey(value);
}

#endif

XAMP_BASE_NAMESPACE_END
