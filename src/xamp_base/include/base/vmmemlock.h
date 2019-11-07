//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

namespace xamp::base {

class XAMP_BASE_API VmMemLock final {
public:
	VmMemLock() noexcept;

	XAMP_DISABLE_COPY(VmMemLock)

	~VmMemLock() noexcept;

	void Lock(void* address, size_t size) noexcept;

	void UnLock() noexcept;
#ifdef _WIN32
	static bool EnableLockMemPrivilege(bool enable) noexcept;
#endif
private:
	void* address_;
	size_t size_;
};

}

