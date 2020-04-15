//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN
#ifdef METADATA_API_EXPORTS
    #define XAMP_METADATA_API __declspec(dllexport)
#else
    #define XAMP_METADATA_API __declspec(dllimport)
#endif
#else
#define XAMP_METADATA_API
#endif

#include <filesystem>

namespace xamp::metadata {
    using namespace xamp::base;

    using RecursiveDirectoryIterator = std::filesystem::recursive_directory_iterator;
    using DirectoryIterator = std::filesystem::directory_iterator;
    using Path = std::filesystem::path;

    class MetadataExtractAdapter;
    class MetadataReader;
    class TaglibMetadataReader;
    class TaglibMetadataWriter;
}
