//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
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

namespace xamp::metadata {
    using namespace xamp::base;

    class MetadataExtractAdapter;
    class MetadataReader;
    class TaglibMetadataReader;
    class TaglibMetadataWriter;
}
