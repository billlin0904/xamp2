//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <string>
#include <optional>

#include <base/base.h>
#include <base/fs.h>

XAMP_BASE_NAMESPACE_BEGIN

struct XAMP_BASE_API ReplayGain {
    double track_gain{ 0 };
    double album_gain{ 0 };
    double track_peak{ 0 };
    double album_peak{ 0 };
    double ref_loudness{ 0 };
    double track_loudness{ 0 };
};

inline constexpr int32_t kMaxTrackNumber = 99;

struct XAMP_BASE_API TrackInfo final {
    TrackInfo() noexcept;

    uint32_t is_cue_file;
    uint32_t rating;
    uint32_t track;
    uint32_t bit_rate;
    uint32_t sample_rate;
    uint64_t year;
    uint64_t file_size;
    int64_t last_write_time;    
    double duration;
    Path file_path;

    std::optional<double>       offset;
    std::optional<std::string>  cover_id;
    std::optional<std::string>  disc_id;
    std::optional<std::wstring> title;
    std::optional<std::wstring> artist;
    std::optional<std::wstring> album;
    std::optional<std::wstring> genre;
    std::optional<std::wstring> comment;
    std::optional<ReplayGain>   replay_gain;

    std::optional<std::wstring> file_name() const {
        if (!file_path.has_filename()) {
            return std::nullopt;
        }
        return file_path.filename().wstring();
    }

    std::optional<std::wstring> parent_path() const {
        if (!file_path.has_parent_path()) {
            return std::nullopt;
        }
        return file_path.parent_path().wstring();
    }

    std::optional<std::wstring> file_ext() const {
        if (!file_path.has_extension()) {
            return std::nullopt;
        }
        return file_path.extension().wstring();
    }

    std::optional<std::wstring> file_name_no_ext() const {
        if (!file_path.has_stem()) {
            return std::nullopt;
        }
        return file_path.stem().wstring();
    }
};

static const std::string kReplaygainAlbumGain{"REPLAYGAIN_ALBUM_GAIN"};
static const std::string kReplaygainTrackGain{"REPLAYGAIN_TRACK_GAIN"};
static const std::string kReplaygainAlbumPeak{"REPLAYGAIN_ALBUM_PEAK"};
static const std::string kReplaygainTrackPeak{"REPLAYGAIN_TRACK_PEAK"};
static const std::string kReplaygainReferenceLoudness{"REPLAYGAIN_REFERENCE_LOUDNESS"};

static const std::string kITunesReplaygainAlbumGain{ "----:com.apple.iTunes:replaygain_album_gain" };
static const std::string kITunesReplaygainTrackGain{ "----:com.apple.iTunes:replaygain_track_gain" };
static const std::string kITunesReplaygainAlbumPeak{ "----:com.apple.iTunes:replaygain_album_peak" };
static const std::string kITunesReplaygainTrackPeak{ "----:com.apple.iTunes:replaygain_track_peak" };
static const std::string kITunesReplaygainReferenceLoudness{ "----:com.apple.iTunes:replaygain_reference_loudness" };

static constexpr double kReferenceLoudness = -23;
static constexpr double kReferenceGain = 84.0;

XAMP_BASE_NAMESPACE_END
