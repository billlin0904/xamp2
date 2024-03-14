//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <base/dll.h>

#include <LibALAC.h>
#include <base/logger_impl.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API AlacLib final {
public:
    AlacLib();

    XAMP_DISABLE_COPY(AlacLib)

private:
    SharedLibraryHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(InitializeEncoder);
    XAMP_DECLARE_DLL_NAME(GetMagicCookieSize);
    XAMP_DECLARE_DLL_NAME(GetMagicCookie);
    XAMP_DECLARE_DLL_NAME(Encode);
    XAMP_DECLARE_DLL_NAME(FinishEncoder);
};

inline AlacLib::AlacLib() try
    : module_(OpenSharedLibrary("LibALAC"))
    , XAMP_LOAD_DLL_API(InitializeEncoder)
    , XAMP_LOAD_DLL_API(GetMagicCookieSize)
    , XAMP_LOAD_DLL_API(GetMagicCookie)
    , XAMP_LOAD_DLL_API(Encode)
    , XAMP_LOAD_DLL_API(FinishEncoder) {
}
catch (const Exception& e) {
    XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

#define ALAC_LIB Singleton<AlacLib>::GetInstance()

XAMP_STREAM_NAMESPACE_END