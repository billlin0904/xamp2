//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
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

    /*
    * Extract metadata from file.
    * 
    * @param[in] path file path.
    * @return TrackInfo
    */
    virtual TrackInfo Extract(const Path &path) = 0;

    /*
    * Get ReplayGain from file.
    * 
    * @param[in] path file path.
    * @return ReplayGain
    */
    virtual std::optional<ReplayGain> GetReplayGain(const Path& path) = 0;
 
    /*
    * Get embedded cover from file.
    * 
    * @param[in] path file path.
    * @return Vector<uint8_t>
    */
    virtual const Vector<uint8_t>& ReadEmbeddedCover(const Path &path) = 0;

    /*
    * Check file is supported.
    * 
    * @param[in] path file path.
    * @return bool
    */
    XAMP_NO_DISCARD virtual bool IsSupported(const Path & path) const = 0;
protected:
    IMetadataReader() = default;
};

XAMP_METADATA_NAMESPACE_END