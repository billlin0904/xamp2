//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <stream/pcmdsdlib.h>

#include <stdexcept>

XAMP_STREAM_NAMESPACE_BEGIN

PcmDsDLib::PcmDsDLib()
    : module_(openSharedLibrary("pcm_dsd_converter"))
    , XAMP_LOAD_DLL_API(pcm_dsd_default_config)
    , XAMP_LOAD_DLL_API(pcm_dsd_default_options)
    , XAMP_LOAD_DLL_API(pcm_dsd_create)
    , XAMP_LOAD_DLL_API(pcm_dsd_create_ex)
    , XAMP_LOAD_DLL_API(pcm_dsd_destroy)
    , XAMP_LOAD_DLL_API(pcm_dsd_process)
    , XAMP_LOAD_DLL_API(pcm_dsd_flush)
    , XAMP_LOAD_DLL_API(pcm_dsd_reset)
    , XAMP_LOAD_DLL_API(pcm_dsd_output_sample_rate)
    , XAMP_LOAD_DLL_API(pcm_dsd_status_string) {
}

XAMP_STREAM_NAMESPACE_END
