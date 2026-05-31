//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/output_device.h>

#include <string_view>

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

inline constexpr int kDefaultRealtimePriorityBoost = 10;

XAMP_OUTPUT_DEVICE_API bool SetRealtimeThreadPriority(std::string_view thread_name,
	int priority_boost = kDefaultRealtimePriorityBoost);

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
