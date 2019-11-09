//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/metadata.h>
#include <metadata/metadata.h>

namespace xamp::metadata {

class XAMP_METADATA_API XAMP_NO_VTABLE MetadataWriter {
public:
	XAMP_BASE_CLASS(MetadataWriter)

	virtual bool IsFileReadOnly(const Path& path) const = 0;
    
    virtual void Write(const Path &path, Metadata &metadata) = 0;

protected:
    MetadataWriter() = default;
};

}

