//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <output_device/posix/thread_priority.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <pthread.h>
#include <sched.h>

#include <base/logger.h>

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

bool setRealtimeThreadPriority(std::string_view thread_name, int priority_boost) {
#ifdef SCHED_RR
	errno = 0;
	const auto min_priority = ::sched_get_priority_min(SCHED_RR);
	if (min_priority < 0) {
		XAMP_LOG_WARN(
			"{} realtime priority query failed: {}.",
			thread_name,
			std::strerror(errno));
		return false;
	}

	errno = 0;
	const auto max_priority = ::sched_get_priority_max(SCHED_RR);
	if (max_priority < 0) {
		XAMP_LOG_WARN(
			"{} realtime priority query failed: {}.",
			thread_name,
			std::strerror(errno));
		return false;
	}

	sched_param param{};
	param.sched_priority = std::clamp(min_priority + priority_boost,
		min_priority,
		max_priority);

	const auto error = ::pthread_setschedparam(::pthread_self(), SCHED_RR, &param);
	if (error == 0) {
		XAMP_LOG_DEBUG(
			"{} uses SCHED_RR priority:{}.",
			thread_name,
			param.sched_priority);
		return true;
	}

	if (error == EPERM) {
		XAMP_LOG_DEBUG(
			"{} uses normal scheduling; realtime priority requires CAP_SYS_NICE or rtprio.",
			thread_name);
		return false;
	}

	XAMP_LOG_WARN(
		"{} realtime priority unavailable: {}. Grant CAP_SYS_NICE or rtprio to enable it.",
		thread_name,
		std::strerror(error));
	return false;
#else
	XAMP_LOG_WARN(
		"{} realtime scheduling is not supported on this platform.",
		thread_name);
	return false;
#endif
}

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
