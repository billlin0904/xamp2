//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/windows_handle.h>

namespace xamp::output_device::win32 {

class FastMutex {
public:
	FastMutex() {
		::InitializeCriticalSectionEx(&cs_, 1, 0);
		::SetCriticalSectionSpinCount(&cs_, 0);
	}

	XAMP_DISABLE_COPY(FastMutex)

	~FastMutex() {
		::DeleteCriticalSection(&cs_);
	}

	void lock() {
		::EnterCriticalSection(&cs_);
	}
	
	void unlock() {
		::LeaveCriticalSection(&cs_);
	}

private:
	CRITICAL_SECTION cs_;
};

}

