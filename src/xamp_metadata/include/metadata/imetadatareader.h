//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <base/fs.h>
#include <base/stl.h>
#include <base/TrackInfo.h>

#include <metadata/metadata.h>

namespace xamp::metadata {

class XAMP_METADATA_API XAMP_NO_VTABLE IMetadataReader {
public:
    XAMP_BASE_CLASS(IMetadataReader)

    virtual TrackInfo Extract(Path const &path) = 0;

    virtual std::optional<ReplayGain> GetReplayGain(const Path& path) = 0;
 
    virtual const Vector<uint8_t>& GetEmbeddedCover(Path const &path) = 0;

    [[nodiscard]] virtual HashSet<std::string> const & GetSupportFileExtensions() const = 0;

    [[nodiscard]] virtual bool IsSupported(Path const & path) const = 0;
protected:
    IMetadataReader() = default;
};

}
