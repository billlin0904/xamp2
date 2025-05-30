//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>
#include <functional>

#include <base/base.h>
#include <base/memory.h>
#include <base/pimplptr.h>

XAMP_BASE_NAMESPACE_BEGIN

class XAMP_BASE_API Timer final {
public:
	Timer();

	XAMP_PIMPL(Timer)

	void Start(std::chrono::milliseconds interval, std::function<void()> callback);

	void Stop();

	bool IsStarted() const;

private:
	class TimerImpl;
	ScopedPtr<TimerImpl> impl_;
};

XAMP_BASE_NAMESPACE_END
