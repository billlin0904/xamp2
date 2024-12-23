#include <base/windows_handle.h>

XAMP_BASE_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN

HANDLE HandleTraits::invalid() noexcept {
	return nullptr;
}

void HandleTraits::close(HANDLE value) noexcept {
	::CloseHandle(value);
}

HANDLE FileHandleTraits::invalid() noexcept {
	return INVALID_HANDLE_VALUE;
}

void FileHandleTraits::close(HANDLE value) noexcept {
	::CloseHandle(value);
}

HMODULE ModuleHandleTraits::invalid() noexcept {
	return nullptr;
}

void ModuleHandleTraits::close(HMODULE value) noexcept {
	::FreeLibrary(value);
}

HANDLE MappingFileHandleTraits::invalid() noexcept {
	return nullptr;
}

void MappingFileHandleTraits::close(HANDLE value) noexcept {
	::CloseHandle(value);
}

void* MappingMemoryAddressTraits::invalid() noexcept {
	return nullptr;
}

void MappingMemoryAddressTraits::close(void* value) noexcept {
	::UnmapViewOfFile(value);
}

HANDLE TimerQueueTraits::invalid() noexcept {
	return nullptr;
}

void TimerQueueTraits::close(HANDLE value) noexcept {
	(void) ::DeleteTimerQueueEx(value, INVALID_HANDLE_VALUE);
}

HKEY RegTraits::invalid() noexcept {
	return nullptr;
}

void RegTraits::close(HKEY value) noexcept {
	::RegCloseKey(value);
}

#endif

XAMP_BASE_NAMESPACE_END
