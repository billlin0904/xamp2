//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <base/stl.h>
#include <base/metadata.h>

#include <metadata/metadata.h>

namespace xamp::metadata {

class XAMP_METADATA_API XAMP_NO_VTABLE MetadataReader {
public:
    XAMP_BASE_CLASS(MetadataReader)

    virtual Metadata Extract(Path const &path) = 0;
 
    virtual const std::vector<uint8_t>& ExtractEmbeddedCover(Path const &path) = 0;

    [[nodiscard]] virtual HashSet<std::string> const & GetSupportFileExtensions() const = 0;

    [[nodiscard]] virtual bool IsSupported(Path const & path) const = 0;
protected:
    MetadataReader() = default;
};

XAMP_METADATA_API void WalkPath(Path const & path, MetadataExtractAdapter* adapter, MetadataReader* reader);

}
