//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/metadata.h>
#include <metadata/metadata.h>

namespace xamp::metadata {

class MetadataReader;

class XAMP_METADATA_API XAMP_NO_VTABLE MetadataExtractAdapter {
public:
    XAMP_BASE_CLASS(MetadataExtractAdapter)

    virtual void OnWalk(const Path &path, Metadata metadata) = 0;

    virtual void OnWalkNext() = 0;

    virtual bool IsCancel() const = 0;

    virtual void Cancel() = 0;

    virtual void Reset() = 0;
protected:
    MetadataExtractAdapter() = default;
};

}
