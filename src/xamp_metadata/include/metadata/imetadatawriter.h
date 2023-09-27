//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/fs.h>
#include <base/trackinfo.h>
#include <metadata/metadata.h>

XAMP_METADATA_NAMESPACE_BEGIN

/*
* IMetadataWriter is a interface for writing metadata to file.
* 
*/
class XAMP_METADATA_API XAMP_NO_VTABLE IMetadataWriter {
public:
    XAMP_BASE_CLASS(IMetadataWriter)

    /*
    * Check file is read only.
    * 
    * @param[in] path file path.
    * @return bool
    */
    [[nodiscard]] virtual bool IsFileReadOnly(Path const & path) const = 0;
    
    /*
    * Write metadata to file.
    * 
    * @param[in] path file path.
    * @param[in] track_info track info.
    */
    virtual void Write(Path const &path, TrackInfo const& track_info) = 0;

    /*
    * Write ReplayGain to file.
    * 
    * @param[in] path file path.
    * @param[in] replay_gain replay gain.
    */
    virtual void WriteReplayGain(Path const& path, const ReplayGain& replay_gain) = 0;

    /*
    * Write embedded cover to file.
    * 
    * @param[in] path file path.
    * @param[in] image image data.
    */
    virtual void WriteEmbeddedCover(Path const& path, std::vector<uint8_t> const& image) const = 0;

protected:
    IMetadataWriter() = default;
};

XAMP_METADATA_NAMESPACE_END
