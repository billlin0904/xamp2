#include <algorithm>
#include <base/base.h>

#include <base/align_ptr.h>

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
	const WinHandle current_process(::GetCurrentProcess());
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
	const auto align_size = GetPageSize();
	return AlignUp(value, align_size);
}

bool PrefetchFile(MemoryMappedFile &file) {
    return PrefetchMemory(const_cast<void*>(file.GetData()), file.GetLength());
}

bool PrefetchFile(std::wstring const & file_name) {
	MemoryMappedFile file;
	file.Open(file_name);
    return PrefetchFile(file);
}
	
}
