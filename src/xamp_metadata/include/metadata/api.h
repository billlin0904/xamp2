//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <metadata/metadata.h>
#include <metadata/imetadatareader.h>
#include <metadata/imetadatawriter.h>

namespace xamp::metadata {

XAMP_METADATA_API AlignPtr<IMetadataReader> MakeMetadataReader();

XAMP_METADATA_API AlignPtr<IMetadataWriter> MakeMetadataWriter();

}
