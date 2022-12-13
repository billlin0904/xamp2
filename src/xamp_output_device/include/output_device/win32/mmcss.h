//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/output_device.h>
#include <output_device/win32/mmcss_types.h>
#include <base/align_ptr.h>

namespace xamp::output_device::win32 {

class Mmcss final {
public:
	Mmcss();

	XAMP_PIMPL(Mmcss)

	void BoostPriority(std::wstring_view task_name = kMmcssProfileProAudio, MmcssThreadPriority priority = MmcssThreadPriority::MMCSS_THREAD_PRIORITY_NORMAL) noexcept;

	void RevertPriority() noexcept;
private:
	class MmcssImpl;
	AlignPtr<MmcssImpl> impl_;
};

}
#endif

