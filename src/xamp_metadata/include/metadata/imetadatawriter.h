//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/fs.h>
#include <base/TrackInfo.h>
#include <metadata/metadata.h>

namespace xamp::metadata {

class XAMP_METADATA_API XAMP_NO_VTABLE IMetadataWriter {
public:
    XAMP_BASE_CLASS(IMetadataWriter)

    [[nodiscard]] virtual bool IsFileReadOnly(Path const & path) const = 0;
    
    virtual void Write(Path const &path, TrackInfo const& trackinfo) = 0;

    virtual void WriteReplayGain(Path const& path, const ReplayGain& replay_gain) = 0;

    virtual void WriteEmbeddedCover(Path const& path, std::vector<uint8_t> const& image) const = 0;

protected:
    IMetadataWriter() = default;
};

}

