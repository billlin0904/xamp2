// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <base/base.h>

namespace xamp::base {

class XAMP_BASE_API FileCacheCallback {
public:
	XAMP_BASE_CLASS(FileCacheCallback)

	virtual void Write(float const *buf, size_t buf_size) = 0;

	virtual void Close() = 0;
protected:
	FileCacheCallback() = default;
};

}
