#include <FastMemcpy_Avx.h>

#include <base/windows_handle.h>
#include <base/memory.h>

namespace xamp::base {

size_t GetPageSize() noexcept {
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return system_info.dwPageSize;
}

size_t GetPageAlignSize(size_t value) noexcept {
	auto align_size = (value + (GetPageSize() - 1)) & ~(GetPageSize() - 1);
	return align_size;
}

__declspec(restrict) void* FastMemcpy(void* dest, const void* src, int32_t size) {
	return memcpy_fast(dest, src, size);
}

}
