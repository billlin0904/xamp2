#include <algorithm>
#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#else
#include <base/posix_handle.h>
#endif
#include <base/memory_mapped_file.h>
#include <base/memory.h>

namespace xamp::base {

#ifdef XAMP_OS_WIN
size_t GetPageSize() noexcept {
	SYSTEM_INFO system_info;
	::GetSystemInfo(&system_info);
	return system_info.dwPageSize;
}

bool PrefetchMemory(void* adddr, size_t length) noexcept {
	_WIN32_MEMORY_RANGE_ENTRY address_range{ adddr, length };
	WinHandle current_process(::GetCurrentProcess());
	return ::PrefetchVirtualMemory(current_process.get(), 1, &address_range, 0);
}
#else
size_t GetPageSize() noexcept {
    return static_cast<size_t>(::getpagesize());
}

bool PrefetchMemory(void* adddr, size_t length) noexcept {
    return ::madvise(adddr, length, MADV_SEQUENTIAL) == 0;
}
#endif

size_t GetPageAlignSize(size_t value) noexcept {
	auto align_size = (value + (GetPageSize() - 1)) & ~(GetPageSize() - 1);
	return align_size;
}

bool PrefactchFile(MemoryMappedFile &file) {
    return PrefetchMemory(const_cast<void*>(file.GetData()), file.GetLength());
}

bool PrefactchFile(std::wstring const & file_name) {
	MemoryMappedFile file;
	file.Open(file_name);
    return PrefactchFile(file);
}

void FastMemcpy(void* dest, const void* src, size_t size) {
#ifdef XAMP_OS_WIN
	//__movsb(static_cast<PBYTE>(dest), static_cast<const BYTE*>(src), size);
	(void) std::memcpy(dest, src, size);
#else
	(void) std::memcpy(dest, src, size);
#endif
}
	
}
