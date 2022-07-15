#include <base/windows_handle.h>

namespace xamp::base {

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

PTP_WORK WorkTraits::invalid() noexcept {
	return nullptr;
}

void WorkTraits::close(PTP_WORK value) {
	::CloseThreadpoolWork(value);
}

PTP_POOL ThreadPoolTraits::invalid() noexcept {
	return nullptr;
}

void ThreadPoolTraits::close(PTP_POOL value) {
	::CloseThreadpool(value);
}

PTP_CLEANUP_GROUP CleanupThreadGroupTraits::invalid() noexcept {
	return nullptr;
}

void CleanupThreadGroupTraits::close(PTP_CLEANUP_GROUP value) {
	::CloseThreadpoolCleanupGroup(value);
}

HANDLE TimerQueueTraits::invalid() noexcept {
	return nullptr;
}

void TimerQueueTraits::close(HANDLE value) {
	::DeleteTimerQueueEx(value, INVALID_HANDLE_VALUE);
}

}