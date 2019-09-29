#include <FastMemcpy_Avx.h>

#ifdef _WIN32
#include <base/windows_handle.h>
#endif

#include <base/memory_mapped_file.h>
#include <base/memory.h>

namespace xamp::base {

#ifdef _WIN32
size_t GetPageSize() noexcept {
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return system_info.dwPageSize;
}

bool PrefetchMemory(void* adddr, size_t length) noexcept {
	_WIN32_MEMORY_RANGE_ENTRY address_range{ adddr, length };
	WinHandle current_process(GetCurrentProcess());
	return PrefetchVirtualMemory(current_process.get(), 1, &address_range, 0);
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

XAMP_RESTRICT void* FastMemcpy(void* dest, const void* src, int32_t size) noexcept {
	return memcpy_fast(dest, src, size);
}

}
