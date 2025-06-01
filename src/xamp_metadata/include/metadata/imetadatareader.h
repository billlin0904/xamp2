//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <expected>

#include <base/fs.h>
#include <base/stl.h>
#include <base/trackinfo.h>

#include <metadata/metadata.h>

XAMP_METADATA_NAMESPACE_BEGIN

enum class ParseMetadataError {
    PARSE_ERROR_OPEN_FILE,
    PARSE_ERROR_NOT_FOUND,
    PARSE_ERROR_NOT_SUPPORT,
};

/*
* IMetadataReader is an interface for reading metadata from file.
* 
*/
class XAMP_METADATA_API XAMP_NO_VTABLE IMetadataReader {
public:
    XAMP_BASE_CLASS(IMetadataReader)

    virtual void Open(const Path& path) = 0;

    /*
    * Extract metadata from file.
    * 
    * @return TrackInfo
    */
    virtual std::expected<TrackInfo, ParseMetadataError> Extract() = 0;

    /*
    * Get ReplayGain from file.
    * 
    * @return ReplayGain
    */
    virtual std::expected<ReplayGain, ParseMetadataError> ReadReplayGain() = 0;
 
    /*
    * Get embedded cover from file.
    * 
    * @return std::vector<std::byte>
    */
    virtual std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover() = 0;

    /*
    * Check file is supported.
    * 
    * @return bool
    */
    [[nodiscard]] virtual bool IsSupported() const = 0;
protected:
    IMetadataReader() = default;
};

XAMP_METADATA_NAMESPACE_END