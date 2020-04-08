//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <base/align_ptr.h>

#include <output_device/win32/mmcss_types.h>
#include <output_device/output_device.h>

namespace xamp::output_device::win32 {

using namespace base;

class Mmcss final {
public:
	Mmcss();

	~Mmcss();

	void Initial();

	XAMP_DISABLE_COPY(Mmcss)

	void BoostPriority(std::wstring_view task_name = MMCSS_PROFILE_PRO_AUDIO);

	void RevertPriority();
private:
	class MmcssImpl;
	align_ptr<MmcssImpl> impl_;
};

}
#endif

