//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/metadata.h>
#include <metadata/metadata.h>

namespace xamp::metadata {

class XAMP_METADATA_API XAMP_NO_VTABLE IMetadataWriter {
public:
    XAMP_BASE_CLASS(IMetadataWriter)

    [[nodiscard]] virtual bool IsFileReadOnly(Path const & path) const = 0;
    
    virtual void Write(Path const &path, Metadata const& metadata) = 0;

    virtual void WriteEmbeddedCover(Path const& path, std::vector<uint8_t> const& image) const = 0;

protected:
    IMetadataWriter() = default;
};

}

