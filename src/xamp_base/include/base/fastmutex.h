//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/memory.h>
#include <base/pimplptr.h>

#include <shared_mutex>

XAMP_BASE_NAMESPACE_BEGIN

#if 0

class XAMP_BASE_API SRWMutex final {
public:
	SRWMutex() noexcept;

	XAMP_PIMPL(SRWMutex)

	void lock() noexcept;
	
	void unlock() noexcept;

	[[nodiscard]] bool try_lock() noexcept;
private:
	class SRWMutexImpl;
	ScopedPtr<SRWMutexImpl> impl_;
};

using FastMutex = SRWMutex;

#else

using FastMutex = std::shared_mutex;

#endif
	
XAMP_BASE_NAMESPACE_END

