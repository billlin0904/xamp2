//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/trackinfo.h>
#include <metadata/metadata.h>

XAMP_METADATA_NAMESPACE_BEGIN

class XAMP_METADATA_API XAMP_NO_VTABLE IMetadataExtractAdapter {
public:
    XAMP_BASE_CLASS(IMetadataExtractAdapter)

    virtual void OnWalk(Path const &path) = 0;

    virtual void OnWalkEnd(DirectoryEntry const& dir_entry) = 0;

	virtual void OnWalkNew() = 0;

    virtual bool IsAccept(Path const& path) const noexcept = 0;
protected:
    IMetadataExtractAdapter() = default;
};

XAMP_METADATA_NAMESPACE_END
