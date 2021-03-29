//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>

#include <base/align_ptr.h>
#include <base/stl.h>
#include <base/dsdsampleformat.h>
#include <output_device/dsddevice.h>
#include <stream/stream_util.h>
#include <player/player.h>

namespace xamp::player::audio_util {

using namespace xamp::base;
using namespace xamp::stream;
using namespace xamp::output_device;

XAMP_PLAYER_API DsdDevice * AsDsdDevice(AlignPtr<Device> const & device) noexcept;
	
XAMP_PLAYER_API std::pair<DsdModes, AlignPtr<FileStream>> MakeFileStream(std::wstring const& file_path,
	std::wstring const& file_ext,
	DeviceInfo const& device_info);

}

