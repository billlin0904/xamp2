//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <expected>

#include <base/fs.h>
#include <base/stl.h>
#include <base/trackinfo.h>
#include <base/archivefile.h>
#include <metadata/metadata.h>

XAMP_METADATA_NAMESPACE_BEGIN

XAMP_MAKE_ENUM(ParseMetadataError,
    PARSE_ERROR_OPEN_FILE,
    PARSE_ERROR_NOT_FOUND,
    PARSE_ERROR_NOT_SUPPORT)

/*
* IMetadataReader is an interface for reading metadata from file.
* 
*/
class XAMP_METADATA_API XAMP_NO_VTABLE IMetadataReader {
public:
    XAMP_BASE_CLASS(IMetadataReader)

    virtual void open(const Path& path) = 0;

    virtual void open(ArchiveEntry archive_entry) = 0;

    /*
    * extract metadata from file.
    * 
    * @return TrackInfo
    */
    virtual std::expected<TrackInfo, ParseMetadataError> extract() = 0;

    /*
    * Get ReplayGain from file.
    * 
    * @return ReplayGain
    */
    virtual std::expected<ReplayGain, ParseMetadataError> readReplayGain() = 0;
 
    /*
    * Get embedded cover from file.
    * 
    * @return std::vector<std::byte>
    */
    virtual std::expected<std::vector<std::byte>, ParseMetadataError> readEmbeddedCover() = 0;

    /*
    * Check file is supported.
    * 
    * @return bool
    */
    [[nodiscard]] virtual bool isSupported() const = 0;
protected:
    IMetadataReader() = default;
};

XAMP_METADATA_NAMESPACE_END