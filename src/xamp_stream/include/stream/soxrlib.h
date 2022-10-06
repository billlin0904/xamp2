//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/dll.h>
#include <stream/stream.h>

#include <soxr.h>

namespace xamp::stream {

class SoxrLib final {
public:
    SoxrLib();

private:  
    ModuleHandle module_;

public:
    XAMP_DECLARE_DLL(soxr_quality_spec) soxr_quality_spec;
    XAMP_DECLARE_DLL(soxr_create) soxr_create;
    XAMP_DECLARE_DLL(soxr_process) soxr_process;
    XAMP_DECLARE_DLL(soxr_delete) soxr_delete;
    XAMP_DECLARE_DLL(soxr_io_spec) soxr_io_spec;
    XAMP_DECLARE_DLL(soxr_runtime_spec) soxr_runtime_spec;
    XAMP_DECLARE_DLL(soxr_clear) soxr_clear;
};

}