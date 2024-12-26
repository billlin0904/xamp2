//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
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

/*
 * Create a metadata reader instance.
*/
XAMP_METADATA_API ScopedPtr<IMetadataReader> MakeMetadataReader();

/*
 * Create a metadata writer instance.
*/
XAMP_METADATA_API ScopedPtr<IMetadataWriter> MakeMetadataWriter();

/*
 * Get the supported file extensions.
*/
XAMP_METADATA_API const HashSet<std::string>& GetSupportFileExtensions();

XAMP_METADATA_API void LoadCueLib();

XAMP_METADATA_API void LoadChromaprintLib();

XAMP_METADATA_NAMESPACE_END

