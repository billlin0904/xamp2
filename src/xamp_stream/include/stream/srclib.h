//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/dll.h>
#include <base/logger.h>

#include <stream/stream.h>
#include <libsamplerate/samplerate.h>

XAMP_STREAM_NAMESPACE_BEGIN

class SrcLib final {
public:
    SrcLib();

    XAMP_DISABLE_COPY(SrcLib)

private:
    SharedLibraryHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(src_new);
    XAMP_DECLARE_DLL_NAME(src_reset);
    XAMP_DECLARE_DLL_NAME(src_callback_new);
    XAMP_DECLARE_DLL_NAME(src_delete);
    XAMP_DECLARE_DLL_NAME(src_process);
    XAMP_DECLARE_DLL_NAME(src_is_valid_ratio);
    XAMP_DECLARE_DLL_NAME(src_get_version);
    XAMP_DECLARE_DLL_NAME(src_strerror);
};

inline SrcLib::SrcLib() try
    : module_(OpenSharedLibrary("samplerate"))
    , XAMP_LOAD_DLL_API(src_new)
    , XAMP_LOAD_DLL_API(src_reset)
    , XAMP_LOAD_DLL_API(src_callback_new)
    , XAMP_LOAD_DLL_API(src_delete)
    , XAMP_LOAD_DLL_API(src_process)
    , XAMP_LOAD_DLL_API(src_is_valid_ratio)
    , XAMP_LOAD_DLL_API(src_get_version)
	, XAMP_LOAD_DLL_API(src_strerror) {
}
catch (const Exception& e) {
    XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

XAMP_STREAM_NAMESPACE_END
