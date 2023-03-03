//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>
#include <base/base.h>
#include <base/logger.h>

namespace xamp::base {

class XAMP_BASE_API VmMemLock final {
public:
	VmMemLock() noexcept;

	~VmMemLock() noexcept;

	XAMP_DISABLE_COPY(VmMemLock)

	void Lock(void* address, size_t size);

	void UnLock() noexcept;
	
	VmMemLock& operator=(VmMemLock&& other) noexcept;
private:
	void* address_{ nullptr };
	size_t size_{ 0 };
	LoggerPtr logger_;
};

}

