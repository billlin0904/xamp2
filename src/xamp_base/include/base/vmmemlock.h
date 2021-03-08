//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/align_ptr.h>

namespace xamp::base {

class XAMP_BASE_API VmMemLock final {
public:
	VmMemLock() noexcept;

	VmMemLock(void* address, size_t size);
	
	XAMP_PIMPL(VmMemLock)

	void Lock(void* address, size_t size);

	void UnLock() noexcept;

private:
	class VmMemLockImpl;
	AlignPtr<VmMemLockImpl> impl_;
};

}

