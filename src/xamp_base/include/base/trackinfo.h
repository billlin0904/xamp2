//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <string>
#include <optional>

#include <base/base.h>

namespace xamp::base {

struct XAMP_BASE_API ReplayGain {
    double track_gain{ 0 };
    double album_gain{ 0 };
    double track_peak{ 0 };
    double album_peak{ 0 };
    double ref_loudness{ 0 };
};

struct XAMP_BASE_API TrackInfo final {
    TrackInfo() noexcept;
    uint32_t track;
    uint32_t bitrate;
    uint32_t samplerate;
    uint32_t year;
    int64_t last_write_time;
    uint64_t file_size;
    size_t parent_path_hash;
	double offset;
    double duration;
    std::string cover_id;
    std::string disc_id;
    std::wstring file_path;
    std::wstring file_name;
    std::wstring file_name_no_ext;
    std::wstring file_ext;
    std::wstring title;
    std::wstring artist;
    std::wstring album;
    std::wstring parent_path;
    std::wstring genre;
    std::wstring comment;
    std::optional<ReplayGain> replay_gain;
};

static constexpr std::string_view kReplaygainAlbumGain{"REPLAYGAIN_ALBUM_GAIN"};
static constexpr std::string_view kReplaygainTrackGain{"REPLAYGAIN_TRACK_GAIN"};
static constexpr std::string_view kReplaygainAlbumPeak{"REPLAYGAIN_ALBUM_PEAK"};
static constexpr std::string_view kReplaygainTrackPeak{"REPLAYGAIN_TRACK_PEAK"};
static constexpr std::string_view kReplaygainReferenceLoudness{"REPLAYGAIN_REFERENCE_LOUDNESS"};

static constexpr std::string_view kITunesReplaygainAlbumGain{ "----:com.apple.iTunes:replaygain_album_gain" };
static constexpr std::string_view kITunesReplaygainTrackGain{ "----:com.apple.iTunes:replaygain_track_gain" };
static constexpr std::string_view kITunesReplaygainAlbumPeak{ "----:com.apple.iTunes:replaygain_album_peak" };
static constexpr std::string_view kITunesReplaygainTrackPeak{ "----:com.apple.iTunes:replaygain_track_peak" };
static constexpr std::string_view kITunesReplaygainReferenceLoudness{ "----:com.apple.iTunes:replaygain_reference_loudness" };

static constexpr double kReferenceLoudness = -23;
static constexpr double kReferenceGain = 84.0;

}