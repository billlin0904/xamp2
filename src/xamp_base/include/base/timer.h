//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>
#include <functional>

#include <base/base.h>
#include <base/align_ptr.h>

namespace xamp::base {

class XAMP_BASE_API Timer final {
public:
	Timer();

	XAMP_PIMPL(Timer)

	void Start(std::chrono::milliseconds interval, std::function<void()> callback);

	void Stop();

	bool IsStarted() const;

private:
	class TimerImpl;
	AlignPtr<TimerImpl> impl_;
};

}
