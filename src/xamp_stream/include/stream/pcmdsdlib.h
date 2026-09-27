//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <base/dll.h>
#include <base/shared_singleton.h>
#include <pcm_dsd_converter.h>

XAMP_STREAM_NAMESPACE_BEGIN

class PcmDsDLib final {
public:
    PcmDsDLib();

    XAMP_DISABLE_COPY(PcmDsDLib)

private:
    SharedLibraryHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(pcm_dsd_default_config);
    XAMP_DECLARE_DLL_NAME(pcm_dsd_default_options);
    XAMP_DECLARE_DLL_NAME(pcm_dsd_create);
    XAMP_DECLARE_DLL_NAME(pcm_dsd_create_ex);
    XAMP_DECLARE_DLL_NAME(pcm_dsd_destroy);
    XAMP_DECLARE_DLL_NAME(pcm_dsd_process);
    XAMP_DECLARE_DLL_NAME(pcm_dsd_flush);
    XAMP_DECLARE_DLL_NAME(pcm_dsd_reset);
    XAMP_DECLARE_DLL_NAME(pcm_dsd_output_sample_rate);
    XAMP_DECLARE_DLL_NAME(pcm_dsd_status_string);
};

#define LIB_PCM_DSD_LIB SharedSingleton<PcmDsDLib>::getInstance()

XAMP_STREAM_NAMESPACE_END
