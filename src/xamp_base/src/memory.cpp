#include <base/memory.h>

#include <base/base.h>
#include <base/simd.h>
#include <base/align_ptr.h>
#include <base/platfrom_handle.h>
#include <base/memory_mapped_file.h>

#include <algorithm>

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
	const auto page_size = GetPageSize();
	return AlignUp(value, page_size);
}

bool PrefetchFile(MemoryMappedFile &file, size_t prefech_size) {
    const auto preread_file_size = (std::min)(prefech_size, file.GetLength());
    return PrefetchMemory(const_cast<void*>(file.GetData()), preread_file_size);
}

bool PrefetchFile(std::wstring const & file_name) {
	MemoryMappedFile file;
	file.Open(file_name);
    return PrefetchFile(file);
}

#ifdef XAMP_ENABLE_REP_MOVSB
void MemorySet(void* dest, int32_t c, size_t size) noexcept {
	static constexpr size_t kRepStosbThresShold = 2048;

	if (SIMD::IsAligned(dest) && size >= kRepStosbThresShold) {
		std::memset(dest, c, size);
	} else {
		__stosb(static_cast<unsigned char*>(dest), static_cast<unsigned char>(c), size);
	}
}
#endif

#ifdef XAMP_ENABLE_REP_MOVSB
void MemoryCopy(void* dest, const void* src, size_t size) noexcept {
	static constexpr size_t kRepMovsbThresShold = 16384;

	if (SIMD::IsAligned(dest) && size >= kRepMovsbThresShold) {
		std::memcpy(dest, src, size);
	}
	else {
		__movsb(static_cast<unsigned char*>(dest), static_cast<const unsigned char*>(src), size);
	}
}
#endif

}
