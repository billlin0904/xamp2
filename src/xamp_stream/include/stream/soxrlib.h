//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>

#include <base/logger.h>
#include <base/logger_impl.h>

#include <base/base.h>
#include <base/dll.h>

#include <soxr.h>

XAMP_STREAM_NAMESPACE_BEGIN

class SoxrLib final {
public:
    SoxrLib();

    XAMP_DISABLE_COPY(SoxrLib)

private:
    SharedLibraryHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(soxr_quality_spec);
    XAMP_DECLARE_DLL_NAME(soxr_create);
    XAMP_DECLARE_DLL_NAME(soxr_process);
    XAMP_DECLARE_DLL_NAME(soxr_delete);
    XAMP_DECLARE_DLL_NAME(soxr_io_spec);
    XAMP_DECLARE_DLL_NAME(soxr_runtime_spec);
    XAMP_DECLARE_DLL_NAME(soxr_clear);
};

inline SoxrLib::SoxrLib() try
    : module_(OpenSharedLibrary("soxr"))
    , XAMP_LOAD_DLL_API(soxr_quality_spec)
    , XAMP_LOAD_DLL_API(soxr_create)
    , XAMP_LOAD_DLL_API(soxr_process)
    , XAMP_LOAD_DLL_API(soxr_delete)
    , XAMP_LOAD_DLL_API(soxr_io_spec)
    , XAMP_LOAD_DLL_API(soxr_runtime_spec)
    , XAMP_LOAD_DLL_API(soxr_clear) {
}
catch (const Exception& e) {
    XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

#define LIBSRC_LIB SharedSingleton<SrcLib>::GetInstance()

XAMP_STREAM_NAMESPACE_END
