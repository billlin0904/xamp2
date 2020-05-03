#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#else
#include <base/posix_handle.h>
#endif

#include <algorithm>
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

void PrefactchFile(const std::wstring& file_name) {
	MemoryMappedFile file;
	file.Open(file_name);
	PrefetchMemory(const_cast<void*>(file.GetData()), file.GetLength());
}

#if 0
XAMP_RESTRICT XAMP_NOALIAS void* FastMemcpy(void* dest, const void* src, size_t size) noexcept {
	return std::memcpy(dest, src, size);
}
#endif

}
