//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>
#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

class Logger;

class XAMP_BASE_API VmMemLock final {
public:
	VmMemLock() ;

	~VmMemLock() ;

	XAMP_DISABLE_COPY(VmMemLock)

	void Lock(void* address, size_t size);

	void UnLock() ;
	
	VmMemLock& operator=(VmMemLock&& other) ;
private:
	void* address_{ nullptr };
	size_t size_{ 0 };
	std::shared_ptr<Logger> logger_;
};

XAMP_BASE_NAMESPACE_END

