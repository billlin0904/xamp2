//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/fs.h>
#include <base/stl.h>
#include <base/trackinfo.h>

#include <metadata/metadata.h>

XAMP_METADATA_NAMESPACE_BEGIN

/*
* IMetadataReader is a interface for reading metadata from file.
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
    virtual TrackInfo Extract() = 0;

    /*
    * Get ReplayGain from file.
    * 
    * @return ReplayGain
    */
    virtual std::optional<ReplayGain> GetReplayGain() = 0;
 
    /*
    * Get embedded cover from file.
    * 
    * @return std::vector<std::byte>
    */
    virtual std::optional<std::vector<std::byte>> ReadEmbeddedCover() = 0;

    /*
    * Check file is supported.
    * 
    * @return bool
    */
    XAMP_NO_DISCARD virtual bool IsSupported() const = 0;
protected:
    IMetadataReader() = default;
};

XAMP_METADATA_NAMESPACE_END