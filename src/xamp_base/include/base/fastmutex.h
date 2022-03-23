//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <ctime>

#include <mutex>
#include <base/windows_handle.h>
#include <base/exception.h>

namespace xamp::base {

#ifdef XAMP_OS_WIN
class XAMP_BASE_API SRWMutex final {
public:
	SRWMutex() noexcept;

	XAMP_DISABLE_COPY(SRWMutex)

	void lock() noexcept;
	
	void unlock() noexcept;

	[[nodiscard]] bool try_lock() noexcept;

	PSRWLOCK native_handle() {
		return &lock_;
	}
private:
	XAMP_CACHE_ALIGNED(kCacheAlignSize) SRWLOCK lock_;
	uint8_t padding_[kCacheAlignSize - sizeof(lock_)]{ 0 };
};
using FastMutex = SRWMutex;
#else
using FastMutex = std::mutex;
#endif
	
}

