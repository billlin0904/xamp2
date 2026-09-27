#include <base/memory.h>

#include <base/base.h>
#include <base/assert.h>
#include <base/math.h>
#include <base/stl.h>
#include <base/platfrom_handle.h>
#include <base/memory_mapped_file.h>

#ifdef XAMP_OS_WIN
#include <malloc.h>
#endif

#include <algorithm>

XAMP_BASE_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN
size_t getPageSize() {
	SYSTEM_INFO system_info;
	::GetSystemInfo(&system_info);
	return system_info.dwPageSize;
}

bool prefetchMemory(void* addr, size_t length) {
	XAMP_EXPECTS(addr != nullptr);
	XAMP_EXPECTS(length > 0);	

	_WIN32_MEMORY_RANGE_ENTRY address_range{ addr, length };
	const WinHandle current_process(::GetCurrentProcess());
	if (::PrefetchVirtualMemory(current_process.get(), 1, &address_range, 0)) {
		/*volatile uint8_t dummy = 0;
		auto page_size = GetPageSize();
		const uint8_t* base = static_cast<const uint8_t*>(addr);
		for (size_t i = 0; i < length; i += page_size) {
			dummy ^= base[i];
		}*/
		return true;
	}
	return false;
}
#else
size_t getPageSize() {
    return static_cast<size_t>(::getpagesize());
}

bool prefetchMemory(void* adddr, size_t length) {
    return ::madvise(adddr, length, MADV_SEQUENTIAL) == 0;
}
#endif

bool prefetchFile(MemoryMappedFile &file_, size_t prefech_size) {
    const auto prefetch_file_size = (std::min)(prefech_size, file_.length());
	if (prefetchMemory(const_cast<void*>(file_.data()), prefetch_file_size)) {		
		return true;
	}
	return false;
}

bool prefetchFile(std::wstring const & file_path) {
#if 0
	FileHandle file(::CreateFileW(file_path.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_SEQUENTIAL_SCAN,
		nullptr));
	if (!file) {
		return false;
	}

	constexpr DWORD kPrefetchFileReadSize = 4096;
	std::vector<char> buffer(kPrefetchFileReadSize);
	DWORD readbytes = 0;
	uint64_t total = 0;

	while (::ReadFile(file.get(), buffer.data(), kPrefetchFileReadSize, &readbytes, nullptr)) {
		if (!readbytes) {
			break;
		}
		total += readbytes;
		if (total >= kMaxPreReadFileSize) {
			break;
		}
	}
	return true;
#else
	MemoryMappedFile file_;
	if (file_.open(file_path)) {
		return prefetchFile(file_);
	}
	return false;
#endif
}


#ifndef XAMP_OS_WIN
void* alignedMalloc(size_t size, size_t aligned_size) {
	void* p = nullptr;
	return ::posix_memalign(&p, aligned_size, size) == 0 ? p : nullptr;
}

void alignedFree(void* p) {
	return ::free(p);
}

void stackFree(void* p) {
	(void)p;
}

#else
void* alignedMalloc(size_t size, size_t aligned_size) {
	XAMP_EXPECTS(isPowerOfTwo(aligned_size));
	return ::_aligned_malloc(size, aligned_size);
}

void alignedFree(void* p) {
	XAMP_EXPECTS(p != nullptr);
	::_aligned_free(p);
}

void stackFree(void* p) {
	XAMP_EXPECTS(p != nullptr);
	::_freea(p);
}
#endif

XAMP_BASE_NAMESPACE_END
