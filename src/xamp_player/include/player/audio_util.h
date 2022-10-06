//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/fs.h>
#include <base/align_ptr.h>
#include <base/stl.h>
#include <base/dsdsampleformat.h>

#include <output_device/api.h>
#include <stream/api.h>
#include <player/player.h>

namespace xamp::player::audio_util {

std::pair<DsdModes, AlignPtr<IAudioStream>> MakeFileStream(Path const &path,
	DeviceInfo const& device_info,
	bool enable_sample_converter = false);

}

