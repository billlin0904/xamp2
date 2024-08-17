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

bool PrefetchFile(MemoryMappedFile &file, size_t prefech_size) {
    const auto preread_file_size = (std::min)(prefech_size, file.GetLength());
    return PrefetchMemory(const_cast<void*>(file.GetData()), preread_file_size);
}

bool PrefetchFile(std::wstring const & file_path) {
#ifndef XAMP_OS_WIN
	MemoryMappedFile file;
	file.Open(file_path);
    return PrefetchFile(file);
#else
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
	Vector<char> buffer(kPrefetchFileReadSize);
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
#endif
}


#ifndef XAMP_OS_WIN
void* AlignedMalloc(size_t size, size_t aligned_size) noexcept {
	void* p = nullptr;
	return ::posix_memalign(&p, aligned_size, size) == 0 ? p : nullptr;
}

void AlignedFree(void* p) noexcept {
	return ::free(p);
}

void* StackAlloc(size_t size) {
	auto ptr = ::alloca(size);
	return ptr;
}

void StackFree(void* p) {
	(void)p;
}

#else
void* AlignedMalloc(size_t size, size_t aligned_size) noexcept {
	XAMP_EXPECTS(IsPowerOfTwo(aligned_size));
	return ::_aligned_malloc(size, aligned_size);
}

void AlignedFree(void* p) noexcept {
	XAMP_EXPECTS(p != nullptr);
	::_aligned_free(p);
}

void* StackAlloc(size_t size) {
	auto ptr = _malloca(size);
	return ptr;
}

void StackFree(void* p) {
	XAMP_EXPECTS(p != nullptr);
	::_freea(p);
}
#endif

XAMP_BASE_NAMESPACE_END
