//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/align_ptr.h>
#include <base/pimplptr.h>

XAMP_BASE_NAMESPACE_BEGIN

class XAMP_BASE_API SRWMutex final {
public:
	SRWMutex() noexcept;

	XAMP_PIMPL(SRWMutex)

	void lock() noexcept;
	
	void unlock() noexcept;

	[[nodiscard]] bool try_lock() noexcept;
private:
	class SRWMutexImpl;
	PimplPtr<SRWMutexImpl> impl_;
};

using FastMutex = SRWMutex;
	
XAMP_BASE_NAMESPACE_END

