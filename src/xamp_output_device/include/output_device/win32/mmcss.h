//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <base/align_ptr.h>
#include <output_device/output_device.h>
#include <output_device/win32/mmcss_types.h>

namespace xamp::output_device::win32 {

class Mmcss final {
public:
	Mmcss();

	XAMP_PIMPL(Mmcss)

	static void LoadAvrtLib();	

	void BoostPriority(std::wstring_view task_name = MMCSS_PROFILE_PRO_AUDIO) noexcept;

	void RevertPriority() noexcept;
private:
	class MmcssImpl;
	AlignPtr<MmcssImpl> impl_;
};

}
#endif

