//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <output_device/posix/pipewire_private.h>

#include <mutex>

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

void ensurePipeWireInitialized() {
	static std::once_flag pipewire_init_once;
	std::call_once(pipewire_init_once, [] {
		::pw_init(nullptr, nullptr);
	});
}

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
