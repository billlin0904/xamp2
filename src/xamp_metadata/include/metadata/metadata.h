//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
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
#elif defined(XAMP_OS_MAC)
#define XAMP_METADATA_API __attribute__((visibility("default")))
#endif

#define XAMP_METADATA_NAMESPACE_BEGIN namespace xamp::metadata {
#define XAMP_METADATA_NAMESPACE_END }

namespace xamp::metadata {
    using namespace xamp::base;

    class IMetadataExtractAdapter;
    class IMetadataReader;
    class IMetadataWriter;
}
