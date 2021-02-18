//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>

#include <base/align_ptr.h>
#include <base/stl.h>
#include <output_device/dsddevice.h>
#include <stream/filestream.h>
#include <player/player.h>

namespace xamp::player::audio_util {

using namespace xamp::base;
using namespace xamp::stream;
using namespace xamp::output_device;

XAMP_PLAYER_API bool TestDsdFileFormatStd(std::wstring const& file_path);

XAMP_PLAYER_API bool TestDsdFileFormat(std::wstring const& file_path);

XAMP_PLAYER_API HashSet<std::string> const & GetSupportFileExtensions();

XAMP_PLAYER_API DsdStream* AsDsdStream(AlignPtr<FileStream> const& stream) noexcept;

XAMP_PLAYER_API DsdDevice* AsDsdDevice(AlignPtr<Device> const& device) noexcept;

XAMP_PLAYER_API AlignPtr<FileStream> MakeStream(std::wstring const& file_ext, AlignPtr<FileStream> old_stream = nullptr);

XAMP_PLAYER_API std::pair<DsdModes, AlignPtr<FileStream>> MakeFileStream(std::wstring const& file_path,
	std::wstring const& file_ext,
	DeviceInfo const& device_info);

}

