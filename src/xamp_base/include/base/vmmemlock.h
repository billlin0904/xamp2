//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <string>

namespace xamp::base {

class XAMP_BASE_API VmMemLock final {
public:
	explicit VmMemLock(const std::string& name);

	VmMemLock() noexcept;
	
	VmMemLock(void* address, size_t size);

	XAMP_DISABLE_COPY(VmMemLock)

	~VmMemLock() noexcept;

	void Lock(void* address, size_t size);

	void UnLock() noexcept;

	void SetName(const std::string& name) {
		name_ = name;
	}

	std::string GetName() const {
		return name_;
	}

private:
	void* address_;
	size_t size_;
	std::string name_;
};

}

