//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include <base/align_ptr.h>
#include <base/stl.h>
#include <base/dsdsampleformat.h>
#include <output_device/idsddevice.h>
#include <stream/api.h>
#include <player/player.h>

namespace xamp::player::audio_util {

using namespace xamp::base;
using namespace xamp::stream;
using namespace xamp::output_device;

std::pair<DsdModes, AlignPtr<FileStream>> MakeFileStream(Path const &path,	
	DeviceInfo const& device_info,
	bool enable_sample_converter = false);

}

