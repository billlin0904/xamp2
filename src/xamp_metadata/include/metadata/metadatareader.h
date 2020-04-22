//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <base/stl.h>
#include <base/metadata.h>

#include <metadata/metadata.h>
#include <metadata/metadataextractadapter.h>

namespace xamp::metadata {

class XAMP_METADATA_API XAMP_NO_VTABLE MetadataReader {
public:
    virtual ~MetadataReader() = default;

    virtual Metadata Extract(const Path &path) = 0;
 
    virtual const std::vector<uint8_t>& ExtractEmbeddedCover(const Path &path) = 0;

    virtual const RobinHoodSet<std::string> & GetSupportFileExtensions() const = 0;

    virtual bool IsSupported(const Path & path) const = 0;
protected:
    MetadataReader() = default;
};

XAMP_METADATA_API void FromPath(const Path& path, MetadataExtractAdapter* adapter, MetadataReader* reader);

}
