//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#pragma warning(push)
#pragma warning(disable:4251)
#pragma warning(pop)

#ifdef METADATA_API_EXPORTS
    #define XAMP_METADATA_API __declspec(dllexport)
#else
    #define XAMP_METADATA_API __declspec(dllimport)
#endif

#include <filesystem>

namespace xamp::metadata {
	using namespace base;
    using RecursiveDirectoryIterator = std::filesystem::recursive_directory_iterator;
    using DirectoryIterator = std::filesystem::directory_iterator;
    using Path = std::filesystem::path;	

    class MetadataExtractAdapter;
    class MetadataReader;
    class TaglibMetadataReader;
    class TaglibMetadataWriter;
}
