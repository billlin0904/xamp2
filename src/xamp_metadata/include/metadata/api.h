//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
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
 * create a metadata reader instance.
*/
XAMP_METADATA_API ScopedPtr<IMetadataReader> makeMetadataReader();

/*
 * create a metadata writer instance.
*/
XAMP_METADATA_API ScopedPtr<IMetadataWriter> makeMetadataWriter();

/*
 * Get the supported file extensions.
*/
XAMP_METADATA_API const HashSet<std::string>& getSupportFileExtensions();

XAMP_METADATA_API void LoadCueLib();

XAMP_METADATA_NAMESPACE_END

