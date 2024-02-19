//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <functional>
#include <base/memory.h>
#include <base/fs.h>
#include <metadata/metadata.h>
#include <metadata/imetadatareader.h>
#include <metadata/imetadatawriter.h>

XAMP_METADATA_NAMESPACE_BEGIN

XAMP_METADATA_API AlignPtr<IMetadataReader> MakeMetadataReader();

XAMP_METADATA_API AlignPtr<IMetadataWriter> MakeMetadataWriter();

XAMP_METADATA_API const HashSet<std::string>& GetSupportFileExtensions();

XAMP_METADATA_NAMESPACE_END

