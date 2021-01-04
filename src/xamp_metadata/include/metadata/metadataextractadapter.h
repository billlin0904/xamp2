//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/metadata.h>
#include <metadata/metadata.h>

namespace xamp::metadata {

class XAMP_METADATA_API XAMP_NO_VTABLE MetadataExtractAdapter {
public:
    virtual ~MetadataExtractAdapter() = default;

    virtual void OnWalk(Path const &path, Metadata metadata) = 0;

    virtual void OnWalkNext() = 0;

	virtual void OnWalkFirst() = 0;

    [[nodiscard]] virtual bool IsCancel() const noexcept = 0;

    virtual void Cancel() = 0;

    virtual void Reset() = 0;
protected:
    MetadataExtractAdapter() = default;
};

}
